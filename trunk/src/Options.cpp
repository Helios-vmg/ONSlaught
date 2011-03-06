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

#include "Options.h"
#include "ScriptInterpreter.h"
#include "IOFunctions.h"
#include <SDL/SDL_keysym.h>
#include <iostream>

NONS_CommandLineOptions CLOptions;
extern std::ofstream textDumpFile;

#define DEFAULT_INPUT_WIDTH 640
#define DEFAULT_INPUT_HEIGHT 480
//#define PSP_RESOLUTION
//#define BIG_RESOLUTION
#ifdef PSP_RESOLUTION
#define DEFAULT_OUTPUT_WIDTH 480
#define DEFAULT_OUTPUT_HEIGHT 272
#elif defined(BIG_RESOLUTION)
#define DEFAULT_OUTPUT_WIDTH 1024
#define DEFAULT_OUTPUT_HEIGHT 768
#else
#define DEFAULT_OUTPUT_WIDTH DEFAULT_INPUT_WIDTH
#define DEFAULT_OUTPUT_HEIGHT DEFAULT_INPUT_HEIGHT
#endif

NONS_CommandLineOptions::NONS_CommandLineOptions(){
	this->scriptencoding=ENCODING::AUTO;
	this->scriptEncryption=ENCRYPTION::NONE;
#ifndef NONS_NO_STDOUT
	this->override_stdout=1;
	this->reset_redirection_files=1;
#endif
	this->debugMode=0;
	this->noconsole=0;
	this->virtualWidth=DEFAULT_INPUT_WIDTH;
	this->virtualHeight=DEFAULT_INPUT_HEIGHT;
	this->realWidth=DEFAULT_OUTPUT_WIDTH;
	this->realHeight=DEFAULT_OUTPUT_HEIGHT;
	this->startFullscreen=0;
	this->verbosity=VERBOSITY_LOG_NOTHING;
	this->no_sound=0;
	this->stopOnFirstError=0;
	this->listImplementation=0;
	this->outputPreprocessedFile=0;
	this->noThreads=0;
	this->preprocessAndQuit=0;
	this->use_long_audio_buffers=0;
	this->default_font=L"default.ttf";
	this->console_font=L"cour.ttf";
}

