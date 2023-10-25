#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "573file/prop-binary-reader.h"
#include "573file/prop-type.h"
#include "573file/prop.h"

#include "util/iobuf.h"
#include "util/log.h"
#include "util/macro.h"

#define ALIGN32(x) (((x) + 3) & ~3)
#define INVALID_OFFSET ((size_t)-1)

struct prop_binary_parser {
  struct const_iobuf head;
  struct const_iobuf body;
  struct const_iobuf align_cave[2];
};

static const char prop_binary_name_chars[] =
    "0123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

static int prop_binary_read_node(struct prop_binary_parser *bp, uint8_t type,
                                 struct prop **p);
int prop_binary_header_read_name(struct prop_binary_parser *bp, char **out);
static int prop_binary_read_attr(struct prop_binary_parser *bp, struct prop *p);
static int prop_binary_slice_value(struct prop_binary_parser *bp, uint8_t type,
                                   struct const_iobuf *out);
static int prop_binary_body_slice_bytes(struct prop_binary_parser *bp,
                                        size_t nbytes, struct const_iobuf *out);
static int prop_binary_body_slice_cave(struct prop_binary_parser *bp,
                                       size_t nbytes, struct const_iobuf *out);

int prop_binary_parse(struct prop **out, const void *bytes, size_t nbytes) {
  struct prop *p;
  struct prop_binary_parser bp;
  struct const_iobuf file;
  uint32_t head_nbytes;
  uint32_t body_nbytes;
  uint8_t type;
  int r;

  assert(out != NULL);
  assert(bytes != NULL);

  *out = NULL;
  p = NULL;
  memset(&bp, 0, sizeof(bp));

  file.bytes = bytes;
  file.nbytes = nbytes;
  file.pos = 4; /* Skip initial magic # */

  r = iobuf_read_be32(&file, &head_nbytes);

  if (r < 0) {
    log_error(r);

    goto end;
  }

  r = iobuf_slice(&bp.head, &file, head_nbytes);

  if (r < 0) {
    log_error(r);

    goto end;
  }

  r = iobuf_align_read(&file, 4);

  if (r < 0) {
    log_error(r);

    goto end;
  }

  r = iobuf_read_be32(&file, &body_nbytes);

  if (r < 0) {
    log_error(r);

    goto end;
  }

  r = iobuf_slice(&bp.body, &file, body_nbytes);

  if (r < 0) {
    log_error(r);

    goto end;
  }

  r = iobuf_read_8(&bp.head, &type);

  if (r < 0) {
    log_write("Failed to read root node type code");

    goto end;
  }

  if (type == 0xFF) {
    log_write("Binary prop has no root node");
    r = -EBADMSG;

    goto end;
  }

  r = prop_binary_read_node(&bp, type, &p);

  if (r < 0) {
    goto end;
  }

  r = iobuf_read_8(&bp.head, &type);

  if (r < 0) {
    goto end;
  }

  if (type != 0xFF) {
    log_write("Expected EOF marker in header, got %#x", type);
    r = -EBADMSG;

    goto end;
  }

  *out = p;
  p = NULL;
  r = 0;

end:
  prop_free(p);

  return r;
}

static int prop_binary_read_node(struct prop_binary_parser *bp, uint8_t type,
                                 struct prop **out) {
  struct const_iobuf value;
  struct prop *child;
  struct prop *p;
  uint8_t child_type;
  char *name;
  int r;

  assert(bp != NULL);
  assert(out != NULL);

  *out = NULL;
  name = NULL;
  p = NULL;

  r = prop_binary_header_read_name(bp, &name);

  if (r < 0) {
    log_write("Failed to read name");

    goto end;
  }

  if (!prop_type_is_valid(type)) {
    log_write("\"%s\": Unsupported type code %#x", name, type);
    r = -ENOTSUP;

    goto end;
  }

  r = prop_binary_slice_value(bp, type, &value);

  if (r < 0) {
    log_write("\"%s\": Failed to read value of type %s", name,
              prop_type_to_string(type));

    goto end;
  }

  r = prop_alloc(&p, name, type, value.bytes, value.nbytes);

  if (r < 0) {
    goto end;
  }

  for (;;) {
    r = iobuf_read_8(&bp->head, &child_type);

    if (r < 0) {
      log_write("\"%s\": Failed to read next child's type code", name);

      goto end;
    }

    if (child_type == 0xFE) {
      break;
    } else if (child_type == PROP_ATTR) {
      r = prop_binary_read_attr(bp, p);

      if (r < 0) {
        goto end;
      }
    } else {
      r = prop_binary_read_node(bp, child_type, &child);

      if (r < 0) {
        goto end;
      }

      prop_append(p, child);
    }
  }

  *out = p;
  p = NULL;

end:
  free(name);
  prop_free(p);

  return r;
}

