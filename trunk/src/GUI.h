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

#ifndef NONS_GUI_H
#define NONS_GUI_H

#include "Common.h"
#include "VirtualScreen.h"
#include "SDL_ttf.h"
#include "Audio.h"
#include "Archive.h"
#include "ConfigFile.h"
#include <SDL/SDL.h>
#include <string>

class NONS_Font{
	TTF_Font *font;
	int size;
	int style;
	int ascent;
	/*uchar *fontbuffer;
	SDL_RWops *fontRWop;*/
public:
	int lineSkip;
	int fontLineSkip;
	int spacing;
	NONS_Font();
	NONS_Font(const char *fontname,int size,int style);
	NONS_Font(SDL_RWops *rwop,int size,int style);
	~NONS_Font();
	TTF_Font *getfont(){
		return this->font;
	}
	int getsize(){
		return this->size;
	}
	int getstyle(){
		return this->style;
	}
	int getascent(){
		return this->ascent;
	}
	void setStyle(int style){
		TTF_SetFontStyle(this->font,style);
	}
	bool valid(){
		return this->font!=0;
	}
};

struct NONS_Glyph{
	//The font structure.
	NONS_Font *font;
	TTF_Font *ttf_font;
	//Surface storing the rendered glyph.
	SDL_Surface *glyph;
	//Unicode code point for the glyph.
	wchar_t codePoint;
	//SDL_Rect storing glyph size information
	SDL_Rect box;
	//Glyph advance
	int advance;
	//The color the glyph was rendered with.
	SDL_Color foreground;
	//The style the glyph was rendered with.
	int style;
	//Check if two SDL_Colors have the same values.
	bool equalColors(SDL_Color *a,SDL_Color *b);
	NONS_Glyph(NONS_Font *font,wchar_t character,int ascent,SDL_Color *foreground,bool shadow);
	~NONS_Glyph();
	wchar_t getcodePoint();
	SDL_Rect getbox();
	int getadvance();
	void putGlyph(SDL_Surface *dst,int x,int y,SDL_Color *foreground,bool method=0);
	SDL_Color getforeground();
};

class NONS_FontCache{
	bool shadow;
	std::vector<NONS_Glyph *> glyphCache;
public:
	//Foreground color. This is not guaranteed to be the color of the individual
	//glyphs, but it is guaranteed to be the color of the glyphs returned by
	//getGlyph().
	SDL_Color foreground;
	NONS_FontCache(NONS_Font *font,SDL_Color *foreground,bool shadow);
	~NONS_FontCache();
	NONS_Font *font;
	std::vector<NONS_Glyph *> *getglyphCache();
	NONS_Glyph *getGlyph(wchar_t codePoint);
	void refreshCache();
};

NONS_Font *init_font(ulong size,NONS_GeneralArchive *archive,const char *filename);

struct NONS_StandardOutput;
struct NONS_ScreenSpace;
class NONS_ScriptInterpreter;
struct NONS_Menu;
struct NONS_Layer;
class NONS_EventQueue;

struct NONS_Button{
	NONS_Layer *offLayer;
	NONS_Layer *onLayer;
	NONS_Layer *shadowLayer;
	SDL_Rect box;
	NONS_ScreenSpace *screen;
	NONS_Font *font;
	bool status;
	int posx,posy;
	int limitX,limitY;
	NONS_Button();
	NONS_Button(NONS_Font *font);
	~NONS_Button();
	void makeTextButton(const std::wstring &text,float center,SDL_Color *on,SDL_Color *off,bool shadow,int limitX,int limitY);
	void makeGraphicButton(SDL_Surface *src,int posx,int posy,int width,int height,int originX,int originY);
	void mergeWithoutUpdate(NONS_VirtualScreen *dst,SDL_Surface *original,bool status,bool force=0);
	void merge(NONS_VirtualScreen *dst,SDL_Surface *original,bool status,bool force=0);
	bool MouseOver(SDL_Event *event);
	bool MouseOver(int x,int y);
private:
	SDL_Rect GetBoundingBox(const std::wstring &str,NONS_FontCache *cache,int limitX,int limitY);
	void write(const std::wstring &str,float center=0);
	int setLineStart(std::vector<NONS_Glyph *> *arr,long start,SDL_Rect *frame,float center);
	int predictLineLength(std::vector<NONS_Glyph *> *arr,long start,int width);
};

struct NONS_ButtonLayer{
	std::vector<NONS_Button *> buttons;
	NONS_Font *font;
	NONS_ScreenSpace *screen;
	std::wstring voiceEntry;
	std::wstring voiceMouseOver;
	std::wstring voiceClick;
	NONS_Audio *audio;
	NONS_GeneralArchive *archive;
	SDL_Rect boundingBox;
	bool exitable;
	NONS_Menu *menu;
	SDL_Surface *loadedGraphic;
	struct{
		bool Wheel,
			btnArea,
			EscapeSpace,
			PageUpDown,
			Enter,
			Tab,
			Function,
			Cursor,
			Insert,
			ZXC;
	} inputOptions;
	NONS_ButtonLayer(SDL_Surface *img,NONS_ScreenSpace *screen);
	NONS_ButtonLayer(NONS_Font *font,NONS_ScreenSpace *screen,bool exitable,NONS_Menu *menu);
	~NONS_ButtonLayer();
	void makeTextButtons(const std::vector<std::wstring> &arr,
		SDL_Color *on,
		SDL_Color *off,
		bool shadow,
		std::wstring *entry,
		std::wstring *mouseover,
		std::wstring *click,
		NONS_Audio *audio,
		NONS_GeneralArchive *archive,
		int width,
		int height);
	void addImageButton(ulong index,int posx,int posy,int width,int height,int originX,int originY);
	/*
	returns:
		if >=0, the index of the button pressed
		-1 if escape was pressed and the layer was exitable (i.e. the layer was being used for the menu)
		-2 if the layer doesn't fit in the screen with the given coordinates
		-3 if escape was pressed and the layer wasn't exitable (i.e. the user tried to access the menu)
		INT_MIN if SDL_QUIT was received
	*/
	int getUserInput(int x,int y);
	/*
	returns:
		if >=0, the index of the button pressed
		-1 if the user left-clicked, but not on a button
		<-1 for different key presses under certain circumstances
		INT_MIN if SDL_QUIT was received
	*/
	int getUserInput(ulong expiration=0);
	ulong countActualButtons();
};

