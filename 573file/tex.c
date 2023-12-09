#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "573file/lz-file.h"
#include "573file/prop-type.h"
#include "573file/prop.h"
#include "573file/tex.h"

#include "picture/picture.h"

#include "util/crypto.h"
#include "util/hex.h"
#include "util/iobuf.h"
#include "util/list.h"
#include "util/log.h"
#include "util/macro.h"
#include "util/str.h"

static const char tex_list_prop_name[] = "texturelist";
static const char tex_texture_prop_name[] = "texture";
static const char tex_texture_size_prop_name[] = "size";
static const char tex_image_prop_name[] = "image";

static void tex_texture_free(struct tex_texture *tx);
static int tex_texture_read(struct tex_texture **out, const struct prop *p);
static int tex_texture_read_size(struct dim *dest, const struct prop *p);
static void tex_image_free(struct tex_image *ti);
static int tex_image_read(struct tex_image **out, const struct prop *p);
static int tex_image_read_rect(struct rect *rect, const struct prop *p,
                               const char *child_name);

int tex_list_read(struct tex_list **out, const struct prop *p) {
  struct tex_list *tl;
  struct tex_texture *tx;
  const struct prop *child;
  const char *p_name;
  const char *a_compress;
  int r;

  assert(out != NULL);
  assert(p != NULL);

  *out = NULL;
  tl = NULL;

  p_name = prop_get_name(p);

  if (!str_eq(p_name, tex_list_prop_name)) {
    r = -EBADMSG;
    log_write("Expected prop name \"%s\", got \"%s\"", tex_list_prop_name,
              p_name);

    goto end;
  }

  a_compress = prop_get_attr(p, "compress");

  if (!str_eq(a_compress, "avslz")) {
    r = -ENOTSUP;
    log_write("Unknown compression format \"%s\"", a_compress);

    goto end;
  }

  tl = calloc(1, sizeof(*tl));

  if (tl == NULL) {
    r = -ENOMEM;

    goto end;
  }

  for (child = prop_get_first_child_const(p); child != NULL;
       child = prop_get_next_sibling_const(child)) {
    r = tex_texture_read(&tx, child);

    if (r < 0) {
      goto end;
    }

    list_append(&tl->textures, &tx->node);
  }

  *out = tl;
  tl = NULL;
  r = 0;

end:
  tex_list_free(tl);

  return r;
}

void tex_list_free(struct tex_list *tl) {
  struct list_node *next;
  struct list_node *pos;
  struct tex_texture *tx;

  if (tl == NULL) {
    return;
  }

  for (pos = tl->textures.head; pos != NULL; pos = next) {
    next = pos->next;
    tx = containerof(pos, struct tex_texture, node);

    tex_texture_free(tx);
  }

  free(tl);
}

const struct tex_texture *
tex_list_get_first_texture(const struct tex_list *tl) {
  struct list_node *head;

  assert(tl != NULL);

  head = tl->textures.head;

  if (head == NULL) {
    return NULL;
  }

  return containerof(head, struct tex_texture, node);
}

