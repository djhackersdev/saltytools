#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "573file/ifs.h"
#include "573file/prop-xml-writer.h"

#include "util/fs.h"
#include "util/log.h"
#include "util/str.h"

static int ifs_dump_child(struct ifs *ifs, const struct ifs_iter *child,
                          const char *parent_path);
static int ifs_dump_dir(struct ifs *ifs, const struct ifs_iter *dirent,
                        const char *path);
static int ifs_dump_file(struct ifs *ifs, const struct ifs_iter *child,
                         const char *path);
static int ifs_dump_toc(struct ifs *ifs, const char *dir);

int main(int argc, char **argv) {
  const char *infile;
  const char *outdir;
  struct ifs_iter root;
  struct ifs *ifs;
  int r;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s [infile] [outdir]\n", argv[0]);

    return EXIT_FAILURE;
  }

  infile = argv[1];
  outdir = argv[2];
  ifs = NULL;

  r = ifs_open(&ifs, infile);

  if (r < 0) {
    goto end;
  }

  r = fs_mkdir(outdir);

  if (r < 0) {
    goto end;
  }

  r = ifs_dump_toc(ifs, outdir);

  if (r < 0) {
    goto end;
  }

  ifs_get_root(ifs, &root);
  r = ifs_dump_child(ifs, &root, outdir);

  if (r < 0) {
    goto end;
  }

end:
  ifs_close(ifs);

  if (r < 0) {
    log_write("%s (%i)", strerror(-r), r);

    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

static int ifs_dump_toc(struct ifs *ifs, const char *dir) {
  struct const_iobuf buf;
  char *path;
  char *xml;
  int r;

  assert(ifs != NULL);
  assert(dir != NULL);

  path = NULL;
  xml = NULL;

  r = str_printf(&path, "%s/toc.xml", dir);

  if (r < 0) {
    goto end;
  }

  r = prop_xml_write(ifs_get_toc_data(ifs), &xml);

  if (r < 0) {
    goto end;
  }

  buf.bytes = (uint8_t *)xml;
  buf.nbytes = strlen(xml);
  buf.pos = 0;

  r = fs_write_file(path, &buf);

  if (r < 0) {
    goto end;
  }

end:
  free(xml);
  free(path);

  return r;
}

static int ifs_dump_dir(struct ifs *ifs, const struct ifs_iter *parent,
                        const char *path) {
  struct ifs_iter child;
  int r;

  assert(ifs != NULL);
  assert(parent != NULL);
  assert(path != NULL);

  for (ifs_iter_get_first_child(parent, &child); ifs_iter_is_valid(&child);
       ifs_iter_get_next_sibling(&child)) {
    r = ifs_dump_child(ifs, &child, path);

    if (r < 0) {
      return r;
    }
  }

  return 0;
}

static int ifs_dump_child(struct ifs *ifs, const struct ifs_iter *child,
                          const char *parent_path) {
  char *name;
  char *path;
  int r;

  assert(ifs != NULL);
  assert(child != NULL);
  assert(parent_path != NULL);

  name = NULL;
  path = NULL;

  r = ifs_iter_get_name(child, &name);

  if (r < 0) {
    goto end;
  }

  r = str_printf(&path, "%s/%s", parent_path, name);

  if (r < 0) {
    goto end;
  }

  if (ifs_iter_is_dir(child)) {
    r = fs_mkdir(path);

    if (r < 0) {
      goto end;
    }

    r = ifs_dump_dir(ifs, child, path);
  } else {
    r = ifs_dump_file(ifs, child, path);
  }

end:
  free(path);
  free(name);

  return r;
}

static int ifs_dump_file(struct ifs *ifs, const struct ifs_iter *child,
                         const char *path) {
  struct const_iobuf src;
  void *bytes;
  size_t nbytes;
  FILE *f;
  int r;

  assert(ifs != NULL);
  assert(child != NULL);
  assert(path != NULL);

  bytes = NULL;
  f = NULL;

  r = ifs_read_file(ifs, child, NULL, &nbytes);

  if (r < 0) {
    goto end;
  }

  bytes = malloc(nbytes);

  if (bytes == NULL) {
    r = -ENOMEM;

    goto end;
  }

  r = ifs_read_file(ifs, child, bytes, &nbytes);

  if (r < 0) {
    goto end;
  }

  r = fs_open(&f, path, "wb");

  if (r < 0) {
    log_write("fopen %s: %s (%i)", path, strerror(-r), r);

    goto end;
  }

  src.bytes = bytes;
  src.nbytes = nbytes;
  src.pos = 0;

  r = fs_write(f, &src);

  if (r < 0) {
    log_write("fwrite %s: %s (%i)", path, strerror(-r), r);

    goto end;
  }

end:
  fs_close(f);
  free(bytes);

  return r;
}
