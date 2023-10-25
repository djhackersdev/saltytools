#include <stdio.h>
#include <stdlib.h>

#include "573file/prop-binary-reader.h"
#include "573file/prop-xml-writer.h"
#include "573file/prop.h"

#include "util/fs.h"
#include "util/iobuf.h"
#include "util/log.h"

int main(int argc, char **argv) {
  const char *infile;
  const char *outfile;
  struct const_iobuf buf;
  struct prop *p;
  void *bytes;
  char *xml;
  size_t nbytes;
  FILE *f;
  FILE *f_dest;
  int r;

  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s [infile] <outfile>\n", argv[0]);

    return EXIT_FAILURE;
  }

  f = NULL;
  bytes = NULL;
  xml = NULL;
  p = NULL;

  infile = argv[1];

  if (argc > 2) {
    outfile = argv[2];
  } else {
    outfile = NULL;
  }

  r = fs_read_file(infile, &bytes, &nbytes);

  if (r < 0) {
    goto end;
  }

  r = prop_binary_parse(&p, bytes, nbytes);

  if (r < 0) {
    goto end;
  }

  r = prop_xml_write(p, &xml);

  if (r < 0) {
    goto end;
  }

  if (outfile != NULL) {
    r = fs_open(&f, outfile, "w");

    if (r < 0) {
      goto end;
    }

    f_dest = f;
  } else {
    f_dest = stdout;
  }

  buf.bytes = (uint8_t *)xml;
  buf.nbytes = strlen(xml);
  buf.pos = 0;

  r = fs_write(f_dest, &buf);

  if (r < 0) {
    goto end;
  }

end:
  fs_close(f);
  free(xml);
  prop_free(p);
  free(bytes);

  if (r < 0) {
    log_write("%s (%i)", strerror(-r), r);

    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
