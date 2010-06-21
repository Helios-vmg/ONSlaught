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

#ifndef NONS_ARCHIVE_H
#define NONS_ARCHIVE_H

#include "Common.h"
#include "IOFunctions.h"
#include "ErrorCodes.h"
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <zlib.h>

struct TreeNode{
	std::wstring name;
	typedef std::map<std::wstring,TreeNode,stdStringCmpCI<wchar_t> > container;
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
	bool read_raw_bytes(void *dst,size_t read_bytes,size_t &bytes_read,const std::wstring &path,Uint64 offset);
	bool get_file_size(Uint64 &size,const std::wstring &path);
	virtual Uint64 get_size(TreeNode *tn)=0;
	virtual bool read_raw_bytes(void *dst,size_t read_bytes,size_t &bytes_read,TreeNode *node,Uint64 offset)=0;
	TreeNode *find_file(const std::wstring &path);
};

struct NSAdata{
	ulong offset;
	enum compression_type{
		COMPRESSION_NONE=0,
		COMPRESSION_SPB=1,
		COMPRESSION_LZSS=2,
		COMPRESSION_BZ2=4,
		COMPRESSION_DEFLATE=5,
	} compression;
	size_t compressed,
		uncompressed;
	NSAdata():offset(0),compression(COMPRESSION_NONE),compressed(0),uncompressed(0){}
};

