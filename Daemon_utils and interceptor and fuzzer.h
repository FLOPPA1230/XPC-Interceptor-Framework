#ifndef DAEMON_UTILS_H
#define DAEMON_UTILS_H

void enumerate_xpc_services(const char *directory);
int file_exists(const char *path);

#endif

#ifndef INTERCEPTOR_H
#define INTERCEPTOR_H

void start_interceptor(const char *target_service);

#endif

#ifndef FUZZER_H
#define FUZZER_H

void start_fuzzer(const char *target_service);

#endif