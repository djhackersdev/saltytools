#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <unistd.h>

#include "util/fs.h"
#include "util/iobuf.h"
#include "util/log.h"

int fs_open(FILE **out, const char *path, const char *mode) {
  FILE *f;

  assert(out != NULL);
  assert(path != NULL);
  assert(mode != NULL);

  *out = NULL;
  f = fopen(path, mode);

  if (f == NULL) {
    return -errno;
  }

  *out = f;

  return 0;
}

void fs_close(FILE *f) {
  if (f != NULL) {
    fclose(f);
  }
}

int fs_seek_to(FILE *f, size_t off) {
  int r;

  assert(f != NULL);

  r = fseek(f, off, SEEK_SET);

  if (r != 0) {
    r = -errno;
  }

  return r;
}

int fs_read(FILE *f, struct iobuf *buf) {
  size_t nbytes;
  size_t nread;
  int r;

  assert(f != NULL);
  assert(buf != NULL);
  assert(buf->pos <= buf->nbytes);

  nbytes = buf->nbytes - buf->pos;
  nread = fread(buf->bytes + buf->pos, 1, nbytes, f);

  if (nread < nbytes) {
    r = -errno;

    if (r != 0) {
      log_write("Read failed: %s (%i)", strerror(-r), r);
    } else {
      r = -ENODATA;
      log_write("Short read: %llu < %llu", (unsigned long long)nread,
                (unsigned long long)nbytes);
    }

    return r;
  }

  buf->pos += nbytes;

  return 0;
}

int fs_write(FILE *f, struct const_iobuf *buf) {
  size_t nbytes;
  size_t nwrit;
  int r;

  assert(f != NULL);
  assert(buf != NULL);
  assert(buf->pos <= buf->nbytes);

  nbytes = buf->nbytes - buf->pos;
  nwrit = fwrite(buf->bytes + buf->pos, 1, nbytes, f);

  if (nwrit < nbytes) {
    r = -errno;

    if (r != 0) {
      log_write("Write failed: %s (%i)", strerror(-r), r);
    } else {
      r = -EINTR; /* I guess..??? */
      log_write("Short write: %llu < %llu", (unsigned long long)nwrit,
                (unsigned long long)nbytes);
    }

    return r;
  }

  buf->pos += nbytes;

  return 0;
}

int fs_read_file(const char *path, void **out_bytes, size_t *out_nbytes) {
  FILE *f;
  struct iobuf buf;
  void *bytes;
  long nbytes;
  int r;

  assert(path != NULL);
  assert(out_bytes != NULL);
  assert(out_nbytes != NULL);

  bytes = NULL;
  *out_bytes = NULL;
  *out_nbytes = 0;

  r = fs_open(&f, path, "rb");

  if (r < 0) {
    goto end;
  }

  r = fseek(f, 0, SEEK_END);

  if (r != 0) {
    r = -errno;
    log_write("fseek(SEEK_END): %s (%i)", strerror(-r), r);

    goto end;
  }

  nbytes = ftell(f);
  r = fseek(f, 0, SEEK_SET);

  if (r != 0) {
    r = -errno;
    log_write("fseek(SEEK_SET): %s (%i)", strerror(-r), r);

    goto end;
  }

  bytes = malloc(nbytes);

  if (bytes == NULL) {
    r = -ENOMEM;

    goto end;
  }

  buf.bytes = bytes;
  buf.nbytes = nbytes;
  buf.pos = 0;

  r = fs_read(f, &buf);

  if (r < 0) {
    goto end;
  }

  *out_bytes = bytes;
  *out_nbytes = nbytes;
  bytes = NULL;

end:
  free(bytes);
  fs_close(f);

  return r;
}

int fs_write_file(const char *path, struct const_iobuf *buf) {
  FILE *f;
  int r;

  assert(path != NULL);
  assert(buf != NULL);

  f = NULL;
  r = fs_open(&f, path, "wb");

  if (r < 0) {
    goto end;
  }

  r = fs_write(f, buf);

  if (r < 0) {
    goto end;
  }

end:
  fs_close(f);

  return r;
}

int fs_mkdir(const char *path) {
  struct stat s;
  int r;

  assert(path != NULL);

  r = stat(path, &s);

  if (r == 0) {
    /* Early return on _success_, not failure. */
    return 0;
  }

  r = -errno;

  if (r != -ENOENT) {
    log_write("stat(%s): %i (%s)", path, r, strerror(-r));

    return r;
  }

#ifdef _WIN32
  r = mkdir(path);
#else
  r = mkdir(path, 0755);
#endif

  if (r != 0) {
    r = -errno;
  }

  return r;
}
