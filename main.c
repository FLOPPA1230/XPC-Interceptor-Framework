#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/common.h"
#include "../include/daemon_utils.h"
#include "../include/interceptor.h"
#include "../include/fuzzer.h"

void print_usage(const char *prog_name) {
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  --enum                 Enumerate all XPC services\n");
    printf("  --intercept <service>  Intercept XPC messages for a specific service\n");
    printf("  --fuzz <service>       Fuzz a specific XPC service\n");
    printf("  --help                 Show this help menu\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    } 
    else if (strcmp(argv[1], "--enum") == 0) {
        enumerate_xpc_services("/System/Library/LaunchDaemons");
        enumerate_xpc_services("/System/Library/LaunchAgents");
    } 
    else if (strcmp(argv[1], "--intercept") == 0) {
        if (argc < 3) {
            printf("Error: Service name required for interception.\n");
            return 1;
        }
        start_interceptor(argv[2]);
    } 
    else if (strcmp(argv[1], "--fuzz") == 0) {
        if (argc < 3) {
            printf("Error: Service name required for fuzzing.\n");
            return 1;
        }
        start_fuzzer(argv[2]);
    } 
    else {
        printf("Error: Unknown option %s\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}