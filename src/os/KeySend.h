/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

// 
// KeySend.h
// Originally Windows-only, now implemented for both Windows and Linux
// Original version by thedavecollins
// Windows implementation updated by dk
// Linux implementation by sauravg
//-----------------------------------------------------------------------------

#include "typedefs.h"
#include "../core/StringX.h"

class CKeySendImpl; // for os-specific stuff

class CKeySend
{
public:
  CKeySend(bool bForceOldMethod = false, unsigned defaultDelay = 10); // bForceOldMethod's Windows-specific
  ~CKeySend();
  void SendString(const StringX &data);
  void ResetKeyboardState() const;
  void SetDelay(unsigned d);
  void SetAndDelay(unsigned d);
  bool isCapsLocked() const ;
  void SetCapsLock(bool bstate);
  void BlockInput(bool bi) const ;
#ifdef __PWS_MACINTOSH__  
  bool SimulateApplicationSwitch();
#endif  
  void SelectAll() const;
  void EmulateMods(bool emulate);
  bool IsEmulatingMods() const;
private:
  unsigned m_delayMS; //delay between keystrokes in milliseconds
  CKeySendImpl *m_impl;
};

