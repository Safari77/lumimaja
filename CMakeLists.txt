# vim: paste
cmake_minimum_required (VERSION 3.0)
project (lumimaja CXX C)
set (lumimaja_VERSION_MAJOR 0)
set (lumimaja_VERSION_MINOR 98)
option(DEBUG "Debug symbols and AddressSanitize")

set(INSTALL_BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Installation directory for executables")
include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)
enable_testing()

check_include_file(stdint.h    HAVE_STDINT_H)

if(NOT NO_YUBI)
  CHECK_LIBRARY_EXISTS(ykpers-1 yk_init "ykpers-1/ykpers.h" HAVE_YKPERS_H)
endif(NOT NO_YUBI)
CHECK_LIBRARY_EXISTS(sodium sodium_init "sodium.h" HAVE_SODIUM_H)

set(wxWidgets_CONFIGURATION mswu)
set(wxWidgets_CONFIG_EXECUTABLE wx-config-3.0)
find_package(wxWidgets COMPONENTS adv base core html REQUIRED)
include(${wxWidgets_USE_FILE})

include(FindXercesC)
find_package(XercesC REQUIRED)
include_directories( ${XercesC_INCLUDE_DIR} )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -DUSE_XML_LIBRARY=XERCES -DWCHAR_INCOMPATIBLE_XMLCH ${CMAKE_WXWINDOWS_CXX_FLAGS}")

if(DEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-4 -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address")
endif(DEBUG)

if(NOT HAVE_YKPERS_H)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_YUBI")
  message(STATUS "Yubikey support disabled")
endif(NOT HAVE_YKPERS_H)

configure_file (
  ${PROJECT_SOURCE_DIR}/src/ui/wxWidgets/version.h.in
  ${PROJECT_BINARY_DIR}/version.h
)

include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/src"
  "${PROJECT_SOURCE_DIR}/src/core"
  "${PROJECT_SOURCE_DIR}/src/ui/wxWidgets")

