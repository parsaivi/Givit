#ifndef DIFF_H
#define DIFF_H

#include "givit.h"

int run_diff(int argc, char *argv[]);
int run_merge(int argc, char *argv[]);

void compare_files(const char *path1, const char *path2,
                   int begin1, int end1, int begin2, int end2);
void compare_commit_dirs(const char *dir1, const char *dir2);
int merge_commit_dirs(const char *dir1, const char *dir2,
                      const char *branch1, const char *branch2);

#endif
