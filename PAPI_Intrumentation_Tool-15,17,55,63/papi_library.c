#include "papi_library.h"
#include <papi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#define STACK_SIZE 1000  // Adjust size as needed

typedef struct {
    char func_name[256];
    struct timespec start_time;
    struct timespec end_time;
    int event_set;
    long long *values;
    int func_no;
    int paused;  // Flag to indicate if the EventSet is paused
} FunctionRecord;

typedef struct {
    FunctionRecord records[STACK_SIZE];
    int top;
} Stack;

static int num_events = 0;
static FILE *output_file = NULL;
static Stack record_stack = {.top = -1}; 
static char *events = NULL; 
static int funccount = 0;

int papi_library_init(int argc, char *argv[]) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
    } else {
        perror("getcwd() error");
    }

    const char *filename = "papi_output.csv";
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-output-file=", 13) == 0) {
            filename = argv[i] + 13;
            break;
        }
    }

    output_file = fopen(filename, "w");
    if (!output_file) {
        perror("Error opening output file");
        fprintf(stderr, "Error code: %d\n", errno);
        exit(1);
    } else {
        printf("Output file '%s' opened successfully.\n", filename);
    }

    
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

    printf("Parsed PAPI events: %s\n", events);

    // Initialize PAPI library
    int papi_version = PAPI_library_init(PAPI_VER_CURRENT);
    if (papi_version != PAPI_VER_CURRENT) {
        fprintf(stderr, "PAPI library init error! Version: %d\n", papi_version);
        exit(1);
    } else {
        printf("PAPI library initialized successfully. Version: %d\n", papi_version);
    }

    char *event_str = strdup(events);
    char *event_token = strtok(event_str, ",");

    // Start building the CSV header
    char header[1024] = "function_name,start_timestamp,end_timestamp";

    while (event_token) {
        strcat(header, ",");
        strcat(header, event_token);
        printf("Prepared event for: %s\n", event_token);
        event_token = strtok(NULL, ",");
        num_events++;
    }
    free(event_str);

    // Write the header to the output file
    fprintf(output_file, "%s\n", header);

    return PAPI_VER_CURRENT;
}

void push_record(const char *func_name, const struct timespec *start) {
    if (record_stack.top < STACK_SIZE - 1) {
        record_stack.top++;
        FunctionRecord *record = &record_stack.records[record_stack.top];
        strncpy(record->func_name, func_name, sizeof(record->func_name));
        record->start_time = *start;
        record->func_no = funccount++;
        // Create an event set for this function
        record->event_set = PAPI_NULL;
        int event_set_status = PAPI_create_eventset(&record->event_set);
        if (event_set_status != PAPI_OK) {
            fprintf(stderr, "Error creating event set: %s\n", PAPI_strerror(event_set_status));
            exit(1);
        }

        // Add events to the EventSet
        char *event_copy = strdup(events); // Duplicate the events string
        char *event_token = strtok(event_copy, ",");
        while (event_token) {
            int event_code;
            int event_conversion_status = PAPI_event_name_to_code(event_token, &event_code);
            if (event_conversion_status != PAPI_OK) {
                fprintf(stderr, "Error converting event name '%s' to code: %s\n", event_token, PAPI_strerror(event_conversion_status));
                free(event_copy);
                exit(1);
            }

            int event_add_status = PAPI_add_event(record->event_set, event_code);
            if (event_add_status != PAPI_OK) {
                fprintf(stderr, "Error adding event code %d to event set: %s\n", event_code, PAPI_strerror(event_add_status));
                free(event_copy);
                exit(1);
            }
            event_token = strtok(NULL, ",");
        }
        free(event_copy);

        record->values = (long long *)malloc(num_events * sizeof(long long));
        if (!record->values) {
            perror("Error allocating memory for record values");
            exit(1);
        }

        // Start the EventSet
        int papi_start_status = PAPI_start(record->event_set);
        if (papi_start_status != PAPI_OK) {
            fprintf(stderr, "Error starting PAPI: %s\n", PAPI_strerror(papi_start_status));
            exit(1);
        }

        record->paused = 0;  // EventSet is running
    } else {
        fprintf(stderr, "Stack overflow: unable to push record\n");
        exit(1);
    }
}


int pop_record_and_write(void) {
    int func_no = -1;
    if (record_stack.top >= 0) {
        FunctionRecord *record = &record_stack.records[record_stack.top];

        if (!record->paused) {
            // Stop the EventSet only if it's running
            int papi_stop_status = PAPI_stop(record->event_set, record->values);
            if (papi_stop_status != PAPI_OK) {
                fprintf(stderr, "Error stopping PAPI: %s\n", PAPI_strerror(papi_stop_status));
                exit(1);
            }
        }

        fprintf(output_file, "%s,%ld.%09ld,%ld.%09ld",
                record->func_name, record->start_time.tv_sec, record->start_time.tv_nsec,
                record->end_time.tv_sec, record->end_time.tv_nsec);

        for (int i = 0; i < num_events; i++) {
            fprintf(output_file, ",%lld", record->values[i]);
        }
        fprintf(output_file, "\n");

        free(record->values);
        PAPI_cleanup_eventset(record->event_set);
        PAPI_destroy_eventset(&record->event_set);
        func_no = record->func_no;
        record_stack.top--;
    }
    fflush(output_file);
    return func_no;
}

void papi_function_entry(const char *func_name) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        perror("Error getting start timestamp");
        exit(1);
    }

    // Pause the current EventSet if there is one
    if (record_stack.top >= 0) {
        FunctionRecord *record = &record_stack.records[record_stack.top];
        if (!record->paused) {
            int papi_pause_status = PAPI_stop(record->event_set, record->values);
            if (papi_pause_status != PAPI_OK) {
                fprintf(stderr, "Error pausing PAPI: %s\n", PAPI_strerror(papi_pause_status));
                exit(1);
            }
            record->paused = 1;
        }
    }

    // Push record onto stack
    push_record(func_name, &ts);
    printf("%d Function entry recorded: %s at %ld.%09ld\n", funccount-1,func_name, ts.tv_sec, ts.tv_nsec);
}

void papi_function_exit(const char *func_name) {
    int func_no;
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        perror("Error getting end timestamp");
        exit(1);
    }

    // Update the latest record with end time and write it to the output file
    if (record_stack.top >= 0) {
        FunctionRecord *record = &record_stack.records[record_stack.top];
        record->end_time = ts;
        func_no = pop_record_and_write();  // Pop the record and write it to the output file
    } else {
        fprintf(stderr, "Error: No record to finalize!\n");
    }

    // Resume the previous EventSet if there is one
    if (record_stack.top >= 0) {
        FunctionRecord *record = &record_stack.records[record_stack.top];
        if (record->paused) {
            int papi_resume_status = PAPI_start(record->event_set);
            if (papi_resume_status != PAPI_OK) {
                fprintf(stderr, "Error resuming PAPI: %s\n", PAPI_strerror(papi_resume_status));
                exit(1);
            }
            record->paused = 0;
        }
    }
    printf("%d Function exit recorded: %s at %ld.%09ld\n", func_no, func_name, ts.tv_sec, ts.tv_nsec);
}

void papi_finalize(void) {
    if (output_file) {
        fclose(output_file);
        output_file = NULL;
    }
    printf("Total function calls made = %d\n",funccount);
    PAPI_shutdown();
}
