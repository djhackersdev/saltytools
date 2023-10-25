#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "util/str.h"

bool str_eq(const char *lhs, const char *rhs) {
  if (lhs == NULL || rhs == NULL) {
    return lhs == rhs;
  }

  return strcmp(lhs, rhs) == 0;
}

int str_printf(char **out, const char *fmt, ...) {
  va_list ap;
  int r;

  assert(out != NULL);
  assert(fmt != NULL);

  va_start(ap, fmt);
  *out = NULL;
  r = str_vprintf(out, fmt, ap);
  va_end(ap);

  return r;
}

int str_vprintf(char **out, const char *fmt, va_list ap) {
  va_list ap2;
  char *chars;
  int len;
  int len2;

  *out = NULL;

  va_copy(ap2, ap);
  len = vsnprintf(NULL, 0, fmt, ap2);
  va_end(ap2);

  chars = malloc(len + 1);

  if (chars == NULL) {
    return -ENOMEM;
  }

  va_copy(ap2, ap);
  len2 = vsnprintf(chars, len + 1, fmt, ap2);
  va_end(ap2);

  assert(len2 == len);

  *out = chars;

  return 0;
}

void strbuf_printf(struct strbuf *dest, const char *fmt, ...) {
  va_list ap;

  assert(dest != NULL);
  assert(fmt != NULL);

  va_start(ap, fmt);
  strbuf_vprintf(dest, fmt, ap);
  va_end(ap);
}

void strbuf_vprintf(struct strbuf *dest, const char *fmt, va_list ap) {
  size_t avail;
  char *chars;
  int result;

  assert(dest != NULL);
  assert(dest->chars == NULL || dest->pos < dest->nchars);
  assert(fmt != NULL);

  if (dest->chars == NULL) {
    avail = 0;
    chars = NULL;
  } else {
    avail = dest->nchars - dest->pos;
    chars = dest->chars + dest->pos;
  }

  result = vsnprintf(chars, avail, fmt, ap);

  assert(dest->chars == NULL || result < avail);

  dest->pos += result;
}

void strbuf_putc(struct strbuf *dest, char c) {
  assert(dest != NULL);

  if (dest->chars != NULL) {
    assert(dest->pos + 1 < dest->nchars);

    dest->chars[dest->pos + 0] = c;
    dest->chars[dest->pos + 1] = '\0';
  }

  dest->pos++;
}

void strbuf_puts(struct strbuf *dest, const char *str) {
  size_t len;

  assert(dest != NULL);
  assert(str != NULL);

  len = strlen(str);

  if (dest->chars != NULL) {
    assert(dest->pos + len < dest->nchars);

    memcpy(dest->chars + dest->pos, str, len + 1);
  }

  dest->pos += len;
}
