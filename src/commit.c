#include "utils.h"
#include "commit.h"
#include "repository.h"

#define COMMITSDB_PATH  GIVIT_DIR "/commitsdb"
#define STATE_PATH      GIVIT_DIR "/state"
#define COMMITS_DIR     GIVIT_DIR "/commits"
#define STAGED_DIR      GIVIT_DIR "/staged"
#define SHORTCUTS_PATH  GIVIT_DIR "/shortcuts"
#define CONFIG_PATH     GIVIT_DIR "/config"
#define DETACHED_PATH   GIVIT_DIR "/booli"

static int read_config(char *username, char *email, char *branch, char *parent_branch) {
    FILE *f = fopen(CONFIG_PATH, "r");
    if (!f) return -1;
    char line[MAX_LINE_LEN];

    if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
    strip_newline(line);
    strcpy(username, line);

    if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
    strip_newline(line);
    strcpy(email, line);

    if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
    strip_newline(line);
    sscanf(line, "branch: %s %s", branch, parent_branch);

    fclose(f);
    return 0;
}

int commit_next_id(void) {
    int id = 0;
    FILE *f = fopen(STATE_PATH, "r");
    if (f) {
        fscanf(f, "%d", &id);
        fclose(f);
    }
    id++;
    f = fopen(STATE_PATH, "w");
    if (f) {
        fprintf(f, "%d\n", id);
        fclose(f);
    }
    return id;
}

Commit *commit_create(const char *message, const char *snapshot_path) {
    Commit *node = malloc(sizeof(Commit));
    if (!node) return NULL;
    memset(node, 0, sizeof(Commit));

    node->timestamp = time(NULL);
    strncpy(node->message, message, MAX_MSG_LEN - 1);
    strncpy(node->snapshot_path, snapshot_path, MAX_PATH_LEN - 1);

    char username[MAX_NAME_LEN], email[MAX_NAME_LEN];
    char branch[MAX_NAME_LEN], parent_branch[MAX_NAME_LEN];
    username[0] = email[0] = branch[0] = parent_branch[0] = '\0';

    read_config(username, email, branch, parent_branch);

    strncpy(node->author_name, username, MAX_NAME_LEN - 1);
    strncpy(node->author_email, email, MAX_NAME_LEN - 1);
    strncpy(node->branch, branch, MAX_NAME_LEN - 1);
    strncpy(node->parent_branch, parent_branch, MAX_NAME_LEN - 1);

    node->branch_parent_id = 0;
    node->merge_parent_id = 0;
    node->prev = NULL;

    return node;
}

void commit_save_list(Commit *head, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return;

    Commit *curr = head;
    while (curr != NULL) {
        fprintf(fp, "%d %ld %d %d \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"\n",
                curr->id,
                (long)curr->timestamp,
                curr->branch_parent_id,
                curr->merge_parent_id,
                curr->message,
                curr->branch,
                curr->parent_branch,
                curr->author_name,
                curr->author_email,
                curr->snapshot_path);
        curr = curr->prev;
    }

    fclose(fp);
}

Commit *commit_load_list(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return NULL;

    Commit *head = NULL;
    Commit *curr = NULL;
    char line[MAX_LINE_LEN * 4];

    while (fgets(line, sizeof(line), fp) != NULL) {
        strip_newline(line);
        if (is_empty_line(line)) continue;

        Commit *node = malloc(sizeof(Commit));
        if (!node) break;
        memset(node, 0, sizeof(Commit));

        long ts;
        sscanf(line, "%d %ld %d %d \"%[^\"]\" \"%[^\"]\" \"%[^\"]\" \"%[^\"]\" \"%[^\"]\" \"%[^\"]\"",
               &node->id,
               &ts,
               &node->branch_parent_id,
               &node->merge_parent_id,
               node->message,
               node->branch,
               node->parent_branch,
               node->author_name,
               node->author_email,
               node->snapshot_path);
        node->timestamp = (time_t)ts;
        node->prev = NULL;

        if (curr != NULL)
            curr->prev = node;
        else
            head = node;
        curr = node;
    }

    fclose(fp);
    return head;
}

void commit_free_list(Commit *head) {
    while (head) {
        Commit *tmp = head;
        head = head->prev;
        free(tmp);
    }
}

Commit *commit_find_by_id(Commit *head, int id) {
    Commit *curr = head;
    while (curr != NULL) {
        if (curr->id == id)
            return curr;
        curr = curr->prev;
    }
    return NULL;
}

Commit *commit_find_by_branch(Commit *head, const char *branch) {
    Commit *curr = head;
    while (curr != NULL) {
        if (strcmp(curr->branch, branch) == 0)
            return curr;
        curr = curr->prev;
    }
    return NULL;
}