void usage(){
	o_stdout <<"Usage: ONSlaught [options]\n"
		"Options:\n"
		"  -h\n"
		"  -?\n"
		"  --help\n"
		"      Display this message.\n"
		"  --version\n"
		"      Display version number.\n"
		"  -implementation\n"
		"      Lists all implemented and unimplemented commands.\n"
		"  -verbosity <number>\n"
		"      Set log verbosity level. 0 by default.\n"
		"  -save-directory <directory name>\n"
		"      Override automatic save game directory selection.\n"
		"      See the documentation for more information.\n"
		"  -archive-directory <directory>\n"
		"      Set where to look for archive files.\n"
		"      Default is \".\"\n"
		"  -f\n"
		"      Start in fullscreen.\n"
		"  -r <virtual width> <virtual height> <real width> <real height>\n"
		"      Sets the screen resolution. The first two numbers are width and height of\n"
		"      the virtual screen. The second two numbers are width and height of the\n"
		"      physical screen or window graphical output will go to.\n"
		"      See the documentation for more information.\n"
		"  -script {auto|<path> {0|1|2|3}}\n"
		"      Select the path and encryption method used by the script.\n"
		"      Default is \'auto\'. On auto, this is the priority order for files:\n"
		"          1. \"0.txt\", method 0\n"
		"          2. \"00.txt\", method 0\n"
		"          3. \"nscr_sec.dat\", method 2\n"
		"          4. \"nscript.___\", method 3\n"
		"          5. \"nscript.dat\", method 1\n"
		"      The documentation contains a detailed description on each of the modes.\n"
		"  -encoding {auto|sjis|iso-8859-1|utf8|ucs2}\n"
		"      Select the encoding to be used for the script.\n"
		"      Default is \'auto\'.\n"
		"  -s\n"
		"      No sound.\n"
		"  -music-format {auto|ogg|mp3|mid|it|xm|s3m|mod}\n"
		"      Select the music format to be used.\n"
		"      Default is \'auto\'.\n"
		"  -music-directory <directory>\n"
		"      Set where to look for music files.\n"
		"      Default is \"./CD\"\n"
		"  -debug\n"
		"      Enable debug mode.\n"
		"      See the documentation for more information.\n"
#if NONS_SYS_WINDOWS && !defined NONS_NO_STDOUT
		"  -no-console\n"
		"      Hide the console.\n"
#endif
#ifndef NONS_NO_STDOUT
		"  -redirect\n"
		"      Redirect stdout and stderr to \"stdout.txt\" and \"stderr.txt\"\n"
		"      correspondingly.\n"
		"      If -debug has been used, it is disabled.\n"
		"      By default, output is redirected.\n"
		"  -!redirect\n"
		"      Sends the output to the console instead of the file system.\n"
		"      See \"-redirect\" for more info.\n"
		"  -!reset-out-files\n"
		"      Only used with \"-redirect\".\n"
		"      Keeps the contents of stdout.txt, stderr.txt, and stdlog.txt when it\n"
		"      opens them and puts the date and time as identification.\n"
#endif
		"  -stop-on-first-error\n"
		"      Stops executing the script when the first error occurs. \"Unimplemented\n"
		"      command\" (when the command will not be implemented) errors don't count.\n"
		"  -pp-output <filename>\n"
		"      Writes the preprocessor output to <filename>. The details of each macro\n"
		"      call are sent to stderr.\n"
		"  -pp-then-quit\n"
		"      Preprocesses the script and quits. Only makes sense when used with\n"
		"      -pp-output.\n"
		"  -disable-threading\n"
		"      Disables threading for blit operations.\n"
		"  -play <filename>\n"
		"      Play the file and quit. The file can be a graphics, audio or video file.\n"
		"      This option can be used to test whether the engine can find and read the\n"
		"      file.\n"
		"  -replace <replacement string>\n"
		"      Sets characters to be replaced in the printing mechanism.\n"
		"      See the documentation for more information.\n"
		"  -use-long-audio-buffers\n"
		"      Allocates longer audio buffers. This fixes some problems with sound in\n"
		"      older systems.\n"
		"-default-font <filename>\n"
		"Use <filename> as the main font. Defaults to \"default.ttf\".\n"
		"-console-font <filename>\n"
		"Use <filename> as the font for the debugging console. Defaults to \"cour.ttf\".\n"
	;
	exit(0);
}

extern const wchar_t *sound_formats[];

