#include <assert.h>
#include <errno.h>
#include <string.h>

#include "573file/lz.h"

#include "util/iobuf.h"
#include "util/log.h"

int lz_file_read(struct const_iobuf *src, void **out_bytes,
                 size_t *out_nbytes) {
  struct iobuf dest;
  uint32_t h_orig_size;
  uint32_t h_comp_size;
  size_t orig_size;
  size_t comp_size;
  const void *payload;
  void *bytes;
  int r;

  assert(src != NULL);
  assert(out_bytes != NULL);
  assert(out_nbytes != NULL);

  bytes = NULL;
  *out_bytes = NULL;
  *out_nbytes = 0;

  r = iobuf_read_be32(src, &h_orig_size);

  if (r < 0) {
    log_error(r);

    goto end;
  }

  r = iobuf_read_be32(src, &h_comp_size);

  if (r < 0) {
    log_error(r);

    goto end;
  }

  comp_size = src->nbytes - src->pos;

  if (comp_size != h_comp_size) {
    r = -EBADMSG;
    log_write(
        "Compressed size mismatch: Header says %#x bytes, actual size is %#lx",
        h_comp_size, (unsigned long)comp_size);

    goto end;
  }

  payload = src->bytes + src->pos;
  memset(&dest, 0, sizeof(dest));
  lz_dec_decompress(payload, comp_size, &dest);

  orig_size = dest.pos;

  if (orig_size != h_orig_size) {
    r = -EBADMSG;
    log_write(
        "Original size mismatch: Header says %#x bytes, actual size is #%lx",
        h_orig_size, (unsigned long)orig_size);

    goto end;
  }

  bytes = malloc(orig_size);

  if (bytes == NULL) {
    r = -EBADMSG;

    goto end;
  }

  dest.bytes = bytes;
  dest.nbytes = orig_size;
  dest.pos = 0;

  lz_dec_decompress(payload, comp_size, &dest);

  *out_bytes = bytes;
  *out_nbytes = orig_size;
  bytes = NULL;

end:
  free(bytes);

  return r;
}
