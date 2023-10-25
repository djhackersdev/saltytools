#pragma once

#include <stdbool.h>

#include "573file/prop.h"

struct ifs;

int ifs_open(struct ifs **ifs, const char *path);
void ifs_close(struct ifs *ifs);
const struct prop *ifs_get_root(struct ifs *ifs);
int ifs_read_file(struct ifs *ifs, const struct prop *dirent, void *bytes,
                  size_t *nbytes);

int ifs_dirent_lookup(const struct prop *dirent, const char *path,
                      const struct prop **out);
const struct prop *ifs_dirent_get_first_child(const struct prop *dirent);
const struct prop *ifs_dirent_get_next_sibling(const struct prop *dirent);
int ifs_dirent_get_name(const struct prop *dirent, char **out);
bool ifs_dirent_is_dir(const struct prop *dirent);
