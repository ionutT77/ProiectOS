#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMD_FILE "/tmp/monitor_command.txt"

volatile sig_atomic_t running = 1;
int global_write_fd; // Global variable for the write end of the pipe

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
            // In child process
            dup2(write_fd, STDOUT_FILENO); // Redirect stdout to the pipe
            close(write_fd); // Close the write end after redirecting

            if (strncmp(line, "list_treasures", 14) == 0) {
                char *hunt = line + 15; // Skip command and space
                execl("./treasure_manager", "./treasure_manager", "--list", hunt, NULL);
            } else if (strncmp(line, "view_treasure", 13) == 0) {
                char hunt[100];
                int id;
                if (sscanf(line + 14, "%s %d", hunt, &id) == 2) {
                    char id_str[20];
                    sprintf(id_str, "%d", id);
                    execl("./treasure_manager", "./treasure_manager", "--view", hunt, id_str, NULL);
                }
            }
            perror("execl failed");
            exit(1);
        } else if (child > 0) {
            waitpid(child, NULL, 0); // Wait for child to finish
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
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <write_fd>\n", argv[0]);
        return EXIT_FAILURE;
    }

    global_write_fd = atoi(argv[1]); // Get the write end of the pipe

    printf("Monitor received write_fd=%d\n", global_write_fd);

    struct sigaction sa_usr1, sa_term;

    sa_usr1.sa_handler = sigusr1_handler;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    sa_term.sa_handler = sigterm_handler;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;
    sigaction(SIGTERM, &sa_term, NULL);

    printf("Monitor process started. Waiting for commands...\n");
    while (running) {
        pause(); // Wait for signals
    }

    printf("Monitor process terminated.\n");
    return 0;
}