#include "hooks.h"
#include "utils.h"

static bool has_extension(const char *filepath, const char *ext)
{
    const char *dot = strrchr(filepath, '.');
    if (dot == NULL) return false;
    return strcmp(dot, ext) == 0;
}

static bool is_c_cpp(const char *filepath)
{
    return has_extension(filepath, ".c") || has_extension(filepath, ".cpp");
}

static int todo_check(const char *filepath)
{
    if (is_c_cpp(filepath)) {
        FILE *file = fopen(filepath, "r");
        if (file == NULL) return -1;
        char line[MAX_LINE_LEN];
        while (fgets(line, MAX_LINE_LEN, file) != NULL) {
            strip_newline(line);
            if (strstr(line, "//TODO") != NULL) {
                fclose(file);
                return 0; /* PASSED: TODO exists in .c/.cpp comment */
            }
        }
        fclose(file);
        return 1; /* FAILED: no //TODO found */
    } else if (has_extension(filepath, ".txt")) {
        FILE *file = fopen(filepath, "r");
        if (file == NULL) return -1;
        char line[MAX_LINE_LEN];
        while (fgets(line, MAX_LINE_LEN, file) != NULL) {
            strip_newline(line);
            if (strstr(line, "TODO") != NULL) {
                fclose(file);
                return 1; /* FAILED: TODO in .txt */
            }
        }
        fclose(file);
        return 0;
    }
    return -1;
}

static int eof_blank_space(const char *filepath)
{
    if (!is_c_cpp(filepath) && !has_extension(filepath, ".txt"))
        return -1;

    FILE *file = fopen(filepath, "r");
    if (file == NULL) return -1;

    char line[MAX_LINE_LEN];
    while (fgets(line, MAX_LINE_LEN, file) != NULL) {
        int len = strlen(line);
        /* remove newline for checking */
        int check_pos = len - 1;
        if (check_pos >= 0 && line[check_pos] == '\n') check_pos--;
        if (check_pos >= 0 && (line[check_pos] == ' ' || line[check_pos] == '\t')) {
            fclose(file);
            return 1; /* FAILED: trailing whitespace found */
        }
    }
    fclose(file);
    return 0;
}

static int format_check(const char *filepath)
{
    if (has_extension(filepath, ".c") || has_extension(filepath, ".cpp") ||
        has_extension(filepath, ".txt") || has_extension(filepath, ".mp3") ||
        has_extension(filepath, ".mp4") || has_extension(filepath, ".mkv")) {
        return 0;
    }
    return 1;
}

static int balance_braces(const char *filepath)
{
    if (!is_c_cpp(filepath))
        return -1;

    FILE *file = fopen(filepath, "r");
    if (file == NULL) return -1;

    int curly = 0, paren = 0, square = 0;
    char line[MAX_LINE_LEN];
    while (fgets(line, MAX_LINE_LEN, file) != NULL) {
        for (int i = 0; line[i] != '\0'; i++) {
            switch (line[i]) {
                case '{': curly++; break;
                case '}': curly--; break;
                case '(': paren++; break;
                case ')': paren--; break;
                case '[': square++; break;
                case ']': square--; break;
            }
        }
    }
    fclose(file);

    if (curly != 0 || paren != 0 || square != 0)
        return 1;
    return 0;
}

static int file_size_check(const char *filepath)
{
    struct stat st;
    if (stat(filepath, &st) != 0) return -1;
    if (st.st_size > 5 * 1024 * 1024)
        return 1;
    return 0;
}

static int character_limit(const char *filepath)
{
    if (!is_c_cpp(filepath))
        return -1;

    FILE *file = fopen(filepath, "r");
    if (file == NULL) return -1;

    int count = 0;
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        count++;
    }
    fclose(file);

    if (count >= 20000)
        return 1;
    return 0;
}

