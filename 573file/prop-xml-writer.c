#include <assert.h>

#include "573file/prop-xml-writer.h"
#include "573file/prop.h"

#include "util/iobuf.h"
#include "util/log.h"
#include "util/str.h"

enum prop_xml_escape {
  PROP_XML_ESCAPE_ATTR,
  PROP_XML_ESCAPE_TEXT,
};

static void prop_xml_write_attr_list(struct strbuf *dest, const struct prop *p);
static void prop_xml_write_children(struct strbuf *dest, const struct prop *p,
                                    unsigned int indent);
static void prop_xml_write_escaped_string(struct strbuf *dest, const char *str,
                                          enum prop_xml_escape ctx);
static void prop_xml_write_indent(struct strbuf *dest, unsigned int indent);
static void prop_xml_write_node(struct strbuf *dest, const struct prop *p,
                                unsigned int indent);
static void prop_xml_write_nonvoid(struct strbuf *dest, const struct prop *p,
                                   unsigned int indent);
static void prop_xml_write_void(struct strbuf *dest, const struct prop *p,
                                unsigned int indent);
static void prop_xml_write_text(struct strbuf *dest, const struct prop *p,
                                enum prop_xml_escape ctx);
static void prop_xml_write_text_s8(struct strbuf *dest,
                                   struct const_iobuf *src);
static void prop_xml_write_text_s16(struct strbuf *dest,
                                    struct const_iobuf *src);
static void prop_xml_write_text_s32(struct strbuf *dest,
                                    struct const_iobuf *src);
static void prop_xml_write_text_s64(struct strbuf *dest,
                                    struct const_iobuf *src);
static void prop_xml_write_text_u8(struct strbuf *dest,
                                   struct const_iobuf *src);
static void prop_xml_write_text_u16(struct strbuf *dest,
                                    struct const_iobuf *src);
static void prop_xml_write_text_u32(struct strbuf *dest,
                                    struct const_iobuf *src);
static void prop_xml_write_text_u64(struct strbuf *dest,
                                    struct const_iobuf *src);
static void prop_xml_write_text_bin(struct strbuf *dest,
                                    struct const_iobuf *src);
static void prop_xml_write_text_str(struct strbuf *dest,
                                    struct const_iobuf *src,
                                    enum prop_xml_escape ctx);
static void prop_xml_write_text_ip4(struct strbuf *dest,
                                    struct const_iobuf *src);
static void prop_xml_write_text_bool(struct strbuf *dest,
                                     struct const_iobuf *src);
static void prop_xml_write_text_s32_tuple(struct strbuf *dest,
                                          struct const_iobuf *src,
                                          size_t count);
static void prop_xml_write_text_u16_tuple(struct strbuf *dest,
                                          struct const_iobuf *src,
                                          size_t count);

int prop_xml_write(const struct prop *p, char **out) {
  struct strbuf buf;
  char *chars;

  assert(p != NULL);
  assert(out != NULL);

  *out = NULL;

  buf.chars = NULL;
  buf.nchars = 0;
  buf.pos = 0;

  prop_xml_write_node(&buf, p, 0);
  chars = malloc(buf.pos + 1);

  if (chars == NULL) {
    return -ENOMEM;
  }

  buf.chars = chars;
  buf.nchars = buf.pos + 1;
  buf.pos = 0;

  prop_xml_write_node(&buf, p, 0);

  assert(buf.pos + 1 == buf.nchars);

  *out = chars;

  return 0;
}

static void prop_xml_write_node(struct strbuf *dest, const struct prop *p,
                                unsigned int indent) {
  enum prop_type type;

  assert(dest != NULL);
  assert(p != NULL);

  type = prop_get_type(p);

  if (type == PROP_VOID) {
    prop_xml_write_void(dest, p, indent);
  } else {
    prop_xml_write_nonvoid(dest, p, indent);
  }
}

