/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// \file Util.cpp
//-----------------------------------------------------------------------------

#include "PWSrand.h"
#include "PwsPlatform.h"
#include "core.h"
#include "StringXStream.h"
#include "PWPolicy.h"

#include "Util.h"

#include "os/debug.h"
#include "os/pws_tchar.h"
#include "os/dir.h"

#include <stdio.h>
#include <time.h>
#ifdef _WIN32
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
#include <sstream>
#include <iomanip>
#include <errno.h>

using namespace std;

//-----------------------------------------------------------------------------
//Overwrite the memory
// used to be a loop here, but this was deemed (1) overly paranoid
// (2) The wrong way to scrub DRAM memory
// see http://www.cs.auckland.ac.nz/~pgut001/pubs/secure_del.html
// and http://www.cypherpunks.to/~peter/usenix01.pdf

#ifdef _WIN32
#pragma optimize("",off)
#endif
void trashMemory(void *buffer, size_t length)
{
  ASSERT(buffer != NULL);
  // {kjp} no point in looping around doing nothing is there?
  if (length > 0) {
    std::memset(buffer,    0, length);
    barrier();
  }
}
#ifdef _WIN32
#pragma optimize("",on)
#endif
void trashMemory(LPTSTR buffer, size_t length)
{
  trashMemory(reinterpret_cast<unsigned char *>(buffer), length * sizeof(buffer[0]));
}

void ConvertString(const StringX &text,
                   unsigned char *&txt,
                   size_t &txtlen)
{
  LPCTSTR txtstr = text.c_str();
  txtlen = text.length();

#ifdef _WIN32
  txt = new unsigned char[3 * txtlen]; // safe upper limit
  int len = WideCharToMultiByte(CP_ACP, 0, txtstr, static_cast<int>(txtlen),
                                LPSTR(txt), static_cast<int>(3 * txtlen), NULL, NULL);
  ASSERT(len != 0);
#else
  mbstate_t mbs;
  memset(&mbs, 0, sizeof(mbstate_t));
  size_t len = wcsrtombs(NULL, &txtstr, 0, &mbs);
  txt = new unsigned char[len + 1];
  len = wcsrtombs(reinterpret_cast<char *>(txt), &txtstr, len, &mbs);
  ASSERT(len != size_t(-1));
#endif
  txtlen = len;
  txt[len] = '\0';
}

// PWSUtil implementations

void PWSUtil::strCopy(LPTSTR target, size_t tcount, const LPCTSTR source, size_t scount)
{
  UNREFERENCED_PARAMETER(tcount); //not used in now in non-MSVS wrapped version of _tcsncpy_s
  _tcsncpy_s(target, tcount, source, scount);
}

size_t PWSUtil::strLength(const LPCTSTR str)
{
  return _tcslen(str);
}

const TCHAR *PWSUtil::UNKNOWN_XML_TIME_STR = _T("1970-01-01 00:00:00");
const TCHAR *PWSUtil::UNKNOWN_ASC_TIME_STR = _T("Unknown");

StringX PWSUtil::ConvertToDateTimeString(const time_t &t, TMC result_format)
{
  StringX ret;
  if (t != 0) {
    TCHAR datetime_str[80];
    struct tm *st;
    struct tm st_s;
    errno_t err;
    err = localtime_s(&st_s, &t);  // secure version
    if (err != 0) // invalid time
      return ConvertToDateTimeString(0, result_format);
    st = &st_s; // hide difference between versions
    switch (result_format) {
    case TMC_EXPORT_IMPORT:
      _tcsftime(datetime_str, sizeof(datetime_str) / sizeof(datetime_str[0]),
                _T("%Y/%m/%d %H:%M:%S"), st);
      break;
    case TMC_XML:
      _tcsftime(datetime_str, sizeof(datetime_str) / sizeof(datetime_str[0]),
                _T("%Y-%m-%dT%H:%M:%S"), st);
      break;
    case TMC_LOCALE:
      setlocale(LC_TIME, "");
      _tcsftime(datetime_str, sizeof(datetime_str) / sizeof(datetime_str[0]),
                _T("%c"), st);
      break;
    case TMC_LOCALE_DATE_ONLY:
      setlocale(LC_TIME, "");
      _tcsftime(datetime_str, sizeof(datetime_str) / sizeof(datetime_str[0]),
                _T("%x"), st);
      break;
    default:
      if (_tasctime_s(datetime_str, 32, st) != 0)
        return ConvertToDateTimeString(0, result_format);
    }
    ret = datetime_str;
  } else { // t == 0
    switch (result_format) {
    case TMC_ASC_UNKNOWN:
      ret = UNKNOWN_ASC_TIME_STR;
      break;
    case TMC_XML:
      ret = UNKNOWN_XML_TIME_STR;
      break;
    default:
      ret = _T("");
    }
  }
  // remove the trailing EOL char.
  TrimRight(ret);
  return ret;
}

