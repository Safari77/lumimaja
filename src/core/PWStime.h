/*
* Copyright (c) 2003-2014 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// PWStime.h
//-----------------------------------------------------------------------------
#ifndef __PWSTIME_H
#define __PWSTIME_H

/** \file
 * "Time is on my side, yes it is" - the Rolling Stones
 *
 * 64 bit times
 */

#include <ctime>

#include "coredefs.h"

class PWStime
{
public:
  enum {TIME_LEN = 8};
  PWStime(); // default c'tor initiates value to current time
  PWStime(const PWStime &);
  PWStime(std::time_t);
  PWStime(const unsigned char *pbuf); // pbuf points to a TIME_LEN array
  ~PWStime();
  PWStime &operator=(const PWStime &);
  PWStime &operator=(std::time_t);

  operator int64() const;
  operator const unsigned char *() const;
  operator const char *() const;

private:
  void setTime(int64 t);
  unsigned char m_time[TIME_LEN];
};

#endif /* __PWSTIME_H */
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
