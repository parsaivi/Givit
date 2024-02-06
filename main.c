// its a version of git that most runs all command in git when it called in terminal and it runs in linux terminal
//402171075
//Parsa Malekian

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <glob.h>
#include <time.h>

#define MAX_FILENAME_LENGTH 1000
#define MAX_COMMIT_MESSAGE_LENGTH 2000
#define MAX_LINE_LENGTH 1000
#define MAX_MESSAGE_LENGTH 1000
#define MAX_CONFIG_LENGTH 1000

#define forLoop(n, i) for(int i = 0 ; i < n ; i++)
#define checkk(n) printf("%s\n",n)

typedef struct Commit {
    int hour;
    int minute;
    int second;
    int commit_ID;
    int prev_attached_commit_ID;
    int merged_ID;
    char message[MAX_LINE_LENGTH];
    char branch[MAX_LINE_LENGTH];
    char parent_branch[MAX_LINE_LENGTH];
    char userName[MAX_LINE_LENGTH];
    char userEmail[MAX_LINE_LENGTH];
    char dir_path[MAX_LINE_LENGTH];
    struct Commit *prev_commit;
    struct Commit *prev_attached_commit;
    struct Commit *merged;
} Commit;

void check_valid(char *name) {
    int len = strlen(name);
    if (name[len - 1] == '\n') {
        name[len - 1] = '\0';
    }
}

int check_line(char *line) {
    int flag = 0;
    forLoop(strlen(line), i) {
        if ((line[i] != ' ') && (line[i] != '\n') && (line[i] != '\t')) {
            flag = 1;
        }
    }
    return flag;
}

void copy_therest(FILE *file1, FILE *file2) {
    char temp[MAX_CONFIG_LENGTH];
    while (fgets(temp, MAX_CONFIG_LENGTH, file1) != NULL) {
        check_valid(temp);
        fprintf(file2, "%s\n", temp);
    }
}

Commit *findCommit_branch(char *branch, char *parent_branch);

Commit *createNode(char *path, char *message, int hour, int minute, int second, Commit *head);

char *filename_maker(char *filepath) { // ALi Moghadasi
    char *filename = malloc(1024);
    strcpy(filename, filepath);
    int len = strlen(filename);
    int j = 0;
    while (filename[len] != '/') {
        j++;
        len--;
    }
    memmove(filename, filepath + len + 1, j * sizeof(char));
    return filename;
}

void saveList(struct Commit *head, char *filename);

struct Commit *loadList(char *filename);

void filename_to_filepath(char *filename, char *cwd);

int run_init(int argc, char *const argv[], char username[], char email[]);

int create_configs(char *username, char *email);

int run_config(int argc, char *const argv[]);

int run_global_config(int argc, char *const argv[]);

int run_add(int argc, char *const argv[]);

void add_file_or_directory(char *filepath, char *directory_path);

bool check_for_exist_file(char *filepath);

bool check_for_exist_directory(char *dirpath);

int is_file_or_directory(char *filepath);

int is_file_or_directory_general(char *filepath);

void add_to_staging(char *filepath);

int run_reset(int argc, char *const argv[]);

void remove_from_staging(char *filepath);

void unstage_file_or_directory(char *filepath, char *directory_path);

int run_commit(int argc, char *const argv[], int hour, int minute, int second, Commit *head);

Commit *create_last_commit_dir(char *message, int hour, int minute, int second, Commit *head);

void find_and_replace_shortcut(char *old_shortcut, char *message);

int run_log(int argc, char *const argv[], Commit *head);

int commit_staged_file(int commit_ID, char *filepath);

int track_file(char *filepath);

bool is_tracked(char *filepath);

bool is_staged(char *filepath);

void print_status(char *filepath, int depth);

int create_commit_file(int commit_ID, char *message);

int find_file_last_commit(char *filepath);

bool file_is_changed(FILE *file_befor, FILE *file_after);

int run_branch(int argc, char *const argv[], int hour, int minute, int second, Commit *head);

int run_checkout(int argc, char *const argv[], int hour, int minute, int second, Commit *head);

int find_file_last_change_before_commit(char *filepath, int commit_ID);

int checkout_file(char *filepath, int commit_ID);

bool line_is_empty(char *line);

void sort_tags();

void print_valid_file(FILE *file, FILE *valid_file);

void
compare_to_file(char *file1_path, char *file2_path, int begin_line1, int begin_line2, int end_line1, int end_line2);

void compare_to_file_merge(char *file1_path, char *file2_path, int begin_line1, int begin_line2, int end_line1,
                           int end_line2);

int compare_to_file_merge_conflict(char *file1_path, char *file2_path, char *commit_path1, char *commit_path2,
                                   char *branch1, char *branch2);
//

void mkdir_all_directories();

Commit *create_newbranch_commit(Commit *node, char *branch, char *parent_branch, int hour, int minute, int second,
                                Commit *head);

Commit *fix_commits(Commit *head);

Commit *fix_commits(Commit *head) {//fix commits prev_attached_commits : it shows last commit in same branch as commit:
    //fix prev_attached_commits:
    Commit *node = head;
    while (node != NULL) {
        Commit *temp = node->prev_commit;
        while (temp != NULL) {
            if (strcmp(temp->branch, node->branch) == 0) {
                node->prev_attached_commit = temp;
                break;
            }
            temp = temp->prev_commit;
        }
        node = node->prev_commit;
    }
    //fix merged_commits:
    node = head;
    while (node != NULL) {
        if (node->merged_ID != 0) {
            Commit *temp = head;
            while (temp != NULL) {
                if (temp->commit_ID == node->merged_ID) {
                    node->merged = temp;
                    break;
                }
                temp = temp->prev_commit;
            }
        }
        node = node->prev_commit;
    }
    return head;
}

Commit *findCommit_ID(int commit_ID, Commit *head);

Commit *findCommit_ID(int commit_ID, Commit *head) {
    Commit *temp = head;
    while (temp != NULL) {
        if (temp->commit_ID == commit_ID) {
            return temp;
        }
        temp = temp->prev_commit;
    }
    return NULL;
}

Commit *createNode(char *path, char *message, int hour, int minute, int second, Commit *head) {
    Commit *node = malloc(sizeof(Commit));
    node->hour = hour;
    node->minute = minute;
    node->second = second;
    time_t t = time(NULL);
    node->commit_ID = (int) t;
    strcpy(node->message, message);
    //read username email and branch name from config file:
    FILE *file = fopen(".givit/config", "r");
    char temp[MAX_CONFIG_LENGTH];
    fgets(temp, MAX_CONFIG_LENGTH, file);
    check_valid(temp);
    strcpy(node->userName, temp);
    fgets(temp, MAX_CONFIG_LENGTH, file);
    check_valid(temp);
    strcpy(node->userEmail, temp);
    fgets(temp, MAX_CONFIG_LENGTH, file);
    check_valid(temp);
    sscanf(temp, "branch: %s %s", node->branch, node->parent_branch);
    fclose(file);
    strcpy(node->dir_path, path);
    node->prev_commit = head;
    int flag = 0;
    if (head == NULL) {
        node->prev_attached_commit_ID = 0;
        node->prev_attached_commit = NULL;
    } else {
        Commit *template = head;
        while (strcmp(template->branch, node->branch) != 0) {
            template = template->prev_commit;
            flag = 1;
            if (template == NULL)
                break;
        }
        if (flag == 0) {
            node->prev_attached_commit = head;
        } else {
            node->prev_attached_commit = template;
        }
    }
    node->merged_ID = 0;
    node->merged = NULL;
    return node;
}

void saveList(struct Commit *head, char *filename) {
    FILE *fp = fopen(filename, "w");

    Commit *curr = head;

    while (curr != NULL) {
        fprintf(fp, "%d %d %d %d %d %d \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"\n", curr->hour, curr->minute,
                curr->second, curr->commit_ID, curr->prev_attached_commit_ID, curr->merged_ID, curr->message,
                curr->branch,
                curr->parent_branch, curr->userName, curr->userEmail, curr->dir_path);
        curr = curr->prev_commit;
    }

    fclose(fp);
}

Commit *loadList(char *filename) {

    Commit *head = NULL;
    Commit *curr = NULL;
    Commit *tmp = NULL;

    FILE *fp = fopen(filename, "r");

    if (fp == NULL) {
        return NULL;
    }
    char temp[1024];
    while (fgets(temp, 1024, fp) != NULL) {
        tmp = malloc(sizeof(Commit));
        check_valid(temp);
        sscanf(temp, "%d %d %d %d %d %d \"%[^\"]\" \"%[^\"]\" \"%[^\"]\" \"%[^\"]\" \"%[^\"]\" \"%[^\"]\"\n",
               &tmp->hour,
               &tmp->minute, &tmp->second, &tmp->commit_ID, &tmp->prev_attached_commit_ID, &tmp->merged_ID,
               tmp->message, tmp->branch,
               tmp->parent_branch, tmp->userName, tmp->userEmail, tmp->dir_path);
        if (curr != NULL) { curr->prev_commit = tmp; }
        else head = tmp;
        curr = tmp;
    }

    fclose(fp);

    return head;
}


int run_init(int argc, char *const argv[], char username[], char email[]) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) return 1;

    char tmp_cwd[1024];
    bool exists = false;
    struct dirent *entry;
    do {
        // find .givit
        DIR *dir = opendir(".");
        if (dir == NULL) {
            perror("Error opening current directory");
            return 1;
        }
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".givit") == 0)
                exists = true;
        }
        closedir(dir);

        // update current working directory
        if (getcwd(tmp_cwd, sizeof(tmp_cwd)) == NULL) return 1;

        // change cwd to parent
        if (strcmp(tmp_cwd, "/") != 0) {
            if (chdir("..") != 0) return 1;
        }

    } while (strcmp(tmp_cwd, "/") != 0);

    // return to the initial cwd
    if (chdir(cwd) != 0) return 1;

    if (!exists) {
        if (mkdir(".givit", 0755) != 0) return 1;
        fprintf(stdout,
                "   ▄██████▄   ▄█   ▄█    █▄   ▄█      ███     \n"
                "  ███    ███ ███  ███    ███ ███  ▀█████████▄ \n"
                "  ███    █▀  ███▌ ███    ███ ███▌    ▀███▀▀██ \n"
                " ▄███        ███▌ ███    ███ ███▌     ███   ▀ \n"
                "▀▀███ ████▄  ███▌ ███    ███ ███▌     ███     \n"
                "  ███    ███ ███  ███    ███ ███      ███     \n"
                "  ███    ███ ███  ███▄  ▄███ ███      ███     \n"
                "  ████████▀  █▀    ▀██████▀  █▀      ▄████▀   \n");
        const char *home = getenv("HOME");
        chdir(home);
        FILE *file = fopen("CLionProjects/Givit/all_repository", "a");
        fprintf(file, "%s\n", cwd);
        chdir(cwd);
        return create_configs(username, email);
    } else {
        perror("givit repository has already initialized");
    }
    return 0;
}

int create_configs(char *username, char *email) {
    FILE *file = fopen(".givit/config", "w");
    if (file == NULL) return 1;

    fprintf(file, "%s\n", username);
    fprintf(file, "%s\n", email);
    fprintf(file, "branch: %s %s\n", "master", "master");
    fprintf(file, "last_commit_ID: %d\n", 0);
    fprintf(file, "current_commit_ID: %d\n", 0);
    fprintf(file, "date %d %d %d", 0, 0, 0);

    fclose(file);

    // create commits folder
    if (mkdir(".givit/commits", 0755) != 0) return 1;

    //create staged folder
    if (mkdir(".givit/staged", 0755) != 0) return 1;

    if (mkdir(".givit/1_staged", 0755) != 0) return 1;
    if (mkdir(".givit/2_staged", 0755) != 0) return 1;
    if (mkdir(".givit/3_staged", 0755) != 0) return 1;
    if (mkdir(".givit/4_staged", 0755) != 0) return 1;
    if (mkdir(".givit/5_staged", 0755) != 0) return 1;
    if (mkdir(".givit/6_staged", 0755) != 0) return 1;
    if (mkdir(".givit/7_staged", 0755) != 0) return 1;
    if (mkdir(".givit/8_staged", 0755) != 0) return 1;
    if (mkdir(".givit/9_staged", 0755) != 0) return 1;
    if (mkdir(".givit/10_staged", 0755) != 0) return 1;
    // create files folder
    if (mkdir(".givit/files", 0755) != 0) return 1;

    if (mkdir(".givit/valid", 0777) != 0) return 1;

    file = fopen(".givit/staging", "w");
    fclose(file);
    file = fopen(".givit/aliases", "w");
    fclose(file);
    file = fopen(".givit/last_staging", "w");
    fclose(file);
    file = fopen(".givit/havebeenstaged", "w");
    fclose(file);
    file = fopen(".givit/commitsdb", "w");
    fclose(file);
    file = fopen(".givit/tracks", "w");
    fclose(file);
    file = fopen(".givit/branchs", "w");
    fprintf(file, "master master\n");
    fclose(file);
    file = fopen(".givit/shortcuts", "w");
    fclose(file);
    file = fopen(".givit/tags", "w");
    fclose(file);
    file = fopen(".givit/hooks", "w");
    fprintf(file, "todo-check\n");
    fprintf(file, "format-check\n");
    fprintf(file, "balance-braces\n");
    fprintf(file, "file-size-check\n");
    fprintf(file, "eof-blank-space\n");
    fprintf(file, "character-limit\n");
    fprintf(file, "time-limit\n");
    fprintf(file, "static-error-check\n");
    fclose(file);
    file = fopen(".givit/applied_hooks", "w");
    fclose(file);
    file = fopen(".givit/bool", "w");
    fprintf(file, "1\n");
    fclose(file);
    return 0;
}

bool is_staged_path(char *filepath) {//get file general path from home/ and check if its in staged files:
    // staging format is : ./filepath
    // general path format is : /home/username/.../filepath
    FILE *staging = fopen(".givit/staging", "r");
    char cwd[1024];
    char temp[MAX_LINE_LENGTH];
    char pathmake[1024];
    while (fgets(temp, MAX_LINE_LENGTH, staging) != NULL) {
        check_valid(temp);
        getcwd(cwd, sizeof(cwd));
        memmove(temp, temp + 1, strlen(temp));
        strcpy(pathmake, cwd);
        strcat(pathmake, temp);
        if (strcmp(pathmake, filepath) == 0) {
            return true;
        }
    }
    fclose(staging);
    return false;
}