stringT PWSUtil::GetNewFileName(const stringT &oldfilename,
                                const stringT &newExtn)
{
  stringT inpath(oldfilename);
  stringT drive, dir, fname, ext;
  stringT outpath;

  if (pws_os::splitpath(inpath, drive, dir, fname, ext)) {
    outpath = pws_os::makepath(drive, dir, fname, newExtn);
  } else
    ASSERT(0);
  return outpath;
}

stringT PWSUtil::GetTimeStamp(const bool bShort)
{
  stringT sTimeStamp;
  GetTimeStamp(sTimeStamp, bShort);
  return sTimeStamp;
}

void PWSUtil::GetTimeStamp(stringT &sTimeStamp, const bool bShort)
{
  // Now re-entrant
  // Gets datetime stamp in format YYYY/MM/DD HH:MM:SS.mmm
  // If bShort == true, don't add milli-seconds

#ifdef _WIN32
  struct _timeb *ptimebuffer;
  ptimebuffer = new _timeb;
  _ftime_s(ptimebuffer);
  time_t the_time = ptimebuffer->time;
#else
  struct timeval *ptimebuffer;
  ptimebuffer = new timeval;
  gettimeofday(ptimebuffer, NULL);
  time_t the_time = ptimebuffer->tv_sec;
#endif
  StringX cmys_now = ConvertToDateTimeString(the_time, TMC_EXPORT_IMPORT);

  if (bShort) {
    sTimeStamp = cmys_now.c_str();
  } else {
    ostringstreamT *p_os;
    p_os = new ostringstreamT;
    *p_os << cmys_now << TCHAR('.') << setw(3) << setfill(TCHAR('0'))
          << static_cast<unsigned int>(the_time);

    sTimeStamp = p_os->str();
    delete p_os;
  }
  delete ptimebuffer;
}

