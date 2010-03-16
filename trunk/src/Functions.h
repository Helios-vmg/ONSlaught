/*
* Copyright (c) 2008, 2009, Helios (helios.vmg@gmail.com)
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

#ifndef NONS_FUNCTIONS_H
#define NONS_FUNCTIONS_H

#include "ErrorCodes.h"
#include <map>
#include "Common.h"
#include <SDL/SDL.h>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define UNICODE_TOUPPER(x) ((x)&0xDF)
#define UNICODE_TOLOWER(x) ((x)|0x20)

#define ABS(x) ((x)<0?-(x):(x))

#define HANDLE_POSSIBLE_ERRORS(x) {\
	ErrorCode possible_error=(x);\
	if (possible_error!=NONS_NO_ERROR)\
		return possible_error;\
}
#define CHECK_FLAG(x,y) (((x)&(y))==(y))
#ifdef USE_ACCURATE_MULTIPLICATION
//Accurate version:
#define INTEGER_MULTIPLICATION(a,b) (((a)*(b))/255)
#else
//Fast version:
#define INTEGER_MULTIPLICATION(a,b) (((a)*(b))>>8)
#endif

//string functions
template <typename T,typename T2>
bool multicomparison(T character,const T2 *characters){
	for (;*characters;characters++)
		if (character==*characters)
			return 1;
	return 0;
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

template <typename T>
std::vector<std::basic_string<T> > getParameterList(const std::basic_string<T> &string,bool leave_quotes,char delim=' '){
	std::vector<std::basic_string<T> > res;
	char tempDelim=delim;
	for (ulong a=0,size=string.size();a<size;){
		if (string[a]=='\"'){
			if (!leave_quotes)
				a++;
			delim='\"';
		}
		ulong end=a;
		if (delim!=tempDelim && leave_quotes)
			end++;
		for (;end<size && string[end]!=delim;end++);
		if (delim!=tempDelim && leave_quotes)
			end++;
		res.push_back(std::basic_string<T>(string,a,end-a));
		delim=tempDelim;
		for (a=end+1;a<size && string[a]==delim;a++);
	}
	return res;
}

/*bool filenames_are_equal(const wchar_t *str0,const wchar_t *str1);
bool filenames_are_equal(const char *str0,const char *str1);*/
template <typename T>
bool isValidIdentifier(const std::basic_string<T> &str){
	if (!str.size() || !NONS_isid1char(str[0]))
		return 0;
	const T *s=&str[1];
	for (ulong a=1,size=str.size();a<size;a++,s++)
		if (!NONS_isidnchar(*s))
			return 0;
	return 1;
}

template <typename T>
bool isValidLabel(const std::basic_string<T> &str){
	const T *s=&str[0];
	for (ulong a=0,size=str.size();a<size;a++,s++)
		if (!NONS_isidnchar(*s))
			return 0;
	return 1;
}

template <typename T>
inline T HEX2DEC(T x){
	return x<='9'?x-'0':(x<='F'?x-'A'+10:x-'a'+10);
}

template <typename T>
long atoi(const std::basic_string<T> &str){
	std::basic_stringstream<T> stream(str);
	long res;
	return !(stream >>res)?0:res;
}

template <typename T,typename T2>
std::basic_string<T> itoa(T2 n,unsigned w=0){
	std::basic_stringstream<T> stream;
	if (w){
		stream.fill('0');
		stream.width(w);
	}
	stream <<n;
	return stream.str();
}
template <typename T> inline std::string  itoac(T n,unsigned w=0){ return itoa<char>   (n,w); }
template <typename T> inline std::wstring itoaw(T n,unsigned w=0){ return itoa<wchar_t>(n,w); }

//1 if the s1 begins with s2 at off
template <typename T>
bool firstchars(const std::basic_string<T> &s1,size_t off,const std::basic_string<T> &s2){
	if (s1.size()-off<s2.size())
		return 0;
	for (ulong a=0;a<s2.size();a++)
		if (s1[off+a]!=s2[a])
			return 0;
	return 1;
}

template <typename T>
bool firstchars(const std::basic_string<T> &s1,size_t off,const T *s2){
	ulong l=0;
	while (s2[l])
		l++;
	if (s1.size()-off<l)
		return 0;
	for (ulong a=0;a<l;a++)
		if (s1[off+a]!=s2[a])
			return 0;
	return 1;
}

