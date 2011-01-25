/*
* Copyright (c) 2008-2011, Helios (helios.vmg@gmail.com)
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

#ifndef NONS_MACROPARSER_H
#define NONS_MACROPARSER_H
#include "Common.h"
#include "enums.h"
#include <string>
#include <vector>
#include <queue>

class cheap_input_stream{
	const std::wstring *source;
	std::vector<wchar_t> characters_put_back;
	size_t offset;
	cheap_input_stream(const cheap_input_stream &){}
	cheap_input_stream &operator=(const cheap_input_stream &){}
public:
	cheap_input_stream(const std::wstring &);
	int peek() const;
	int get();
	bool eof() const{ return !this->characters_put_back.size() && this->offset>=this->source->size(); }
	void putback(wchar_t c){ this->characters_put_back.push_back(c); }
	std::wstring getline();
	std::wstring get_all_remaining();
};

namespace NONS_Macro{
class interpreter_state;

struct file_element{
	virtual std::wstring to_string(interpreter_state * =0)=0;
};

struct text:public file_element{
	std::wstring str;
	std::wstring to_string(interpreter_state * =0);
	text(const std::wstring &s):str(s){}
};

struct file;

struct call:public file_element{
	std::wstring identifier;
	std::vector<std::wstring> parameters;
	ulong times_delayed;
	call(const std::wstring &s,ulong d=0):identifier(s),times_delayed(d){}
	std::wstring to_string(interpreter_state * =0);
};

struct file{
	std::vector<file_element *> list;
	std::wstring to_string(interpreter_state * =0);
};
};
#endif
