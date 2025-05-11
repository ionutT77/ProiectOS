#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

typedef struct {
    int id;
    char username[50];
    double latitude;
    double longitude;
    char clue[256];
    int value;
} Treasure;

void calculate_scores(const char *hunt_path) {
    char treasures_file[256];
    snprintf(treasures_file, sizeof(treasures_file), "%s/treasures%s.dat", hunt_path, hunt_path + 6);

    FILE *file = fopen(treasures_file, "rb");
    if (!file) {
        perror("Error opening treasures file");
        return;
    }

    Treasure t;
    struct {
        char username[50];
        int score;
    } scores[100];
    int user_count = 0;

    while (fread(&t, sizeof(Treasure), 1, file) == 1) {
        int found = 0;
        for (int i = 0; i < user_count; i++) {
            if (strcmp(scores[i].username, t.username) == 0) {
                scores[i].score += t.value;
                found = 1;
                break;
            }
        }
        if (!found) {
            strcpy(scores[user_count].username, t.username);
            scores[user_count].score = t.value;
            user_count++;
        }
    }

    fclose(file);

    printf("Scores for hunt %s:\n", hunt_path);
    for (int i = 0; i < user_count; i++) {
        printf("User: %s, Score: %d\n", scores[i].username, scores[i].score);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    calculate_scores(argv[1]);
    return EXIT_SUCCESS;
}