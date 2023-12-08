#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include "util/macro.h"

struct strbuf {
  char *chars;
  size_t nchars;
  size_t pos;
};

int str_dup(char **dest, const char *src);
bool str_eq(const char *lhs, const char *rhs);
int str_printf(char **out, const char *fmt, ...)
    gcc_attribute((format(printf, 2, 3)));
int str_vprintf(char **out, const char *fmt, va_list ap);

void strbuf_printf(struct strbuf *dest, const char *fmt, ...)
    gcc_attribute((format(printf, 2, 3)));
void strbuf_vprintf(struct strbuf *dest, const char *fmt, va_list ap);
void strbuf_putc(struct strbuf *dest, char c);
void strbuf_puts(struct strbuf *dest, const char *str);
