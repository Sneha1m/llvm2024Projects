#ifndef PAPI_LIBRARY_H
#define PAPI_LIBRARY_H

#ifdef __cplusplus
extern "C" {
#endif

int papi_library_init(int argc, char *argv[]);

void papi_function_entry(const char *func_name);

void papi_function_exit(const char *func_name);

void papi_finalize(void);

void check_papi_status(int retval, const char *message);

#ifdef __cplusplus
}
#endif

#endif