static void prop_xml_write_void(struct strbuf *dest, const struct prop *p,
                                unsigned int indent) {
  const struct prop *first_child;
  const char *name;

  assert(dest != NULL);
  assert(p != NULL);

  name = prop_get_name(p);

  prop_xml_write_indent(dest, indent);
  strbuf_printf(dest, "<%s", name);
  prop_xml_write_attr_list(dest, p);

  first_child = prop_get_first_child_const(p);

  if (first_child != NULL) {
    strbuf_puts(dest, ">\n");
    prop_xml_write_children(dest, p, indent + 1);
    prop_xml_write_indent(dest, indent);
    strbuf_printf(dest, "</%s>\n", name);
  } else {
    strbuf_puts(dest, "/>\n");
  }
}

/* AVS doesn't understand this __value attribute here, this is my own
   invention, but generally we don't see any property pages with mixed content
   nodes being stored in XML format to begin with. */

static void prop_xml_write_nonvoid(struct strbuf *dest, const struct prop *p,
                                   unsigned int indent) {
  const struct prop *first_child;
  const char *name;
  const char *type_str;
  enum prop_type type;

  assert(dest != NULL);
  assert(p != NULL);

  name = prop_get_name(p);
  type = prop_get_type(p);
  type_str = prop_type_to_string(type);

  prop_xml_write_indent(dest, indent);
  strbuf_printf(dest, "<%s __type=\"%s\"", name, type_str);

  first_child = prop_get_first_child_const(p);

  if (first_child != NULL) {
    strbuf_puts(dest, " __value=\"");
    prop_xml_write_text(dest, p, PROP_XML_ESCAPE_ATTR);
    strbuf_putc(dest, '"');

    prop_xml_write_attr_list(dest, p);
    strbuf_puts(dest, ">\n");

    prop_xml_write_children(dest, p, indent + 1);

    prop_xml_write_indent(dest, indent);
    strbuf_printf(dest, "</%s>\n", name);
  } else {
    prop_xml_write_attr_list(dest, p);
    strbuf_putc(dest, '>');

    prop_xml_write_text(dest, p, PROP_XML_ESCAPE_TEXT);

    strbuf_printf(dest, "</%s>\n", name);
  }
}

static void prop_xml_write_children(struct strbuf *dest, const struct prop *p,
                                    unsigned int indent) {
  const struct prop *child;

  assert(dest != NULL);
  assert(p != NULL);

  for (child = prop_get_first_child_const(p); child != NULL;
       child = prop_get_next_sibling_const(child)) {
    prop_xml_write_node(dest, child, indent);
  }
}

static void prop_xml_write_attr_list(struct strbuf *dest,
                                     const struct prop *p) {
  const struct attr *a;
  const char *attr_key;
  const char *attr_val;

  assert(dest != NULL);
  assert(p != NULL);

  for (a = prop_get_first_attr(p); a != NULL; a = attr_get_next_sibling(a)) {
    attr_key = attr_get_key(a);
    attr_val = attr_get_val(a);

    strbuf_printf(dest, " %s=\"", attr_key);
    prop_xml_write_escaped_string(dest, attr_val, PROP_XML_ESCAPE_ATTR);
    strbuf_putc(dest, '"');
  }
}

static void prop_xml_write_escaped_string(struct strbuf *dest, const char *str,
                                          enum prop_xml_escape ctx) {
  const char *pos;
  char c;

  assert(dest != NULL);
  assert(str != NULL);

  for (pos = str; *pos != '\0'; pos++) {
    c = *pos;

    switch (c) {
    case '<':
      strbuf_puts(dest, "&lt;");

      break;

    case '>':
      strbuf_puts(dest, "&gt;");

      break;

    case '&':
      strbuf_puts(dest, "&amp;");

      break;

    case '\'':
      if (ctx == PROP_XML_ESCAPE_ATTR) {
        strbuf_puts(dest, "&apos;");
      } else {
        strbuf_putc(dest, c);
      }

      break;

    case '"':
      if (ctx == PROP_XML_ESCAPE_ATTR) {
        strbuf_puts(dest, "&quot;");
      } else {
        strbuf_putc(dest, c);
      }

      break;

    default:
      strbuf_putc(dest, c);

      break;
    }
  }
}

