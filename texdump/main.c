#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "573file/prop-binary-reader.h"
#include "573file/prop.h"
#include "573file/tex.h"

#include "picture/picture-png.h"
#include "picture/picture.h"

#include "util/fs.h"
#include "util/iobuf.h"
#include "util/log.h"
#include "util/str.h"

static int tex_dump_image(const struct tex_image *ti, const char *indir,
                          const char *outdir);
static int tex_dump_list(const struct tex_list *tl, const char *indir,
                         const char *outdir);
static int tex_dump_load_toc(struct tex_list **out, const char *indir);

static int tex_dump_load_toc(struct tex_list **out, const char *indir) {
  struct tex_list *tl;
  struct prop *p;
  char *toc_path;
  void *bytes;
  size_t nbytes;
  int r;

  assert(out != NULL);
  assert(indir != NULL);

  *out = NULL;
  toc_path = NULL;
  bytes = NULL;
  p = NULL;
  tl = NULL;

  r = str_printf(&toc_path, "%s/tex/texturelist.xml", indir);

  if (r < 0) {
    goto end;
  }

  r = fs_read_file(toc_path, &bytes, &nbytes);

  if (r < 0) {
    goto end;
  }

  r = prop_binary_parse(&p, bytes, nbytes);

  if (r < 0) {
    goto end;
  }

  r = tex_list_read(&tl, p);

  if (r < 0) {
    goto end;
  }

  *out = tl;
  tl = NULL;

end:
  tex_list_free(tl);
  prop_free(p);
  free(bytes);
  free(toc_path);

  return r;
}

static int tex_dump_list(const struct tex_list *tl, const char *indir,
                         const char *outdir) {
  const struct tex_texture *tx;
  const struct tex_image *ti;
  int r;

  assert(tl != NULL);
  assert(indir != NULL);
  assert(outdir != NULL);

  for (tx = tex_list_get_first_texture(tl); tx != NULL;
       tx = tex_texture_get_next_sibling(tx)) {
    for (ti = tex_texture_get_first_image(tx); ti != NULL;
         ti = tex_image_get_next_sibling(ti)) {
      r = tex_dump_image(ti, indir, outdir);

      if (r < 0) {
        return r;
      }
    }
  }

  return 0;
}

static int tex_dump_image(const struct tex_image *ti, const char *indir,
                          const char *outdir) {
  struct const_iobuf buf;
  struct picture *pic;
  void *bytes;
  size_t nbytes;
  char *in_path;
  char *out_path;
  int r;

  assert(ti != NULL);
  assert(indir != NULL);
  assert(outdir != NULL);

  in_path = NULL;
  bytes = NULL;
  pic = NULL;
  out_path = NULL;

  r = str_printf(&in_path, "%s/tex/%s", indir, ti->name_md5);

  if (r < 0) {
    goto end;
  }

  r = fs_read_file(in_path, &bytes, &nbytes);

  if (r < 0) {
    goto end;
  }

  buf.bytes = bytes;
  buf.nbytes = nbytes;
  buf.pos = 0;

  r = tex_image_read_pixels(ti, &buf, &pic);

  if (r < 0) {
    log_write("Error reading image \"%s\" (md5=%s)", ti->name, ti->name_md5);

    goto end;
  }

  r = str_printf(&out_path, "%s/%s.png", outdir, ti->name);

  if (r < 0) {
    goto end;
  }

  r = picture_write_png(pic, out_path);

  if (r < 0) {
    goto end;
  }

end:
  free(out_path);
  free(pic);
  free(bytes);
  free(in_path);

  return r;
}

int main(int argc, char **argv) {
  struct tex_list *tl;
  const char *indir;
  const char *outdir;
  int r;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s [indir] [outdir]\n", argv[0]);

    return EXIT_FAILURE;
  }

  tl = NULL;
  indir = argv[1];
  outdir = argv[2];

  r = tex_dump_load_toc(&tl, indir);

  if (r < 0) {
    goto end;
  }

  r = fs_mkdir(outdir);

  if (r < 0) {
    goto end;
  }

  r = tex_dump_list(tl, indir, outdir);

  if (r < 0) {
    goto end;
  }

end:
  tex_list_free(tl);

  return r >= 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
