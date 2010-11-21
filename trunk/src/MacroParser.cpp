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

#include "Archive.h"
#include "CommandLineOptions.h"
#include "MacroParser.tab.hpp"
#include "../preprocessor.h"
#include "Plugin/LibraryLoader.h"
#include <cassert>

bool preprocess(std::wstring &dst,const std::wstring &script,NONS_Macro::interpreter_state *state);

namespace NONS_Macro{
class interpreter_state{
	std::queue<std::wstring> queue;
	NONS_LibraryLoader lib;
public:
	enum Error{
		SUCCESS=0,
		LIBRARY_NOT_FOUND,
		INVALID_LIBRARY,
		UNDEFINED_ERROR
	} e;
	PP_init_f PP_init;
	PP_get_error_string_f PP_get_error_string;
	PP_free_error_string_f PP_free_error_string;
	PP_destroy_f PP_destroy;
	PP_preprocess_f PP_preprocess;
	PP_done_f PP_done;
	PP_instance *instance;
	ulong calls;
	interpreter_state(const char *script);
	~interpreter_state();
	void push(const std::wstring &s){
		this->queue.push(s);
	}
	std::wstring pop(){
		if (this->queue.empty())
			return L"";
		std::wstring temp=this->queue.front();
		this->queue.pop();
		return temp;
	}
};

interpreter_state::interpreter_state(const char *script):lib("preprocessor",0){
	if (this->lib.error==NONS_LibraryLoader::LIBRARY_NOT_FOUND){
		this->e=LIBRARY_NOT_FOUND;
		return;
	}
	this->PP_init=0;
	this->PP_destroy=0;
	this->PP_get_error_string=0;
	this->PP_free_error_string=0;
	this->PP_preprocess=0;
	this->PP_done=0;
#define IS_GET_FUNCTION(x)                                        \
	this->x=(x##_f)this->lib.getFunction(#x);                     \
	if (this->lib.error==NONS_LibraryLoader::FUNCTION_NOT_FOUND){ \
		this->e=INVALID_LIBRARY;                                  \
		return;                                                   \
	}
	IS_GET_FUNCTION(PP_init);
	IS_GET_FUNCTION(PP_destroy);
	IS_GET_FUNCTION(PP_get_error_string);
	IS_GET_FUNCTION(PP_free_error_string);
	IS_GET_FUNCTION(PP_preprocess);
	IS_GET_FUNCTION(PP_done);
	void *error_data;
	this->instance=this->PP_init(script,&error_data);
	if (!this->instance){
		o_stderr <<this->PP_get_error_string(error_data);
		this->PP_free_error_string(error_data);
		this->e=UNDEFINED_ERROR;
		return;
	}
	this->e=SUCCESS;
}

interpreter_state::~interpreter_state(){
	if (this->PP_destroy)
		this->PP_destroy(this->instance);
}

std::wstring push::to_string(interpreter_state *is){
	assert(is);
	is->push(this->str);
	is->calls++;
	return L"";
}

std::wstring call::to_string(interpreter_state *is){
	assert(is);
	is->calls++;
	PP_parameters p;
	std::wstring pop_signal;
	pop_signal.push_back(0);
	std::string function=UniToUTF8(this->identifier);
	std::vector<std::string> parameters(this->parameters.size());
	for (size_t a=0;a<parameters.size();a++){
		if (this->parameters[a]!=pop_signal)
			parameters[a]=UniToUTF8(this->parameters[a]);
		else
			parameters[a]=UniToUTF8(is->pop());
	}
	std::vector<const char *> cstring_parameters(parameters.size());
	std::vector<size_t> sizes(parameters.size());
	for (size_t a=0;a<parameters.size();a++){
		cstring_parameters[a]=parameters[a].c_str();
		sizes[a]=parameters[a].size();
	}
	p.function=function.c_str();
	p.parameters=&cstring_parameters[0];
	p.sizes=&sizes[0];
	p.array_size=sizes.size();
	PP_result result=is->PP_preprocess(is->instance,p);
	std::wstring string_res=UniFromUTF8(std::string(result.string,result.string_length));
	is->PP_done(is->instance,result);
	if (!result.good){
		o_stderr <<string_res<<"\n";
		return L"";
	}
	return string_res;
}
}

cheap_input_stream::cheap_input_stream(const std::wstring &s)
	:source(&s),offset(0){}

int cheap_input_stream::peek() const{
	if (this->eof())
		return -1;
	const std::vector<wchar_t> &v=this->characters_put_back;
	wchar_t c=(v.size())?v.back():(*this->source)[this->offset];
	return (c!=13)?c:10;
}

int cheap_input_stream::get(){
	if (this->eof())
		return -1;
	wchar_t c;
	std::vector<wchar_t> &v=this->characters_put_back;
	if (v.size()){
		c=v.back();
		v.pop_back();
		if (c==13){
			if (v.size()>=2 && v[v.size()-2]==10)
				v.pop_back();
			c=10;
		}
	}else{
		c=(*this->source)[this->offset++];
		if (c==13){
			if (this->offset<this->source->size() && (*this->source)[this->offset]==10)
				this->offset++;
			c=10;
		}
	}
	return c;
}

std::wstring cheap_input_stream::getline(){
	std::wstring r;
	while (1){
		int c=this->get();
		if (c<0 || c==10)
			break;
		r.push_back((wchar_t)c);
	}
	return r;
}

bool preprocess(std::wstring &dst,const std::wstring &script,NONS_Macro::interpreter_state *state){
	NONS_Macro::file *f=0;
	{
		cheap_input_stream stream=script;
		macroParser_yydebug=1;
		ParserState::ParserState state=ParserState::TEXT;
		if (!!macroParser_yyparse(stream,state,f))
			return 0;
	}
	for (size_t a=0;a<f->list.size();a++)
		dst.append(f->list[a]->to_string(state));
	delete f;
	return 1;
}

bool preprocess(std::wstring &dst,const std::wstring &script){
	bool r=0;
	std::string python_script;
	{
		size_t size;
		char *s=(char *)general_archive.getFileBuffer(L"macros.py",size);
		if (!s)
			return r;
		python_script.assign(s,size);
		delete[] s;
	}
	NONS_Macro::interpreter_state is(python_script.c_str());
	r=preprocess(dst,script,&is);
	if (!r)
		return 0;
	while (1){
		std::wstring temp;
		is.calls=0;
		if (preprocess(temp,dst,&is) && is.calls){
			dst=temp;
			continue;
		}
		break;
	}
	return 1;
}
