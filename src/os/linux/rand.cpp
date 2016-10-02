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
#include <linux/random.h>
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <errno.h>

using namespace std;

static FILE *frandom;

void pws_os::DeleteInstance(void)
{
  if (frandom) {
    fclose(frandom);
  }
  frandom = NULL;
}

bool pws_os::GetRandomData(void *p, unsigned long len)
{
  if (!len) return true;
  if (len > 1048576) {
    fprintf(stderr, "pws_os::GetRandomData requested %lu bytes p=%p\n", len, p);
    exit(1);
  }
#if defined(SYS_getrandom) && defined(__linux__)
  long ret;
  do {
    ret = syscall(SYS_getrandom, p, len, 0, 0, 0, 0);
  } while ((ret == -1) && (errno == EINTR));
  if (ret == len) return true;
  if ((ret == -1) && (errno != ENOSYS)) {
    fprintf(stderr, "pws_os::GetRandomData getrandom failed: %s\n",
            strerror(errno));
    exit(1);
  }
#else
#  warning getrandom not supported
#endif
  if (frandom == NULL) {
    frandom = fopen("/dev/urandom", "rb");
    if (frandom == NULL) {
		  fprintf(stderr, "pws_os::GetRandomData getrandom not supported and can't "
              "open /dev/urandom: %s\n", strerror(errno));
      exit(1);
    }
    setbuf(frandom, NULL);
  }
  if (fread(p, 1, len, frandom) != len) {
    fprintf(stderr, "pws_os::GetRandomData failed to read /dev/urandom: %s\n",
            strerror(errno));
    exit(1);
  }
  return true;
}

