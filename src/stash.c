#include "stash.h"
#include "utils.h"
#include "commit.h"
#include "repository.h"

static int stash_next_index(void)
{
    FILE *f = fopen(".givit/stash_list", "r");
    if (f == NULL) return 0;

    int max_index = -1;
    char temp[MAX_LINE_LEN];
    while (fgets(temp, MAX_LINE_LEN, f) != NULL) {
        int idx;
        if (sscanf(temp, "%d", &idx) == 1) {
            if (idx > max_index) max_index = idx;
        }
    }
    fclose(f);
    return max_index + 1;
}


static int stash_max_index(void)
{
    FILE *f = fopen(".givit/stash_list", "r");
    if (f == NULL) return -1;

    int max_idx = -1;
    char temp[MAX_LINE_LEN];
    while (fgets(temp, MAX_LINE_LEN, f) != NULL) {
        int idx;
        if (sscanf(temp, "%d", &idx) == 1) {
            if (idx > max_idx) max_idx = idx;
        }
    }
    fclose(f);
    return max_idx;
}

static void copy_modified_files_recursive(const char *work_dir, const char *commit_dir,
                                          const char *stash_dir)
{
    DIR *dir = opendir(work_dir);
    if (dir == NULL) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, ".givit") == 0)
            continue;

        char work_path[MAX_PATH_LEN], commit_path[MAX_PATH_LEN], stash_path[MAX_PATH_LEN];
        path_join(work_path, work_dir, entry->d_name);
        path_join(commit_path, commit_dir, entry->d_name);
        path_join(stash_path, stash_dir, entry->d_name);

        if (entry->d_type == DT_DIR) {
            if (dir_exists(commit_path)) {
                ensure_dir(stash_path);
                copy_modified_files_recursive(work_path, commit_path, stash_path);
            } else {
                ensure_dir(stash_path);
                copy_dir(work_path, stash_path);
            }
        } else {
            if (file_exists(commit_path)) {
                if (files_differ(work_path, commit_path)) {
                    copy_file(work_path, stash_path);
                }
            } else {
                copy_file(work_path, stash_path);
            }
        }
    }
    closedir(dir);
}

static void restore_from_commit(const char *snapshot_path)
{
    remove_working_files();

    DIR *dir = opendir(snapshot_path);
    if (dir == NULL) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char src[MAX_PATH_LEN], dst[MAX_PATH_LEN];
        path_join(src, snapshot_path, entry->d_name);
        snprintf(dst, MAX_PATH_LEN, "./%s", entry->d_name);

        if (entry->d_type == DT_DIR) {
            ensure_dir(dst);
            copy_dir(src, dst);
        } else {
            copy_file(src, dst);
        }
    }
    closedir(dir);
}

static void apply_stash_files(const char *stash_dir)
{
    DIR *dir = opendir(stash_dir);
    if (dir == NULL) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char src[MAX_PATH_LEN], dst[MAX_PATH_LEN];
        path_join(src, stash_dir, entry->d_name);
        snprintf(dst, MAX_PATH_LEN, "./%s", entry->d_name);

        if (entry->d_type == DT_DIR) {
            ensure_dir(dst);
            apply_stash_files(src);
        } else {
            copy_file(src, dst);
        }
    }
    closedir(dir);
}

static void reindex_stashes(void)
{
    FILE *f = fopen(".givit/stash_list", "r");
    if (f == NULL) return;

    char lines[256][MAX_LINE_LEN];
    int count = 0;
    char temp[MAX_LINE_LEN];
    while (fgets(temp, MAX_LINE_LEN, f) != NULL && count < 256) {
        strip_newline(temp);
        if (strlen(temp) > 0) {
            strcpy(lines[count], temp);
            count++;
        }
    }
    fclose(f);

    f = fopen(".givit/stash_list", "w");
    for (int i = 0; i < count; i++) {
        int old_idx;
        char branch[MAX_NAME_LEN], msg[MAX_MSG_LEN];
        long ts;
        sscanf(lines[i], "%d \"%[^\"]\" \"%[^\"]\" %ld", &old_idx, branch, msg, &ts);

        char old_dir[MAX_PATH_LEN], new_dir[MAX_PATH_LEN];
        snprintf(old_dir, MAX_PATH_LEN, ".givit/stashes/%d", old_idx);
        snprintf(new_dir, MAX_PATH_LEN, ".givit/stashes/%d", i);

        if (old_idx != i && dir_exists(old_dir)) {
            ensure_dir(new_dir);
            copy_dir(old_dir, new_dir);
            remove_dir(old_dir);
        }

        fprintf(f, "%d \"%s\" \"%s\" %ld\n", i, branch, msg, ts);
    }
    fclose(f);
}

