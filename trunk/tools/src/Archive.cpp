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

#include "Archive.h"

void writeByte(std::string &dst,ulong src,size_t offset){
	if (offset==ULONG_MAX)
		offset=dst.size();
	if (offset<dst.size())
		dst[offset]=(char)src;
	else
		dst.push_back((char)src);
}

ulong readByte(void *src,size_t &offset){
	return ((uchar *)src)[offset++];
}

void writeLittleEndian(size_t size,std::string &dst,ulong src,size_t offset){
	if (offset==ULONG_MAX)
		offset=dst.size();
	if (offset+size>=dst.size())
		dst.resize(offset+size);
	for (int a=size;a;a--,offset++){
		dst[offset]=(char)src;
		src>>=8;
	}
}

void writeLittleEndian(size_t size,void *_src,ulong src,size_t &offset){
	for (int a=size;a;a--,offset++){
		((uchar *)_src)[offset]=(uchar)src;
		src>>=8;
	}
}

void writeBigEndian(size_t size,std::string &dst,ulong src,size_t offset){
	if (offset==ULONG_MAX)
		offset=dst.size();
	if (offset+size>=dst.size())
		dst.resize(offset+size);
	offset+=size-1;
	for (int a=size;a;a--,offset--){
		dst[offset]=(char)src;
		src>>=8;
	}
	offset+=size;
}

void writeBigEndian(size_t size,void *_src,ulong src,size_t &offset){
	offset+=size-1;
	for (int a=size;a;a--,offset--){
		((uchar *)_src)[offset]=(uchar)src;
		src>>=8;
	}
	offset+=size;
}

ulong readLittleEndian(size_t size,void *_src,size_t &offset){
	ulong res=0;
	uchar *src=(uchar *)_src;
	src+=offset+size-1;
	for (int a=size;a;a--,src--)
		res=(res<<8)|*src;
	offset+=size;
	return res;
}

ulong readBigEndian(size_t size,void *_src,size_t &offset){
	ulong res=0;
	uchar *src=(uchar *)_src;
	src+=offset;
	for (int a=size;a;a--,src++)
		res=(res<<8)|*src;
	offset+=size;
	return res;
}
