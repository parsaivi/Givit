#include "staging.h"
#include "repository.h"
#include "commit.h"
#include "utils.h"

bool is_staged(const char *filepath)
{
    FILE *fp = fopen(".givit/staging", "r");
    if (fp == NULL)
        return false;

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), fp) != NULL) {
        strip_newline(line);
        if (strcmp(line, filepath) == 0) {
            fclose(fp);
            return true;
        }
    }
    fclose(fp);
    return false;
}

bool is_tracked(const char *filepath)
{
    FILE *fp = fopen(".givit/tracks", "r");
    if (fp == NULL)
        return false;

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), fp) != NULL) {
        strip_newline(line);
        if (strcmp(line, filepath) == 0) {
            fclose(fp);
            return true;
        }
    }
    fclose(fp);
    return false;
}

static bool is_in_file(const char *list_path, const char *filepath)
{
    FILE *fp = fopen(list_path, "r");
    if (fp == NULL)
        return false;

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), fp) != NULL) {
        strip_newline(line);
        if (strcmp(line, filepath) == 0) {
            fclose(fp);
            return true;
        }
    }
    fclose(fp);
    return false;
}

void add_to_staging(const char *filepath)
{
    if (!file_exists(filepath))
        return;

    if (!is_in_file(".givit/staging", filepath)) {
        FILE *fp = fopen(".givit/staging", "a");
        if (fp != NULL) {
            fprintf(fp, "%s\n", filepath);
            fclose(fp);
        }
    }

    if (!is_in_file(".givit/havebeenstaged", filepath)) {
        FILE *fp = fopen(".givit/havebeenstaged", "a");
        if (fp != NULL) {
            fprintf(fp, "%s\n", filepath);
            fclose(fp);
        }
    }

    if (!is_in_file(".givit/tracks", filepath)) {
        FILE *fp = fopen(".givit/tracks", "a");
        if (fp != NULL) {
            fprintf(fp, "%s\n", filepath);
            fclose(fp);
        }
    }

    char staged_path[MAX_PATH_LEN];
    snprintf(staged_path, sizeof(staged_path), ".givit/staged/%s", filepath);
    ensure_staged_subdirs(staged_path);
    copy_file(filepath, staged_path);
}

void remove_from_staging(const char *filepath)
{
    FILE *fp = fopen(".givit/staging", "r");
    FILE *tmp = fopen(".givit/staging.tmp", "w");
    if (fp == NULL || tmp == NULL) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        return;
    }

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), fp) != NULL) {
        strip_newline(line);
        if (strcmp(line, filepath) != 0)
            fprintf(tmp, "%s\n", line);
    }
    fclose(fp);
    fclose(tmp);
    remove(".givit/staging");
    rename(".givit/staging.tmp", ".givit/staging");

    char staged_path[MAX_PATH_LEN];
    snprintf(staged_path, sizeof(staged_path), ".givit/staged/%s", filepath);
    remove(staged_path);
}

void add_file_or_dir(const char *filepath, const char *base_dir)
{
    char full[MAX_PATH_LEN];
    if (base_dir[strlen(base_dir) - 1] != '/')
        snprintf(full, sizeof(full), "%s/%s", base_dir, filepath);
    else
        snprintf(full, sizeof(full), "%s%s", base_dir, filepath);

    int type = is_file_or_dir(full);
    if (type == 1) {
        add_to_staging(full);
    } else if (type == 2) {
        DIR *dir = opendir(full);
        if (dir == NULL)
            return;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 ||
                strcmp(entry->d_name, GIVIT_DIR) == 0)
                continue;
            add_file_or_dir(entry->d_name, full);
        }
        closedir(dir);
    }
}

void unstage_file_or_dir(const char *filepath, const char *base_dir)
{
    char full[MAX_PATH_LEN];
    if (base_dir[strlen(base_dir) - 1] != '/')
        snprintf(full, sizeof(full), "%s/%s", base_dir, filepath);
    else
        snprintf(full, sizeof(full), "%s%s", base_dir, filepath);

    int type = is_file_or_dir(full);
    if (type == 1) {
        remove_from_staging(full);
    } else if (type == 2) {
        DIR *dir = opendir(full);
        if (dir == NULL)
            return;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 ||
                strcmp(entry->d_name, GIVIT_DIR) == 0)
                continue;
            unstage_file_or_dir(entry->d_name, full);
        }
        closedir(dir);
    }
}

