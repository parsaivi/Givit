#include "repository.h"
#include "utils.h"

static int create_configs(const char *username, const char *email);

bool repo_exists(void)
{
    char cwd[MAX_PATH_LEN];
    char tmp[MAX_PATH_LEN];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return false;

    bool found = false;

    do {
        char givit_path[MAX_PATH_LEN];
        path_join(givit_path, ".", GIVIT_DIR);
        if (dir_exists(givit_path))
            found = true;

        if (getcwd(tmp, sizeof(tmp)) == NULL)
            break;

        if (strcmp(tmp, "/") != 0) {
            if (chdir("..") != 0)
                break;
        }
    } while (strcmp(tmp, "/") != 0);

    chdir(cwd);
    return found;
}

int repo_read_config(char *username, char *email, char *branch, char *parent_branch)
{
    FILE *fp = fopen(".givit/config", "r");
    if (fp == NULL)
        return 1;

    char line[MAX_LINE_LEN];

    if (fgets(line, sizeof(line), fp) == NULL) { fclose(fp); return 1; }
    strip_newline(line);
    strcpy(username, line);

    if (fgets(line, sizeof(line), fp) == NULL) { fclose(fp); return 1; }
    strip_newline(line);
    strcpy(email, line);

    if (fgets(line, sizeof(line), fp) == NULL) { fclose(fp); return 1; }
    strip_newline(line);
    if (sscanf(line, "branch: %s %s", branch, parent_branch) != 2) {
        fclose(fp);
        return 1;
    }

    fclose(fp);
    return 0;
}

int repo_write_branch(const char *branch, const char *parent_branch)
{
    FILE *fp = fopen(".givit/config", "r");
    FILE *tmp = fopen(".givit/config.tmp", "w");
    if (fp == NULL || tmp == NULL) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        return 1;
    }

    char line[MAX_LINE_LEN];
    int lineno = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        lineno++;
        if (lineno == 3) {
            fprintf(tmp, "branch: %s %s\n", branch, parent_branch);
        } else {
            strip_newline(line);
            fprintf(tmp, "%s\n", line);
        }
    }

    fclose(fp);
    fclose(tmp);
    remove(".givit/config");
    rename(".givit/config.tmp", ".givit/config");
    return 0;
}

void repo_read_global_config(char *username, char *email)
{
    const char *home = getenv("HOME");
    if (home == NULL) {
        strcpy(username, "admin");
        strcpy(email, "admin@givit.com");
        return;
    }

    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/.givitconfig", home);

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        strcpy(username, "admin");
        strcpy(email, "admin@givit.com");
        return;
    }

    char line[MAX_LINE_LEN];

    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        strcpy(username, "admin");
        strcpy(email, "admin@givit.com");
        return;
    }
    strip_newline(line);
    strcpy(username, line);

    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        strcpy(email, "admin@givit.com");
        return;
    }
    strip_newline(line);
    strcpy(email, line);

    fclose(fp);
}

int run_init(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    char cwd[MAX_PATH_LEN];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return 1;

    if (repo_exists()) {
        fprintf(stderr, "givit repository has already initialized\n");
        return 0;
    }

    if (mkdir(GIVIT_DIR, 0755) != 0)
        return 1;

    fprintf(stdout,
        "   \xe2\x96\x84\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x84"
        "   \xe2\x96\x84\xe2\x96\x88   \xe2\x96\x84\xe2\x96\x88    \xe2\x96\x88\xe2\x96\x84"
        "   \xe2\x96\x84\xe2\x96\x88      \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88     \n"
        "  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88    \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88"
        " \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88    \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88"
        " \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x80\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88"
        "\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x84 \n");

    char username[MAX_NAME_LEN];
    char email[MAX_NAME_LEN];
    repo_read_global_config(username, email);

    if (chdir(cwd) != 0)
        return 1;

    int ret = create_configs(username, email);
    if (ret != 0)
        return ret;

    const char *home = getenv("HOME");
    if (home != NULL) {
        char repos_path[MAX_PATH_LEN];
        snprintf(repos_path, sizeof(repos_path), "%s/.givit_repos", home);
        FILE *fp = fopen(repos_path, "a");
        if (fp != NULL) {
            fprintf(fp, "%s\n", cwd);
            fclose(fp);
        }
    }

    return 0;
}

