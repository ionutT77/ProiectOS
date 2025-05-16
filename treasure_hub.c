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

void print_help() {
    printf("Available commands:\n");
    printf("  start_monitor            - Start the monitor process\n");
    printf("  stop_monitor             - Stop the monitor process\n");
    printf("  list_treasures <hunt>    - List all treasures in a hunt\n");
    printf("  view_treasure <hunt> <id>- View details of a specific treasure\n");
    printf("  list_hunts               - List all hunts and their treasure counts\n");
    printf("  calculate_score          - Show scores for all users in each hunt\n");
    printf("  help                     - Show this help message\n");
    printf("  exit                     - Exit the program\n");
}

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
        } else if (strcmp(input, "help") == 0) {
            print_help();
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
