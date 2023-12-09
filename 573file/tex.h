#pragma once

#include "573file/prop.h"

#include "util/iobuf.h"
#include "util/list.h"

#include "picture/picture.h"

struct tex_list {
  struct list textures;
};

struct tex_texture {
  struct list_node node;
  char *format;
  struct dim size;
  struct list images;
};

struct tex_image {
  struct list_node node;
  char *name;
  char *name_md5;
  struct rect uvrect;
  struct rect imgrect;
};

int tex_list_read(struct tex_list **out, const struct prop *p);
void tex_list_free(struct tex_list *tl);
const struct tex_texture *tex_list_get_first_texture(const struct tex_list *tl);

const struct tex_texture *
tex_texture_get_next_sibling(const struct tex_texture *tx);
const struct tex_image *
tex_texture_get_first_image(const struct tex_texture *tx);

const struct tex_image *tex_image_get_next_sibling(const struct tex_image *ti);
int tex_image_read_pixels(const struct tex_image *tx,
                          struct const_iobuf *lz_pixels, struct picture **out);
