/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#include <limits.h>
#include "os/rand.h"
#include <inttypes.h>

#include "PwsPlatform.h"
#include "PWSrand.h"

PWSrand *PWSrand::self = NULL;

PWSrand *PWSrand::GetInstance()
{
  if (self == NULL) {
    self = new PWSrand;
  }
  return self;
}

void PWSrand::DeleteInstance()
{
  pws_os::DeleteInstance();
  delete self;
  self = NULL;
}

PWSrand::PWSrand()
{
  fprintf(stderr, "PWSrand::PWSrand %p test %u\n", this, RangeRand(UINT32_MAX));
  fprintf(stderr, "PWSrand::PWSrand64 %p test %" PRIu64 "\n", this, RangeRand64(UINT64_MAX));
}

PWSrand::~PWSrand()
{
}

void PWSrand::GetRandomData( void * const buffer, unsigned long length )
{
  pws_os::GetRandomData(buffer, length);
}

// generate random numbers from a buffer filled in by GetRandomData()
uint32 PWSrand::RandUInt()
{
  uint32 u;

  GetRandomData(&u, sizeof(u));
  return u;
}

stringT PWSrand::RandAZ(const size_t length)
{
  const uint8_t rndch[] = "abcdefghijklmnopqrstuvwxyz0123456789";
  stringT az;
  uint8_t stackaz[length];

  GetRandomData(stackaz, length);
  for (size_t i = 0; i < length; i++)
    az += rndch[stackaz[i] % (sizeof(rndch)-1)];
  return az;
}

uint64 PWSrand::RandUInt64()
{
  uint64 u;

  GetRandomData(&u, sizeof(u));
  return u;
}

/*
*  RangeRand(len)
*
*  Returns a random number in the range 0 to (len-1).
*  For example, RangeRand(256) returns a value from 0 to 255.
*/
uint32 PWSrand::RangeRand(uint32 len)
{
  if (len > 1) {
    uint32 r;
    const uint32 ceil = -len % len;
    for (;;) {
      r = RandUInt();
      if (r >= ceil)
        break;
    }
    return (r % len);
  } else {
    return 0;
  }
}

/*
*  RangeRand64(len)
*  64 bit version
*/
uint64 PWSrand::RangeRand64(uint64 len)
{
  if (len > 1) {
    uint64 r;
    const uint64 ceil = -len % len;
    for (;;) {
      r = RandUInt64();
      if (r >= ceil)
        break;
    }
    return (r % len);
  } else {
    return 0;
  }
}

