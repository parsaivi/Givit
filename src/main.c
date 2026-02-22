#include "givit.h"
#include "utils.h"
#include "repository.h"
#include "staging.h"
#include "commit.h"
#include "log.h"
#include "branch.h"
#include "diff.h"
#include "tag.h"
#include "hooks.h"
#include "grep.h"
#include "stash.h"

static int check_alias(const char *name) {
    FILE *f = fopen(GIVIT_DIR "/aliases", "r");
    if (!f) return 0;
    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), f)) {
        strip_newline(line);
        char alias[MAX_NAME_LEN], command[MAX_LINE_LEN];
        if (sscanf(line, "\"%[^\"]\" \"%[^\"]\"", alias, command) == 2) {
            if (strcmp(alias, name) == 0) {
                fclose(f);
                char cmd[MAX_LINE_LEN];
                snprintf(cmd, sizeof(cmd), "givit %s", command);
                return system(cmd) == 0 ? 1 : -1;
            }
        }
    }
    fclose(f);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: givit <command> [args]\n");
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "init") == 0)
        return run_init(argc, argv);

    if (strcmp(cmd, "config") == 0) {
        if (argc >= 3 && strcmp(argv[2], "--global") == 0)
            return run_global_config(argc, argv);
        return run_config(argc, argv);
    }

    if (!repo_exists()) {
        fprintf(stderr, "fatal: not a givit repository\n");
        return 1;
    }

    ensure_staged_subdirs(".");

    if (argc == 2) {
        int r = check_alias(cmd);
        if (r == 1) return 0;
        if (r == -1) return 1;
    }

    if (strcmp(cmd, "add") == 0)         return run_add(argc, argv);
    if (strcmp(cmd, "reset") == 0)       return run_reset(argc, argv);
    if (strcmp(cmd, "status") == 0)      return run_status(argc, argv);
    if (strcmp(cmd, "commit") == 0)      return run_commit(argc, argv);
    if (strcmp(cmd, "set") == 0)         return run_set_shortcut(argc, argv);
    if (strcmp(cmd, "replace") == 0)     return run_replace_shortcut(argc, argv);
    if (strcmp(cmd, "remove") == 0)      return run_remove_shortcut(argc, argv);
    if (strcmp(cmd, "log") == 0)         return run_log(argc, argv);
    if (strcmp(cmd, "branch") == 0)      return run_branch(argc, argv);
    if (strcmp(cmd, "checkout") == 0)    return run_checkout(argc, argv);
    if (strcmp(cmd, "revert") == 0)      return run_revert(argc, argv);
    if (strcmp(cmd, "tag") == 0)         return run_tag(argc, argv);
    if (strcmp(cmd, "pre-commit") == 0)  return run_precommit(argc, argv);
    if (strcmp(cmd, "grep") == 0)        return run_grep(argc, argv);
    if (strcmp(cmd, "diff") == 0)        return run_diff(argc, argv);
    if (strcmp(cmd, "merge") == 0)       return run_merge(argc, argv);
    if (strcmp(cmd, "stash") == 0)       return run_stash(argc, argv);

    fprintf(stderr, "givit: '%s' is not a givit command\n", cmd);
    return 1;
}