bool is_tracked_path(char *filepath) {//get file general path from home/ and check if its in tracked files:
    // staging format is : ./filepath
    // general path format is : /home/username/.../filepath
    FILE *staging = fopen(".givit/tracks", "r");
    char cwd[1024];
    char temp[MAX_LINE_LENGTH];
    char pathmake[1024];
    while (fgets(temp, MAX_LINE_LENGTH, staging) != NULL) {
        check_valid(temp);
        getcwd(cwd, sizeof(cwd));
        memmove(temp, temp + 1, strlen(temp));
        strcpy(pathmake, cwd);
        strcat(pathmake, temp);
        if (strcmp(pathmake, filepath) == 0) {
            return true;
        }
    }
    fclose(staging);
    return false;
}

int is_changed_last_commit(char *filepath) {// filepath format is : ./filepath
    //find current branch:
    FILE *config = fopen(".givit/config", "r");
    char temp[MAX_LINE_LENGTH];
    fgets(temp, MAX_LINE_LENGTH, config);
    fgets(temp, MAX_LINE_LENGTH, config);
    fgets(temp, MAX_LINE_LENGTH, config);
    check_valid(temp);
    char branch[MAX_LINE_LENGTH], trash[MAX_LINE_LENGTH];
    sscanf(temp, "branch: %s %s", branch, trash);
    // find last commit in this branch:
    Commit *head = loadList(".givit/commitsdb");
    head = fix_commits(head);
    Commit *node = head;
    while (node != NULL) {
        if (strcmp(node->branch, branch) == 0) {
            break;
        }
        node = node->prev_commit;
    }
    //find last commit directory:
    char commited_path[1024];
    strcpy(commited_path, node->dir_path);
    strcat(commited_path, filepath);
    //
    FILE *file_befor = fopen(commited_path, "r");
    if (file_befor == NULL) {
        return 2;
    }
    FILE *file_after = fopen(filepath, "r");
    if (file_is_changed(file_befor, file_after)) {
        return 1;
    }
    return 0;
}


bool is_changed_lastcommit_general(
        char *generalpath) {//get file general path from home/ and check if its in changed files in last commit:
    // staging format is : ./filepath
    // general path format is : /home/username/.../filepath
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    int len = strlen(cwd);
    char filepath[MAX_LINE_LENGTH];
    memmove(filepath, generalpath + len, strlen(generalpath) - len);
    char commited_path[1024];
    //find current branch:
    FILE *config = fopen(".givit/config", "r");
    char temp[MAX_LINE_LENGTH];
    fgets(temp, MAX_LINE_LENGTH, config);
    fgets(temp, MAX_LINE_LENGTH, config);
    fgets(temp, MAX_LINE_LENGTH, config);
    check_valid(temp);
    char branch[MAX_LINE_LENGTH], trash[MAX_LINE_LENGTH];
    sscanf(temp, "branch: %s %s", branch, trash);
    // find last commit in this branch:
    Commit *head = loadList(".givit/commitsdb");
    head = fix_commits(head);
    Commit *node = head;
    while (node != NULL) {
        if (strcmp(node->branch, branch) == 0) {
            break;
        }
        node = node->prev_commit;
    }
    //find last commit directory:
    strcpy(commited_path, node->dir_path);
    strcat(commited_path, filepath);
    //
    FILE *commited_file = fopen(commited_path, "r");
    FILE *current_file = fopen(generalpath, "r");
    return file_is_changed(commited_file, current_file);
}

void add_n(char *dir_path, int depth, int max_depth) { // idea from Ali Moghadasi
    if (depth == max_depth) {
        return;
    }
    DIR *dir = opendir(dir_path);
    struct dirent *entry;
    char entry_path[1024];
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 &&
                (strcmp(entry->d_name, ".givit") != 0)) {
                forLoop(depth, i) {
                    printf("\t");
                }
                printf("└─── %s\n", entry->d_name);
                strcpy(entry_path, dir_path);
                strcat(entry_path, "/");
                strcat(entry_path, entry->d_name);
                add_n(entry_path, depth + 1, max_depth);
            }

        } else if (entry->d_type == DT_REG) {
            forLoop(depth, i) {
                printf("\t");
            }
            printf("└─── ");
            char path[1024];
            strcpy(path, dir_path);
            strcat(path, "/");
            strcat(path, entry->d_name);
            if (is_staged(path)) {
                printf("\e[37m%s\n\e[m", entry->d_name);
            } else {
                printf("\e[31m%s\n\e[m", entry->d_name);
            }

        }
    }
}


int run_add(int argc,
            char *const argv[]) { // 1: add files into staging area 2: add -n shows that each file are staged or not
    if (argc < 3) {
        perror("please specify a file");
        return 1;
    }
    if (strcmp(argv[2], "-n") ==
        0) { // checks all files and shows that they are staged or not - depth shows how much we must go deep in folders
        int depth = 0;
        if (argc == 4) {
            depth = atoi(argv[3]);
        }
        if (depth > 0) {
            add_n(".", 0, depth);
        } else {
            perror("please enter a valid depth");
        }

        return 0;
    }
    //copy staged directory into last_staged:
    system("rm -rf .givit/10_staged/* > /dev/null 2>&1");
    system("cp -R .givit/9_staged/* .givit/10_staged > /dev/null 2>&1");
    system("rm -rf .givit/9_staged/* > /dev/null 2>&1");
    system("cp -R .givit/8_staged/* .givit/9_staged > /dev/null 2>&1");
    system("rm -rf .givit/8_staged/* > /dev/null 2>&1");
    system("cp -R .givit/7_staged/* .givit/8_staged > /dev/null 2>&1");
    system("rm -rf .givit/7_staged/* > /dev/null 2>&1");
    system("cp -R .givit/6_staged/* .givit/7_staged > /dev/null 2>&1");
    system("rm -rf .givit/6_staged/* > /dev/null 2>&1");
    system("cp -R .givit/5_staged/* .givit/6_staged > /dev/null 2>&1");
    system("rm -rf .givit/5_staged/* > /dev/null 2>&1");
    system("cp -R .givit/4_staged/* .givit/5_staged > /dev/null 2>&1");
    system("rm -rf .givit/4_staged/* > /dev/null 2>&1");
    system("cp -R .givit/3_staged/* .givit/4_staged > /dev/null 2>&1");
    system("rm -rf .givit/3_staged/* > /dev/null 2>&1");
    system("cp -R .givit/2_staged/* .givit/3_staged > /dev/null 2>&1");
    system("rm -rf .givit/2_staged/* > /dev/null 2>&1");
    system("cp -R .givit/1_staged/* .givit/2_staged > /dev/null 2>&1");
    system("rm -rf .givit/1_staged/* > /dev/null 2>&1");
    system("cp -R .givit/staged/* .givit/1_staged > /dev/null 2>&1");
    if (strcmp(argv[2], "-redo") == 0) {// stages files that are in havebeenstaged but not exist in staged
        FILE *staging = fopen(".givit/staging", "r");
        FILE *havebeenstaged = fopen(".givit/havebeenstaged", "r");
        char temp[MAX_LINE_LENGTH];
        while (fgets(temp, MAX_LINE_LENGTH, havebeenstaged) != NULL) {
            check_valid(temp);
            if (!is_staged(temp)) {
                add_to_staging(temp);
            }
        }
        fclose(staging);
        fclose(havebeenstaged);
        return 0;

    }
    int i = 2;
    if (strcmp(argv[2], "-f") == 0) {// -f is optional
        i++;
    }
    char main_path[1024];
    for (i; i < argc; i++) {
        getcwd(main_path, sizeof(main_path));
        strcat(main_path, "/");
        strcat(main_path, argv[i]);
        FILE *file = fopen(main_path, "r");
        DIR *dir = opendir(main_path);
        if (file != NULL || dir != NULL) {
            add_file_or_directory(argv[i], ".");
        } else {
            perror("file not found");
            return 1;
        }
    }
    return 0;
}

void add_file_or_directory(char *filepath, char *directory_path) {
    char temp[MAX_LINE_LENGTH];
    strcpy(temp, directory_path);
    if ((directory_path[strlen(directory_path) - 1]) != '/')
        strcat(temp, "/");
    strcat(temp, filepath);
    if (is_file_or_directory(temp) == 1) {
        add_to_staging(temp);
    } else if (is_file_or_directory(temp) == 2) {
        //strcat(temp, "/");

        DIR *dir = opendir(temp);
        struct dirent *entry;
        if (dir == NULL) {
            perror("Error opening current directory");
            return;
        }
        //strcat(temp, "/");
        while ((entry = readdir(dir)) != NULL) {
            if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) &&
                (strcmp(entry->d_name, ".givit") != 0)) {
                add_file_or_directory(entry->d_name, temp);
            }
        }
        closedir(dir);
    } else {
        perror("file not found");
        return;
    }
}

bool check_for_exist_file(char *filepath) {
    char temp[MAX_LINE_LENGTH];
    getcwd(temp, sizeof(temp));
    strcat(temp, "/");
    strcat(temp, filepath);
    FILE *file = fopen(temp, "r");
    if (file == NULL) {
        return false;
    }
    fclose(file);
    return true;
}

bool check_for_exist_directory(char *dirpath) {
    char temp[MAX_LINE_LENGTH];
    getcwd(temp, sizeof(temp));
    strcat(temp, "/");
    strcat(temp, dirpath);
    DIR *dir = opendir(temp);
    if (dir == NULL) {
        return false;
    }
    closedir(dir);
    return true;
}

int is_file_or_directory_general(char *filepath) {//general path
    FILE *file = fopen(filepath, "r");
    char temp[1024];
    strcpy(temp, filepath);
    strcat(temp, "/");
    DIR *dir = opendir(temp);
    if (file == NULL) {
        if (dir == NULL) {
            return 0;
        }
        closedir(dir);
        return 2;
    }
    fclose(file);
    return 1;
}

int is_file_or_directory(char *filepath) {
    char temp[MAX_LINE_LENGTH];
    getcwd(temp, sizeof(temp));
    if (filepath[0] != '/') { strcat(temp, "/"); }
    strcat(temp, filepath);

    FILE *file = fopen(temp, "r");
    if (file == NULL) {

        return 0;
    }
    if (check_for_exist_directory(filepath)) {
        return 2;
    }
    fclose(file);
    return 1;
}

void find_file(char *filepath, char *filename) { //find file and returns the filepath
    DIR *dir = opendir(".");
    struct dirent *entry;
    if (dir == NULL) {
        perror("Error opening current directory");
        return;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && strcmp(entry->d_name, filename) == 0) {
            //filename to filepath:
            char cwd[1024];
            filename_to_filepath(filename, cwd);
            strcpy(filepath, cwd);
            closedir(dir);
            return;
        }
        if (entry->d_type == DT_DIR) {
            chdir(entry->d_name);
            find_file(filepath, filename);
            chdir("..");
        }
    }
    closedir(dir);
}

void add_to_staging(char *filepath) {
    //check filepath exist in staging:
    int flag1 = 0, flag2 = 0, flag3 = 0;
    FILE *staging = fopen(".givit/staging", "r");
    FILE *havebeenstaged = fopen(".givit/havebeenstaged", "r");
    FILE *tracked = fopen(".givit/tracks", "r");
    char temp[MAX_LINE_LENGTH];
    while (fgets(temp, MAX_LINE_LENGTH, staging) != NULL) {
        check_valid(temp);
        if (strcmp(temp, filepath) == 0) {
            flag1 = 1;
            break;
        }
    }
    while (fgets(temp, MAX_LINE_LENGTH, havebeenstaged) != NULL) {
        check_valid(temp);
        if (strcmp(temp, filepath) == 0) {
            flag2 = 1;
            break;
        }
    }
    while (fgets(temp, MAX_LINE_LENGTH, tracked) != NULL) {
        check_valid(temp);
        if (strcmp(temp, filepath) == 0) {
            flag3 = 1;
            break;
        }
    }
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        return;
    }
    if (flag1 == 0) {
        staging = fopen(".givit/staging", "a");
        fprintf(staging, "%s\n", filepath);
    }
    if (flag2 == 0) {
        havebeenstaged = fopen(".givit/havebeenstaged", "a");
        fprintf(havebeenstaged, "%s\n", filepath);
    }
    if (flag3 == 0) {
        tracked = fopen(".givit/tracks", "a");
        fprintf(tracked, "%s\n", filepath);
    }
    fclose(staging);
    //copy file into .givit/staged
    char staged_file_path[MAX_LINE_LENGTH];
    strcpy(staged_file_path, ".givit/staged/");
    strcat(staged_file_path, filepath);
    FILE *staged_file = fopen(staged_file_path, "w");
    while (fgets(temp, MAX_LINE_LENGTH, file) != NULL) {
        fprintf(staged_file, "%s", temp);
    }
    fclose(file);
    fclose(staged_file);
}

