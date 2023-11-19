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

enum ifs_stat_entry {
  IFS_STAT_ENTRY_OFFSET,
  IFS_STAT_ENTRY_NBYTES,
  IFS_STAT_ENTRY_TIMESTAMP,
  IFS_STAT_LENGTH_
};

struct ifs {
  FILE *f;
  uint32_t body_start;
  struct prop *root;
};

static int ifs_dirent_match(const struct prop *dirent, const char *path);

int ifs_open(struct ifs **out, const char *path) {
  struct ifs *ifs;
  struct iobuf dest;
  struct const_iobuf src;
  uint8_t header_bytes[0x24];
  uint32_t header_words[9];
  void *pp_bytes;
  size_t pp_nbytes;
  size_t i;
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
    log_write("%s: Error opening file: %s (%i)", path, strerror(-r), r);

    goto end;
  }

  dest.bytes = header_bytes;
  dest.nbytes = sizeof(header_bytes);
  dest.pos = 0;

  r = fs_read(ifs->f, &dest);

  if (r < 0) {
    log_write("%s: Error reading header: %s (%i)", path, strerror(-r), r);

    goto end;
  }

  src.bytes = header_bytes;
  src.nbytes = sizeof(header_bytes);
  src.pos = 0;

  for (i = 0; i < lengthof(header_words); i++) {
    r = iobuf_read_be32(&src, &header_words[i]);

    assert(r >= 0);
  }

  ifs->body_start = header_words[4];

  pp_nbytes = ifs->body_start - sizeof(header_bytes);
  pp_bytes = malloc(pp_nbytes);

  if (pp_bytes == NULL) {
    r = -ENOMEM;

    goto end;
  }

  dest.bytes = pp_bytes;
  dest.nbytes = pp_nbytes;
  dest.pos = 0;

  r = fs_read(ifs->f, &dest);

  if (r < 0) {
    log_write("%s: Error reading TOC: %s (%i)", path, strerror(-r), r);

    goto end;
  }

  r = prop_binary_parse(&ifs->root, pp_bytes, pp_nbytes);

  if (r < 0) {
    goto end;
  }

  if (!ifs_dirent_is_dir(ifs->root)) {
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

  prop_free(ifs->root);
  free(ifs);
}

const struct prop *ifs_get_root(struct ifs *ifs) {
  assert(ifs != NULL);

  return ifs->root;
}

int ifs_read_file(struct ifs *ifs, const struct prop *dirent, void *bytes,
                  size_t *nbytes_out) {
  struct iobuf dest;
  struct const_iobuf stat_buf;
  uint32_t stat[IFS_STAT_LENGTH_];
  uint32_t offset;
  uint32_t nbytes;
  size_t i;
  int r;

  assert(ifs != NULL);
  assert(dirent != NULL);
  assert(nbytes_out != NULL);

  if (prop_get_type(dirent) != PROP_3S32) {
    log_write("%s: Expected dirent to be PROP_3S32", prop_get_name(dirent));
    r = -EINVAL;

    return r;
  }

  prop_borrow_value(dirent, &stat_buf);

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
                prop_get_name(dirent), offset, strerror(-r), r);

      return r;
    }

    dest.bytes = bytes;
    dest.nbytes = nbytes;
    dest.pos = 0;

    r = fs_read(ifs->f, &dest);

    if (r < 0) {
      log_write("%s: IFS read failed (nbytes=%#x): %s (%i)",
                prop_get_name(dirent), nbytes, strerror(-r), r);

      return r;
    }
  }

  *nbytes_out = nbytes;

  return 0;
}

int ifs_dirent_lookup(const struct prop *dirent, const char *path,
                      const struct prop **out) {
  const struct prop *pos;
  int r;

  assert(dirent != NULL);
  assert(path != NULL);
  assert(out != NULL);

  *out = NULL;

  for (pos = ifs_dirent_get_first_child(dirent); pos != NULL;
       pos = ifs_dirent_get_next_sibling(pos)) {
    r = ifs_dirent_match(pos, path);

    if (r < 0) {
      return r;
    }

    if (r > 0) {
      *out = pos;

      return r;
    }
  }

  return 0;
}

static int ifs_dirent_match(const struct prop *dirent, const char *path) {
  char *str;
  int r;

  assert(dirent != NULL);
  assert(path != NULL);

  str = NULL;
  r = ifs_dirent_get_name(dirent, &str);

  if (r < 0) {
    goto end;
  }

  r = str_eq(str, path) ? 1 : 0;

end:
  free(str);

  return r;
}

const struct prop *ifs_dirent_get_first_child(const struct prop *dirent) {
  const struct prop *pos;

  assert(dirent != NULL);

  pos = prop_get_first_child_const(dirent);

  if (pos != NULL && str_eq(prop_get_name(pos), "_info_")) {
    pos = prop_get_next_sibling_const(pos);
  }

  return pos;
}

const struct prop *ifs_dirent_get_next_sibling(const struct prop *dirent) {
  assert(dirent != NULL);

  return prop_get_next_sibling_const(dirent);
}

int ifs_dirent_get_name(const struct prop *dirent, char **out) {
  bool escape;
  uint32_t i;
  uint32_t j;
  const char *raw;
  char *str;

  assert(dirent != NULL);
  assert(out != NULL);

  *out = NULL;

  raw = prop_get_name(dirent);
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

bool ifs_dirent_is_dir(const struct prop *dirent) {
  enum prop_type type;

  assert(dirent != NULL);

  type = prop_get_type(dirent);

  return type == PROP_VOID || type == PROP_S32;
}