static int create_configs(const char *username, const char *email)
{
    FILE *fp = fopen(".givit/config", "w");
    if (fp == NULL)
        return 1;
    fprintf(fp, "%s\n", username);
    fprintf(fp, "%s\n", email);
    fprintf(fp, "branch: master master\n");
    fprintf(fp, "last_commit_ID: 0\n");
    fclose(fp);

    if (mkdir(".givit/commits", 0755) != 0) return 1;
    if (mkdir(".givit/staged", 0755) != 0) return 1;

    if (mkdir(".givit/undo", 0755) != 0) return 1;
    for (int i = 1; i <= MAX_UNDO_STEPS; i++) {
        char dir_path[MAX_PATH_LEN];
        snprintf(dir_path, sizeof(dir_path), ".givit/undo/%d", i);
        if (mkdir(dir_path, 0755) != 0)
            return 1;
    }

    if (mkdir(".givit/files", 0755) != 0) return 1;
    if (mkdir(".givit/valid", 0777) != 0) return 1;
    if (mkdir(".givit/stashes", 0755) != 0) return 1;

    const char *empty_files[] = {
        ".givit/staging",
        ".givit/aliases",
        ".givit/havebeenstaged",
        ".givit/commitsdb",
        ".givit/tracks",
        ".givit/shortcuts",
        ".givit/tags",
        ".givit/stash_list",
        ".givit/applied_hooks",
        NULL
    };
    for (int i = 0; empty_files[i] != NULL; i++) {
        fp = fopen(empty_files[i], "w");
        if (fp == NULL)
            return 1;
        fclose(fp);
    }

    fp = fopen(".givit/branches", "w");
    if (fp == NULL) return 1;
    fprintf(fp, "master master\n");
    fclose(fp);

    fp = fopen(".givit/hooks", "w");
    if (fp == NULL) return 1;
    fprintf(fp, "todo-check\n");
    fprintf(fp, "format-check\n");
    fprintf(fp, "balance-braces\n");
    fprintf(fp, "file-size-check\n");
    fprintf(fp, "eof-blank-space\n");
    fprintf(fp, "character-limit\n");
    fprintf(fp, "time-limit\n");
    fprintf(fp, "static-error-check\n");
    fclose(fp);

    fp = fopen(".givit/detached", "w");
    if (fp == NULL) return 1;
    fprintf(fp, "1\n");
    fclose(fp);

    fp = fopen(".givit/state", "w");
    if (fp == NULL) return 1;
    fprintf(fp, "0\n");
    fclose(fp);

    return 0;
}

int run_config(int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr, "please enter a valid username or email\n");
        return 1;
    }

    char line[MAX_LINE_LEN];

    if (strcmp(argv[2], "user.name") == 0) {
        FILE *cfg = fopen(".givit/config", "r");
        FILE *tmp = fopen(".givit/config.tmp", "w");
        if (cfg == NULL || tmp == NULL) {
            if (cfg) fclose(cfg);
            if (tmp) fclose(tmp);
            return 1;
        }
        fprintf(tmp, "%s\n", argv[3]);
        fgets(line, sizeof(line), cfg); /* skip old username */
        copy_remaining_lines(cfg, tmp);
        fclose(cfg);
        fclose(tmp);
        remove(".givit/config");
        rename(".givit/config.tmp", ".givit/config");
        return 0;
    } else if (strcmp(argv[2], "user.email") == 0) {
        FILE *cfg = fopen(".givit/config", "r");
        FILE *tmp = fopen(".givit/config.tmp", "w");
        if (cfg == NULL || tmp == NULL) {
            if (cfg) fclose(cfg);
            if (tmp) fclose(tmp);
            return 1;
        }
        /* keep line 1 (username) */
        if (fgets(line, sizeof(line), cfg) != NULL) {
            strip_newline(line);
            fprintf(tmp, "%s\n", line);
        }
        fprintf(tmp, "%s\n", argv[3]);
        fgets(line, sizeof(line), cfg); /* skip old email */
        copy_remaining_lines(cfg, tmp);
        fclose(cfg);
        fclose(tmp);
        remove(".givit/config");
        rename(".givit/config.tmp", ".givit/config");
        return 0;
    } else if (strncmp(argv[2], "alias.", 6) == 0) {
        const char *alias_name = argv[2] + 6;
        FILE *fp = fopen(".givit/aliases", "a");
        if (fp == NULL)
            return 1;
        fprintf(fp, "\"%s\" \"%s\"\n", alias_name, argv[3]);
        fclose(fp);
        return 0;
    } else {
        fprintf(stderr, "invalid command! you must choose between user.name and user.email or set an alias\n");
        return 1;
    }
}

