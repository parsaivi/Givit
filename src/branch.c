#include "commit.h"
#include "utils.h"
#include "repository.h"
#include "staging.h"

static void restore_snapshot(const char *snapshot_path)
{
    DIR *dir = opendir(snapshot_path);
    if (dir == NULL)
        return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char src[MAX_PATH_LEN], dst[MAX_PATH_LEN];
        path_join(src, snapshot_path, entry->d_name);
        path_join(dst, ".", entry->d_name);

        if (entry->d_type == DT_DIR) {
            ensure_dir(dst);
            copy_dir(src, dst);
        } else {
            copy_file(src, dst);
        }
    }
    closedir(dir);
}

static bool has_uncommitted_changes(Commit *head)
{
    char username[MAX_NAME_LEN], email[MAX_NAME_LEN];
    char current_branch[MAX_NAME_LEN], parent_branch[MAX_NAME_LEN];
    repo_read_config(username, email, current_branch, parent_branch);

    Commit *node = head;
    while (node != NULL) {
        if (strcmp(node->branch, current_branch) == 0)
            break;
        node = node->prev;
    }
    if (node == NULL)
        return false;

    /* Check if staging file has any entries */
    FILE *fp = fopen(".givit/staging", "r");
    if (fp != NULL) {
        char line[MAX_LINE_LEN];
        while (fgets(line, sizeof(line), fp) != NULL) {
            strip_newline(line);
            if (strlen(line) > 0) {
                fclose(fp);
                return true;
            }
        }
        fclose(fp);
    }

    /* Check if any tracked file differs from the last commit snapshot */
    fp = fopen(".givit/tracks", "r");
    if (fp != NULL) {
        char line[MAX_LINE_LEN];
        while (fgets(line, sizeof(line), fp) != NULL) {
            strip_newline(line);
            if (strlen(line) == 0)
                continue;

            char committed[MAX_PATH_LEN];
            path_join(committed, node->snapshot_path, line);

            if (file_exists(line) && file_exists(committed)) {
                if (files_differ(line, committed)) {
                    fclose(fp);
                    return true;
                }
            } else if (file_exists(line) != file_exists(committed)) {
                fclose(fp);
                return true;
            }
        }
        fclose(fp);
    }

    return false;
}

int run_branch(int argc, char *argv[])
{
    Commit *head = commit_load_list(".givit/commitsdb");

    if (argc == 2) {
        FILE *fp = fopen(".givit/branches", "r");
        if (fp == NULL) {
            perror("failed to open branches file");
            commit_free_list(head);
            return 1;
        }
        char line[MAX_LINE_LEN];
        while (fgets(line, sizeof(line), fp) != NULL) {
            strip_newline(line);
            char branch[MAX_NAME_LEN], parent[MAX_NAME_LEN];
            if (sscanf(line, "%s %s", branch, parent) == 2)
                printf("%s attached to: %s\n", branch, parent);
        }
        fclose(fp);
        commit_free_list(head);
        return 0;
    }

    const char *new_branch = argv[2];

    FILE *fp = fopen(".givit/branches", "r");
    if (fp == NULL) {
        perror("failed to open branches file");
        commit_free_list(head);
        return 1;
    }
    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), fp) != NULL) {
        strip_newline(line);
        char branch[MAX_NAME_LEN], parent[MAX_NAME_LEN];
        if (sscanf(line, "%s %s", branch, parent) == 2) {
            if (strcmp(branch, new_branch) == 0) {
                perror("branch already exists");
                fclose(fp);
                commit_free_list(head);
                return 1;
            }
        }
    }
    fclose(fp);

    char username[MAX_NAME_LEN], email[MAX_NAME_LEN];
    char current_branch[MAX_NAME_LEN], parent_branch[MAX_NAME_LEN];
    repo_read_config(username, email, current_branch, parent_branch);

    printf("current head branch: %s\n", current_branch);

    fp = fopen(".givit/branches", "a");
    if (fp == NULL) {
        perror("failed to open branches file");
        commit_free_list(head);
        return 1;
    }
    fprintf(fp, "%s %s\n", new_branch, current_branch);
    fclose(fp);

    Commit *node = head;
    while (node != NULL) {
        if (strcmp(node->branch, current_branch) == 0)
            break;
        node = node->prev;
    }

    if (node == NULL) {
        perror("no commit found on current branch");
        commit_free_list(head);
        return 1;
    }

    head = commit_create_for_branch(node, new_branch, current_branch, head);
    commit_save_list(head, ".givit/commitsdb");
    commit_free_list(head);
    return 0;
}