int run_global_config(int argc, char *const argv[]) {
    if (argc < 5) {
        perror("please enter a valid username or email");
        return 1;
    }
    char temp[MAX_CONFIG_LENGTH];
    if (strcmp(argv[3], "user.name") == 0) {
        const char *home = getenv("HOME");
        chdir(home);
        FILE *config = fopen("CLionProjects/Givit/config", "r");
        FILE *config2 = fopen("CLionProjects/Givit/config2", "w");
        check_valid(argv[4]);
        fprintf(config2, "%s\n", argv[4]);
        fgets(temp, MAX_CONFIG_LENGTH, config);
        fgets(temp, MAX_CONFIG_LENGTH, config);
        check_valid(temp);
        fprintf(config2, "%s", temp);
        fclose(config);
        fclose(config2);
        remove("CLionProjects/Givit/config");
        rename("CLionProjects/Givit/config2", "CLionProjects/Givit/config");
        FILE *file = fopen("CLionProjects/Givit/all_repository", "r");
        while (fgets(temp, MAX_CONFIG_LENGTH, file) != NULL) {
            check_valid(temp);
            chdir(temp);
            FILE *config = fopen(".givit/config", "r");
            FILE *config2 = fopen(".givit/config2", "w");
            check_valid(argv[4]);
            fprintf(config2, "%s\n", argv[4]);
            fgets(temp, MAX_CONFIG_LENGTH, config);
            fgets(temp, MAX_CONFIG_LENGTH, config);
            check_valid(temp);
            fprintf(config2, "%s\n", temp);
            copy_therest(config, config2);
            fclose(config);
            fclose(config2);
            remove(".givit/config");
            rename(".givit/config2", ".givit/config");
        }
        return 0;
    } else if (strcmp(argv[3], "user.email") == 0) {
        const char *home = getenv("HOME");
        chdir(home);
        FILE *config = fopen("CLionProjects/Givit/config", "r");
        FILE *config2 = fopen("CLionProjects/Givit/config2", "w");
        fgets(temp, MAX_CONFIG_LENGTH, config);
        check_valid(temp);
        check_valid(argv[4]);
        fprintf(config2, "%s\n", temp);
        fprintf(config2, "%s\n", argv[4]);
        fclose(config);
        fclose(config2);
        remove("CLionProjects/Givit/config");
        rename("CLionProjects/Givit/config2", "CLionProjects/Givit/config");
        FILE *file = fopen("CLionProjects/Givit/all_repository", "r");
        while (fgets(temp, MAX_CONFIG_LENGTH, file) != NULL) {
            check_valid(temp);
            chdir(temp);
            config = fopen(".givit/config", "r");
            config2 = fopen(".givit/config2", "w");
            fgets(temp, MAX_CONFIG_LENGTH, config);
            check_valid(temp);
            check_valid(argv[4]);
            fprintf(config2, "%s\n", temp);
            fprintf(config2, "%s\n", argv[4]);
            fgets(temp, MAX_CONFIG_LENGTH, config);
            copy_therest(config, config2);
            fclose(config);
            fclose(config2);
            remove(".givit/config");
            rename(".givit/config2", ".givit/config");
        }
        return 0;
    } else if (strncmp(argv[3], "alias", 5) == 0) { //alias.<alias name> "command"
        const char *home = getenv("HOME"); //TODO run it in all repository
        char *alias_name = argv[3] + 6;
        char command[MAX_CONFIG_LENGTH];
        chdir(home);
        FILE *config = fopen("CLionProjects/Givit/aliases", "a");
        strcpy(command, argv[4]);
        fprintf(config, "\"%s\" \"%s\"\n", alias_name, command);
        //run it in all repository in Givit/all_repository:
        FILE *file = fopen("CLionProjects/Givit/all_repository", "r");
        while (fgets(temp, MAX_CONFIG_LENGTH, file) != NULL) {
            check_valid(temp);
            chdir(temp);
            config = fopen(".givit/aliases", "a");
            strcpy(command, argv[4]);
            fprintf(config, "\"%s\" \"%s\"\n", alias_name, command);
        }
        return 0;
    } else {
        perror("invalid command! you must choose between user.name and user.email or set an alias");
        return 1;
    }
}

int run_config(int argc, char *const argv[]) {
    if (argc < 4) {
        perror("please enter a valid username or email");
        return 1;
    }
    char temp[MAX_CONFIG_LENGTH];
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) return 1;
    chdir(cwd);
    if (strcmp(argv[2], "user.name") == 0) {
        FILE *config = fopen(".givit/config", "r");
        FILE *config2 = fopen(".givit/config2", "w");
        check_valid(argv[3]);
        fprintf(config2, "%s\n", argv[3]);
        fgets(temp, MAX_CONFIG_LENGTH, config);
        copy_therest(config, config2);
        fclose(config);
        fclose(config2);
        remove(".givit/config");
        rename(".givit/config2", ".givit/config");
        return 0;
    } else if (strcmp(argv[2], "user.email") == 0) {
        FILE *config = fopen(".givit/config", "r");
        FILE *config2 = fopen(".givit/config2", "w");
        fgets(temp, MAX_CONFIG_LENGTH, config);
        check_valid(argv[3]);
        check_valid(temp);
        fprintf(config2, "%s\n", temp);
        fprintf(config2, "%s\n", argv[3]);
        fgets(temp, MAX_CONFIG_LENGTH, config);
        copy_therest(config, config2);
        fclose(config);
        fclose(config2);
        remove(".givit/config");
        rename(".givit/config2", ".givit/config");
        return 0;
    } else if (strncmp(argv[2], "alias", 5) == 0) { //alias.<alias name> "command"
        char *alias_name = argv[2] + 6;
        char command[MAX_CONFIG_LENGTH];
        FILE *config = fopen(".givit/aliases", "a");
        strcpy(command, argv[3]);
        fprintf(config, "\"%s\" \"%s\"\n", alias_name, command);
        return 0;
    } else {
        perror("invalid command! you must choose between user.name and user.email or set an alias");
        return 1;

    }
}

int read_adminator(char *username, char *email) {
    const char *home = getenv("HOME");
    chdir(home);
    FILE *admin = fopen("CLionProjects/Givit/config", "r");
    fgets(username, MAX_CONFIG_LENGTH, admin);
    fgets(email, MAX_CONFIG_LENGTH, admin);
    check_valid(username);
    check_valid(email);
    return 0;
}

bool check_file_directory_exists(char *filepath) {
    DIR *dir = opendir(".givit/files");
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, filepath) == 0) return true;
    }
    closedir(dir);

    return false;
}

int track_file(char *filepath) {
    check_valid(filepath);
    if (is_tracked(filepath)) return 0;
    FILE *file = fopen(".givit/tracks", "a");
    if (file == NULL) return 1;
    fprintf(file, "%s\n", filepath);
    return 0;
}

bool is_tracked(char *filepath) {
    FILE *file = fopen(".givit/tracks", "r");
    if (file == NULL) return false;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL) {
        check_valid(line);
        if (strcmp(line, filepath) == 0) return true;
    }
    fclose(file);

    return false;
}

bool is_staged(char *filepath) {
    system("cp .givit/staging .givit/last_staging");
    FILE *file = fopen(".givit/staging", "r");
    if (file == NULL) return false;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL) {
        check_valid(line);
        if (strcmp(line, filepath) == 0) return true;
    }
    fclose(file);

    return false;
}

bool file_is_changed(FILE *file_befor, FILE *file_after) {
    char line_befor[MAX_LINE_LENGTH], line_after[MAX_LINE_LENGTH];
    while (fgets(line_befor, sizeof(line_befor), file_befor) != NULL) {
        check_valid(line_befor);
        if (fgets(line_after, sizeof(line_after), file_after) == NULL) return true;
        check_valid(line_after);
        if (strcmp(line_befor, line_after) != 0) return true;
    }
    if (fgets(line_after, sizeof(line_after), file_after) != NULL) return true;
    return false;
}

int run_reset(int argc, char *const argv[]) {
    if (argc < 3) {
        perror("please specify a file");
        return 1;
    }
    int i = 2;
    if (strcmp(argv[2], "-f") == 0) {
        i++;
    }
    if (strcmp(argv[2], "-undo") == 0) {
        system("rm -rf .givit/staged/* > /dev/null 2>&1");
        system("cp -R .givit/1_staged/* .givit/staged > /dev/null 2>&1");
        system("rm -rf .givit/1_staged/* > /dev/null 2>&1");
        system("cp -R .givit/2_staged/* .givit/1_staged > /dev/null 2>&1");
        system("rm -rf .givit/2_staged/* > /dev/null 2>&1");
        system("cp -R .givit/3_staged/* .givit/2_staged > /dev/null 2>&1");
        system("rm -rf .givit/3_staged/* > /dev/null 2>&1");
        system("cp -R .givit/4_staged/* .givit/3_staged > /dev/null 2>&1");
        system("rm -rf .givit/4_staged/* > /dev/null 2>&1");
        system("cp -R .givit/5_staged/* .givit/4_staged > /dev/null 2>&1");
        system("rm -rf .givit/5_staged/* > /dev/null 2>&1");
        system("cp -R .givit/6_staged/* .givit/5_staged > /dev/null 2>&1");
        system("rm -rf .givit/6_staged/* > /dev/null 2>&1");
        system("cp -R .givit/7_staged/* .givit/6_staged > /dev/null 2>&1");
        system("rm -rf .givit/7_staged/* > /dev/null 2>&1");
        system("cp -R .givit/8_staged/* .givit/7_staged > /dev/null 2>&1");
        system("rm -rf .givit/8_staged/* > /dev/null 2>&1");
        system("cp -R .givit/9_staged/* .givit/8_staged > /dev/null 2>&1");
        system("rm -rf .givit/9_staged/* > /dev/null 2>&1");
        system("cp -R .givit/10_staged/* .givit/9_staged > /dev/null 2>&1");
        system("rm -rf .givit/10_staged/* > /dev/null 2>&1");
        return 0;
    }
    for (i; i < argc; ++i) {
        char main_path[1024];
        getcwd(main_path, sizeof(main_path));
        strcat(main_path, "/");
        strcat(main_path, argv[i]);
        FILE *file = fopen(main_path, "r");
        DIR *dir = opendir(main_path);
        if (file != NULL || dir != NULL) {
            unstage_file_or_directory(argv[i], ".");
        } else {
            perror("file not found");
            return 1;
        }
    }
}

void remove_from_staging(
        char *filepath) { // if its a file, remove it from staging and if its directory removes all of files into it from directory
    FILE *staging = fopen(".givit/staging", "r");
    FILE *staging2 = fopen(".givit/staging2", "w");
    char temp[1024];
    while (fgets(temp, MAX_LINE_LENGTH, staging) != NULL) {
        check_valid(temp);
        if (strcmp(temp, filepath) != 0) {
            fprintf(staging2, "%s\n", temp);
        }
    }
    fclose(staging);
    fclose(staging2);
    remove(".givit/staging");
    rename(".givit/staging2", ".givit/staging");
}

void unstage_file_or_directory(char *filepath, char *directory_path) {
    char temp[MAX_LINE_LENGTH];
    strcpy(temp, directory_path);
    strcat(temp, "/");
    strcat(temp, filepath);
    if (is_file_or_directory(temp) == 1) {
        FILE *staging = fopen(".givit/staging", "r");
        FILE *staging2 = fopen(".givit/staging2", "w");
        char template[1024];
        while (fgets(template, MAX_LINE_LENGTH, staging) != NULL) {
            check_valid(template);
            if (strcmp(template, temp) != 0) {
                fprintf(staging2, "%s\n", template);
            }
        }
        fclose(staging);
        fclose(staging2);
        remove(".givit/staging");
        rename(".givit/staging2", ".givit/staging");
        char path[1024];
        strcpy(path, ".givit/staged/");
        strcat(path, temp);
        remove(path);
    } else if (is_file_or_directory(temp) == 2) {
        DIR *dir = opendir(temp);
        struct dirent *entry;
        if (dir == NULL) {
            perror("Error opening current directory");
            return;
        }
        while ((entry = readdir(dir)) != NULL) {
            if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) &&
                (strcmp(entry->d_name, ".givit") != 0)) {
                unstage_file_or_directory(entry->d_name, temp);
            }
        }
        closedir(dir);
    } else {
        perror("file not found");
        return;
    }
}

int run_status(int argc, char *const argv[]) {
    DIR *dir = opendir(".");
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) &&
            (strcmp(entry->d_name, ".givit") != 0)) {
            char temp[1024];
            strcpy(temp, "./");
            strcat(temp, entry->d_name);
            print_status(temp, 0);
        }
    }
    closedir(dir);

    return false;
}

void print_status(char *filepath,
                  int depth) {//for all files in folder prints : <file path> <+ for staged - for not staged> <M if file is changed from staged version/ D if file is deleted / A for else>
    if (is_file_or_directory(filepath) == 1) {
        char filename[1024];
        strcpy(filename, filename_maker(filepath));
        forLoop(depth, i) {
            printf("\t");
        }
        printf("└─── ");
        if (is_staged(filepath)) {
            printf("%s +", filename);
        } else {
            printf("%s -", filename);
        }
        if (is_tracked(filepath)) {
            FILE *file = fopen(filepath, "r");
            if (file == NULL) {
                printf("D\n");
                return;
            }
            if (is_changed_last_commit(filepath)) {
                printf("M\n");
                return;
            } else {
                printf("A\n");
                return;
            }
        } else {
            printf("file is not tracked!\n");
        }
    } else if (is_file_or_directory(filepath) == 2) {
        char dirname[1024];
        strcpy(dirname, filename_maker(filepath));
        DIR *dir = opendir(filepath);
        struct dirent *entry;
        if (dir == NULL) {
            perror("Error opening current directory");
            return;
        }
        forLoop(depth, i) {
            printf("\t");
        }
        printf("└─── \e[37m%s\e[m\n", dirname);
        while ((entry = readdir(dir)) != NULL) {
            if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) &&
                (strcmp(entry->d_name, ".givit") != 0)) {
                char temp[1024];
                strcpy(temp, filepath);
                strcat(temp, "/");
                strcat(temp, entry->d_name);
                print_status(temp, depth + 1);
            }
        }
        closedir(dir);
    } else {
        perror("file not found");
        return;
    }

}

void print_info(FILE *file) {
    char temp[MAX_CONFIG_LENGTH];
    FILE *config = fopen(".givit/config", "r");
    while (fgets(temp, MAX_CONFIG_LENGTH, config) != NULL) {
        fprintf(file, "%s", temp);
    }
    fclose(config);
}

