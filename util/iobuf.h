#pragma once

#include <stddef.h>
#include <stdint.h>

struct iobuf {
  uint8_t *bytes;
  size_t nbytes;
  size_t pos;
};

struct const_iobuf {
  const uint8_t *bytes;
  size_t nbytes;
  size_t pos;
};

int iobuf_slice(struct const_iobuf *dest, struct const_iobuf *src,
                size_t nbytes);
int iobuf_align_read(struct const_iobuf *buf, size_t alignment);

int iobuf_peek(struct const_iobuf *src, void *bytes, size_t nbytes);
int iobuf_read(struct const_iobuf *src, void *bytes, size_t nbytes);
int iobuf_read_8(struct const_iobuf *src, uint8_t *value);
int iobuf_read_be16(struct const_iobuf *src, uint16_t *value);
int iobuf_read_be32(struct const_iobuf *src, uint32_t *value);
int iobuf_read_be64(struct const_iobuf *src, uint64_t *value);
int iobuf_read_le16(struct const_iobuf *src, uint16_t *value);
int iobuf_read_le32(struct const_iobuf *src, uint32_t *value);
int iobuf_read_le64(struct const_iobuf *src, uint64_t *value);

void iobuf_write(struct iobuf *dest, const void *bytes, size_t nbytes);
void iobuf_write_8(struct iobuf *dest, uint8_t value);
void iobuf_write_be16(struct iobuf *dest, uint16_t value);
void iobuf_write_be32(struct iobuf *dest, uint32_t value);
void iobuf_write_be64(struct iobuf *dest, uint64_t value);
void iobuf_write_le16(struct iobuf *dest, uint16_t value);
void iobuf_write_le32(struct iobuf *dest, uint32_t value);
void iobuf_write_le64(struct iobuf *dest, uint64_t value);
