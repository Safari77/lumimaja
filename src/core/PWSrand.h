/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// PWSrand.h
//-----------------------------------------------------------------------------

#ifndef __PWSRAND_H
#define __PWSRAND_H

#include "Util.h"

class PWSrand
{
public:
  static PWSrand *GetInstance();
  static void DeleteInstance();

  void GetRandomData( void * const buffer, unsigned long length );
  stringT RandAZ(const size_t length); // return stringT filled with random a-z0-9 chars
                                       // length is length

  uint32 RandUInt(); // generate a random uint
  uint64 RandUInt64();

  //  generate a random integer in [0, len)
  uint32 RangeRand(uint32 len);
  uint64 RangeRand64(uint64 len);

private:
  PWSrand(); // start with some minimal entropy
  ~PWSrand();

  static PWSrand *self;
};
#endif /*  __PWSRAND_H */
