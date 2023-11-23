#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "573file/lz.h"

#include "util/iobuf.h"

#define LZCOMP_MIN_MATCH 3
#define LZCOMP_MAX_MATCH (LZCOMP_MIN_MATCH + 15)
#define LZCOMP_NIL ((uint16_t)-1)

struct lz_dec {
  const uint8_t *src_pos;
  const uint8_t *src_end;
  uint8_t ring[0x1000];
  uint16_t ring_pos;
  uint16_t flags;
  uint16_t copy_pos;
  uint8_t copy_len;
};

static int lz_dec_backref_begin(struct lz_dec *lz);
static int lz_dec_backref_getc(struct lz_dec *lz);
static int lz_dec_get_flag(struct lz_dec *lz);
static int lz_dec_getc(struct lz_dec *lz);

int lz_dec_decompress(const uint8_t *in_bytes, size_t in_nbytes,
                      struct iobuf *out) {
  struct lz_dec *lz;
  int byte;

  assert(in_bytes != NULL);
  assert(out != NULL);

  lz = calloc(1, sizeof(*lz));

  if (lz == NULL) {
    return -ENOMEM;
  }

  lz->src_pos = in_bytes;
  lz->src_end = in_bytes + in_nbytes;
  lz->flags = 0x0001;

  for (;;) {
    byte = lz_dec_getc(lz);

    if (byte < 0) {
      break;
    }

    if (out->bytes != NULL) {
      assert(out->pos < out->nbytes);

      out->bytes[out->pos] = (uint8_t)byte;
    }

    out->pos++;
  }

  free(lz);

  return 0;
}

static int lz_dec_getc(struct lz_dec *lz) {
  int byte;
  int flag;

  assert(lz != NULL);

  if (lz->src_pos == lz->src_end) {
    return -1;
  }

  if (lz->copy_len > 0) {
    byte = lz_dec_backref_getc(lz);
  } else {
    flag = lz_dec_get_flag(lz);

    if (flag < 0) {
      byte = -1;
    } else if (flag) {
      if (lz->src_pos < lz->src_end) {
        byte = *lz->src_pos++;
      } else {
        byte = -1;
      }
    } else {
      byte = lz_dec_backref_begin(lz);
    }
  }

  if (byte < 0) {
    return byte;
  }

  lz->ring[lz->ring_pos] = (uint8_t)byte;
  lz->ring_pos = (lz->ring_pos + 1) & 0x0FFF;

  return byte;
}

static int lz_dec_backref_getc(struct lz_dec *lz) {
  int byte;

  assert(lz != NULL);
  assert(lz->copy_len > 0);

  byte = lz->ring[lz->copy_pos];

  lz->copy_pos = (lz->copy_pos + 1) & 0x0FFF;
  lz->copy_len -= 1;

  return byte;
}

static int lz_dec_backref_begin(struct lz_dec *lz) {
  uint16_t copy_len;
  uint16_t copy_off;
  uint8_t hi;
  uint8_t lo;

  assert(lz != NULL);

  if (lz->src_pos + 1 >= lz->src_end) {
    return -1;
  }

  hi = *lz->src_pos++;
  lo = *lz->src_pos++;

  copy_len = lo & 0x0F;
  copy_off = (hi << 4) | (lo >> 4);

  /* This use of a backref marker as an explicit EOF is really weird */
  if (copy_off == 0) {
    lz->src_pos = NULL;
    lz->src_end = NULL;

    return -1;
  }

  lz->copy_len = copy_len + 3;
  lz->copy_pos = (lz->ring_pos - copy_off) & 0x0FFF;

  return lz_dec_backref_getc(lz);
}

static int lz_dec_get_flag(struct lz_dec *lz) {
  int result;

  assert(lz != NULL);

  if (lz->flags == 0x0001) {
    if (lz->src_pos == lz->src_end) {
      return -1;
    }

    lz->flags = 0x0100 | *lz->src_pos++;
  }

  result = lz->flags & 1;
  lz->flags >>= 1;

  return result;
}
