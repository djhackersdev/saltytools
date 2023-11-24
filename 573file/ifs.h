#pragma once

#include <stdbool.h>

#include "573file/prop.h"

struct ifs;

struct ifs_iter {
  const struct prop *p;
};

int ifs_open(struct ifs **ifs, const char *path);
void ifs_close(struct ifs *ifs);
void ifs_get_root(struct ifs *ifs, struct ifs_iter *out);
const struct prop *ifs_get_toc_data(const struct ifs *ifs);
int ifs_read_file(struct ifs *ifs, const struct ifs_iter *iter, void *bytes,
                  size_t *nbytes);

void ifs_iter_init(struct ifs_iter *iter);
bool ifs_iter_is_valid(const struct ifs_iter *iter);
int ifs_iter_lookup(const struct ifs_iter *iter, const char *path,
                    struct ifs_iter *out);
void ifs_iter_get_first_child(const struct ifs_iter *iter,
                              struct ifs_iter *out);
void ifs_iter_get_next_sibling(struct ifs_iter *iter);
int ifs_iter_get_name(const struct ifs_iter *iter, char **out);
bool ifs_iter_is_dir(const struct ifs_iter *iter);
