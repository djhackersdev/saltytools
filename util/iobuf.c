#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "util/iobuf.h"

static int iobuf_bounds_check(const struct const_iobuf *buf, size_t nbytes);

static int iobuf_bounds_check(const struct const_iobuf *buf, size_t nbytes) {
  assert(buf != NULL);

  if (buf->pos + nbytes > buf->nbytes) {
    return -ENODATA;
  }

  return 0;
}

int iobuf_slice(struct const_iobuf *dest, struct const_iobuf *src,
                size_t nbytes) {
  int r;

  assert(dest != NULL);
  assert(src != NULL);

  r = iobuf_bounds_check(src, nbytes);

  if (r < 0) {
    return r;
  }

  dest->bytes = src->bytes + src->pos;
  dest->nbytes = nbytes;
  dest->pos = 0;

  src->pos += nbytes;

  return 0;
}

int iobuf_align_read(struct const_iobuf *buf, size_t alignment) {
  size_t mask;
  size_t new_pos;

  assert(buf != NULL);
  assert(alignment != 0);

  mask = alignment - 1;

  assert((alignment & mask) == 0);

  new_pos = (buf->pos + mask) & ~mask;

  if (new_pos > buf->nbytes) {
    return -ENODATA;
  }

  buf->pos = new_pos;

  return 0;
}

int iobuf_peek(struct const_iobuf *src, void *bytes, size_t nbytes) {
  int r;

  assert(src != NULL);
  assert(bytes != NULL);

  r = iobuf_bounds_check(src, nbytes);

  if (r < 0) {
    return r;
  }

  memcpy(bytes, &src->bytes[src->pos], nbytes);

  return 0;
}

int iobuf_read(struct const_iobuf *src, void *bytes, size_t nbytes) {
  int r;

  assert(src != NULL);
  assert(bytes != NULL);

  r = iobuf_bounds_check(src, nbytes);

  if (r < 0) {
    return r;
  }

  memcpy(bytes, src->bytes + src->pos, nbytes);
  src->pos += nbytes;

  return 0;
}

int iobuf_read_8(struct const_iobuf *src, uint8_t *out) {
  assert(src != NULL);
  assert(out != NULL);

  if (src->pos + sizeof(uint8_t) > src->nbytes) {
    return -ENODATA;
  }

  *out = src->bytes[src->pos++];

  return 0;
}

int iobuf_read_be16(struct const_iobuf *src, uint16_t *out) {
  uint16_t value;

  assert(src != NULL);
  assert(out != NULL);

  if (src->pos + sizeof(uint16_t) > src->nbytes) {
    return -ENODATA;
  }

  value = src->bytes[src->pos++] << 8;
  value |= src->bytes[src->pos++];

  *out = value;

  return 0;
}

int iobuf_read_be32(struct const_iobuf *src, uint32_t *out) {
  uint32_t value;

  assert(src != NULL);
  assert(out != NULL);

  if (src->pos + sizeof(uint32_t) > src->nbytes) {
    return -ENODATA;
  }

  value = src->bytes[src->pos++] << 24;
  value |= src->bytes[src->pos++] << 16;
  value |= src->bytes[src->pos++] << 8;
  value |= src->bytes[src->pos++];

  *out = value;

  return 0;
}

int iobuf_read_be64(struct const_iobuf *src, uint64_t *out) {
  uint64_t value;

  assert(src != NULL);
  assert(out != NULL);

  if (src->pos + sizeof(uint64_t) > src->nbytes) {
    return -ENODATA;
  }

  value = ((uint64_t)src->bytes[src->pos++]) << 56;
  value |= ((uint64_t)src->bytes[src->pos++]) << 48;
  value |= ((uint64_t)src->bytes[src->pos++]) << 40;
  value |= ((uint64_t)src->bytes[src->pos++]) << 32;
  value |= ((uint64_t)src->bytes[src->pos++]) << 24;
  value |= ((uint64_t)src->bytes[src->pos++]) << 16;
  value |= ((uint64_t)src->bytes[src->pos++]) << 8;
  value |= ((uint64_t)src->bytes[src->pos++]);

  *out = value;

  return 0;
}

int iobuf_read_le16(struct const_iobuf *src, uint16_t *out) {
  uint16_t value;

  assert(src != NULL);
  assert(out != NULL);

  if (src->pos + sizeof(uint16_t) > src->nbytes) {
    return -ENODATA;
  }

  value = src->bytes[src->pos++];
  value |= src->bytes[src->pos++] << 8;

  *out = value;

  return 0;
}

