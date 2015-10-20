/*
 * Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */

/** \file pwsclip.h
 *
 * Small wrapper for Clipboard operations
 */

#ifndef _PWSCLIP_H_
#define _PWSCLIP_H_
#include "core/StringX.h"
class PWSclipboard
{
public:
  static PWSclipboard *self; //*< singleton pointer
  static PWSclipboard *GetInstance();
  static void DeleteInstance();
  bool SetData(const StringX &data);
  bool ClearData();
#if defined(__X__) || defined(__WXGTK__)
  void UsePrimarySelection(bool primary, bool clearOnChange=true);
#endif
private:
  PWSclipboard();
  ~PWSclipboard() {};
  PWSclipboard(const PWSclipboard &);
  PWSclipboard &operator=(const PWSclipboard &);
  void GenHash(const unsigned char* in, size_t inlen, unsigned char *out,
               size_t outlen);

  bool m_set;//<* true if we stored our data
  unsigned char m_digest[32];//*< our data hash
  wxMutex m_clipboardMutex;//*< mutex for clipboard access
};

#endif /* _PWSCLIP_H_ */

