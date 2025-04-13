#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#define BASE_DIR "hunts"

void create_symbolic_links() {
    DIR *base_dir = opendir(BASE_DIR);
    if (!base_dir) {
        perror("Error opening hunts directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(base_dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct the path to the hunt directory
        char hunt_path[256];
        snprintf(hunt_path, sizeof(hunt_path), "%s/%s", BASE_DIR, entry->d_name);

        // Construct the path to the log file
        char log_file_path[256];
        snprintf(log_file_path, sizeof(log_file_path), "%s/treasure_%s_log.txt", hunt_path, entry->d_name);

        // Check if the log file exists
        if (access(log_file_path, F_OK) == 0) {
            // Construct the symbolic link name
            char symlink_name[256];
            snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", entry->d_name);

            // Create the symbolic link
            if (symlink(log_file_path, symlink_name) == -1) {
                perror("Error creating symbolic link");
            } else {
                printf("Created symbolic link: %s -> %s\n", symlink_name, log_file_path);
            }
        }
    }

    closedir(base_dir);
}

int main() {
    create_symbolic_links();
    return 0;
}