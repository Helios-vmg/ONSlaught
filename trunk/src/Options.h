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

#ifndef NONS_COMMANDLINEOPTIONS_H
#define NONS_COMMANDLINEOPTIONS_H

#include "enums.h"
#include "Common.h"
#include "tinyxml/tinyxml.h"
#include <string>
#include <vector>
#include <map>

static const int VERBOSITY_LOG_NOTHING=0,
	VERBOSITY_LOG_LINE_NUMBERS=1,
	VERBOSITY_LOG_OPEN_STREAMS=2,
	VERBOSITY_LOG_OPEN_FAILURES=3,
	VERBOSITY_LOG_EXPRESSIONS=4,
	VERBOSITY_LOG_EVERYTHING=5,
	VERBOSITY_RESERVED=255;

struct NONS_CommandLineOptions{
	ENCODING::ENCODING scriptencoding;
	std::wstring musicFormat;
	std::wstring musicDirectory;
	std::wstring scriptPath;
	ENCRYPTION::ENCRYPTION scriptEncryption;
#ifndef NONS_NO_STDOUT
	bool override_stdout;
	bool reset_redirection_files;
#endif
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
	typedef std::map<wchar_t,wchar_t> replaceArray_t;
	replaceArray_t replaceArray;
	bool use_long_audio_buffers;
	std::wstring default_font,
		console_font;
	bool never_clear_log;
	NONS_CommandLineOptions();
	~NONS_CommandLineOptions(){}
	void parse(const std::vector<std::wstring> &arguments);
};

extern NONS_CommandLineOptions CLOptions;

void usage();

class NONS_Settings{
	TiXmlDocument doc;
	std::wstring path;
	void load_text_speed(TiXmlElement *settings);
	void save_text_speed(TiXmlElement *settings);
	void load_mute(TiXmlElement *settings);
	void save_mute(TiXmlElement *settings);
public:
	~NONS_Settings(){
		this->save();
	}
	void init(const std::wstring &path);
	void save();
	template <typename T>
	struct setting{
		bool set;
		T data;
		setting():set(0),data(){}
	};
	setting<int> text_speed;
	setting<bool> mute;
};

extern NONS_Settings settings;
#endif
