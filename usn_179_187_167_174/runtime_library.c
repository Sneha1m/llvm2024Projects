#include "runtime_library.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <papi.h>
#include <time.h>
#include <errno.h>  // For error handling
#include <unistd.h> // Include for getcwd function

// Macro for PAPI initialization and error checking
#define CHECK_PAPI_OK(func) do { \
    int retval = (func); \
    if (retval != PAPI_OK) { \
        fprintf(stderr, "PAPI error: %s\n", PAPI_strerror(retval)); \
        exit(1); \
    } \
} while(0)

static int event_set = PAPI_NULL;
static long long *values = NULL;
static int num_events = 0;
static FILE *output_file = NULL;

void runtime_library_init(int argc, char *argv[]) {
    // Debug: Print the current working directory
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
    } else {
        perror("getcwd() error");
    }

    // Open the output file and check for errors
    output_file = fopen("papi_output.csv", "w");
    if (!output_file) {
        perror("Error opening output file");
        fprintf(stderr, "Error code: %d\n", errno); // Print error code
        exit(1);
    } else {
        printf("Output file 'papi_output.csv' opened successfully.\n"); // Debugging info
    }

    // Write CSV header
    fprintf(output_file, "function_name,start_timestamp,end_timestamp");

    // Parse command-line arguments to get PAPI events
    const char *events = NULL;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-trace-papievents=", 18) == 0) {
            events = argv[i] + 18;
            break;
        }
    }
    if (!events) {
        fprintf(stderr, "Error: No PAPI events specified!\n");
        exit(1);
    }

    // Debug: Print parsed events
    printf("Parsed PAPI events: %s\n", events);

    // Initialize PAPI library
    int papi_version = PAPI_library_init(PAPI_VER_CURRENT);
    if (papi_version != PAPI_VER_CURRENT) {
        fprintf(stderr, "PAPI library init error! Version: %d\n", papi_version);
        exit(1);
    } else {
        printf("PAPI library initialized successfully. Version: %d\n", papi_version);
    }

    // Create an event set
    int event_set_status = PAPI_create_eventset(&event_set);
    if (event_set_status != PAPI_OK) {
        fprintf(stderr, "Error creating event set! PAPI Error: %s\n", PAPI_strerror(event_set_status));
        exit(1);
    } else {
        printf("Event set created successfully.\n"); // Debugging info
    }

    // Parse the events and add them to the event set
    char *event_str = strdup(events);
    char *event_token = strtok(event_str, ",");
    while (event_token) {
        int event_code;
        int event_conversion_status = PAPI_event_name_to_code(event_token, &event_code);
        if (event_conversion_status != PAPI_OK) {
            fprintf(stderr, "Error converting event name to code: %s\n", event_token);
            fprintf(stderr, "PAPI Error: %s\n", PAPI_strerror(event_conversion_status));
            exit(1);
        }
        
        int event_add_status = PAPI_add_event(event_set, event_code);
        if (event_add_status != PAPI_OK) {
            fprintf(stderr, "Error adding event to event set! Event: %s\n", event_token);
            fprintf(stderr, "PAPI Error: %s\n", PAPI_strerror(event_add_status));
            exit(1);
        }
        num_events++;
        printf("Added event: %s (Code: %d)\n", event_token, event_code); // Debugging info
        event_token = strtok(NULL, ",");
    }
    free(event_str);

    // Allocate memory for event values
    values = (long long *)malloc(num_events * sizeof(long long));
    if (!values) {
        perror("Error allocating memory for values");
        exit(1);
    } else {
        printf("Memory allocated for %d PAPI events.\n", num_events); // Debugging info
    }

    // Write the events to the header of the CSV file
    char *event_header_str = strdup(events); // Create a new copy for header
    event_token = strtok(event_header_str, ",");
    while (event_token) {
        fprintf(output_file, ",%s", event_token);
        event_token = strtok(NULL, ",");
    }
    fprintf(output_file, "\n");
    fflush(output_file); // Ensure the header is written to the file immediately
    free(event_header_str);
    printf("CSV header written successfully.\n"); // Debugging info
}

void runtime_function_entry(const char *func_name) {
    // Start counting
    int papi_start_status = PAPI_start(event_set);
    if (papi_start_status != PAPI_OK) {
        fprintf(stderr, "Error starting PAPI! PAPI Error: %s\n", PAPI_strerror(papi_start_status));
        exit(1);
    }

    // Record start timestamp
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        perror("Error getting start timestamp");
        exit(1);
    }

     // Flush after writing the start timestamp
    printf("Function entry recorded: %s at %ld.%09ld\n", func_name, ts.tv_sec, ts.tv_nsec);
    fprintf(output_file, "%s,%ld.%09ld", func_name, ts.tv_sec, ts.tv_nsec);
    fflush(output_file); // Debugging info
}

void runtime_function_exit(const char *func_name) {
    // Stop counting
    int papi_stop_status = PAPI_stop(event_set, values);
    if (papi_stop_status != PAPI_OK) {
        fprintf(stderr, "Error stopping PAPI! PAPI Error: %s\n", PAPI_strerror(papi_stop_status));
        exit(1);
    }

    // Record end timestamp
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        perror("Error getting end timestamp");
        exit(1);
    }

    // Ensure the data is written to the file immediately

    printf("Function exit recorded: %s at %ld.%09ld\n", func_name, ts.tv_sec, ts.tv_nsec);
    fprintf(output_file, ",%ld.%09ld", ts.tv_sec, ts.tv_nsec);

    // Write event values to CSV
    for (int i = 0; i < num_events; i++) {
        fprintf(output_file, ",%lld", values[i]);
    }
    fprintf(output_file, "\n");
    fflush(output_file); // // Debugging info
    printf("PAPI values recorded for %s:\n", func_name); // Debugging info
    for (int i = 0; i < num_events; i++) {
        printf("  Event %d: %lld\n", i, values[i]);
    }

    // Reset the event counters
    int papi_reset_status = PAPI_reset(event_set);
    if (papi_reset_status != PAPI_OK) {
        fprintf(stderr, "Error resetting PAPI! PAPI Error: %s\n", PAPI_strerror(papi_reset_status));
        exit(1);
    }
}

void runtime_library_finalize() {
    // Cleanup
    PAPI_shutdown();
    free(values);
    if (output_file) {
        fflush(output_file); // Ensure all data is written before closing
        fclose(output_file);
        printf("Output file 'papi_output.csv' closed successfully.\n"); // Debugging info
    }
    printf("Runtime library finalized.\n"); // Debugging info
}