Commit *commit_fix_links(Commit *head) {
    Commit *node = head;
    while (node != NULL) {
        Commit *temp = node->prev;
        node->branch_parent_id = 0;
        while (temp != NULL) {
            if (strcmp(temp->branch, node->branch) == 0) {
                node->branch_parent_id = temp->id;
                break;
            }
            temp = temp->prev;
        }
        node = node->prev;
    }
    return head;
}

static void copy_dir_contents(const char *src_dir, const char *dest_dir) {
    char cmd[MAX_PATH_LEN * 3];
    snprintf(cmd, sizeof(cmd), "cp -r \"%s\"/* \"%s\"/ 2>/dev/null", src_dir, dest_dir);
    system(cmd);
}

Commit *commit_create_snapshot(const char *message, Commit *head) {
    int id = commit_next_id();

    char snapshot_dir[MAX_PATH_LEN];
    snprintf(snapshot_dir, sizeof(snapshot_dir), "%s/%d", COMMITS_DIR, id);
    ensure_dir(snapshot_dir);

    char branch[MAX_NAME_LEN], parent_branch[MAX_NAME_LEN];
    char username[MAX_NAME_LEN], email[MAX_NAME_LEN];
    branch[0] = parent_branch[0] = username[0] = email[0] = '\0';
    read_config(username, email, branch, parent_branch);

    Commit *prev_branch_commit = commit_find_by_branch(head, branch);
    if (prev_branch_commit != NULL) {
        copy_dir_contents(prev_branch_commit->snapshot_path, snapshot_dir);
    }

    copy_dir_contents(STAGED_DIR, snapshot_dir);

    Commit *node = commit_create(message, snapshot_dir);
    if (!node) return head;
    node->id = id;
    node->prev = head;
    head = node;

    head = commit_fix_links(head);
    commit_save_list(head, COMMITSDB_PATH);

    return head;
}

Commit *commit_create_for_branch(Commit *base, const char *branch,
                                  const char *parent_branch, Commit *head) {
    if (!base) return head;

    int id = commit_next_id();

    char snapshot_dir[MAX_PATH_LEN];
    snprintf(snapshot_dir, sizeof(snapshot_dir), "%s/%d", COMMITS_DIR, id);
    ensure_dir(snapshot_dir);

    copy_dir_contents(base->snapshot_path, snapshot_dir);

    char msg[MAX_MSG_LEN];
    snprintf(msg, sizeof(msg), "branch %s", branch);

    Commit *node = commit_create(msg, snapshot_dir);
    if (!node) return head;
    node->id = id;
    strncpy(node->branch, branch, MAX_NAME_LEN - 1);
    strncpy(node->parent_branch, parent_branch, MAX_NAME_LEN - 1);
    node->prev = head;
    head = node;

    head = commit_fix_links(head);

    return head;
}

static void find_shortcut_message(const char *shortcut, char *message) {
    FILE *f = fopen(SHORTCUTS_PATH, "r");
    if (!f) {
        fprintf(stderr, "shortcut not found\n");
        message[0] = '\0';
        return;
    }
    char line[MAX_LINE_LEN];
    char shrt[MAX_NAME_LEN], msg[MAX_MSG_LEN];
    while (fgets(line, sizeof(line), f) != NULL) {
        strip_newline(line);
        if (sscanf(line, "\"%[^\"]\" \"%[^\"]\"", shrt, msg) == 2) {
            if (strcmp(shrt, shortcut) == 0) {
                strcpy(message, msg);
                fclose(f);
                return;
            }
        }
    }
    fclose(f);
    fprintf(stderr, "shortcut not found\n");
    message[0] = '\0';
}

int run_commit(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "please enter a valid message\n");
        return 1;
    }

    FILE *booli = fopen(DETACHED_PATH, "r");
    if (booli) {
        char tmp[MAX_LINE_LEN];
        if (fgets(tmp, sizeof(tmp), booli)) {
            strip_newline(tmp);
            if (strcmp(tmp, "0") == 0) {
                fclose(booli);
                fprintf(stderr, "you can't set commit you must fix files in staging area first!\n");
                return 1;
            }
        }
        fclose(booli);
    }

    if (strcmp(argv[2], "-m") != 0 && strcmp(argv[2], "-s") != 0) {
        fprintf(stderr, "please enter a valid message\n");
        return 1;
    }

    if (strlen(argv[3]) > MAX_COMMIT_MSG_LEN) {
        fprintf(stderr, "please enter a valid message\n");
        return 1;
    }

    char message[MAX_MSG_LEN];
    if (strcmp(argv[2], "-m") == 0) {
        strncpy(message, argv[3], MAX_MSG_LEN - 1);
        message[MAX_MSG_LEN - 1] = '\0';
    } else {
        find_shortcut_message(argv[3], message);
        if (message[0] == '\0')
            return 1;
    }

    Commit *head = commit_load_list(COMMITSDB_PATH);
    head = commit_create_snapshot(message, head);

    char cmd[MAX_PATH_LEN * 2];
    snprintf(cmd, sizeof(cmd), "cp %s/staging %s/last_staging", GIVIT_DIR, GIVIT_DIR);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "cp -r %s/staged/ %s/last_commit", GIVIT_DIR, GIVIT_DIR);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "rm -rf %s/staged/*", GIVIT_DIR);
    system(cmd);

    commit_free_list(head);
    return 0;
}

