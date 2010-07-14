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

#include <fstream>
#include <cstdlib>
#include <csignal>
#include <iostream>

#include "Common.h"
#include "ErrorCodes.h"
#include "ScriptInterpreter.h"
#include "IOFunctions.h"
#include "ThreadManager.h"
#include "CommandLineOptions.h"
#include "version.h"

#if NONS_SYS_WINDOWS
#include <windows.h>
#endif

void mainThread(void *);

void useArgumentsFile(const char *filename,std::vector<std::wstring> &argv){
	std::ifstream file(filename);
	if (!file)
		return;
	std::string str;
	std::getline(file,str);
	std::wstring copy=UniFromUTF8(str);
	std::vector<std::wstring> vec=getParameterList(copy,0);
	argv.insert(argv.end(),vec.begin(),vec.end());
}

void enditall(bool stop_thread){
	if (stop_thread)
		gScriptInterpreter->stop();
}

void handle_SIGTERM(int){
	enditall(1);
}

void handle_SIGINT(int){
	exit(0);
}

volatile bool stopEventHandling=0;

int lastClickX=0;
int lastClickY=0;
bool useDebugMode=0,
	video_playback=0;

void handleInputEvent(SDL_Event &event){
	long x,y;
	switch(event.type){
		case SDL_QUIT:
			enditall(1);
			InputObserver.notify(&event);
			break;
		case SDL_KEYDOWN:
			if (useDebugMode){
				bool notify=0,
					full=0;
				switch (event.key.keysym.sym){
					case SDLK_RETURN:
						if (!!(event.key.keysym.mod&KMOD_ALT))
							full=1;
						else
							notify=1;
						break;
					case SDLK_F12:
						o_stdout <<"Screenshot saved to \""<<gScriptInterpreter->screen->screen->takeScreenshot()<<"\".\n";
						break;
					case SDLK_LCTRL:
					case SDLK_RCTRL:
					case SDLK_NUMLOCK:
					case SDLK_CAPSLOCK:
					case SDLK_SCROLLOCK:
					case SDLK_RSHIFT:
					case SDLK_LSHIFT:
					case SDLK_RALT:
					case SDLK_LALT:
					case SDLK_RMETA:
					case SDLK_LMETA:
					case SDLK_LSUPER:
					case SDLK_RSUPER:
					case SDLK_MODE:
					case SDLK_COMPOSE:
						break;
					default:
						notify=1;
				}
				if (full && gScriptInterpreter->screen)
					gScriptInterpreter->screen->screen->toggleFullscreen();
				if (notify)
					InputObserver.notify(&event);
			}else{
				bool notify=0,
					full=0;
				switch (event.key.keysym.sym){
					case SDLK_LCTRL:
					case SDLK_RCTRL:
						ctrlIsPressed=1;
						break;
					case SDLK_PERIOD:
						ctrlIsPressed=!ctrlIsPressed;
						break;
					case SDLK_f:
						if (!video_playback)
							full=1;
						else
							notify=1;
						break;
					case SDLK_s:
						if (gScriptInterpreter->audio)
							gScriptInterpreter->audio->toggleMute();
						break;
					case SDLK_RETURN:
						if (CHECK_FLAG(event.key.keysym.mod,KMOD_ALT) && !video_playback)
							full=1;
						else
							notify=1;
						break;
					case SDLK_F12:
						if (!video_playback)
							o_stdout <<"Screenshot saved to \""<<gScriptInterpreter->screen->screen->takeScreenshot()<<"\".\n";
						else
							notify=1;
						break;
					case SDLK_NUMLOCK:
					case SDLK_CAPSLOCK:
					case SDLK_SCROLLOCK:
					case SDLK_RSHIFT:
					case SDLK_LSHIFT:
					case SDLK_RALT:
					case SDLK_LALT:
					case SDLK_RMETA:
					case SDLK_LMETA:
					case SDLK_LSUPER:
					case SDLK_RSUPER:
					case SDLK_MODE:
					case SDLK_COMPOSE:
						break;
					default:
						notify=1;
				}
				if (full && gScriptInterpreter->screen)
					gScriptInterpreter->screen->screen->toggleFullscreen();
				if (notify)
					InputObserver.notify(&event);
			}
			break;
		case SDL_KEYUP:
			InputObserver.notify(&event);
			if (event.key.keysym.sym==SDLK_LCTRL || event.key.keysym.sym==SDLK_RCTRL)
				ctrlIsPressed=0;
			break;
		case SDL_MOUSEMOTION:
			x=event.motion.x;
			y=event.motion.y;
			if (gScriptInterpreter->screen){
				x=gScriptInterpreter->screen->screen->unconvertX(x);
				y=gScriptInterpreter->screen->screen->unconvertY(y);
				event.motion.x=(Uint16)x;
				event.motion.y=(Uint16)y;
			}
			if (x>0 && y>0)
				InputObserver.notify(&event);
			break;
		case SDL_MOUSEBUTTONDOWN:
			x=event.button.x;
			y=event.button.y;
			if (gScriptInterpreter->screen){
				x=gScriptInterpreter->screen->screen->unconvertX(x);
				y=gScriptInterpreter->screen->screen->unconvertY(y);
				event.button.x=(Uint16)x;
				event.button.y=(Uint16)y;
			}
			if (x>0 && y>0)
				InputObserver.notify(&event);
			lastClickX=event.button.x;
			lastClickY=event.button.y;
			break;
		default:
			InputObserver.notify(&event);
	}
}

