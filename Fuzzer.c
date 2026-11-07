#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xpc/xpc.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#include "../include/fuzzer.h"
#include "../include/logger.h"
#include "../include/common.h"

static uint64_t fuzz_count = 0;
static uint64_t crash_count = 0;
static pthread_mutex_t fuzz_mutex = PTHREAD_MUTEX_INITIALIZER;

static uint32_t xorshift_state = 2463534242;

uint32_t xorshift_rand() {
    uint32_t x = xorshift_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    xorshift_state = x;
    return x;
}

void generate_random_bytes(uint8_t *buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        buffer[i] = (uint8_t)(xorshift_rand() & 0xFF);
    }
}

xpc_object_t generate_fuzz_payload() {
    xpc_object_t message = xpc_dictionary_create(NULL, NULL, 0);
    
    int payload_type = xorshift_rand() % 5;
    uint32_t payload_size = xorshift_rand() % MAX_MSG_SIZE;
    
    if (payload_size < 64) payload_size = 64;
    
    uint8_t *random_data = (uint8_t *)malloc(payload_size);
    generate_random_bytes(random_data, payload_size);
    
    switch (payload_type) {
        case 0:
            xpc_dictionary_set_data(message, "FUZZ_DATA", random_data, payload_size);
            break;
        case 1:
            xpc_dictionary_set_string(message, "FUZZ_STRING", (const char *)random_data);
            break;
        case 2:
            xpc_dictionary_set_int64(message, "FUZZ_INT", (int64_t)xorshift_rand());
            break;
        case 3:
            xpc_dictionary_set_bool(message, "FUZZ_BOOL", xorshift_rand() % 2);
            break;
        case 4: {
            xpc_object_t array = xpc_array_create(NULL, 0);
            for (int i = 0; i < 100; i++) {
                xpc_array_set_int64(array, XPC_ARRAY_APPEND, (int64_t)xorshift_rand());
            }
            xpc_dictionary_set_value(message, "FUZZ_ARRAY", array);
            xpc_release(array);
            break;
        }
    }
    
    free(random_data);
    return message;
}

void *fuzz_worker_thread(void *arg) {
    const char *service_name = (const char *)arg;
    
    xpc_connection_t connection = xpc_connection_create_mach_service(service_name, NULL, 0);
    if (!connection) {
        return NULL;
    }
    
    xpc_connection_set_event_handler(connection, ^(xpc_object_t event) {
        if (xpc_get_type(event) == XPC_TYPE_ERROR) {
            __sync_fetch_and_add(&crash_count, 1);
        }
    });
    xpc_connection_resume(connection);
    
    for (uint64_t i = 0; i < FUZZ_ITERATIONS; i++) {
        xpc_object_t payload = generate_fuzz_payload();
        if (payload) {
            xpc_connection_send_message(connection, payload);
            xpc_release(payload);
        }
        
        __sync_fetch_and_add(&fuzz_count, 1);
        
        if (i % 1000 == 0) {
            usleep(1000);
        }
    }
    
    xpc_connection_cancel(connection);
    return NULL;
}

void start_fuzzer(const char *target_service) {
    printf(" Starting XPC Fuzzer for: %s\n", target_service);
    
    xorshift_state = (uint32_t)time(NULL);
    
    int num_threads = 4;
    pthread_t threads[num_threads];
    
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, fuzz_worker_thread, (void *)target_service);
    }
    
    while (1) {
        sleep(2);
        pthread_mutex_lock(&fuzz_mutex);
        printf(" Fuzzing Status: %llu iterations, %llu potential crashes\n", fuzz_count, crash_count);
        pthread_mutex_unlock(&fuzz_mutex);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
}