template <typename T>
std::basic_string<T> string_replace(
		const std::basic_string<T> &src,
		const std::basic_string<T> &what,
		const std::basic_string<T> &with){
	if (!what.size())
		return src;
	std::basic_string<T> res;
	for (ulong a=0,end=src.size();a<end;){
		ulong found=src.find(what,a);
		if (found!=src.npos){
			res.append(src,a,found-a);
			res.append(with);
			a=found+what.size();
		}else{
			res.append(src,a,found);
			a=found;
		}
	}
	return res;
}

template <typename T>
inline std::basic_string<T> string_replace(
		const std::basic_string<T> &src,
		const T *what,
		const T *with){
	if (!what || !*what)
		return src;
	std::basic_string<T> res;
	ulong l=0,l2=0;
	while (what[l])
		l++;
	if (with)
		while (with[l2])
			l2++;
	for (ulong a=0,end=src.size();a<end;){
		ulong found=src.find(what,a,l);
		if (found!=src.npos){
			res.append(src,a,found-a);
			if (with)
				res.append(with,l2);
			a=found+l;
		}else{
			res.append(src,a,found);
			a=found;
		}
	}
	return res;
}


#undef min
//string parsing
template <typename T>
std::basic_string<T> tagName(const std::basic_string<T> &string,size_t off){
	if (string[off]!='<')
		return std::basic_string<T>();
	ulong a=string.find('>',off+1),
		b=string.find('=',off+1);
	a=std::min(a,b);
	if (a==string.npos)
		return std::basic_string<T>();
	std::basic_string<T> temp(string,off+1,a-(off+1));
	trim_string(temp);
	return temp;
}

template <typename T>
std::basic_string<T> tagValue(const std::basic_string<T> &string,size_t off){
	if (string[off]!='<')
		return std::basic_string<T>();
	ulong a=string.find('>',off+1),
		b=string.find('=',off+1);
	a=std::min(a,b);
	if (a==string.npos || string[a]=='>')
		return std::basic_string<T>();
	a++;
	ulong c=string.find('>',a);
	std::basic_string<T> temp(string,a,a-c);
	trim_string(temp);
	return temp;
}

//binary parsing functions
bool getbit(void *arr,ulong *byteoffset,uchar *bitoffset);
ulong getbits(void *arr,uchar bits,ulong *byteoffset,uchar *bitoffset);
Uint8 readByte(void *buffer,ulong &offset);
Sint16 readSignedWord(char *buffer,ulong &offset);
Uint16 readWord(void *buffer,ulong &offset);
Sint32 readSignedDWord(char *buffer,ulong &offset);
Uint32 readDWord(void *buffer,ulong &offset);
std::string readString(char *buffer,ulong &offset);
void writeByte(Uint8 a,std::string &str,ulong offset=ULONG_MAX);
void writeWord(Uint16 a,std::string &str,ulong offset=ULONG_MAX);
void writeDWord(Uint32 a,std::string &str,ulong offset=ULONG_MAX);
void writeWordBig(Uint16 a,std::string &str,ulong offset=ULONG_MAX);
void writeDWordBig(Uint32 a,std::string &str,ulong offset=ULONG_MAX);
void writeString(const std::wstring &a,std::string &str);

template <typename T>
std::vector<Sint32> getIntervals(typename std::map<Sint32,T>::iterator i,typename std::map<Sint32,T>::iterator end){
	std::vector<Sint32> intervals;
	Sint32 last=i->first;
	intervals.push_back(last++);
	while (++i!=end){
		if (i->first!=last){
			intervals.push_back(last-intervals.back());
			last=i->first;
			intervals.push_back(last++);
		}else
			last++;
	}
	intervals.push_back(last-intervals.back());
	return intervals;
}

extern const int rmask;
extern const int gmask;
extern const int bmask;
extern const int amask;

//#define USE_HARDWARE_SURFACES SDL_HWSURFACE
#define USE_HARDWARE_SURFACES 0

inline SDL_Surface *makeSurface(ulong w,ulong h,ulong bits,Uint32 r=rmask,Uint32 g=gmask,Uint32 b=bmask,Uint32 a=amask){
	return SDL_CreateRGBSurface(USE_HARDWARE_SURFACES|SDL_SRCALPHA,(int)w,(int)h,bits,r,g,b,a);
}

