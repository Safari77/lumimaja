/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#ifndef __WXMESSAGES_H
#define __WXMESSAGES_H

#include "core/Proxy.h"
#include <wx/msgdlg.h>

class wxAsker : public Asker
{
  // asker get untranslated messages when called from core, so it should translate them
  bool operator()(const std::wstring &question) {
    wxMessageDialog dlg(NULL, wxGetTranslation(question.c_str()), _("Lumimaja"),
                        wxYES_NO | wxICON_QUESTION | wxNO_DEFAULT);
    return dlg.ShowModal() == wxID_YES;
  }
  bool operator()(const std::wstring &title, const std::wstring &question) {
    wxMessageDialog dlg(NULL, wxGetTranslation(question.c_str()), wxGetTranslation(title.c_str()),
                        wxYES_NO | wxICON_QUESTION | wxNO_DEFAULT);
    return dlg.ShowModal() == wxID_YES;
  }
};

class wxReporter : public Reporter
{
  // reporter get untranslated messages when called from core, so it should translate them
  void operator()(const std::wstring &message) {
    wxMessageDialog dlg(NULL, wxGetTranslation(message.c_str()), _("Lumimaja"),
                        wxOK | wxICON_EXCLAMATION);
    dlg.ShowModal();
  }
  void operator()(const std::wstring &title, const std::wstring &message) {
    wxMessageDialog dlg(NULL, wxGetTranslation(message.c_str()), wxGetTranslation(title.c_str()),
                        wxOK | wxICON_EXCLAMATION);
    dlg.ShowModal();
  }
};

#endif /* __WXMESSAGES_H */
