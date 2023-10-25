#pragma once

#include "util/macro.h"

#define log_write(...) log_write_(__func__, __VA_ARGS__)
#define log_error(r) log_error_(__func__, __FILE__, __LINE__, r);

void log_write_(const char *func, const char *fmt, ...)
    gcc_attribute((format(printf, 2, 3)));
void log_error_(const char *func, const char *file, int line, int r);
