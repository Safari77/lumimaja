/*
* Copyright (c) 2003-2014 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/** \file
* Implementation of PWStime
*/

#include <ctime>
#include <cstring>
#include <algorithm>

#include "PWStime.h"
#include "Util.h"

// Pesky WinDef.h...
#ifdef min
#undef min
#endif

PWStime::PWStime()
{
  setTime(reinterpret_cast<int64>(std::time(NULL)));
}

PWStime::PWStime(const PWStime &pwt)
{
  memcpy(m_time, pwt.m_time, sizeof(m_time));
}

PWStime::PWStime(time_t t)
{
  setTime(int64(t));
}

PWStime::PWStime(const unsigned char *pbuf)
{
  memcpy(m_time, pbuf, TIME_LEN);
}

PWStime::~PWStime()
{
}

PWStime &PWStime::operator=(const PWStime &that)
{
  if (this != &that) 
    memcpy(m_time, that.m_time, sizeof(m_time));
  return *this;

}

PWStime &PWStime::operator=(std::time_t t)
{
  setTime(int64(t));
  return *this;
}

PWStime::operator int64() const
{
  return getInt64(m_time);
}

PWStime::operator const unsigned char *() const
{
  return m_time;
}

PWStime::operator const char *() const
{
  return reinterpret_cast<const char *>(m_time);
}

void PWStime::setTime(int64 t)
{
  putInt64(m_time, t);
}
