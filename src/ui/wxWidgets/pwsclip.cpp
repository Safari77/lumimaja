/*
 * Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */

#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <sodium.h>

/*

  NOTE: In VS2013 wxWidgets 3.0.x builds:
    Both <wx/clipbrd.h> & <wx/dataobj.h> cause 51 warnings about using unsecure
    versions of standard calls, such as 'wcscpy' instead of 'wcscpy_s', if any
    previously inluded header file includes <string> even though pre-processor
    variables _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES and
    _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT are defined.

    The solution is to ensure that any header files containing <string>, e.g.
    "core/StringX.h", are placed after these two wxWidgets include statements.

  This issue did not occur with wxWidgets 2.8.12.

  For this reason, "pwsclip.h", which includes "core/StringX.h" that also includes
  <string>, is placed here after <wx/clipbrd.h> & <wx/dataobj.h>.

*/

#include "pwsclip.h"
#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>
#endif

PWSclipboard *PWSclipboard::self = nullptr;

/**
 * Get pointer to single instance of clipboard manager
 */
PWSclipboard *PWSclipboard::GetInstance()
{
  if (self == nullptr) {
    self = new PWSclipboard();
  }
  return self;
}

/**
 * Destroy the instance
*/
void PWSclipboard::DeleteInstance()
{
  delete self;
  self = nullptr;
}

PWSclipboard::PWSclipboard(): m_set(false)
{
  memset(m_digest, 0, sizeof(m_digest));
}

void PWSclipboard::GenHash(const unsigned char* in, size_t inlen, unsigned char *out,
                           size_t outlen)
{
  static const unsigned char bkey[] = "Lumimaja Clipboard";

  if (crypto_generichash_blake2b(out, outlen, in, inlen,
                                 bkey, sizeof(bkey)) != 0) {
    fprintf(stderr, "blake2b fail\n");
    memset(out, 0, outlen);
  }
}

/**
 * Put text data to clipboard
 * @param[in] data data to store in clipboard
 * @param isSensitive if data sensitive, we remeber it's hash and will clear on ClearData() call
 * @return \c true, if we could open the clipboard and put the data
*/
bool PWSclipboard::SetData(const StringX &data)
{
  wxMutexLocker clip(m_clipboardMutex);

  bool res = false;
  if (wxTheClipboard->Open()) {
    res = wxTheClipboard->SetData(new wxTextDataObject(data.c_str()));
    wxTheClipboard->Close();
  }
  m_set = true;
  if (res) {
    // identify data in clipboard as ours, so as not to clear the wrong data later
    // of course, we don't want an extra copy of a password floating around
    // in memory, so we'll use the hash
    GenHash(reinterpret_cast<const unsigned char *>(data.c_str()),
            data.length()*sizeof(wchar_t), m_digest, sizeof(m_digest));
  }
  return res;
}

/**
 * Clear from clipboard data, that we put there previously
 * @return \c true, if we cleared our data, or stored data don't belong to us
*/
bool PWSclipboard::ClearData()
{
  wxMutexLocker clip(m_clipboardMutex);

  if (m_set && wxTheClipboard->Open()) {
    wxTextDataObject obj;
    if (wxTheClipboard->IsSupported(wxDF_UNICODETEXT) && wxTheClipboard->GetData(obj)) {
      StringX buf(obj.GetText().data(), obj.GetText().size());
      if (buf.length()) {
        // check if the data on the clipboard is the same we put there
        unsigned char digest[32];

        GenHash(reinterpret_cast<const unsigned char *>(buf.c_str()),
                buf.length()*sizeof(wchar_t), digest, sizeof(digest));
        if (crypto_verify_32(digest, m_digest) == 0) {
          // clear & reset
          wxTheClipboard->Clear();
          memset(m_digest, 0, sizeof(m_digest));
          m_set = false;
          // Also trash data in buffer and clipboard somehow?
          pws_os::Trace0(L"Cleared our data from buffer.\n");
        }
        else{
          pws_os::Trace0(L"Buffer doesn't contain our data. Nothing to clear.\n");
        }
      }
    }
    wxTheClipboard->Close();
  }
  return !m_set;
}

#if defined(__X__) || defined(__WXGTK__)
/**
 * Set current clipboard buffer
 * @param primary if set to \c true, will use PRIMARY selection, otherwise CLIPBOARD X11
 * @param clearOnChange if set to \c true, our previous data will be cleared from previous buffer
 */
void PWSclipboard::UsePrimarySelection(bool primary, bool clearOnChange) {
  if (primary != wxTheClipboard->IsUsingPrimarySelection()) {
    if (clearOnChange)
      ClearData();
    wxTheClipboard->UsePrimarySelection(primary);
  }
}
#endif
