#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "picture/picture.h"

int picture_alloc(struct picture **out, const struct dim *dim) {
  struct picture *pic;
  size_t nbytes;

  assert(out != NULL);
  assert(dim != NULL);

  *out = NULL;
  nbytes = sizeof(*pic) + dim->width * dim->height * sizeof(uint32_t);

  if (nbytes > UINT_MAX) {
    return -EOVERFLOW;
  }

  pic = malloc(nbytes);

  if (pic == NULL) {
    return -ENOMEM;
  }

  memcpy(&pic->dim, dim, sizeof(*dim));

  *out = pic;

  return 0;
}
