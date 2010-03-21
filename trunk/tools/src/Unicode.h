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

#ifndef UNICODE_H
#define UNICODE_H
#include <string>
#include <algorithm>
#include <cstring>

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;

#if WCHAR_MAX<0xFFFF
#error "Wide characters on this platform are too narrow."
#endif

#define BOM16B 0xFEFF
#define BOM16BA 0xFE
#define BOM16BB 0xFF
#define BOM16L 0xFFFE
#define BOM16LA BOM16BB
#define BOM16LB BOM16BA
#define BOM8A ((uchar)0xEF)
#define BOM8B ((uchar)0xBB)
#define BOM8C ((uchar)0xBF)
#define NONS_BIG_ENDIAN 0
#define NONS_LITTLE_ENDIAN 1
#define UNDEFINED_ENDIANNESS 2

inline bool NONS_isupper(unsigned character){
	return character>='A' && character<='Z';
}
#define UNICODE_TOLOWER(x) ((x)|0x20)
inline unsigned NONS_tolower(unsigned character){
	return NONS_isupper(character)?UNICODE_TOLOWER(character):character;
}
inline void NONS_tolower(wchar_t *param){
	for (;*param;param++)
		*param=NONS_tolower(*param);
}
inline void NONS_tolower(char *param){
	for (;*param;param++)
		*param=NONS_tolower(*param);
}
template <typename T>
inline void tolower(std::basic_string<T> &str){
	std::transform<
		typename std::basic_string<T>::iterator,
		typename std::basic_string<T>::iterator,
		unsigned(*)(unsigned)>(str.begin(),str.end(),str.begin(),NONS_tolower);
}
template <typename T>
void toforwardslash(std::basic_string<T> &s){
	for (ulong a=0,size=s.size();a<size;a++)
		s[a]=(s[a]==0x5C)?0x2F:s[a];
}
template <typename T>
void tobackslash(std::basic_string<T> &s){
	for (ulong a=0,size=s.size();a<size;a++)
		s[a]=(s[a]==0x2F)?0x5C:s[a];
}

std::string UniToUTF8(const std::wstring &str,bool addBOM=0);
std::wstring UniFromUTF8(const std::string &str);
std::string UniToSJIS(const std::wstring &str);
std::wstring UniFromSJIS(const std::string &str);
std::wstring UniFromUCS2(const std::string &str,char end=UNDEFINED_ENDIANNESS);
bool isValidUTF8(const char *buffer,ulong size);
bool isValidSJIS(const char *buffer,ulong size);
std::wstring UniFromISO88591(const std::string &str);
std::string UniToUCS2(const std::wstring &str,char end=UNDEFINED_ENDIANNESS);
std::string UniToISO88591(const std::wstring &str);
ulong getUTF8size(const wchar_t *buffer,ulong size);

template <typename T1,typename T2>
int lexcmp_CI_bounded(const T1 *a,size_t sizeA,const T2 *b,size_t sizeB){
	for (size_t c=0;c<sizeA && c<sizeB;a++,b++,c++){
		unsigned d=NONS_tolower(*a),
		e=NONS_tolower(*b);
		if (d<e)
			return -1;
		if (d>e)
			return 1;
	}
	if (sizeA<sizeB)
		return -1;
	if (sizeA>sizeB)
		return 1;
	return 0;
}

template <typename T1,typename T2>
int stdStrCmpCI(const std::basic_string<T1> &s1,const std::basic_string<T2> &s2){
	return lexcmp_CI_bounded(s1.c_str(),s1.size(),s2.c_str(),s2.size());
}
#endif
