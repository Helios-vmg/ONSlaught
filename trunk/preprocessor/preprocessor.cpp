/*
* Copyright (c) 2010, Helios (helios.vmg@gmail.com)
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

#include "../preprocessor.h"
#include <vector>
#include <sstream>
#include <string>

#ifdef _DEBUG
#undef _DEBUG
#define PP_DEBUG
#endif
#include <python.h>
#include <frameobject.h>
#ifdef PP_DEBUG
#undef PP_DEBUG
#define _DEBUG
#endif

std::string PyUnicode_AsStdString(PyObject *o){
	if (!o || !PyUnicode_Check(o))
		return "";
	PyObject *utf8_string=PyUnicode_AsUTF8String(o);
	char *temp;
	Py_ssize_t size;
	PyBytes_AsStringAndSize(utf8_string,&temp,&size);
	assert(temp);
	std::string ret(temp,size);
	Py_DECREF(utf8_string);
	return ret;
}

std::string PyObject_AsStdString(PyObject *o){
	PyObject *str=PyObject_Str(o);
	std::string ret=PyUnicode_AsStdString(str);
	Py_DECREF(str);
	return ret;
}

std::string PyTypeObject_AsStdString(PyObject *o){
	std::string ret=PyObject_AsStdString(o);
	return ret.substr(8,ret.size()-10);
}

std::string get_python_error(){
	std::string dst;
	PyObject *type=0,
		*value=0,
		*tb=0;
	std::stringstream stream;
	PyErr_Fetch(&type,&value,&tb);
	if (tb){
		PyErr_NormalizeException(&type,&value,&tb);
		PyTracebackObject *traceback=(PyTracebackObject *)tb;
		stream <<"Traceback:\n";
		for (;traceback;traceback=traceback->tb_next){
			stream
				<<"  File \""
				<<PyUnicode_AsStdString(traceback->tb_frame->f_code->co_filename)
				<<"\", line "
				<<traceback->tb_lineno
				<<", in "
				<<PyUnicode_AsStdString(traceback->tb_frame->f_code->co_name)
				<<std::endl;
		}
	}
	if (value && type){
		stream <<PyTypeObject_AsStdString(type)<<": "<<PyObject_AsStdString(value)<<std::endl;
		dst=stream.str();
	}
	Py_XDECREF(tb);
	Py_XDECREF(value);
	Py_XDECREF(type);
	return dst;
}

struct PP_instance{
	bool good;
	std::vector<std::string *> strings;
	PyObject *module;
	PP_instance(const char *file,std::string &error_string){
		this->good=0;
		Py_Initialize();
		this->module=0;
		PyObject *compiled=Py_CompileString(file,"macros.txt",Py_file_input);
		if (compiled){
			this->module=PyImport_ExecCodeModule("macros",compiled);
			Py_DECREF(compiled);
			this->good=!!this->module;
			if (this->good)
				return;
		}
		error_string=get_python_error();
	}
	~PP_instance(){
		for (size_t a=0;a<this->strings.size();a++)
			delete strings[a];
		Py_XDECREF(this->module);
		Py_Finalize();
	}
	size_t new_result_string(){
		for (size_t a=0;a<this->strings.size();a++){
			if (!this->strings[a]){
				this->strings[a]=new std::string;
				return a;
			}
		}
		this->strings.push_back(new std::string);
		return this->strings.size()-1;
	}
};

extern "C" PP_DLLexport PP_instance *PP_init(const char *s,void **data){
	std::string error;
	PP_instance *r=new PP_instance(s,error);
	if (!r->good){
		delete r;
		*data=new std::string(error);
		return 0;
	}
	return r;
}

extern "C" PP_DLLexport const char *PP_get_error_string(void *data){
	return ((std::string *)data)->c_str();
}

extern "C" PP_DLLexport void PP_free_error_string(void *data){
	delete (std::string *)data;
}

extern "C" PP_DLLexport void PP_destroy(PP_instance *i){
	delete i;
}

bool call_python(PP_instance *i,std::string &dst,const std::string &function_name,const std::vector<std::string> &parameters){
	bool ret=0;
	PyObject *module=0,
		*function=0,
		*return_value=0,
		*py_parameters=0;
	if (!i->good){
		dst="module doesn't contain the symbol";
		goto call_python_end;
	}
	module=i->module;
	function=PyObject_GetAttrString(module,function_name.c_str());
	if (!function){
		dst="module doesn't contain the symbol";
		goto call_python_end;
	}
	if (!PyCallable_Check(function)){
		dst="module contains the symbol but it isn't callable";
		goto call_python_end;
	}
	py_parameters=PyTuple_New(parameters.size());
	assert(py_parameters);
	for (size_t a=0;a<parameters.size();a++){
		const std::string &s=parameters[a];
		PyObject *string=PyUnicode_Decode(s.c_str(),s.size(),"UTF-8",0);
		assert(string);
		PyTuple_SetItem(py_parameters,a,string);
	}
	return_value=PyObject_CallObject(function,py_parameters);
	Py_DECREF(py_parameters);
	if (!return_value){
		dst="function didn't return a value";
		goto call_python_end;
	}
	if (!PyUnicode_Check(return_value))
		dst="function didn't return a string";
	else{
		dst=PyUnicode_AsStdString(return_value);
		ret=1;
	}
call_python_end:
	if (!ret)
		dst=get_python_error();
	Py_XDECREF(return_value);
	Py_XDECREF(function);
	return ret;
}

extern "C" PP_DLLexport PP_result PP_preprocess(PP_instance *i,PP_parameters p){
	std::vector<std::string> parameters(p.array_size);
	for (size_t a=0;a<p.array_size;a++)
		parameters[a].assign(p.parameters[a],p.sizes[a]);
	PP_result r;
	std::string *dst=i->strings[r.index=i->new_result_string()];
	r.good=call_python(i,*dst,p.function,parameters);
	r.string=dst->c_str();
	r.string_length=dst->size();
	return r;
}

extern "C" PP_DLLexport void PP_done(PP_instance *i,PP_result r){
	if (!i || r.index>=i->strings.size() || !i->strings[r.index])
		return;
	delete i->strings[r.index];
	i->strings[r.index]=0;
}
