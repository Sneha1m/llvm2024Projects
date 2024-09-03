#ifndef RUNTIME_LIBRARY_H
#define RUNTIME_LIBRARY_H

void runtime_library_init(int argc, char *argv[]);
void runtime_function_entry(const char *func_name);
void runtime_function_exit(const char *func_name);
void runtime_library_finalize();

#endif

