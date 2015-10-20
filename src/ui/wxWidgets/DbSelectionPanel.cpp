/*
 * Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */

/** \file DbSelectionPanel.cpp
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

#include "./DbSelectionPanel.h"
#include "./OpenFilePickerValidator.h"
#include "./SafeCombinationCtrl.h"
#include "./wxutils.h"
#include "../../core/PWScore.h"

#include <wx/filename.h>

#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>
#endif

DbSelectionPanel::DbSelectionPanel(wxWindow* parent, 
                                    const wxString& filePrompt,
                                    const wxString& filePickerCtrlTitle,
                                    bool autoValidate,
                                    PWScore* core,
                                    unsigned rowsep) : wxPanel(parent), m_filepicker(0),
                                                                        m_sc(0),
                                                                        m_bAutoValidate(autoValidate),
                                                                        m_core(core)
{
  wxSizerFlags borderFlags = wxSizerFlags().Border(wxLEFT|wxRIGHT, SideMargin);

  /* This doesn't work since the second Border() call overwrites all the
   * previous border values.  So now we have to insert separators by hand
   */
  //wxSizerFlags rowFlags = borderFlags.Border(wxBOTTOM, RowSeparation);

  wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
  panelSizer->AddSpacer(TopMargin);

  panelSizer->Add(new wxStaticText(this, wxID_ANY, filePrompt), borderFlags);
  panelSizer->AddSpacer(RowSeparation);
  COpenFilePickerValidator validator(m_filepath);
  m_filepicker = new wxFilePickerCtrl(this, wxID_ANY, wxEmptyString, 
                                          filePickerCtrlTitle,
                                          _("Lumimaja Databases (*.lumi3; *.dat)|*.lumi3;*.dat|Lumimaja Backups (*.bak)|*.bak|Lumimaja Intermediate Backups (*.ibak)|*.ibak|All files (*.*; *)|*.*;*"), 
                                          wxDefaultPosition, wxDefaultSize, 
                                          wxFLP_DEFAULT_STYLE | wxFLP_USE_TEXTCTRL, 
                                          validator);
  panelSizer->Add(m_filepicker, borderFlags.Expand());
  panelSizer->AddSpacer(RowSeparation*rowsep);
  m_filepicker->Connect( m_filepicker->GetEventType(), 
             wxFileDirPickerEventHandler(DbSelectionPanel::OnFilePicked),
             NULL, this);

  panelSizer->Add(new wxStaticText(this, wxID_ANY, _("Safe Combination:")), borderFlags);
  panelSizer->AddSpacer(RowSeparation);
  
  m_sc = new CSafeCombinationCtrl(this);
  m_sc->SetValidatorTarget(&m_combination);
  panelSizer->Add(m_sc, borderFlags.Expand());
  
  SetSizerAndFit(panelSizer);
  
  //The parent window must call our TransferDataToWindow and TransferDataFromWindow
  m_parent->SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);
}

DbSelectionPanel::~DbSelectionPanel()
{
}

void DbSelectionPanel::SelectCombinationText()
{
  m_sc->SelectCombinationText();
}

bool DbSelectionPanel::DoValidation()
{
  //the data has not been transferred from the window to our members yet, so get them from the controls
  if (wxWindow::Validate()) {

    wxFileName wxfn(m_filepicker->GetPath());

    //Did the user enter a valid file path
    if (!wxfn.FileExists()) {
      wxMessageBox( _("File or path not found."), _("Error"), wxOK | wxICON_EXCLAMATION, this);
      return false;
    }

    //Did he enter the same file that's currently open?
    if (wxfn.SameAs(wxFileName(towxstring(m_core->GetCurFile())))) {
      // It is the same damn file
      wxMessageBox(_("That file is already open."), _("Error"), wxOK | wxICON_WARNING, this);
      return false;
    }

    StringX combination = m_sc->GetCombination();
    const char *err;
    //Does the combination match?
    switch(m_core->CheckPasskey(tostringx(wxfn.GetFullPath()), combination)) {
    case PWScore::SUCCESS:
      return true;
    case PWScore::TRUNCATED_FILE:
      err = "Lumimaja database was truncated";;
      break;
    case PWScore::CRYPTO_ERROR:
      err = "Corrupt Lumimaja database or unsupported cipher/hash";
      break;
    case PWScore::ARGON2_FAIL:
      err = "Argon2 key derivation function failed, out of memory?";
      break;
    case PWScore::WRONG_PASSWORD:
      err = "Incorrect passkey";
      break;
    case PWScore::NOT_LUMI3_FILE:
      err = "Not Lumimaja database";
    default:
      err = "Unknown error checking passkey";
      break;
    }
    wxString errmess(_(err));
    wxMessageBox(errmess, _("Error"), wxOK | wxICON_ERROR, this);
    SelectCombinationText();
    return false;
  }
  else {
    return false;
  }
}

void DbSelectionPanel::OnFilePicked(wxFileDirPickerEvent& /* event */)
{
  // Don't shift focus if we are in the text ctrl
  if ( !wxDynamicCast(FindFocus(), wxTextCtrl) )
    m_sc->SelectCombinationText();
}
