#pragma once

enum prop_type {
  PROP_INVALID_TYPE = 0x00,
  PROP_VOID = 0x01,
  PROP_S8 = 0x02,
  PROP_U8 = 0x03,
  PROP_S16 = 0x04,
  PROP_U16 = 0x05,
  PROP_S32 = 0x06,
  PROP_U32 = 0x07,
  PROP_S64 = 0x08,
  PROP_U64 = 0x09,
  PROP_BIN = 0x0A,
  PROP_STR = 0x0B,
  PROP_IP4 = 0x0C,
  PROP_TIME = 0x0D,
  PROP_2U16 = 0x13,
  PROP_3S32 = 0x1E,
  PROP_4U16 = 0x27,
  PROP_ATTR = 0x2E, /* Pseudo-type */
  PROP_BOOL = 0x34,
  PROP_ARRAY_FLAG = 0x40
};

bool prop_type_is_array(enum prop_type type);
bool prop_type_is_valid(enum prop_type type);
const char *prop_type_to_string(enum prop_type type);
int prop_type_to_size(enum prop_type type);