static int tex_texture_read(struct tex_texture **out, const struct prop *p) {
  struct tex_texture *tx;
  struct tex_image *ti;
  const struct prop *child;
  const char *p_name;
  const char *a_format;
  bool got_size;
  int r;

  assert(out != NULL);
  assert(p != NULL);

  *out = NULL;
  tx = NULL;
  got_size = false;

  p_name = prop_get_name(p);

  if (!str_eq(p_name, tex_texture_prop_name)) {
    r = -EBADMSG;
    log_write("Expected prop name \"%s\", got \"%s\"", tex_texture_prop_name,
              p_name);

    goto end;
  }

  tx = calloc(1, sizeof(*tx));

  if (tx == NULL) {
    r = -ENOMEM;

    goto end;
  }

  a_format = prop_get_attr(p, "format");

  if (a_format == NULL) {
    r = -EBADMSG;
    log_write("No format attr on texture");

    goto end;
  }

  r = str_dup(&tx->format, a_format);

  if (r < 0) {
    goto end;
  }

  for (child = prop_get_first_child_const(p); child != NULL;
       child = prop_get_next_sibling_const(child)) {
    if (str_eq(prop_get_name(child), tex_texture_size_prop_name)) {
      r = tex_texture_read_size(&tx->size, child);

      if (r < 0) {
        goto end;
      }

      got_size = true;
    } else {
      r = tex_image_read(&ti, child);

      if (r < 0) {
        goto end;
      }

      list_append(&tx->images, &ti->node);
    }
  }

  if (!got_size) {
    r = -EBADMSG;
    log_write("Texture has no \"%s\" node", tex_texture_size_prop_name);

    goto end;
  }

  *out = tx;
  tx = NULL;
  r = 0;

end:
  tex_texture_free(tx);

  return r;
}

static int tex_texture_read_size(struct dim *dest, const struct prop *p) {
  enum prop_type type;
  struct const_iobuf src;
  uint16_t u16;
  int r;

  assert(dest != NULL);
  assert(p != NULL);

  memset(dest, 0, sizeof(*dest));

  type = prop_get_type(p);

  if (type != PROP_2U16) {
    log_write("Expected \"%s\" node type 2u16, got type %x",
              tex_texture_size_prop_name, type);

    return -EBADMSG;
  }

  prop_borrow_value(p, &src);

  r = iobuf_read_be16(&src, &u16);
  assert(r >= 0);
  dest->width = u16;

  r = iobuf_read_be16(&src, &u16);
  assert(r >= 0);
  dest->height = u16;

  return 0;
}

static void tex_texture_free(struct tex_texture *tx) {
  struct list_node *pos;
  struct list_node *next;
  struct tex_image *ti;

  if (tx == NULL) {
    return;
  }

  free(tx->format);

  for (pos = tx->images.head; pos != NULL; pos = next) {
    next = pos->next;
    ti = containerof(pos, struct tex_image, node);

    tex_image_free(ti);
  }

  free(tx);
}

const struct tex_texture *
tex_texture_get_next_sibling(const struct tex_texture *tx) {
  struct list_node *next;

  assert(tx != NULL);

  next = tx->node.next;

  if (next == NULL) {
    return NULL;
  }

  return containerof(next, struct tex_texture, node);
}

const struct tex_image *
tex_texture_get_first_image(const struct tex_texture *tx) {
  struct list_node *head;

  assert(tx != NULL);

  head = tx->images.head;

  if (head == NULL) {
    return NULL;
  }

  return containerof(head, struct tex_image, node);
}

static int tex_image_read(struct tex_image **out, const struct prop *p) {
  struct tex_image *ti;
  struct md5_hash md5;
  const char *a_name;
  const char *p_name;
  int r;

  assert(out != NULL);
  assert(p != NULL);

  *out = NULL;
  ti = NULL;

  p_name = prop_get_name(p);

  if (!str_eq(p_name, tex_image_prop_name)) {
    r = -EBADMSG;
    log_write("Expected prop name \"%s\", got \"%s\"", tex_image_prop_name,
              p_name);
  }

  ti = calloc(1, sizeof(*ti));

  if (ti == NULL) {
    r = -ENOMEM;

    goto end;
  }

  a_name = prop_get_attr(p, "name");

  if (a_name == NULL) {
    r = -EBADMSG;
    log_write("Prop has no \"name\" attr");

    goto end;
  }

  r = str_dup(&ti->name, a_name);

  if (r < 0) {
    goto end;
  }

  r = md5_compute(&md5, a_name, strlen(a_name));

  if (r < 0) {
    goto end;
  }

  r = hex_encode_lc(&ti->name_md5, md5.b, sizeof(md5.b));

  if (r < 0) {
    goto end;
  }

  r = tex_image_read_rect(&ti->uvrect, p, "uvrect");

  if (r < 0) {
    goto end;
  }

  r = tex_image_read_rect(&ti->imgrect, p, "imgrect");

  if (r < 0) {
    goto end;
  }

  *out = ti;
  ti = NULL;
  r = 0;

end:
  tex_image_free(ti);

  return r;
}

