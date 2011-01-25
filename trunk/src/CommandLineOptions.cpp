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

#include "CommandLineOptions.h"
#include "ScriptInterpreter.h"
#include "IOFunctions.h"
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
	this->override_stdout=1;
	this->reset_redirection_files=1;
	this->debugMode=0;
	this->noconsole=0;
	this->virtualWidth=DEFAULT_INPUT_WIDTH;
	this->virtualHeight=DEFAULT_INPUT_HEIGHT;
	this->realWidth=DEFAULT_OUTPUT_WIDTH;
	this->realHeight=DEFAULT_OUTPUT_HEIGHT;
	this->startFullscreen=0;
	this->verbosity=0;
	this->no_sound=0;
	this->stopOnFirstError=0;
	this->listImplementation=0;
	this->outputPreprocessedFile=0;
	this->noThreads=0;
	this->preprocessAndQuit=0;
	this->use_long_audio_buffers=0;
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
#if NONS_SYS_WINDOWS
		"  -no-console\n"
		"      Hide the console.\n"
#endif
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
		"  -play [a] <filename>\n"
		"      Play the file and quit. The file can be a graphics, audio or video file.\n"
		"      This option can be used to test whether the engine can find and read the\n"
		"      file.\n"
		"  -replace <replacement string>\n"
		"      Sets characters to be replaced in the printing mechanism.\n"
		"      See the documentation for more information.\n"
		"  -use-long-audio-buffers\n"
		"      Allocates longer audio buffers. This fixes some problems with sound in\n"
		"      older systems.\n"
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
					std::cerr <<"Invalid argument syntax: \""<<arguments[a]<<"\""<<std::endl;
					break;
				}
				if (arguments[++a]==L"auto"){
					this->scriptencoding=ENCODING::AUTO;
					break;
				}
				if (a+1>=size){
					std::cerr <<"Invalid argument syntax: \""<<arguments[a-1]<<"\""<<std::endl;
					break;
				}
				this->scriptPath=arguments[a];
				this->scriptEncryption=(ENCRYPTION::ENCRYPTION)atoi(arguments[++a]);
				break;
			case 2: //-encoding
				if (a+1>=size){
					std::cerr <<"Invalid argument syntax: \""<<arguments[a]<<"\""<<std::endl;
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
				std::cerr <<"Unrecognized encoding: \""<<arguments[a]<<"\""<<std::endl;
				break;
			case 3: //-music-format
				{
					if (a+1>=size){
						std::cerr <<"Invalid argument syntax: \""<<arguments[a]<<"\""<<std::endl;
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
						std::cerr <<"Unrecognized music format: \""<<arguments[a]<<"\""<<std::endl;
				}
				break;
			case 4: //-music-directory
				if (a+1>=size){
					std::cerr <<"Invalid argument syntax: \""<<arguments[a]<<"\""<<std::endl;
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
			case 9: //-redirect
				this->override_stdout=1;
				this->debugMode=0;
				break;
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
						std::cerr <<"Invalid argument syntax: \""<<arguments[a]<<"\""<<std::endl;
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
					std::cerr <<"Invalid argument syntax: \""<<arguments[a]<<"\""<<std::endl;
					break;
				}
				this->virtualWidth=(ushort)atoi(arguments[++a]);
				this->virtualHeight=(ushort)atoi(arguments[++a]);
				this->realWidth=(ushort)atoi(arguments[++a]);
				this->realHeight=(ushort)atoi(arguments[++a]);
				break;
			case 16: //-verbosity
				if (a+1>=size){
					std::cerr <<"Invalid argument syntax: \""<<arguments[a]<<"\""<<std::endl;
					break;
				}
				this->verbosity=(uchar)atoi(arguments[++a]);
				break;
			case 18: //-s
				this->no_sound=1;
				break;
			case 21: //-save-directory
				if (a+1>=size)
					std::cerr <<"Invalid argument syntax: \""<<arguments[a]<<"\""<<std::endl;
				else{
					std::wstring copy=arguments[++a];
					toforwardslash(copy);
					copy=copy.substr(0,copy.find('/'));
					if (copy.size())
						this->savedir=copy;
				}
				break;
			case 22: //-!reset-out-files
				this->reset_redirection_files=0;
				break;
			case 23: //-!redirect
				this->override_stdout=0;
				break;
			case 24: //-stop-on-first-error
				this->stopOnFirstError=1;
				break;
			case 25: //-archive-directory
				if (a+1>=size)
					std::cerr <<"Invalid argument syntax: \""<<arguments[a]<<"\""<<std::endl;
				else{
					this->archiveDirectory=arguments[++a];
					toforwardslash(this->archiveDirectory);
				}
				break;
			case 26: //-pp-output
				if (a+1>=size)
					std::cerr <<"Invalid argument syntax: \""<<arguments[a]<<"\""<<std::endl;
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
					std::cerr <<"Invalid argument syntax: \""<<arguments[a]<<"\""<<std::endl;
				else{
					this->play=arguments[++a];
					toforwardslash(this->play);
				}
				break;
			case 30: //-replace
				if (a+1>=size)
					std::cerr <<"Invalid argument syntax: \""<<arguments[a]<<"\""<<std::endl;
				else{
					std::wstring str=arguments[++a];
					if (str.size()%2)
						str.resize(str.size()-1);
					for (size_t b=0;b<str.size();b+=2)
						CLOptions.replaceArray[str[b]]=str[b+1];
				}
				break;
			case 32: //-use-long-audio-buffers
				this->use_long_audio_buffers=1;
				break;
			case 7: //-image-cache-size
			case 12: //-no-console
			case 17: //-sdebug
			default:
				std::cerr <<"Unrecognized command line option: \""<<arguments[a]<<"\""<<std::endl;
		}
	}
}
