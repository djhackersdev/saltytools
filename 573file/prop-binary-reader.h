#pragma once

#include <stddef.h>

#include "573file/prop.h"

int prop_binary_parse(struct prop **p, const void *bytes, size_t nbytes);
