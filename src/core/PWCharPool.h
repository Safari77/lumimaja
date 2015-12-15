/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// \file PWCharPool.h
//-----------------------------------------------------------------------------

#ifndef __PWCHARPOOL_H
#define __PWCHARPOOL_H

#include "os/typedefs.h"
#include "StringX.h"
#include "PWPolicy.h"

#include <algorithm>

/*
 * This class is used to create a random password based on the policy
 * defined in the constructor.
 * The class ensures that if a character type is selected, then at least one
 * character from that type will be in the generated password. (i.e., at least
 * one digit if usedigits is set in the constructor).
 *
 * The usage scenario is something like:
 * CPasswordCharPool pwgen(policy);
 * StringX pwd = pwgen.MakePassword();
 *
 * CheckPassword() is used to verify the strength of existing passwords,
 * i.e., the password used to protect the database.
 */

class CPasswordCharPool
{
public:
  CPasswordCharPool(const PWPolicy &policy);
  StringX MakePassword() const;

  ~CPasswordCharPool();

  static bool CheckPassword(const StringX &pwd, StringX &error);
  static stringT GetDefaultSymbols();
  static stringT GetPronounceableSymbols() {return pronounceable_symbol_chars;}
  static void ResetDefaultSymbols(); // reset the preference string

private:
  enum CharType {LOWERCASE = 0, UPPERCASE = 1,
                 DIGIT = 2, SYMBOL = 3, NUMTYPES = 4};
  // select a chartype with weighted probability
  charT GetRandomChar() const;
  StringX MakePronounceable() const;
  StringX MakeHex() const;

  // here are all the character types
  static const charT std_lowercase_chars[];
  static const charT std_uppercase_chars[];
  static const charT std_digit_chars[];
  static const charT std_symbol_chars[];
  static const charT pronounceable_symbol_chars[];

  stringT m_char_array;

  // Following state vars set by ctor, used by MakePassword()
  const uint m_pwlen;
  const bool m_uselowercase;
  const bool m_useuppercase;
  const bool m_usedigits;
  const bool m_usesymbols;
  const bool m_pronounceable;

  bool m_bDefaultSymbols;

  CPasswordCharPool &operator=(const CPasswordCharPool &);
};

#endif /*  __PWCHARPOOL_H */