int run_global_config(int argc, char *argv[])
{
    if (argc < 5) {
        fprintf(stderr, "please enter a valid username or email\n");
        return 1;
    }

    const char *home = getenv("HOME");
    if (home == NULL)
        return 1;

    char global_cfg_path[MAX_PATH_LEN];
    snprintf(global_cfg_path, sizeof(global_cfg_path), "%s/.givitconfig", home);

    char repos_path[MAX_PATH_LEN];
    snprintf(repos_path, sizeof(repos_path), "%s/.givit_repos", home);

    char line[MAX_LINE_LEN];
    char saved_cwd[MAX_PATH_LEN];
    if (getcwd(saved_cwd, sizeof(saved_cwd)) == NULL)
        return 1;

    if (strcmp(argv[3], "user.name") == 0) {
        /* update global config */
        char old_email[MAX_NAME_LEN];
        strcpy(old_email, "admin@givit.com");
        {
            FILE *fp = fopen(global_cfg_path, "r");
            if (fp != NULL) {
                fgets(line, sizeof(line), fp); /* old name, discard */
                if (fgets(line, sizeof(line), fp) != NULL) {
                    strip_newline(line);
                    strcpy(old_email, line);
                }
                fclose(fp);
            }
        }
        {
            FILE *fp = fopen(global_cfg_path, "w");
            if (fp == NULL) return 1;
            fprintf(fp, "%s\n", argv[4]);
            fprintf(fp, "%s\n", old_email);
            fclose(fp);
        }

        /* update all repos */
        FILE *repos = fopen(repos_path, "r");
        if (repos != NULL) {
            char repo[MAX_PATH_LEN];
            while (fgets(repo, sizeof(repo), repos) != NULL) {
                strip_newline(repo);
                if (strlen(repo) == 0) continue;
                if (chdir(repo) != 0) continue;

                FILE *cfg = fopen(".givit/config", "r");
                FILE *tmp = fopen(".givit/config.tmp", "w");
                if (cfg == NULL || tmp == NULL) {
                    if (cfg) fclose(cfg);
                    if (tmp) fclose(tmp);
                    continue;
                }
                fprintf(tmp, "%s\n", argv[4]);
                fgets(line, sizeof(line), cfg); /* skip old name */
                copy_remaining_lines(cfg, tmp);
                fclose(cfg);
                fclose(tmp);
                remove(".givit/config");
                rename(".givit/config.tmp", ".givit/config");
            }
            fclose(repos);
        }
        chdir(saved_cwd);
        return 0;
    } else if (strcmp(argv[3], "user.email") == 0) {
        /* update global config */
        char old_name[MAX_NAME_LEN];
        strcpy(old_name, "admin");
        {
            FILE *fp = fopen(global_cfg_path, "r");
            if (fp != NULL) {
                if (fgets(line, sizeof(line), fp) != NULL) {
                    strip_newline(line);
                    strcpy(old_name, line);
                }
                fclose(fp);
            }
        }
        {
            FILE *fp = fopen(global_cfg_path, "w");
            if (fp == NULL) return 1;
            fprintf(fp, "%s\n", old_name);
            fprintf(fp, "%s\n", argv[4]);
            fclose(fp);
        }

        /* update all repos */
        FILE *repos = fopen(repos_path, "r");
        if (repos != NULL) {
            char repo[MAX_PATH_LEN];
            while (fgets(repo, sizeof(repo), repos) != NULL) {
                strip_newline(repo);
                if (strlen(repo) == 0) continue;
                if (chdir(repo) != 0) continue;

                FILE *cfg = fopen(".givit/config", "r");
                FILE *tmp = fopen(".givit/config.tmp", "w");
                if (cfg == NULL || tmp == NULL) {
                    if (cfg) fclose(cfg);
                    if (tmp) fclose(tmp);
                    continue;
                }
                if (fgets(line, sizeof(line), cfg) != NULL) {
                    strip_newline(line);
                    fprintf(tmp, "%s\n", line);
                }
                fprintf(tmp, "%s\n", argv[4]);
                fgets(line, sizeof(line), cfg); /* skip old email */
                copy_remaining_lines(cfg, tmp);
                fclose(cfg);
                fclose(tmp);
                remove(".givit/config");
                rename(".givit/config.tmp", ".givit/config");
            }
            fclose(repos);
        }
        chdir(saved_cwd);
        return 0;
    } else if (strncmp(argv[3], "alias.", 6) == 0) {
        const char *alias_name = argv[3] + 6;

        /* add to all repos */
        FILE *repos = fopen(repos_path, "r");
        if (repos != NULL) {
            char repo[MAX_PATH_LEN];
            while (fgets(repo, sizeof(repo), repos) != NULL) {
                strip_newline(repo);
                if (strlen(repo) == 0) continue;
                if (chdir(repo) != 0) continue;

                FILE *fp = fopen(".givit/aliases", "a");
                if (fp != NULL) {
                    fprintf(fp, "\"%s\" \"%s\"\n", alias_name, argv[4]);
                    fclose(fp);
                }
            }
            fclose(repos);
        }
        chdir(saved_cwd);
        return 0;
    } else {
        fprintf(stderr, "invalid command! you must choose between user.name and user.email or set an alias\n");
        return 1;
    }
}
