/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// \file PWCharPool.cpp
//-----------------------------------------------------------------------------

#include "PWCharPool.h"
#include "Util.h"
#include "core.h"
#include "PWSrand.h"
#include "PWSprefs.h"
#include "trigram.h" // for pronounceable passwords

#include "os/typedefs.h"
#include "os/pws_tchar.h"

#include <string>
#include <vector>

using namespace std;

// Following macro get length of std_*_chars less the trailing \0
// compile time equivalent of strlen()
#define LENGTH(s) (sizeof(s)/sizeof(s[0]) - 1)

const charT CPasswordCharPool::std_lowercase_chars[] = _T("abcdefghijklmnopqrstuvwxyz");
const charT CPasswordCharPool::std_uppercase_chars[] = _T("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
const charT CPasswordCharPool::std_digit_chars[] = _T("0123456789");
const charT CPasswordCharPool::std_symbol_chars[] = _T("+-=_@#$%^&;:,.<>/~\\[](){}?!|*");
const charT CPasswordCharPool::pronounceable_symbol_chars[] = _T("@&(#!|$+");

//-----------------------------------------------------------------------------

CPasswordCharPool::CPasswordCharPool(const PWPolicy &policy)
  : m_pwlen(policy.length),
    m_uselowercase(policy.flags & PWPolicy::UseLowercase ? true : false),
    m_useuppercase(policy.flags & PWPolicy::UseUppercase ? true : false),
    m_usedigits(policy.flags & PWPolicy::UseDigits ? true : false),
    m_usesymbols(policy.flags & PWPolicy::UseSymbols ? true : false),
    m_pronounceable(policy.flags & PWPolicy::MakePronounceable ? true : false),
    m_bDefaultSymbols(false)
{
  ASSERT(m_pwlen > 0);
  ASSERT(m_uselowercase || m_useuppercase || m_usedigits ||
         m_usesymbols   || m_pronounceable);

  m_char_array.assign(L"");
  if (m_uselowercase) m_char_array += std_lowercase_chars;
  if (m_useuppercase) m_char_array += std_uppercase_chars;
  if (m_usedigits) m_char_array += std_digit_chars;
  if (m_usesymbols) {
    if (policy.symbols.empty()) {
      StringX sx_symbols = PWSprefs::GetInstance()->GetPref(PWSprefs::DefaultSymbols);
      if (sx_symbols.empty()) {
        m_char_array += std_symbol_chars;
      } else {
        m_bDefaultSymbols = true;
        m_char_array += sx_symbols.c_str();
      }
    } else {
      m_char_array += policy.symbols.c_str();
    }
  }
  ASSERT(m_char_array.length() > 0);
  fprintf(stderr, "m_char_array=[%ls] len=%zu\n",
          m_char_array.c_str(), m_char_array.length());
}

CPasswordCharPool::~CPasswordCharPool()
{
    m_char_array.assign(L"");
}

stringT CPasswordCharPool::GetDefaultSymbols()
{
  stringT symbols;
  const StringX sx_symbols = PWSprefs::GetInstance()->GetPref(PWSprefs::DefaultSymbols);
  if (!sx_symbols.empty())
    symbols = sx_symbols.c_str();
  else
    symbols = std_symbol_chars;
  return symbols;
}

void CPasswordCharPool::ResetDefaultSymbols()
{
  PWSprefs::GetInstance()->SetPref(PWSprefs::DefaultSymbols, std_symbol_chars);
}

charT CPasswordCharPool::GetRandomChar() const
{
  ASSERT(m_char_array.length() > 0);
  PWSrand *ri = PWSrand::GetInstance();
  uint r = ri->RangeRand(static_cast<uint32>(m_char_array.length()));
  return m_char_array.at(r);
}

StringX CPasswordCharPool::MakePassword() const
{
  // We don't care if the policy is inconsistent e.g. 
  // number of lower case chars > 1 + make pronouceable
  // The individual routines (normal, hex, pronouceable) will
  // ignore what they don't need.
  // Saves an awful amount of bother with setting values to zero and
  // back as the user changes their minds!

  ASSERT(m_pwlen > 0);

  if (m_pronounceable)
    return MakePronounceable();

  StringX password;
  bool hasalpha, hasdigit;
  do {
    hasalpha = !m_uselowercase && !m_useuppercase; // prevent infinite loop
    hasdigit = !m_usedigits;
    charT ch;

    password.assign(_T(""));
    for (uint i = 0; i < m_pwlen; i++) {
      ch = GetRandomChar();
      password += ch;
      if (_istalpha(ch)) hasalpha = true;
      if (_istdigit(ch)) hasdigit = true;
    }
    if (m_pwlen <= 4) break;
  } while(!hasalpha || !hasdigit);

  ASSERT(password.length() == size_t(m_pwlen));
  return password;
};

StringX CPasswordCharPool::MakePronounceable() const
{
  /**
   * Following based on gpw.C from
   * http://www.multicians.org/thvv/tvvtools.html
   * Thanks to Tom Van Vleck, Morrie Gasser, and Dan Edwards.
   */
  charT c1, c2, c3;  /* array indices */
  long sumfreq;      /* total frequencies[c1][c2][*] */
  long ranno;        /* random number in [0,sumfreq] */
  long sum;          /* running total of frequencies */
  uint nchar;        /* number of chars in password so far */
  PWSrand *pwsrnd = PWSrand::GetInstance();
  stringT password(m_pwlen, 0);

  /* Pick a random starting point. */
  /* (This cheats a little; the statistics for three-letter
     combinations beginning a word are different from the stats
     for the general population.  For example, this code happily
     generates "mmitify" even though no word in my dictionary
     begins with mmi. So what.) */
  sumfreq = sigma;  // sigma calculated by loadtris
  ranno = static_cast<long>(pwsrnd->RangeRand(sumfreq+1)); // Weight by sum of frequencies
  sum = 0;
  for (c1 = 0; c1 < 26; c1++) {
    for (c2 = 0; c2 < 26; c2++) {
      for (c3 = 0; c3 < 26; c3++) {
        sum += tris[int(c1)][int(c2)][int(c3)];
        if (sum > ranno) { // Pick first value
          password[0] = charT('a') + c1;
          password[1] = charT('a') + c2;
          password[2] = charT('a') + c3;
          c1 = c2 = c3 = 26; // Break all loops.
        } // if sum
      } // for c3
    } // for c2
  } // for c1

  /* Do a random walk. */
  nchar = 3;  // We have three chars so far.
  while (nchar < m_pwlen) {
    c1 = password[nchar-2] - charT('a'); // Take the last 2 chars
    c2 = password[nchar-1] - charT('a'); // .. and find the next one.
    sumfreq = 0;
    for (c3 = 0; c3 < 26; c3++)
      sumfreq += tris[int(c1)][int(c2)][int(c3)];
    /* Note that sum < duos[c1][c2] because
       duos counts all digraphs, not just those
       in a trigraph. We want sum. */
    if (sumfreq == 0) { // If there is no possible extension..
      break;  // Break while nchar loop & print what we have.
    }
    /* Choose a continuation. */
    ranno = static_cast<long>(pwsrnd->RangeRand(sumfreq+1)); // Weight by sum of frequencies
    sum = 0;
    for (c3 = 0; c3 < 26; c3++) {
      sum += tris[int(c1)][int(c2)][int(c3)];
      if (sum > ranno) {
        password[nchar++] = charT('a') + c3;
        c3 = 26;  // Break the for c3 loop.
      }
    } // for c3
  } // while nchar
  /*
   * password now has an all-lowercase pronounceable password
   * We now want to modify it per policy:
   * If m_usedigits and/or m_usesymbols, replace some chars with
   * corresponding 'leet' values
   * Also enforce m_useuppercase & m_uselowercase policies
   */

  // case
  uint i;
  if (m_uselowercase && !m_useuppercase)
    ; // nothing to do here
  else if (!m_uselowercase && m_useuppercase)
    for (i = 0; i < m_pwlen; i++) {
      if (_istalpha(password[i]))
        password[i] = static_cast<charT>(_totupper(password[i]));
    }
  else if (m_uselowercase && m_useuppercase) // mixed case
    for (i = 0; i < m_pwlen; i++) {
      if (_istalpha(password[i]) && pwsrnd->RandUInt() % 2)
        password[i] = static_cast<charT>(_totupper(password[i]));
    }

  return password.c_str();
}

bool CPasswordCharPool::CheckPasswordClasses(const StringX &pwd)
{
  // check for at least one uppercase and lowercase and one (digit or other)
  bool has_uc = false, has_lc = false, has_digit = false, has_other = false;

  for (size_t i = 0; i < pwd.length(); i++) {
    charT c = pwd[i];
    if (_istlower(c)) has_lc = true;
    else if (_istupper(c)) has_uc = true;
    else if (_istdigit(c)) has_digit = true;
    else has_other = true;
  }

  if (has_uc && has_lc && (has_digit || has_other)) {
    return true;
  } else {
    return false;
  }
}

bool CPasswordCharPool::CheckPassword(const StringX &pwd, StringX &error)
{
  /**
   * A password is "Good enough" if:
   * It is at least SufficientLength characters long
   * OR
   * It is at least MinLength characters long AND it has
   * at least one uppercase and one lowercase and one (digit or other).
   *
   * A future enhancement of this might be to calculate the Shannon Entropy
   * in combination with a minimum password length.
   * http://rosettacode.org/wiki/Entropy
   */

  const size_t SufficientLength = 12;
  const size_t MinLength = 8;

  if (pwd.length() >= SufficientLength) {
    return true;
  }

  if (pwd.length() < MinLength) {
    LoadAString(error, IDSC_PASSWORDTOOSHORT);
    return false;
  }

  if (CheckPasswordClasses(pwd)) {
    return true;
  } else {
    LoadAString(error, IDSC_PASSWORDPOOR);
    return false;
  }
}
