/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// ItemField.h
//-----------------------------------------------------------------------------

#ifndef __ITEMFIELD_H
#define __ITEMFIELD_H

#include "StringX.h"
//-----------------------------------------------------------------------------

/*
* CItemField contains the data for a given CItemData field in encrypted
* form.
* Set() encrypts, Get() decrypts
*/

class CItemField
{
public:
  explicit CItemField(unsigned char type = 0xff): m_Type(type), m_Length(0), m_Data(NULL)
  {}
  CItemField(const CItemField &that); // copy ctor
  ~CItemField() {if (m_Length > 0) delete[] m_Data;}

  CItemField &operator=(const CItemField &that);

  void Set(const StringX &value, unsigned char type = 0xff);
  void Set(const unsigned char* value, size_t length, unsigned char type = 0xff);

  void Get(StringX &value) const;
  void Get(unsigned char *value, size_t &length) const;
  unsigned char GetType() const {return m_Type;}
  size_t GetLength() const {return m_Length;}
  bool IsEmpty() const {return m_Length == 0;}
  void Empty();

private:
  unsigned char m_Type; // almost const
  size_t m_Length;
  unsigned char *m_Data;
};

#endif /* __ITEMFIELD_H */
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
