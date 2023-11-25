#pragma once

#include <stdint.h>

typedef uint32_t pixel_t;

struct dim {
  unsigned int width;
  unsigned int height;
};

struct point {
  unsigned int x;
  unsigned int y;
};

struct rect {
  struct point p1;
  struct point p2;
};

struct picture {
  struct dim dim;
  pixel_t pixels[];
};

int picture_alloc(struct picture **out, const struct dim *dim);
