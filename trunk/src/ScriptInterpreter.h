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

#ifndef NONS_SCRIPTINTERPRETER_H
#define NONS_SCRIPTINTERPRETER_H

#include "Common.h"
#include "ErrorCodes.h"
#include "VariableStore.h"
#include "ScreenSpace.h"
#include "Script.h"
#include "INIfile.h"
#include "ImageLoader.h"
#include "SaveFile.h"
#include "IOFunctions.h"
#include "enums.h"
#include <set>
#include <stack>
#include <cmath>
#include <cstdlib>
#include <ctime>
//#include <SDL/smpeg.h>

extern SDL_Surface *(*rotationFunction)(SDL_Surface *,double);
extern SDL_Surface *(*resizeFunction)(SDL_Surface *,int,int);

#define MINIMUM_PARAMETERS(min) if (stmt.parameters.size()<(min)) return NONS_INSUFFICIENT_PARAMETERS
#define GET_INT_VALUE(dst,src) HANDLE_POSSIBLE_ERRORS(this->store->getIntValue(stmt.parameters[(src)],(dst),0))
#define GET_COORDINATE(dst,axis,src) {\
	long GET_COORDINATE_temp;\
	float GET_COORDINATE_temp_f;\
	GET_INT_VALUE(GET_COORDINATE_temp,(src));\
	GET_COORDINATE_temp_f=float(GET_COORDINATE_temp)*float(this->virtual_size[(axis)])/float(this->base_size[(axis)]);\
	if (GET_COORDINATE_temp_f>=0)\
		GET_COORDINATE_temp_f=(float)floor(GET_COORDINATE_temp_f+.5f);\
	else\
		GET_COORDINATE_temp_f=(float)ceil(GET_COORDINATE_temp_f-.5f);\
	(dst)=GET_COORDINATE_temp_f;\
}
#define GET_INT_COORDINATE(dst,axis,src) {\
	long GET_COORDINATE_temp;\
	float GET_COORDINATE_temp_f;\
	GET_INT_VALUE(GET_COORDINATE_temp,(src));\
	GET_COORDINATE_temp_f=float(GET_COORDINATE_temp)*float(this->virtual_size[(axis)])/float(this->base_size[(axis)]);\
	if (GET_COORDINATE_temp_f>=0)\
		GET_COORDINATE_temp_f=(float)floor(GET_COORDINATE_temp_f+.5f);\
	else\
		GET_COORDINATE_temp_f=(float)ceil(GET_COORDINATE_temp_f-.5f);\
	(dst)=(long)GET_COORDINATE_temp_f;\
}
#define GET_STR_VALUE(dst,src) HANDLE_POSSIBLE_ERRORS(this->store->getWcsValue(stmt.parameters[(src)],(dst),0))
#define GET_VARIABLE(varName,src) HANDLE_POSSIBLE_ERRORS(getVar((varName),stmt.parameters[(src)],this->store))
#define GET_INT_VARIABLE(varName,src) HANDLE_POSSIBLE_ERRORS(getIntVar((varName),stmt.parameters[(src)],this->store))
#define GET_STR_VARIABLE(varName,src) HANDLE_POSSIBLE_ERRORS(getStrVar((varName),stmt.parameters[(src)],this->store))
#define GET_LABEL(dst,src){\
	std::wstring &GET_LABEL_temp=stmt.parameters[(src)];\
	if (GET_LABEL_temp[0]=='*')\
		(dst)=GET_LABEL_temp;\
	else{\
		GET_STR_VALUE((dst),(src));\
	}\
}

typedef std::map<std::wstring,INIfile *> INIcacheType;


struct printingPage{
	std::wstring print;
	std::wstring reduced;
	std::pair<ulong,ulong> startAt;
	//first: position in the string that is actually printed.
	//second: position in the reduced string
	std::vector<std::pair<ulong,ulong> > stops;
	printingPage(){}
	printingPage(const printingPage &b){*this=b;}
	printingPage &operator=(const printingPage &b);
};