stringT PWSUtil::Base64Encode(const BYTE *strIn, size_t len)
{
  stringT cs_Out;
  static const TCHAR base64ABC[] =
    _S("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

  for (size_t i = 0; i < len; i += 3) {
    long l = ( static_cast<long>(strIn[i]) << 16 ) |
               (((i + 1) < len) ? (static_cast<long>(strIn[i + 1]) << 8) : 0) |
               (((i + 2) < len) ? static_cast<long>(strIn[i + 2]) : 0);

    cs_Out += base64ABC[(l >> 18) & 0x3F];
    cs_Out += base64ABC[(l >> 12) & 0x3F];
    if (i + 1 < len) cs_Out += base64ABC[(l >> 6) & 0x3F];
    if (i + 2 < len) cs_Out += base64ABC[(l ) & 0x3F];
  }

  switch (len % 3) {
    case 1:
      cs_Out += TCHAR('=');
    case 2:
      cs_Out += TCHAR('=');
    default:
      break;
  }
  return cs_Out;
}

void PWSUtil::Base64Decode(const StringX &inString, BYTE * &outData, size_t &out_len)
{
  static const char szCS[]=
    "=ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  int iDigits[4] = {0,0,0,0};

  size_t st_length = 0;
  const size_t in_length = inString.length();

  size_t i1, i2, i3;
  for (i2 = 0; i2 < in_length; i2 += 4) {
    iDigits[0] = iDigits[1] = iDigits[2] = iDigits[3] = -1;

    for (i1 = 0; i1 < sizeof(szCS) - 1; i1++) {
      for (i3 = i2; i3 < i2 + 4; i3++) {
        if (i3 < in_length &&  inString[i3] == szCS[i1])
          iDigits[i3 - i2] = reinterpret_cast<int &>(i1) - 1;
      }
    }

    outData[st_length] = (static_cast<BYTE>(iDigits[0]) << 2);

    if (iDigits[1] >= 0) {
      outData[st_length] += (static_cast<BYTE>(iDigits[1]) >> 4) & 0x3;
    }

    st_length++;

    if (iDigits[2] >= 0) {
      outData[st_length++] = ((static_cast<BYTE>(iDigits[1]) & 0x0f) << 4)
        | ((static_cast<BYTE>(iDigits[2]) >> 2) & 0x0f);
    }

    if (iDigits[3] >= 0) {
      outData[st_length++] = ((static_cast<BYTE>(iDigits[2]) & 0x03) << 6)
        | (static_cast<BYTE>(iDigits[3]) & 0x3f);
    }
  }

  out_len = st_length;
}

StringX PWSUtil::NormalizeTTT(const StringX &in, size_t maxlen)
{
  StringX ttt;
  if (in.length() >= maxlen) {
    ttt = in.substr(0, maxlen/2-6) +
      _T(" ... ") + in.substr(in.length() - maxlen/2);
  } else {
    ttt = in;
  }
  return ttt;
}

bool ValidateXMLCharacters(const StringX &value, ostringstream &ostInvalidPositions)
{
  // From: http://www.w3.org/TR/REC-xml/#sec-cdata-sect
  // CDATA Sections
  // [18]    CDSect   ::=    CDStart CData CDEnd
  // [19]    CDStart  ::=    '<![CDATA['
  // [20]    CData    ::=    (Char* - (Char* ']]>' Char*))
  // [21]    CDEnd    ::=    ']]>'

  // From: http://www.w3.org/TR/REC-xml/#NT-Char
  //  Char    ::=    #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] |
  //                 [#x10000-#x10FFFF]
  // any Unicode character, excluding the surrogate blocks, FFFE, and FFFF.

  // Easy to check low values (0x00-0x1f excluding 3 above) and the 2 special values
  // Not so easy for the range 0xD800 to 0xDFFF (Unicode only) - could use regex
  // but expected to be slower than this!

  ostInvalidPositions.str("");
  bool bInvalidFound(false);
  for (size_t i = 0; i < value.length(); i++) {
    TCHAR current = value[i];
    if (!((current == 0x09) ||
          (current == 0x0A) ||
          (current == 0x0D) ||
          ((current >=    0x20) && (current <=   0xD7FF)) ||
          ((current >=  0xE000) && (current <=   0xFFFD)) ||
          ((current >= 0x10000) && (current <= 0x10FFFF)))) {
      if (bInvalidFound) {
        // Already 1 position, add a comma
        ostInvalidPositions << ", ";
      }
      bInvalidFound = true;
      ostInvalidPositions << (i + 1);
    }
  }
  return !bInvalidFound;
}

bool PWSUtil::WriteXMLField(ostream &os, const char *fname,
                            const StringX &value, CUTF8Conv &utf8conv,
                            const char *tabs)
{
  const unsigned char *utf8 = NULL;
  size_t utf8Len = 0;
  ostringstream ostInvalidPositions;
  if (!ValidateXMLCharacters(value, ostInvalidPositions)) {
    os << tabs << "<!-- Field '<" << fname << ">' contains invalid XML character(s)" << endl;
    os << tabs << "   at position(s): " << ostInvalidPositions.str().c_str() << endl;
    os << tabs << "   and has been skipped -->" << endl;
    return false;
  }

  StringX::size_type p = value.find(_T("]]>")); // special handling required
  if (p == StringX::npos) {
    // common case
    os << tabs << "<" << fname << "><![CDATA[";
    if (utf8conv.ToUTF8(value, utf8, utf8Len))
      os.write(reinterpret_cast<const char *>(utf8), utf8Len);
    else
      os << "Internal error - unable to convert field to utf-8";
    os << "]]></" << fname << ">" << endl;
  } else {
    // value has "]]>" sequence(s) that need(s) to be escaped
    // Each "]]>" splits the field into two CDATA sections, one ending with
    // ']]', the other starting with '>'
    os << tabs << "<" << fname << ">";
    size_t from = 0, to = p + 2;
    do {
      StringX slice = value.substr(from, (to - from));
      os << "<![CDATA[";
      if (utf8conv.ToUTF8(slice, utf8, utf8Len))
        os.write(reinterpret_cast<const char *>(utf8), utf8Len);
      else
        os << "Internal error - unable to convert field to utf-8";
      os << "]]><![CDATA[";
      from = to;
      p = value.find(_T("]]>"), from); // are there more?
      if (p == StringX::npos) {
        to = value.length();
        slice = value.substr(from, (to - from));
      } else {
        to = p + 2;
        slice = value.substr(from, (to - from));
        from = to;
        to = value.length();
      }
      if (utf8conv.ToUTF8(slice, utf8, utf8Len))
        os.write(reinterpret_cast<const char *>(utf8), utf8Len);
      else
        os << "Internal error - unable to convert field to utf-8";
      os << "]]>";
    } while (p != StringX::npos);
    os << "</" << fname << ">" << endl;
  } // special handling of "]]>" in value.
  return true;
}

string PWSUtil::GetXMLTime(int indent, const char *name,
                           time_t t, CUTF8Conv &utf8conv)
{
  int i;
  const StringX tmp = PWSUtil::ConvertToDateTimeString(t, TMC_XML);
  ostringstream oss;
  const unsigned char *utf8 = NULL;
  size_t utf8Len = 0;


  for (i = 0; i < indent; i++) oss << "\t";
  oss << "<" << name << ">" ;
  utf8conv.ToUTF8(tmp.substr(0, 10), utf8, utf8Len);
  oss.write(reinterpret_cast<const char *>(utf8), utf8Len);
  oss << "T";
  utf8conv.ToUTF8(tmp.substr(tmp.length() - 8), utf8, utf8Len);
  oss.write(reinterpret_cast<const char *>(utf8), utf8Len);
  oss << "</" << name << ">" << endl;
  return oss.str();
}

/**
 * Get TCHAR buffer size by format string with parameters
 * @param[in] fmt - format string
 * @param[in] args - arguments for format string
 * @return buffer size including NULL-terminating character
*/
int GetStringBufSize(const TCHAR *fmt, va_list args)
{
  TCHAR *buffer=NULL;

  int len=0;

#ifdef _WIN32
  len = _vsctprintf(fmt, args) + 1;
#else
  va_list ar;
  va_copy(ar, args);
  // Linux doesn't do this correctly :-(
  int guess = 16;
  while (1) {
    len = guess;
    buffer = new TCHAR[len];
    len = _vstprintf_s(buffer, len, fmt, ar);
    va_end(ar);//after using args we should reset list
    va_copy(ar, args);
    if (len++ > 0)
      break;
    else { // too small, resize & try again
      delete[] buffer;
      buffer = NULL;
      guess *= 2;
    }
  }
  va_end(ar);
#endif
  if (buffer)
    delete[] buffer;

  ASSERT(len > 0);
  return len;
}

StringX PWSUtil::DeDupString(StringX &in_string)
{
  // Size of input string
  const size_t len = in_string.length();

  // Create output string
  StringX out_string;

  // It will never be longer than the input string
  out_string.reserve(len);
  const charT *c = in_string.c_str();

  // Cycle through characters - only appending if not already there
  for (size_t i = 0; i < len; i++) {
    if (out_string.find_first_of(c) == StringX::npos) {
      out_string.append(c, 1);
    }
    c++;
  }
  return out_string;
}

stringT PWSUtil::GetSafeXMLString(const StringX &sxInString)
{
  stringT retval(_T(""));
  ostringstreamT os;

  StringX::size_type p = sxInString.find(_T("]]>")); // special handling required
  if (p == StringX::npos) {
    // common case
    os << "<![CDATA[" << sxInString << "]]>";
  } else {
    // value has "]]>" sequence(s) that need(s) to be escaped
    // Each "]]>" splits the field into two CDATA sections, one ending with
    // ']]', the other starting with '>'
    const StringX value = sxInString;
    size_t from = 0, to = p + 2;
    do {
      StringX slice = value.substr(from, (to - from));
      os << "<![CDATA[" << slice << "]]><![CDATA[";
      from = to;
      p = value.find(_T("]]>"), from); // are there more?
      if (p == StringX::npos) {
        to = value.length();
        slice = value.substr(from, (to - from));
      } else {
        to = p + 2;
        slice = value.substr(from, (to - from));
        from = to;
        to = value.length();
      }
      os <<  slice << "]]>";
    } while (p != StringX::npos);
  }
  retval = os.str().c_str();
  return retval;
}

bool operator==(const std::string& str1, const stringT& str2)
{
    CUTF8Conv conv;
    StringX xstr;
    VERIFY( conv.FromUTF8( reinterpret_cast<const unsigned char*>(str1.data()), str1.size(), xstr) );
    return stringx2std(xstr) == str2;
}