void save_undo_snapshot(void)
{
    for (int i = MAX_UNDO_STEPS - 1; i >= 1; i--) {
        char src[MAX_PATH_LEN], dst[MAX_PATH_LEN];
        snprintf(src, sizeof(src), ".givit/undo/%d", i);
        snprintf(dst, sizeof(dst), ".givit/undo/%d", i + 1);
        remove_dir(dst);
        ensure_dir(dst);
        copy_dir(src, dst);
    }
    char dst1[MAX_PATH_LEN];
    snprintf(dst1, sizeof(dst1), ".givit/undo/1");
    remove_dir(dst1);
    ensure_dir(dst1);
    copy_dir(".givit/staged", dst1);
}

void restore_undo_snapshot(void)
{
    remove_dir(".givit/staged");
    ensure_dir(".givit/staged");
    copy_dir(".givit/undo/1", ".givit/staged");

    for (int i = 1; i < MAX_UNDO_STEPS; i++) {
        char src[MAX_PATH_LEN], dst[MAX_PATH_LEN];
        snprintf(dst, sizeof(dst), ".givit/undo/%d", i);
        snprintf(src, sizeof(src), ".givit/undo/%d", i + 1);
        remove_dir(dst);
        ensure_dir(dst);
        copy_dir(src, dst);
    }
    char last[MAX_PATH_LEN];
    snprintf(last, sizeof(last), ".givit/undo/%d", MAX_UNDO_STEPS);
    remove_dir(last);
    ensure_dir(last);
}

int is_changed_from_last_commit(const char *filepath)
{
    char username[MAX_NAME_LEN], email[MAX_NAME_LEN];
    char branch[MAX_NAME_LEN], parent_branch[MAX_NAME_LEN];

    if (repo_read_config(username, email, branch, parent_branch) != 0)
        return 0;

    Commit *head = commit_load_list(".givit/commitsdb");
    if (head == NULL)
        return 0;

    head = commit_fix_links(head);

    Commit *node = commit_find_by_branch(head, branch);
    if (node == NULL) {
        commit_free_list(head);
        return 0;
    }

    char committed_path[MAX_PATH_LEN];
    snprintf(committed_path, sizeof(committed_path), "%s/%s",
             node->snapshot_path, filepath);

    if (!file_exists(committed_path)) {
        commit_free_list(head);
        return 2;
    }

    bool changed = files_differ(committed_path, filepath);
    commit_free_list(head);
    return changed ? 1 : 0;
}

static void add_n_tree(const char *dir_path, int depth, int max_depth)
{
    if (depth >= max_depth)
        return;

    DIR *dir = opendir(dir_path);
    if (dir == NULL)
        return;

    struct dirent *entry;
    char entry_path[MAX_PATH_LEN];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, GIVIT_DIR) == 0)
            continue;

        for (int i = 0; i < depth; i++)
            printf("\t");

        snprintf(entry_path, sizeof(entry_path), "%s/%s", dir_path, entry->d_name);

        if (entry->d_type == DT_DIR) {
            printf("\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80 %s\n", entry->d_name);
            add_n_tree(entry_path, depth + 1, max_depth);
        } else if (entry->d_type == DT_REG) {
            printf("\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80 ");
            if (is_staged(entry_path))
                printf("\e[37m%s\n\e[m", entry->d_name);
            else
                printf("\e[31m%s\n\e[m", entry->d_name);
        }
    }
    closedir(dir);
}


