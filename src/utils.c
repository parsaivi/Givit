#include "utils.h"

void strip_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len - 1] == '\n')
        str[len - 1] = '\0';
}

bool is_empty_line(const char *line) {
    for (int i = 0; line[i]; i++) {
        if (line[i] != ' ' && line[i] != '\n' && line[i] != '\t')
            return false;
    }
    return true;
}

int check_line_content(const char *line) {
    return !is_empty_line(line) ? 1 : 0;
}

char *extract_filename(const char *filepath) {
    const char *last_slash = strrchr(filepath, '/');
    if (last_slash)
        return strdup(last_slash + 1);
    return strdup(filepath);
}

void path_join(char *dest, const char *base, const char *name) {
    strcpy(dest, base);
    int len = strlen(dest);
    if (len > 0 && dest[len - 1] != '/')
        strcat(dest, "/");
    strcat(dest, name);
}

bool file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

bool dir_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

int is_file_or_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    if (S_ISREG(st.st_mode)) return 1;
    if (S_ISDIR(st.st_mode)) return 2;
    return 0;
}

void copy_file(const char *src, const char *dest) {
    FILE *in = fopen(src, "rb");
    if (!in) return;
    FILE *out = fopen(dest, "wb");
    if (!out) { fclose(in); return; }
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        fwrite(buf, 1, n, out);
    fclose(in);
    fclose(out);
}

void copy_dir(const char *src, const char *dest) {
    char cmd[MAX_PATH_LEN * 3];
    snprintf(cmd, sizeof(cmd), "cp -r \"%s\" \"%s\"", src, dest);
    system(cmd);
}

void remove_dir(const char *path) {
    char cmd[MAX_PATH_LEN * 2];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", path);
    system(cmd);
}

void remove_working_files(void) {
    DIR *dir = opendir(".");
    if (!dir) return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, GIVIT_DIR) == 0)
            continue;
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "./%s", entry->d_name);
        if (entry->d_type == DT_DIR)
            remove_dir(path);
        else
            remove(path);
    }
    closedir(dir);
}

void ensure_dir(const char *path) {
    mkdir(path, 0755);
}

void copy_remaining_lines(FILE *src, FILE *dest) {
    char buf[MAX_LINE_LEN];
    while (fgets(buf, sizeof(buf), src) != NULL) {
        strip_newline(buf);
        fprintf(dest, "%s\n", buf);
    }
}

bool files_differ(const char *path1, const char *path2) {
    FILE *f1 = fopen(path1, "r");
    FILE *f2 = fopen(path2, "r");
    if (!f1 || !f2) {
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        return true;
    }
    char line1[MAX_LINE_LEN], line2[MAX_LINE_LEN];
    while (fgets(line1, sizeof(line1), f1) != NULL) {
        if (fgets(line2, sizeof(line2), f2) == NULL) {
            fclose(f1); fclose(f2);
            return true;
        }
        if (strcmp(line1, line2) != 0) {
            fclose(f1); fclose(f2);
            return true;
        }
    }
    bool diff = (fgets(line2, sizeof(line2), f2) != NULL);
    fclose(f1); fclose(f2);
    return diff;
}

void ensure_staged_subdirs(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_DIR) continue;
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, GIVIT_DIR) == 0)
            continue;
        char subdir[MAX_PATH_LEN], staged_subdir[MAX_PATH_LEN];
        path_join(subdir, dir_path, entry->d_name);
        snprintf(staged_subdir, sizeof(staged_subdir), "%s/staged/%s", GIVIT_DIR, subdir);
        ensure_dir(staged_subdir);
        ensure_staged_subdirs(subdir);
    }
    closedir(dir);
}