//bitmap processing functions
inline ulong SDLcolor2rgb(const SDL_Color &color){
	return (color.r<<16)|(color.g<<8)|color.b;
}

inline SDL_Color rgb2SDLcolor(ulong rgb){
	SDL_Color c={(rgb>>16)&0xFF,(rgb>>8)&0xFF,rgb&0xFF,0};
	return c;
}

typedef long manualBlitAlpha_t;
DECLSPEC void manualBlit(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect,manualBlitAlpha_t alpha=255);
DECLSPEC void manualBlit_unthreaded(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect,manualBlitAlpha_t alpha=255);
void multiplyBlend(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect);
void FlipSurfaceH(SDL_Surface *src,SDL_Surface *dst);
void FlipSurfaceV(SDL_Surface *src,SDL_Surface *dst);
void FlipSurfaceHV(SDL_Surface *src,SDL_Surface *dst);
SDL_Surface *horizontalShear(SDL_Surface *src,float amount);
SDL_Surface *verticalShear(SDL_Surface *src,float amount);
SDL_Surface *applyTransformationMatrix(SDL_Surface *src,float matrix[4]);

inline SDL_Surface *copySurface(SDL_Surface *src,bool fast=1){
	SDL_Surface *res=makeSurface(src->w,src->h,src->format->BitsPerPixel,src->format->Rmask,src->format->Gmask,src->format->Bmask,src->format->Amask);
	if (fast)
		manualBlit(src,0,res,0);
	else
		SDL_BlitSurface(src,0,res,0);
	res->clip_rect=src->clip_rect;
	return res;
}

//other functions
Uint32 secondsSince1970();
/*
Compresses src[0..srcl-1].
Return value: allocated compressed buffer.
src: input buffer.
srcl: length of the input buffer in bytes.
dstl: the length of the compressed buffer will be written here.
*/
char *compressBuffer_BZ2(char *src,unsigned long srcl,unsigned long *dstl);
/*
Decompresses src[0..srcl-1].
Return value: allocated decompressed buffer.
src: input buffer.
srcl: length of the input buffer in bytes.
dstl: the length of the decompressed buffer will be written here.
*/
char *decompressBuffer_BZ2(char *src,unsigned long srcl,unsigned long *dstl);

template <typename T1,typename T2>
bool binary_search(const T1 *set,size_t begin,size_t end,const T2 &value,size_t &at_offset,int (*comp_f)(const T2 &,const T1 &)){
	if (begin<=end){
		size_t size=end-begin+1;
		while (begin<=end){
			size_t pivot=begin+size/2;
			int cmp=comp_f(value,set[pivot]);
			if (size==1){
				if (!cmp){
					at_offset=pivot;
					return 1;
				}
				break;
			}
			if (cmp<0)
				end=pivot-1;
			else if (cmp>0)
				begin=pivot+1;
			else{
				at_offset=pivot;
				return 1;
			}
			size=end-begin+1;
		}
	}
	return 0;
}

ErrorCode inPlaceDecryption(char *buffer,ulong length,ulong mode);

#if NONS_SYS_WINDOWS
void findMainWindow(const wchar_t *caption);
#endif

//Unicode functions:
#define STR_WHITESPACE "\x09\x0A\x0B\x0C\x0D\x20\x85\xA0"
#define WCS_WHITESPACE L"\x0009\x000A\x000B\x000C\x000D\x0020\x0085\x00A0\x1680\x180E\x2002\x2003\x2004\x2005\x2006\x2008\x2009\x200A\x200B\x200C\x200D\x205F\x3000"
#define WCS_NON_NEWLINE_WHITESPACE L"\x0009\x000B\x000C\x0020\x0085\x00A0\x1680\x180E\x2002\x2003\x2004\x2005\x2006\x2008\x2009\x200A\x200B\x200C\x200D\x205F\x3000"

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