Commit *create_last_commit_dir(char *message, int hour, int minute, int second,
                               Commit *head) { // sets current time for commit id / sets user and email from config file / sets branch name / create a directory for creating backup
    FILE *config = fopen(".givit/config", "r");
    char id[MAX_CONFIG_LENGTH];
    sprintf(id, "%d-%d-%d", hour, minute, second);
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    char cwd2[1024];
    getcwd(cwd2, sizeof(cwd2));
    strcat(cwd, "/.givit/commits/");
    strcat(cwd2, "/.givit/commits/");
    strcat(cwd, id);
    mkdir(cwd, 0755);
    head = createNode(cwd, message, hour, minute, second, head);
    //copy all files in .givit/staged into .givit/commits/<id>:
    head = fix_commits(head);
    char cmd[1024];
    char *src;
    char *dest = cwd;
    if (head->prev_attached_commit != NULL) {
        char prev_id[1024];
        sprintf(prev_id, "%d-%d-%d", head->prev_attached_commit->hour, head->prev_attached_commit->minute,
                head->prev_attached_commit->second);
        strcat(cwd2, prev_id);
        strcat(cwd2, "/*");
        src = cwd2;
        strcpy(cmd, "cp -r ");
        strcat(cmd, src);
        strcat(cmd, " ");
        strcat(cmd, dest);
        system(cmd);
    }
    cmd[0] = '\0';
    src = ".givit/staged/*";
    strcpy(cmd, "cp -r ");
    strcat(cmd, src);
    strcat(cmd, " ");
    strcat(cmd, dest);
    system(cmd);
    return head;
}

int run_commit(int argc, char *const argv[], int hour, int minute, int second, Commit *head) {
    if (argc < 3) {
        perror("please enter a valid message");
        return 1;
    }
    FILE *booli = fopen(".givit/bool", "r");
    char temp[MAX_LINE_LENGTH];
    fgets(temp, MAX_LINE_LENGTH, booli);
    check_valid(temp);
    if (strcmp(temp, "0") == 0) {
        perror("you can't set commit you must fix files in staging area first!\n");
        return 1;
    }
    head = loadList(".givit/commitsdb");
    if ((strcmp(argv[2], "-m") != 0) && (strcmp(argv[2], "-s") != 0)) {
        perror("please enter a valid message");
    }
    if (strlen(argv[3]) > 70) {
        perror("please enter a valid message");
    }
    char message[MAX_COMMIT_MESSAGE_LENGTH];
    if (strcmp(argv[2], "-m") == 0) {
        strcpy(message, argv[3]);
    } else {
        find_and_replace_shortcut(argv[3], message);
    }
    head = create_last_commit_dir(message, hour, minute, second, head);
    saveList(head, ".givit/commitsdb");
    system("cp .givit/staging .givit/last_staging");
    system("cp -r .givit/staged/ .givit/last_commit");
    system("rm -r .givit/staged/*");
    return 0;
}

void find_and_replace_shortcut(char *old_shortcut, char *message) {
    FILE *shortcuts = fopen(".givit/shortcuts", "r");
    char temp[MAX_LINE_LENGTH];
    char mess[MAX_LINE_LENGTH];
    char shrt[MAX_LINE_LENGTH];
    while (fgets(temp, MAX_LINE_LENGTH, shortcuts) != NULL) {
        check_valid(temp);
        sscanf(temp, "\"%[^\"]\" \"%[^\"]\"", shrt, mess);
        if (strcmp(shrt, old_shortcut) == 0) {
            strcpy(message, mess);
            return;
        }
    }
    perror("shortcut not found");
}

int set_shortcut(int argc, char *const argv[]) {
    if (argc < 6) {
        perror("please enter a valid shortcut");
        return 1;
    }
    FILE *shortcuts = fopen(".givit/shortcuts", "a");
    fprintf(shortcuts, "\"%s\" \"%s\"\n", argv[5], argv[3]);
    printf("shortcut added!\n");
    return 0;
}

int replace_shortcut(int argc, char *const argv[]) {
    if (argc < 6) {
        perror("please enter a valid shortcut");
        return 1;
    }
    FILE *shortcuts = fopen(".givit/shortcuts", "r");
    FILE *shortcuts2 = fopen(".givit/shortcuts2", "w");
    char temp[MAX_LINE_LENGTH];
    char mess[MAX_LINE_LENGTH], shrt[MAX_LINE_LENGTH];
    while (fgets(temp, MAX_LINE_LENGTH, shortcuts) != NULL) {
        check_valid(temp);
        sscanf(temp, "\"%[^\"]\" \"%[^\"]\"", shrt, mess);
        if (strcmp(shrt, argv[5]) == 0) {
            fprintf(shortcuts2, "\"%s\" \"%s\"\n", shrt, argv[3]);
        } else {
            fprintf(shortcuts2, "\"%s\" \"%s\"\n", shrt, mess);
        }
    }
    fclose(shortcuts);
    fclose(shortcuts2);
    remove(".givit/shortcuts");
    rename(".givit/shortcuts2", ".givit/shortcuts");
    printf("shortcut replaced!\n");
    return 0;
}

int remove_shortcut(int argc, char *const argv[]) {
    if (argc < 4) {
        perror("please enter a valid shortcut");
        return 1;
    }
    FILE *shortcuts = fopen(".givit/shortcuts", "r");
    FILE *shortcuts2 = fopen(".givit/shortcuts2", "w");
    char temp[MAX_LINE_LENGTH];
    char mess[MAX_LINE_LENGTH], shrt[MAX_LINE_LENGTH];
    char badmess[MAX_LINE_LENGTH], badshrt[MAX_LINE_LENGTH];
    while (fgets(temp, MAX_LINE_LENGTH, shortcuts) != NULL) {
        check_valid(temp);
        sscanf(temp, "\"%[^\"]\" \"%[^\"]\"", shrt, mess);
        if (strcmp(shrt, argv[3]) != 0) {
            fprintf(shortcuts2, "\"%s\" \"%s\"\n", shrt, mess);
        } else {
            strcpy(badmess, mess);
            strcpy(badshrt, shrt);
        }
    }
    fclose(shortcuts);
    fclose(shortcuts2);
    remove(".givit/shortcuts");
    rename(".givit/shortcuts2", ".givit/shortcuts");
    printf("shortcut named %s with message %s successfully removed!\n", badshrt, badmess);
    return 0;
}

int run_log(int argc, char *const argv[], Commit *head) {
    head = loadList(".givit/commitsdb");
    if (head == NULL) {
        perror("no commit has been made yet");
        return 1;
    }
    printf("\e[37m><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<\e[m\n");
    Commit *node = head;
    if (argc > 2) {
        if (strcmp(argv[2], "-n") == 0) {
            int n = atoi(argv[3]);
            forLoop(n, i) {
                printf("commit id: %d\n", node->commit_ID);
                printf("message: %s\n", node->message);
                printf("user: %s\n", node->userName);
                printf("email: %s\n", node->userEmail);
                printf("branch: %s\n", node->branch);
                printf("parent branch: %s\n", node->parent_branch);
                printf("time: %d:%d:%d\n", node->hour, node->minute, node->second);
                printf("directory path: %s\n", node->dir_path);
                printf("\e[37m><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<\e[m\n");
                node = node->prev_commit;
            }
            return 0;
        }
        if (strcmp(argv[2], "-branch") == 0) {
            char branch[MAX_LINE_LENGTH];
            strcpy(branch, argv[3]);
            while (node != NULL) {
                if (strcmp(node->branch, branch) == 0) {
                    printf("commit id: %d\n", node->commit_ID);
                    printf("message: %s\n", node->message);
                    printf("user: %s\n", node->userName);
                    printf("email: %s\n", node->userEmail);
                    printf("branch: %s\n", node->branch);
                    printf("parent branch: %s\n", node->parent_branch);
                    printf("time: %d:%d:%d\n", node->hour, node->minute, node->second);
                    printf("directory path: %s\n", node->dir_path);
                    printf("\e[37m><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<\e[m\n");
                }
                node = node->prev_commit;
            }
            return 0;
        }
        if (strcmp(argv[2], "-author") == 0) {
            char author[MAX_LINE_LENGTH];
            strcpy(author, argv[3]);
            while (node != NULL) {
                if (strcmp(node->userName, author) == 0) {
                    printf("commit id: %d\n", node->commit_ID);
                    printf("message: %s\n", node->message);
                    printf("user: %s\n", node->userName);
                    printf("email: %s\n", node->userEmail);
                    printf("branch: %s\n", node->branch);
                    printf("parent branch: %s\n", node->parent_branch);
                    printf("time: %d:%d:%d\n", node->hour, node->minute, node->second);
                    printf("directory path: %s\n", node->dir_path);
                    printf("\e[37m><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<\e[m\n");
                }
                node = node->prev_commit;
            }
            return 0;
        }
        if (strcmp(argv[2], "-since") == 0) {
            int hour = atoi(argv[3]);
            int minute = atoi(argv[4]);
            int second = atoi(argv[5]);
            while (node != NULL) {
                if ((node->hour > hour) || ((node->hour == hour) && (node->minute > minute)) ||
                    ((node->hour == hour) && (node->minute == minute) && (node->second > second))) {
                    printf("commit id: %d\n", node->commit_ID);
                    printf("message: %s\n", node->message);
                    printf("user: %s\n", node->userName);
                    printf("email: %s\n", node->userEmail);
                    printf("branch: %s\n", node->branch);
                    printf("parent branch: %s\n", node->parent_branch);
                    printf("time: %d:%d:%d\n", node->hour, node->minute, node->second);
                    printf("directory path: %s\n", node->dir_path);
                    printf("\e[37m><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<\e[m\n");
                }
                node = node->prev_commit;
            }
            return 0;
        }
        if (strcmp(argv[2], "-before") == 0) {
            int hour = atoi(argv[3]);
            int minute = atoi(argv[4]);
            int second = atoi(argv[5]);
            while (node != NULL) {
                if ((node->hour < hour) || ((node->hour == hour) && (node->minute < minute)) ||
                    ((node->hour == hour) && (node->minute == minute) && (node->second < second))) {
                    printf("commit id: %d\n", node->commit_ID);
                    printf("message: %s\n", node->message);
                    printf("user: %s\n", node->userName);
                    printf("email: %s\n", node->userEmail);
                    printf("branch: %s\n", node->branch);
                    printf("parent branch: %s\n", node->parent_branch);
                    printf("time: %d:%d:%d\n", node->hour, node->minute, node->second);
                    printf("directory path: %s\n", node->dir_path);
                    printf("\e[37m><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<\e[m\n");
                }
                node = node->prev_commit;
            }
            return 0;
        }
        if (strcmp(argv[2], "-search") ==
            0) {// argv[3] ,argv[4] ... argv[10]. are words, if any of them was in commit message, print the commit
            while (node != NULL) {
                int flag = 0;
                for (int i = 3; i < argc; ++i) {
                    if (strstr(node->message, argv[i]) != NULL) {
                        flag = 1;
                        break;
                    }
                }
                if (flag) {
                    printf("commit id: %d\n", node->commit_ID);
                    printf("message: %s\n", node->message);
                    printf("user: %s\n", node->userName);
                    printf("email: %s\n", node->userEmail);
                    printf("branch: %s\n", node->branch);
                    printf("parent branch: %s\n", node->parent_branch);
                    printf("time: %d:%d:%d\n", node->hour, node->minute, node->second);
                    printf("directory path: %s\n", node->dir_path);
                    printf("\e[37m><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<\e[m\n");
                }
                node = node->prev_commit;
            }
            return 0;
        }
    }
    while (node != NULL) {
        printf("commit id: %d\n", node->commit_ID);
        printf("message: %s\n", node->message);
        printf("user: %s\n", node->userName);
        printf("email: %s\n", node->userEmail);
        printf("branch: %s\n", node->branch);
        printf("parent branch: %s\n", node->parent_branch);
        printf("time: %d:%d:%d\n", node->hour, node->minute, node->second);
        printf("directory path: %s\n", node->dir_path);
        printf("\e[37m><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<\e[m\n");
        node = node->prev_commit;
    }
    return 0;
}


Commit *create_newbranch_commit(Commit *node, char *branch, char *parent_branch, int hour, int minute, int second,
                                Commit *head) {
    char message[MAX_COMMIT_MESSAGE_LENGTH];
    strcpy(message, "branch ");
    strcat(message, branch);
    //copy last commit into staged:
    char cmd[1024];
    strcpy(cmd, "cp -r ");
    strcat(cmd, node->dir_path);
    strcat(cmd, "/*");
    strcat(cmd, " ");
    strcat(cmd, ".givit/staged");
    head = create_last_commit_dir(message, hour, minute, second, head);
    head->prev_attached_commit = node;
    strcpy(head->branch, branch);
    strcpy(head->parent_branch, parent_branch);

    return head;
}

int
run_branch(int argc, char *const argv[], int hour, int minute, int second, Commit *head) {
    head = loadList(".givit/commitsdb");
    if (argc < 2) {
        perror("please enter a valid command");
        return 1;
    }
    if (argc == 2) {
        FILE *branchs = fopen(".givit/branchs", "r");
        char temp[1024];
        char parent[1024];
        char branch[1024];
        while (fgets(temp, 1024, branchs) != NULL) {
            check_valid(temp);
            sscanf(temp, "%s %s", branch, parent);
            printf("%s attached to: %s\n", branch, parent);
        }
        return 0;
    }
    FILE *branchs = fopen(".givit/branchs", "r");
    char temp[1024];
    char parent[1024];
    char branch[1024];
    while (fgets(temp, 1024, branchs) != NULL) {
        check_valid(temp);
        sscanf(temp, "%s %s", branch, parent);
        if (strcmp(branch, argv[2]) == 0) {
            perror("branch already exists");
            return 1;
        }
    }
    FILE *branchs2 = fopen(".givit/branchs", "a");
    branchs = fopen(".givit/branchs", "r");
    FILE *config = fopen(".givit/config", "r");
    fgets(temp, 1024, config);
    fgets(temp, 1024, config);
    fgets(temp, 1024, config);
    check_valid(temp);
    char trash[1024], master[1024];
    sscanf(temp, "branch: %s %s", master, trash);
    printf("current head branch: %s\n", master);
    fprintf(branchs2, "%s %s\n", argv[2], master);
    fclose(branchs);
    fclose(branchs2);
    // here we must commit the last commit in parent branch but in new branch:
    Commit *node = head;
    while (node != NULL) {
        if (strcmp(node->branch, master) == 0) {
            break;
        }
        node = node->prev_commit;
    }
    head = create_newbranch_commit(node, argv[2], trash, hour, minute, second, head);
    saveList(head, ".givit/commitsdb");
    return 0;
}

