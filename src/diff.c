#include "diff.h"
#include "utils.h"
#include "commit.h"

void compare_files(const char *path1, const char *path2,
                   int begin1, int end1, int begin2, int end2)
{
    FILE *file1 = fopen(path1, "r");
    FILE *file2 = fopen(path2, "r");
    if (file1 == NULL || file2 == NULL) {
        printf("Error opening files.\n");
        if (file1) fclose(file1);
        if (file2) fclose(file2);
        return;
    }

    char *file1name = extract_filename(path1);
    char *file2name = extract_filename(path2);

    char line1[MAX_LINE_LEN];
    char line2[MAX_LINE_LEN];
    int line1_number = 0;
    int line2_number = 0;

    for (int i = 0; i < begin1 - 1; i++) {
        if (fgets(line1, MAX_LINE_LEN, file1) == NULL) break;
        line1_number++;
    }
    for (int i = 0; i < begin2 - 1; i++) {
        if (fgets(line2, MAX_LINE_LEN, file2) == NULL) break;
        line2_number++;
    }

    while (fgets(line1, MAX_LINE_LEN, file1) != NULL &&
           fgets(line2, MAX_LINE_LEN, file2) != NULL) {
        line1_number++;
        line2_number++;
        if (line1_number > end1) {
            line2_number--;
            break;
        }
        if (line2_number > end2) {
            line1_number--;
            break;
        }
        strip_newline(line1);
        strip_newline(line2);
        if (strcmp(line1, line2) != 0) {
            printf("\e[33m%s-%d\n%s\e[m\n", file1name, line1_number, line1);
            printf("\e[37m%s-%d\n%s\e[m\n", file2name, line2_number, line2);
        }
    }

    while (fgets(line1, MAX_LINE_LEN, file1) != NULL) {
        line1_number++;
        if (line1_number > end1) break;
        strip_newline(line1);
        printf("\e[33m%s-%d\n%s\e[m\n", file1name, line1_number, line1);
    }

    while (fgets(line2, MAX_LINE_LEN, file2) != NULL) {
        line2_number++;
        if (line2_number > end2) break;
        strip_newline(line2);
        printf("\e[37m%s-%d\n%s\e[m\n", file2name, line2_number, line2);
    }

    fclose(file1);
    fclose(file2);
    free(file1name);
    free(file2name);
}

void compare_commit_dirs(const char *dir1, const char *dir2)
{
    DIR *d1 = opendir(dir1);
    DIR *d2 = opendir(dir2);
    struct dirent *entry;

    if (d1 == NULL || d2 == NULL) {
        printf("Error opening directories.\n");
        if (d1) closedir(d1);
        if (d2) closedir(d2);
        return;
    }

    while ((entry = readdir(d1)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, ".givit") == 0)
            continue;

        char file1_path[MAX_PATH_LEN], file2_path[MAX_PATH_LEN];
        path_join(file1_path, dir1, entry->d_name);
        path_join(file2_path, dir2, entry->d_name);

        if (file_exists(file2_path)) {
            if (entry->d_type == DT_DIR) {
                compare_commit_dirs(file1_path, file2_path);
            } else {
                printf("\e[36mFile %s exists in both commit directories.\e[m\n", entry->d_name);
                compare_files(file1_path, file2_path, 1, 1000, 1, 1000);
            }
        } else {
            printf("\e[36mFile %s exists only in the first commit directory.\e[m\n", entry->d_name);
        }
    }
    closedir(d1);

    while ((entry = readdir(d2)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char file1_path[MAX_PATH_LEN];
        path_join(file1_path, dir1, entry->d_name);

        if (!file_exists(file1_path) && !dir_exists(file1_path)) {
            printf("\e[36mFile %s exists only in the second commit directory.\e[m\n", entry->d_name);
        }
    }
    closedir(d2);
}

