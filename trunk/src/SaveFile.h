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

#ifndef NONS_SAVEFILE_H
#define NONS_SAVEFILE_H

#include "Common.h"
#include "ErrorCodes.h"
#include "VariableStore.h"
#include "Script.h"
#include <vector>
#include <map>
#include <ctime>
#include <SDL/SDL.h>

#define NONS_SAVEFILE_VERSION 5

std::vector<tm *> existing_files(const std::wstring &location=L"./");
std::wstring getConfigLocation();
std::wstring getSaveLocation(unsigned hash[5]);
tm *getDate(const std::wstring &filename);

struct printingPage;

struct pipelineElement;

struct NONS_SaveFile{
	char format;
	ushort version;
	ErrorCode error;
	bool italic,
		bold;
	bool fontShadow;
	long shadowPosX,
		shadowPosY;
	bool rmode;
	NONS_Color windowTextColor;
	struct Cursor{
		std::wstring string;
		bool absolute;
		long x,y;
		Cursor():absolute(0),x(0),y(0){}
	} arrow,page;
	ulong windowTransition;
	ulong windowTransitionDuration;
	std::wstring windowTransitionRule;
	NONS_LongRect windowFrame;
	ulong windowFrameColumns,
		windowFrameRows;
	ulong windowFontWidth,
		windowFontHeight;
	ulong spacing;
	short lineSkip;
	NONS_Color windowColor;
	bool transparentWindow;
	bool hideWindow;
	bool hideWindow2;
	ulong textSpeed;
	NONS_LongRect textWindow;
	ulong char_baseline;
	std::wstring unknownString_000;
	std::wstring background;
	struct Sprite{
		std::wstring string;
		long x,y;
		bool visibility;
		uchar alpha;
		ulong animOffset;
		Sprite():x(0),y(0),visibility(1),alpha(255),animOffset(0){}
	};
	Sprite characters[3];
	uchar charactersBlendOrder[3];
	bool blendSprites;
	std::vector<Sprite *> sprites;
	variables_map_T variables;
	arrays_map_T arrays;
	ushort fontSize;
	struct stackEl{
		StackFrameType::StackFrameType type;
		std::wstring label;
		ulong linesBelow,
			statementNo,
			offset_deprecated;
		Sint32 variable;
		long to;
		long step;
		std::wstring leftovers;
		ulong textgosubLevel;
		//std::vector<printingPage> pages;
		wchar_t trigger;
		std::vector<std::wstring> parameters;
	};
	std::vector<stackEl *> stack;
	std::vector<pipelineElement> pipelines[2];
	//0: apply negative first; 1: apply monochrome first
	bool nega_parameter;
	ulong asyncEffect_no,
		asyncEffect_freq;
	std::wstring midi;
	std::wstring wav;
	std::wstring music;
	long musicTrack;
	bool loopMidi,
		loopWav,
		playOnce,
		loopMp3,
		saveMp3;
	std::wstring btnDef;
	std::wstring loopBGM0,
		loopBGM1;
	std::vector<std::wstring> logPages;
	ulong indentationLevel;
	ulong currentLine;
	ulong currentSubline;
	std::wstring loadgosub;

	//Current position data:
	std::wstring currentLabel;
	ulong linesBelow,
		statementNo,
		currentOffset_deprecated;

	unsigned hash[5];
	std::wstring currentBuffer;
	ushort textX,
		textY;
	NONS_Color bgColor;
	ulong spritePriority;
	uchar musicVolume;
	struct Channel{
		std::wstring name;
		bool loop;
		uchar volume;
	};
	std::vector<Channel *> channels;
	NONS_SaveFile();
	~NONS_SaveFile();
	void load(std::wstring filename);
	bool save(std::wstring filename);
};

extern std::wstring save_directory;
extern std::wstring config_directory;
extern const wchar_t *settings_filename;
#endif
