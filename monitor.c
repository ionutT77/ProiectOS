#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

// Define the Treasure struct (must match your other files)
typedef struct {
    int id;
    char username[50];
    double latitude;
    double longitude;
    char clue[256];
    int value;
} Treasure;

#define CMD_FILE "/tmp/monitor_command.txt"

volatile sig_atomic_t running = 1;
int global_write_fd; // Global variable for the write end of the pipe
char global_fifo_path[256] = {0}; // Add this at the top

void handle_command(int write_fd) {
    FILE *fp = fopen(CMD_FILE, "r");
    if (!fp) {
        perror("fopen");
        return;
    }
    char line[256];
    if (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0; // Remove newline character

        dprintf(write_fd, "Command received: %s\n", line);

        pid_t child = fork();
        if (child == 0) {
            dup2(write_fd, STDOUT_FILENO);
            close(write_fd);

            if (strncmp(line, "list_treasures", 14) == 0) {
                char *hunt = line + 15;
                execl("./treasure_manager", "./treasure_manager", "--list", hunt, NULL);
            } else if (strncmp(line, "view_treasure", 13) == 0) {
                char hunt[100];
                int id;
                if (sscanf(line + 14, "%s %d", hunt, &id) == 2) {
                    char id_str[20];
                    sprintf(id_str, "%d", id);
                    execl("./treasure_manager", "./treasure_manager", "--view", hunt, id_str, NULL);
                }
            } else if (strncmp(line, "list_hunts", 10) == 0) {
                execl("./monitor", "./monitor", "--list_hunts", global_fifo_path, NULL);
            } else if (strncmp(line, "calculate_score", 15) == 0) {
                execl("./monitor", "./monitor", "--calculate_score", global_fifo_path, NULL);
            }
            perror("execl failed");
            exit(1);
        } else if (child > 0) {
            waitpid(child, NULL, 0);
        } else {
            perror("fork");
        }
    }
    fclose(fp);
}

void sigusr1_handler(int sig) {
    handle_command(global_write_fd);
}

void sigterm_handler(int sig) {
    printf("Monitor received stop signal. Exiting...\n");
    close(global_write_fd); // Close the write end of the pipe
    running = 0;
}

int main(int argc, char *argv[]) {
    // Handle --list_hunts and --calculate_score as subcommands
    if (argc == 3 && (strcmp(argv[1], "--list_hunts") == 0 || strcmp(argv[1], "--calculate_score") == 0)) {
        strncpy(global_fifo_path, argv[2], sizeof(global_fifo_path)-1);
        global_write_fd = open(argv[2], O_WRONLY);
        if (global_write_fd == -1) {
            perror("open fifo for writing");
            return EXIT_FAILURE;
        }

        if (strcmp(argv[1], "--list_hunts") == 0) {
            DIR *dir = opendir("hunts");
            if (!dir) {
                dprintf(global_write_fd, "Error opening hunts directory\n");
                close(global_write_fd);
                return 1;
            }
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
                char treasures_file[512];
                snprintf(treasures_file, sizeof(treasures_file), "hunts/%s/treasures%s.dat", entry->d_name, entry->d_name + 4);
                FILE *file = fopen(treasures_file, "rb");
                int treasure_count = 0;
                if (file) {
                    fseek(file, 0, SEEK_END);
                    treasure_count = ftell(file) / sizeof(Treasure);
                    fclose(file);
                }
                dprintf(global_write_fd, "Hunt: %s, Total Treasures: %d\n", entry->d_name, treasure_count);
            }
            closedir(dir);
            close(global_write_fd);
            return 0;
        }

        if (strcmp(argv[1], "--calculate_score") == 0) {
            DIR *dir = opendir("hunts");
            if (!dir) {
                dprintf(global_write_fd, "Error opening hunts directory\n");
                close(global_write_fd);
                return 1;
            }
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
                char hunt_path[512];
                snprintf(hunt_path, sizeof(hunt_path), "hunts/%s", entry->d_name);
                int fd[2];
                pipe(fd);
                pid_t pid = fork();
                if (pid == 0) {
                    dup2(fd[1], STDOUT_FILENO);
                    close(fd[0]);
                    close(fd[1]);
                    execl("./calculate_scores", "./calculate_scores", hunt_path, NULL);
                    perror("execl failed");
                    exit(1);
                } else if (pid > 0) {
                    close(fd[1]);
                    char buffer[1024];
                    ssize_t n;
                    while ((n = read(fd[0], buffer, sizeof(buffer))) > 0) {
                        write(global_write_fd, buffer, n);
                    }
                    close(fd[0]);
                    waitpid(pid, NULL, 0);
                }
            }
            closedir(dir);
            close(global_write_fd);
            return 0;
        }
        // No need to continue, return after handling subcommand
        return 0;
    }

    // Normal monitor mode
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <fifo_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    strncpy(global_fifo_path, argv[1], sizeof(global_fifo_path)-1);
    global_write_fd = open(global_fifo_path, O_WRONLY);
    if (global_write_fd == -1) {
        perror("open fifo for writing");
        return EXIT_FAILURE;
    }

    printf("Monitor process started. Waiting for commands...\n");

    struct sigaction sa_usr1, sa_term;

    sa_usr1.sa_handler = sigusr1_handler;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    sa_term.sa_handler = sigterm_handler;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;
    sigaction(SIGTERM, &sa_term, NULL);

    while (running) {
        pause(); // Wait for signals
    }

    printf("Monitor process terminated.\n");
    return 0;
}