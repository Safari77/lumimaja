/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// pws_platform.h
//------------------------------------------------------------------------------
//
// This header file exists to determine, at compile time, which target we're
// compiling to and to make the appropriate #defines and #includes.
//
// The following macros are defined:
//
//    PWS_BIG_ENDIAN    - Defined only if the target CPU is big-endian.
//    PWS_LITTLE_ENDIAN - Defined only if the target CPU is little-endian.
//    PWS_PLATFORM      - A string, the target platform, e.g. "Pocket PC".
//    PWS_PLATFORM_EX   - A string, the target platform, e.g. "Pocket PC 2000".
//    POCKET_PC         - Defined only if target is Pocket PC 2000 or later.
//    POCKET_PC_VER     - Defined only if target is Pocket PC 2000 or later.
//
// Notes:
//
// 1. PWS_BIG_ENDIAN and PWS_LITTLE_ENDIAN are mutually exclusive.
// 2. PWS_BIG_ENDIAN and PWS_LITTLE_ENDIAN may be defined on the complier
//    command line but checks are made to ensure that one and only one is
//    defined.
//
// Supported Configurations:
// -------------------------
//
// Windows
//
//    Win32 X86
//    Win32 and Win64 x64 (unofficial)
//
// Linux
// MacOS (unofficial)
// FreeBSD (unofficial)

#ifndef __PWSPLATFORM_H
#define __PWSPLATFORM_H

#if (defined(_WIN32) || defined (_WIN64))
// Make sure Windows.h does not define min & max macros
#define NOMINMAX
#endif

#if (defined(_WIN32) || defined (_WIN64)) && !defined(__WX__)
// ONLY place in core which refers to parent. Ugh.
#include "../ui/Windows/stdafx.h"
#else
// some globally useful includes for non-Windows
#include <cassert>
#endif

#ifndef _MSC_VER
#include <stdint.h>
#endif

#undef PWS_PLATFORM
#undef POCKET_PC

#if defined(_WIN32)
#ifdef BIG_ENDIAN
#define PWS_BIG_ENDIAN
#endif
#ifdef LITTLE_ENDIAN
#define PWS_LITTLE_ENDIAN
#endif
#endif

// PWS_BIG_ENDIAN and PWS_LITTLE_ENDIAN can be specified on the
#if defined(PWS_BIG_ENDIAN)
#undef PWS_BIG_ENDIAN
#define PWS_BIG_ENDIAN
#endif

#if defined(PWS_LITTLE_ENDIAN)
#undef PWS_LITTLE_ENDIAN
#define PWS_LITTLE_ENDIAN
#endif

// **********************************************
// * Windows 32                                 *
// **********************************************
#if defined(_WIN32)
#if defined(x86) || defined(_x86) || defined(_X86) || defined(_X86_) || defined(_M_IX86) || defined(_M_X64)
#define PWS_PLATFORM "Windows"
#define PWS_LITTLE_ENDIAN
#endif
// **********************************************
// * Linux on Intel                             *
// **********************************************
#elif defined(__linux)
#define PWS_PLATFORM "Linux"
#if defined(__i386__) || defined(__i486__) || defined(__x86_64__)
#define PWS_LITTLE_ENDIAN
#endif
// **********************************************
// * cygwin on Intel                             *
// **********************************************
#elif defined(__CYGWIN__)
#define PWS_PLATFORM "Cygwin"
#if defined(__i386__) || defined(__i486__)
#define PWS_LITTLE_ENDIAN
#endif
/* http://predef.sourceforge.net/preos.html*/
#elif defined (macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__)
#define PWS_PLATFORM "Mac"
#define __PWS_MACINTOSH__
/* following line was added to avoid "Impossible constraint in 'asm'" errors in ROLc()
   and RORc() functions below, just like it is defined for Linux and cygwin above */
#if defined(__APPLE__) && defined(__MACH__)
#define PWS_PLATFORM_EX MacOSX
#else
#define PWS_PLATFORM_EX MacOS9
#endif
/* gcc shipped with snow leopard defines this.  Try "cpp -dM dummy.h"*/
#if defined (__LITTLE_ENDIAN__) && (__LITTLE_ENDIAN__ == 1)
#define PWS_LITTLE_ENDIAN
#else
#define PWS_BIG_ENDIAN
#endif
// **********************************************
// * FreeBSD on Intel                           *
// **********************************************
#elif defined(__FreeBSD) || defined(__FreeBSD__)
#define PWS_PLATFORM "FreeBSD"
#if defined(__i386__) || defined(__amd64__)
#define PWS_LITTLE_ENDIAN
#endif
// **********************************************
// * Add other platforms here...                *
// **********************************************
#endif

//
#if !defined(PWS_PLATFORM)
#error Unable to determine the target platform - please fix PwsPlatform.h
#endif

#if !defined(PWS_LITTLE_ENDIAN) && !defined(PWS_BIG_ENDIAN)
#error Cannot determine whether the target CPU is big or little endian - please fix PwsPlatform.h
#endif

#if defined(PWS_BIG_ENDIAN) && defined(PWS_LITTLE_ENDIAN)
#error Both PWS_BIG_ENDIAN and PWS_LITTLE_ENDIAN are defined, only one should be defined.
#endif

/* fix for MSVC ...evil! */
#ifdef _MSC_VER
#define CONST64(n) n ## ui64
typedef unsigned __int64 ulong64;
#else
#define CONST64(n) n ## ULL
typedef uint64_t ulong64;
#endif

#define NumberOf(array) ((sizeof array) / sizeof(array[0]))

#if !defined(_MFC_VER) && !defined(_WIN32)
#define UNREFERENCED_PARAMETER(P) (void)(P)
#endif

#endif /* __PWSPLATFORM_H */