int run_add(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "please specify a file\n");
        return 1;
    }

    if (strcmp(argv[2], "-n") == 0) {
        int depth = 0;
        if (argc == 4)
            depth = atoi(argv[3]);
        if (depth > 0)
            add_n_tree(".", 0, depth);
        else
            fprintf(stderr, "please enter a valid depth\n");
        return 0;
    }

    save_undo_snapshot();

    if (strcmp(argv[2], "-redo") == 0) {
        FILE *fp = fopen(".givit/havebeenstaged", "r");
        if (fp == NULL)
            return 0;
        char line[MAX_LINE_LEN];
        while (fgets(line, sizeof(line), fp) != NULL) {
            strip_newline(line);
            if (strlen(line) > 0 && !is_staged(line))
                add_to_staging(line);
        }
        fclose(fp);
        return 0;
    }

    int i = 2;
    if (strcmp(argv[2], "-f") == 0)
        i++;

    for (; i < argc; i++) {
        /* check for wildcard */
        if (strchr(argv[i], '*') != NULL) {
            glob_t g;
            if (glob(argv[i], GLOB_NOSORT, NULL, &g) == 0) {
                for (size_t j = 0; j < g.gl_pathc; j++)
                    add_file_or_dir(g.gl_pathv[j], ".");
                globfree(&g);
            }
            continue;
        }

        char full_path[MAX_PATH_LEN];
        char cwd[MAX_PATH_LEN];
        if (getcwd(cwd, sizeof(cwd)) == NULL)
            return 1;
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, argv[i]);

        if (file_exists(full_path) || dir_exists(full_path)) {
            add_file_or_dir(argv[i], ".");
        } else {
            fprintf(stderr, "file not found\n");
            return 1;
        }
    }
    return 0;
}

int run_reset(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "please specify a file\n");
        return 1;
    }

    if (strcmp(argv[2], "-undo") == 0) {
        restore_undo_snapshot();
        return 0;
    }

    int i = 2;
    if (strcmp(argv[2], "-f") == 0)
        i++;

    for (; i < argc; i++) {
        char full_path[MAX_PATH_LEN];
        char cwd[MAX_PATH_LEN];
        if (getcwd(cwd, sizeof(cwd)) == NULL)
            return 1;
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, argv[i]);

        if (file_exists(full_path) || dir_exists(full_path)) {
            unstage_file_or_dir(argv[i], ".");
        } else {
            fprintf(stderr, "file not found\n");
            return 1;
        }
    }
    return 0;
}

static void print_status_recursive(const char *filepath, int depth)
{
    int type = is_file_or_dir(filepath);

    if (type == 1) {
        char *filename = extract_filename(filepath);
        for (int i = 0; i < depth; i++)
            printf("\t");
        printf("\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80 ");

        char x = is_staged(filepath) ? '+' : '-';
        char y;

        if (!is_tracked(filepath)) {
            printf("%s %c file is not tracked!\n", filename, x);
            free(filename);
            return;
        }

        if (!file_exists(filepath)) {
            y = 'D';
        } else {
            int changed = is_changed_from_last_commit(filepath);
            if (changed == 1)
                y = 'M';
            else if (changed == 2)
                y = 'A';
            else
                y = 'A';
        }

        printf("%s %c%c\n", filename, x, y);
        free(filename);
    } else if (type == 2) {
        char *dirname = extract_filename(filepath);
        for (int i = 0; i < depth; i++)
            printf("\t");
        printf("\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80 \e[37m%s\e[m\n", dirname);
        free(dirname);

        DIR *dir = opendir(filepath);
        if (dir == NULL)
            return;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 ||
                strcmp(entry->d_name, GIVIT_DIR) == 0)
                continue;

            char child[MAX_PATH_LEN];
            snprintf(child, sizeof(child), "%s/%s", filepath, entry->d_name);
            print_status_recursive(child, depth + 1);
        }
        closedir(dir);
    }
}

int run_status(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    DIR *dir = opendir(".");
    if (dir == NULL)
        return 1;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, GIVIT_DIR) == 0)
            continue;

        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "./%s", entry->d_name);
        print_status_recursive(path, 0);
    }
    closedir(dir);

    FILE *fp = fopen(".givit/tracks", "r");
    if (fp != NULL) {
        char line[MAX_LINE_LEN];
        while (fgets(line, sizeof(line), fp) != NULL) {
            strip_newline(line);
            if (strlen(line) == 0)
                continue;
            if (!file_exists(line)) {
                char *filename = extract_filename(line);
                char x = is_staged(line) ? '+' : '-';
                printf("\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80 %s %cD\n", filename, x);
                free(filename);
            }
        }
        fclose(fp);
    }

    return 0;
}
