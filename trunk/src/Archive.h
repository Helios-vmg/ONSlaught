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

#ifndef NONS_ARCHIVE_H
#define NONS_ARCHIVE_H

#include "Common.h"
#include "IOFunctions.h"
#include "ErrorCodes.h"
#include <string>
#include <vector>
#include <map>
#include <fstream>

struct TreeNodeComp{
	bool operator()(const std::wstring &opA,const std::wstring &opB) const{
		return stdStrCmpCI(opA,opB)<0;
	}
};

struct TreeNode{
	std::wstring name;
	typedef std::map<std::wstring,TreeNode,TreeNodeComp> container;
	typedef container::iterator container_iterator;
	typedef container::const_iterator const_container_iterator;
	container branches;
public:
	void *extraData;
	void(*freeExtraData)(void *);
	bool is_dir,
		skip;
	TreeNode():is_dir(0),skip(0),extraData(0),freeExtraData(0){}
	TreeNode(const std::wstring &s):is_dir(0),name(s),skip(0),extraData(0),freeExtraData(0){}
	~TreeNode();
	void clear(){ this->branches.clear(); }
	TreeNode *get_branch(const std::wstring &path,bool create);
};

class Archive{
protected:
	TreeNode root;
public:
	bool good;
	Archive():root(L""),good(0){
		this->root.is_dir=1;
		this->root.skip=1;
	}
	virtual ~Archive(){}
	bool exists(const std::wstring &path);
	bool test();
	bool test_rec(TreeNode *node);
	uchar *get_file_buffer(const std::wstring &path,ulong &size);
protected:
	virtual uchar *get_file_buffer(const std::wstring &path,TreeNode *node,ulong &size)=0;
};

struct NSAdata{
	ulong offset;
	enum compression_type{
		COMPRESSION_NONE=0,
		COMPRESSION_SPB=1,
		COMPRESSION_LZSS=2,
		COMPRESSION_BZ2=4
	} compression;
	ulong compressed,
		uncompressed;
	NSAdata():offset(0),compression(COMPRESSION_NONE),compressed(0),uncompressed(0){}
};

class NSAarchive:public Archive{
	std::wstring path;
	NONS_File file;
public:
	bool nsa;
	NSAarchive(const std::wstring &path,bool nsa);
	uchar *get_file_buffer(const std::wstring &path,TreeNode *node,ulong &size);
	static void freeExtraData(void *);
	//dereference extra data
	static const NSAdata &derefED(void *p){
		return *(NSAdata *)p;
	}
};

struct ZIPdata{
	ulong bit_flag;
	enum compression_type{
		COMPRESSION_NONE=0,
		COMPRESSION_DEFLATE=8,
		COMPRESSION_BZ2=12,
		COMPRESSION_LZMA=14
	} compression;
	Uint32 crc32;
	ulong compressed,
		uncompressed,
		disk,
		data_offset;
	ZIPdata()
		:bit_flag(0),
		compression(COMPRESSION_NONE),
		crc32(0),
		compressed(0),
		uncompressed(0),
		disk(0),
		data_offset(0){}
};

class ZIParchive:public Archive{
	std::vector<std::wstring> disks;
public:
	ZIParchive(const std::wstring &path);
	uchar *get_file_buffer(const std::wstring &path,TreeNode *node,ulong &size);
	static void freeExtraData(void *);
	static const ZIPdata &derefED(void *p){
		return *(ZIPdata *)p;
	}

	enum SignatureType{
		NOT_A_SIGNATURE=0,
		LOCAL_HEADER,
		CENTRAL_HEADER,
		EOCDR
	};
	static SignatureType getSignatureType(void *buffer);
	static const ulong   local_signature=0x04034B50,
	                   central_signature=0x02014B50,
	                     EOCDR_signature=0x06054b50;
};

struct NONS_GeneralArchive{
	std::vector<Archive *> archives;
	NONS_GeneralArchive();
	~NONS_GeneralArchive();
	uchar *getFileBuffer(const std::wstring &filepath,ulong &buffersize);
	//Same as above, but doesn't try the file system
	uchar *getFileBufferWithoutFS(const std::wstring &filepath,ulong &buffersize);
	bool exists(const std::wstring &filepath);
};
#endif