void NONS_CommandLineOptions::parse(const std::vector<std::wstring> &arguments){
	static const wchar_t *options[]={
		L"--help",                  //0
		L"-script",                 //1
		L"-encoding",               //2
		L"-music-format",           //3
		L"-music-directory",        //4
		L"",                        //5
		L"",                        //6
		L"",                        //7
		L"-debug",                  //8
		L"-redirect",               //9
		L"--version",               //10
		L"-implementation",         //11
		L"",                        //12
		L"-dump-text",              //13
		L"-f",                      //14
		L"-r",                      //15
		L"-verbosity",              //16
		L"-sdebug",                 //17
		L"-s",                      //18
		L"-h",                      //19
		L"-?",                      //20
		L"-save-directory",         //21
		L"-!reset-out-files",       //22
		L"-!redirect",              //23
		L"-stop-on-first-error",    //24
		L"-archive-directory",      //25
		L"-pp-output",              //26
		L"-disable-threading",      //27
		L"-pp-then-quit",           //28
		L"-play",                   //29
		L"-replace",                //30
		L"-use-long-audio-buffers", //31
		L"-default-font",           //32
		L"-console-font",           //33
		0
	};

	for (ulong a=0,size=arguments.size();a<size;a++){
		long option=-1;
		for (long b=0;options[b] && option<0;b++){
			if (arguments[a]==options[b])
				option=b;
		}
		switch(option){
			case 0: //--help
			case 19: //-h
			case 20: //-?
				usage();
			case 1: //-script
				if (a+1>=size){
					STD_CERR <<"Invalid argument syntax: \""<<arguments[a]<<"\"\n";
					break;
				}
				if (arguments[++a]==L"auto"){
					this->scriptencoding=ENCODING::AUTO;
					break;
				}
				if (a+1>=size){
					STD_CERR <<"Invalid argument syntax: \""<<arguments[a-1]<<"\"\n";
					break;
				}
				this->scriptPath=arguments[a];
				this->scriptEncryption=(ENCRYPTION::ENCRYPTION)atol(arguments[++a]);
				break;
			case 2: //-encoding
				if (a+1>=size){
					STD_CERR <<"Invalid argument syntax: \""<<arguments[a]<<"\"\n";
					break;
				}
				if (arguments[++a]==L"auto"){
					this->scriptencoding=ENCODING::AUTO;
					break;
				}
				if (arguments[a]==L"sjis"){
					this->scriptencoding=ENCODING::SJIS;
					break;
				}
				if (arguments[a]==L"iso-8859-1"){
					this->scriptencoding=ENCODING::ISO_8859_1;
					break;
				}
				if (arguments[a]==L"utf8"){
					this->scriptencoding=ENCODING::UTF8;
					break;
				}
				STD_CERR <<"Unrecognized encoding: \""<<arguments[a]<<"\"\n";
				break;
			case 3: //-music-format
				{
					if (a+1>=size){
						STD_CERR <<"Invalid argument syntax: \""<<arguments[a]<<"\"\n";
						break;
					}
					if (arguments[++a]==L"auto"){
						this->musicFormat.clear();
						break;
					}
					long format=-1;
					for (ulong b=0;sound_formats[b] && format<0;b++)
						if (arguments[a]==sound_formats[b])
							format=b;
					if (format>=0)
						this->musicFormat=arguments[a];
					else
						STD_CERR <<"Unrecognized music format: \""<<arguments[a]<<"\"\n";
				}
				break;
			case 4: //-music-directory
				if (a+1>=size){
					STD_CERR <<"Invalid argument syntax: \""<<arguments[a]<<"\"\n";
					break;
				}
				this->musicDirectory=arguments[++a];
				toforwardslash(this->musicDirectory);
				break;
			case 5: //-transparency-method-layer
				break;
			case 6: //-transparency-method-anim
				break;
			case 8: //-debug
				this->debugMode=1;
				this->noconsole=0;
				break;
#ifndef NONS_NO_STDOUT
			case 9: //-redirect
				this->override_stdout=1;
				this->debugMode=0;
				break;
#endif
			case 11: //-implementation
				this->listImplementation=1;
			case 10: //--version
				{
					NONS_ScriptInterpreter(0);
				}
				exit(0);
			case 13: //-dump-text
				{
					if (a+1>=size){
						STD_CERR <<"Invalid argument syntax: \""<<arguments[a]<<"\"\n";
						break;
					}
					textDumpFile.open(UniToUTF8(arguments[++a]).c_str(),std::ios::app);
				}
				break;
			case 14: //-f
				this->startFullscreen=1;
				break;
			case 15: //-r
				if (a+4>=size){
					STD_CERR <<"Invalid argument syntax: \""<<arguments[a]<<"\"\n";
					break;
				}
				this->virtualWidth=(ushort)atol(arguments[++a]);
				this->virtualHeight=(ushort)atol(arguments[++a]);
				this->realWidth=(ushort)atol(arguments[++a]);
				this->realHeight=(ushort)atol(arguments[++a]);
				break;
			case 16: //-verbosity
				if (a+1>=size){
					STD_CERR <<"Invalid argument syntax: \""<<arguments[a]<<"\"\n";
					break;
				}
				this->verbosity=(uchar)atol(arguments[++a]);
				break;
			case 18: //-s
				this->no_sound=1;
				break;
			case 21: //-save-directory
				if (a+1>=size)
					STD_CERR <<"Invalid argument syntax: \""<<arguments[a]<<"\"\n";
				else{
					std::wstring copy=arguments[++a];
					toforwardslash(copy);
					copy=copy.substr(0,copy.find('/'));
					if (copy.size())
						this->savedir=copy;
				}
				break;
#ifndef NONS_NO_STDOUT
			case 22: //-!reset-out-files
				this->reset_redirection_files=0;
				break;
			case 23: //-!redirect
				this->override_stdout=0;
				break;
#endif
			case 24: //-stop-on-first-error
				this->stopOnFirstError=1;
				break;
			case 25: //-archive-directory
				if (a+1>=size)
					STD_CERR <<"Invalid argument syntax: \""<<arguments[a]<<"\"\n";
				else{
					this->archiveDirectory=arguments[++a];
					toforwardslash(this->archiveDirectory);
				}
				break;
			case 26: //-pp-output
				if (a+1>=size)
					STD_CERR <<"Invalid argument syntax: \""<<arguments[a]<<"\"\n";
				else{
					this->outputPreprocessedFile=1;
					this->preprocessedFile=arguments[++a];
					toforwardslash(this->preprocessedFile);
				}
				break;
			case 27: //-disable-threading
				this->noThreads=1;
				break;
			case 28: //-pp-then-quit
				this->preprocessAndQuit=1;
				break;
			case 29: //-play
				if (a+1>=size)
					STD_CERR <<"Invalid argument syntax: \""<<arguments[a]<<"\"\n";
				else{
					this->play=arguments[++a];
					toforwardslash(this->play);
				}
				break;
			case 30: //-replace
				if (a+1>=size)
					STD_CERR <<"Invalid argument syntax: \""<<arguments[a]<<"\"\n";
				else{
					std::wstring str=arguments[++a];
					if (str.size()%2)
						str.resize(str.size()-1);
					for (size_t b=0;b<str.size();b+=2)
						this->replaceArray[str[b]]=str[b+1];
				}
				break;
			case 31: //-use-long-audio-buffers
				this->use_long_audio_buffers=1;
				break;
			case 32: //-default-font
			case 33: //-console-font
				if (a+1>=size)
					STD_CERR <<"Invalid argument syntax: \""<<arguments[a]<<"\"\n";
				else{
					std::wstring filename=arguments[++a];
					if (option==32)
						this->default_font=filename;
					else
						this->console_font=filename;
				}
				break;
			case 7: //-image-cache-size
			case 12: //-no-console
			case 17: //-sdebug
			default:
				STD_CERR <<"Unrecognized command line option: \""<<arguments[a]<<"\"\n";
		}
	}
}

