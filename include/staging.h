#ifndef STAGING_H
#define STAGING_H

#include "givit.h"

int run_add(int argc, char *argv[]);
int run_reset(int argc, char *argv[]);
int run_status(int argc, char *argv[]);

bool is_staged(const char *filepath);
bool is_tracked(const char *filepath);
void add_to_staging(const char *filepath);
void remove_from_staging(const char *filepath);
void add_file_or_dir(const char *filepath, const char *base_dir);
void unstage_file_or_dir(const char *filepath, const char *base_dir);
void save_undo_snapshot(void);
void restore_undo_snapshot(void);
int is_changed_from_last_commit(const char *filepath);

#endif
