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
    FunctionInstrumenter(Rewriter &R) : rewriter(R) {}

    bool VisitFunctionDecl(FunctionDecl *FuncDecl) {
        if (FuncDecl->hasBody() && FuncDecl->isThisDeclarationADefinition()) {
            if (FuncDecl->getNameAsString() == "main") {
                handleMainFunction(FuncDecl);
            } else {
                handleOtherFunctions(FuncDecl);
            }
        }
        return true;
    }

private:
    Rewriter &rewriter;

    void handleMainFunction(FunctionDecl *FuncDecl) {
        Stmt *FuncBody = FuncDecl->getBody();
        SourceLocation FuncStart = FuncBody->getBeginLoc().getLocWithOffset(1);
        SourceLocation FuncEnd = FuncBody->getEndLoc().getLocWithOffset(-10);

        rewriter.InsertText(FuncStart, "\nruntime_library_init(argc, argv);\n", true, true);
        rewriter.InsertText(FuncEnd, "\nruntime_library_finalize();\n", true, true);

        // Replace the main function signature with int main(int argc, char *argv[])
        SourceLocation FuncDeclStart = FuncDecl->getBeginLoc();
        SourceLocation FuncDeclEnd = FuncDecl->getBody()->getBeginLoc();
        std::string newMainSig = "int main(int argc, char *argv[]) {";
        rewriter.ReplaceText(SourceRange(FuncDeclStart, FuncDeclEnd), newMainSig);
    }

    void handleOtherFunctions(FunctionDecl *FuncDecl) {
        Stmt *FuncBody = FuncDecl->getBody();
        SourceLocation FuncStart = FuncBody->getBeginLoc().getLocWithOffset(1);
        SourceLocation FuncEnd = FuncBody->getEndLoc().getLocWithOffset(-1);

        rewriter.InsertText(FuncStart, "\nruntime_function_entry(\"" + FuncDecl->getNameAsString() + "\");\n", true, true);
        rewriter.InsertText(FuncEnd, "\nruntime_function_exit(\"" + FuncDecl->getNameAsString() + "\");\n", true, true);

        InsertExitBeforeReturn Inserter(rewriter, FuncDecl->getNameAsString());
        Inserter.TraverseStmt(FuncBody);
    }

    class InsertExitBeforeReturn : public RecursiveASTVisitor<InsertExitBeforeReturn> {
    public:
        InsertExitBeforeReturn(Rewriter &R, const std::string &FuncName) : rewriter(R), funcName(FuncName) {}

        bool VisitReturnStmt(ReturnStmt *Return) {
            SourceLocation RetLoc = Return->getBeginLoc();
            rewriter.InsertText(RetLoc, "runtime_function_exit(\"" + funcName + "\");\n", true, true);
            return true;
        }

    private:
        Rewriter &rewriter;
        std::string funcName;
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
        // Insert #include "runtime_library.h" at the beginning of the file
        SourceLocation Start = SM.getLocForStartOfFile(SM.getMainFileID());
        rewriter.InsertText(Start, "#include \"runtime_library.h\"\n", true, true);
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