class NSAarchive:public Archive{
	std::wstring path;
	NONS_File file;
public:
	bool nsa;
	NSAarchive(const std::wstring &path,bool nsa);
	bool read_raw_bytes(void *dst,size_t read_bytes,size_t &bytes_read,TreeNode *node,Uint64 offset);
	static void freeExtraData(void *);
	//dereference extra data
	static const NSAdata &derefED(void *p){
		return *(NSAdata *)p;
	}
	Uint64 get_size(TreeNode *tn){
		return derefED(tn->extraData).uncompressed;
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
	Uint64 compressed,
		uncompressed,
		data_offset;
	ulong disk;
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
	bool read_raw_bytes(void *dst,size_t read_bytes,size_t &bytes_read,TreeNode *node,Uint64 offset);
	static void freeExtraData(void *);
	static const ZIPdata &derefED(void *p){
		return *(ZIPdata *)p;
	}
	Uint64 get_size(TreeNode *tn){
		return derefED(tn->extraData).uncompressed;
	}

	enum SignatureType{
		NOT_A_SIGNATURE=0,
		LOCAL_HEADER,
		CENTRAL_HEADER,
		EOCDR,
		EOCDR64_LOCATOR,
		EOCDR64,
	};
	static SignatureType getSignatureType(void *buffer);
	static const Uint64   local_signature=0x04034B50,
	                    central_signature=0x02014B50,
	                      EOCDR_signature=0x06054b50,
	            EOCDR64_locator_signature=0x07064b50,
	                    EOCDR64_signature=0x06064b50;
};

struct NONS_GeneralArchive{
	std::vector<NONS_DataSource *> archives;
	~NONS_GeneralArchive();
	void init();
	uchar *getFileBuffer(const std::wstring &filepath,size_t &buffersize,bool use_filesystem=1);
	//Same as above, but doesn't try the file system
	uchar *getFileBufferWithoutFS(const std::wstring &filepath,size_t &buffersize);
	NONS_DataStream *open(const std::wstring &path);
	bool close(NONS_DataStream *stream);
	bool exists(const std::wstring &filepath);
};

extern NONS_GeneralArchive general_archive;

class NONS_ArchiveStream;

class NONS_ArchiveSource:public NONS_DataSource{
protected:
	Archive *archive;
public:
	virtual bool good(){ return this->archive->good; }
	bool get_size(Uint64 &size,const std::wstring &name);
	Uint64 get_size(TreeNode *node);
	bool read(void *dst,size_t &bytes_read,NONS_DataStream &stream,size_t count);
	bool read(void *dst,size_t &bytes_read,NONS_ArchiveStream &stream,size_t count);
	TreeNode *get_node(const std::wstring &path);
	uchar *read_all(const std::wstring &name,size_t &bytes_read);
	virtual uchar *read_all(TreeNode *node,size_t &bytes_read)=0;
	bool exists(const std::wstring &name);
};

class NONS_nsaArchiveSource:public NONS_ArchiveSource{
public:
	NONS_nsaArchiveSource(const std::wstring &name,bool nsa);
	~NONS_nsaArchiveSource();
	NONS_DataStream *open(const std::wstring &name);
	uchar *read_all(TreeNode *node,size_t &bytes_read);
};

class NONS_zipArchiveSource:public NONS_ArchiveSource{
public:
	NONS_zipArchiveSource(const std::wstring &name);
	~NONS_zipArchiveSource();
	NONS_DataStream *open(const std::wstring &name);
	uchar *read_all(TreeNode *node,size_t &bytes_read);
};

class NONS_ArchiveStream:public NONS_DataStream{
protected:
	Uint64 compressed_offset;
	TreeNode *node;
public:
	NONS_ArchiveStream(NONS_DataSource &ds,const std::wstring &name);
	TreeNode *get_node() const{ return this->node; }
};

class NONS_nsaArchiveStream:public NONS_ArchiveStream{
public:
	NONS_nsaArchiveStream(NONS_DataSource &ds,const std::wstring &name):NONS_ArchiveStream(ds,name){}
	~NONS_nsaArchiveStream(){}
	bool read(void *dst,size_t &bytes_read,size_t count);
};

class NONS_zipArchiveStream:public NONS_ArchiveStream{
public:
	NONS_zipArchiveStream(NONS_DataSource &ds,const std::wstring &name):NONS_ArchiveStream(ds,name){}
	~NONS_zipArchiveStream(){}
	bool read(void *dst,size_t &bytes_read,size_t count);
};

struct base_in_decompression{
	uchar *in_buffer;
	size_t remaining;
	base_in_decompression():in_buffer(0){}
	virtual ~base_in_decompression(){}
	virtual in_func get_f()=0;
};

struct base_out_decompression{
	static const ulong bits=15,
		size=1<<bits;
	std::vector<uchar> out;
	base_out_decompression():out(size){}
	virtual ~base_out_decompression(){}
	virtual out_func get_f()=0;
};

struct decompress_from_regular_file:public base_in_decompression{
	NONS_File *file;
	Uint64 offset;
	std::vector<uchar> in;
	decompress_from_regular_file():base_in_decompression(),offset(0){}
	in_func get_f(){ return in_func; }
	static unsigned in_func(void *p,unsigned char **buffer);
};

struct decompress_from_file:public base_in_decompression{
	Archive *archive;
	TreeNode *node;
	Uint64 offset;
	std::vector<uchar> in;
	decompress_from_file():base_in_decompression(),offset(0){}
	in_func get_f(){ return in_func; }
	static unsigned in_func(void *p,unsigned char **buffer);
};

struct decompress_from_memory:public base_in_decompression{
	uchar *src;
	decompress_from_memory():base_in_decompression(){}
	in_func get_f(){ return in_func; }
	static unsigned in_func(void *p,unsigned char **buffer);
};

struct decompress_to_file:public base_out_decompression{
	NONS_File *file;
	out_func get_f(){ return out_func; }
	static int out_func(void *p,unsigned char *buffer,unsigned size);
};

struct decompress_to_memory:public base_out_decompression{
	void *buffer;
	out_func get_f(){ return out_func; }
	static int out_func(void *p,unsigned char *buffer,unsigned size);
};

struct decompress_to_unknown_size:public base_out_decompression{
	std::list<std::vector<uchar> > *dst;
	size_t final_size;
	decompress_to_unknown_size():base_out_decompression(),final_size(0){}
	out_func get_f(){ return out_func; }
	static int out_func(void *p,unsigned char *buffer,unsigned size);
};
#endif