int run_checkout(int argc, char *argv[])
{
    Commit *head = commit_load_list(".givit/commitsdb");
    if (argc < 3) {
        perror("please enter a valid command");
        commit_free_list(head);
        return 1;
    }

    if (has_uncommitted_changes(head)) {
        fprintf(stderr, "error: you have uncommitted changes, please commit or stash them first\n");
        commit_free_list(head);
        return 1;
    }

    /* Try branch checkout first */
    FILE *fp = fopen(".givit/branches", "r");
    if (fp != NULL) {
        char line[MAX_LINE_LEN];
        while (fgets(line, sizeof(line), fp) != NULL) {
            strip_newline(line);
            char branch[MAX_NAME_LEN], parent[MAX_NAME_LEN];
            if (sscanf(line, "%s %s", branch, parent) == 2) {
                if (strcmp(branch, argv[2]) == 0) {
                    fclose(fp);

                    /* Update config branch line */
                    repo_write_branch(argv[2], parent);

                    /* Find last commit on this branch */
                    Commit *node = head;
                    while (node != NULL) {
                        if (strcmp(node->branch, argv[2]) == 0)
                            break;
                        node = node->prev;
                    }

                    if (node == NULL) {
                        perror("no commit found on branch");
                        commit_free_list(head);
                        return 1;
                    }

                    remove_working_files();
                    restore_snapshot(node->snapshot_path);

                    FILE *detached = fopen(".givit/detached", "w");
                    if (detached != NULL) {
                        fprintf(detached, "1");
                        fclose(detached);
                    }

                    commit_free_list(head);
                    return 0;
                }
            }
        }
        fclose(fp);
    }

    /* HEAD or HEAD-N / HEAD_N checkout */
    if (strncmp(argv[2], "HEAD", 4) == 0) {
        int offset = 0;
        if (strlen(argv[2]) > 4) {
            /* Support both HEAD-N and HEAD_N */
            offset = atoi(argv[2] + 5);
        }

        Commit *node = head;
        for (int i = 0; i < offset; i++) {
            if (node == NULL || node->prev == NULL) {
                perror("you can't checkout that many commits");
                commit_free_list(head);
                return 1;
            }
            node = node->prev;
        }

        if (node == NULL) {
            perror("no commit found");
            commit_free_list(head);
            return 1;
        }

        remove_working_files();
        restore_snapshot(node->snapshot_path);

        FILE *detached = fopen(".givit/detached", "w");
        if (detached != NULL) {
            if (offset == 0)
                fprintf(detached, "1");
            else
                fprintf(detached, "0");
            fclose(detached);
        }

        commit_free_list(head);
        return 0;
    }

    /* Commit-ID checkout (numeric) */
    int commit_id = atoi(argv[2]);
    Commit *node = commit_find_by_id(head, commit_id);
    if (node == NULL) {
        perror("commit not found");
        commit_free_list(head);
        return 1;
    }

    remove_working_files();
    restore_snapshot(node->snapshot_path);

    /* Check if this commit is the latest on its branch (detached HEAD) */
    Commit *latest = head;
    while (latest != NULL) {
        if (strcmp(latest->branch, node->branch) == 0)
            break;
        latest = latest->prev;
    }

    FILE *detached = fopen(".givit/detached", "w");
    if (detached != NULL) {
        if (latest != NULL && latest->id != node->id)
            fprintf(detached, "0");
        else
            fprintf(detached, "1");
        fclose(detached);
    }

    commit_free_list(head);
    return 0;
}