static int time_limit_check(const char *filepath)
{
    if (!has_extension(filepath, ".mp3") && !has_extension(filepath, ".mp4"))
        return -1;

    struct stat st;
    if (stat(filepath, &st) != 0) return -1;

    /* approximate: assume ~128kbps bitrate for audio, ~1Mbps for video */
    long size = st.st_size;
    double duration_sec;
    if (has_extension(filepath, ".mp3")) {
        duration_sec = (double)size / (128000.0 / 8.0); /* 128kbps */
    } else {
        duration_sec = (double)size / (1000000.0 / 8.0); /* 1Mbps */
    }

    if (duration_sec >= 10.0 * 60.0)
        return 1;
    return 0;
}

static int static_error_check(const char *filepath)
{
    if (!is_c_cpp(filepath))
        return -1;

    char cmd[MAX_PATH_LEN * 2];
    snprintf(cmd, sizeof(cmd),
             "gcc -fsyntax-only \"%s\" 2> /dev/null", filepath);
    int ret = system(cmd);
    if (ret != 0)
        return 1;
    return 0;
}

typedef struct {
    const char *name;
    int (*func)(const char *);
} Hook;

static Hook all_hooks[] = {
    { "todo-check",         todo_check },
    { "eof-blank-space",    eof_blank_space },
    { "format-check",       format_check },
    { "balance-braces",     balance_braces },
    { "file-size-check",    file_size_check },
    { "character-limit",    character_limit },
    { "time-limit",         time_limit_check },
    { "static-error-check", static_error_check },
};
static const int NUM_ALL_HOOKS = sizeof(all_hooks) / sizeof(all_hooks[0]);

static int find_hook_index(const char *name)
{
    for (int i = 0; i < NUM_ALL_HOOKS; i++) {
        if (strcmp(all_hooks[i].name, name) == 0)
            return i;
    }
    return -1;
}

static void print_hook_result(const char *filepath, const char *hook_name, int result)
{
    char *fname = extract_filename(filepath);
    printf("\"%s\": \"%s\" ....", fname, hook_name);
    if (result == 1) {
        printf("\e[31mFAILED\e[m\n");
    } else if (result == 0) {
        printf("\e[32mPASSED\e[m\n");
    } else {
        printf("\e[33mSKIPPED\e[m\n");
    }
    free(fname);
}

static int apply_hooks_to_file(const char *filepath)
{
    FILE *ah = fopen(".givit/applied_hooks", "r");
    if (ah == NULL) return 0;

    int failed = 0;
    char temp[MAX_LINE_LEN];
    while (fgets(temp, MAX_LINE_LEN, ah) != NULL) {
        strip_newline(temp);
        if (strlen(temp) == 0) continue;
        int idx = find_hook_index(temp);
        if (idx < 0) continue;
        int result = all_hooks[idx].func(filepath);
        print_hook_result(filepath, temp, result);
        if (result == 1) failed = 1;
    }
    fclose(ah);
    return failed;
}

static int apply_hooks_to_dir(const char *dir_path)
{
    DIR *dir = opendir(dir_path);
    if (dir == NULL) return 0;

    int failed = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[MAX_PATH_LEN];
        path_join(path, dir_path, entry->d_name);

        if (entry->d_type == DT_DIR) {
            if (apply_hooks_to_dir(path))
                failed = 1;
        } else {
            if (apply_hooks_to_file(path))
                failed = 1;
        }
    }
    closedir(dir);
    return failed;
}

int precommit_check_staged(void)
{
    FILE *ah = fopen(".givit/applied_hooks", "r");
    if (ah == NULL) return 0;
    char temp[MAX_LINE_LEN];
    bool has_hooks = false;
    while (fgets(temp, MAX_LINE_LEN, ah) != NULL) {
        strip_newline(temp);
        if (strlen(temp) > 0) { has_hooks = true; break; }
    }
    fclose(ah);
    if (!has_hooks) return 0;
    return apply_hooks_to_dir(".givit/staged");
}

