/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/*
 * Declaration of utility functions that parse the two small
 * 'languages' used for 'autotype' and 'run' command processing.
 */

#ifndef __PWSAUXPARSE_H
#define __PWSAUXPARSE_H

#include "StringX.h"
#include "ItemData.h"
#include "PWScore.h"

#define DEFAULT_AUTOTYPE _T("\\u\\t\\p\\n")

namespace PWSAuxParse {
  // Call following with NULL ci and/or empty sxCurrentDB
  // will only validate the run command (non-empty serrmsg means
  // parse failed, reason in same).
  StringX GetExpandedString(const StringX &sxRun_Command,
                            const StringX &sxCurrentDB,
                            const CItemData *pci, const CItemData *pbci,
                            bool &bAutoType,
                            StringX &sxAutotype, stringT &serrmsg,
                            StringX::size_type &st_column,
                            bool &bURLSpecial);

  StringX GetAutoTypeString(const StringX &sxAutoCmd,
                            const StringX &sxgroup, const StringX &sxtitle,
                            const StringX &sxuser,  const StringX &sxpwd,
                            const StringX &sxnotes, const StringX &sx_url,
                            const StringX &sx_email,
                            std::vector<size_t> &vactionverboffsets);
  StringX GetAutoTypeString(const CItemData &ci,
                            const PWScore &core,
                            std::vector<size_t> &vactionverboffsets);
  // Do some runtime parsing (mainly delay commands) and send it to PC
  // as keystrokes:
  void SendAutoTypeString(const StringX &sx_autotype,
                          const std::vector<size_t> &vactionverboffsets);
}

#endif /* __PWSAUXPARSE_H */
