/*
* Copyright (c) 2008-2010, Helios (helios.vmg@gmail.com)
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice,
*       this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * The name of the author may not be used to endorse or promote products
*       derived from this software without specific prior written permission.
*     * Products derived from this software may not be called "ONSlaught" nor
*       may "ONSlaught" appear in their names without specific prior written
*       permission from the author.
*
* THIS SOFTWARE IS PROVIDED BY HELIOS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
* EVENT SHALL HELIOS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef NONS_COMMON_H
#define NONS_COMMON_H
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;

#ifndef NONS_SYS_WINDOWS
#define NONS_SYS_WINDOWS (defined _WIN32 || defined _WIN64)
#endif
#ifndef NONS_SYS_LINUX
#define NONS_SYS_LINUX (defined linux || defined __linux)
#endif
#ifndef NONS_SYS_BSD
#define NONS_SYS_BSD (defined __bsdi__)
#endif
#ifndef NONS_SYS_UNIX
#define NONS_SYS_UNIX (defined __unix__ || defined __unix)
#endif
#ifndef NONS_SYS_PSP
#define NONS_SYS_PSP (defined PSP)
#endif

#if NONS_SYS_WINDOWS
typedef void *HANDLE;
#ifdef BUILD_ONSLAUGHT
#define NONS_DECLSPEC __declspec(dllexport)
#else
#define NONS_DECLSPEC __declspec(dllimport)
#endif
#define NONS_DLLexport __declspec(dllexport)
#define NONS_DLLimport __declspec(dllimport)

#ifndef _WIN64
typedef ulong DWORD;
#else
typedef ushort DWORD;
#endif
#else
#define NONS_DECLSPEC
#define NONS_DLLexport
#define NONS_DLLimport
#endif

#ifdef _MSC_VER
#define NONS_COMPILER_VCPP
#pragma warning(disable:4065) //no "switch statement contains 'default' but no 'case' labels", generated by Bison.
#pragma warning(disable:4996) //no "unsafe" functions
#endif

#include <cwchar>
#ifndef WCHAR_MAX
#error "WCHAR_MAX is not defined."
#endif
#if WCHAR_MAX<0xFFFF
#error "Wide characters on this implementation are too narrow."
#endif

#include <climits>
#if ULONG_MAX<0xFFFFFFFF
#error "longs on this implementation are too small."
#endif

extern const int rmask;
extern const int gmask;
extern const int bmask;
extern const int amask;
extern const int rshift;
extern const int gshift;
extern const int bshift;
extern const int ashift;
extern int lastClickX;
extern int lastClickY;
extern NONS_DECLSPEC ulong cpu_count;

extern NONS_DECLSPEC volatile bool ctrlIsPressed;
extern NONS_DECLSPEC volatile bool forceSkip;

#define CURRENTLYSKIPPING (ctrlIsPressed || forceSkip)

#if NONS_SYS_PSP
#include <string>
#include <sstream>
namespace std{
typedef basic_string<wchar_t> wstring;
typedef basic_stringstream<wchar_t> wstringstream;
}
#define NONS_LOW_MEMORY_ENVIRONMENT
#endif
#endif
