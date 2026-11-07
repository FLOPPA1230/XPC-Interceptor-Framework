#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stddef.h>

int init_logger(const char *filename);
void log_xpc_message(const char *tag, const char *format, ...);
void log_hex_dump(const char *tag, const void *data, size_t size);
void close_logger();
void get_logger_stats(uint64_t *total, uint64_t *dropped);

#endif