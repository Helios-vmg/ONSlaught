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

#ifndef NONS_GUI_H
#define NONS_GUI_H

#include "Common.h"
#include "VirtualScreen.h"
#include "Audio.h"
#include "Archive.h"
#include "ConfigFile.h"
#include <SDL/SDL.h>
#include <string>
#include <set>
#include <ft2build.h>
#include FT_FREETYPE_H

class NONS_FreeType_Lib{
	FT_Library library;
	NONS_FreeType_Lib();
	NONS_FreeType_Lib(const NONS_FreeType_Lib &){}
	~NONS_FreeType_Lib();
public:
	static NONS_FreeType_Lib instance;
	FT_Library get_lib() const{ return this->library; }
};

class NONS_Font{
	FT_Face ft_font;
	FT_Error error;
	ulong size;
	uchar *buffer;

	NONS_Font(const NONS_Font &){}
	void operator=(const NONS_Font &){}
public:
	ulong ascent,
		line_skip;

	NONS_Font(const std::string &filename);
	NONS_Font(uchar *buffer,size_t size);
	~NONS_Font();
	bool good() const{ return !this->error; }
	FT_Error get_error() const{ return this->error; }
	bool check_flag(unsigned flag) const{ return (this->ft_font->face_flags&flag)==flag; }
	bool is_monospace() const;
	void set_size(ulong size);
	FT_GlyphSlot get_glyph(wchar_t codepoint,bool italic,bool bold) const;
	FT_GlyphSlot render_glyph(wchar_t codepoint,bool italic,bool bold) const;
	FT_GlyphSlot render_glyph(FT_GlyphSlot) const;
	FT_Face get_font() const{ return this->ft_font; }
};

class NONS_FontCache;

class NONS_Glyph{
	NONS_FontCache &fc;
	wchar_t codepoint;
	//style properties:
	ulong size,
		outline_size;
	SDL_Color color,
		outline_color;
	bool italic,
		bold;
	//~style properties
	uchar *base_bitmap,
		*outline_base_bitmap;
	SDL_Rect bounding_box,
		outline_bounding_box;
	ulong advance;
public:
	ulong refCount;
	NONS_Glyph(NONS_FontCache &fc,wchar_t codepoint,ulong size,const SDL_Color &color,bool italic,bool bold,ulong outline_size,const SDL_Color &outline_color);
	~NONS_Glyph();
	bool operator<(const NONS_Glyph &b) const{ return this->codepoint<b.codepoint; }
	void setColor(const SDL_Color &color){ this->color=color; }
	void setOutlineColor(const SDL_Color &color){ this->outline_color=color; }
	ulong get_advance_fixed() const{ return this->advance; }
	const SDL_Rect &get_bounding_box() const{ return this->bounding_box; }
	SDL_Rect get_put_bounding_box(Sint16 x,Sint16 y) const{
		SDL_Rect ret=(!this->outline_base_bitmap)?this->bounding_box:this->outline_bounding_box;
		ret.x+=x;
		ret.y+=y;
		return ret;
	}
	wchar_t get_codepoint() const{ return this->codepoint; }
	bool needs_redraw(ulong size,bool italic,bool bold,ulong outline_size) const;
	long get_advance();
	void put(SDL_Surface *dst,int x,int y,uchar alpha=255);
	const NONS_FontCache &get_cache() const{ return this->fc; }
	NONS_FontCache &get_cache(){ return this->fc; }
	void done();
	ulong type(){ return 0; }
};

#ifdef _DEBUG
#define FONTCACHE_DEBUG_PARAMETERS , __FILE__ , __LINE__
#else
#define FONTCACHE_DEBUG_PARAMETERS
#endif