int run_set_shortcut(int argc, char *argv[]) {
    if (argc < 6) {
        fprintf(stderr, "please enter a valid shortcut\n");
        return 1;
    }

    char *msg = NULL;
    char *shortcut = NULL;

    for (int i = 2; i < argc - 1; i++) {
        if (strcmp(argv[i], "-m") == 0)
            msg = argv[i + 1];
        else if (strcmp(argv[i], "-s") == 0)
            shortcut = argv[i + 1];
    }

    if (!msg || !shortcut) {
        fprintf(stderr, "please enter a valid shortcut\n");
        return 1;
    }

    FILE *f = fopen(SHORTCUTS_PATH, "a");
    if (!f) {
        fprintf(stderr, "cannot open shortcuts file\n");
        return 1;
    }
    fprintf(f, "\"%s\" \"%s\"\n", shortcut, msg);
    fclose(f);
    printf("shortcut added!\n");
    return 0;
}

int run_replace_shortcut(int argc, char *argv[]) {
    if (argc < 6) {
        fprintf(stderr, "please enter a valid shortcut\n");
        return 1;
    }

    char *msg = NULL;
    char *shortcut = NULL;

    for (int i = 2; i < argc - 1; i++) {
        if (strcmp(argv[i], "-m") == 0)
            msg = argv[i + 1];
        else if (strcmp(argv[i], "-s") == 0)
            shortcut = argv[i + 1];
    }

    if (!msg || !shortcut) {
        fprintf(stderr, "please enter a valid shortcut\n");
        return 1;
    }

    FILE *fin = fopen(SHORTCUTS_PATH, "r");
    FILE *fout = fopen(GIVIT_DIR "/shortcuts2", "w");
    if (!fin || !fout) {
        if (fin) fclose(fin);
        if (fout) fclose(fout);
        fprintf(stderr, "cannot open shortcuts file\n");
        return 1;
    }

    char line[MAX_LINE_LEN];
    char shrt[MAX_NAME_LEN], mess[MAX_MSG_LEN];
    while (fgets(line, sizeof(line), fin) != NULL) {
        strip_newline(line);
        if (sscanf(line, "\"%[^\"]\" \"%[^\"]\"", shrt, mess) == 2) {
            if (strcmp(shrt, shortcut) == 0)
                fprintf(fout, "\"%s\" \"%s\"\n", shrt, msg);
            else
                fprintf(fout, "\"%s\" \"%s\"\n", shrt, mess);
        }
    }
    fclose(fin);
    fclose(fout);

    remove(SHORTCUTS_PATH);
    rename(GIVIT_DIR "/shortcuts2", SHORTCUTS_PATH);
    printf("shortcut replaced!\n");
    return 0;
}

int run_remove_shortcut(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "please enter a valid shortcut\n");
        return 1;
    }

    char *shortcut = NULL;
    for (int i = 2; i < argc - 1; i++) {
        if (strcmp(argv[i], "-s") == 0)
            shortcut = argv[i + 1];
    }

    if (!shortcut) {
        fprintf(stderr, "please enter a valid shortcut\n");
        return 1;
    }

    FILE *fin = fopen(SHORTCUTS_PATH, "r");
    FILE *fout = fopen(GIVIT_DIR "/shortcuts2", "w");
    if (!fin || !fout) {
        if (fin) fclose(fin);
        if (fout) fclose(fout);
        fprintf(stderr, "cannot open shortcuts file\n");
        return 1;
    }

    char line[MAX_LINE_LEN];
    char shrt[MAX_NAME_LEN], mess[MAX_MSG_LEN];
    char removed_shrt[MAX_NAME_LEN] = "", removed_msg[MAX_MSG_LEN] = "";

    while (fgets(line, sizeof(line), fin) != NULL) {
        strip_newline(line);
        if (sscanf(line, "\"%[^\"]\" \"%[^\"]\"", shrt, mess) == 2) {
            if (strcmp(shrt, shortcut) != 0) {
                fprintf(fout, "\"%s\" \"%s\"\n", shrt, mess);
            } else {
                strncpy(removed_shrt, shrt, MAX_NAME_LEN - 1);
                strncpy(removed_msg, mess, MAX_MSG_LEN - 1);
            }
        }
    }
    fclose(fin);
    fclose(fout);

    remove(SHORTCUTS_PATH);
    rename(GIVIT_DIR "/shortcuts2", SHORTCUTS_PATH);

    if (removed_shrt[0] != '\0')
        printf("shortcut named %s with message %s successfully removed!\n", removed_shrt, removed_msg);

    return 0;
}
