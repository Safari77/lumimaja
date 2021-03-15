/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/**
 * \file Linux-specific implementation of rand.h
 */
#include "../rand.h"
#include "../os/logit.h"

#include <sys/random.h>
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#include <string.h>
#include <errno.h>

using namespace std;

void pws_os::DeleteInstance(void)
{
}

bool pws_os::GetRandomData(void *buf, unsigned long len)
{
  ssize_t ret;
  uint8_t *p = static_cast<uint8_t *>(buf);

  if (!len) return true;

  while (len > 0) {
    do {
      errno = 0;
      ret = getrandom(p, len, 0);
    } while ((ret == -1) && (errno == EINTR));
    if (ret <= 0) {
      fprintf(stderr, "getrandom failed: %s\n", strerror(errno));
      exit(1);
    }
    p += ret;
    len -= ret;
  }
  return true;
}

