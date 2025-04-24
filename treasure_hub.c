// Phase 2: treasure_hub.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CMD_FILE "/tmp/monitor_command.txt"

pid_t monitor_pid = -1;
int waiting_for_monitor_exit = 0;

void write_command(const char *cmd) {
    FILE *fp = fopen(CMD_FILE, "w");
    if (!fp) {
        perror("fopen");
        return;
    }
    fprintf(fp, "%s\n", cmd);
    fclose(fp);
}

void sigchld_handler(int sig) {
    int status;
    pid_t pid = waitpid(monitor_pid, &status, 0);
    if (pid > 0) {
        printf("Monitor terminated with status %d\n", WEXITSTATUS(status));
        monitor_pid = -1;
        waiting_for_monitor_exit = 0;
    }
}

void start_monitor() {
    if (monitor_pid != -1) {
        printf("Monitor already running.\n");
        return;
    }
    monitor_pid = fork();
    if (monitor_pid == 0) {
        execl("./monitor", "./monitor", NULL);
        perror("execl");
        exit(1);
    } else if (monitor_pid < 0) {
        perror("fork");
        exit(1);
    } else {
        printf("Monitor started (PID %d).\n", monitor_pid);
    }
}

void stop_monitor() {
    if (monitor_pid == -1) {
        printf("No monitor is running.\n");
        return;
    }
    kill(monitor_pid, SIGTERM);
    waiting_for_monitor_exit = 1;
}

void send_monitor_command(const char *cmd) {
    if (monitor_pid == -1) {
        printf("Monitor not running.\n");
        return;
    }
    if (waiting_for_monitor_exit) {
        printf("Waiting for monitor to terminate.\n");
        return;
    }
    write_command(cmd);
    kill(monitor_pid, SIGUSR1);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGCHLD, &sa, NULL);

    char input[256];

    while (1) {
        printf("treasure_hub> ");
        if (!fgets(input, sizeof(input), stdin)) break;

        input[strcspn(input, "\n")] = 0;

        if (strncmp(input, "start_monitor", 13) == 0) {
            start_monitor();
        } else if (strncmp(input, "stop_monitor", 12) == 0) {
            stop_monitor();
        } else if (strncmp(input, "list_hunts", 10) == 0) {
            send_monitor_command("list_hunts");
        } else if (strncmp(input, "list_treasures", 14) == 0) {
            send_monitor_command(input); // includes hunt name
        } else if (strncmp(input, "view_treasure", 13) == 0) {
            send_monitor_command(input); // includes hunt and ID
        } else if (strncmp(input, "exit", 4) == 0) {
            if (monitor_pid != -1) {
                printf("Error: Monitor is still running.\n");
            } else {
                break;
            }
        } else {
            printf("Unknown command.\n");
        }
    }

    return 0;
}
