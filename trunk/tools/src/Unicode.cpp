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

#include "Unicode.h"
#include "SJIS.table.cpp"
#include <string>
#include <cwchar>
#include <iostream>

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;

#if WCHAR_MAX<0xFFFF
#error "Wide characters on this platform are too narrow."
#endif

void WC_UTF8(uchar *dst,const wchar_t *src,ulong srcl){
	for (ulong a=0;a<srcl;a++){
		wchar_t character=*src++;
		if (character<0x80)
			*dst++=(uchar)character;
		else if (character<0x800){
			*dst++=(character>>6)|0xC0;
			*dst++=character&0x3F|0x80;
#if WCHAR_MAX==0xFFFF
		}else{
#else
		}else if (character<0x10000){
#endif
			*dst++=(character>>12)|0xE0;
			*dst++=((character>>6)&0x3F)|0x80;
			*dst++=character&0x3F|0x80;
#if WCHAR_MAX==0xFFFF
		}
#else
		}else{
			*dst++=(character>>18)|0xF0;
			*dst++=((character>>12)&0x3F)|0x80;
			*dst++=((character>>6)&0x3F)|0x80;
			*dst++=character&0x3F|0x80;
		}
#endif
	}
}

ulong getUTF8size(const wchar_t *buffer,ulong size){
	ulong res=0;
	for (ulong a=0;a<size;a++){
		if (buffer[a]<0x80)
			res++;
		else if (buffer[a]<0x800)
			res+=2;
#if WCHAR_MAX==0xFFFF
		else
#else
		else if (buffer[a]<0x10000)
#endif
			res+=3;
#if WCHAR_MAX!=0xFFFF
		else
			res+=4;
#endif
	}
	return res;
}

std::string UniToUTF8(const std::wstring &str,bool addBOM){
	std::string res;
	res.resize(getUTF8size(&str[0],str.size())+addBOM*3);
	if (addBOM){
		res.push_back(BOM8A);
		res.push_back(BOM8B);
		res.push_back(BOM8C);
	}
	WC_UTF8((uchar *)&res[addBOM*3],&str[0],str.size());
	return res;
}

void UTF8_WC(wchar_t *dst,const uchar *src,ulong srcl){
	for (ulong a=0;a<srcl;a++){
		uchar byte=*src++;
		wchar_t c=0;
		if (!(byte&0x80))
			c=byte;
		else if ((byte&0xC0)==0x80)
			continue;
		else if ((byte&0xE0)==0xC0){
			c=byte&0x1F;
			c<<=6;
			c|=*src&0x3F;
		}else if ((byte&0xF0)==0xE0){
			c=byte&0x0F;
			c<<=6;
			c|=*src&0x3F;
			c<<=6;
			c|=src[1]&0x3F;
		}else if ((byte&0xF8)==0xF0){
#if WCHAR_MAX==0xFFFF
			c='?';
#else
			c=byte&0x07;
			c<<=6;
			c|=src[1]&0x3F;
			c<<=6;
			c|=src[2]&0x3F;
			c<<=6;
			c|=src[3]&0x3F;
#endif
		}
		*dst++=c;
	}
}

std::wstring UniFromUTF8(const std::string &str){
	ulong start=0;
	if (str.size()>=3 && (uchar)str[0]==BOM8A && (uchar)str[1]==BOM8B && (uchar)str[2]==BOM8C)
		start+=3;
	const uchar *str2=(const uchar *)&str[0]+start;
	ulong size=0;
	for (ulong a=start,end=str.size();a<end;a++,str2++)
		if (*str2<128 || (*str2&192)==192)
			size++;
	std::wstring res;
	res.resize(size);
	str2=(const uchar *)&str[0]+start;
	UTF8_WC(&res[0],str2,str.size()-start);
	return res;
}

ulong WC_SJIS(uchar *dst,const wchar_t *src,ulong srcl){
	ulong ret=0;
	for (ulong a=0;a<srcl;a++){
		wchar_t srcc=*src++,
			character=Unicode2SJIS[srcc];
		if (character=='?' && srcc!='?'){
			std::cerr <<"ENCODING ERROR: Character U+"<<std::hex;
			std::cerr.width(4);
			std::cerr <<srcc<<" is unsupported by this Unicode->Shift JIS implementation. Replacing with '?'.\n";
		}
		if (character<0x100)
			dst[ret++]=(uchar)character;
		else{
			dst[ret++]=character>>8;
			dst[ret++]=character&0xFF;
		}
	}
	return ret;
}

/*
Note: All sizes are powers of two, and wchar_t is guaranteed to be at least as
big as char, so this division will always give 1, 2, 4, etc.
*/
const ulong sizeRatio=sizeof(wchar_t)/sizeof(char);

std::string UniToSJIS(const std::wstring &str){
	std::string res;
	res.resize(str.size()*sizeRatio);
	res.resize(WC_SJIS((uchar *)&res[0],&str[0],str.size()));
	return res;
}

#define IS_SJIS_WIDE(x) ((x)>=0x81 && (x)<=0x9F || (x)>=0xE0 && (x)<=0xEF)
ulong SJIS_WC(wchar_t *dst,const uchar *src,ulong srcl){
	ulong ret=0;
	for (ulong a=0;a<srcl;a++,ret++){
		uchar c0=*src++;
		wchar_t c1;
		if (IS_SJIS_WIDE(c0)){
			c1=(c0<<8)|*src++;
			a++;
		}else
			c1=c0;
		if (SJIS2Unicode[c1]=='?' && c1!='?'){
			std::cerr <<"ENCODING ERROR: Character SJIS+"<<std::hex;
			std::cerr.width(4);
			std::cerr <<c1<<" is unsupported by this Shift JIS->Unicode implementation. Replacing with '?'.\n";
		}
		*dst++=SJIS2Unicode[c1];
	}
	return ret;
}