int merge_commit_dirs(const char *dir1, const char *dir2,
                      const char *branch1, const char *branch2)
{
    DIR *d1 = opendir(dir1);
    DIR *d2 = opendir(dir2);
    struct dirent *entry;

    if (d1 == NULL || d2 == NULL) {
        printf("Error opening directories.\n");
        if (d1) closedir(d1);
        if (d2) closedir(d2);
        return 1;
    }

    while ((entry = readdir(d1)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, ".givit") == 0)
            continue;

        char file1_path[MAX_PATH_LEN], file2_path[MAX_PATH_LEN];
        path_join(file1_path, dir1, entry->d_name);
        path_join(file2_path, dir2, entry->d_name);

        if (entry->d_type == DT_DIR) {
            if (dir_exists(file2_path)) {
                if (merge_commit_dirs(file1_path, file2_path, branch1, branch2)) {
                    closedir(d1);
                    closedir(d2);
                    return 1;
                }
            } else {
                char staged_path[MAX_PATH_LEN];
                char rel[MAX_PATH_LEN];
                memmove(rel, file1_path + strlen(dir1) + 1,
                        strlen(file1_path) - strlen(dir1));
                path_join(staged_path, ".givit/staged", rel);
                ensure_dir(staged_path);
                copy_dir(file1_path, staged_path);
            }
        } else {
            if (file_exists(file2_path)) {
                /* conflict resolution */
                char rel[MAX_PATH_LEN];
                memmove(rel, file1_path + strlen(dir1) + 1,
                        strlen(file1_path) - strlen(dir1));
                char staged_path[MAX_PATH_LEN];
                path_join(staged_path, ".givit/staged", rel);
                ensure_staged_subdirs(staged_path);

                FILE *f1 = fopen(file1_path, "r");
                FILE *f2 = fopen(file2_path, "r");
                FILE *out = fopen(staged_path, "w");

                char line1[MAX_LINE_LEN], line2[MAX_LINE_LEN];
                int ln1 = 0, ln2 = 0;

                while (fgets(line1, MAX_LINE_LEN, f1) != NULL &&
                       fgets(line2, MAX_LINE_LEN, f2) != NULL) {
                    ln1++;
                    ln2++;
                    strip_newline(line1);
                    strip_newline(line2);

                    while (is_empty_line(line1)) {
                        ln1++;
                        if (fgets(line1, MAX_LINE_LEN, f1) == NULL) goto done;
                        strip_newline(line1);
                    }
                    while (is_empty_line(line2)) {
                        ln2++;
                        if (fgets(line2, MAX_LINE_LEN, f2) == NULL) goto done;
                        strip_newline(line2);
                    }

                    if (strcmp(line1, line2) != 0) {
                        printf("\e[33m%s-%d:\n%s\e[m\n", branch1, ln1, line1);
                        printf("\e[37m%s-%d:\n%s\e[m\n", branch2, ln2, line2);
                        char input[MAX_LINE_LEN];
                        scanf("%s", input);
                        if (strcmp(input, "1") == 0) {
                            fprintf(out, "%s\n", line1);
                        } else if (strcmp(input, "2") == 0) {
                            fprintf(out, "%s\n", line2);
                        } else if (strcmp(input, "quit") == 0) {
                            fclose(f1);
                            fclose(f2);
                            fclose(out);
                            closedir(d1);
                            closedir(d2);
                            return 1;
                        } else {
                            fprintf(out, "%s\n", input);
                        }
                    } else {
                        fprintf(out, "%s\n", line1);
                    }
                }

                while (fgets(line1, MAX_LINE_LEN, f1) != NULL) {
                    strip_newline(line1);
                    fprintf(out, "%s\n", line1);
                }
                while (fgets(line2, MAX_LINE_LEN, f2) != NULL) {
                    strip_newline(line2);
                    fprintf(out, "%s\n", line2);
                }
done:
                fclose(f1);
                fclose(f2);
                fclose(out);
            } else {
                char rel[MAX_PATH_LEN];
                memmove(rel, file1_path + strlen(dir1) + 1,
                        strlen(file1_path) - strlen(dir1));
                char staged_path[MAX_PATH_LEN];
                path_join(staged_path, ".givit/staged", rel);
                ensure_staged_subdirs(staged_path);
                copy_file(file1_path, staged_path);
            }
        }
    }
    closedir(d1);

    while ((entry = readdir(d2)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char file1_path[MAX_PATH_LEN], file2_path[MAX_PATH_LEN];
        path_join(file1_path, dir1, entry->d_name);
        path_join(file2_path, dir2, entry->d_name);

        if (!file_exists(file1_path) && !dir_exists(file1_path)) {
            if (entry->d_type == DT_DIR) {
                char rel[MAX_PATH_LEN];
                memmove(rel, file2_path + strlen(dir2) + 1,
                        strlen(file2_path) - strlen(dir2));
                char staged_path[MAX_PATH_LEN];
                path_join(staged_path, ".givit/staged", rel);
                ensure_dir(staged_path);
                copy_dir(file2_path, staged_path);
            } else {
                char rel[MAX_PATH_LEN];
                memmove(rel, file2_path + strlen(dir2) + 1,
                        strlen(file2_path) - strlen(dir2));
                char staged_path[MAX_PATH_LEN];
                path_join(staged_path, ".givit/staged", rel);
                ensure_staged_subdirs(staged_path);
                copy_file(file2_path, staged_path);
            }
        }
    }
    closedir(d2);
    return 0;
}

