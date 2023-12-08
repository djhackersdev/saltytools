#pragma once

#include <stddef.h>
#include <stdint.h>

struct md5_hash {
  uint8_t b[16];
};

int md5_compute(struct md5_hash *dest, const void *bytes, size_t nbytes);