std::vector<std::wstring> getArgumentsVector(char **argv){
	std::vector<std::wstring> ret;
	for (argv++;*argv;argv++)
		ret.push_back(UniFromUTF8(std::string(*argv)));
	return ret;
}

std::vector<std::wstring> getArgumentsVector(wchar_t **argv){
	std::vector<std::wstring> ret;
	for (argv++;*argv;argv++)
		ret.push_back(std::wstring(*argv));
	return ret;
}

extern wchar_t SJIS2Unicode[0x10000],
	Unicode2SJIS[0x10000],
	SJIS2Unicode_compact[];
void initialize_conversion_tables();
extern uchar integer_division_lookup[0x10000];

#if NONS_SYS_PSP
#include <pspkernel.h>
PSP_MODULE_INFO("ONSlaught", 0, 1, 1);
#endif

#if NONS_SYS_WINDOWS && defined _CONSOLE && defined main
#undef main
#endif

extern ConfigFile settings;

void initialize(int argc,char **argv){
	srand((unsigned int)time(0));
	signal(SIGTERM,handle_SIGTERM);
	signal(SIGINT,handle_SIGINT);
	initialize_conversion_tables();
	//initialize lookup table/s
	memset(integer_division_lookup,0,256);
	for (ulong y=1;y<256;y++)
		for (ulong x=0;x<256;x++)
			integer_division_lookup[x+y*256]=uchar(x*255/y);

	config_directory=getConfigLocation();

	std::vector<std::wstring> cmdl_arg=getArgumentsVector(argv);
	useArgumentsFile("arguments.txt",cmdl_arg);
	CLOptions.parse(cmdl_arg);

	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(250,20);

	general_archive.init();

	if (CLOptions.override_stdout){
		o_stdout.redirect();
		o_stderr.redirect();
		std::cout <<"Redirecting.\n";
	}

	threadManager.setCPUcount();
#ifdef USE_THREAD_MANAGER
	threadManager.init(cpu_count);
#endif

	settings.init(config_directory+settings_filename,ENCODING::UTF8);
	
	SDL_WM_SetCaption("ONSlaught ("ONSLAUGHT_BUILD_VERSION_STR")",0);
#if NONS_SYS_WINDOWS
	findMainWindow(L"ONSlaught ("ONSLAUGHT_BUILD_VERSION_WSTR L")");
#endif
}

void print_version_string(){
	std::cout <<"ONSlaught: An ONScripter clone with Unicode support.\n";
#if ONSLAUGHT_BUILD_VERSION<99999999
	std::cout <<"Build "<<ONSLAUGHT_BUILD_VERSION<<", ";
#endif
	std::cout <<ONSLAUGHT_BUILD_VERSION_STR"\n"
#ifdef NONS_LOW_MEMORY_ENVIRONMENT
		"Low memory usage build.\n"
#endif
		"\n"
		"Copyright (c) "ONSLAUGHT_COPYRIGHT_YEAR_STR", Helios (helios.vmg@gmail.com)\n"
		"All rights reserved.\n\n\n";
}

int main(int argc,char **argv){
	print_version_string();
	initialize(argc,argv);

	NONS_ScriptInterpreter interpreter;
	gScriptInterpreter=&interpreter;
	if (CLOptions.debugMode)
		console.init();
	{
		NONS_Thread thread(mainThread,0);
		SDL_Event event;
		while (!stopEventHandling){
			while (SDL_PollEvent(&event) && !stopEventHandling)
				handleInputEvent(event);
			SDL_Delay(10);
		}
	}
	return 0;
}

void mainThread(void *){
	if (CLOptions.play.size())
		gScriptInterpreter->generic_play(CLOptions.play,CLOptions.play_from_archive);
	else
		while (gScriptInterpreter->interpretNextLine());
	stopEventHandling=1;
}