int run_checkout(int argc, char *const argv[], int hour, int minute, int second, Commit *head) {
    head = loadList(".givit/commitsdb");
    if (argc < 3) {
        perror("please enter a valid command");
        return 1;
    }
    FILE *branchs = fopen(".givit/branchs", "r");
    char temp[1024];
    while (fgets(temp, 1024, branchs) != NULL) {
        check_valid(temp);
        char parent_branch[1024], current_branch[1024];
        sscanf(temp, "%s %s", current_branch, parent_branch);
        if (strcmp(current_branch, argv[2]) == 0) {
            char template[1024];
            FILE *config = fopen(".givit/config", "r");
            FILE *config2 = fopen(".givit/config2", "w");
            fgets(template, 1024, config);
            check_valid(template);
            fprintf(config2, "%s\n", template);
            fgets(template, 1024, config);
            check_valid(template);
            fprintf(config2, "%s\n", template);
            fgets(template, 1024, config);
            fprintf(config2, "branch: %s %s\n", argv[2], parent_branch);
            copy_therest(config, config2);
            fclose(config);
            fclose(config2);
            remove(".givit/config");
            rename(".givit/config2", ".givit/config");
            // find last commit in this branch:
            Commit *node = head;
            while (node != NULL) {
                if (strcmp(node->branch, argv[2]) == 0) {
                    break;
                }
                node = node->prev_commit;
            }
            //remove all file in current project but ,givit/
            DIR *dir = opendir(".");
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) &&
                    (strcmp(entry->d_name, ".givit") != 0)) {
                    char temp[1024];
                    strcpy(temp, "./");
                    strcat(temp, entry->d_name);
                    if (entry->d_type == DT_DIR) {
                        char cmd[1024];
                        strcpy(cmd, "rm -r ");
                        strcat(cmd, temp);
                        system(cmd);
                    } else {
                        remove(temp);
                    }
                }
            }
            //copy all files in node commit into current project:
            char cmd[1024];
            strcpy(cmd, "cp -r ");
            strcat(cmd, node->dir_path);
            strcat(cmd, "/* .");
            system(cmd);
            return 0;
        }
    }
    if (strncmp(argv[2], "HEAD", 4) == 0) {
        if (strncmp(argv[2], "HEAD_", 5) == 0) {
            int x = atoi(argv[2] + 5);
            Commit *node = head;
            forLoop(x, i) {
                if (node->prev_commit == NULL) {
                    perror("you can't checkout that many commits");
                    return 1;
                }
                if (node->merged_ID != 0) {
                    perror("you can't checkout a merged commit");
                    return 1;
                }
                node = node->prev_commit;
            }
            //remove all file in current project but ,givit/
            DIR *dir = opendir(".");
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) &&
                    (strcmp(entry->d_name, ".givit") != 0)) {
                    char temp[1024];
                    strcpy(temp, "./");
                    strcat(temp, entry->d_name);
                    if (entry->d_type == DT_DIR) {
                        char cmd[1024];
                        strcpy(cmd, "rm -r ");
                        strcat(cmd, temp);
                        system(cmd);
                    } else {
                        remove(temp);
                    }
                }
            }
            //copy all files in node commit into current project:
            char cmd[1024];
            strcpy(cmd, "cp -r ");
            strcat(cmd, node->dir_path);
            strcat(cmd, "/* .");
            system(cmd);
            return 0;
        }

        Commit *node = head;
        //remove all file in current project but ,givit/
        DIR *dir = opendir(".");
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) &&
                (strcmp(entry->d_name, ".givit") != 0)) {
                char temp[1024];
                strcpy(temp, "./");
                strcat(temp, entry->d_name);
                if (entry->d_type == DT_DIR) {
                    char cmd[1024];
                    strcpy(cmd, "rm -r ");
                    strcat(cmd, temp);
                    system(cmd);
                } else {
                    remove(temp);
                }
            }
        }
        //copy all files in node commit into current project:
        char cmd[1024];
        strcpy(cmd, "cp -r ");
        strcat(cmd, node->dir_path);
        strcat(cmd, "/* .");
        system(cmd);
        return 0;
    }
    //its not branch and its old commit:
    Commit *node = head;
    while (node != NULL) {
        if (node->commit_ID == atoi(argv[2])) {
            //remove all file in current project but ,givit/
            DIR *dir = opendir(".");
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) &&
                    (strcmp(entry->d_name, ".givit") != 0)) {
                    char temp[1024];
                    strcpy(temp, "./");
                    strcat(temp, entry->d_name);
                    if (entry->d_type == DT_DIR) {
                        char cmd[1024];
                        strcpy(cmd, "rm -r ");
                        strcat(cmd, temp);
                        system(cmd);
                    } else {
                        remove(temp);
                    }
                }
            }
            //copy all files in node commit into current project:
            char cmd[1024];
            strcpy(cmd, "cp -r ");
            strcat(cmd, node->dir_path);
            strcat(cmd, "/* .");
            system(cmd);
            Commit *check_head = head;
            while (strcmp(check_head->branch, node->branch) != 0) {
                check_head = check_head->prev_commit;
            }
            if (check_head->commit_ID != node->commit_ID) {
                FILE *booli = fopen(".givit/booli", "w");
                fprintf(booli, "0");
                fclose(booli);
            } else {
                FILE *booli = fopen(".givit/booli", "w");
                fprintf(booli, "1");
                fclose(booli);
            }
            return 0;
        }
        node = node->prev_commit;
    }
}

int run_revert(int argc, char *const argv[], int hour, int minute, int second, Commit *head) {//checkout and commit
    head = loadList(".givit/commitsdb");
    if (argc < 3) {
        perror("please enter a valid command");
        return 1;
    }
    char message[MAX_COMMIT_MESSAGE_LENGTH];
    int commit_id = 0;
    if (strcmp(argv[2], "-n") == 0) {
        if (argc == 3) {
            commit_id = head->commit_ID;
        } else if (argc == 4 && strncmp(argv[3], "HEAD", 4) == 0) { // HEAD-X
            int x = atoi(argv[3] + 5);
            Commit *node = head;
            forLoop(x, i) {
                if (node->prev_commit == NULL) {
                    perror("you can't revert that many commits");
                    return 1;
                }
                if (node->merged_ID != 0) {
                    perror("you can't revert a merged commit");
                    return 1;
                }
                node = node->prev_commit;
            }
            commit_id = node->commit_ID;
        } else if (argc == 4) {
            commit_id = atoi(argv[3]);
        } else {
            perror("please enter a valid command");
            return 1;
        }
    } else {
        if (strcmp(argv[2], "-m") == 0) {
            strcpy(message, argv[3]);
            //HEAD handle:
            if (argc == 5 && strncmp(argv[4], "HEAD", 4) == 0) { // HEAD-X
                int x = atoi(argv[4] + 5);
                Commit *node = head;
                forLoop(x, i) {
                    if (node->prev_commit == NULL) {
                        perror("you can't revert that many commits");
                        return 1;
                    }
                    if (node->merged_ID != 0) {
                        perror("you can't revert a merged commit");
                        return 1;
                    }
                    node = node->prev_commit;
                }
                commit_id = node->commit_ID;
            } else {
                commit_id = atoi(argv[4]);
            }
        } else {
            if (strncmp(argv[2], "HEAD", 4) == 0) {
                int x = atoi(argv[2] + 5);
                Commit *node = head;
                forLoop(x, i) {
                    if (node->prev_commit == NULL) {
                        perror("you can't revert that many commits");
                        return 1;
                    }
                    if (node->merged_ID != 0) {
                        perror("you can't revert a merged commit");
                        return 1;
                    }
                    node = node->prev_commit;
                }
                commit_id = node->commit_ID;
                strcpy(message, node->message);
            } else {
                commit_id = atoi(argv[2]);
                Commit *node = head;
                while (node != NULL) {
                    if (node->commit_ID == commit_id) {
                        strcpy(message, node->message);
                        break;
                    }
                    node = node->prev_commit;
                }
            }
        }
    }
    char cmd[1024], commit_id_str[1024];
    sprintf(commit_id_str, "%d", commit_id);
    strcpy(cmd, "Givit checkout ");
    strcat(cmd, commit_id_str);
    system(cmd);
    if (strcmp(argv[2], "-n") != 0) {
        system("Givit add *");
        cmd[0] = '\0';
        strcpy(cmd, "Givit commit -m ");
        strcat(cmd, message);
        system(cmd);
    }
}

bool line_is_empty(char *line) {
    forLoop(strlen(line), i) {
        if (line[i] != ' ' && line[i] != '\n' && line[i] != '\t') {
            return false;
        }
    }
    return true;
}

void create_tag(char *tag_name, int commit_id, char *message, int hour, int minute, int second, char *user_name,
                char *user_email) {
    FILE *tags = fopen(".givit/tags", "a");
    FILE *config = fopen(".givit/config", "r");
    char user_name_config[1024], user_email_config[1024], temp[1024];
    fgets(temp, 1000, config);
    check_valid(temp);
    strcpy(user_name_config, temp);
    fgets(temp, 1000, config);
    check_valid(temp);
    strcpy(user_email_config, temp);
    fprintf(tags, "\"%s\" %d \"%s\" \"%s\" %d-%d-%d \"%s\"\n", tag_name, commit_id, user_name_config, user_email_config,
            hour, minute, second, message);
    fclose(tags);
    sort_tags();
}

bool tag_exist(char *tag_name) {
    FILE *tags = fopen(".givit/tags", "r");
    char temp[MAX_LINE_LENGTH];
    while (fgets(temp, MAX_LINE_LENGTH, tags) != NULL) {
        check_valid(temp);
        char tag[1024];
        sscanf(temp, "\"%[^\"]\"", tag);
        if (strcmp(tag, tag_name) == 0) {
            return true;
        }
    }
    return false;
}

int run_tag(int argc, char *const argv[], int hour, int minute, int second,
            Commit *head) {//neogit tag -a <tag-name> [-m <message>] [-c <commit-id>] [-f] //tags file must be sorted by alphabet. // data: tag_name - commit_id - username - date - message
    head = loadList(".givit/commitsdb");
    char tag_name[1024];
    if (argc == 2) {
        FILE *tags = fopen(".givit/tags", "r");
        char temp[MAX_LINE_LENGTH];
        while (fgets(temp, MAX_LINE_LENGTH, tags) != NULL) {
            check_valid(temp);
            char tag[1024];
            sscanf(temp, "\"%[^\"]\"", tag);
            printf("%s\n", tag);
        }
        return 0;
    }
    if (strcmp(argv[2], "show") == 0) {
        strcpy(tag_name, argv[3]);
        FILE *tags = fopen(".givit/tags", "r");
        char temp[1024], user_name[1024], user_email[1024], message[1024];
        int commit_id, hour_scan, minute_scan, second_scan;
        while (fgets(temp, 1000, tags) != NULL) {
            check_valid(temp);
            sscanf(temp, "\"%[^\"]\" %d \"%[^\"]\" \"%[^\"]\" %d-%d-%d \"%[^\"]\"", tag_name, &commit_id, user_name,
                   user_email, &hour_scan,
                   &minute_scan, &second_scan, message);
            if (strcmp(argv[3], tag_name) == 0) {
                printf("\e[37mGivit tag show %s:\e[m\n\ttag %s\n\tcommit %d\n\tAuthor: %s %s\n\tDate: %d:%d:%d\n\tMessage: %s\n",
                       tag_name, tag_name, commit_id, user_name, user_email, hour_scan, minute_scan, second_scan,
                       message);
                return 0;
            }
        }
    }
    if (strcmp(argv[2], "-a") == 0) {
        strcpy(tag_name, argv[3]);
        // handle -m, -c, -f flags:
        if (argc == 4) {//empty message , head commit
            if (tag_exist(tag_name)) {
                perror("tag already exists");
                return 1;
            }
            create_tag(tag_name, head->commit_ID, "", hour, minute, second, head->userName, head->userEmail);
            return 0;
        }
        if (strcmp(argv[4], "-m") == 0) {
            char message[1024];
            strcpy(message, argv[5]);
            //handle -c -f flags:
            if (argc == 6) {//head commit
                if (tag_exist(tag_name)) {
                    perror("tag already exists");
                    return 1;
                }
                create_tag(tag_name, head->commit_ID, message, hour, minute, second, head->userName, head->userEmail);
                return 0;
            }
            if (strcmp(argv[6], "-c") == 0) {
                int commit_id = atoi(argv[7]);
                //handle -f flag:
                if (argc == 8) {
                    if (tag_exist(tag_name)) {
                        perror("tag already exists");
                        return 1;
                    }
                    create_tag(tag_name, commit_id, message, hour, minute, second, head->userName, head->userEmail);
                    return 0;
                }
                if (strcmp(argv[8], "-f") == 0) {
                    //force tag
                    create_tag(tag_name, commit_id, message, hour, minute, second, head->userName, head->userEmail);
                    return 0;
                }
                return 0;
            }
            if (strcmp(argv[6], "-f") == 0) {
                //force tag
                create_tag(tag_name, head->commit_ID, message, hour, minute, second, head->userName, head->userEmail);
                return 0;
            }
            return 1;
        }
        if (strcmp(argv[4], "-c") == 0) {//empty message
            // create tag with commit id
            int commit_id = atoi(argv[5]);
            //handle -f flag:
            if (argc == 6) {
                if (tag_exist(tag_name)) {
                    perror("tag already exists");
                    return 1;
                }
                create_tag(tag_name, commit_id, "", hour, minute, second, head->userName, head->userEmail);
                return 0;
            }
            if (strcmp(argv[6], "-f") == 0) {
                //force tag
                create_tag(tag_name, commit_id, "", hour, minute, second, head->userName, head->userEmail);
                return 0;
            }
            return 1;
        }
        if (strcmp(argv[4], "-f") == 0) {
            // force tag
            create_tag(tag_name, head->commit_ID, "", hour, minute, second, head->userName, head->userEmail);
            return 0;
        }
    } else {
        perror("please enter a valid command");
        return 1;
    }
}

