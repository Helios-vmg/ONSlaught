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

#include "LibraryLoader.h"
#include "../Functions.h"
#include <string>
#include <iostream>

#if NONS_SYS_WINDOWS
#include <windows.h>
#elif NONS_SYS_UNIX
#include <dlfcn.h>
#endif

NONS_LibraryLoader::NONS_LibraryLoader(const char *libName,bool append_debug){
#if NONS_SYS_WINDOWS
	std::wstring filename=UniFromISO88591(libName);
#ifdef _DEBUG
	if (append_debug)
		filename.append(L"_d");
#endif
	filename.append(L".dll");
	this->lib=LoadLibrary(filename.c_str());
#elif NONS_SYS_UNIX
	std::string filename="lib";
	filename.append(libName);
	filename.append(".so");
	this->lib=dlopen(filename.c_str(),RTLD_LAZY);
#endif
	this->error=reason((!this->lib)?LIBRARY_NOT_FOUND:NO_ERROR);
}

NONS_LibraryLoader::~NONS_LibraryLoader(){
	if (!this->lib)
		return;
#if NONS_SYS_WINDOWS
	FreeLibrary((HMODULE)this->lib);
#else
	dlclose(this->lib);
#endif
}

void *NONS_LibraryLoader::getFunction(const char *functionName){
	if (!this->lib)
		return 0;
	void *ret=
#if NONS_SYS_WINDOWS
		(void *)GetProcAddress((HMODULE)this->lib,functionName);
#else
		dlsym(this->lib,functionName);
#endif
	this->error=reason((!ret)?FUNCTION_NOT_FOUND:NO_ERROR);
	return ret;
}

NONS_LibraryLoader pluginLibrary("plugin");
