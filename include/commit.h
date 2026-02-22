#ifndef COMMIT_H
#define COMMIT_H

#include "givit.h"

Commit *commit_create(const char *message, const char *snapshot_path);
void commit_save_list(Commit *head, const char *filename);
Commit *commit_load_list(const char *filename);
void commit_free_list(Commit *head);
Commit *commit_find_by_id(Commit *head, int id);
Commit *commit_find_by_branch(Commit *head, const char *branch);
Commit *commit_fix_links(Commit *head);
int commit_next_id(void);
Commit *commit_create_snapshot(const char *message, Commit *head);
Commit *commit_create_for_branch(Commit *base, const char *branch,
                                  const char *parent_branch, Commit *head);

int run_commit(int argc, char *argv[]);
int run_set_shortcut(int argc, char *argv[]);
int run_replace_shortcut(int argc, char *argv[]);
int run_remove_shortcut(int argc, char *argv[]);

#endif