inline bool NONS_isdigit(unsigned character){
	return character>='0' && character<='9';
}
inline bool NONS_isupper(unsigned character){
	return character>='A' && character<='Z';
}
inline bool NONS_islower(unsigned character){
	return character>='a' && character<='z';
}
inline bool NONS_isalpha(unsigned character){
	return NONS_isupper(character) || NONS_islower(character);
}
inline bool NONS_isalnum(unsigned character){
	return NONS_isalpha(character) || NONS_isdigit(character);
}
inline unsigned NONS_toupper(unsigned character){
	return NONS_islower(character)?UNICODE_TOUPPER(character):character;
}
inline unsigned NONS_tolower(unsigned character){
	return NONS_isupper(character)?UNICODE_TOLOWER(character):character;
}
inline bool NONS_ishexa(unsigned character){
	return NONS_isdigit(character) || NONS_toupper(character)>='A' && NONS_toupper(character)<='F';
}
//1 if the character matches the regex [A-Za-z_] (the first character in a C-style identifier)
inline bool NONS_isid1char(unsigned character){
	return NONS_isalpha(character) || character=='_';
}
//1 if the character matches the regex [A-Za-z_0-9] (the second and beyond character in a C-style identifier)
inline bool NONS_isidnchar(unsigned character){
	return NONS_isid1char(character) || NONS_isdigit(character);
}

template <typename T1,typename T2>
int lexcmp(const T1 *a,const T2 *b){
	for (;*a || *b;a++,b++){
		unsigned c=*a,
		d=*b;
		if (c<d)
			return -1;
		if (c>d)
			return 1;
	}
	return 0;
}

template <typename T1,typename T2>
int lexcmp_CI(const T1 *a,const T2 *b){
	for (;*a || *b;a++,b++){
		unsigned c=NONS_toupper(*a),
		d=NONS_toupper(*b);
		if (c<d)
			return -1;
		if (c>d)
			return 1;
	}
	return 0;
}