int run_precommit(int argc, char *argv[])
{
    if (argc == 2) {
        /* run all applied hooks on staged files */
        int failed = apply_hooks_to_dir(".givit/staged");
        if (failed) {
            printf("You can't commit your staging area!\n");
            FILE *det = fopen(".givit/detached", "w");
            if (det) {
                fprintf(det, "0");
                fclose(det);
            }
            return 1;
        }
        return 0;
    }

    if (argc >= 4 && strcmp(argv[2], "hooks") == 0 && strcmp(argv[3], "list") == 0) {
        FILE *hooks = fopen(".givit/hooks", "r");
        if (hooks == NULL) {
            perror("hooks not found");
            return 1;
        }
        char temp[MAX_LINE_LEN];
        while (fgets(temp, MAX_LINE_LEN, hooks) != NULL) {
            strip_newline(temp);
            printf("%s\n", temp);
        }
        fclose(hooks);
        return 0;
    }

    if (argc >= 4 && strcmp(argv[2], "applied") == 0 && strcmp(argv[3], "hooks") == 0) {
        FILE *ah = fopen(".givit/applied_hooks", "r");
        if (ah == NULL) {
            perror("applied hooks not found");
            return 1;
        }
        char temp[MAX_LINE_LEN];
        while (fgets(temp, MAX_LINE_LEN, ah) != NULL) {
            strip_newline(temp);
            printf("%s\n", temp);
        }
        fclose(ah);
        return 0;
    }

    if (argc >= 5 && strcmp(argv[2], "add") == 0 && strcmp(argv[3], "hook") == 0) {
        const char *hook_id = argv[4];
        /* verify hook exists in available hooks */
        FILE *hooks = fopen(".givit/hooks", "r");
        if (hooks == NULL) {
            perror("hooks not found");
            return 1;
        }
        char temp[MAX_LINE_LEN];
        bool found = false;
        while (fgets(temp, MAX_LINE_LEN, hooks) != NULL) {
            strip_newline(temp);
            if (strcmp(temp, hook_id) == 0) {
                found = true;
                break;
            }
        }
        fclose(hooks);

        if (!found) {
            perror("hook not found");
            return 1;
        }

        FILE *ah = fopen(".givit/applied_hooks", "a");
        fprintf(ah, "%s\n", hook_id);
        fclose(ah);
        return 0;
    }

    if (argc >= 5 && strcmp(argv[2], "remove") == 0 && strcmp(argv[3], "hook") == 0) {
        const char *hook_id = argv[4];
        FILE *ah = fopen(".givit/applied_hooks", "r");
        if (ah == NULL) {
            perror("applied hooks not found");
            return 1;
        }

        bool found = false;
        char temp[MAX_LINE_LEN];
        while (fgets(temp, MAX_LINE_LEN, ah) != NULL) {
            strip_newline(temp);
            if (strcmp(temp, hook_id) == 0) {
                found = true;
                break;
            }
        }
        fclose(ah);

        if (!found) {
            perror("hook not found");
            return 1;
        }

        ah = fopen(".givit/applied_hooks", "r");
        FILE *tmp = fopen(".givit/applied_hooks_temp", "w");
        while (fgets(temp, MAX_LINE_LEN, ah) != NULL) {
            strip_newline(temp);
            if (strcmp(temp, hook_id) != 0) {
                fprintf(tmp, "%s\n", temp);
            }
        }
        fclose(ah);
        fclose(tmp);
        remove(".givit/applied_hooks");
        rename(".givit/applied_hooks_temp", ".givit/applied_hooks");
        return 0;
    }

    if (strcmp(argv[2], "-f") == 0) {
        int failed = 0;
        for (int i = 3; i < argc; i++) {
            char path[MAX_PATH_LEN];
            snprintf(path, MAX_PATH_LEN, ".givit/staged/%s", argv[i]);
            if (apply_hooks_to_file(path))
                failed = 1;
        }
        return failed ? 1 : 0;
    }

    perror("please enter a valid command");
    return 1;
}
