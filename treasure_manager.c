#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

#define BASE_DIR "hunts"
#define LOG_FILE "treasure_hunt_log_final.txt"
#define MAX_USERNAME 50
#define MAX_CLUE 256

// Treasure structure
typedef struct {
    int id;
    char username[MAX_USERNAME];
    double latitude;
    double longitude;
    char clue[MAX_CLUE];
    int value;
} Treasure;

// Function prototypes
void add_treasure(const char *hunt_id);
void list_treasures(const char *hunt_id);
void view_treasure(const char *hunt_id, int id);
void remove_treasure(const char *hunt_id, int id);
void remove_hunt(const char *hunt_id);
void log_action(const char *hunt_id, const char *action);

// Log function implementation
void log_action(const char *hunt_id, const char *action) {
    // Log to the global log file
    FILE *global_log_file = fopen(LOG_FILE, "a");
    if (!global_log_file) {
        perror("Error opening global log file");
        return;
    }

    // Create a hunt-specific log file path
    char hunt_log_path[256];
    snprintf(hunt_log_path, sizeof(hunt_log_path), "%s/%s/treasure_%s_log.txt", BASE_DIR, hunt_id, hunt_id);

    FILE *hunt_log_file = fopen(hunt_log_path, "a");
    if (!hunt_log_file) {
        perror("Error opening hunt-specific log file");
        fclose(global_log_file);
        return;
    }

    // Get the current time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (!t) {
        perror("Error getting local time");
        fclose(global_log_file);
        fclose(hunt_log_file);
        return;
    }

    // Format the log entry
    char log_entry[512];
    snprintf(log_entry, sizeof(log_entry), "[%04d-%02d-%02d %02d:%02d:%02d] Hunt: %s - %s\n",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec, hunt_id, action);

    // Write to the hunt-specific log file
    fputs(log_entry, hunt_log_file);

    // Write to the global log file
    fputs(log_entry, global_log_file);

    // Close the files
    fclose(hunt_log_file);
    fclose(global_log_file);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s --command hunt_id [args]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    const char *command = argv[1];
    const char *hunt_id = argv[2];
    
    if (strcmp(command, "--add") == 0) {
        add_treasure(hunt_id);
    } else if (strcmp(command, "--list") == 0) {
        list_treasures(hunt_id);
    } else if (strcmp(command, "--view") == 0 && argc == 4) {
        view_treasure(hunt_id, atoi(argv[3]));
    } else if (strcmp(command, "--remove") == 0 && argc == 4) {
        remove_treasure(hunt_id, atoi(argv[3]));
    } else if (strcmp(command, "--remove_hunt") == 0) {
        remove_hunt(hunt_id);
    } else {
        fprintf(stderr, "Invalid command.\n");
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

void add_treasure(const char *hunt_id) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s/treasures%s.dat", BASE_DIR, hunt_id, hunt_id + 4); // Extract the number from hunt_id

    // Ensure BASE_DIR exists
    umask(0);
    if (mkdir(BASE_DIR, 0755) == -1 && errno != EEXIST) {
        perror("Error creating base directory");
        return;
    }

    // Ensure hunt_id directory exists
    char hunt_path[256];
    snprintf(hunt_path, sizeof(hunt_path), "%s/%s", BASE_DIR, hunt_id);
    if (mkdir(hunt_path, 0755) == -1 && errno != EEXIST) {
        perror("Error creating hunt directory");
        return;
    }

    // Open the treasuresX.dat file for appending
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Error opening treasuresX.dat file");
        return;
    }

    Treasure t;
    printf("Enter Treasure ID: ");
    if (scanf("%d", &t.id) != 1) {
        fprintf(stderr, "Invalid input for Treasure ID.\n");
        close(fd);
        return;
    }
    printf("Enter Username: ");
    if (scanf("%s", t.username) != 1) {
        fprintf(stderr, "Invalid input for Username.\n");
        close(fd);
        return;
    }
    while (getchar() != '\n');  // Clear input buffer
    printf("Enter Clue: ");
    if (!fgets(t.clue, MAX_CLUE, stdin)) {
        perror("Error reading clue");
        close(fd);
        return;
    }
    t.clue[strcspn(t.clue, "\n")] = 0;  
    printf("Enter Latitude: ");
    if (scanf("%lf", &t.latitude) != 1) {
        fprintf(stderr, "Invalid input for Latitude.\n");
        close(fd);
        return;
    }
    printf("Enter Longitude: ");
    if (scanf("%lf", &t.longitude) != 1) {
        fprintf(stderr, "Invalid input for Longitude.\n");
        close(fd);
        return;
    }
    printf("Enter Value: ");
    if (scanf("%d", &t.value) != 1) {
        fprintf(stderr, "Invalid input for Value.\n");
        close(fd);
        return;
    }
    if (write(fd, &t, sizeof(Treasure)) != sizeof(Treasure)) {
        perror("Error writing to treasuresX.dat file");
    }
    close(fd);
    log_action(hunt_id, "Added treasure");
}

