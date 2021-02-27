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

#ifndef FILE_H
#define FILE_H
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
#include <boost/cstdint.hpp>
#include <string>
#include <fstream>

typedef boost::uint16_t Uint16;
typedef boost::int16_t  Sint16;
typedef boost::uint32_t Uint32;
typedef boost::int32_t  Sint32;
typedef boost::uint64_t Uint64;
typedef boost::int64_t  Sint64;

class File{
#if NONS_SYS_WINDOWS
	void *
#elif NONS_SYS_UNIX
	int
#else
	std::fstream
#endif
		file;
	bool opened_for_read;
	bool is_open;
	Uint64 _filesize;
	File(const File &){}
	const File &operator=(const File &){ return *this; }
	Uint64 reload_filesize();
public:
	typedef unsigned char type;
	File():is_open(0){}
	File(const std::wstring &path,bool open_for_read,bool truncate=1);
	~File(){ this->close(); }
	void open(const std::wstring &path,bool open_for_read,bool truncate=1);
	void close();
	bool operator!();
	bool read(void *dst,size_t read_bytes,size_t &bytes_read,Uint64 offset);
	bool read(void *dst,size_t &bytes_read){ return this->read(dst,(size_t)this->_filesize,bytes_read,0); }
	type *read(size_t read_bytes,size_t &bytes_read,Uint64 offset);
	type *read(size_t &bytes_read){ return this->read((size_t)this->_filesize,bytes_read,0); }
	bool write(const void *buffer,size_t size,bool write_at_end=1);
	bool write_at_offset(const void *buffer,size_t size,Uint64 offset);
	Uint64 filesize(){ return (this->opened_for_read)?this->_filesize:this->reload_filesize(); }
	static type *read(const std::wstring &path,size_t read_bytes,size_t &bytes_read,Uint64 offset);
	static type *read(const std::wstring &path,size_t &bytes_read);
	static bool write(const std::wstring &path,const void *buffer,size_t size);
	static bool delete_file(const std::wstring &path);
	static bool file_exists(const std::wstring &name);
	static bool get_file_size(Uint64 &size,const std::wstring &name);
};
#endif
