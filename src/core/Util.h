/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#ifndef __UTIL_H
#define __UTIL_H

// Util.h
//-----------------------------------------------------------------------------

#include "StringX.h"
#include "PwsPlatform.h"
#include "UTF8Conv.h"
#include "miniutf.hpp"

#include "../os/debug.h"
#include "../os/typedefs.h"
#include "../os/mem.h"

#include <sstream>
#include <stdarg.h>
#include <stdalign.h>
#include <iostream>
#include <vector>

#define SaltLength 20
#define StuffSize 10

#define SaltLengthV3 32

//Use non-standard dash (ANSI decimal 173) for separation
#define SPLTCHR _T('\xAD')
#define SPLTSTR _T("  \xAD  ")
#define DEFUSERCHR _T('\xA0')

//Version defines
#define V10 0
#define V15 1

#define barrier() asm volatile("": : :"memory")

extern void trashMemory(void *buffer, size_t length);
extern void trashMemory(LPTSTR buffer, size_t length);

extern void ConvertString(const StringX &text,
                          unsigned char *&txt, size_t &txtlen);
// Do Unicode NFC normalization
// On error returns false and does not write any string to txt
extern bool ConvertStringNFC(const StringX &text,
                             unsigned char *&txt,
                             size_t &txtlen);

/*
* Get an integer that is stored in little-endian format not assuming any alignments
*/
inline uint16 getInt16(const unsigned char buf[2])
{
  return buf[0] | (buf[1] << 8);
}

/*
* Store an integer that is stored in little-endian format
*/
inline void putInt16(unsigned char buf[2], const uint16 val)
{
  buf[0] = val;
  buf[1] = (val >> 8);
}

inline uint32 getInt32(const unsigned char buf[4])
{
  return (buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24));
}

inline void putInt32(unsigned char buf[4], const uint32 val)
{
  buf[0] = val;
  buf[1] = (val >> 8);
  buf[2] = (val >> 16);
  buf[3] = (val >> 24);
}

inline uint64 getInt64(const unsigned char buf[8])
{
  return (buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24) |
          (uint64(buf[4]) << 32) | (uint64(buf[5]) << 40) |
          (uint64(buf[6]) << 48) | (uint64(buf[7]) << 56));
}

inline void putInt64(unsigned char buf[8], const uint64 val)
{
  buf[0] = val;
  buf[1] = (val >> 8);
  buf[2] = (val >> 16);
  buf[3] = (val >> 24);
  buf[4] = (val >> 32);
  buf[5] = (val >> 40);
  buf[6] = (val >> 48);
  buf[7] = (val >> 56);
}

static inline void vec_push32(std::vector<uint8_t>&out, const uint32 val)
{
  out.insert(out.end(), val);
  out.insert(out.end(), val >> 8);
  out.insert(out.end(), val >> 16);
  out.insert(out.end(), val >> 24);
}

static inline uint32 vec_pull32(const std::vector<uint8_t>&in, size_t &pos)
{
  uint32 val = in.at(pos++);
  val |= (uint32(in.at(pos++)) << 8);
  val |= (uint32(in.at(pos++)) << 16);
  val |= (uint32(in.at(pos++)) << 24);
  return val;
}

bool operator==(const std::string& str1, const stringT& str2);
inline bool operator==(const stringT& str1, const std::string &str2) { return str2 == str1; }

namespace PWSUtil {
  // namespace of common utility functions

  // For Windows implementation, hide Unicode abstraction,
  // and use secure versions (_s) when available
  void strCopy(LPTSTR target, size_t tcount, const LPCTSTR source, size_t scount);
  size_t strLength(const LPCTSTR str);
  // Time conversion result formats:
  enum TMC {TMC_ASC_UNKNOWN, TMC_ASC_NULL, TMC_EXPORT_IMPORT, TMC_XML,
            TMC_LOCALE, TMC_LOCALE_DATE_ONLY};
  StringX ConvertToDateTimeString(const time_t &t, TMC result_format);
  stringT GetNewFileName(const stringT &oldfilename, const stringT &newExtn);
  extern const TCHAR *UNKNOWN_ASC_TIME_STR, *UNKNOWN_XML_TIME_STR;
  void GetTimeStamp(stringT &sTimeStamp, const bool bShort = false);
  stringT GetTimeStamp(const bool bShort = false);
  stringT Base64Encode(const BYTE *inData, size_t len);
  void Base64Decode(const StringX &inString, BYTE* &outData, size_t &out_len);
  StringX NormalizeTTT(const StringX &in, size_t maxlen = 64);
  bool WriteXMLField(std::ostream &os, const char *fname,
                     const StringX &value, CUTF8Conv &utf8conv,
                     const char *tabs = "\t\t");
  std::string GetXMLTime(int indent, const char *name,
                         time_t t, CUTF8Conv &utf8conv);

  StringX DeDupString(StringX &in_string);
  stringT GetSafeXMLString(const StringX &sxInString);
}

///////////////////////////////////////////////////////
// Following two templates lets us use the two types
// of iterators in a common (templatized) function when 
// all we need to do is to access the underlying value
template <typename PairAssociativeContainer>
class get_second {
  public:
    typedef typename PairAssociativeContainer::mapped_type mapped_type;
    typedef typename PairAssociativeContainer::const_iterator const_iterator;
    const mapped_type& operator()(const_iterator val) { return val->second; }
};

template <typename SequenceContainer>
class dereference {
  public:
    typedef typename SequenceContainer::value_type value_type;
    typedef typename SequenceContainer::const_iterator const_iterator;
    const value_type& operator()(const_iterator itr) { return *itr; }
};

extern int GetStringBufSize(const TCHAR *fmt, va_list args);
#endif /* __UTIL_H */
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
