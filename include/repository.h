#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "givit.h"

int run_init(int argc, char *argv[]);
int run_config(int argc, char *argv[]);
int run_global_config(int argc, char *argv[]);

bool repo_exists(void);
int repo_read_config(char *username, char *email, char *branch, char *parent_branch);
int repo_write_branch(const char *branch, const char *parent_branch);
void repo_read_global_config(char *username, char *email);

#endif