struct NONS_StackElement{
	StackFrameType::StackFrameType type;
	struct statementPair{
		ulong line,statement;
		bool operator==(const statementPair &opB){
			return this->line==opB.line && this->statement==opB.statement;
		}
		bool operator!=(const statementPair &opB){
			return !(*this==opB);
		}
		std::pair<ulong,ulong> toPair(){
			return std::pair<ulong,ulong>(this->line,this->statement);
		}
	} returnTo;
	//subroutine data
	NONS_ScriptLine interpretAtReturn;
	//for data
	NONS_VariableMember *var;
	long from,
		to,
		step;
	statementPair end;
	//textgosub data
	ulong textgosubLevel;
	wchar_t textgosubTriggeredBy;
	std::vector<printingPage> pages;
	//User command data
	std::vector<std::wstring> parameters;

	NONS_StackElement(ulong level);
	NONS_StackElement(const std::pair<ulong,ulong> &returnTo,const NONS_ScriptLine &interpretAtReturn,ulong beginAtStatement,ulong level);
	NONS_StackElement(NONS_VariableMember *variable,const std::pair<ulong,ulong> &startStatement,long from,long to,long step,ulong level);
	NONS_StackElement(const std::vector<printingPage> &pages,wchar_t trigger,ulong level);
	NONS_StackElement(NONS_StackElement *copy,const std::vector<std::wstring> &vector);
};

class NONS_ScriptInterpreter;
typedef ErrorCode(NONS_ScriptInterpreter::*commandFunctionPointer)(NONS_Statement &);

struct NONS_DefineFlag{};
struct NONS_RunFlag{};
struct NONS_CommandObject{
	commandFunctionPointer function;
	bool allow_define,allow_run;
	NONS_CommandObject(commandFunctionPointer f):function(f),allow_define(0),allow_run(0){}
	NONS_CommandObject():function(0),allow_define(0),allow_run(0){}
};

extern NONS_DefineFlag ALLOW_IN_DEFINE;
extern NONS_RunFlag ALLOW_IN_RUN;

inline NONS_CommandObject operator|(NONS_CommandObject co,NONS_DefineFlag){
	co.allow_define=1;
	return co;
}

inline NONS_CommandObject operator|(NONS_CommandObject co,NONS_RunFlag){
	co.allow_run=1;
	return co;
}

typedef std::map<std::wstring,NONS_CommandObject,stdStringCmpCI<wchar_t> > commandMapType;
typedef std::set<std::wstring,stdStringCmpCI<wchar_t> > commandListType;
typedef commandListType defineModeCommandListType;
typedef commandListType userCommandListType;
typedef commandListType allowedCommandListType;

class NONS_ScriptInterpreter{
	bool Printer_support(std::vector<printingPage> &pages,ulong *totalprintedchars,bool *justTurnedPage,ErrorCode *error);
	ErrorCode Printer(const std::wstring &line);
	void reduceString(const std::wstring &src,std::wstring &dst,std::set<NONS_VariableMember *> *visited=0,std::vector<std::pair<std::wstring,NONS_VariableMember *> > *stack=0);
	void handleKeys(SDL_Event &event);
	bool was_initialized;
	void uninit();
	void init();

	bool stop_interpreting;
	commandMapType commandList;
	userCommandListType userCommandList;
	allowedCommandListType allowedCommandList;
	NONS_ScriptThread *thread;
	std::set<ulong> printed_lines;
	std::set<std::wstring> implementationErrors;
	enum{
		UNDEFINED_MODE,
		DEFINE_MODE,
		RUN_MODE
	} interpreter_mode;
	std::string nsadir;
	std::vector<NONS_StackElement *> callStack;
	bool mp3_save;
	bool mp3_loop;
	bool wav_loop;
	long default_speed;
	long default_speed_slow;
	long default_speed_med;
	long default_speed_fast;
	char current_speed_setting;
	int defaultfs;
	bool legacy_set_window;
	INIcacheType INIcache;
	NONS_Cursor *arrowCursor;
	NONS_Cursor *pageCursor;
	bool hideTextDuringEffect;
	SDL_Color selectOn;
	SDL_Color selectOff;
	std::wstring selectVoiceEntry;
	std::wstring selectVoiceMouseOver;
	std::wstring selectVoiceClick;
	NONS_EventQueue inputQueue;
	std::wstring trapLabel;
	std::wstring clickStr;
	ulong autoclick;
	ulong timer;
	NONS_Menu *menu;
	NONS_ButtonLayer *imageButtons;
	bool new_if;
	ulong btnTimer;
	ulong imageButtonExpiration;
	NONS_SaveFile *saveGame;
	std::wstring currentBuffer;
	std::wstring textgosub;
	bool textgosubRecurses;
	std::wstring loadgosub;
	bool useWheel,
		useEscapeSpace;
	SDL_Surface *screenshot;
	std::queue<NONS_ScriptLine *> commandQueue;
	ulong base_size[2],
		virtual_size[2];

