#include <assert.h>
#include <stdbool.h>

#include "573file/prop-type.h"

#include "util/macro.h"

static const char *const prop_type_names[64] = {
    [PROP_VOID] = "void", [PROP_S8] = "s8",     [PROP_U8] = "u8",
    [PROP_S16] = "s16",   [PROP_U16] = "u16",   [PROP_S32] = "s32",
    [PROP_U32] = "u32",   [PROP_S64] = "s64",   [PROP_U64] = "u64",
    [PROP_BIN] = "bin",   [PROP_STR] = "str",   [PROP_IP4] = "ip4",
    [PROP_TIME] = "time", [PROP_2U16] = "2u16", [PROP_3S32] = "3s32",
    [PROP_4U16] = "4u16", [PROP_BOOL] = "bool",
};

static const int prop_type_sizes[64] = {
    [PROP_VOID] = 0,  [PROP_S8] = 1,   [PROP_U8] = 1,    [PROP_S16] = 2,
    [PROP_U16] = 2,   [PROP_S32] = 4,  [PROP_U32] = 4,   [PROP_S64] = 8,
    [PROP_U64] = 8,   [PROP_BIN] = -1, [PROP_STR] = -1,  [PROP_IP4] = 4,
    [PROP_TIME] = 4,  [PROP_2U16] = 4, [PROP_3S32] = 12, [PROP_4U16] = 8,
    [PROP_ATTR] = -1, [PROP_BOOL] = 1,
};

bool prop_type_is_valid(enum prop_type type) {
  if (type < 0 || type >= lengthof(prop_type_names)) {
    return false;
  }

  return prop_type_names[type] != NULL;
}

const char *prop_type_to_string(enum prop_type type) {
  assert(prop_type_is_valid(type));

  return prop_type_names[type];
}

int prop_type_to_size(enum prop_type type) {
  assert(type == PROP_ATTR /* hack */ || prop_type_is_valid(type));

  return prop_type_sizes[type];
}