static void prop_xml_write_indent(struct strbuf *dest, unsigned int indent) {
  unsigned int i;

  assert(dest != NULL);

  for (i = 0; i < indent; i++) {
    strbuf_puts(dest, "  ");
  }
}

static void prop_xml_write_text(struct strbuf *dest, const struct prop *p,
                                enum prop_xml_escape ctx) {
  struct const_iobuf src;
  enum prop_type type;

  type = prop_get_type(p);
  prop_borrow_value(p, &src);

  switch (type) {
  case PROP_S8:
    prop_xml_write_text_s8(dest, &src);

    break;

  case PROP_S16:
    prop_xml_write_text_s16(dest, &src);

    break;

  case PROP_S32:
    prop_xml_write_text_s32(dest, &src);

    break;

  case PROP_S64:
    prop_xml_write_text_s64(dest, &src);

    break;

  case PROP_U8:
    prop_xml_write_text_u8(dest, &src);

    break;

  case PROP_U16:
    prop_xml_write_text_u16(dest, &src);

    break;

  case PROP_U32:
    prop_xml_write_text_u32(dest, &src);

    break;

  case PROP_U64:
    prop_xml_write_text_u64(dest, &src);

    break;

  case PROP_BIN:
    prop_xml_write_text_bin(dest, &src);

    break;

  case PROP_STR:
    prop_xml_write_text_str(dest, &src, ctx);

    break;

  case PROP_IP4:
    prop_xml_write_text_ip4(dest, &src);

    break;

  case PROP_TIME:
    /* Reuse the u32 writer for timestamps */
    prop_xml_write_text_u32(dest, &src);

    break;

  case PROP_2U16:
    prop_xml_write_text_u16_tuple(dest, &src, 2);

    break;

  case PROP_3S32:
    prop_xml_write_text_s32_tuple(dest, &src, 3);

    break;

  case PROP_4U16:
    prop_xml_write_text_u16_tuple(dest, &src, 4);

    break;

  case PROP_BOOL:
    prop_xml_write_text_bool(dest, &src);

    break;

  default:
    log_write("Unsupported type: %#x", type);

    abort();
  }
}

static void prop_xml_write_text_s8(struct strbuf *dest,
                                   struct const_iobuf *src) {
  uint8_t value;
  int r;

  assert(dest != NULL);
  assert(src != NULL);

  r = iobuf_read_8(src, &value);

  assert(r >= 0);

  strbuf_printf(dest, "%i", (int8_t)value);
}

static void prop_xml_write_text_s16(struct strbuf *dest,
                                    struct const_iobuf *src) {
  uint16_t value;
  int r;

  assert(dest != NULL);
  assert(src != NULL);

  r = iobuf_read_be16(src, &value);

  assert(r >= 0);

  strbuf_printf(dest, "%i", (int16_t)value);
}

static void prop_xml_write_text_s32(struct strbuf *dest,
                                    struct const_iobuf *src) {
  uint32_t value;
  int r;

  assert(dest != NULL);
  assert(src != NULL);

  r = iobuf_read_be32(src, &value);

  assert(r >= 0);

  strbuf_printf(dest, "%i", (int32_t)value);
}

static void prop_xml_write_text_s64(struct strbuf *dest,
                                    struct const_iobuf *src) {
  uint64_t value;
  int r;

  assert(dest != NULL);
  assert(src != NULL);

  r = iobuf_read_be64(src, &value);

  assert(r >= 0);

  strbuf_printf(dest, "%li", (int64_t)value);
}

static void prop_xml_write_text_u8(struct strbuf *dest,
                                   struct const_iobuf *src) {
  uint8_t value;
  int r;

  assert(dest != NULL);
  assert(src != NULL);

  r = iobuf_read_8(src, &value);

  assert(r >= 0);

  strbuf_printf(dest, "%i", value);
}