static int stash_push(int argc, char *argv[])
{
    char username[MAX_NAME_LEN], email[MAX_NAME_LEN];
    char branch[MAX_NAME_LEN], parent_branch[MAX_NAME_LEN];
    repo_read_config(username, email, branch, parent_branch);

    char message[MAX_MSG_LEN] = "WIP";
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            strncpy(message, argv[i + 1], MAX_MSG_LEN - 1);
            message[MAX_MSG_LEN - 1] = '\0';
            i++;
        }
    }

    Commit *head = commit_load_list(".givit/commitsdb");
    if (head == NULL) {
        perror("no commits found");
        return 1;
    }
    head = commit_fix_links(head);

    Commit *latest = commit_find_by_branch(head, branch);
    if (latest == NULL) {
        perror("no commit on current branch");
        commit_free_list(head);
        return 1;
    }

    int idx = stash_next_index();
    char stash_dir[MAX_PATH_LEN];
    snprintf(stash_dir, MAX_PATH_LEN, ".givit/stashes/%d", idx);
    ensure_dir(".givit/stashes");
    ensure_dir(stash_dir);

    copy_modified_files_recursive(".", latest->snapshot_path, stash_dir);

    FILE *f = fopen(".givit/stash_list", "a");
    time_t now = time(NULL);
    fprintf(f, "%d \"%s\" \"%s\" %ld\n", idx, branch, message, (long)now);
    fclose(f);

    restore_from_commit(latest->snapshot_path);
    commit_free_list(head);
    return 0;
}

static int stash_list(void)
{
    FILE *f = fopen(".givit/stash_list", "r");
    if (f == NULL) {
        return 0;
    }

    char temp[MAX_LINE_LEN];
    while (fgets(temp, MAX_LINE_LEN, f) != NULL) {
        strip_newline(temp);
        if (strlen(temp) == 0) continue;

        int idx;
        char branch[MAX_NAME_LEN], msg[MAX_MSG_LEN];
        long ts;
        sscanf(temp, "%d \"%[^\"]\" \"%[^\"]\" %ld", &idx, branch, msg, &ts);
        printf("stash@{%d}: On %s: %s\n", idx, branch, msg);
    }
    fclose(f);
    return 0;
}

static void show_stash_diff_recursive(const char *stash_dir, const char *base_dir)
{
    DIR *dir = opendir(stash_dir);
    if (dir == NULL) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char stash_path[MAX_PATH_LEN], base_path[MAX_PATH_LEN];
        path_join(stash_path, stash_dir, entry->d_name);
        path_join(base_path, base_dir, entry->d_name);

        if (entry->d_type == DT_DIR) {
            show_stash_diff_recursive(stash_path, base_path);
        } else {
            if (!dir_exists(base_dir) || !file_exists(base_path)) {
                printf("New file: %s\n", entry->d_name);
            } else {
                printf("Modified: %s\n", entry->d_name);
                /* simple line-by-line diff */
                FILE *sf = fopen(stash_path, "r");
                FILE *bf = fopen(base_path, "r");
                char sl[MAX_LINE_LEN], bl[MAX_LINE_LEN];
                int ln = 0;
                while (fgets(sl, MAX_LINE_LEN, sf) != NULL &&
                       fgets(bl, MAX_LINE_LEN, bf) != NULL) {
                    ln++;
                    strip_newline(sl);
                    strip_newline(bl);
                    if (strcmp(sl, bl) != 0) {
                        printf("  line %d:\n", ln);
                        printf("    - %s\n", bl);
                        printf("    + %s\n", sl);
                    }
                }
                fclose(sf);
                fclose(bf);
            }
        }
    }
    closedir(dir);
}

static int stash_show(int index)
{
    char stash_dir[MAX_PATH_LEN];
    snprintf(stash_dir, MAX_PATH_LEN, ".givit/stashes/%d", index);

    if (!dir_exists(stash_dir)) {
        printf("stash@{%d} not found\n", index);
        return 1;
    }

    /* find the branch for this stash to get the base commit */
    FILE *f = fopen(".givit/stash_list", "r");
    if (f == NULL) {
        perror("stash list not found");
        return 1;
    }

    char branch[MAX_NAME_LEN] = "";
    char temp[MAX_LINE_LEN];
    while (fgets(temp, MAX_LINE_LEN, f) != NULL) {
        strip_newline(temp);
        int idx;
        char br[MAX_NAME_LEN], msg[MAX_MSG_LEN];
        long ts;
        sscanf(temp, "%d \"%[^\"]\" \"%[^\"]\" %ld", &idx, br, msg, &ts);
        if (idx == index) {
            strcpy(branch, br);
            break;
        }
    }
    fclose(f);

    if (strlen(branch) == 0) {
        printf("stash@{%d} not found\n", index);
        return 1;
    }

    Commit *head = commit_load_list(".givit/commitsdb");
    if (head) head = commit_fix_links(head);
    Commit *base = commit_find_by_branch(head, branch);
    if (base == NULL) {
        printf("base commit not found for branch %s\n", branch);
        commit_free_list(head);
        return 1;
    }

    show_stash_diff_recursive(stash_dir, base->snapshot_path);
    commit_free_list(head);
    return 0;
}

