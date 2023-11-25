#include <assert.h>
#include <errno.h>
#include <string.h>

#include <png.h>

#include "picture/picture-png.h"
#include "picture/picture.h"

#include "util/log.h"

int picture_write_png(const struct picture *pic, const char *path) {
  png_image png;
  int r;

  assert(pic != NULL);
  assert(path != NULL);

  memset(&png, 0, sizeof(png));
  png.version = PNG_IMAGE_VERSION;
  png.width = pic->dim.width;
  png.height = pic->dim.height;
  png.format = PNG_FORMAT_BGRA;

  r = png_image_write_to_file(&png, path, 0, pic->pixels, 0, NULL);

  if (r != 1 || (png.warning_or_error & 2)) {
    log_write("libpng error writing to \"%s\": %s", path, png.message);

    return -EIO;
  }

  return 0;
}
