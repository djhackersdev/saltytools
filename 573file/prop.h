#pragma once

#include <stdint.h>

#include "573file/prop-type.h"

#include "util/iobuf.h"

struct attr;
struct prop;

int prop_alloc(struct prop **p, const char *name, enum prop_type type,
               const void *bytes, uint32_t nbytes);
void prop_free(struct prop *p);
void prop_append(struct prop *p, struct prop *child);
void prop_borrow_value(const struct prop *p, struct const_iobuf *out);
const char *prop_get_attr(const struct prop *p, const char *key);
uint32_t prop_get_count(const struct prop *p);
const struct attr *prop_get_first_attr(const struct prop *p);
struct prop *prop_get_first_child(struct prop *p);
const struct prop *prop_get_first_child_const(const struct prop *p);
const char *prop_get_name(const struct prop *p);
struct prop *prop_get_next_sibling(struct prop *p);
const struct prop *prop_get_next_sibling_const(const struct prop *p);
struct prop *prop_get_parent(struct prop *p);
enum prop_type prop_get_type(const struct prop *p);
const char *prop_get_value_str(const struct prop *p);
struct prop *prop_search_child(struct prop *p, const char *name);
int prop_set_attr(struct prop *p, const char *key, const char *val);

const struct attr *attr_get_next_sibling(const struct attr *a);
const char *attr_get_key(const struct attr *a);
const char *attr_get_val(const struct attr *a);