std::wstring UniFromSJIS(const std::string &str){
	std::wstring res;
	res.resize(str.size());
	res.resize(SJIS_WC(&res[0],(const uchar *)&str[0],str.size()));
	return res;
}

inline wchar_t invertWC(wchar_t val){
#if WCHAR_MAX<=0xFFFF
	return (val>>8)|(val<<8);
#elif WCHAR_MAX<=0xFFFFFFFF
	return (val>>16)|0xFF&(val>>8)|0xFF00&(val<<8)|(val<<16);
#endif
}

char checkNativeEndianness(){
	ulong a=0x12345678;
	if (*(uchar *)&a==0x12)
		return NONS_BIG_ENDIAN;
	else
		return NONS_LITTLE_ENDIAN;
}

char nativeEndianness=checkNativeEndianness();

void UCS2_WC(wchar_t *dst,const uchar *src,ulong srcl,uchar end){
	memcpy(dst,src,srcl);
	srcl/=sizeRatio;
	if ((uchar)nativeEndianness!=end)
		for (ulong a=0;a<srcl;a++)
			dst[a]=invertWC(dst[a]);
}

char checkEnd(wchar_t a){
	if (a==BOM16B)
		return NONS_BIG_ENDIAN;
	else if (a==BOM16L)
		return NONS_LITTLE_ENDIAN;
	else
		return UNDEFINED_ENDIANNESS;
}

std::wstring UniFromUCS2(const std::string &str,char end){
	std::wstring res;
	ulong size=(str.size()&1)?str.size()-1:str.size();
	if (size<2)
		return res;
	wchar_t firstChar=(str[0]<<8)|str[1];
	char realEnd=checkEnd(firstChar);
	bool usesBOM=(realEnd!=UNDEFINED_ENDIANNESS);
	ulong start=0;
	if (usesBOM)
		start=2;
	else
		realEnd=NONS_BIG_ENDIAN;
	if (end==UNDEFINED_ENDIANNESS)
		end=realEnd;
	size-=start;
	res.resize(str.size()/sizeRatio);
	UCS2_WC(&res[0],(const uchar *)&str[0]+start,size,end);
	return res;
}

bool isValidUTF8(const char *buffer,ulong size){
	const uchar *unsigned_buffer=(const uchar *)buffer;
	for (ulong a=0;a<size;a++){
		ulong char_len;
		if (!(*unsigned_buffer&128))
			char_len=1;
		else if ((*unsigned_buffer&224)==192)
			char_len=2;
		else if ((*unsigned_buffer&240)==224)
			char_len=3;
		else if ((*unsigned_buffer&248)==240)
			char_len=4;
		else
			return 0;
		unsigned_buffer++;
		if (char_len<2)
			continue;
		a++;
		for (ulong b=1;b<char_len;b++,a++,unsigned_buffer++)
			if (*unsigned_buffer<0x80 || (*unsigned_buffer&0xC0)!=0x80)
				return 0;
	}
	return 1;
}

bool isValidSJIS(const char *buffer,ulong size){
	const uchar *unsigned_buffer=(const uchar *)buffer;
	for (ulong a=0;a<size;a++,unsigned_buffer++){
		if (!IS_SJIS_WIDE(*unsigned_buffer)){
			//Don't bother trying to understand what's going on here. It took
			//*me* around ten minutes. It works, and that's all you need to
			//know.
			if (*unsigned_buffer>=0x80 && *unsigned_buffer<=0xA0 || *unsigned_buffer>=0xF0)
				return 0;
			continue;
		}
		a++;
		unsigned_buffer++;
		if (*unsigned_buffer<0x40 || *unsigned_buffer>0xFC || *unsigned_buffer==0x7F)
			return 0;
	}
	return 1;
}

void ISO_WC(wchar_t *dst,const uchar *src,ulong srcl){
	for (ulong a=0;a<srcl;a++,src++,dst++)
		*dst=*src;
}

std::wstring UniFromISO88591(const std::string &str){
	std::wstring res;
	res.resize(str.size());
	ISO_WC(&res[0],(const uchar *)&str[0],str.size());
	return res;
}

void WC_UCS2(uchar *dst,const wchar_t *src,ulong srcl,char end){
	bool useBOM=(end<UNDEFINED_ENDIANNESS);
	if (!useBOM)
		end=NONS_BIG_ENDIAN;
	if (nativeEndianness==NONS_BIG_ENDIAN){
		dst[0]=BOM16BA;
		dst[1]=BOM16BB;
	}else{
		dst[0]=BOM16LA;
		dst[1]=BOM16LB;
	}
	srcl*=sizeRatio;
	memcpy(dst+sizeof(uchar)*useBOM,src,srcl);
	srcl+=sizeof(uchar)*useBOM;
	if (nativeEndianness!=end){
		for (ulong a=0;a<srcl;a+=2,dst+=2){
			char temp=*dst;
			*dst=dst[1];
			dst[1]=temp;
		}
	}
}

std::string UniToUCS2(const std::wstring &str,char end){
	std::string res;
	res.resize(str.size()*2+(end!=UNDEFINED_ENDIANNESS)*2);
	WC_UCS2((uchar *)&res[0],&str[0],str.size(),end);
	return res;
}

void WC_88591(uchar *dst,const wchar_t *src,ulong srcl){
	for (ulong a=0;a<srcl;a++,src++,dst++)
		*dst=(*src>0xFF)?'?':*src;
}

std::string UniToISO88591(const std::wstring &str){
	std::string res;
	res.resize(str.size());
	WC_88591((uchar *)&res[0],&str[0],str.size());
	return res;
}

