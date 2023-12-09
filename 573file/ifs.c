#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "573file/ifs.h"
#include "573file/prop-binary-reader.h"
#include "573file/prop.h"

#include "util/fs.h"
#include "util/iobuf.h"
#include "util/log.h"
#include "util/macro.h"
#include "util/str.h"

static const size_t ifs_header_size = 0x24;

enum ifs_stat_entry {
  IFS_STAT_ENTRY_OFFSET,
  IFS_STAT_ENTRY_NBYTES,
  IFS_STAT_ENTRY_TIMESTAMP,
  IFS_STAT_LENGTH_
};

struct ifs_header {
  uint32_t words[9];
};

struct ifs {
  FILE *f;
  uint32_t body_start;
  struct prop *toc;
};

static int ifs_header_read(FILE *f, struct ifs_header *header);
static int ifs_iter_match(const struct ifs_iter *dirent, const char *path);

static int ifs_header_read(FILE *f, struct ifs_header *header) {
  uint8_t header_bytes[ifs_header_size];
  struct const_iobuf src;
  struct iobuf dest;
  size_t i;
  int r;

  assert(f != NULL);
  assert(header != NULL);

  dest.bytes = header_bytes;
  dest.nbytes = sizeof(header_bytes);
  dest.pos = 0;

  r = fs_read(f, &dest);

  if (r < 0) {
    return r;
  }

  src.bytes = header_bytes;
  src.nbytes = sizeof(header_bytes);
  src.pos = 0;

  for (i = 0; i < lengthof(header->words); i++) {
    r = iobuf_read_be32(&src, &header->words[i]);

    assert(r >= 0);
  }

  return 0;
}

int ifs_open(struct ifs **out, const char *path) {
  struct ifs *ifs;
  struct ifs_header header;
  struct ifs_iter root;
  struct iobuf pp_buf;
  void *pp_bytes;
  size_t pp_nbytes;
  int r;

  assert(out != NULL);
  assert(path != NULL);

  *out = NULL;
  pp_bytes = NULL;

  ifs = calloc(1, sizeof(*ifs));

  if (ifs == NULL) {
    r = -ENOMEM;

    goto end;
  }

  r = fs_open(&ifs->f, path, "rb");

  if (r < 0) {
    goto end;
  }

  r = ifs_header_read(ifs->f, &header);

  if (r < 0) {
    log_write("%s: Error reading header: %s (%i)", path, strerror(-r), r);

    goto end;
  }

  ifs->body_start = header.words[4];

  pp_nbytes = ifs->body_start - ifs_header_size;
  pp_bytes = malloc(pp_nbytes);

  if (pp_bytes == NULL) {
    r = -ENOMEM;

    goto end;
  }

  pp_buf.bytes = pp_bytes;
  pp_buf.nbytes = pp_nbytes;
  pp_buf.pos = 0;

  r = fs_read(ifs->f, &pp_buf);

  if (r < 0) {
    log_write("%s: Error reading TOC: %s (%i)", path, strerror(-r), r);

    goto end;
  }

  r = prop_binary_parse(&ifs->toc, pp_bytes, pp_nbytes);

  if (r < 0) {
    goto end;
  }

  root.p = ifs->toc;

  if (!ifs_iter_is_dir(&root)) {
    log_write("%s: Root dirent is not a directory", path);
    r = -EBADMSG;

    goto end;
  }

  *out = ifs;
  ifs = NULL;
  r = 0;

end:
  free(pp_bytes);
  ifs_close(ifs);

  return r;
}

void ifs_close(struct ifs *ifs) {
  if (ifs == NULL) {
    return;
  }

  if (ifs->f != NULL) {
    fclose(ifs->f);
  }

  prop_free(ifs->toc);
  free(ifs);
}

void ifs_get_root(struct ifs *ifs, struct ifs_iter *out) {
  assert(ifs != NULL);
  assert(out != NULL);

  out->p = ifs->toc;
}

const struct prop *ifs_get_toc_data(const struct ifs *ifs) {
  assert(ifs != NULL);

  return ifs->toc;
}