void sort_tags() {
    FILE *tags = fopen(".givit/tags", "r");
    FILE *tags2 = fopen(".givit/tags2", "w");
    char temp[MAX_LINE_LENGTH];
    char tags_array[1024][1024];
    int i = 0;
    while (fgets(temp, MAX_LINE_LENGTH, tags) != NULL) {
        check_valid(temp);
        char tag[1024];
        sscanf(temp, "\"%[^\"]\"", tag);
        strcpy(tags_array[i], tag);
        i++;
    }
    for (int j = 0; j < i; ++j) {
        for (int k = j + 1; k < i; ++k) {
            if (strcmp(tags_array[j], tags_array[k]) > 0) {
                char temp[1024];
                strcpy(temp, tags_array[j]);
                strcpy(tags_array[j], tags_array[k]);
                strcpy(tags_array[k], temp);
            }
        }
    }
    for (int j = 0; j < i; ++j) {
        fseek(tags, 0, SEEK_SET);
        while (fgets(temp, MAX_LINE_LENGTH, tags) != NULL) {
            check_valid(temp);
            char tag[1024];
            sscanf(temp, "\"%[^\"]\"", tag);
            if (strcmp(tag, tags_array[j]) == 0) {
                fprintf(tags2, "%s\n", temp);
            }
        }
    }
    fclose(tags);
    fclose(tags2);
    remove(".givit/tags");
    rename(".givit/tags2", ".givit/tags");
}

void compare_commit_directories(char *commit1_path, char *commit2_path) {
    DIR *dir1 = opendir(commit1_path);
    DIR *dir2 = opendir(commit2_path);
    struct dirent *entry1, *entry2;

    if (dir1 == NULL || dir2 == NULL) {
        printf("Error opening directories.\n");
        return;
    }

    while ((entry1 = readdir(dir1)) != NULL) {
        char file1_path[1024], file2_path[1024];

        if (strcmp(entry1->d_name, ".") == 0 || strcmp(entry1->d_name, "..") == 0 ||
            strcmp(entry1->d_name, ".givit") == 0)
            continue;

        strcpy(file1_path, commit1_path);
        strcat(file1_path, "/");
        strcat(file1_path, entry1->d_name);
        strcpy(file2_path, commit2_path);
        strcat(file2_path, "/");
        strcat(file2_path, entry1->d_name);
        FILE *check = fopen(file2_path, "r");
        if (check != NULL) {
            fclose(check);
            if (entry1->d_type == DT_DIR) {
                compare_commit_directories(file1_path, file2_path);
            } else {
                printf("\e[36mFile %s exists in both commit directories.\e[m\n", entry1->d_name);
                char cmd[1024], cwd1[1024], cwd2[1024];
                strcpy(cmd, "cp -R ");
                strcat(cmd, file1_path);
                strcat(cmd, " .givit/valid");
                system(cmd);
                strcpy(cmd, "cp -R ");
                strcat(cmd, file2_path);
                strcat(cmd, " .givit/valid");
                system(cmd);
                strcpy(cwd1, ".givit/valid/");
                strcat(cwd1, filename_maker(file1_path));
                strcat(cwd1, "-");
                strcat(cwd1, filename_maker(commit1_path));
                strcpy(cwd2, ".givit/valid/");
                strcat(cwd2, filename_maker(file2_path));
                strcat(cwd2, "-");
                strcat(cwd2, filename_maker(commit2_path));
                /*checkk("$$");
                checkk(file1_path);
                checkk(cwd1);*/
                print_valid_file(fopen(file1_path, "r"), fopen(cwd1, "w"));
                print_valid_file(fopen(file2_path, "r"), fopen(cwd2, "w"));
                compare_to_file(cwd1, cwd2, 1, 1, 1000, 1000);
            }
        } else {
            printf("\e[36mFile %s exists only in the first commit directory.\e[m\n", entry1->d_name);
            fclose(check);
        }

    }
    while ((entry2 = readdir(dir2)) != NULL) {
        char file1_path[1024], file2_path[1024];

        if (strcmp(entry2->d_name, ".") == 0 || strcmp(entry2->d_name, "..") == 0)
            continue;

        strcpy(file1_path, commit1_path);
        strcat(file1_path, "/");
        strcat(file1_path, entry1->d_name);
        strcpy(file2_path, commit2_path);
        strcat(file2_path, "/");
        strcat(file2_path, entry1->d_name);

        FILE *check = fopen(file1_path, "r");
        if (check != NULL) {
            printf("\e[36mFile %s exists only in the second commit directory.\e[m\n", file2_path);
        }
        fclose(check);
    }

    closedir(dir1);
    closedir(dir2);
    return;
}

int merge_commit_directories(char *commit1_path, char *commit2_path, char *branch1, char *branch2) {
    DIR *dir1 = opendir(commit1_path);
    DIR *dir2 = opendir(commit2_path);
    struct dirent *entry1, *entry2;

    if (dir1 == NULL || dir2 == NULL) {
        printf("Error opening directories.\n");
        return 1;
    }

    while ((entry1 = readdir(dir1)) != NULL) {
        char file1_path[1024], file2_path[1024];

        if (strcmp(entry1->d_name, ".") == 0 || strcmp(entry1->d_name, "..") == 0 ||
            strcmp(entry1->d_name, ".givit") == 0)
            continue;

        strcpy(file1_path, commit1_path);
        strcat(file1_path, "/");
        strcat(file1_path, entry1->d_name);
        strcpy(file2_path, commit2_path);
        strcat(file2_path, "/");
        strcat(file2_path, entry1->d_name);
        if (entry1->d_type == DT_DIR) {
            merge_commit_directories(file1_path, file2_path, branch1, branch2);
        } else {
            FILE *check = fopen(file2_path, "r");
            if (check != NULL) {
                fclose(check);
                //printf("\e[36mFile %s exists in both commit directories.\nresults:\e[m\n", entry1->d_name);
                printf("%s\n", file1_path);
                if (compare_to_file_merge_conflict(file1_path, file2_path, commit1_path, commit2_path, branch1,
                                                   branch2)) {
                    printf("merge canceled\n");
                    return 1;
                }

            } else {
                fclose(check);
                //fixing path:
                char cwd[1024], filepath[1024];
                memmove(filepath, file1_path + strlen(commit1_path) + 1,
                        strlen(file1_path) - strlen(commit1_path) + 10);
                strcpy(cwd, ".givit/staged/");
                strcat(cwd, filepath);
                //copy file into staging area:
                char cmd[1024];
                strcpy(cmd, "cp -R ");
                strcat(cmd, file1_path);
                strcat(cmd, " ");
                strcat(cmd, cwd);
                system(cmd);
            }
        }
    }
    closedir(dir1);
    while ((entry2 = readdir(dir2)) != NULL) {
        char file1_path[1024], file2_path[1024];
        if (entry2->d_type == DT_DIR) {
            continue;
        }
        if (strcmp(entry2->d_name, ".") == 0 || strcmp(entry2->d_name, "..") == 0)
            continue;

        strcpy(file1_path, commit1_path);
        strcat(file1_path, "/");
        strcat(file1_path, entry2->d_name);
        strcpy(file2_path, commit2_path);
        strcat(file2_path, "/");
        strcat(file2_path, entry2->d_name);

        FILE *check = fopen(file1_path, "r");
        if (check == NULL) {
            printf("\e[36mFile %s exists only in the second commit directory.\e[m\n", file2_path);
            //fixing path:
            char cwd[1024], filepath[1024];
            memmove(filepath, file2_path + strlen(commit2_path) + 1,
                    strlen(file2_path) - strlen(commit2_path) + 10);
            strcpy(cwd, ".givit/staged/");
            strcat(cwd, filepath);
            //copy file into staging area:
            char cmd[1024];
            strcpy(cmd, "cp -R ");
            strcat(cmd, file2_path);
            strcat(cmd, " ");
            strcat(cmd, cwd);
            system(cmd);
        }
    }
    closedir(dir2);
    return 0;
}

void print_valid_file(FILE *file, FILE *valid_file) {
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
        check_valid(line);
        if (!line_is_empty(line)) {
            fprintf(valid_file, "%s\n", line);
        }
    }
    fclose(file);
    fclose(valid_file);
}

void compare_to_file_merge(char *file1_path, char *file2_path, int begin_line1, int begin_line2, int end_line1,
                           int end_line2) {
    FILE *file1 = fopen(file1_path, "r");
    FILE *file2 = fopen(file2_path, "r");
    char *file1name = filename_maker(file1_path);
    char *file2name = filename_maker(file2_path);
    char line1[MAX_LINE_LENGTH];
    char line2[MAX_LINE_LENGTH];
    int line1_number = 0;
    int line2_number = 0;
    forLoop(begin_line1 - 1, i) {
        fgets(line1, MAX_LINE_LENGTH, file1);
        line1_number++;
    }
    forLoop(begin_line2 - 1, i) {
        fgets(line2, MAX_LINE_LENGTH, file2);
        line2_number++;
    }
    while (fgets(line1, MAX_LINE_LENGTH, file1) != NULL && fgets(line2, MAX_LINE_LENGTH, file2) != NULL) {
        line1_number++;
        line2_number++;
        if (line1_number > end_line1) {
            line2_number--;
            break;
        }
        if (line2_number > end_line2) {
            line1_number--;
            break;
        }
        check_valid(line1);
        check_valid(line2);
        if (strcmp(line1, line2) != 0) {
            printf("\e[33m«««««%s»»»»»\n%s-%d\n%s\e[m\n", file1_path, file1name, line1_number, line1);
            printf("\e[37m«««««%s»»»»»\n%s-%d\n%s\e[m\n", file2_path, file2name, line2_number, line2);
        }
    }
    while (fgets(line1, MAX_LINE_LENGTH, file1) != NULL) {
        line1_number++;
        if (line1_number > end_line1) {
            break;
        }
        check_valid(line1);
        printf("\e[33m«««««%s»»»»»\n%s-%d\n%s\e[m\n", file1_path, file1name, line1_number, line1);
    }
    while (fgets(line2, MAX_LINE_LENGTH, file2) != NULL) {
        line2_number++;
        if (line2_number > end_line2) {
            break;
        }
        check_valid(line2);
        printf("\e[37m«««««%s»»»»»\n%s-%d\n%s\e[m\n", file2_path, file2name, line2_number, line2);
    }
    fclose(file1);
    fclose(file2);
}

int compare_to_file_merge_conflict(char *file1_path, char *file2_path, char *commit_path1, char *commit_path2,
                                   char *branch1, char *branch2) {
    FILE *file1 = fopen(file1_path, "r");
    FILE *file2 = fopen(file2_path, "r");
    char cwd[1024], conflict_path[1024];
    memmove(conflict_path, file1_path + strlen(commit_path1) + 1,
            strlen(file1_path) - strlen(commit_path1) + 20);
    strcpy(cwd, ".givit/staged/");
    strcat(cwd, conflict_path);
    FILE *conflict = fopen(cwd, "w");
    char *file1name = filename_maker(file1_path);
    char *file2name = filename_maker(file2_path);
    char line1[MAX_LINE_LENGTH];
    char line2[MAX_LINE_LENGTH];
    int line1_number = 0;
    int line2_number = 0;
    rewind(file1);
    rewind(file2);
    while (fgets(line1, MAX_LINE_LENGTH, file1) != NULL && fgets(line2, MAX_LINE_LENGTH, file2) != NULL) {
        line1_number++;
        line2_number++;
        check_valid(line1);
        check_valid(line2);
        while (check_line(line1) == 0) {
            line1_number++;
            if (fgets(line1, MAX_LINE_LENGTH, file1) == NULL) {
                return 0;
            }
            check_valid(line1);
        }
        while (check_line(line2) == 0) {
            line2_number++;
            if (fgets(line2, MAX_LINE_LENGTH, file2) == NULL) {
                return 0;
            }
            check_valid(line2);
        }
        if (strcmp(line1, line2) != 0) {
            //printf("\e[33m«««««%s»»»»»\n%s-%d\n%s\e[m\n",file1_path, file1name, line1_number, line1);
            printf("\e[33m%s-%d:\n%s\e[m\n", branch1, line1_number, line1);
            printf("\e[37m%s-%d:\n%s\e[m\n", branch2, line2_number, line2);
            char input[1024];
            scanf("%s", input);
            if (strcmp(input, "1") == 0) {
                fprintf(conflict, "%s\n", line1);
            } else if (strcmp(input, "2") == 0) {
                fprintf(conflict, "%s\n", line2);
            } else if (strcmp(input, "quit") == 0) {
                //turn staged area into 1_staged:
                system("Givit reset -undo > /dev/null 2>&1");
                return 1;
            } else {
                fprintf(conflict, "%s\n", input);
            }
        } else {
            fprintf(conflict, "%s\n", line1);
        }
    }
    while (fgets(line1, MAX_LINE_LENGTH, file1) != NULL) {
        line1_number++;
        check_valid(line1);
        fprintf(conflict, "%s\n", line1);
    }
    while (fgets(line2, MAX_LINE_LENGTH, file2) != NULL) {
        line2_number++;
        check_valid(line2);
        fprintf(conflict, "%s\n", line2);
    }
    fclose(file2);
    fclose(file1);
    fclose(conflict);
    return 0;
}

void
compare_to_file(char *file1_path, char *file2_path, int begin_line1, int begin_line2, int end_line1,
                int end_line2) {
    FILE *file1 = fopen(file1_path, "r");
    FILE *file2 = fopen(file2_path, "r");
    char *file1name = filename_maker(file1_path);
    char *file2name = filename_maker(file2_path);
    char line1[1000];
    char line2[1000];
    int line1_number = 0;
    int line2_number = 0;
    forLoop(begin_line1 - 1, i) {
        fgets(line1, MAX_LINE_LENGTH, file1);
        line1_number++;
    }
    forLoop(begin_line2 - 1, i) {
        fgets(line2, MAX_LINE_LENGTH, file2);
        line2_number++;
    }
    while ((fgets(line1, MAX_LINE_LENGTH, file1) != NULL) && (fgets(line2, MAX_LINE_LENGTH, file2) != NULL)) {
        line1_number++;
        line2_number++;
        if (line1_number > end_line1) {
            line2_number--;
            break;
        }
        if (line2_number > end_line2) {
            line1_number--;
            break;
        }
        check_valid(line1);
        check_valid(line2);
        if (strcmp(line1, line2) != 0) {
            printf("\e[33m%s-%d\n%s\e[m\n", file1name, line1_number, line1);
            printf("\e[37m%s-%d\n%s\e[m\n", file2name, line2_number, line2);
        }
    }
    while (fgets(line1, MAX_LINE_LENGTH, file1) != NULL) {
        line1_number++;
        if (line1_number > end_line1) {
            break;
        }
        check_valid(line1);
        printf("\e[33m%s-%d\n%s\e[m\n", file1name, line1_number, line1);
    }
    while (fgets(line2, MAX_LINE_LENGTH, file2) != NULL) {
        line2_number++;
        if (line2_number > end_line2) {
            break;
        }
        check_valid(line2);
        printf("\e[37m%s-%d\n%s\e[m\n", file2name, line2_number, line2);
    }
    fclose(file1);
    fclose(file2);
}

