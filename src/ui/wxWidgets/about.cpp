/*
 * Copyright (c) 2003-2016 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */

/** \file about.cpp
*
*/
// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include <wx/url.h>


////@begin includes
////@end includes

#include "about.h"
#include "version.h"
#include "passwordsafeframe.h"
#include "core/CheckVersion.h"

#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>
#endif

////@begin XPM images
#include "graphics/cpane.xpm"
////@end XPM images


/*!
 * CAbout type definition
 */

IMPLEMENT_CLASS( CAbout, wxDialog )


/*!
 * CAbout event table definition
 */

BEGIN_EVENT_TABLE( CAbout, wxDialog )

  EVT_HYPERLINK( ID_SITEHYPERLINK, CAbout::OnVisitSiteClicked )
  EVT_BUTTON( wxID_CLOSE, CAbout::OnCloseClick )

END_EVENT_TABLE()


/*!
 * CAbout constructors
 */

CAbout::CAbout()
{
  Init();
}

CAbout::CAbout( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
  Init();
  Create(parent, id, caption, pos, size, style);
}


/*!
 * CAbout creator
 */

bool CAbout::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
  SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
  wxDialog::Create( parent, id, caption, pos, size, style );

  CreateControls();
  if (GetSizer())
  {
    GetSizer()->SetSizeHints(this);
    // currently (wx 3.0.2 GTK+) after SetSizeHints() style flags are ignored
    // and maximize/minimize buttons reappear, so we need to force max size
    // to remove maximize and minimize buttons
    if (! (style & wxMAXIMIZE_BOX)) {
      SetMaxSize(GetSize());
    }
  }
  Centre();
  return true;
}


/*!
 * CAbout destructor
 */

CAbout::~CAbout()
{
////@begin CAbout destruction
////@end CAbout destruction
}


/*!
 * Member initialization
 */

void CAbout::Init()
{
  m_newVerStatus = NULL;
}


/*!
 * Control creation for CAbout
 */

void CAbout::CreateControls()
{
  CAbout* aboutDialog = this;

  wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
  aboutDialog->SetSizer(mainSizer);

  wxStaticBitmap* logoBitmap = new wxStaticBitmap(aboutDialog, wxID_STATIC, aboutDialog->GetBitmapResource(L"./graphics/cpane.xpm"), wxDefaultPosition, wxDefaultSize, 0);
  mainSizer->Add(logoBitmap, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
  mainSizer->Add(rightSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* versionStaticText = new wxStaticText(aboutDialog, wxID_VERSIONSTR, _("Lumimaja")+L" vx.yy (abcd)", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
  rightSizer->Add(versionStaticText, 0, wxALIGN_LEFT|wxALL, 5);

  wxBoxSizer* visitSiteSizer = new wxBoxSizer(wxHORIZONTAL);
  rightSizer->Add(visitSiteSizer, 0, wxALIGN_LEFT|wxALL, 0);

  wxStaticText* visitSiteStaticTextBegin = new wxStaticText(aboutDialog, wxID_STATIC, _("Please visit the "), wxDefaultPosition, wxDefaultSize, 0);
  visitSiteSizer->Add(visitSiteStaticTextBegin, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxHyperlinkCtrl* visitSiteHyperlinkCtrl = new wxHyperlinkCtrl(aboutDialog, ID_SITEHYPERLINK, _("Lumimaja website"), L"https://samifar.in/", wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
  visitSiteSizer->Add(visitSiteHyperlinkCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* visitSiteStaticTextEnd = new wxStaticText(aboutDialog, wxID_STATIC, _("See LICENSE for open source details."), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
  rightSizer->Add(visitSiteStaticTextEnd, 0, wxALIGN_LEFT|wxALL, 5);

  wxStaticText* copyrightStaticText = new wxStaticText(aboutDialog, wxID_STATIC, _("Copyright © 2003-2016 by Rony Shapiro, 2015-2020 by Sami Farin"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
  rightSizer->Add(copyrightStaticText, 0, wxALIGN_LEFT|wxALL, 5);

  m_newVerStatus = new wxTextCtrl(aboutDialog, ID_TEXTCTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxNO_BORDER); //wxSize(aboutDialog->ConvertDialogToPixels(wxSize(120, -1)).x, -1)
  m_newVerStatus->SetBackgroundColour(wxColour(230, 231, 232));
  rightSizer->Add(m_newVerStatus, 0, wxALIGN_LEFT|wxALL|wxEXPAND|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5);
  m_newVerStatus->Hide();

  wxButton* closeButton = new wxButton(aboutDialog, wxID_CLOSE, _("&Close"), wxDefaultPosition, wxDefaultSize, 0);
  rightSizer->Add(closeButton, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  const wxString vstring = pwsafeAppName + L" " + pwsafeVersionString;
  versionStaticText->SetLabel(vstring);
}


/*!
 * Should we show tooltips?
 */

bool CAbout::ShowToolTips()
{
  return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap CAbout::GetBitmapResource( const wxString& name )
{
  // Bitmap retrieval
  if (name == L"./graphics/cpane.xpm")
  {
    wxBitmap bitmap(cpane_xpm);
    return bitmap;
  }
  return wxNullBitmap;
}

/*!
 * Get icon resources
 */

wxIcon CAbout::GetIconResource( const wxString& WXUNUSED(name) )
{
  // Icon retrieval
////@begin CAbout icon retrieval
  return wxNullIcon;
////@end CAbout icon retrieval
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */

void CAbout::OnCloseClick( wxCommandEvent& WXUNUSED(event) )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE in CAbout.
  // Before editing this code, remove the block markers.
  EndModal(wxID_CLOSE);
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE in CAbout.
}

void CAbout::OnVisitSiteClicked(wxHyperlinkEvent& event) {
  // Do nothing to prevent double open, because GTK control opens URL by itself,
  // otherwise default handler will call xdg-open to open URL
#ifndef __WXGTK__
  // skip this hook and leave default processing for non-GTK builds
  event.Skip();
#else
  wxUnusedVar(event);
#endif
}
