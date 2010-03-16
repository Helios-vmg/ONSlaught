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

#ifndef NONS_COMMANDLINEOPTIONS_H
#define NONS_COMMANDLINEOPTIONS_H

#include "enums.h"
#include "Common.h"
#include <string>
#include <vector>

struct NONS_CommandLineOptions{
	ENCODINGS scriptencoding;
	std::wstring musicFormat;
	std::wstring musicDirectory;
	std::wstring archiveDirectory;
	std::wstring scriptPath;
	ENCRYPTION scriptEncryption;
	bool override_stdout;
	bool reset_redirection_files;
	bool debugMode;
	bool noconsole;
	ushort virtualWidth,virtualHeight,
		realWidth,realHeight;
	bool startFullscreen;
	uchar verbosity;
	bool no_sound;
	std::wstring savedir;
	bool stopOnFirstError;
	bool listImplementation;
	bool outputPreprocessedFile;
	std::wstring preprocessedFile;
	bool noThreads;
	bool preprocessAndQuit;
	std::wstring play;
	bool play_from_archive;
	NONS_CommandLineOptions();
	~NONS_CommandLineOptions(){}
	void parse(const std::vector<std::wstring> &arguments);
};

extern NONS_CommandLineOptions CLOptions;

void usage();
#endif
