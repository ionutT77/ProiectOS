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
    // Extract the hunt number from the path (e.g., "hunts/hunt1" -> "1")
    const char *hunt_dir = strrchr(hunt_path, '/');
    if (!hunt_dir) hunt_dir = hunt_path; else hunt_dir++;
    const char *hunt_num = hunt_dir + 4; // skip "hunt"
    char treasures_file[256];
    snprintf(treasures_file, sizeof(treasures_file), "%s/treasures%s.dat", hunt_path, hunt_num);

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
    fflush(stdout); // <-- add this
    for (int i = 0; i < user_count; i++) {
        printf("User: %s, Score: %d\n", scores[i].username, scores[i].score);
        fflush(stdout); // <-- add this
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0); // <-- add this line
    if (argc != 2 || strcmp(argv[1], "--help") == 0) {
        printf("Usage: calculate_scores <hunt_path>\n");
        return EXIT_SUCCESS;
    }

    calculate_scores(argv[1]);
    return EXIT_SUCCESS;
}