#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "../include/daemon_utils.h"

void enumerate_xpc_services(const char *directory) {
    DIR *dir;
    struct dirent *ent;
    
    if ((dir = opendir(directory)) != NULL) {
        printf("[*] Scanning: %s\n", directory);
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".plist") != NULL) {
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s/%s", directory, ent->d_name);
                if (file_exists(full_path)) {
                    printf(" Found Service: %s\n", ent->d_name);
                }
            }
        }
        closedir(dir);
    } else {
        perror("Access Denied");
    }
}

int file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}