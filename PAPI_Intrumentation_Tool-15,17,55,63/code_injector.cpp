#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/CommandLine.h>

using namespace clang;
using namespace clang::tooling;

class FunctionInstrumenter : public RecursiveASTVisitor<FunctionInstrumenter> {
public:
    FunctionInstrumenter(Rewriter &R) : rewriter(R), hasReturnStmt(false) {}

    bool VisitFunctionDecl(FunctionDecl *FuncDecl) {
        if (FuncDecl->hasBody() && FuncDecl->isThisDeclarationADefinition()) {
            hasReturnStmt = false;  // Reset for each function
            if (FuncDecl->getNameAsString() == "main") {
                ModifyMainSignature(FuncDecl);
                AppendMainFunction(FuncDecl);
            } else {
                AppendOtherFunctions(FuncDecl);
            }

            if (!hasReturnStmt) {
                InsertExitAtEnd(FuncDecl);
            }
        }
        return true;
    }

private:
    Rewriter &rewriter;
    bool hasReturnStmt;

    void ModifyMainSignature(FunctionDecl *FuncDecl) {
        // Replace the main function signature with int main(int argc, char *argv[])
        SourceLocation DeclStart = FuncDecl->getBeginLoc();
        SourceLocation DeclEnd = FuncDecl->getBody()->getBeginLoc();
        std::string newMainSignature = "int main(int argc, char *argv[]) {";
        rewriter.ReplaceText(SourceRange(DeclStart, DeclEnd), newMainSignature);
    }

    void AppendMainFunction(FunctionDecl *FuncDecl) {
        Stmt *FuncBody = FuncDecl->getBody();
        SourceLocation FuncStart = FuncBody->getBeginLoc().getLocWithOffset(1);

        rewriter.InsertText(FuncStart, "\npapi_library_init(argc, argv);\n", true, true);

        // Traverse the function body and insert `papi_finalize` before `return 0;` in main.
        InsertFinalizeBeforeReturn Inserter(rewriter);
        Inserter.TraverseStmt(FuncBody);
    }

    void AppendOtherFunctions(FunctionDecl *FuncDecl) {
        Stmt *FuncBody = FuncDecl->getBody();
        SourceLocation FuncStart = FuncBody->getBeginLoc().getLocWithOffset(1);
        
        rewriter.InsertText(FuncStart, "\npapi_function_entry(\"" + FuncDecl->getNameAsString() + "\");\n", true, true);

        // Traverse the function body and insert `papi_function_exit` before each return statement.
        InsertExitBeforeReturn Inserter(rewriter, FuncDecl->getNameAsString(), hasReturnStmt);
        Inserter.TraverseStmt(FuncBody);
    }

    void InsertExitAtEnd(FunctionDecl *FuncDecl) {
        Stmt *FuncBody = FuncDecl->getBody();
        SourceLocation FuncEnd = FuncBody->getEndLoc().getLocWithOffset(-1);

        rewriter.InsertText(FuncEnd, "\npapi_function_exit(\"" + FuncDecl->getNameAsString() + "\");\n", true, true);
    }

    class InsertExitBeforeReturn : public RecursiveASTVisitor<InsertExitBeforeReturn> {
    public:
        InsertExitBeforeReturn(Rewriter &R, const std::string &FuncName, bool &HasReturn)
            : rewriter(R), funcName(FuncName), hasReturn(HasReturn) {}

        bool VisitReturnStmt(ReturnStmt *Return) {
            hasReturn = true;  // Mark that the function contains a return statement
            SourceLocation RetLoc = Return->getBeginLoc();
            rewriter.InsertText(RetLoc, "papi_function_exit(\"" + funcName + "\");\n", true, true);
            return true;
        }

    private:
        Rewriter &rewriter;
        std::string funcName;
        bool &hasReturn;
    };

    class InsertFinalizeBeforeReturn : public RecursiveASTVisitor<InsertFinalizeBeforeReturn> {
    public:
        InsertFinalizeBeforeReturn(Rewriter &R) : rewriter(R) {}

        bool VisitReturnStmt(ReturnStmt *Return) {
            // Check if the return value is 0, indicating 'return 0;' in main.
            if (auto RetExpr = dyn_cast<IntegerLiteral>(Return->getRetValue())) {
                if (RetExpr->getValue() == 0) {
                    SourceLocation RetLoc = Return->getBeginLoc();
                    rewriter.InsertText(RetLoc, "papi_finalize();\n", true, true);
                }
            }
            return true;
        }

    private:
        Rewriter &rewriter;
    };
};

class InstrumentASTConsumer : public ASTConsumer {
public:
    InstrumentASTConsumer(Rewriter &R) : Visitor(R) {}

    bool HandleTopLevelDecl(DeclGroupRef DR) override {
        for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
            Visitor.TraverseDecl(*b);
        return true;
    }

private:
    FunctionInstrumenter Visitor;
};

class InstrumentFrontendAction : public ASTFrontendAction {
public:
    InstrumentFrontendAction() {}

    void EndSourceFileAction() override {
        SourceManager &SM = rewriter.getSourceMgr();
        // Insert #include "papi_library.h" at the beginning of the file
        SourceLocation Start = SM.getLocForStartOfFile(SM.getMainFileID());
        rewriter.InsertText(Start, "#include \"papi_library.h\"\n", true, true);
        rewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
    }

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
        rewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
        return std::make_unique<InstrumentASTConsumer>(rewriter);
    }

private:
    Rewriter rewriter;
};

static llvm::cl::OptionCategory MyToolCategory("my-tool options");

int main(int argc, const char **argv) {
    auto expectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
    if (!expectedParser) {
        llvm::errs() << expectedParser.takeError();
        return 1;
    }
    CommonOptionsParser &OptionsParser = *expectedParser;

    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    return Tool.run(newFrontendActionFactory<InstrumentFrontendAction>().get());
}