static void prop_xml_write_text_u16(struct strbuf *dest,
                                    struct const_iobuf *src) {
  uint16_t value;
  int r;

  assert(dest != NULL);
  assert(src != NULL);

  r = iobuf_read_be16(src, &value);

  assert(r >= 0);

  strbuf_printf(dest, "%u", value);
}

static void prop_xml_write_text_u32(struct strbuf *dest,
                                    struct const_iobuf *src) {
  uint32_t value;
  int r;

  assert(dest != NULL);
  assert(src != NULL);

  r = iobuf_read_be32(src, &value);

  assert(r >= 0);

  strbuf_printf(dest, "%u", value);
}

static void prop_xml_write_text_u64(struct strbuf *dest,
                                    struct const_iobuf *src) {
  uint64_t value;
  int r;

  assert(dest != NULL);
  assert(src != NULL);

  r = iobuf_read_be64(src, &value);

  assert(r >= 0);

  strbuf_printf(dest, "%lu", value);
}

static void prop_xml_write_text_bin(struct strbuf *dest,
                                    struct const_iobuf *src) {
  uint8_t byte;
  uint8_t nibble;
  uint8_t nibbles[2];
  size_t i;
  size_t j;
  char c;

  assert(dest != NULL);
  assert(src != NULL);

  for (i = src->pos; i < src->nbytes; i++) {
    byte = src->bytes[i];
    nibbles[0] = byte >> 4;
    nibbles[1] = byte & 0x0f;

    for (j = 0; j < 2; j++) {
      nibble = nibbles[j];

      if (nibble < 10) {
        c = '0' + nibble;
      } else {
        c = 'a' + (nibble - 10);
      }

      strbuf_putc(dest, c);
    }
  }

  src->pos = i;
}

static void prop_xml_write_text_str(struct strbuf *dest,
                                    struct const_iobuf *src,
                                    enum prop_xml_escape ctx) {
  const char *chars;
  size_t nchars;

  assert(dest != NULL);
  assert(src != NULL);
  assert(src->pos <= src->nbytes);

  chars = (const char *)&src->bytes[src->pos];
  nchars = src->nbytes - src->pos;

  assert(chars[nchars - 1] == '\0');

  prop_xml_write_escaped_string(dest, chars, ctx);
  src->pos += nchars;
}

static void prop_xml_write_text_ip4(struct strbuf *dest,
                                    struct const_iobuf *src) {
  const uint8_t *b;

  assert(dest != NULL);
  assert(src != NULL);
  assert(src->pos + 4 <= src->nbytes);

  b = src->bytes + src->pos;
  strbuf_printf(dest, "%i.%i.%i.%i", b[0], b[1], b[2], b[3]);
}

static void prop_xml_write_text_bool(struct strbuf *dest,
                                     struct const_iobuf *src) {
  assert(dest != NULL);
  assert(src != NULL);
  assert(src->pos < src->nbytes);

  if (src->bytes[src->pos]) {
    strbuf_putc(dest, '1');
  } else {
    strbuf_putc(dest, '0');
  }
}

/* I don't know how tuples are actually serialized to XML, the real format
   might be different. */

static void prop_xml_write_text_s32_tuple(struct strbuf *dest,
                                          struct const_iobuf *src,
                                          size_t count) {
  uint32_t value;
  size_t i;
  int r;

  assert(dest != NULL);
  assert(src != NULL);

  for (i = 0; i < count; i++) {
    if (i > 0) {
      strbuf_putc(dest, ',');
    }

    r = iobuf_read_be32(src, &value);

    assert(r >= 0);

    strbuf_printf(dest, "%i", (int32_t)value);
  }
}

static void prop_xml_write_text_u16_tuple(struct strbuf *dest,
                                          struct const_iobuf *src,
                                          size_t count) {
  uint16_t value;
  size_t i;
  int r;

  assert(dest != NULL);
  assert(src != NULL);

  for (i = 0; i < count; i++) {
    if (i > 0) {
      strbuf_putc(dest, ',');
    }

    r = iobuf_read_be16(src, &value);

    assert(r >= 0);

    strbuf_printf(dest, "%u", value);
  }
}