int run_diff(int argc, char *argv[])
{
    if (argc < 5) {
        perror("please enter a valid command");
        return 1;
    }

    if (strcmp(argv[2], "-c") == 0) {
        Commit *head = commit_load_list(".givit/commitsdb");
        int commit1_id = atoi(argv[3]);
        int commit2_id = atoi(argv[4]);
        Commit *node1 = commit_find_by_id(head, commit1_id);
        Commit *node2 = commit_find_by_id(head, commit2_id);
        if (node1 == NULL || node2 == NULL) {
            perror("commit not found");
            commit_free_list(head);
            return 1;
        }
        compare_commit_dirs(node1->snapshot_path, node2->snapshot_path);
        commit_free_list(head);
        return 0;
    }

    if (strcmp(argv[2], "-f") != 0) {
        perror("please enter a valid command");
        return 1;
    }

    int begin1 = 1, end1 = 1000, begin2 = 1, end2 = 1000;
    char file1_path[MAX_PATH_LEN], file2_path[MAX_PATH_LEN];
    snprintf(file1_path, MAX_PATH_LEN, "./%s", argv[3]);
    snprintf(file2_path, MAX_PATH_LEN, "./%s", argv[4]);

    for (int i = 5; i < argc; i++) {
        if (strcmp(argv[i], "--line1") == 0 && i + 1 < argc) {
            sscanf(argv[i + 1], "%d-%d", &begin1, &end1);
            i++;
        } else if (strcmp(argv[i], "--line2") == 0 && i + 1 < argc) {
            sscanf(argv[i + 1], "%d-%d", &begin2, &end2);
            i++;
        }
    }

    compare_files(file1_path, file2_path, begin1, end1, begin2, end2);
    return 0;
}

int run_merge(int argc, char *argv[])
{
    if (argc < 5) {
        perror("please enter a valid command");
        return 1;
    }
    if (strcmp(argv[2], "-b") != 0) {
        perror("please enter a valid command");
        return 1;
    }

    char branch1[MAX_NAME_LEN], branch2[MAX_NAME_LEN];
    strncpy(branch1, argv[3], MAX_NAME_LEN - 1);
    branch1[MAX_NAME_LEN - 1] = '\0';
    strncpy(branch2, argv[4], MAX_NAME_LEN - 1);
    branch2[MAX_NAME_LEN - 1] = '\0';

    Commit *head = commit_load_list(".givit/commitsdb");
    if (head == NULL) {
        perror("no commits found");
        return 1;
    }
    head = commit_fix_links(head);

    Commit *node1 = commit_find_by_branch(head, branch1);
    Commit *node2 = commit_find_by_branch(head, branch2);

    if (node1 == NULL || node2 == NULL) {
        perror("branch not found");
        commit_free_list(head);
        return 1;
    }

    if (merge_commit_dirs(node1->snapshot_path, node2->snapshot_path, branch1, branch2)) {
        commit_free_list(head);
        return 1;
    }

    char message[MAX_MSG_LEN];
    snprintf(message, MAX_MSG_LEN, "merge %s with %s", branch1, branch2);

    Commit *new_head = commit_create_snapshot(message, head);
    if (new_head != NULL) {
        new_head->branch_parent_id = node1->id;
        new_head->merge_parent_id = node2->id;
        commit_save_list(new_head, ".givit/commitsdb");
        commit_free_list(new_head);
    } else {
        commit_free_list(head);
    }

    return 0;
}
