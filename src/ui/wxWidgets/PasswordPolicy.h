/*
 * Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */

/** \file PasswordPolicy.h
* 
*/


#ifndef _PASSWORDPOLICY_H_
#define _PASSWORDPOLICY_H_


/*!
 * Includes
 */

////@begin includes
#include "wx/valgen.h"
#include "wx/spinctrl.h"
////@end includes
#include "core/coredefs.h"
#include "core/PWPolicy.h"
#include "core/PWScore.h"

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxGridSizer;
class wxBoxSizer;
class wxSpinCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

#ifndef wxDIALOG_MODAL
#define wxDIALOG_MODAL 0
#endif

////@begin control identifiers
#define ID_CPASSWORDPOLICY 10221
#define ID_POLICYNAME 10223
#define ID_PWLENSB 10117
#define ID_CHECKBOX3 10118
#define ID_CHECKBOX4 10119
#define ID_CHECKBOX5 10120
#define ID_CHECKBOX6 10121
#define IDC_OWNSYMBOLS 10212
#define ID_RESET_SYMBOLS 10113
#define ID_CHECKBOX8 10123
#define SYMBOL_CPASSWORDPOLICY_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX|wxDIALOG_MODAL|wxTAB_TRAVERSAL
#define SYMBOL_CPASSWORDPOLICY_TITLE _("Password Policy")
#define SYMBOL_CPASSWORDPOLICY_IDNAME ID_CPASSWORDPOLICY
#define SYMBOL_CPASSWORDPOLICY_SIZE wxSize(400, 300)
#define SYMBOL_CPASSWORDPOLICY_POSITION wxDefaultPosition
////@end control identifiers


/*!
 * CPasswordPolicy class declaration
 */

class CPasswordPolicy: public wxDialog
{    
  DECLARE_EVENT_TABLE()

public:
  /// Constructors
  CPasswordPolicy( wxWindow* parent, PWScore &core,
                   const PSWDPolicyMap &polmap,
                   wxWindowID id = SYMBOL_CPASSWORDPOLICY_IDNAME,
                   const wxString& caption = SYMBOL_CPASSWORDPOLICY_TITLE,
                   const wxPoint& pos = SYMBOL_CPASSWORDPOLICY_POSITION,
                   const wxSize& size = SYMBOL_CPASSWORDPOLICY_SIZE,
                   long style = SYMBOL_CPASSWORDPOLICY_STYLE );

  /// Creation
  bool Create( wxWindow* parent, wxWindowID id = SYMBOL_CPASSWORDPOLICY_IDNAME, const wxString& caption = SYMBOL_CPASSWORDPOLICY_TITLE, const wxPoint& pos = SYMBOL_CPASSWORDPOLICY_POSITION, const wxSize& size = SYMBOL_CPASSWORDPOLICY_SIZE, long style = SYMBOL_CPASSWORDPOLICY_STYLE );

  /// Destructor
  ~CPasswordPolicy();

  /// Initialises member variables
  void Init();

  /// Creates the controls and sizers
  void CreateControls();

////@begin CPasswordPolicy event handler declarations

  /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX3
  void OnPwPolUseLowerCase( wxCommandEvent& event );

  /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX4
  void OnPwPolUseUpperCase( wxCommandEvent& event );

  /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX5
  void OnPwPolUseDigits( wxCommandEvent& event );

  /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX6
  void OnPwPolUseSymbols( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RESET_SYMBOLS
  void OnResetSymbolsClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX7
  void OnEZreadCBClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX8
  void OnPronouceableCBClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
  void OnOkClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
  void OnCancelClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
  void OnHelpClick( wxCommandEvent& event );

////@end CPasswordPolicy event handler declarations

////@begin CPasswordPolicy member function declarations

  wxString GetSymbols() const { return m_Symbols ; }
  void SetSymbols(wxString value) { m_Symbols = value ; }

  wxString GetPolname() const { return m_polname ; }
  void SetPolname(wxString value) { m_polname = value ; }

  bool GetPwMakePronounceable() const { return m_pwMakePronounceable ; }
  void SetPwMakePronounceable(bool value) { m_pwMakePronounceable = value ; }

  bool GetPwUseDigits() const { return m_pwUseDigits ; }
  void SetPwUseDigits(bool value) { m_pwUseDigits = value ; }

  bool GetPwUseLowercase() const { return m_pwUseLowercase ; }
  void SetPwUseLowercase(bool value) { m_pwUseLowercase = value ; }

  bool GetPwUseSymbols() const { return m_pwUseSymbols ; }
  void SetPwUseSymbols(bool value) { m_pwUseSymbols = value ; }

  bool GetPwUseUppercase() const { return m_pwUseUppercase ; }
  void SetPwUseUppercase(bool value) { m_pwUseUppercase = value ; }

  int GetPwdefaultlength() const { return m_pwdefaultlength ; }
  void SetPwdefaultlength(int value) { m_pwdefaultlength = value ; }

  /// Retrieves bitmap resources
  wxBitmap GetBitmapResource( const wxString& name );

  /// Retrieves icon resources
  wxIcon GetIconResource( const wxString& name );
////@end CPasswordPolicy member function declarations
  void SetPolicyData(const wxString &polname, const PWPolicy &pol);
  void GetPolicyData(wxString &polname, PWPolicy &pol)
  {polname = m_polname; pol = m_st_pp;}

  /// Should we show tooltips?
  static bool ShowToolTips();

////@begin CPasswordPolicy member variables
  wxGridSizer* m_pwMinsGSzr;
  wxCheckBox* m_pwpUseLowerCtrl;
  wxCheckBox* m_pwpUseUpperCtrl;
  wxCheckBox* m_pwpUseDigitsCtrl;
  wxCheckBox* m_pwpSymCtrl;
  wxTextCtrl* m_OwnSymbols;
  wxCheckBox* m_pwpPronounceCtrl;
private:
  wxString m_Symbols;
  wxString m_polname;
  bool m_pwMakePronounceable;
  bool m_pwUseDigits;
  bool m_pwUseLowercase;
  bool m_pwUseSymbols;
  bool m_pwUseUppercase;
  int m_pwdefaultlength;
////@end CPasswordPolicy member variables
  void SetDefaultSymbolDisplay(bool restore_defaults);
  void CBox2Spin(wxCheckBox *cb, wxSpinCtrl *sp);
  bool UpdatePolicy();
  bool Verify();

  PWScore &m_core;
  const PSWDPolicyMap &m_MapPSWDPLC; // used to detect existing name
  wxString m_oldpolname;
  int m_oldpwdefaultlength;
  bool m_oldpwUseLowercase;
  bool m_oldpwUseUppercase;
  bool m_oldpwUseDigits;
  bool m_oldpwUseSymbols;
  bool m_oldpwMakePronounceable;
  wxString m_oldSymbols;
  PWPolicy m_st_pp; // The edited policy
};

#endif
  // _PASSWORDPOLICY_H_