static int tex_image_read_rect(struct rect *rect, const struct prop *p,
                               const char *child_name) {
  const struct prop *child;
  struct const_iobuf buf;
  enum prop_type type;
  uint16_t u16;
  int r;

  assert(rect != NULL);
  assert(p != NULL);
  assert(child_name != NULL);

  memset(rect, 0, sizeof(*rect));
  child = prop_search_child_const(p, child_name);

  if (child == NULL) {
    log_write("Child \"%s\" not found", child_name);

    return -EBADMSG;
  }

  type = prop_get_type(child);

  if (type != PROP_4U16) {
    log_write("\"%s\": Expected PROP_4U16, got %#x", child_name, type);

    return -EBADMSG;
  }

  prop_borrow_value(child, &buf);

  r = iobuf_read_be16(&buf, &u16);
  assert(r >= 0);
  rect->p1.x = u16;

  r = iobuf_read_be16(&buf, &u16);
  assert(r >= 0);
  rect->p2.x = u16;

  r = iobuf_read_be16(&buf, &u16);
  assert(r >= 0);
  rect->p1.y = u16;

  r = iobuf_read_be16(&buf, &u16);
  assert(r >= 0);
  rect->p2.y = u16;

  if (rect->p1.x > rect->p2.x) {
    log_write("\"%s\": p1.x > p2.x", child_name);

    return -EBADMSG;
  }

  if (rect->p1.y > rect->p2.y) {
    log_write("\"%s\": p1.y > p2.y", child_name);

    return -EBADMSG;
  }

  return 0;
}

static void tex_image_free(struct tex_image *ti) {
  if (ti == NULL) {
    return;
  }

  free(ti->name);
  free(ti->name_md5);
  free(ti);
}

const struct tex_image *tex_image_get_next_sibling(const struct tex_image *ti) {
  struct list_node *next;

  assert(ti != NULL);

  next = ti->node.next;

  if (next == NULL) {
    return NULL;
  }

  return containerof(next, struct tex_image, node);
}

int tex_image_read_pixels(const struct tex_image *ti,
                          struct const_iobuf *lz_pixels, struct picture **out) {
  struct picture *p;
  struct dim dim;
  void *pixels;
  size_t nbytes;
  size_t expected_nbytes;
  int r;

  assert(ti != NULL);
  assert(lz_pixels != NULL);
  assert(out != NULL);

  *out = NULL;
  pixels = NULL;
  p = NULL;

  r = lz_file_read(lz_pixels, &pixels, &nbytes);

  if (r < 0) {
    goto end;
  }

  assert(ti->imgrect.p1.x <= ti->imgrect.p2.x);
  assert(ti->imgrect.p1.y <= ti->imgrect.p2.y);

  /* Texture dimensions have to be divided by 2 for some reason */
  dim.width = (ti->imgrect.p2.x - ti->imgrect.p1.x) / 2;
  dim.height = (ti->imgrect.p2.y - ti->imgrect.p1.y) / 2;

  expected_nbytes = dim.width * dim.height * sizeof(pixel_t);

  if (expected_nbytes != nbytes) {
    r = -EBADMSG;
    log_write("Expected %#x pixel bytes, decompressed to %#x",
              (unsigned int)expected_nbytes, (unsigned int)nbytes);

    goto end;
  }

  r = picture_alloc(&p, &dim);

  if (r < 0) {
    goto end;
  }

  memcpy(p->pixels, pixels, nbytes);

  r = 0;
  *out = p;
  p = NULL;

end:
  free(p);
  free(pixels);

  return r;
}
