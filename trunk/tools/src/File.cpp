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

#include "File.h"

#if NONS_SYS_WINDOWS
#include <windows.h>
#elif NONS_SYS_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

File::File(const std::wstring &path,bool read,bool truncate){
	this->is_open=0;
#if NONS_SYS_WINDOWS
	this->file=INVALID_HANDLE_VALUE;
#elif NONS_SYS_UNIX
	this->file=0;
#endif
	this->open(path,read,truncate);
}

void File::open(const std::wstring &path,bool open_for_read,bool truncate){
	this->close();
	this->opened_for_read=open_for_read;
#if NONS_SYS_WINDOWS
	this->file=CreateFile(
		&path[0],
		(open_for_read?GENERIC_READ:GENERIC_WRITE),
		(open_for_read?FILE_SHARE_READ:0),
		0,
		((open_for_read || !truncate)?OPEN_EXISTING:TRUNCATE_EXISTING),
		FILE_ATTRIBUTE_NORMAL,
		0
	);
	if (this->file==INVALID_HANDLE_VALUE && GetLastError()==ERROR_FILE_NOT_FOUND){
		this->file=CreateFile(
			&path[0],
			(open_for_read?GENERIC_READ:GENERIC_WRITE),
			(open_for_read?FILE_SHARE_READ:0),
			0,
			(open_for_read?OPEN_EXISTING:CREATE_NEW),
			FILE_ATTRIBUTE_NORMAL,
			0
		);
	}
#elif NONS_SYS_UNIX
	this->file=open(
		UniToUTF8(path).c_str(),
		(open_for_read?O_RDONLY:O_WRONLY|O_TRUNC)|O_LARGEFILE
	);
	if (this->file<0)
		this->file=open(
			UniToUTF8(path).c_str(),
			(open_for_read?O_RDONLY:O_WRONLY|O_CREAT)|O_LARGEFILE
		);
#else
	this->file.open(UniToUTF8(path).c_str(),std::ios::binary|(open_for_read?std::ios::in:std::ios::out));
#endif
	this->_filesize=this->reload_filesize();
}

Uint64 File::reload_filesize(){
	if (this->is_open=!!*this){
#if NONS_SYS_WINDOWS
		LARGE_INTEGER li;
		return (GetFileSizeEx(this->file,&li))?li.QuadPart:0;
#elif NONS_SYS_UNIX
		return (Uint64)lseek64(this->file,0,SEEK_END);
#else
		if (this->opened_for_read){
			this->file.seekg(0,std::ios::end);
			return this->file.tellg();
		}else{
			this->file.seekp(0,std::ios::end);
			return this->file.tellp();
		}
#endif
	}
	return 0;
}

void File::close(){
	if (!this->is_open)
		return;
#if NONS_SYS_WINDOWS
	if (!!*this)
		CloseHandle(this->file);
#elif NONS_SYS_UNIX
	if (!!*this)
		close(this->file);
#else
	this->file.close();
#endif
	this->is_open=0;
}
	
bool File::operator!(){
	return
#if NONS_SYS_WINDOWS
		this->file==INVALID_HANDLE_VALUE;
#elif NONS_SYS_UNIX
		this->file>=0;
#else
		!this->file;
#endif
}

bool File::read(void *dst,size_t read_bytes,size_t &bytes_read,Uint64 offset){
	if (!this->is_open || !this->opened_for_read){
		bytes_read=0;
		return 0;
	}
	if (offset>=this->_filesize){
		bytes_read=0;
		return 0;
	}
#if NONS_SYS_WINDOWS
	{
		LARGE_INTEGER temp;
		temp.QuadPart=offset;
		SetFilePointerEx(this->file,temp,0,FILE_BEGIN);
	}
#elif NONS_SYS_UNIX
	lseek64(this->file,offset,SEEK_SET);
#else
	this->file.seekg((size_t)offset);
#endif
	if (this->_filesize-offset<read_bytes)
		read_bytes=size_t(this->_filesize-offset);
	if (dst){
#if NONS_SYS_WINDOWS
		DWORD rb=read_bytes,br=0;
		ReadFile(this->file,dst,rb,&br,0);
#elif NONS_SYS_UNIX
		read(this->file,dst,read_bytes);
#else
		this->file.read((char *)dst,read_bytes);
#endif
	}
	bytes_read=read_bytes;
	return 1;
}

File::type *File::read(size_t read_bytes,size_t &bytes_read,Uint64 offset){
	if (!this->read(0,read_bytes,bytes_read,offset)){
		bytes_read=0;
		return 0;
	}
	File::type *buffer=new File::type[bytes_read];
	this->read(buffer,read_bytes,bytes_read,offset);
	return buffer;
}

bool File::write(const void *buffer,size_t size,bool write_at_end){
	if (!this->is_open || this->opened_for_read)
		return 0;
#if NONS_SYS_WINDOWS
	if (!write_at_end)
		SetFilePointer(this->file,0,0,FILE_BEGIN);
	else
		SetFilePointer(this->file,0,0,FILE_END);
	DWORD a=size;
	WriteFile(this->file,buffer,a,&a,0);
#elif NONS_SYS_UNIX
	if (!write_at_end)
		lseek64(this->file,0,SEEK_SET);
	else
		lseek64(this->file,0,SEEK_END);
	write(this->file,buffer,size);
#else
	if (!write_at_end)
		this->file.seekp(0);
	else
		this->file.seekp(0,std::ios::end);
	this->file.write((char *)buffer,size);
#endif
	return 1;
}

bool File::write_at_offset(const void *buffer,size_t size,Uint64 offset){
	if (!this->is_open || this->opened_for_read)
		return 0;
#if NONS_SYS_WINDOWS
	LARGE_INTEGER li;
	li.QuadPart=offset;
	SetFilePointerEx(this->file,li,&li,FILE_BEGIN);
	if (li.QuadPart!=offset)
		return 0;
	DWORD a=size;
	WriteFile(this->file,buffer,a,&a,0);
	
#elif NONS_SYS_UNIX
	if (lseek64(this->file,offset,SEEK_SET)!=offset)
		return 0;
	write(this->file,buffer,size);
#else
	this->file.seekp((std::streampos)0);
	if (this->file.tellp()!=offset)
		return 0;
	this->file.write((char *)buffer,size);
#endif
	return 1;
}

File::type *File::read(const std::wstring &path,size_t read_bytes,size_t &bytes_read,Uint64 offset){
	File file(path,1);
	return file.read(read_bytes,bytes_read,offset);
}

File::type *File::read(const std::wstring &path,size_t &bytes_read){
	File file(path,1);
	return file.read(bytes_read);
}

bool File::write(const std::wstring &path,const void *buffer,size_t size){
	File file(path,0);
	return file.write(buffer,size);
}

bool File::delete_file(const std::wstring &path){
#if NONS_SYS_WINDOWS
	return !!DeleteFile(&path[0]);
#else
	std::string s=UniToUTF8(path);
	return remove(&s[0])==0;
#endif
}

bool File::file_exists(const std::wstring &name){
	bool ret;
#if NONS_SYS_WINDOWS
	HANDLE file=CreateFile(&name[0],0,0,0,OPEN_EXISTING,0,0);
	ret=(file!=INVALID_HANDLE_VALUE);
	CloseHandle(file);
#else
	std::ifstream file(UniToUTF8(name).c_str());
	ret=!!file;
	file.close();
#endif
	return ret;
}

bool File::get_file_size(Uint64 &size,const std::wstring &name){
	File file(name,1);
	if (!file)
		return 0;
	size=file.filesize();
	return 1;
}
