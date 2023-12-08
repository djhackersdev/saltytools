#include <assert.h>
#include <errno.h>
#include <stddef.h>

#include <openssl/evp.h>

#include "util/crypto.h"
#include "util/log.h"

int md5_compute(struct md5_hash *dest, const void *bytes, size_t nbytes) {
  EVP_MD_CTX *ctx;
  const EVP_MD *md5;
  unsigned int len;
  int libr;
  int r;

  assert(dest != NULL);
  assert(bytes != NULL);

  ctx = EVP_MD_CTX_new();

  if (ctx == NULL) {
    r = -ENOMEM;

    goto end;
  }

  md5 = EVP_md5();
  libr = EVP_DigestInit_ex(ctx, md5, NULL);

  if (libr != 1) {
    r = -ENOSYS;
    log_error(r);

    goto end;
  }

  libr = EVP_DigestUpdate(ctx, bytes, nbytes);

  if (libr != 1) {
    r = -ENOSYS;
    log_error(r);

    goto end;
  }

  len = sizeof(dest->b);
  libr = EVP_DigestFinal_ex(ctx, dest->b, &len);

  if (libr != 1 || len != sizeof(dest->b)) {
    r = -ENOSYS;
    log_error(r);

    goto end;
  }

  r = 0;

end:
  EVP_MD_CTX_free(ctx);

  return r;
}
