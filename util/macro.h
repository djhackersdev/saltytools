#pragma once

#include <stddef.h>

#ifdef __GNUC__
#define gcc_attribute(x) __attribute__(x)
#else
#define gcc_attribute(x)
#endif

#define containerof(member, outer_type, outer_member)                          \
  ((outer_type *)(((char *)member) - offsetof(outer_type, outer_member)))

#define lengthof(x) (sizeof(x) / sizeof(x[0]))