	ErrorCode command_caption(NONS_Statement &stmt);
	ErrorCode command_alias(NONS_Statement &stmt);
	ErrorCode command_game(NONS_Statement &stmt);
	ErrorCode command_nsa(NONS_Statement &stmt);
	ErrorCode command_nsadir(NONS_Statement &stmt);
	ErrorCode command_goto(NONS_Statement &stmt);
	ErrorCode command_globalon(NONS_Statement &stmt);
	ErrorCode command_gosub(NONS_Statement &stmt);
	ErrorCode command_return(NONS_Statement &stmt);
	ErrorCode command_if(NONS_Statement &stmt);
	ErrorCode command_mov(NONS_Statement &stmt);
	ErrorCode command_add(NONS_Statement &stmt);
	ErrorCode command_inc(NONS_Statement &stmt);
	ErrorCode command_cmp(NONS_Statement &stmt);
	ErrorCode command_skip(NONS_Statement &stmt);
	ErrorCode command_len(NONS_Statement &stmt);
	ErrorCode command_rnd(NONS_Statement &stmt);
	ErrorCode command_play(NONS_Statement &stmt);
	ErrorCode command_playstop(NONS_Statement &stmt);
	ErrorCode command_wave(NONS_Statement &stmt);
	ErrorCode command_wavestop(NONS_Statement &stmt);
	ErrorCode command_itoa(NONS_Statement &stmt);
	ErrorCode command_intlimit(NONS_Statement &stmt);
	ErrorCode command_wait(NONS_Statement &stmt);
	ErrorCode command_end(NONS_Statement &stmt);
	ErrorCode command_date(NONS_Statement &stmt);
	ErrorCode command_mp3vol(NONS_Statement &stmt);
	ErrorCode command_getmp3vol(NONS_Statement &stmt);
	ErrorCode command_effect(NONS_Statement &stmt);
	ErrorCode command_mid(NONS_Statement &stmt);
	ErrorCode command_dim(NONS_Statement &stmt);
	ErrorCode command_movl(NONS_Statement &stmt);
	ErrorCode command_time(NONS_Statement &stmt);
	ErrorCode command_jumpf(NONS_Statement &stmt);
	ErrorCode command_atoi(NONS_Statement &stmt);
	ErrorCode command_getversion(NONS_Statement &stmt);
	ErrorCode command_dwave(NONS_Statement &stmt);
	ErrorCode command_dwaveload(NONS_Statement &stmt);
	//ErrorCode command_dwavefree(NONS_Statement &stmt);
	ErrorCode command_mp3fadeout(NONS_Statement &stmt);
	ErrorCode command_stop(NONS_Statement &stmt);
	ErrorCode command_getini(NONS_Statement &stmt);
	ErrorCode command_setwindow(NONS_Statement &stmt);
	ErrorCode command_new_set_window(NONS_Statement &stmt);
	ErrorCode command_set_default_font_size(NONS_Statement &stmt);
	ErrorCode command_bg(NONS_Statement &stmt);
	ErrorCode command_setcursor(NONS_Statement &stmt);
	ErrorCode command_br(NONS_Statement &stmt);
	ErrorCode command_ld(NONS_Statement &stmt);
	ErrorCode command_cl(NONS_Statement &stmt);
	ErrorCode command_tal(NONS_Statement &stmt);
	ErrorCode command_undocumented(NONS_Statement &stmt);
	ErrorCode command_unimplemented(NONS_Statement &stmt);
	ErrorCode command_lsp(NONS_Statement &stmt);
	ErrorCode command_csp(NONS_Statement &stmt);
	ErrorCode command_unalias(NONS_Statement &stmt);
	ErrorCode command_vsp(NONS_Statement &stmt);
	ErrorCode command_windoweffect(NONS_Statement &stmt);
	ErrorCode command_literal_print(NONS_Statement &stmt);
	ErrorCode command_print(NONS_Statement &stmt);
	ErrorCode command_delay(NONS_Statement &stmt);
	ErrorCode command_monocro(NONS_Statement &stmt);
	ErrorCode command_nega(NONS_Statement &stmt);
	ErrorCode command_textonoff(NONS_Statement &stmt);
	ErrorCode command_erasetextwindow(NONS_Statement &stmt);
	ErrorCode command_for(NONS_Statement &stmt);
	ErrorCode command_next(NONS_Statement &stmt);
	ErrorCode command_break(NONS_Statement &stmt);
	ErrorCode command_effectblank(NONS_Statement &stmt);
	ErrorCode command_select(NONS_Statement &stmt);
	ErrorCode command_selectcolor(NONS_Statement &stmt);
	ErrorCode command_selectvoice(NONS_Statement &stmt);
	ErrorCode command_trap(NONS_Statement &stmt);
	ErrorCode command_defaultspeed(NONS_Statement &stmt);
	ErrorCode command_clickstr(NONS_Statement &stmt);
	ErrorCode command_click(NONS_Statement &stmt);
	ErrorCode command_autoclick(NONS_Statement &stmt);
	ErrorCode command_textclear(NONS_Statement &stmt);
	ErrorCode command_locate(NONS_Statement &stmt);
	ErrorCode command_textspeed(NONS_Statement &stmt);
	ErrorCode command_repaint(NONS_Statement &stmt);
	ErrorCode command_resettimer(NONS_Statement &stmt);
	ErrorCode command_waittimer(NONS_Statement &stmt);
	ErrorCode command_gettimer(NONS_Statement &stmt);
	ErrorCode command_rmenu(NONS_Statement &stmt);
	ErrorCode command_menusetwindow(NONS_Statement &stmt);
	//ErrorCode command_kidokuskip(NONS_Statement &stmt);
	ErrorCode command_menuselectcolor(NONS_Statement &stmt);
	ErrorCode command_savename(NONS_Statement &stmt);
	ErrorCode command_menuselectvoice(NONS_Statement &stmt);
	ErrorCode command_rmode(NONS_Statement &stmt);
	//ErrorCode command_skipoff(NONS_Statement &stmt);
	ErrorCode command_savenumber(NONS_Statement &stmt);
	ErrorCode command_systemcall(NONS_Statement &stmt);
	ErrorCode command_reset(NONS_Statement &stmt);
	ErrorCode command_btndef(NONS_Statement &stmt);
	ErrorCode command_btn(NONS_Statement &stmt);
	ErrorCode command_btnwait(NONS_Statement &stmt);
	ErrorCode command_msp(NONS_Statement &stmt);
	ErrorCode command_use_new_if(NONS_Statement &stmt);
	ErrorCode command_getbtntimer(NONS_Statement &stmt);
	ErrorCode command_btntime(NONS_Statement &stmt);
	ErrorCode command_humanz(NONS_Statement &stmt);
	ErrorCode command_sinusoidal_quake(NONS_Statement &stmt);
	ErrorCode command_quake(NONS_Statement &stmt);
	ErrorCode command_filelog(NONS_Statement &stmt);
	ErrorCode command_lookbackbutton(NONS_Statement &stmt);
	ErrorCode command_lookbackcolor(NONS_Statement &stmt);
	ErrorCode command_lookbackflush(NONS_Statement &stmt);
	ErrorCode command_savegame(NONS_Statement &stmt);
	ErrorCode command_loadgame(NONS_Statement &stmt);
	ErrorCode command_centerh(NONS_Statement &stmt);
	ErrorCode command_centerv(NONS_Statement &stmt);
	ErrorCode command_blt(NONS_Statement &stmt);
	ErrorCode command_fileexist(NONS_Statement &stmt);
	ErrorCode command_menu_full(NONS_Statement &stmt);
	ErrorCode command_savefileexist(NONS_Statement &stmt);
	ErrorCode command_savescreenshot(NONS_Statement &stmt);
	ErrorCode command_savetime(NONS_Statement &stmt);
	ErrorCode command_savetime2(NONS_Statement &stmt);
	ErrorCode command_split(NONS_Statement &stmt);
	ErrorCode command_isdown(NONS_Statement &stmt);
	ErrorCode command_isfull(NONS_Statement &stmt);
	ErrorCode command_getcursorpos(NONS_Statement &stmt);
	ErrorCode command_textgosub(NONS_Statement &stmt);
	ErrorCode command_ispage(NONS_Statement &stmt);
	ErrorCode command_labellog(NONS_Statement &stmt);
	ErrorCode command_underline(NONS_Statement &stmt);
	ErrorCode command_stdout(NONS_Statement &stmt);
	ErrorCode command_versionstr(NONS_Statement &stmt);
	ErrorCode command_cell(NONS_Statement &stmt);
	ErrorCode command_bgcopy(NONS_Statement &stmt);
	ErrorCode command_draw(NONS_Statement &stmt);
	ErrorCode command_drawbg(NONS_Statement &stmt);
	ErrorCode command_drawclear(NONS_Statement &stmt);
	ErrorCode command_drawfill(NONS_Statement &stmt);
	ErrorCode command_drawsp(NONS_Statement &stmt);
	ErrorCode command_drawtext(NONS_Statement &stmt);
	ErrorCode command_allsphide(NONS_Statement &stmt);
	ErrorCode command_movN(NONS_Statement &stmt);
	ErrorCode command_humanorder(NONS_Statement &stmt);
	ErrorCode command_loadgosub(NONS_Statement &stmt);
	ErrorCode command_defsub(NONS_Statement &stmt);
	ErrorCode command___userCommandCall__(NONS_Statement &stmt);
	ErrorCode command_getparam(NONS_Statement &stmt);
	ErrorCode command_getcursor(NONS_Statement &stmt);
	ErrorCode command_useescspc(NONS_Statement &stmt);
	ErrorCode command_getpage(NONS_Statement &stmt);
	ErrorCode command_getenter(NONS_Statement &stmt);
	ErrorCode command_gettab(NONS_Statement &stmt);
	ErrorCode command_getfunction(NONS_Statement &stmt);
	ErrorCode command_getinsert(NONS_Statement &stmt);
	ErrorCode command_getzxc(NONS_Statement &stmt);
	ErrorCode command_usewheel(NONS_Statement &stmt);
	ErrorCode command_shadedistance(NONS_Statement &stmt);
	ErrorCode command_tablegoto(NONS_Statement &stmt);
	ErrorCode command_indent(NONS_Statement &stmt);
	ErrorCode command_getscreenshot(NONS_Statement &stmt);
	ErrorCode command_deletescreenshot(NONS_Statement &stmt);
	ErrorCode command_gettext(NONS_Statement &stmt);
	ErrorCode command_maxkaisoupage(NONS_Statement &stmt);
	ErrorCode command_checkpage(NONS_Statement &stmt);
	ErrorCode command_getlog(NONS_Statement &stmt);
	ErrorCode command_async_effect(NONS_Statement &stmt);
	ErrorCode command_add_filter(NONS_Statement &stmt);
	ErrorCode command_avi(NONS_Statement &stmt);
	ErrorCode command_base_resolution(NONS_Statement &stmt);
	ErrorCode command_use_nice_svg(NONS_Statement &stmt);
	ErrorCode command_transmode(NONS_Statement &stmt);
	ErrorCode command_clock(NONS_Statement &stmt);
	/*
	ErrorCode command_(NONS_Statement &stmt);
	*/
public:
	NONS_VariableStore *store;
	NONS_GFXstore *gfx_store;
	NONS_ScreenSpace *screen;
	NONS_Audio *audio;
	NONS_Script *script;
	NONS_ScriptInterpreter(bool initialize=1);
	~NONS_ScriptInterpreter();
	bool interpretNextLine();
	NONS_Font *main_font;
	NONS_FontCache *font_cache;
	ErrorCode interpretString(NONS_Statement &stmt,NONS_ScriptLine *lineNo,ulong offset);
	ErrorCode interpretString(const std::wstring &str,NONS_ScriptLine *lineNo,ulong offset);
	ulong totalCommands();
	ulong implementedCommands();
	void listImplementation();
	ErrorCode load(int file);
	bool save(int file);
	std::wstring convertParametersToString(NONS_Statement &line);
	ulong getCurrentTextgosubLevel();
	ulong insideTextgosub();
	bool goto_label(const std::wstring &label);
	bool gosub_label(const std::wstring &label);
	void stop();
	void getCommandListing(std::vector<std::wstring> &vector);
	void getSymbolListing(std::vector<std::wstring> &vector);
	std::wstring getValue(const std::wstring &str);
	std::wstring interpretFromConsole(const std::wstring &str);
	void queue(NONS_ScriptLine *line);
	bool generic_play(const std::wstring &filename,bool from_archive);
	ErrorCode play_video(const std::wstring &filename,bool skippable);
};

extern NONS_ScriptInterpreter *gScriptInterpreter;
#endif
