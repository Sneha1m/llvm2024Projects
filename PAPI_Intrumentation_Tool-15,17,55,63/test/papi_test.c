#include <stdio.h>
#include <stdlib.h>
#include <papi.h>

void check_papi(int retval, const char *msg) {
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI error in %s: %s (Error Code: %d)\n", msg, PAPI_strerror(retval), retval);
        exit(1);
    }
}

int main() {
    int retval;

    // Initialize the PAPI library
    retval = PAPI_library_init(PAPI_VER_CURRENT);
    if (retval != PAPI_VER_CURRENT) {
        fprintf(stderr, "PAPI library init error! Version: %d\n", retval);
        exit(1);
    } else {
        printf("PAPI library initialized successfully. Version: %d\n", retval);
    }

    // Create an event set
    int event_set = PAPI_NULL;
    retval = PAPI_create_eventset(&event_set);
    check_papi(retval, "PAPI_create_eventset");

    // Add a sample event (PAPI_TOT_CYC) to the event set
    retval = PAPI_add_event(event_set, PAPI_TOT_CYC);
    if (retval != PAPI_OK) {
        fprintf(stderr, "Error adding event to event set! Event: PAPI_TOT_CYC\n");
        check_papi(retval, "PAPI_add_event");
    } else {
        printf("Event PAPI_TOT_CYC added successfully.\n");
    }

    // Start counting
    retval = PAPI_start(event_set);
    check_papi(retval, "PAPI_start");

    // Stop counting
    long long event_values;
    retval = PAPI_stop(event_set, &event_values);
    check_papi(retval, "PAPI_stop");

    // Output the result
    printf("PAPI_TOT_CYC count: %lld\n", event_values);

    // Cleanup
    retval = PAPI_cleanup_eventset(event_set);
    check_papi(retval, "PAPI_cleanup_eventset");

    PAPI_shutdown();

    return 0;
}