static int stash_pop(void)
{
    int top = stash_max_index();
    if (top < 0) {
        printf("No stash entries\n");
        return 1;
    }

    char stash_dir[MAX_PATH_LEN];
    snprintf(stash_dir, MAX_PATH_LEN, ".givit/stashes/%d", top);

    if (!dir_exists(stash_dir)) {
        printf("stash@{%d} not found\n", top);
        return 1;
    }

    apply_stash_files(stash_dir);
    remove_dir(stash_dir);

    /* remove entry from stash_list */
    FILE *f = fopen(".givit/stash_list", "r");
    FILE *tmp = fopen(".givit/stash_list_tmp", "w");
    char temp[MAX_LINE_LEN];
    while (fgets(temp, MAX_LINE_LEN, f) != NULL) {
        strip_newline(temp);
        if (strlen(temp) == 0) continue;
        int idx;
        char br[MAX_NAME_LEN], msg[MAX_MSG_LEN];
        long ts;
        sscanf(temp, "%d \"%[^\"]\" \"%[^\"]\" %ld", &idx, br, msg, &ts);
        if (idx != top) {
            fprintf(tmp, "%s\n", temp);
        }
    }
    fclose(f);
    fclose(tmp);
    remove(".givit/stash_list");
    rename(".givit/stash_list_tmp", ".givit/stash_list");

    reindex_stashes();
    return 0;
}

static int stash_clear(void)
{
    if (dir_exists(".givit/stashes")) {
        remove_dir(".givit/stashes");
    }
    ensure_dir(".givit/stashes");

    FILE *f = fopen(".givit/stash_list", "w");
    if (f) fclose(f);
    return 0;
}

static int stash_drop(int index)
{
    char stash_dir[MAX_PATH_LEN];
    snprintf(stash_dir, MAX_PATH_LEN, ".givit/stashes/%d", index);

    if (!dir_exists(stash_dir)) {
        printf("stash@{%d} not found\n", index);
        return 1;
    }

    remove_dir(stash_dir);

    FILE *f = fopen(".givit/stash_list", "r");
    FILE *tmp = fopen(".givit/stash_list_tmp", "w");
    char temp[MAX_LINE_LEN];
    while (fgets(temp, MAX_LINE_LEN, f) != NULL) {
        strip_newline(temp);
        if (strlen(temp) == 0) continue;
        int idx;
        char br[MAX_NAME_LEN], msg[MAX_MSG_LEN];
        long ts;
        sscanf(temp, "%d \"%[^\"]\" \"%[^\"]\" %ld", &idx, br, msg, &ts);
        if (idx != index) {
            fprintf(tmp, "%s\n", temp);
        }
    }
    fclose(f);
    fclose(tmp);
    remove(".givit/stash_list");
    rename(".givit/stash_list_tmp", ".givit/stash_list");

    reindex_stashes();
    return 0;
}

int run_stash(int argc, char *argv[])
{
    if (argc < 3) {
        perror("please enter a valid command");
        return 1;
    }

    if (strcmp(argv[2], "push") == 0) {
        return stash_push(argc, argv);
    }

    if (strcmp(argv[2], "list") == 0) {
        return stash_list();
    }

    if (strcmp(argv[2], "show") == 0) {
        if (argc < 4) {
            perror("please provide stash index");
            return 1;
        }
        return stash_show(atoi(argv[3]));
    }

    if (strcmp(argv[2], "pop") == 0) {
        return stash_pop();
    }

    if (strcmp(argv[2], "clear") == 0) {
        return stash_clear();
    }

    if (strcmp(argv[2], "drop") == 0) {
        if (argc >= 4) {
            return stash_drop(atoi(argv[3]));
        }
        int top = stash_max_index();
        if (top < 0) {
            printf("No stash entries\n");
            return 1;
        }
        return stash_drop(top);
    }

    perror("please enter a valid command");
    return 1;
}
