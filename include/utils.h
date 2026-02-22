#ifndef UTILS_H
#define UTILS_H

#include "givit.h"

void strip_newline(char *str);
bool is_empty_line(const char *line);
int check_line_content(const char *line);
char *extract_filename(const char *filepath);
void path_join(char *dest, const char *base, const char *name);
bool file_exists(const char *path);
bool dir_exists(const char *path);
int is_file_or_dir(const char *path);
void copy_file(const char *src, const char *dest);
void copy_dir(const char *src, const char *dest);
void remove_dir(const char *path);
void remove_working_files(void);
void ensure_dir(const char *path);
void copy_remaining_lines(FILE *src, FILE *dest);
bool files_differ(const char *path1, const char *path2);
void ensure_staged_subdirs(const char *dir_path);

#endif
