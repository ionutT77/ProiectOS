// Phase 2: monitor.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>

#define CMD_FILE "/home/ionut777/ProiectOS/monitor_commad.txt"

// Define the Treasure struct
typedef struct {
    int id;
    char username[50];
    double latitude;
    double longitude;
    char clue[256];
    int value;
} Treasure;

volatile sig_atomic_t running = 1;
volatile sig_atomic_t command_ready = 0;

void sigterm_handler(int sig) {
    running = 0;
}

void sigusr1_handler(int sig) {
    command_ready = 1;
}

void list_hunts() {
    DIR *dir = opendir("hunts");
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue; // Skip "." and ".."

        // Validate the length of entry->d_name
        if (strlen(entry->d_name) > 250) {
            fprintf(stderr, "Error: Hunt name is too long: %s\n", entry->d_name);
            continue;
        }

        char treasures_file[512]; // Increased buffer size to avoid truncation
        if (snprintf(treasures_file, sizeof(treasures_file), "hunts/%s/treasures%s.dat", entry->d_name, entry->d_name + 4) >= sizeof(treasures_file)) {
            fprintf(stderr, "Error: Treasures file path is too long.\n");
            continue;
        }

        FILE *fp = fopen(treasures_file, "rb");
        if (!fp) {
            printf("Hunt: %s (No treasures file)\n", entry->d_name);
            continue;
        }

        int treasure_count = 0;
        fseek(fp, 0, SEEK_END);
        treasure_count = ftell(fp) / sizeof(Treasure); // Assuming Treasure struct is used
        fclose(fp);

        printf("Hunt: %s (Treasures: %d)\n", entry->d_name, treasure_count);
    }

    closedir(dir);
}

void process_command(const char *cmd) {
    if (strcmp(cmd, "list_hunts") == 0) {
        list_hunts();
    } else if (strncmp(cmd, "list_treasures", 14) == 0) {
        printf("Listing treasures for hunt...\n");
        // Add logic to list treasures in a hunt
    } else if (strncmp(cmd, "view_treasure", 13) == 0) {
        printf("Viewing treasure details...\n");
        // Add logic to view a specific treasure
    } else {
        printf("Unknown command: %s\n", cmd);
    }
}

void write_command(const char *cmd) {
    FILE *fp = fopen(CMD_FILE, "w");
    if (!fp) {
        perror("fopen");
        return;
    }
    printf("Writing command to file: %s\n", cmd); // Debug print
    fprintf(fp, "%s\n", cmd);
    fclose(fp);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    printf("Monitor started.\n");

    while (running) {
        if (command_ready) {
            command_ready = 0;

            FILE *fp = fopen(CMD_FILE, "r");
            if (fp) {
                char cmd[256];
                if (fgets(cmd, sizeof(cmd), fp)) {
                    cmd[strcspn(cmd, "\n")] = 0; // Remove newline
                    process_command(cmd);
                }
                fclose(fp);
            }
        }
        sleep(1); // Optional: Polling fallback
    }

    printf("Monitor terminated.\n");
    return 0;
}
