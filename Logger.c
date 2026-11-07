#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zlib.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <stdint.h>
#include <stddef.h>

#define LOG_BUFFER_SIZE 8192
#define MAX_LOG_SIZE (10 * 1024 * 1024)
#define ARCHIVE_DIR "/tmp/xpc_nexus_archive"
#define CURRENT_LOG "/tmp/xpc_nexus.log"

static FILE *log_file = NULL;
static pthread_mutex_t logger_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint64_t total_logs = 0;
static uint64_t dropped_logs = 0;
static AES_KEY aes_key;
static unsigned char aes_iv[AES_BLOCK_SIZE];

static void generate_random_key() {
    if (RAND_bytes((unsigned char *)&aes_key, sizeof(AES_KEY)) != 1) {
        memset(&aes_key, 0xAA, sizeof(AES_KEY));
    }
    if (RAND_bytes(aes_iv, AES_BLOCK_SIZE) != 1) {
        memset(aes_iv, 0xBB, AES_BLOCK_SIZE);
    }
}

static void rotate_log() {
    struct stat st;
    if (stat(CURRENT_LOG, &st) == 0 && st.st_size > MAX_LOG_SIZE) {
        char archive_path[256];
        time_t now = time(NULL);
        snprintf(archive_path, sizeof(archive_path), "%s/archive_%ld.log", ARCHIVE_DIR, now);
        
        if (log_file) {
            fclose(log_file);
            log_file = NULL;
        }
        
        rename(CURRENT_LOG, archive_path);
        
        int fd = open(CURRENT_LOG, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd >= 0) close(fd);
        
        log_file = fopen(CURRENT_LOG, "a");
    }
}

int init_logger(const char *filename) {
    pthread_mutex_lock(&logger_mutex);
    
    mkdir(ARCHIVE_DIR, 0755);
    
    if (log_file) {
        fclose(log_file);
    }
    
    log_file = fopen(filename, "a");
    if (log_file) {
        setvbuf(log_file, NULL, _IOFBF, LOG_BUFFER_SIZE);
        generate_random_key();
    }
    
    pthread_mutex_unlock(&logger_mutex);
    return (log_file != NULL) ? 0 : -1;
}

void log_xpc_message(const char *tag, const char *format, ...) {
    pthread_mutex_lock(&logger_mutex);
    
    if (!log_file) {
        pthread_mutex_unlock(&logger_mutex);
        __sync_fetch_and_add(&dropped_logs, 1);
        return;
    }
    
    char buffer[LOG_BUFFER_SIZE];
    char time_buffer[32];
    char encrypted_buffer[LOG_BUFFER_SIZE];
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    char final_log[LOG_BUFFER_SIZE + 128];
    snprintf(final_log, sizeof(final_log), "[%s] [%s] [PID:%d] [TID:%lu] %s\n", 
             time_buffer, tag, getpid(), (unsigned long)pthread_self(), buffer);
    
    size_t len = strlen(final_log);
    size_t padded_len = (len + AES_BLOCK_SIZE - 1) & ~(AES_BLOCK_SIZE - 1);
    memset(final_log + len, 0, padded_len - len);
    
    for (size_t i = 0; i < padded_len; i += AES_BLOCK_SIZE) {
        AES_encrypt((const unsigned char *)final_log + i, (unsigned char *)encrypted_buffer + i, &aes_key);
    }
    
    fprintf(log_file, "%s\n", encrypted_buffer);
    fflush(log_file);
    
    __sync_fetch_and_add(&total_logs, 1);
    
    rotate_log();
    
    pthread_mutex_unlock(&logger_mutex);
}

void log_hex_dump(const char *tag, const void *data, size_t size) {
    if (!data || size == 0) return;
    
    const unsigned char *ptr = (const unsigned char *)data;
    char dump_buffer[LOG_BUFFER_SIZE];
    size_t offset = 0;
    
    offset += snprintf(dump_buffer + offset, sizeof(dump_buffer) - offset, "[%s] HEX DUMP (Size: %zu):\n", tag, size);
    
    for (size_t i = 0; i < size && offset < sizeof(dump_buffer) - 80; i += 16) {
        offset += snprintf(dump_buffer + offset, sizeof(dump_buffer) - offset, "  %08zx: ", i);
        for (size_t j = 0; j < 16; j++) {
            if (i + j < size) {
                offset += snprintf(dump_buffer + offset, sizeof(dump_buffer) - offset, "%02x ", ptr[i + j]);
            } else {
                offset += snprintf(dump_buffer + offset, sizeof(dump_buffer) - offset, "   ");
            }
        }
        offset += snprintf(dump_buffer + offset, sizeof(dump_buffer) - offset, " | ");
        for (size_t j = 0; j < 16; j++) {
            if (i + j < size) {
                char c = (ptr[i + j] >= ' ' && ptr[i + j] <= '~') ? ptr[i + j] : '.';
                offset += snprintf(dump_buffer + offset, sizeof(dump_buffer) - offset, "%c", c);
            }
        }
        offset += snprintf(dump_buffer + offset, sizeof(dump_buffer) - offset, "\n");
    }
    
    log_xpc_message("HEX", "%s", dump_buffer);
}

void close_logger() {
    pthread_mutex_lock(&logger_mutex);
    if (log_file) {
        fprintf(log_file, "[SYSTEM] Logger closed. Total Logs: %llu, Dropped: %llu\n", total_logs, dropped_logs);
        fclose(log_file);
        log_file = NULL;
    }
    pthread_mutex_unlock(&logger_mutex);
}

void get_logger_stats(uint64_t *total, uint64_t *dropped) {
    pthread_mutex_lock(&logger_mutex);
    if (total) *total = total_logs;
    if (dropped) *dropped = dropped_logs;
    pthread_mutex_unlock(&logger_mutex);
}