int run_diff(int argc,
             char *const argv[]) {//givit diff -f <file1> <file2> –line1 <begin-end> –line2 <begin-end> or givit diff -c <commit1-id> <commit2-id>
    if (argc < 5) {
        perror("please enter a valid command");
        return 1;
    }
    if (strcmp(argv[2], "-c") == 0) {
        Commit *head = loadList(".givit/commitsdb");
        Commit *node2 = head;
        Commit *node1 = head;
        int commit1_id = atoi(argv[3]);
        int commit2_id = atoi(argv[4]);
        while (node1 != NULL) {
            if (node1->commit_ID == commit1_id) {
                break;
            }
            node1 = node1->prev_commit;
        }
        while (node2 != NULL) {
            if (node2->commit_ID == commit2_id) {
                break;
            }
            node2 = node2->prev_commit;
        }
        if (node1 == NULL || node2 == NULL) {
            perror("commit not found");
            return 1;
        }
        char cwd1[1024], cwd2[1024], node1_cwd[1024], node2_cwd[1024];
        getcwd(cwd1, sizeof(cwd1));
        getcwd(cwd2, sizeof(cwd2));
        memmove(node1_cwd, node1->dir_path + strlen(cwd1) + 1, strlen(node1->dir_path) - strlen(cwd1));
        memmove(node2_cwd, node2->dir_path + strlen(cwd2) + 1, strlen(node2->dir_path) - strlen(cwd2));
        compare_commit_directories(node1_cwd, node2_cwd);
        return 0;
    }
    if (strcmp(argv[2], "-f") != 0) {
        perror("please enter a valid command");
        return 1;
    }
    if (argc == 5) {
        sprintf(argv[6], "%d-%d", 1, 1000);
        sprintf(argv[8], "%d-%d", 1, 1000);
    }
    char file1_path[MAX_LINE_LENGTH];
    char file2_path[MAX_LINE_LENGTH];
    strcpy(file1_path, "./");
    strcpy(file2_path, "./");
    strcat(file1_path, argv[3]);
    strcat(file2_path, argv[4]);
    int begin_line1, end_line1, begin_line2, end_line2;
    sscanf(argv[6], "%d-%d", &begin_line1, &end_line1);
    sscanf(argv[8], "%d-%d", &begin_line2, &end_line2);
    char cmd[1024];
    strcpy(cmd, "cp -R ");
    strcat(cmd, file1_path);
    strcat(cmd, " .givit/valid");
    system(cmd);
    strcpy(cmd, "cp -R ");
    strcat(cmd, file2_path);
    strcat(cmd, " .givit/valid");
    system(cmd);
    char cwd1[1024], cwd2[1024];
    strcpy(cwd1, ".givit/valid/");
    strcpy(cwd2, ".givit/valid/");
    strcat(cwd1, filename_maker(file1_path));
    strcat(cwd2, filename_maker(file2_path));
    print_valid_file(fopen(file1_path, "r"), fopen(cwd1, "w"));
    print_valid_file(fopen(file2_path, "r"), fopen(cwd2, "w"));
    compare_to_file(cwd1, cwd2, begin_line1, begin_line2, end_line1, end_line2);
    return 0;
}

int run_merge(int argc, char *const argv[]) {
    if (argc < 5) {
        perror("please enter a valid command");
        return 1;
    }
    if (strcmp(argv[2], "-b") == 0) {
        char branch1[MAX_LINE_LENGTH], branch2[MAX_LINE_LENGTH];
        strcpy(branch1, argv[3]);
        strcpy(branch2, argv[4]);
        Commit *head = loadList(".givit/commitsdb");
        Commit *node1 = head;
        Commit *node2 = head;
        while (node1 != NULL) {
            if (strcmp(node1->branch, branch1) == 0) {
                break;
            }
            node1 = node1->prev_commit;
        }
        while (node2 != NULL) {
            if (strcmp(node2->branch, branch2) == 0) {
                break;
            }
            node2 = node2->prev_commit;
        }
        if (node1 == NULL || node2 == NULL) {
            perror("branch not found");
            return 1;
        }
        //merge in staged:
        if (merge_commit_directories(node1->dir_path, node2->dir_path, branch1, branch2)) {
            return 1;
        }
        //merge two commits and create a new commit:
        char message[MAX_COMMIT_MESSAGE_LENGTH];
        strcpy(message, "merge ");
        strcat(message, branch1);
        strcat(message, " with ");
        strcat(message, branch2);
        int hour, minute, second;
        time_t currentTime;
        time(&currentTime);
        struct tm *timestruct = localtime(&currentTime);
        minute = timestruct->tm_min;
        second = timestruct->tm_sec;
        hour = timestruct->tm_hour;
        head = create_last_commit_dir(message, hour, minute, second, head);
        head->prev_attached_commit_ID = node1->commit_ID;
        head->merged_ID = node2->commit_ID;
        saveList(head, ".givit/commitsdb");
        return 0;
    }
}

void filename_to_filepath(char *filename, char *cwd) {
    getcwd(cwd, sizeof(cwd));
    int len = strlen(cwd);
    cwd[len - 1] = '/';
    cwd[len] = '\0';
    strcat(cwd, filename);
}

void mkdir_all_directories(char *directory_path) {
    DIR *staged = opendir(".givit/staged");
    DIR *dir = opendir(directory_path);
    struct dirent *entry;
    if (dir == NULL) {
        perror("Error opening current directory");
        return;
    }
    while ((entry = readdir(dir)) != NULL) {
        if ((entry->d_type == DT_DIR) && (strcmp(entry->d_name, ".") != 0) &&
            (strcmp(entry->d_name, "..") != 0) &&
            (strcmp(entry->d_name, ".givit") != 0)) {
            char temp[MAX_LINE_LENGTH], temp_stage[MAX_LINE_LENGTH], temp_commit[MAX_LINE_LENGTH];
            strcpy(temp, directory_path);
            strcat(temp, "/");
            strcat(temp, entry->d_name);
            strcat(temp_stage, ".givit/staged/");
            strcat(temp_stage, temp);
            if (!check_for_exist_directory(temp_stage)) {
                mkdir(temp_stage, 0755);
            }
            strcat(temp_commit, ".givit/commits/");
            strcat(temp_commit, temp);
            if (!check_for_exist_directory(temp_commit)) {
                //mkdir(temp_commit, 0755);
            }
            mkdir_all_directories(temp);
            temp_stage[0] = '\0';
        }
    }

}

int check_alias_exist(char *alias) {
    FILE *aliases = fopen(".givit/aliases", "r"); //"alias name" "command"
    char temp[MAX_LINE_LENGTH];
    while (fgets(temp, MAX_LINE_LENGTH, aliases) != NULL) {
        check_valid(temp);
        char alias_name[1024];
        sscanf(temp, "\"%[^\"]\"", alias_name);
        if (strcmp(alias_name, alias) == 0) {
            return 1;
        }
    }
    return 0;
}

int print_head(Commit *head) {// show commits in tree:

}

int run_grep(int argc, char *const argv[]) {//givit grep -f <file> -p <word> [-c <commit-id>] [-n]
    if (argc < 6) {
        perror("please enter a valid command");
        return 1;
    }
    if (strcmp(argv[2], "-f") != 0 || strcmp(argv[4], "-p") != 0) {
        perror("please enter a valid command");
        return 1;
    }
    char file_path[MAX_LINE_LENGTH];
    strcpy(file_path, "./");
    strcat(file_path, argv[3]);
    char word[MAX_LINE_LENGTH];
    strcpy(word, argv[5]);
    int commit_id = -1;
    int n = 0;
    if (argc > 6) {
        if (strcmp(argv[6], "-c") == 0) {
            commit_id = atoi(argv[7]);
            if (argc > 8) {
                if (strcmp(argv[8], "-n") == 0) {
                    n = 1;
                }
            }
        }
        if (strcmp(argv[6], "-n") == 0) {
            n = 1;
        }
    }
    if (commit_id == -1) {
        FILE *file = fopen(file_path, "r");
        char line[MAX_LINE_LENGTH];
        int line_number = 0;
        while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
            line_number++;
            check_valid(line);
            if (strstr(line, word) != NULL) {
                if (n) {
                    printf("%d: %s\n", line_number, line);
                } else {
                    printf("%s\n", line);
                }
            }
        }
        fclose(file);
    } else {
        Commit *head = loadList(".givit/commitsdb");
        Commit *node = head;
        while (node != NULL) {
            if (node->commit_ID == commit_id) {
                break;
            }
            node = node->prev_commit;
        }
        if (node == NULL) {
            perror("commit not found");
            return 1;
        }
        char cwd[1024], commit_cwd[1024];
        getcwd(cwd, sizeof(cwd));
        memmove(commit_cwd, node->dir_path + strlen(cwd) + 1, strlen(node->dir_path) - strlen(cwd));
        char file_path_commit[1024];
        strcpy(file_path_commit, commit_cwd);
        strcat(file_path_commit, "/");
        strcat(file_path_commit, argv[3]);
        FILE *file = fopen(file_path_commit, "r");
        char line[MAX_LINE_LENGTH];
        int line_number = 0;
        while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
            line_number++;
            check_valid(line);
            if (strstr(line, word) != NULL) {
                if (n) {
                    printf("%d: %s\n", line_number, line);
                } else {
                    printf("%s\n", line);
                }
            }
        }
        fclose(file);
    }
}

int todo_check(char *filepath) {
    if (strstr(filepath, ".cpp") != NULL || strstr(filepath, ".c") != NULL) {
        FILE *file = fopen(filepath, "r");
        char line[MAX_LINE_LENGTH];
        while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
            check_valid(line);
            if (strstr(line, "//TODO") != NULL) {
                return 1;
            }
        }
        fclose(file);
    } else if (strstr(filepath, ".txt") != NULL) {
        FILE *file = fopen(filepath, "r");
        char line[MAX_LINE_LENGTH];
        while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
            check_valid(line);
            if (strstr(line, "TODO") != NULL) {
                return 1;
            }
        }
        fclose(file);
    } else {
        return -1;
    }
    return 0;
}

int eof_checker(char *filepath) {
    if (strstr(filepath, ".cpp") == NULL && strstr(filepath, ".c") == NULL || strstr(filepath, ".txt") != NULL) {
        return -1;
    }
    FILE *file = fopen(filepath, "r");
    char line[MAX_LINE_LENGTH];
    fseek(file, -1, SEEK_END);
    fgetc(file);
    if (fgetc(file) != '\n' || fgetc(file) != EOF) {
        return 1;
    }
    fclose(file);
    return 0;
}

int static_error_check(char *filepath) { //like compile error
    if (strstr(filepath, ".cpp") == NULL && strstr(filepath, ".c") == NULL) {
        return -1;
    }
    char cmd[1024];
    strcpy(cmd, "g++ -o .givit/valid/valid ");
    strcat(cmd, filepath);
    strcat(cmd, " 2> .givit/valid/error");
    system(cmd);
    FILE *error = fopen(".givit/valid/error", "r");
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, error) != NULL) {
        check_valid(line);
        if (strstr(line, "error") != NULL) {
            return 1;
        }
    }
    fclose(error);
    return 0;
}

int character_size_checker(char *filepath) {
    if (strstr(filepath, ".cpp") == NULL && strstr(filepath, ".c") == NULL || strstr(filepath, ".txt") != NULL) {
        return -1;
    }
    FILE *file = fopen(filepath, "r");
    char line[MAX_LINE_LENGTH];
    int size = 0;
    while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
        check_valid(line);
        size += strlen(line);
    }
    if (size > 20000) {
        return 1;
    }
    fclose(file);
    return 0;
}

int balance_brackets(char *filepath) {
    if (strstr(filepath, ".cpp") == NULL && strstr(filepath, ".c") == NULL || strstr(filepath, ".txt") != NULL) {
        return -1;
    }
    FILE *file = fopen(filepath, "r");
    char line[MAX_LINE_LENGTH];
    int open_brackets = 0;
    int close_brackets = 0;
    while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
        check_valid(line);
        for (int i = 0; i < strlen(line); i++) {
            if (line[i] == '{') {
                open_brackets++;
            }
            if (line[i] == '}') {
                close_brackets++;
            }
        }
    }
    fclose(file);
    if (open_brackets != close_brackets) {
        return 1;
    }
    return 0;
}

int format_checker(char *file) {// .cpp .c .txt .mp3 .mp4 .mkv .txt
    if (strstr(file, ".cpp") != NULL || strstr(file, ".c") != NULL || strstr(file, ".txt") != NULL ||
        strstr(file, ".mp3") != NULL || strstr(file, ".mp4") != NULL || strstr(file, ".mkv") != NULL ||
        strstr(file, ".txt") != NULL) {
        return 0;
    }
    return 1;
}

int time_limit(char *file) {
    if (strstr(file, ".mp4") == NULL && strstr(file, ".mp3") == NULL || strstr(file, ".wav") != NULL) {
        return -1;
    }
    struct stat st;
    stat(file, &st);
    time_t mtime = st.st_mtime;
    time_t now = time(NULL);
    if (difftime(now, mtime) > 60 * 10) {
        return 1;
    }
    return 0;
}

int filesize_checker(char *file) {//filesize 5mb
    struct stat st;
    stat(file, &st);
    int size = st.st_size;
    if (size > 5 * 1024 * 1024) {
        return 1;
    }
    return 0;
}