int prop_binary_header_read_name(struct prop_binary_parser *bp, char **out) {
  uint8_t nchars;
  uint8_t x;
  uint8_t y;
  uint8_t z;
  char *name;
  int i;
  int index;
  int r;

  *out = NULL;
  name = NULL;

  r = iobuf_read_8(&bp->head, &nchars);

  if (r < 0) {
    log_error(r);

    goto end;
  }

  name = malloc(nchars + 1);

  if (name == NULL) {
    r = -ENOMEM;

    goto end;
  }

  x = 0;
  y = 0;
  z = 0;

  for (i = 0; i < nchars; i++) {
    switch (i % 4) {
    case 0:
      r = iobuf_read_8(&bp->head, &x);

      if (r < 0) {
        log_error(r);

        goto end;
      }

      index = (x >> 2) & 0x3F;

      break;

    case 1:
      r = iobuf_read_8(&bp->head, &y);

      if (r < 0) {
        log_error(r);

        goto end;
      }

      index = ((x & 0x03) << 4) | ((y >> 4) & 0x0F);

      break;

    case 2:
      r = iobuf_read_8(&bp->head, &z);

      if (r < 0) {
        log_error(r);

        goto end;
      }

      index = ((y & 0x0F) << 2) | ((z >> 6) & 0x03);

      break;

    case 3:
      index = z & 0x3F;

      break;
    }

    assert(index < lengthof(prop_binary_name_chars));

    name[i] = prop_binary_name_chars[index];
  }

  name[nchars] = '\0';

  *out = name;
  name = NULL;
  r = 0;

end:
  free(name);

  return r;
}

static int prop_binary_read_attr(struct prop_binary_parser *bp,
                                 struct prop *p) {
  struct const_iobuf value;
  char *name;
  int r;

  name = NULL;
  r = prop_binary_header_read_name(bp, &name);

  if (r < 0) {
    goto end;
  }

  r = prop_binary_slice_value(bp, PROP_ATTR, &value);

  if (r < 0) {
    goto end;
  }

  if (value.nbytes == 0) {
    log_write("Attr @%s has zero length", name);
    r = -EBADMSG;

    goto end;
  }

  if (value.bytes[value.nbytes - 1] != '\0') {
    log_write("Attr @%s is not NUL terminated", name);
    r = -EBADMSG;

    goto end;
  }

  r = prop_set_attr(p, name, (const char *)value.bytes);

  if (r < 0) {
    goto end;
  }

  r = 0;

end:
  free(name);

  return r;
}

static int prop_binary_slice_value(struct prop_binary_parser *bp, uint8_t type,
                                   struct const_iobuf *out) {
  int orig_nbytes;
  uint32_t nbytes;
  bool is_variable;
  int r;

  assert(bp != NULL);
  assert(out != NULL);

  memset(out, 0, sizeof(*out));

  r = iobuf_align_read(&bp->body, 4);

  if (r < 0) {
    log_error(r);

    return r;
  }

  orig_nbytes = prop_type_to_size(type);

  if (orig_nbytes < 0) {
    is_variable = true;
    r = iobuf_read_be32(&bp->body, &nbytes);

    if (r < 0) {
      log_error(r);

      return r;
    }
  } else {
    is_variable = false;
    nbytes = orig_nbytes;
  }

  if (is_variable || nbytes >= 4) {
    return prop_binary_body_slice_bytes(bp, nbytes, out);
  } else if (nbytes > 0) {
    return prop_binary_body_slice_cave(bp, nbytes, out);
  }

  return 0;
}

static int prop_binary_body_slice_bytes(struct prop_binary_parser *bp,
                                        size_t nbytes,
                                        struct const_iobuf *out) {
  int r;

  r = iobuf_slice(out, &bp->body, nbytes);

  if (r < 0) {
    log_error(r);

    return r;
  }

  return 0;
}

static int prop_binary_body_slice_cave(struct prop_binary_parser *bp,
                                       size_t nbytes, struct const_iobuf *out) {
  struct const_iobuf *cave;
  int r;

  assert(bp != NULL);
  assert(nbytes == 1 || nbytes == 2);
  assert(out != NULL);

  cave = &bp->align_cave[nbytes - 1];

  if (cave->pos >= cave->nbytes) {
    r = iobuf_slice(cave, &bp->body, 4);

    if (r < 0) {
      log_error(r);

      return r;
    }
  }

  r = iobuf_slice(out, cave, nbytes);

  assert(r >= 0);

  return 0;
}
