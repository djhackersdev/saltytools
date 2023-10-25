#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "util/log.h"

static void log_vwrite_(const char *func, const char *fmt, va_list ap);

void log_write_(const char *func, const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  log_vwrite_(func, fmt, ap);
  va_end(ap);
}

static void log_vwrite_(const char *func, const char *fmt, va_list ap) {
  printf("%s: ", func);
  vprintf(fmt, ap);
  puts("");
}

void log_error_(const char *func, const char *file, int line, int r) {
  log_write_(func, "%s:%i: %s (%i)", file, line, strerror(-r), r);
}