int run_revert(int argc, char *argv[])
{
    Commit *head = commit_load_list(".givit/commitsdb");
    if (argc < 3) {
        perror("please enter a valid command");
        commit_free_list(head);
        return 1;
    }

    char message[MAX_MSG_LEN];
    int commit_id = 0;
    bool no_commit = false;

    message[0] = '\0';

    if (strcmp(argv[2], "-n") == 0) {
        /* givit revert -n <commit-id> or givit revert -n HEAD-N */
        no_commit = true;
        if (argc == 3) {
            commit_id = head->id;
        } else if (strncmp(argv[3], "HEAD", 4) == 0) {
            int offset = 0;
            if (strlen(argv[3]) > 4)
                offset = atoi(argv[3] + 5);
            Commit *node = head;
            for (int i = 0; i < offset; i++) {
                if (node == NULL || node->prev == NULL) {
                    perror("you can't revert that many commits");
                    commit_free_list(head);
                    return 1;
                }
                node = node->prev;
            }
            commit_id = node->id;
        } else {
            commit_id = atoi(argv[3]);
        }
    } else if (strcmp(argv[2], "-m") == 0) {
        /* givit revert -m "message" <commit-id|HEAD-N> */
        if (argc < 5) {
            perror("please enter a valid command");
            commit_free_list(head);
            return 1;
        }
        snprintf(message, sizeof(message), "%s", argv[3]);

        if (strncmp(argv[4], "HEAD", 4) == 0) {
            int offset = 0;
            if (strlen(argv[4]) > 4)
                offset = atoi(argv[4] + 5);
            Commit *node = head;
            for (int i = 0; i < offset; i++) {
                if (node == NULL || node->prev == NULL) {
                    perror("you can't revert that many commits");
                    commit_free_list(head);
                    return 1;
                }
                node = node->prev;
            }
            commit_id = node->id;
        } else {
            commit_id = atoi(argv[4]);
        }
    } else {
        /* givit revert <commit-id> or givit revert HEAD-N */
        if (strncmp(argv[2], "HEAD", 4) == 0) {
            int offset = 0;
            if (strlen(argv[2]) > 4)
                offset = atoi(argv[2] + 5);
            Commit *node = head;
            for (int i = 0; i < offset; i++) {
                if (node == NULL || node->prev == NULL) {
                    perror("you can't revert that many commits");
                    commit_free_list(head);
                    return 1;
                }
                node = node->prev;
            }
            commit_id = node->id;
            snprintf(message, sizeof(message), "%s", node->message);
        } else {
            commit_id = atoi(argv[2]);
            Commit *node = commit_find_by_id(head, commit_id);
            if (node != NULL)
                snprintf(message, sizeof(message), "%s", node->message);
        }
    }

    /* Checkout the target commit directly */
    Commit *target = commit_find_by_id(head, commit_id);
    if (target == NULL) {
        perror("commit not found for revert");
        commit_free_list(head);
        return 1;
    }

    remove_working_files();
    restore_snapshot(target->snapshot_path);

    if (!no_commit) {
        /* Stage all working files (skip .givit) */
        DIR *dir = opendir(".");
        if (dir != NULL) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 ||
                    strcmp(entry->d_name, "..") == 0 ||
                    strcmp(entry->d_name, ".givit") == 0)
                    continue;
                add_file_or_dir(entry->d_name, ".");
            }
            closedir(dir);
        }

        /* Create a new commit */
        head = commit_create_snapshot(message, head);
        commit_save_list(head, ".givit/commitsdb");
    }

    commit_free_list(head);
    return 0;
}
