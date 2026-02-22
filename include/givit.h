#ifndef GIVIT_H
#define GIVIT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <glob.h>

#define GIVIT_DIR           ".givit"
#define MAX_PATH_LEN        1024
#define MAX_MSG_LEN         2048
#define MAX_NAME_LEN        256
#define MAX_LINE_LEN        1024
#define MAX_UNDO_STEPS      10
#define MAX_COMMIT_MSG_LEN  72

typedef struct Commit {
    int id;
    time_t timestamp;
    int branch_parent_id;
    int merge_parent_id;
    char message[MAX_MSG_LEN];
    char branch[MAX_NAME_LEN];
    char parent_branch[MAX_NAME_LEN];
    char author_name[MAX_NAME_LEN];
    char author_email[MAX_NAME_LEN];
    char snapshot_path[MAX_PATH_LEN];
    struct Commit *prev;
} Commit;

typedef struct StashEntry {
    int index;
    char branch[MAX_NAME_LEN];
    char message[MAX_MSG_LEN];
    char path[MAX_PATH_LEN];
    time_t timestamp;
} StashEntry;

#endif