int iobuf_read_le32(struct const_iobuf *src, uint32_t *out) {
  uint32_t value;

  assert(src != NULL);
  assert(out != NULL);

  if (src->pos + sizeof(uint32_t) > src->nbytes) {
    return -ENODATA;
  }

  value = src->bytes[src->pos++];
  value |= src->bytes[src->pos++] << 8;
  value |= src->bytes[src->pos++] << 16;
  value |= src->bytes[src->pos++] << 24;

  *out = value;

  return 0;
}

int iobuf_read_le64(struct const_iobuf *src, uint64_t *out) {
  uint64_t value;

  assert(src != NULL);
  assert(out != NULL);

  if (src->pos + sizeof(uint64_t) > src->nbytes) {
    return -ENODATA;
  }

  value = ((uint64_t)src->bytes[src->pos++]);
  value |= ((uint64_t)src->bytes[src->pos++]) << 8;
  value |= ((uint64_t)src->bytes[src->pos++]) << 16;
  value |= ((uint64_t)src->bytes[src->pos++]) << 24;
  value |= ((uint64_t)src->bytes[src->pos++]) << 32;
  value |= ((uint64_t)src->bytes[src->pos++]) << 40;
  value |= ((uint64_t)src->bytes[src->pos++]) << 48;
  value |= ((uint64_t)src->bytes[src->pos++]) << 56;

  *out = value;

  return 0;
}

void iobuf_write(struct iobuf *dest, const void *bytes, size_t nbytes) {
  assert(dest != NULL);
  assert(bytes != NULL || nbytes == 0);

  if (dest->pos + nbytes <= dest->nbytes) {
    memcpy(dest->bytes + dest->pos, bytes, nbytes);
  }

  dest->pos += nbytes;
}

void iobuf_write_8(struct iobuf *dest, uint8_t value) {
  assert(dest != NULL);

  if (dest->pos + 1 <= dest->nbytes) {
    dest->bytes[dest->pos] = value;
  }

  dest->pos += 1;
}

void iobuf_write_be16(struct iobuf *dest, uint16_t value) {
  assert(dest != NULL);

  if (dest->pos + 2 <= dest->nbytes) {
    dest->bytes[dest->pos + 0] = value >> 8;
    dest->bytes[dest->pos + 1] = value;
  }

  dest->pos += 2;
}

void iobuf_write_be32(struct iobuf *dest, uint32_t value) {
  assert(dest != NULL);

  if (dest->pos + 4 <= dest->nbytes) {
    dest->bytes[dest->pos + 0] = value >> 24;
    dest->bytes[dest->pos + 1] = value >> 16;
    dest->bytes[dest->pos + 2] = value >> 8;
    dest->bytes[dest->pos + 3] = value;
  }

  dest->pos += 4;
}

void iobuf_write_be64(struct iobuf *dest, uint64_t value) {
  assert(dest != NULL);

  if (dest->pos + 8 <= dest->nbytes) {
    dest->bytes[dest->pos + 0] = value >> 56;
    dest->bytes[dest->pos + 1] = value >> 48;
    dest->bytes[dest->pos + 2] = value >> 40;
    dest->bytes[dest->pos + 3] = value >> 32;
    dest->bytes[dest->pos + 4] = value >> 24;
    dest->bytes[dest->pos + 5] = value >> 16;
    dest->bytes[dest->pos + 6] = value >> 8;
    dest->bytes[dest->pos + 7] = value;
  }

  dest->pos += 8;
}

void iobuf_write_le16(struct iobuf *dest, uint16_t value) {
  assert(dest != NULL);

  if (dest->pos + 2 <= dest->nbytes) {
    dest->bytes[dest->pos + 0] = value;
    dest->bytes[dest->pos + 1] = value >> 8;
  }

  dest->pos += 2;
}

void iobuf_write_le32(struct iobuf *dest, uint32_t value) {
  assert(dest != NULL);

  if (dest->pos + 4 <= dest->nbytes) {
    dest->bytes[dest->pos + 0] = value;
    dest->bytes[dest->pos + 1] = value >> 8;
    dest->bytes[dest->pos + 2] = value >> 16;
    dest->bytes[dest->pos + 3] = value >> 24;
  }

  dest->pos += 4;
}

void iobuf_write_le64(struct iobuf *dest, uint64_t value) {
  assert(dest != NULL);

  if (dest->pos + 8 <= dest->nbytes) {
    dest->bytes[dest->pos + 0] = value;
    dest->bytes[dest->pos + 1] = value >> 8;
    dest->bytes[dest->pos + 2] = value >> 16;
    dest->bytes[dest->pos + 3] = value >> 24;
    dest->bytes[dest->pos + 4] = value >> 32;
    dest->bytes[dest->pos + 5] = value >> 40;
    dest->bytes[dest->pos + 6] = value >> 48;
    dest->bytes[dest->pos + 7] = value >> 56;
  }

  dest->pos += 8;
}