int apply_hooks(char *filepath) {
    printf("\e[37m$ %s $:\e[m\n", filepath);
    FILE *hooks = fopen(".givit/hooks", "r");
    int flag = 0;
    if (hooks == NULL) {
        perror("hooks not found");
        return 1;
    }
    char temp[MAX_LINE_LENGTH];
    while (fgets(temp, MAX_LINE_LENGTH, hooks) != NULL) {
        check_valid(temp);
        if (strcmp(temp, "todo-check") == 0) {
            if (todo_check(filepath) == 1) {
                printf("todo-check");
                forLoop(16, i) {
                    printf(".");
                }
                printf("\e[31mFAILED\e[m\n");
                flag = 1;
            } else if (todo_check(filepath) == -1) {
                printf("todo-check");
                forLoop(16, i) {
                    printf(".");
                }
                printf("\e[33mSKIPPED\e[m\n");
            } else {
                printf("todo-check");
                forLoop(16, i) {
                    printf(".");
                }
                printf("\e[32mPASSED\e[m\n");
            }
        }
        if (strcmp(temp, "balance-braces") == 0) {
            if (balance_brackets(filepath) == 1) {
                printf("balance-brackets");
                forLoop(10, i) {
                    printf(".");
                }
                printf("\e[31mFAILED\e[m\n");
                flag = 1;
            } else if (balance_brackets(filepath) == -1) {
                printf("balance-brackets");
                forLoop(10, i) {
                    printf(".");
                }
                printf("\e[33mSKIPPED\e[m\n");
            } else {
                printf("balance-brackets");
                forLoop(10, i) {
                    printf(".");
                }
                printf("\e[32mPASSED\e[m\n");
            }
        }
        if (strcmp(temp, "format-check") == 0) {
            if (format_checker(filepath)) {
                printf("format-checker");
                forLoop(12, i) {
                    printf(".");
                }
                printf("\e[31mFAILED\e[m\n");
                flag = 1;
            } else {
                printf("format-checker");
                forLoop(12, i) {
                    printf(".");
                }
                printf("\e[32mPASSED\e[m\n");
            }
        }
        if (strcmp(temp, "file-size-check") == 0) {
            if (filesize_checker(filepath)) {
                printf("filesize-checker");
                forLoop(10, i) {
                    printf(".");
                }
                printf("\e[31mFAILED\e[m\n");
                flag = 1;
            } else {
                printf("filesize-checker");
                forLoop(10, i) {
                    printf(".");
                }
                printf("\e[32mPASSED\e[m\n");
            }
        }
        if (strcmp(temp, "eof-blank-space") == 0) {
            if (eof_checker(filepath) == 1) {
                printf("eof-blank-space");
                forLoop(14, i) {
                    printf(".");
                }
                printf("\e[31mFAILED\e[m\n");
                flag = 1;
            } else if (eof_checker(filepath) == -1) {
                printf("eof-blank-space");
                forLoop(14, i) {
                    printf(".");
                }
                printf("\e[33mSKIPPED\e[m\n");
            } else {
                printf("eof-blank-space");
                forLoop(14, i) {
                    printf(".");
                }
                printf("\e[32mPASSED\e[m\n");
            }
        }
        if (strcmp(temp, "character-limit") == 0) {
            if (character_size_checker(filepath) == 1) {
                printf("character-limit");
                forLoop(10, i) {
                    printf(".");
                }
                printf("\e[31mFAILED\e[m\n");
                flag = 1;
            } else if (character_size_checker(filepath) == -1) {
                printf("character-limit");
                forLoop(10, i) {
                    printf(".");
                }
                printf("\e[33mSKIPPED\e[m\n");
            } else {
                printf("character-limit");
                forLoop(10, i) {
                    printf(".");
                }
                printf("\e[32mPASSED\e[m\n");
            }
        }
        if (strcmp(temp, "time-limit") == 0) {
            if (time_limit(filepath) == 1) {
                printf("time-limit");
                forLoop(14, i) {
                    printf(".");
                }
                printf("\e[31mFAILED\e[m\n");
                flag = 1;
            } else if (time_limit(filepath) == -1) {
                printf("time-limit");
                forLoop(14, i) {
                    printf(".");
                }
                printf("\e[33mSKIPPED\e[m\n");
            } else {
                printf("time-limit");
                forLoop(14, i) {
                    printf(".");
                }
                printf("\e[32mPASSED\e[m\n");
            }
        }
        if (strcmp(temp, "static-error-check") == 0) {
            if (static_error_check(filepath) == 1) {
                printf("static-error-check");
                forLoop(10, i) {
                    printf(".");
                }
                printf("\e[31mFAILED\e[m\n");
                flag = 1;
            } else if (static_error_check(filepath) == -1) {
                printf("static-error-check");
                forLoop(10, i) {
                    printf(".");
                }
                printf("\e[33mSKIPPED\e[m\n");
            } else {
                printf("static-error-check");
                forLoop(10, i) {
                    printf(".");
                }
                printf("\e[32mPASSED\e[m\n");
            }
        }
    }
    if (flag) {
        return 1;
    }
    return 0;
}

int apply_for_dir_files(char *dir_path) {
    int flag = 0;
    DIR *dir = opendir(dir_path);
    struct dirent *entry;
    if (dir == NULL) {
        perror("Error opening current directory");
        return 1;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char file_path[MAX_LINE_LENGTH];
        strcpy(file_path, dir_path);
        strcat(file_path, "/");
        strcat(file_path, entry->d_name);
        if (entry->d_type == DT_DIR) {
            apply_for_dir_files(file_path);
        } else {
            if (apply_hooks(file_path)) {
                flag = 1;
            }
        }
    }
    closedir(dir);
    if (flag) {
        return 1;
    }
    return 0;
}

int run_precommit(int argc, char *const argv[]) {
    if (argc < 2) {
        perror("please enter a valid command");
        return 1;
    }
    if (argc == 2) {
        //aplly all hooks on all files in staged area:
        int flag = apply_for_dir_files(".givit/staged");
        if (flag) {
            printf("You can't commit your staging area!\n");
            FILE *booli = fopen(".givit/bool", "w");
            fprintf(booli, "0");
            fclose(booli);
            return 1;
        }
        return 0;
    }
    if (strcmp(argv[2], "hooks") == 0 && strcmp(argv[3], "list") == 0) {
        FILE *hooks = fopen(".givit/hooks", "r");
        if (hooks == NULL) {
            perror("hooks not found");
            return 1;
        }
        char temp[MAX_LINE_LENGTH];
        while (fgets(temp, MAX_LINE_LENGTH, hooks) != NULL) {
            check_valid(temp);
            printf("%s\n", temp);
        }
        return 0;
    }
    if (strcmp(argv[2], "applied") == 0 && strcmp(argv[3], "hooks") == 0) {
        //show applied hooks on all files in staged area
        FILE *a_hooks = fopen(".givit/applied_hooks", "r");
        if (a_hooks == NULL) {
            perror("applied hooks not found");
            return 1;
        }
        char temp[MAX_LINE_LENGTH];
        while (fgets(temp, MAX_LINE_LENGTH, a_hooks) != NULL) {
            check_valid(temp);
            printf("%s\n", temp);
        }
        fclose(a_hooks);
        return 0;
    }
    if (strcmp(argv[2], "add") == 0 && strcmp(argv[3], "hook") == 0) {
        char hook_id[MAX_LINE_LENGTH];
        strcpy(hook_id, argv[4]);
        FILE *hooks = fopen(".givit/hooks", "r");
        if (hooks == NULL) {
            perror("hooks not found");
            return 1;
        }
        char temp[MAX_LINE_LENGTH];
        while (fgets(temp, MAX_LINE_LENGTH, hooks) != NULL) {
            check_valid(temp);
            if (strcmp(temp, hook_id) == 0) {
                FILE *a_hooks = fopen(".givit/applied_hooks", "a");
                fprintf(a_hooks, "%s\n", hook_id);
                fclose(a_hooks);
                return 0;
            }
        }
        perror("hook not found");
        return 1;
    }
    if (strcmp(argv[2], "remove") == 0 && strcmp(argv[3], "hook") == 0) {
        //remove hook from applied hooks
        char hook_id[MAX_LINE_LENGTH];
        strcpy(hook_id, argv[4]);
        FILE *a_hooks = fopen(".givit/applied_hooks", "r");
        if (a_hooks == NULL) {
            perror("applied hooks not found");
            return 1;
        }
        char temp[MAX_LINE_LENGTH];
        while (fgets(temp, MAX_LINE_LENGTH, a_hooks) != NULL) {
            check_valid(temp);
            if (strcmp(temp, hook_id) == 0) {
                FILE *a_hooks_temp = fopen(".givit/applied_hooks_temp", "w");
                fclose(a_hooks);
                a_hooks = fopen(".givit/applied_hooks", "r");
                while (fgets(temp, MAX_LINE_LENGTH, a_hooks) != NULL) {
                    check_valid(temp);
                    if (strcmp(temp, hook_id) != 0) {
                        fprintf(a_hooks_temp, "%s\n", temp);
                    }
                }
                fclose(a_hooks);
                fclose(a_hooks_temp);
                remove(".givit/applied_hooks");
                rename(".givit/applied_hooks_temp", ".givit/applied_hooks");
                return 0;
            }
        }
        perror("hook not found");
        return 1;
    }
    if (strcmp(argv[2], "-f") == 0) {//givit pre-commit -f <file1> <file2>
        if (argc < 5) {
            perror("please enter a valid command");
            return 1;
        }
        char file1_path[MAX_LINE_LENGTH];
        char file2_path[MAX_LINE_LENGTH];
        strcpy(file1_path, ".givit/staged/");
        strcpy(file2_path, ".givit/staged/");
        strcat(file1_path, argv[3]);
        strcat(file2_path, argv[4]);
        if (apply_hooks(file1_path) || apply_hooks(file2_path)) {
            return 1;
        }
        return 0;
    }
}

int main(int argc, char *argv[]) {
    Commit *head = NULL;
    time_t currentTime;
    time(&currentTime);
    struct tm *timestruct = localtime(&currentTime);
    int minute = timestruct->tm_min;
    int second = timestruct->tm_sec;
    int hour = timestruct->tm_hour;
    if (argc < 2) {
        fprintf(stdout, "please enter a valid command");
        return 1;
    }
    if (argc == 2) {//check alias
        if (opendir(".givit") != NULL) {
            if (check_alias_exist(argv[1])) {
                FILE *aliases = fopen(".givit/aliases", "r");
                char temp[MAX_LINE_LENGTH];
                while (fgets(temp, MAX_LINE_LENGTH, aliases) != NULL) {
                    check_valid(temp);
                    char alias_name[1024], command[1024];
                    sscanf(temp, "\"%[^\"]\" \"%[^\"]\"", alias_name, command);
                    if (strcmp(alias_name, argv[1]) == 0) {
                        char cmd[1024];
                        strcpy(cmd, "Givit ");
                        strcat(cmd, command);
                        system(cmd);
                        return 0;
                    }
                }

            }
        }
    }
    mkdir_all_directories(".");
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    char ADMIN_USERNAME[MAX_CONFIG_LENGTH], ADMIN_EMAIL[MAX_CONFIG_LENGTH];
    read_adminator(ADMIN_USERNAME, ADMIN_EMAIL);
    chdir(cwd);
    if (strcmp(argv[1], "init") == 0) {
        return run_init(argc, argv, ADMIN_USERNAME, ADMIN_EMAIL);
    } else if (strcmp(argv[1], "add") == 0) {
        return run_add(argc, argv);
    } else if (strcmp(argv[1], "reset") == 0) {
        return run_reset(argc, argv);
    } else if (strcmp(argv[1], "status") == 0) {
        return run_status(argc, argv);
    } else if (strcmp(argv[1], "commit") == 0) {
        return run_commit(argc, argv, hour, minute, second, head);
    } else if (strcmp(argv[1], "set") == 0) {
        return set_shortcut(argc, argv);
    } else if (strcmp(argv[1], "replace") == 0) {
        return replace_shortcut(argc, argv);
    } else if (strcmp(argv[1], "remove") == 0) {
        return remove_shortcut(argc, argv);
    } else if (strcmp(argv[1], "log") == 0) {
        return run_log(argc, argv, head);
    } else if (strcmp(argv[1], "branch") == 0) {
        return run_branch(argc, argv, hour, minute, second, head);
    } else if (strcmp(argv[1], "checkout") == 0) {
        return run_checkout(argc, argv, hour, minute, second, head);
    } else if (strcmp(argv[1], "revert") == 0) {
        return run_revert(argc, argv, hour, minute, second, head);
    } else if (strcmp(argv[1], "tag") == 0) {
        return run_tag(argc, argv, hour, minute, second, head);
    } else if (strcmp(argv[1], "pre-commit") == 0) {
        return run_precommit(argc, argv);
    } else if (strcmp(argv[1], "grep") == 0) {
        return run_grep(argc, argv);
    } else if (strcmp(argv[1], "diff") == 0) {
        return run_diff(argc, argv);
    } else if (strcmp(argv[1], "merge") == 0) {
        return run_merge(argc, argv);
    } else if (strcmp(argv[1], "tree") == 0) {
        //  return run_tree(argc, argv);
    } else if (strcmp(argv[1], "config") == 0) {
        if (strcmp(argv[2], "--global") == 0)
            return run_global_config(argc, argv);
        else return run_config(argc, argv);
    }
    return 0;
}
/*
 * forLoop(depth, i){
                glob_t globbuf;
                glob(temp, 0, NULL, &globbuf);
                forLoop(globbuf.gl_pathc, j){
                    forLoop(i, k){
                        printf("\t");
                    }
                    if(is_file_or_directory_general(globbuf.gl_pathv[j])==1){
                        if(is_tracked_path(globbuf.gl_pathv[j])){
                            if(is_staged_path(globbuf.gl_pathv[j])){
                                printf("%s +\n", filename_maker(globbuf.gl_pathv[j]));
                            }
                            else{
                                printf("%s -\n", filename_maker(globbuf.gl_pathv[j]));
                            }
                        }
                        else{
                            printf("%s -\n", filename_maker(globbuf.gl_pathv[j]));
                        }
                    }
                }

                strcat(temp, "/");
                strcat(temp, "*");
            }
 */