template <typename T1,typename T2>
int lexcmp_CI_bounded(const T1 *a,size_t sizeA,const T2 *b,size_t sizeB){
	for (size_t c=0;c<sizeA && c<sizeB;a++,b++,c++){
		unsigned d=NONS_toupper(*a),
		e=NONS_toupper(*b);
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

ulong getUTF8size(const wchar_t *buffer,ulong size);
std::wstring UniFromISO88591(const std::string &str);
std::wstring UniFromUTF8(const std::string &str);
/*
Important note: this procedure assumes that the text string is a valid UCS-2
string, so while it does take BOM into account, it doesn't compensate for
streams with an odd length, as all valid UCS-2 strings have an even length.
*/
std::wstring UniFromUCS2(const std::string &str,char end=UNDEFINED_ENDIANNESS);
std::wstring UniFromSJIS(const std::string &str);
std::string UniToISO88591(const std::wstring &str);
std::string UniToUTF8(const std::wstring &str,bool addBOM=0);
std::string UniToUCS2(const std::wstring &str,char end=UNDEFINED_ENDIANNESS);
std::string UniToSJIS(const std::wstring &str);

template <typename T>
inline void toupper(std::basic_string<T> &str){
	std::transform<
		typename std::basic_string<T>::iterator,
		typename std::basic_string<T>::iterator,
		unsigned(*)(unsigned)>(str.begin(),str.end(),str.begin(),NONS_toupper);
}

template <typename T>
inline void tolower(std::basic_string<T> &str){
	std::transform<
		typename std::basic_string<T>::iterator,
		typename std::basic_string<T>::iterator,
		unsigned(*)(unsigned)>(str.begin(),str.end(),str.begin(),NONS_tolower);
}

template <typename T>
inline std::basic_string<T> toupperCopy(std::basic_string<T> str){
	toupper(str);
	return str;
}

template <typename T>
inline std::basic_string<T> tolowerCopy(std::basic_string<T> str){
	tolower(str);
	return str;
}

inline std::ostream &operator<<(std::ostream &stream,std::wstring &str){
	return stream <<UniToUTF8(str);
}

char checkEnd(wchar_t a);
//Determines the system's endianness.
char checkNativeEndianness();

bool isValidUTF8(const char *buffer,ulong size);
bool isValidSJIS(const char *buffer,ulong size);

bool iswhitespace(char character);
bool iswhitespace(wchar_t character);
bool iswhitespaceASCIIe(char character);
bool isbreakspace(char character);
bool isbreakspace(wchar_t character);
bool isbreakspaceASCIIe(char character);
void NONS_tolower(wchar_t *param);
void NONS_tolower(char *param);

template <typename T>
struct stdStringCmpCI{
	bool operator()(const std::basic_string<T> &s1,const std::basic_string<T> &s2) const{
		return lexcmp_CI_bounded(s1.c_str(),s1.size(),s2.c_str(),s2.size())<0;
	}
};

template <typename T1,typename T2>
int stdStrCmpCI(const std::basic_string<T1> &s1,const T2 *s2){
	std::basic_string<T2> temp=s2;
	return lexcmp_CI_bounded(s1.c_str(),s1.size(),temp.c_str(),temp.size());
}

template <typename T1,typename T2>
int stdStrCmpCI(const std::basic_string<T1> &s1,const std::basic_string<T2> &s2){
	return lexcmp_CI_bounded(s1.c_str(),s1.size(),s2.c_str(),s2.size());
}

//1 if the s1 begins with s2 at off
template <typename T>
bool firstcharsCI(const std::basic_string<T> &s1,size_t off,const std::basic_string<T> &s2){
	if (s1.size()-off<s2.size())
		return 0;
	for (ulong a=0;a<s2.size();a++)
		if (NONS_tolower(s1[off+a])!=NONS_tolower(s2[a]))
			return 0;
	return 1;
}

template <typename T>
bool firstcharsCI(const std::basic_string<T> &s1,size_t off,const T *s2){
	ulong l=0;
	while (s2[l])
		l++;
	if (s1.size()-off<l)
		return 0;
	for (ulong a=0;a<l;a++)
		if (NONS_tolower(s1[off+a])!=NONS_tolower(s2[a]))
			return 0;
	return 1;
}

template <typename T>
void trim_string(std::basic_string<T> &str){
	ulong first=0,
		second,
		size=str.size();
	for (;first<size && iswhitespace(str[first]);first++);
	if (first==size){
		str.clear();
		return;
	}
	second=size-1;
	for (;second>first && iswhitespace(str[second]);second--);
	second++;
	str=str.substr(first,second-first);
}

template <typename T>
std::basic_string<T> getTimeString(tm *time,bool useISO=0){
	const char *strings[]={
		"%Y-%m-%d %H:%M:%S",
		"%Y%m%dT%H%M%S"
	};
	char buffer[]="0000-00-00 00:00:00";
	strftime(buffer,sizeof(buffer),strings[(int)useISO],time);
	std::string temp=buffer;
	return std::basic_string<T>(temp.begin(),temp.end());
}

template <typename T>
std::basic_string<T> getTimeString(bool useISO=0,bool useLocalTime=1){
	typedef tm *(*time_f)(const time_t *);
	time_f functions[]={
		gmtime,
		localtime
	};
	time_t secs=time(0);
	return getTimeString<T>(functions[(int)useLocalTime](&secs),useISO);
}

struct surfaceData{
	uchar *pixels;
	uchar Roffset,
		Goffset,
		Boffset,
		Aoffset;
	ulong advance,
		pitch,
		w,h;
	bool alpha;
	surfaceData(){}
	surfaceData(const SDL_Surface *surface){
		*this=surface;
	}
	const surfaceData &operator=(const SDL_Surface *surface){
		this->pixels=(uchar *)surface->pixels;
		this->Roffset=(surface->format->Rshift)>>3;
		this->Goffset=(surface->format->Gshift)>>3;
		this->Boffset=(surface->format->Bshift)>>3;
		this->Aoffset=(surface->format->Ashift)>>3;
		this->advance=surface->format->BytesPerPixel;
		this->pitch=surface->pitch;
		this->w=surface->w;
		this->h=surface->h;
		this->alpha=(this->Aoffset!=this->Roffset && this->Aoffset!=this->Goffset && this->Aoffset!=this->Boffset);
		return *this;
	}
};

struct NONS_Rect{
	float x,y,w,h;
	SDL_Rect to_SDL_Rect(){
		SDL_Rect ret={(Sint16)x,(Sint16)y,(Uint16)w,(Uint16)h};
		return ret;
	}
	NONS_Rect &operator=(const SDL_Rect &s){
		this->x=(float)s.x;
		this->y=(float)s.y;
		this->w=(float)s.w;
		this->h=(float)s.h;
		return *this;
	}
};

template <typename T>
void freePointerVector(std::vector<T *> &v){
	for (size_t a=0;a<v.size();a++)
		if (v[a])
			delete v[a];
	v.clear();
}
#endif
