/*
 * Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */

/** \file version.cpp
 * 
 */

#include "version.h"

#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>
#endif

const wxString pwsafeAppName(APPNAME);
#ifndef _DEBUG
const wxString pwsafeVersionString = wxString::Format(wxString(_T("v%d.%d %ls")),
                                                      MAJORVERSION, MINORVERSION, SPECIALBUILD);
#else
const wxString pwsafeVersionString = wxString::Format(wxString(_T("v%d.%d [debug] %ls")),
                                                      MAJORVERSION, MINORVERSION, SPECIALBUILD);
#endif
