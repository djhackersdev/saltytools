#pragma once

#include <stdio.h>

#include "util/iobuf.h"

int fs_open(FILE **out, const char *path, const char *mode);
void fs_close(FILE *f);
int fs_seek_to(FILE *f, size_t off);
int fs_read(FILE *f, struct iobuf *buf);
int fs_write(FILE *f, struct const_iobuf *buf);

int fs_read_file(const char *path, void **bytes, size_t *nbytes);
int fs_write_file(const char *path, struct const_iobuf *buf);
int fs_mkdir(const char *path);
