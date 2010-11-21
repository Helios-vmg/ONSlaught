/*
* Copyright (c) 2010, Helios (helios.vmg@gmail.com)
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

#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H
#include <string>

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

typedef unsigned long ulong;
typedef unsigned char uchar;

#ifndef BUILD_PP
#define PP_DECLARE_FUNCTION(return,name,parameters) typedef PP_DECLSPEC return (*name##_f)parameters;\
PP_DECLSPEC return name parameters
#else
#define PP_DECLARE_FUNCTION(return,name,parameters) typedef return(*name##_f)parameters
#endif

#if NONS_SYS_WINDOWS
#ifdef BUILD_PP
#define PP_DECLSPEC __declspec(dllexport)
#else
#define PP_DECLSPEC __declspec(dllimport)
#endif
#define PP_DLLexport __declspec(dllexport)
#define PP_DLLimport __declspec(dllimport)
#else
#define PP_DECLSPEC
#define PP_DLLexport
#define PP_DLLimport
#endif

struct PP_instance;

struct PP_parameters{
	const char *function;
	const char **parameters;
	const size_t *sizes;
	size_t array_size;
};

struct PP_result{
	bool good;
	const char *string;
	size_t string_length;
	size_t index;
};

extern "C"{
PP_DECLARE_FUNCTION(PP_instance *,PP_init,(const char *,void **));
PP_DECLARE_FUNCTION(const char *,PP_get_error_string,(void *));
PP_DECLARE_FUNCTION(void,PP_free_error_string,(void *));
PP_DECLARE_FUNCTION(void,PP_destroy,(PP_instance *));
PP_DECLARE_FUNCTION(PP_result,PP_preprocess,(PP_instance *,PP_parameters));
PP_DECLARE_FUNCTION(void,PP_done,(PP_instance *,PP_result));
}

std::string to_string(const PP_result &r){
	return std::string(r.string,r.string_length);
}
#endif
