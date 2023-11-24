#pragma once

#include <stddef.h>

#include "util/iobuf.h"

int lz_file_read(struct const_iobuf *src, void **out_bytes, size_t *out_nbytes);
