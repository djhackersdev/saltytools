#pragma once

#include <stddef.h>

int hex_encode_lc(char **out, const void *bytes, size_t nbytes);
int hex_encode_uc(char **out, const void *bytes, size_t nbytes);
