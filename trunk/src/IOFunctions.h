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

#ifndef NONS_IOFUNCTIONS_H
#define NONS_IOFUNCTIONS_H

#include "Common.h"
#include "Functions.h"
#include "ErrorCodes.h"
#include "VirtualScreen.h"
#include "ThreadManager.h"
#include <SDL/SDL.h>
#include <string>
#include <fstream>
#include <queue>

class NONS_EventQueue{
	std::queue<SDL_Event> data;
	NONS_Mutex mutex;
public:
	NONS_EventQueue();
	~NONS_EventQueue();
	void push(SDL_Event a);
	SDL_Event pop();
	//returns true if SDL_QUIT was found in the queue
	bool emptify();
	bool empty();
	void WaitForEvent(int delay=100);
};

struct NONS_InputObserver{
	std::vector<NONS_EventQueue *> data;
	NONS_Mutex mutex;
	NONS_InputObserver();
	~NONS_InputObserver();
	void attach(NONS_EventQueue *what);
	void detach(NONS_EventQueue *what);
	void notify(SDL_Event *event);
};

ErrorCode handleErrors(ErrorCode error,ulong original_line,const char *caller,bool queue,std::wstring extraInfo=L"");
void waitUntilClick(NONS_EventQueue *queue=0);
void waitCancellable(long delay,NONS_EventQueue *queue=0);
void waitNonCancellable(long delay);
Uint8 getCorrectedMousePosition(NONS_VirtualScreen *screen,int *x,int *y);
inline std::ostream &operator<<(std::ostream &stream,const std::wstring &str){
	return stream<<UniToUTF8(str);
}

class NONS_File{
#if NONS_SYS_WINDOWS
	HANDLE
#else
	std::fstream
#endif
		file;
	bool opened_for_read;
	bool is_open;
	NONS_File(const NONS_File &){}
	const NONS_File &operator=(const NONS_File &){ return *this; }
public:
	typedef uchar type;
	NONS_File():is_open(0){}
	NONS_File(const std::wstring &path,bool open_for_read);
	~NONS_File(){ this->close(); }
	void open(const std::wstring &path,bool open_for_read);
	void close();
	bool operator!();
	type *read(ulong read_bytes,ulong &bytes_read,ulong offset);
	type *read(ulong &bytes_read);
	bool write(void *buffer,ulong size,bool write_at_end=1);
	ulong filesize();
	static type *read(const std::wstring &path,ulong read_bytes,ulong &bytes_read,ulong offset);
	static type *read(const std::wstring &path,ulong &bytes_read);
	static bool write(const std::wstring &path,void *buffer,ulong size);
	static bool delete_file(const std::wstring &path);
};
bool fileExists(const std::wstring &name);

struct NONS_CommandLineOptions;
extern NONS_CommandLineOptions CLOptions;

#if NONS_SYS_WINDOWS
class VirtualConsole{
	HANDLE near_end,
		far_end,
		process;
public:
	bool good;
	VirtualConsole(const std::string &name,ulong color);
	~VirtualConsole();
	void put(const char *str,size_t size=0);
	void put(const std::string &str){
		this->put(str.c_str(),str.size());
	}
};
#endif

#define INDENTATION_STRING "    "

struct NONS_RedirectedOutput{
	std::ofstream *file;
	std::ostream &cout;
#if NONS_SYS_WINDOWS
	VirtualConsole *vc;
#endif
	ulong indentation;
	bool addIndentationNext;
	NONS_RedirectedOutput(std::ostream &a);
	~NONS_RedirectedOutput();
	NONS_RedirectedOutput &operator<<(ulong);
	NONS_RedirectedOutput &outputHex(ulong,ulong=0);
	NONS_RedirectedOutput &operator<<(long);
	NONS_RedirectedOutput &operator<<(wchar_t);
	NONS_RedirectedOutput &operator<<(const char *);
	NONS_RedirectedOutput &operator<<(const std::string &);
	NONS_RedirectedOutput &operator<<(const std::wstring &);
	void redirect();
	//std::ostream &getstream();
	void indent(long);
private:
	void write_to_stream(const std::stringstream &str);
};

extern NONS_InputObserver InputObserver;
extern NONS_RedirectedOutput o_stdout;
extern NONS_RedirectedOutput o_stderr;
//extern NONS_RedirectedOutput o_stdlog;
#endif