int ifs_read_file(struct ifs *ifs, const struct ifs_iter *iter, void *bytes,
                  size_t *nbytes_out) {
  struct iobuf dest;
  struct const_iobuf stat_buf;
  uint32_t stat[IFS_STAT_LENGTH_];
  uint32_t offset;
  uint32_t nbytes;
  size_t i;
  int r;

  assert(ifs != NULL);
  assert(ifs_iter_is_valid(iter));
  assert(nbytes_out != NULL);

  if (prop_get_type(iter->p) != PROP_3S32) {
    log_write("%s: Expected dirent to be PROP_3S32", prop_get_name(iter->p));
    r = -EINVAL;

    return r;
  }

  prop_borrow_value(iter->p, &stat_buf);

  for (i = 0; i < lengthof(stat); i++) {
    r = iobuf_read_be32(&stat_buf, &stat[i]);

    assert(r >= 0);
  }

  nbytes = stat[IFS_STAT_ENTRY_NBYTES];

  if (bytes != NULL) {
    if (*nbytes_out < nbytes) {
      return -ENOSPC;
    }

    offset = ifs->body_start + stat[IFS_STAT_ENTRY_OFFSET];
    r = fs_seek_to(ifs->f, offset);

    if (r < 0) {
      log_write("%s: IFS seek failed (offset=%#x): %s (%i)",
                prop_get_name(iter->p), offset, strerror(-r), r);

      return r;
    }

    dest.bytes = bytes;
    dest.nbytes = nbytes;
    dest.pos = 0;

    r = fs_read(ifs->f, &dest);

    if (r < 0) {
      log_write("%s: IFS read failed (nbytes=%#x): %s (%i)",
                prop_get_name(iter->p), nbytes, strerror(-r), r);

      return r;
    }
  }

  *nbytes_out = nbytes;

  return 0;
}

void ifs_iter_init(struct ifs_iter *iter) {
  assert(iter != NULL);

  iter->p = NULL;
}

bool ifs_iter_is_valid(const struct ifs_iter *iter) {
  assert(iter != NULL);

  return iter->p != NULL;
}

int ifs_iter_lookup(const struct ifs_iter *parent, const char *path,
                    struct ifs_iter *child) {
  struct ifs_iter pos;
  int r;

  assert(parent != NULL);
  assert(path != NULL);
  assert(child != NULL);

  ifs_iter_init(child);

  for (ifs_iter_get_first_child(parent, &pos); ifs_iter_is_valid(&pos);
       ifs_iter_get_next_sibling(&pos)) {
    r = ifs_iter_match(&pos, path);

    if (r < 0) {
      return r;
    }

    if (r > 0) {
      *child = pos;

      return r;
    }
  }

  return 0;
}

static int ifs_iter_match(const struct ifs_iter *iter, const char *path) {
  char *str;
  int r;

  assert(iter != NULL);
  assert(path != NULL);

  str = NULL;
  r = ifs_iter_get_name(iter, &str);

  if (r < 0) {
    goto end;
  }

  r = str_eq(str, path) ? 1 : 0;

end:
  free(str);

  return r;
}

void ifs_iter_get_first_child(const struct ifs_iter *iter,
                              struct ifs_iter *out) {
  const struct prop *pos;

  assert(ifs_iter_is_valid(iter));
  assert(out != NULL);

  ifs_iter_init(out);
  pos = prop_get_first_child_const(iter->p);

  if (pos != NULL && str_eq(prop_get_name(pos), "_info_")) {
    pos = prop_get_next_sibling_const(pos);
  }

  out->p = pos;
}

void ifs_iter_get_next_sibling(struct ifs_iter *iter) {
  assert(ifs_iter_is_valid(iter));

  iter->p = prop_get_next_sibling_const(iter->p);
}

int ifs_iter_get_name(const struct ifs_iter *dirent, char **out) {
  bool escape;
  uint32_t i;
  uint32_t j;
  const char *raw;
  char *str;

  assert(ifs_iter_is_valid(dirent));
  assert(out != NULL);

  *out = NULL;

  raw = prop_get_name(dirent->p);
  escape = false;
  j = 0;

  for (i = 0; raw[i] != '\0'; i++) {
    if (escape) {
      escape = false;
      j++;
    } else if (raw[i] == '_') {
      escape = true;
    } else {
      j++;
    }
  }

  str = malloc(j + 1); /* Count terminating NUL */

  if (str == NULL) {
    return -ENOMEM;
  }

  escape = false;
  j = 0;

  for (i = 0; raw[i] != '\0'; i++) {
    if (escape) {
      if (raw[i] == 'E') {
        str[j++] = '.';
      } else {
        /* Probably some other escapes I don't know about ... */
        str[j++] = raw[i];
      }

      escape = false;
    } else if (raw[i] == '_') {
      escape = true;
    } else {
      str[j++] = raw[i];
    }
  }

  str[j] = '\0';
  *out = str;

  return 0;
}

bool ifs_iter_is_dir(const struct ifs_iter *dirent) {
  enum prop_type type;

  assert(ifs_iter_is_valid(dirent));

  type = prop_get_type(dirent->p);

  return type == PROP_VOID || type == PROP_S32;
}
