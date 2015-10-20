/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// \file ItemField.cpp
//-----------------------------------------------------------------------------

#include <math.h>

#include "ItemField.h"
#include "Util.h"
#include "PWSrand.h"
#include "os/funcwrap.h"

CItemField::CItemField(const CItemField &that)
  : m_Type(that.m_Type), m_Length(that.m_Length)
{
  if (m_Length > 0) {
    m_Data = new unsigned char[m_Length];
    memcpy(m_Data, that.m_Data, m_Length);
  } else {
    m_Data = NULL;
  }
}

CItemField &CItemField::operator=(const CItemField &that)
{
  if (this != &that) {
    m_Type = that.m_Type;
    m_Length = that.m_Length;
    delete[] m_Data;
    if (m_Length > 0) {
      m_Data = new unsigned char[m_Length];
      memcpy(m_Data, that.m_Data, m_Length);
    } else {
      m_Data = NULL;
    }
  }
  return *this;
}

void CItemField::Empty()
{
  if (m_Data != NULL) {
    delete[] m_Data;
    m_Data = NULL;
    m_Length = 0;
  }
}

void CItemField::Set(const unsigned char* value, size_t length,
                     unsigned char type)
{
  m_Length = length;

  delete[] m_Data;

  if (m_Length == 0) {
    m_Data = NULL;
  } else {
    m_Data = new unsigned char[m_Length];
    if (m_Data == NULL) { // out of memory - try to fail gracefully
      m_Length = 0; // at least keep structure consistent
      return;
    }
    memcpy(m_Data, value, m_Length);
  }
  if (type != 0xff)
    m_Type = type;
}

void CItemField::Set(const StringX &value, unsigned char type)
{
  const LPCTSTR plainstr = value.c_str();

  Set(reinterpret_cast<const unsigned char *>(plainstr),
      value.length() * sizeof(*plainstr), type);
}

void CItemField::Get(unsigned char *value, size_t &length) const
{
  // Sanity check: length is 0 iff data ptr is NULL
  ASSERT((m_Length == 0 && m_Data == NULL) ||
         (m_Length > 0 && m_Data != NULL));
  /*
  * length is an in/out parameter:
  * In: size of value array - must be at least 1
  * Out: size of data stored: m_Length (No trailing zero!)
  */
  if (m_Length == 0) {
    value[0] = TCHAR('\0');
    length = 0;
  } else {
    ASSERT(length >= m_Length);
    memcpy(value, m_Data, m_Length);
    length = m_Length;
  }
}

void CItemField::Get(StringX &value) const
{
  // Sanity check: length is 0 iff data ptr is NULL
  ASSERT((m_Length == 0 && m_Data == NULL) ||
         (m_Length > 0 && m_Data != NULL && m_Length % sizeof(TCHAR) == 0));

  if (m_Length == 0) {
    value = _T("");
  } else {
    TCHAR *pt = reinterpret_cast<TCHAR *>(m_Data);
    size_t x;

    // copy to value TCHAR by TCHAR
    for (x = 0; x < m_Length/sizeof(TCHAR); x++)
      value += pt[x];
  }
}