set(CMAKE_REQUIRED_LIBRARIES sodium argon2)
CHECK_CXX_SOURCE_COMPILES("
#include <string.h>
#include <argon2.h>
#include <iostream>

int main(int argc, char *argv[])
{
  unsigned char out[32];
  unsigned char salt[32] = {1, 2 };
  int ret;
  Argon2_Context ctx(out, sizeof(out), reinterpret_cast<uint8_t*>(argv[0]), strlen(argv[0]),
                     salt, sizeof(salt), NULL, 0, NULL, 0,
                     1, 1024, 4, 4,
                     NULL, NULL, true, false, true, false);
  ret = Argon2d(&ctx);
  if (ret != ARGON2_OK) {
    std::cout << \"FAIL\" << std::endl;
    return 1;
  }
  return 0;
}
" HAVE_ARGON2_H)
# FAIL_REGEX is for output _during_ compiling :(

if(NOT HAVE_ARGON2_H)
  message( FATAL_ERROR "Please install Argon2 from https://github.com/khovratovich/Argon2.git , directory Source/C++11" )
endif(NOT HAVE_ARGON2_H)

if(NOT HAVE_SODIUM_H)
  message( FATAL_ERROR "Please install libsodium from https://github.com/jedisct1/libsodium" )
endif(NOT HAVE_SODIUM_H)

if(HAVE_YKPERS_H)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/include/ykpers-1")
  list(APPEND CMAKE_REQUIRED_LIBRARIES ykpers-1)
  list(APPEND LUMIMAJA_SRCS src/os/linux/PWYubi.cpp src/ui/wxWidgets/yubicfg.cpp src/ui/wxWidgets/YubiMixin.cpp)
  list(APPEND LUMIMAJA_HDRS src/os/linux/PWYubi.h src/ui/wxWidgets/yubicfg.h src/ui/wxWidgets/YubiMixin.h)
endif(HAVE_YKPERS_H)

list(APPEND LUMIMAJA_HDRS
    src/core/Util.h
    src/core/ItemData.h
    src/core/PWSrand.h
    src/core/PwsPlatform.h
    src/core/PWSfileV3.h
    src/core/coredefs.h
    src/core/PWScore.h
    src/core/PWSAuxParse.h
    src/core/PWCharPool.h
    src/core/PWSprefs.h
    src/core/core.h
    src/core/CheckVersion.h
    src/core/DBCompareData.h
    src/core/ExpiredList.h
    src/core/Match.h
    src/core/PWHistory.h
    src/core/PWSFilters.h
    src/core/PWSLog.h
    src/core/Proxy.h
    src/core/Report.h
    src/core/StringXStream.h
    src/core/SysInfo.h
    src/core/UIinterface.h
    src/core/UTF8Conv.h
    src/core/trigram.h
    src/core/ItemField.h
    src/core/PWSdirs.h
    src/core/StringX.h
    src/core/XMLprefs.h
    src/core/PWSfile.h
    src/core/VerifyFormat.h
    src/core/UnknownField.h
    src/core/PWPolicy.h
    src/core/Command.h
    src/core/CommandInterface.h
    src/core/XML/Xerces/XFileSAX2Handlers.h
    src/core/XML/Xerces/XFileValidator.h
    src/core/XML/Xerces/XFileXMLProcessor.h
    src/core/XML/Xerces/XFilterSAX2Handlers.h
    src/core/XML/Xerces/XFilterXMLProcessor.h
    src/core/XML/Xerces/XMLChConverter.h
    src/core/XML/Xerces/XSecMemMgr.h
    src/core/XML/XMLDefs.h
    src/core/XML/XMLFileHandlers.h
    src/core/XML/XMLFileValidation.h
    src/os/rand.h
    src/os/file.h
    src/os/typedefs.h
    src/os/KeySend.h
    src/os/UUID.h
    src/os/debug.h
    src/os/dir.h
    src/os/env.h
    src/os/funcwrap.h
    src/os/lib.h
    src/os/logit.h
    src/os/mem.h
    src/os/pws_str.h
    src/os/pws_tchar.h
    src/os/registry.h
    src/os/run.h
    src/os/sleep.h
    src/os/utf8conv.h
    src/os/linux/xsendstring.h
    src/os/linux/unicode2keysym.h
    src/os/linux/pws_str.h
    src/os/linux/pws_time.h
    src/ui/wxWidgets/pwsafeapp.h
    src/ui/wxWidgets/passwordsafeframe.h
    src/ui/wxWidgets/about.h
    src/ui/wxWidgets/optionspropsheet.h
    src/ui/wxWidgets/PasswordSafeSearch.h
    src/ui/wxWidgets/ViewReport.h
    src/ui/wxWidgets/guiinfo.h
    src/ui/wxWidgets/helpmap.h
    src/ui/wxWidgets/wxMessages.h
    src/ui/wxWidgets/pwsmenushortcuts.h
    src/ui/wxWidgets/safecombinationchange.h
    src/ui/wxWidgets/safecombinationentry.h
    src/ui/wxWidgets/safecombinationsetup.h
    src/ui/wxWidgets/addeditpropsheet.h
    src/ui/wxWidgets/properties.h
    src/ui/wxWidgets/pwsclip.h
    src/ui/wxWidgets/createshortcutdlg.h
    src/ui/wxWidgets/deleteconfirmation.h
    src/ui/wxWidgets/dragbar.h
    src/ui/wxWidgets/editshortcut.h
    src/ui/wxWidgets/fieldselectiondlg.h
    src/ui/wxWidgets/fieldselectionpanel.h
    src/ui/wxWidgets/wxutils.h
    src/ui/wxWidgets/AdvancedSelectionDlg.h
    src/ui/wxWidgets/CompareDlg.h
    src/ui/wxWidgets/ComparisonGridTable.h
    src/ui/wxWidgets/DbSelectionPanel.h
    src/ui/wxWidgets/ExportTextWarningDlg.h
    src/ui/wxWidgets/ExternalKeyboardButton.h
    src/ui/wxWidgets/ImportTextDlg.h
    src/ui/wxWidgets/ImportXmlDlg.h
    src/ui/wxWidgets/ManagePwdPolicies.h
    src/ui/wxWidgets/MergeDlg.h
    src/ui/wxWidgets/OpenFilePickerValidator.h
    src/ui/wxWidgets/PWSDragBar.h
    src/ui/wxWidgets/PWSgrid.h
    src/ui/wxWidgets/PWSgridtable.h
    src/ui/wxWidgets/PWStree.h
    src/ui/wxWidgets/PasswordPolicy.h
    src/ui/wxWidgets/PwsSync.h
    src/ui/wxWidgets/PwsToolbarButtons.h
    src/ui/wxWidgets/RUEList.h
    src/ui/wxWidgets/RecentDBList.h
    src/ui/wxWidgets/SafeCombinationCtrl.h
    src/ui/wxWidgets/SelectionCriteria.h
    src/ui/wxWidgets/SizeRestrictedPanel.h
    src/ui/wxWidgets/SystemTray.h
    src/ui/wxWidgets/SystemTrayMenuId.h
    src/ui/wxWidgets/safecombinationprompt.h
    src/core/core_st.h
)


list(APPEND LUMIMAJA_SRCS
    src/core/Util.cpp
    src/core/PWSdirs.cpp
    src/core/PWCharPool.cpp
    src/core/Command.cpp
    src/core/ExpiredList.cpp
    src/core/Match.cpp
    src/core/PWHistory.cpp
    src/core/PWSLog.cpp
    src/core/Report.cpp
    src/core/StringX.cpp
    src/core/SysInfo.cpp
    src/core/UTF8Conv.cpp
    src/core/UnknownField.cpp
    src/core/PWSprefs.cpp
    src/core/CoreImpExp.cpp
    src/core/PWSFilters.cpp
    src/core/ItemData.cpp
    src/core/PWSAuxParse.cpp
    src/core/XMLprefs.cpp
    src/core/CheckVersion.cpp
    src/core/PWSfile.cpp
    src/core/PWScore.cpp
    src/core/PWSfileV3.cpp
    src/core/PWSrand.cpp
    src/core/VerifyFormat.cpp
    src/core/PWPolicy.cpp
    src/core/CoreOtherDB.cpp
    src/core/pugixml/pugixml.cpp
    src/core/ItemField.cpp
    src/core/XML/Xerces/XFileSAX2Handlers.cpp
    src/core/XML/Xerces/XFileValidator.cpp
    src/core/XML/Xerces/XFileXMLProcessor.cpp
    src/core/XML/Xerces/XFilterSAX2Handlers.cpp
    src/core/XML/Xerces/XFilterXMLProcessor.cpp
    src/core/XML/Xerces/XSecMemMgr.cpp
    src/core/XML/XMLFileHandlers.cpp
    src/core/XML/XMLFileValidation.cpp
    src/os/linux/rand.cpp
    src/os/linux/file.cpp
    src/os/linux/xsendstring.cpp
    src/os/linux/sleep.cpp
    src/os/linux/unicode2keysym.cpp
    src/os/linux/utf8conv.cpp
    src/os/linux/debug.cpp
    src/os/linux/dir.cpp
    src/os/linux/KeySend.cpp
    src/os/linux/UUID.cpp
    src/os/linux/env.cpp
    src/os/linux/logit.cpp
    src/os/linux/mem.cpp
    src/os/linux/pws_str.cpp
    src/os/linux/pws_time.cpp
    src/os/linux/registry.cpp
    src/os/linux/run.cpp
    src/ui/wxWidgets/optionspropsheet.cpp
    src/ui/wxWidgets/CompareDlg.cpp
    src/ui/wxWidgets/ImportTextDlg.cpp
    src/ui/wxWidgets/addeditpropsheet.cpp
    src/ui/wxWidgets/PasswordSafeSearch.cpp
    src/ui/wxWidgets/SystemTray.cpp
    src/ui/wxWidgets/about.cpp
    src/ui/wxWidgets/mainManage.cpp
    src/ui/wxWidgets/guiinfo.cpp
    src/ui/wxWidgets/version.cpp
    src/ui/wxWidgets/pwsclip.cpp
    src/ui/wxWidgets/safecombinationentry.cpp
    src/ui/wxWidgets/mainEdit.cpp
    src/ui/wxWidgets/ViewReport.cpp
    src/ui/wxWidgets/safecombinationprompt.cpp
    src/ui/wxWidgets/passwordsafeframe.cpp
    src/ui/wxWidgets/properties.cpp
    src/ui/wxWidgets/createshortcutdlg.cpp
    src/ui/wxWidgets/deleteconfirmation.cpp
    src/ui/wxWidgets/dragbar.cpp
    src/ui/wxWidgets/editshortcut.cpp
    src/ui/wxWidgets/fieldselectiondlg.cpp
    src/ui/wxWidgets/fieldselectionpanel.cpp
    src/ui/wxWidgets/pwsmenushortcuts.cpp
    src/ui/wxWidgets/safecombinationchange.cpp
    src/ui/wxWidgets/ExternalKeyboardButton.cpp
    src/ui/wxWidgets/PasswordPolicy.cpp
    src/ui/wxWidgets/DbSelectionPanel.cpp
    src/ui/wxWidgets/pwsafeapp.cpp
    src/ui/wxWidgets/safecombinationsetup.cpp
    src/ui/wxWidgets/wxutils.cpp
    src/ui/wxWidgets/AdvancedSelectionDlg.cpp
    src/ui/wxWidgets/ComparisonGridTable.cpp
    src/ui/wxWidgets/ExportTextWarningDlg.cpp
    src/ui/wxWidgets/ImportXmlDlg.cpp
    src/ui/wxWidgets/ManagePwdPolicies.cpp
    src/ui/wxWidgets/MergeDlg.cpp
    src/ui/wxWidgets/OpenFilePickerValidator.cpp
    src/ui/wxWidgets/PWSDragBar.cpp
    src/ui/wxWidgets/PWSgrid.cpp
    src/ui/wxWidgets/PWSgridtable.cpp
    src/ui/wxWidgets/PWStree.cpp
    src/ui/wxWidgets/PwsSync.cpp
    src/ui/wxWidgets/RUEList.cpp
    src/ui/wxWidgets/SafeCombinationCtrl.cpp
    src/ui/wxWidgets/SelectionCriteria.cpp
    src/ui/wxWidgets/SizeRestrictedPanel.cpp
    src/core/core_st.cpp
)

#add_custom_command(
#  PRE_BUILD
#  OUTPUT core_st.cpp
#  OUTPUT core_st.h
#  COMMAND ${PROJECT_SOURCE_DIR}/src/core/rc2cpp.pl ${PROJECT_SOURCE_DIR}/src/core/core.rc2
#  DEPENDS src/core/core.rc2
#)
#set_property(SOURCE src/core/StringX.cpp APPEND PROPERTY OBJECT_DEPENDS core_st.h core_st.cpp)
# XXX

add_executable(lumimaja ${LUMIMAJA_SRCS})
target_link_libraries(lumimaja ${wxWidgets_LIBRARIES} uuid Xtst X11 ${CMAKE_REQUIRED_LIBRARIES} ${XercesC_LIBRARY})