class NONS_FontCache{
	std::map<wchar_t,NONS_Glyph *> glyphs;
	NONS_Font &font;
	ulong size,
		outline_size;
	SDL_Color color,
		outline_color;
	bool italic,
		bold;
	std::set<NONS_Glyph *> garbage;
public:
	long spacing;
	ulong line_skip,
		font_line_skip,
		ascent;
#ifdef _DEBUG
	std::string declared_in;
	ulong line;
	NONS_FontCache(NONS_Font &font,ulong size,const SDL_Color &color,bool italic,bool bold,ulong outline_size,const SDL_Color &outline_color,const char *file,ulong line);
	NONS_FontCache(const NONS_FontCache &fc,const char *file,ulong line);
private:
	NONS_FontCache(const NONS_FontCache &fc);
public:
#else
	NONS_FontCache(NONS_Font &font,ulong size,const SDL_Color &color,bool italic,bool bold,ulong outline_size,const SDL_Color &outline_color);
	NONS_FontCache(const NONS_FontCache &fc);
#endif
	~NONS_FontCache();
	void resetStyle(ulong size,bool italic,bool bold,ulong outline_size);
	void set_outline_size(ulong size){ this->outline_size=size; }
	void set_size(ulong size);
	void set_to_normal(){
		this->italic=0;
		this->bold=0;
	}
	void set_italic(bool i){ this->italic=i; }
	void set_bold(bool b){ this->bold=b; }
	void setColor(const SDL_Color &color){ this->color=color; }
	void setOutlineColor(const SDL_Color &color){ this->outline_color=color; }
	NONS_Glyph *getGlyph(wchar_t c);
	void done(NONS_Glyph *g);
	const NONS_Font &get_font() const{ return this->font; }
	NONS_Font &get_font(){ return this->font; }
	ulong get_size() const{ return this->size; }
	const SDL_Color &get_color() const{ return this->color; }
	bool get_italic() const{ return this->italic; }
	bool get_bold() const{ return this->bold; }
};

class NONS_AutoGlyph{
	NONS_FontCache &cache;
	NONS_Glyph &glyph;
public:
	NONS_AutoGlyph(NONS_FontCache &fc,NONS_Glyph &g):cache(fc),glyph(g){}
	~NONS_AutoGlyph(){ this->cache.done(&this->glyph); }
};

NONS_Font *init_font(NONS_GeneralArchive *archive,const std::string &filename);

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
	NONS_FontCache *font_cache;
	bool status;
	int posx,posy;
	int limitX,limitY;
	NONS_Button();
	NONS_Button(const NONS_FontCache &fc);
	~NONS_Button();
	void makeTextButton(const std::wstring &text,const NONS_FontCache &fc,float center,const SDL_Color &on,const SDL_Color &off,bool shadow,int limitX,int limitY);
	void makeGraphicButton(SDL_Surface *src,int posx,int posy,int width,int height,int originX,int originY);
	void mergeWithoutUpdate(NONS_VirtualScreen *dst,SDL_Surface *original,bool status,bool force=0);
	void merge(NONS_VirtualScreen *dst,SDL_Surface *original,bool status,bool force=0);
	bool MouseOver(SDL_Event *event);
	bool MouseOver(int x,int y);
private:
	SDL_Rect GetBoundingBox(const std::wstring &str,NONS_FontCache *cache,int limitX,int limitY,int &offsetX,int &offsetY);
	void write(const std::wstring &str,int offsetX,int offsetY,float center=0);
	int setLineStart(std::vector<NONS_Glyph *> *arr,long start,SDL_Rect *frame,float center,int offsetX);
	int predictLineLength(std::vector<NONS_Glyph *> *arr,long start,int width,int offsetX);
};

struct NONS_ButtonLayer{
	std::vector<NONS_Button *> buttons;
	NONS_FontCache *font_cache;
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
	NONS_ButtonLayer(const NONS_FontCache &fc,NONS_ScreenSpace *screen,bool exitable,NONS_Menu *menu);
	~NONS_ButtonLayer();
	void makeTextButtons(const std::vector<std::wstring> &arr,
		const SDL_Color &on,
		const SDL_Color &off,
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
	NONS_FontCache *font_cache,
		*default_font_cache;
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
	NONS_FontCache &get_font_cache(){ return *(this->font_cache?this->font_cache:this->default_font_cache); }
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
