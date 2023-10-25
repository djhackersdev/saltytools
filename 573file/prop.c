#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "573file/prop-type.h"
#include "573file/prop.h"

#include "util/list.h"
#include "util/log.h"
#include "util/macro.h"
#include "util/str.h"

struct attr {
  struct list_node node;
  char *key;
  char *val;
};

struct prop {
  struct list_node node;
  struct prop *parent;
  struct list attrs;
  struct list children;
  char *name;
  uint32_t nbytes;
  enum prop_type type;
  uint8_t bytes[];
};

static int attr_set(struct attr *a, const char *val);

static int prop_validate(const char *name, enum prop_type type,
                         const void *bytes, uint32_t nbytes);

static int prop_validate(const char *name, enum prop_type type,
                         const void *bytes, uint32_t nbytes) {
  const char *chars;
  int expected_nbytes;

  if (type & PROP_ARRAY_FLAG) {
    log_write("\"%s\": Arrays are not currently supported", name);

    return -ENOTSUP;
  }

  if (!prop_type_is_valid(type)) {
    log_write("\"%s\": Unsupported type code %#x", name, type);

    return -ENOTSUP;
  }

  expected_nbytes = prop_type_to_size(type);

  if (expected_nbytes >= 0 && nbytes != expected_nbytes) {
    log_write("\"%s\": Incorrect size %#x for type %s (expected %#x)", name,
              nbytes, prop_type_to_string(type), expected_nbytes);

    return -EINVAL;
  }

  if (type == PROP_STR) {
    if (nbytes == 0) {
      log_write("\"%s\": String node has zero length", name);

      return -EBADMSG;
    }

    chars = bytes;

    if (chars[nbytes - 1] != '\0') {
      log_write("\"%s\": String node is not NUL terminated", name);

      return -EBADMSG;
    }
  }

  return 0;
}

int prop_alloc(struct prop **out, const char *name, enum prop_type type,
               const void *bytes, uint32_t nbytes) {
  struct prop *p;
  int r;

  assert(out != NULL);
  assert(name != NULL);
  assert(bytes != NULL || nbytes == 0);

  *out = NULL;
  p = NULL;

  r = prop_validate(name, type, bytes, nbytes);

  if (r < 0) {
    goto end;
  }

  p = calloc(1, sizeof(*p) + nbytes);

  if (p == NULL) {
    r = -ENOMEM;

    goto end;
  }

  p->name = strdup(name);

  if (p->name == NULL) {
    r = -ENOMEM;

    goto end;
  }

  p->type = type;
  p->nbytes = nbytes;

  memcpy(p->bytes, bytes, nbytes);

  *out = p;
  p = NULL;
  r = 0;

end:
  prop_free(p);

  return r;
}

void prop_free(struct prop *p) {
  struct list_node *pos;
  struct list_node *next;
  struct attr *attr;
  struct prop *child;

  if (p == NULL) {
    return;
  }

  for (pos = p->children.head; pos != NULL; pos = next) {
    next = pos->next;
    child = containerof(pos, struct prop, node);

    prop_free(child);
  }

  for (pos = p->attrs.head; pos != NULL; pos = next) {
    next = pos->next;
    attr = containerof(pos, struct attr, node);

    free(attr->key);
    free(attr->val);
    free(attr);
  }

  free(p->name);
  free(p);
}

void prop_append(struct prop *p, struct prop *child) {
  assert(p != NULL);
  assert(child != NULL);
  assert(child->parent == NULL);

  list_append(&p->children, &child->node);
  child->parent = p;
}

void prop_borrow_value(const struct prop *p, struct const_iobuf *out) {
  assert(p != NULL);
  assert(out != NULL);

  out->bytes = p->bytes;
  out->nbytes = p->nbytes;
  out->pos = 0;
}

const char *prop_get_attr(const struct prop *p, const char *key) {
  struct list_node *pos;
  struct attr *a;

  assert(p != NULL);
  assert(key != NULL);

  for (pos = p->attrs.head; pos != NULL; pos = pos->next) {
    a = containerof(pos, struct attr, node);

    if (str_eq(key, a->key)) {
      return a->val;
    }
  }

  return NULL;
}

const struct attr *prop_get_first_attr(const struct prop *p) {
  struct list_node *attr;

  assert(p != NULL);

  attr = p->attrs.head;

  if (attr == NULL) {
    return NULL;
  }

  return containerof(attr, struct attr, node);
}

struct prop *prop_get_first_child(struct prop *p) {
  struct list_node *child;

  assert(p != NULL);

  child = p->children.head;

  if (child == NULL) {
    return NULL;
  }

  return containerof(child, struct prop, node);
}

const struct prop *prop_get_first_child_const(const struct prop *p) {
  const struct list_node *child;

  assert(p != NULL);

  child = p->children.head;

  if (child == NULL) {
    return NULL;
  }

  return containerof(child, struct prop, node);
}

const char *prop_get_name(const struct prop *p) {
  assert(p != NULL);

  return p->name;
}

struct prop *prop_get_next_sibling(struct prop *p) {
  assert(p != NULL);

  return containerof(p->node.next, struct prop, node);
}

const struct prop *prop_get_next_sibling_const(const struct prop *p) {
  assert(p != NULL);

  return containerof(p->node.next, struct prop, node);
}

struct prop *prop_get_parent(struct prop *p) {
  assert(p != NULL);

  return p->parent;
}

enum prop_type prop_get_type(const struct prop *p) {
  assert(p != NULL);

  return p->type;
}

const char *prop_get_value_str(const struct prop *p) {
  assert(p != NULL);
  assert(p->type == PROP_STR);

  return (const char *)p->bytes;
}

struct prop *prop_search_child(struct prop *p, const char *name) {
  struct list_node *pos;
  struct prop *child;

  assert(p != NULL);
  assert(name != NULL);

  for (pos = p->children.head; pos != NULL; pos = pos->next) {
    child = containerof(pos, struct prop, node);

    if (str_eq(child->name, name)) {
      return child;
    }
  }

  return NULL;
}

int prop_set_attr(struct prop *p, const char *key, const char *val) {
  struct list_node *pos;
  struct attr *a;
  int r;

  assert(p != NULL);
  assert(key != NULL);
  assert(val != NULL);

  for (pos = p->attrs.head; pos != NULL; pos = pos->next) {
    a = containerof(pos, struct attr, node);

    if (str_eq(key, a->key) == 0) {
      return attr_set(a, val);
    }
  }

  a = calloc(1, sizeof(*a));

  if (a == NULL) {
    r = -ENOMEM;

    goto end;
  }

  a->key = strdup(key);

  if (a->key == NULL) {
    r = -ENOMEM;

    goto end;
  }

  a->val = strdup(val);

  if (a->val == NULL) {
    r = -ENOMEM;

    goto end;
  }

  list_append(&p->attrs, &a->node);

  a = NULL;
  r = 0;

end:
  free(a);

  return r;
}

const struct attr *attr_get_next_sibling(const struct attr *a) {
  const struct list_node *next;

  assert(a != NULL);

  next = a->node.next;

  if (next == NULL) {
    return NULL;
  }

  return containerof(next, struct attr, node);
}

const char *attr_get_key(const struct attr *a) {
  assert(a != NULL);

  return a->key;
}

const char *attr_get_val(const struct attr *a) {
  assert(a != NULL);

  return a->val;
}

static int attr_set(struct attr *a, const char *val) {
  char *replace;

  replace = strdup(val);

  if (replace == NULL) {
    return -ENOMEM;
  }

  free(a->val);
  a->val = replace;

  return 0;
}
