#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>

static int hex_encode(char **out, const void *ptr, size_t nbytes,
                      const char *digits) {
  const uint8_t *bytes;
  char *chars;
  size_t i;

  assert(out != NULL);
  assert(ptr != NULL);
  assert(digits != NULL);

  *out = NULL;
  bytes = ptr;

  chars = malloc(2 * nbytes + 1);

  if (chars == NULL) {
    return -ENOMEM;
  }

  for (i = 0; i < nbytes; i++) {
    chars[i * 2 + 0] = digits[bytes[i] >> 4];
    chars[i * 2 + 1] = digits[bytes[i] & 15];
  }

  chars[nbytes * 2] = '\0';
  *out = chars;

  return 0;
}

int hex_encode_lc(char **out, const void *bytes, size_t nbytes) {
  assert(out != NULL);
  assert(bytes != NULL);

  return hex_encode(out, bytes, nbytes, "0123456789abcdef");
}

int hex_encode_uc(char **out, const void *bytes, size_t nbytes) {
  assert(out != NULL);
  assert(bytes != NULL);

  return hex_encode(out, bytes, nbytes, "0123456789ABCDEF");
}
