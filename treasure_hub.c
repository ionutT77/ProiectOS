// Phase 2: monitor.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define CMD_FILE "/tmp/monitor_command.txt"
#define FIFO_PATH "/tmp/monitor_output_fifo"

// Define the Treasure struct
typedef struct {
    int id;
    char username[50];
    double latitude;
    double longitude;
    char clue[256];
    int value;
} Treasure;

pid_t monitor_pid = -1; // PID of the monitor process
int monitor_running = 0; // Flag to track if the monitor is running

void write_command(const char *cmd) {
    FILE *fp = fopen(CMD_FILE, "w");
    if (!fp) {
        perror("fopen");
        return;
    }
    fprintf(fp, "%s\n", cmd);
    fclose(fp);
}

void start_monitor() {
    if (monitor_running) {
        printf("Monitor is already running.\n");
        return;
    }

    monitor_pid = fork();
    if (monitor_pid == 0) {
        // In child process, start the monitor
        execl("./monitor", "./monitor", NULL);
        perror("execl failed");
        exit(1);
    } else if (monitor_pid > 0) {
        // In parent process
        monitor_running = 1;
        printf("Monitor started with PID %d.\n", monitor_pid);
    } else {
        perror("fork failed");
    }
}

void start_monitor_with_pipe(int pipe_fd[2]) {
    if (monitor_running) {
        printf("Monitor is already running.\n");
        return;
    }

    monitor_pid = fork();
    if (monitor_pid == 0) {
        // In child process
        close(pipe_fd[0]); // Close read end
        char write_fd_str[10];
        snprintf(write_fd_str, sizeof(write_fd_str), "%d", pipe_fd[1]); // Convert write_fd to string
        execl("./monitor", "./monitor", write_fd_str, NULL);
        perror("execl failed");
        exit(1);
    } else if (monitor_pid > 0) {
        // In parent process
        close(pipe_fd[1]); // Close write end in parent
        monitor_running = 1;
        printf("Monitor started with PID %d.\n", monitor_pid);
    } else {
        perror("fork failed");
    }
}

void start_monitor_with_fifo() {
    if (monitor_running) {
        printf("Monitor is already running.\n");
        return;
    }

    monitor_pid = fork();
    if (monitor_pid == 0) {
        // In child process
        execl("./monitor", "./monitor", FIFO_PATH, NULL);
        perror("execl failed");
        exit(1);
    } else if (monitor_pid > 0) {
        monitor_running = 1;
        printf("Monitor started with PID %d.\n", monitor_pid);
    } else {
        perror("fork failed");
    }
}

void stop_monitor() {
    if (!monitor_running) {
        printf("Monitor is not running.\n");
        return;
    }

    kill(monitor_pid, SIGTERM); // Send SIGTERM to the monitor
    printf("Sent stop signal to monitor. Waiting for it to terminate...\n");

    int status;
    waitpid(monitor_pid, &status, 0); // Wait for the monitor to terminate
    if (WIFEXITED(status)) {
        printf("Monitor terminated with exit code %d.\n", WEXITSTATUS(status));
    } else {
        printf("Monitor terminated abnormally.\n");
    }

    monitor_running = 0;
    monitor_pid = -1;
}

void send_command(const char *cmd) {
    if (!monitor_running) {
        printf("Error: Monitor is not running.\n");
        return;
    }

    write_command(cmd);
    kill(monitor_pid, SIGUSR1); // Notify the monitor
}

void read_monitor_output(int read_fd) {
    char buffer[1024];
    ssize_t n;
    // Read whatever is available (you may want to improve this for your use case)
    while ((n = read(read_fd, buffer, sizeof(buffer)-1)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
        // Optionally break if you expect only one message per command
        break;
    }
}

void handle_exit() {
    if (monitor_running) {
        printf("Error: Monitor is still running. Stop it first using stop_monitor.\n");
    } else {
        printf("Exiting treasure_hub.\n");
        exit(0);
    }
}

/* void list_hunts() {
    DIR *dir = opendir("hunts");
    if (!dir) {
        perror("Error opening hunts directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct the path to the treasures file
        char treasures_file[512]; // Increase buffer size to avoid truncation
        if (snprintf(treasures_file, sizeof(treasures_file), "hunts/%s/treasures%s.dat", entry->d_name, entry->d_name + 4) >= sizeof(treasures_file)) {
            fprintf(stderr, "Error: Path to treasures file is too long.\n");
            continue;
        }

        // Open the treasures file and count the treasures
        FILE *file = fopen(treasures_file, "rb");
        if (!file) {
            perror("Error opening treasures file");
            continue;
        }

        int treasure_count = 0;
        fseek(file, 0, SEEK_END);
        treasure_count = ftell(file) / sizeof(Treasure);
        fclose(file);

        // Print the hunt name and treasure count
        printf("Hunt: %s, Total Treasures: %d\n", entry->d_name, treasure_count);
    }

    closedir(dir);
}

void calculate_score() {
    DIR *dir = opendir("hunts");
    if (!dir) {
        perror("Error opening hunts directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Ensure the constructed path fits within the buffer
        char hunt_path[512]; // Increase buffer size to avoid truncation
        if (snprintf(hunt_path, sizeof(hunt_path), "hunts/%s", entry->d_name) >= sizeof(hunt_path)) {
            fprintf(stderr, "Error: Hunt path is too long.\n");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            // In child process
            execl("./calculate_scores", "./calculate_scores", hunt_path, NULL);
            perror("execl failed");
            exit(1);
        } else if (pid > 0) {
            // In parent process, wait for the child
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                printf("Scores for hunt %s calculated successfully.\n", entry->d_name);
            } else {
                printf("Error calculating scores for hunt %s.\n", entry->d_name);
            }
        } else {
            perror("fork failed");
        }
    }

    closedir(dir);
}
 */
int main() {
    char input[256];

    // Create the FIFO if it doesn't exist
    if (mkfifo(FIFO_PATH, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo failed");
        return EXIT_FAILURE;
    }

    printf("FIFO created at %s\n", FIFO_PATH);
    printf("Welcome to treasure_hub. Type a command:\n");
    while (1) {
        printf("> ");
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }

        input[strcspn(input, "\n")] = 0; // Remove newline character

        if (strcmp(input, "start_monitor") == 0) {
            start_monitor_with_fifo();
        } else if (
            strncmp(input, "list_treasures", 14) == 0 ||
            strncmp(input, "view_treasure", 13) == 0 ||
            strcmp(input, "list_hunts") == 0 ||
            strcmp(input, "calculate_score") == 0
        ) {
            send_command(input);
            // No need to read from FIFO here; cat will do it in the other terminal
        } else if (strcmp(input, "stop_monitor") == 0) {
            stop_monitor();
        } else if (strcmp(input, "exit") == 0) {
            handle_exit();
        } else {
            printf("Unknown command: %s\n", input);
        }
    }

    return 0;
}