struct NONS_Menu{
	std::vector<std::wstring> strings;
	std::vector<std::wstring> commands;
	SDL_Color off;
	SDL_Color on;
	SDL_Color nofile;
	bool shadow;
	NONS_ButtonLayer *buttons;
	ushort slots;
	NONS_ScriptInterpreter *interpreter;
	int x,y;
	NONS_Layer *shade;
	NONS_Font *font;
	NONS_Font *defaultFont;
	long fontsize,spacing,lineskip;
	SDL_Color shadeColor;
	std::wstring stringSave;
	std::wstring stringLoad;
	std::wstring stringSlot;
	std::wstring voiceEntry;
	std::wstring voiceCancel;
	std::wstring voiceMO;
	std::wstring voiceClick;
	std::wstring voiceYes;
	std::wstring voiceNo;
	NONS_Audio *audio;
	NONS_GeneralArchive *archive;
	uchar rightClickMode;

	NONS_Menu(NONS_ScriptInterpreter *interpreter);
	NONS_Menu(std::vector<std::wstring> *options,NONS_ScriptInterpreter *interpreter);
	~NONS_Menu();
	int callMenu();
	void reset();
	void resetStrings(std::vector<std::wstring> *options);
	int save();
	int load();
	//0 if the user chose to quit, INT_MIN if SDL_QUIT was received
	int windowerase();
	int skip();
	int call(const std::wstring &string);
private:
	int write(const std::wstring &txt,int y);
};

struct NONS_Lookback{
	SDL_Color foreground;
	NONS_StandardOutput *output;
	NONS_Button *up,
		*down;
	SDL_Surface *sUpon;
	SDL_Surface *sUpoff;
	SDL_Surface *sDownon;
	SDL_Surface *sDownoff;
	NONS_Lookback(NONS_StandardOutput *output,uchar r,uchar g,uchar b);
	~NONS_Lookback();
	bool setUpButtons(const std::wstring &upon,const std::wstring &upoff,const std::wstring &downon,const std::wstring &downoff);
	int display(NONS_VirtualScreen *dst);
	void reset(NONS_StandardOutput *output);
private:
	bool changePage(int dir,long &currentPage,SDL_Surface *copyDst,NONS_VirtualScreen *dst,SDL_Surface *preBlit,uchar &visibility,int &mouseOver);
};

struct NONS_Cursor{
	NONS_Layer *data;
	long xpos,ypos;
	bool absolute;
	NONS_ScreenSpace *screen;
	NONS_Cursor(NONS_ScreenSpace *screen);
	NONS_Cursor(const std::wstring &string,long x,long y,long absolute,NONS_ScreenSpace *screen);
	~NONS_Cursor();
	int animate(NONS_Menu *menu,ulong expiration);
private:
	//0 if the caller should return, 1 if it should continue
	bool callMenu(NONS_Menu *menu,NONS_EventQueue *queue);
	//0 if the caller should return, 1 if it should continue
	bool callLookback(NONS_EventQueue *queue);
};

struct NONS_CursorPair{
	NONS_Cursor *on;
	NONS_Cursor *off;
};

struct NONS_GeneralArchive;

#define CONLOCATE(x,y) ((x)+(y)*this->screenW)

class NONS_DebuggingConsole{
	NONS_Font *font;
	NONS_FontCache *cache;
	ulong characterWidth,
		characterHeight,
		screenW,
		screenH,
		cursorX,
		cursorY;
	std::vector<wchar_t> screen;
	std::vector<std::wstring> pastInputs;
	std::wstring partial;
	bool print_prompt;
	void redraw(NONS_ScreenSpace *dst,long startFromLine,ulong cursor,const std::wstring &line);
	//Note: It does NOT lock the screen.
	void redraw(NONS_ScreenSpace *dst,long startFromLine,ulong lineHeight);
	std::vector<std::wstring> autocompleteVector;
	wchar_t &locate(size_t x,size_t y){
		return this->screen[CONLOCATE(x,y)];
	}
	bool input(std::wstring &input,NONS_ScreenSpace *dst);
	void autocomplete(std::vector<std::wstring> &dst,const std::wstring &line,std::wstring &suggestion,ulong cursor,ulong &space);
	void outputMatches(const std::vector<std::wstring> &matches,NONS_ScreenSpace *dst/*,long startFromLine,ulong cursor,const std::wstring &line*/);
public:
	NONS_DebuggingConsole();
	~NONS_DebuggingConsole();
	void init(NONS_GeneralArchive *archive);
	void enter(NONS_ScreenSpace *dst);
	void output(const std::wstring &str,NONS_ScreenSpace *dst);
};

extern NONS_DebuggingConsole console;
#endif