NONS_Settings::NONS_Settings(){
	if (!this->forward_key_lookup.size())
		this->initialize_key_lookup();
}

#define initialize_key_lookup_INITKEY(n) forward_key_lookup[n]=#n

void NONS_Settings::initialize_key_lookup(){
	forward_key_lookup.resize(SDLK_LAST);
	initialize_key_lookup_INITKEY(SDLK_UNKNOWN);
	initialize_key_lookup_INITKEY(SDLK_FIRST);
	initialize_key_lookup_INITKEY(SDLK_BACKSPACE);
	initialize_key_lookup_INITKEY(SDLK_TAB);
	initialize_key_lookup_INITKEY(SDLK_CLEAR);
	initialize_key_lookup_INITKEY(SDLK_RETURN);
	initialize_key_lookup_INITKEY(SDLK_PAUSE);
	initialize_key_lookup_INITKEY(SDLK_ESCAPE);
	initialize_key_lookup_INITKEY(SDLK_SPACE);
	initialize_key_lookup_INITKEY(SDLK_EXCLAIM);
	initialize_key_lookup_INITKEY(SDLK_QUOTEDBL);
	initialize_key_lookup_INITKEY(SDLK_HASH);
	initialize_key_lookup_INITKEY(SDLK_DOLLAR);
	initialize_key_lookup_INITKEY(SDLK_AMPERSAND);
	initialize_key_lookup_INITKEY(SDLK_QUOTE);
	initialize_key_lookup_INITKEY(SDLK_LEFTPAREN);
	initialize_key_lookup_INITKEY(SDLK_RIGHTPAREN);
	initialize_key_lookup_INITKEY(SDLK_ASTERISK);
	initialize_key_lookup_INITKEY(SDLK_PLUS);
	initialize_key_lookup_INITKEY(SDLK_COMMA);
	initialize_key_lookup_INITKEY(SDLK_MINUS);
	initialize_key_lookup_INITKEY(SDLK_PERIOD);
	initialize_key_lookup_INITKEY(SDLK_SLASH);
	initialize_key_lookup_INITKEY(SDLK_0);
	initialize_key_lookup_INITKEY(SDLK_1);
	initialize_key_lookup_INITKEY(SDLK_2);
	initialize_key_lookup_INITKEY(SDLK_3);
	initialize_key_lookup_INITKEY(SDLK_4);
	initialize_key_lookup_INITKEY(SDLK_5);
	initialize_key_lookup_INITKEY(SDLK_6);
	initialize_key_lookup_INITKEY(SDLK_7);
	initialize_key_lookup_INITKEY(SDLK_8);
	initialize_key_lookup_INITKEY(SDLK_9);
	initialize_key_lookup_INITKEY(SDLK_COLON);
	initialize_key_lookup_INITKEY(SDLK_SEMICOLON);
	initialize_key_lookup_INITKEY(SDLK_LESS);
	initialize_key_lookup_INITKEY(SDLK_EQUALS);
	initialize_key_lookup_INITKEY(SDLK_GREATER);
	initialize_key_lookup_INITKEY(SDLK_QUESTION);
	initialize_key_lookup_INITKEY(SDLK_AT);
	initialize_key_lookup_INITKEY(SDLK_LEFTBRACKET);
	initialize_key_lookup_INITKEY(SDLK_BACKSLASH);
	initialize_key_lookup_INITKEY(SDLK_RIGHTBRACKET);
	initialize_key_lookup_INITKEY(SDLK_CARET);
	initialize_key_lookup_INITKEY(SDLK_UNDERSCORE);
	initialize_key_lookup_INITKEY(SDLK_BACKQUOTE);
	initialize_key_lookup_INITKEY(SDLK_DELETE);
	initialize_key_lookup_INITKEY(SDLK_WORLD_0);
	initialize_key_lookup_INITKEY(SDLK_WORLD_1);
	initialize_key_lookup_INITKEY(SDLK_WORLD_2);
	initialize_key_lookup_INITKEY(SDLK_WORLD_3);
	initialize_key_lookup_INITKEY(SDLK_WORLD_4);
	initialize_key_lookup_INITKEY(SDLK_WORLD_5);
	initialize_key_lookup_INITKEY(SDLK_WORLD_6);
	initialize_key_lookup_INITKEY(SDLK_WORLD_7);
	initialize_key_lookup_INITKEY(SDLK_WORLD_8);
	initialize_key_lookup_INITKEY(SDLK_WORLD_9);
	initialize_key_lookup_INITKEY(SDLK_WORLD_10);
	initialize_key_lookup_INITKEY(SDLK_WORLD_11);
	initialize_key_lookup_INITKEY(SDLK_WORLD_12);
	initialize_key_lookup_INITKEY(SDLK_WORLD_13);
	initialize_key_lookup_INITKEY(SDLK_WORLD_14);
	initialize_key_lookup_INITKEY(SDLK_WORLD_15);
	initialize_key_lookup_INITKEY(SDLK_WORLD_16);
	initialize_key_lookup_INITKEY(SDLK_WORLD_17);
	initialize_key_lookup_INITKEY(SDLK_WORLD_18);
	initialize_key_lookup_INITKEY(SDLK_WORLD_19);
	initialize_key_lookup_INITKEY(SDLK_WORLD_20);
	initialize_key_lookup_INITKEY(SDLK_WORLD_21);
	initialize_key_lookup_INITKEY(SDLK_WORLD_22);
	initialize_key_lookup_INITKEY(SDLK_WORLD_23);
	initialize_key_lookup_INITKEY(SDLK_WORLD_24);
	initialize_key_lookup_INITKEY(SDLK_WORLD_25);
	initialize_key_lookup_INITKEY(SDLK_WORLD_26);
	initialize_key_lookup_INITKEY(SDLK_WORLD_27);
	initialize_key_lookup_INITKEY(SDLK_WORLD_28);
	initialize_key_lookup_INITKEY(SDLK_WORLD_29);
	initialize_key_lookup_INITKEY(SDLK_WORLD_30);
	initialize_key_lookup_INITKEY(SDLK_WORLD_31);
	initialize_key_lookup_INITKEY(SDLK_WORLD_32);
	initialize_key_lookup_INITKEY(SDLK_WORLD_33);
	initialize_key_lookup_INITKEY(SDLK_WORLD_34);
	initialize_key_lookup_INITKEY(SDLK_WORLD_35);
	initialize_key_lookup_INITKEY(SDLK_WORLD_36);
	initialize_key_lookup_INITKEY(SDLK_WORLD_37);
	initialize_key_lookup_INITKEY(SDLK_WORLD_38);
	initialize_key_lookup_INITKEY(SDLK_WORLD_39);
	initialize_key_lookup_INITKEY(SDLK_WORLD_40);
	initialize_key_lookup_INITKEY(SDLK_WORLD_41);
	initialize_key_lookup_INITKEY(SDLK_WORLD_42);
	initialize_key_lookup_INITKEY(SDLK_WORLD_43);
	initialize_key_lookup_INITKEY(SDLK_WORLD_44);
	initialize_key_lookup_INITKEY(SDLK_WORLD_45);
	initialize_key_lookup_INITKEY(SDLK_WORLD_46);
	initialize_key_lookup_INITKEY(SDLK_WORLD_47);
	initialize_key_lookup_INITKEY(SDLK_WORLD_48);
	initialize_key_lookup_INITKEY(SDLK_WORLD_49);
	initialize_key_lookup_INITKEY(SDLK_WORLD_50);
	initialize_key_lookup_INITKEY(SDLK_WORLD_51);
	initialize_key_lookup_INITKEY(SDLK_WORLD_52);
	initialize_key_lookup_INITKEY(SDLK_WORLD_53);
	initialize_key_lookup_INITKEY(SDLK_WORLD_54);
	initialize_key_lookup_INITKEY(SDLK_WORLD_55);
	initialize_key_lookup_INITKEY(SDLK_WORLD_56);
	initialize_key_lookup_INITKEY(SDLK_WORLD_57);
	initialize_key_lookup_INITKEY(SDLK_WORLD_58);
	initialize_key_lookup_INITKEY(SDLK_WORLD_59);
	initialize_key_lookup_INITKEY(SDLK_WORLD_60);
	initialize_key_lookup_INITKEY(SDLK_WORLD_61);
	initialize_key_lookup_INITKEY(SDLK_WORLD_62);
	initialize_key_lookup_INITKEY(SDLK_WORLD_63);
	initialize_key_lookup_INITKEY(SDLK_WORLD_64);
	initialize_key_lookup_INITKEY(SDLK_WORLD_65);
	initialize_key_lookup_INITKEY(SDLK_WORLD_66);
	initialize_key_lookup_INITKEY(SDLK_WORLD_67);
	initialize_key_lookup_INITKEY(SDLK_WORLD_68);
	initialize_key_lookup_INITKEY(SDLK_WORLD_69);
	initialize_key_lookup_INITKEY(SDLK_WORLD_70);
	initialize_key_lookup_INITKEY(SDLK_WORLD_71);
	initialize_key_lookup_INITKEY(SDLK_WORLD_72);
	initialize_key_lookup_INITKEY(SDLK_WORLD_73);
	initialize_key_lookup_INITKEY(SDLK_WORLD_74);
	initialize_key_lookup_INITKEY(SDLK_WORLD_75);
	initialize_key_lookup_INITKEY(SDLK_WORLD_76);
	initialize_key_lookup_INITKEY(SDLK_WORLD_77);
	initialize_key_lookup_INITKEY(SDLK_WORLD_78);
	initialize_key_lookup_INITKEY(SDLK_WORLD_79);
	initialize_key_lookup_INITKEY(SDLK_WORLD_80);
	initialize_key_lookup_INITKEY(SDLK_WORLD_81);
	initialize_key_lookup_INITKEY(SDLK_WORLD_82);
	initialize_key_lookup_INITKEY(SDLK_WORLD_83);
	initialize_key_lookup_INITKEY(SDLK_WORLD_84);
	initialize_key_lookup_INITKEY(SDLK_WORLD_85);
	initialize_key_lookup_INITKEY(SDLK_WORLD_86);
	initialize_key_lookup_INITKEY(SDLK_WORLD_87);
	initialize_key_lookup_INITKEY(SDLK_WORLD_88);
	initialize_key_lookup_INITKEY(SDLK_WORLD_89);
	initialize_key_lookup_INITKEY(SDLK_WORLD_90);
	initialize_key_lookup_INITKEY(SDLK_WORLD_91);
	initialize_key_lookup_INITKEY(SDLK_WORLD_92);
	initialize_key_lookup_INITKEY(SDLK_WORLD_93);
	initialize_key_lookup_INITKEY(SDLK_WORLD_94);
	initialize_key_lookup_INITKEY(SDLK_WORLD_95);
	initialize_key_lookup_INITKEY(SDLK_KP0);
	initialize_key_lookup_INITKEY(SDLK_KP1);
	initialize_key_lookup_INITKEY(SDLK_KP2);
	initialize_key_lookup_INITKEY(SDLK_KP3);
	initialize_key_lookup_INITKEY(SDLK_KP4);
	initialize_key_lookup_INITKEY(SDLK_KP5);
	initialize_key_lookup_INITKEY(SDLK_KP6);
	initialize_key_lookup_INITKEY(SDLK_KP7);
	initialize_key_lookup_INITKEY(SDLK_KP8);
	initialize_key_lookup_INITKEY(SDLK_KP9);
	initialize_key_lookup_INITKEY(SDLK_KP_PERIOD);
	initialize_key_lookup_INITKEY(SDLK_KP_DIVIDE);
	initialize_key_lookup_INITKEY(SDLK_KP_MULTIPLY);
	initialize_key_lookup_INITKEY(SDLK_KP_MINUS);
	initialize_key_lookup_INITKEY(SDLK_KP_PLUS);
	initialize_key_lookup_INITKEY(SDLK_KP_ENTER);
	initialize_key_lookup_INITKEY(SDLK_KP_EQUALS);
	initialize_key_lookup_INITKEY(SDLK_UP);
	initialize_key_lookup_INITKEY(SDLK_DOWN);
	initialize_key_lookup_INITKEY(SDLK_RIGHT);
	initialize_key_lookup_INITKEY(SDLK_LEFT);
	initialize_key_lookup_INITKEY(SDLK_INSERT);
	initialize_key_lookup_INITKEY(SDLK_HOME);
	initialize_key_lookup_INITKEY(SDLK_END);
	initialize_key_lookup_INITKEY(SDLK_PAGEUP);
	initialize_key_lookup_INITKEY(SDLK_PAGEDOWN);
	initialize_key_lookup_INITKEY(SDLK_F1);
	initialize_key_lookup_INITKEY(SDLK_F2);
	initialize_key_lookup_INITKEY(SDLK_F3);
	initialize_key_lookup_INITKEY(SDLK_F4);
	initialize_key_lookup_INITKEY(SDLK_F5);
	initialize_key_lookup_INITKEY(SDLK_F6);
	initialize_key_lookup_INITKEY(SDLK_F7);
	initialize_key_lookup_INITKEY(SDLK_F8);
	initialize_key_lookup_INITKEY(SDLK_F9);
	initialize_key_lookup_INITKEY(SDLK_F10);
	initialize_key_lookup_INITKEY(SDLK_F11);
	initialize_key_lookup_INITKEY(SDLK_F12);
	initialize_key_lookup_INITKEY(SDLK_F13);
	initialize_key_lookup_INITKEY(SDLK_F14);
	initialize_key_lookup_INITKEY(SDLK_F15);
	initialize_key_lookup_INITKEY(SDLK_NUMLOCK);
	initialize_key_lookup_INITKEY(SDLK_CAPSLOCK);
	initialize_key_lookup_INITKEY(SDLK_SCROLLOCK);
	initialize_key_lookup_INITKEY(SDLK_RSHIFT);
	initialize_key_lookup_INITKEY(SDLK_LSHIFT);
	initialize_key_lookup_INITKEY(SDLK_RCTRL);
	initialize_key_lookup_INITKEY(SDLK_LCTRL);
	initialize_key_lookup_INITKEY(SDLK_RALT);
	initialize_key_lookup_INITKEY(SDLK_LALT);
	initialize_key_lookup_INITKEY(SDLK_RMETA);
	initialize_key_lookup_INITKEY(SDLK_LMETA);
	initialize_key_lookup_INITKEY(SDLK_LSUPER);
	initialize_key_lookup_INITKEY(SDLK_RSUPER);
	initialize_key_lookup_INITKEY(SDLK_MODE);
	initialize_key_lookup_INITKEY(SDLK_COMPOSE);
	initialize_key_lookup_INITKEY(SDLK_HELP);
	initialize_key_lookup_INITKEY(SDLK_PRINT);
	initialize_key_lookup_INITKEY(SDLK_SYSREQ);
	initialize_key_lookup_INITKEY(SDLK_BREAK);
	initialize_key_lookup_INITKEY(SDLK_MENU);
	initialize_key_lookup_INITKEY(SDLK_POWER);
	initialize_key_lookup_INITKEY(SDLK_EURO);
	initialize_key_lookup_INITKEY(SDLK_UNDO);
	initialize_key_lookup_INITKEY(SDLK_LAST);
}

void NONS_Settings::init(const std::wstring &path){
	this->path=path;
	if (!this->doc.LoadFile(path)){
		if (NONS_File::file_exists(path))
			//Using old config format or file is damaged. Delete.
			NONS_File::delete_file(path);
		//Initialize.
		this->doc.LinkEndChild(new TiXmlElement("settings"));
	}
	TiXmlElement *settings=this->doc.FirstChildElement("settings");
	if (!settings)
		this->doc.LinkEndChild(settings=new TiXmlElement("settings"));
	this->load_text_speed(settings);
}

void NONS_Settings::save(){
	this->save_text_speed(this->doc.FirstChildElement("settings"));
	this->doc.SaveFile(this->path);
}

void NONS_Settings::load_text_speed(TiXmlElement *settings){
	this->text_speed.set=settings->QueryIntAttribute("text_speed",&this->text_speed.data);
}

void NONS_Settings::save_text_speed(TiXmlElement *settings){
	if (text_speed.set)
		settings->SetAttribute("text_speed",this->text_speed.data);
}
