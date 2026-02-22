#include "grep.h"
#include "utils.h"
#include "commit.h"

int run_grep(int argc, char *argv[])
{
    if (argc < 6) {
        perror("please enter a valid command");
        return 1;
    }
    if (strcmp(argv[2], "-f") != 0 || strcmp(argv[4], "-p") != 0) {
        perror("please enter a valid command");
        return 1;
    }

    char file_path[MAX_PATH_LEN];
    snprintf(file_path, MAX_PATH_LEN, "./%s", argv[3]);

    char word[MAX_LINE_LEN];
    strncpy(word, argv[5], MAX_LINE_LEN - 1);
    word[MAX_LINE_LEN - 1] = '\0';

    int commit_id = -1;
    int show_line_numbers = 0;

    for (int i = 6; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            commit_id = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-n") == 0) {
            show_line_numbers = 1;
        }
    }

    if (commit_id != -1) {
        Commit *head = commit_load_list(".givit/commitsdb");
        Commit *node = commit_find_by_id(head, commit_id);
        if (node == NULL) {
            perror("commit not found");
            commit_free_list(head);
            return 1;
        }
        snprintf(file_path, MAX_PATH_LEN, "%s/%s", node->snapshot_path, argv[3]);
        commit_free_list(head);
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("file not found");
        return 1;
    }

    char line[MAX_LINE_LEN];
    int line_number = 0;
    while (fgets(line, MAX_LINE_LEN, file) != NULL) {
        line_number++;
        strip_newline(line);
        if (strstr(line, word) != NULL) {
            if (show_line_numbers) {
                printf("%d: %s\n", line_number, line);
            } else {
                printf("%s\n", line);
            }
        }
    }
    fclose(file);
    return 0;
}