void list_treasures(const char *hunt_id) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s/treasures%s.dat", BASE_DIR, hunt_id, hunt_id + 4); // Extract the number from hunt_id

    FILE *file = fopen(path, "rb");
    if (!file) {
        perror("Error opening treasuresX.dat file");
        return;
    }

    Treasure t;
    printf("Treasures in hunt '%s':\n", hunt_id);
    int treasures_found = 0;

    while (fread(&t, sizeof(Treasure), 1, file) == 1) {
        printf("ID: %d, User: %s, Coordinates: (%.6f, %.6f), Clue: %s, Value: %d\n",
               t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
        treasures_found++;
    }

    if (treasures_found == 0) {
        printf("No treasures found in this hunt.\n");
    }

    fclose(file);
}

void view_treasure(const char *hunt_id, int id) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s/treasures%s.dat", BASE_DIR, hunt_id, hunt_id + 4); // Extract the number from hunt_id

    FILE *file = fopen(path, "rb");
    if (!file) {
        perror("Error opening treasuresX.dat file");
        return;
    }

    Treasure t;
    while (fread(&t, sizeof(Treasure), 1, file) == 1) {
        if (t.id == id) {
            printf("Treasure Found:\n");
            printf("ID: %d, User: %s, Coordinates: (%.6f, %.6f), Clue: %s, Value: %d\n",
                   t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
            fclose(file);
            return;
        }
    }

    printf("Treasure with ID %d not found in hunt '%s'.\n", id, hunt_id);
    fclose(file);
}

void remove_treasure(const char *hunt_id, int id) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s/treasures%s.dat", BASE_DIR, hunt_id, hunt_id + 4); // Extract the number from hunt_id

    char temp_path[256];
    snprintf(temp_path, sizeof(temp_path), "%.*s.tmp", (int)(sizeof(temp_path) - 5), path);

    // Open the original file for reading
    FILE *file = fopen(path, "rb");
    if (!file) {
        perror("Error opening treasuresX.dat file");
        return;
    }

    // Open the temporary file for writing
    FILE *temp_file = fopen(temp_path, "wb");
    if (!temp_file) {
        perror("Error creating temporary file");
        fclose(file);
        return;
    }

    Treasure t;
    int treasure_found = 0;

    // Read treasures from the original file and write to the temporary file
    while (fread(&t, sizeof(Treasure), 1, file) == 1) {
        if (t.id == id) {
            treasure_found = 1; // Mark that the treasure was found
            continue; // Skip writing this treasure to the temp file
        }
        if (fwrite(&t, sizeof(Treasure), 1, temp_file) != 1) {
            perror("Error writing to temporary file");
            fclose(file);
            fclose(temp_file);
            remove(temp_path); // Clean up the temporary file
            return;
        }
    }

    fclose(file);
    fclose(temp_file);

    // If the treasure was not found, remove the temporary file and return
    if (!treasure_found) {
        printf("Treasure with ID %d not found in hunt '%s'.\n", id, hunt_id);
        remove(temp_path); // Remove the temporary file
        return;
    }

    // Remove the original file before renaming the temporary file
    if (remove(path) != 0) {
        perror("Error removing original treasuresX.dat file");
        remove(temp_path); // Clean up the temporary file
        return;
    }

    // Rename the temporary file to replace the original file
    if (rename(temp_path, path) != 0) {
        perror("Error renaming temporary file");
        remove(temp_path); // Clean up the temporary file
        return;
    }

    // Log the action and notify the user
    log_action(hunt_id, "Removed treasure");
    printf("Treasure with ID %d removed from hunt '%s'.\n", id, hunt_id);
}

void remove_hunt(const char *hunt_id) {
    char hunt_path[256];
    if (snprintf(hunt_path, sizeof(hunt_path), "%s/%s", BASE_DIR, hunt_id) >= sizeof(hunt_path)) {
        fprintf(stderr, "Error: Hunt path is too long.\n");
        return;
    }

    DIR *dir = opendir(hunt_path);
    if (!dir) {
        perror("Error opening hunt directory");
        return;
    }

    struct dirent *entry;
    char file_path[256];

    // Remove all files in the directory
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue; // Skip "." and ".."
        }

        if (snprintf(file_path, sizeof(file_path), "%s/%s", hunt_path, entry->d_name) >= sizeof(file_path)) {
            fprintf(stderr, "Error: File path is too long.\n");
            closedir(dir);
            return;
        }

        if (remove(file_path) != 0) {
            perror("Error removing file");
        }
    }

    closedir(dir);

    // Remove the directory itself
    if (rmdir(hunt_path) != 0) {
        perror("Error removing hunt directory");
        return;
    }

    log_action(hunt_id, "Removed hunt");
    printf("Hunt '%s' removed successfully.\n", hunt_id);
}
