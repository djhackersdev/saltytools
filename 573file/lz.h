#pragma once

#include <stddef.h>
#include <stdint.h>

#include "util/iobuf.h"

int lz_dec_decompress(const uint8_t *in_bytes, size_t in_nbytes,
                      struct iobuf *out);
