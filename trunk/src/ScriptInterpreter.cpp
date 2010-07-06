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

#include "ScriptInterpreter.h"
#include "IOFunctions.h"
#include "CommandLineOptions.h"
#include "Plugin/LibraryLoader.h"
#include <iomanip>
#include <iostream>
#include "version.h"

#if NONS_SYS_WINDOWS
#include <windows.h>
HWND mainWindow=0;
#endif

DECLSPEC volatile bool ctrlIsPressed=0;
DECLSPEC volatile bool forceSkip=0;
NONS_ScriptInterpreter *gScriptInterpreter=0;

#undef ABS
#include "SDL_bilinear.h"

#if 0
SDL_Surface *(*rotationFunction)(SDL_Surface *,double)=SDL_RotateSmooth;
SDL_Surface *(*resizeFunction)(SDL_Surface *,int,int)=SDL_ResizeSmooth;
#else
SDL_Surface *(*rotationFunction)(SDL_Surface *,double)=SDL_Rotate;
SDL_Surface *(*resizeFunction)(SDL_Surface *,int,int)=SDL_Resize;
#endif

NONS_DefineFlag ALLOW_IN_DEFINE;
NONS_RunFlag ALLOW_IN_RUN;

ErrorCode getVar(NONS_VariableMember *&var,const std::wstring &str,NONS_VariableStore *store){
	ErrorCode error;
	var=store->retrieve(str,&error);
	if (!var)
		return error;
	if (var->isConstant())
		return NONS_EXPECTED_VARIABLE;
	if (var->getType()==INTEGER_ARRAY)
		return NONS_EXPECTED_SCALAR;
	return NONS_NO_ERROR;
}

ErrorCode getIntVar(NONS_VariableMember *&var,const std::wstring &str,NONS_VariableStore *store){
	ErrorCode error=getVar(var,str,store);
	HANDLE_POSSIBLE_ERRORS(error);
	if (var->getType()!=INTEGER)
		return NONS_EXPECTED_NUMERIC_VARIABLE;
	return NONS_NO_ERROR;
}

ErrorCode getStrVar(NONS_VariableMember *&var,const std::wstring &str,NONS_VariableStore *store){
	ErrorCode error=getVar(var,str,store);
	HANDLE_POSSIBLE_ERRORS(error);
	if (var->getType()!=INTEGER)
		return NONS_EXPECTED_STRING_VARIABLE;
	return NONS_NO_ERROR;
}

printingPage &printingPage::operator=(const printingPage &b){
	this->print=b.print;
	this->reduced=b.reduced;
	this->stops=b.stops;
	return *this;
}


NONS_StackElement::NONS_StackElement(ulong level){
	this->type=StackFrameType::UNDEFINED;
	this->var=0;
	this->from=0;
	this->to=0;
	this->step=0;
	this->textgosubLevel=level;
	this->textgosubTriggeredBy=0;
}

NONS_StackElement::NONS_StackElement(const std::pair<ulong,ulong> &returnTo,const NONS_ScriptLine &interpretAtReturn,ulong beginAtStatement,ulong level)
		:interpretAtReturn(interpretAtReturn,beginAtStatement){
	this->returnTo.line=returnTo.first;
	this->returnTo.statement=returnTo.second;
	this->type=StackFrameType::SUBROUTINE_CALL;
	this->textgosubLevel=level;
	this->textgosubTriggeredBy=0;
}

NONS_StackElement::NONS_StackElement(NONS_VariableMember *variable,const std::pair<ulong,ulong> &startStatement,long from,long to,long step,ulong level){
	this->returnTo.line=startStatement.first;
	this->returnTo.statement=startStatement.second;
	this->type=StackFrameType::FOR_NEST;
	this->var=variable;
	this->from=from;
	this->to=to;
	this->step=step;
	this->end=this->returnTo;
	this->textgosubLevel=level;
	this->textgosubTriggeredBy=0;
}

NONS_StackElement::NONS_StackElement(const std::vector<printingPage> &pages,wchar_t trigger,ulong level){
	this->type=StackFrameType::TEXTGOSUB_CALL;
	this->textgosubLevel=level;
	this->pages=pages;
	this->textgosubTriggeredBy=trigger;
}

NONS_StackElement::NONS_StackElement(NONS_StackElement *copy,const std::vector<std::wstring> &vector){
	this->interpretAtReturn=copy->interpretAtReturn;
	this->returnTo.line=copy->returnTo.line;
	this->returnTo.statement=copy->returnTo.statement;
	this->textgosubLevel=copy->textgosubLevel;
	this->textgosubTriggeredBy=copy->textgosubTriggeredBy;
	this->type=StackFrameType::USERCMD_CALL;
	this->parameters=vector;
}

ConfigFile settings;

ErrorCode init_script(NONS_Script *&script,const std::wstring &filename,ENCODING::ENCODING encoding,ENCRYPTION::ENCRYPTION encryption){
	script=new NONS_Script();
	ErrorCode error_code=script->init(filename,encoding,encryption);
	if (error_code!=NONS_NO_ERROR){
		delete script;
		script=0;
		return error_code;
	}
	return NONS_NO_ERROR;
}

ErrorCode init_script(NONS_Script *&script,ENCODING::ENCODING encoding){
	const wchar_t *names[]={
		L"0.txt",
		L"00.txt",
		L"nscr_sec.dat",
		L"nscript.___",
		L"nscript.dat",
		0
	};
	ENCRYPTION::ENCRYPTION encryptions[]={
		ENCRYPTION::NONE,
		ENCRYPTION::NONE,
		ENCRYPTION::VARIABLE_XOR,
		ENCRYPTION::TRANSFORM_THEN_XOR84,
		ENCRYPTION::XOR84
	};
	for (ulong a=0;names[a];a++){
		ErrorCode error_code=init_script(script,names[a],encoding,encryptions[a]);
		if (error_code==NONS_NO_ERROR)
			return NONS_NO_ERROR;
		if (error_code==NONS_NOT_IMPLEMENTED)
			return NONS_NOT_IMPLEMENTED;
	}
	return NONS_SCRIPT_NOT_FOUND;
}

std::wstring getDefaultFontFilename(){
	if (!settings.exists(L"default font"))
		settings.assignWString(L"default font",L"default.ttf");
	return settings.getWString(L"default font");
}

NONS_ScreenSpace *init_screen(NONS_FontCache &fc){
	NONS_ScreenSpace *screen=new NONS_ScreenSpace(20,fc);
	if (!CLOptions.play.size()){
		screen->output->shadeLayer->Clear();
		screen->Background->Clear();
		screen->BlendNoCursor(1);
	}
	std::cout <<"Screen initialized."<<std::endl;
	return screen;
}

NONS_ScriptInterpreter::NONS_ScriptInterpreter(bool initialize):stop_interpreting(0){
	this->arrowCursor=0;
	this->pageCursor=0;
	this->menu=0;
	this->imageButtons=0;
	this->saveGame=0;
	this->screen=0;
	this->audio=0;
	this->script=0;
	this->store=0;
	this->gfx_store=0;
	this->main_font=0;
	this->font_cache=0;
	this->thread=0;
	this->screenshot=0;
	if (initialize){
		this->init();
	}
	this->was_initialized=initialize;

	this->commandList[L"abssetcursor"]=            &NONS_ScriptInterpreter::command_setcursor                            |ALLOW_IN_RUN;
	this->commandList[L"add"]=                     &NONS_ScriptInterpreter::command_add                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"allsphide"]=               &NONS_ScriptInterpreter::command_allsphide                            |ALLOW_IN_RUN;
	this->commandList[L"allspresume"]=             &NONS_ScriptInterpreter::command_allsphide                            |ALLOW_IN_RUN;
	this->commandList[L"amsp"]=                    &NONS_ScriptInterpreter::command_msp                                  |ALLOW_IN_RUN;
	this->commandList[L"arc"]=                     &NONS_ScriptInterpreter::command_nsa                  |ALLOW_IN_DEFINE             ;
	this->commandList[L"atoi"]=                    &NONS_ScriptInterpreter::command_atoi                 |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"autoclick"]=               &NONS_ScriptInterpreter::command_autoclick                            |ALLOW_IN_RUN;
	this->commandList[L"automode_time"]=           0                                                     |ALLOW_IN_DEFINE             ;
	this->commandList[L"automode"]=                0                                                     |ALLOW_IN_DEFINE             ;
	this->commandList[L"avi"]=                     &NONS_ScriptInterpreter::command_avi                                  |ALLOW_IN_RUN;
	this->commandList[L"bar"]=                     0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"barclear"]=                0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"bg"]=                      &NONS_ScriptInterpreter::command_bg                                   |ALLOW_IN_RUN;
	this->commandList[L"bgcopy"]=                  &NONS_ScriptInterpreter::command_bgcopy                               |ALLOW_IN_RUN;
	this->commandList[L"bgcpy"]=                   &NONS_ScriptInterpreter::command_bgcopy                               |ALLOW_IN_RUN;
	this->commandList[L"bgm"]=                     &NONS_ScriptInterpreter::command_play                                 |ALLOW_IN_RUN;
	this->commandList[L"bgmonce"]=                 &NONS_ScriptInterpreter::command_play                                 |ALLOW_IN_RUN;
	this->commandList[L"bgmstop"]=                 &NONS_ScriptInterpreter::command_playstop                             |ALLOW_IN_RUN;
	this->commandList[L"bgmvol"]=                  &NONS_ScriptInterpreter::command_mp3vol                               |ALLOW_IN_RUN;
	this->commandList[L"blt"]=                     &NONS_ScriptInterpreter::command_blt                                  |ALLOW_IN_RUN;
	this->commandList[L"br"]=                      &NONS_ScriptInterpreter::command_br                                   |ALLOW_IN_RUN;
	this->commandList[L"break"]=                   &NONS_ScriptInterpreter::command_break                |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"btn"]=                     &NONS_ScriptInterpreter::command_btn                                  |ALLOW_IN_RUN;
	this->commandList[L"btndef"]=                  &NONS_ScriptInterpreter::command_btndef                               |ALLOW_IN_RUN;
	this->commandList[L"btndown"]=                 0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"btntime"]=                 &NONS_ScriptInterpreter::command_btntime                              |ALLOW_IN_RUN;
	this->commandList[L"btntime2"]=                &NONS_ScriptInterpreter::command_unimplemented                        |ALLOW_IN_RUN;
	this->commandList[L"btnwait"]=                 &NONS_ScriptInterpreter::command_btnwait                              |ALLOW_IN_RUN;
	this->commandList[L"btnwait2"]=                &NONS_ScriptInterpreter::command_btnwait                              |ALLOW_IN_RUN;
	this->commandList[L"caption"]=                 &NONS_ScriptInterpreter::command_caption              |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"cell"]=                    &NONS_ScriptInterpreter::command_cell                                 |ALLOW_IN_RUN;
	this->commandList[L"cellcheckexbtn"]=          0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"cellcheckspbtn"]=          0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"checkpage"]=               &NONS_ScriptInterpreter::command_checkpage                            |ALLOW_IN_RUN;
	this->commandList[L"chvol"]=                   0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"cl"]=                      &NONS_ScriptInterpreter::command_cl                                   |ALLOW_IN_RUN;
	this->commandList[L"click"]=                   &NONS_ScriptInterpreter::command_click                                |ALLOW_IN_RUN;
	this->commandList[L"clickstr"]=                &NONS_ScriptInterpreter::command_clickstr             |ALLOW_IN_DEFINE             ;
	this->commandList[L"clickvoice"]=              0                                                     |ALLOW_IN_DEFINE             ;
	this->commandList[L"cmp"]=                     &NONS_ScriptInterpreter::command_cmp                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"cos"]=                     &NONS_ScriptInterpreter::command_add                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"csel"]=                    0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"cselbtn"]=                 0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"cselgoto"]=                0                                                     |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"csp"]=                     &NONS_ScriptInterpreter::command_csp                                  |ALLOW_IN_RUN;
	this->commandList[L"date"]=                    &NONS_ScriptInterpreter::command_date                                 |ALLOW_IN_RUN;
	this->commandList[L"dec"]=                     &NONS_ScriptInterpreter::command_inc                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"defaultfont"]=             &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE             ;
	this->commandList[L"defaultspeed"]=            &NONS_ScriptInterpreter::command_defaultspeed         |ALLOW_IN_DEFINE             ;
	this->commandList[L"definereset"]=             &NONS_ScriptInterpreter::command_reset                                |ALLOW_IN_RUN;
	this->commandList[L"defmp3vol"]=               &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE             ;
	this->commandList[L"defsevol"]=                &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE             ;
	this->commandList[L"defsub"]=                  &NONS_ScriptInterpreter::command_defsub               |ALLOW_IN_DEFINE             ;
	this->commandList[L"defvoicevol"]=             &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE             ;
	this->commandList[L"delay"]=                   &NONS_ScriptInterpreter::command_delay                                |ALLOW_IN_RUN;
	this->commandList[L"deletescreenshot"]=        &NONS_ScriptInterpreter::command_deletescreenshot                     |ALLOW_IN_RUN;
	this->commandList[L"dim"]=                     &NONS_ScriptInterpreter::command_dim                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"div"]=                     &NONS_ScriptInterpreter::command_add                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"draw"]=                    &NONS_ScriptInterpreter::command_draw                                 |ALLOW_IN_RUN;
	this->commandList[L"drawbg"]=                  &NONS_ScriptInterpreter::command_drawbg                               |ALLOW_IN_RUN;
	this->commandList[L"drawbg2"]=                 &NONS_ScriptInterpreter::command_drawbg                               |ALLOW_IN_RUN;
	this->commandList[L"drawclear"]=               &NONS_ScriptInterpreter::command_drawclear                            |ALLOW_IN_RUN;
	this->commandList[L"drawfill"]=                &NONS_ScriptInterpreter::command_drawfill                             |ALLOW_IN_RUN;
	this->commandList[L"drawsp"]=                  &NONS_ScriptInterpreter::command_drawsp                               |ALLOW_IN_RUN;
	this->commandList[L"drawsp2"]=                 &NONS_ScriptInterpreter::command_drawsp                               |ALLOW_IN_RUN;
	this->commandList[L"drawsp3"]=                 &NONS_ScriptInterpreter::command_drawsp                               |ALLOW_IN_RUN;
	this->commandList[L"drawtext"]=                &NONS_ScriptInterpreter::command_drawtext                             |ALLOW_IN_RUN;
	this->commandList[L"dwave"]=                   &NONS_ScriptInterpreter::command_dwave                                |ALLOW_IN_RUN;
	this->commandList[L"dwaveload"]=               &NONS_ScriptInterpreter::command_dwaveload                            |ALLOW_IN_RUN;
	this->commandList[L"dwaveloop"]=               &NONS_ScriptInterpreter::command_dwave                                |ALLOW_IN_RUN;
	this->commandList[L"dwaveplay"]=               0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"dwaveplayloop"]=           0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"dwavestop"]=               0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"effect"]=                  &NONS_ScriptInterpreter::command_effect               |ALLOW_IN_DEFINE             ;
	this->commandList[L"effectblank"]=             &NONS_ScriptInterpreter::command_effectblank          |ALLOW_IN_DEFINE             ;
	this->commandList[L"effectcut"]=               &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE             ;
	this->commandList[L"end"]=                     &NONS_ScriptInterpreter::command_end                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"erasetextwindow"]=         &NONS_ScriptInterpreter::command_erasetextwindow                      |ALLOW_IN_RUN;
	this->commandList[L"exbtn_d"]=                 0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"exbtn"]=                   0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"exec_dll"]=                &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"existspbtn"]=              &NONS_ScriptInterpreter::command_undocumented                         |ALLOW_IN_RUN;
	this->commandList[L"fileexist"]=               &NONS_ScriptInterpreter::command_fileexist                            |ALLOW_IN_RUN;
	this->commandList[L"filelog"]=                 &NONS_ScriptInterpreter::command_filelog              |ALLOW_IN_DEFINE             ;
	this->commandList[L"for"]=                     &NONS_ScriptInterpreter::command_for                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"game"]=                    &NONS_ScriptInterpreter::command_game                 |ALLOW_IN_DEFINE             ;
	this->commandList[L"getbgmvol"]=               &NONS_ScriptInterpreter::command_getmp3vol                            |ALLOW_IN_RUN;
	this->commandList[L"getbtntimer"]=             &NONS_ScriptInterpreter::command_getbtntimer                          |ALLOW_IN_RUN;
	this->commandList[L"getcselnum"]=              0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"getcselstr"]=              0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"getcursor"]=               &NONS_ScriptInterpreter::command_getcursor                            |ALLOW_IN_RUN;
	this->commandList[L"getcursorpos"]=            &NONS_ScriptInterpreter::command_getcursorpos                         |ALLOW_IN_RUN;
	this->commandList[L"getenter"]=                &NONS_ScriptInterpreter::command_getenter                             |ALLOW_IN_RUN;
	this->commandList[L"getfunction"]=             &NONS_ScriptInterpreter::command_getfunction                          |ALLOW_IN_RUN;
	this->commandList[L"getinsert"]=               &NONS_ScriptInterpreter::command_getinsert                            |ALLOW_IN_RUN;
	this->commandList[L"getlog"]=                  &NONS_ScriptInterpreter::command_getlog                               |ALLOW_IN_RUN;
	this->commandList[L"getmousepos"]=             0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"getmp3vol"]=               &NONS_ScriptInterpreter::command_getmp3vol                            |ALLOW_IN_RUN;
	this->commandList[L"getpage"]=                 &NONS_ScriptInterpreter::command_getpage                              |ALLOW_IN_RUN;
	this->commandList[L"getpageup"]=               &NONS_ScriptInterpreter::command_undocumented                         |ALLOW_IN_RUN;
	this->commandList[L"getparam"]=                &NONS_ScriptInterpreter::command_getparam             |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"getreg"]=                  &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"getret"]=                  &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"getscreenshot"]=           &NONS_ScriptInterpreter::command_getscreenshot                        |ALLOW_IN_RUN;
	this->commandList[L"getsevol"]=                &NONS_ScriptInterpreter::command_undocumented                         |ALLOW_IN_RUN;
	this->commandList[L"getspmode"]=               0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"getspsize"]=               0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"gettab"]=                  &NONS_ScriptInterpreter::command_gettab                               |ALLOW_IN_RUN;
	this->commandList[L"gettag"]=                  0                                                     |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"gettext"]=                 &NONS_ScriptInterpreter::command_gettext                              |ALLOW_IN_RUN;
	this->commandList[L"gettimer"]=                &NONS_ScriptInterpreter::command_gettimer                             |ALLOW_IN_RUN;
	this->commandList[L"getversion"]=              &NONS_ScriptInterpreter::command_getversion                           |ALLOW_IN_RUN;
	this->commandList[L"getvoicevol"]=             &NONS_ScriptInterpreter::command_undocumented                         |ALLOW_IN_RUN;
	this->commandList[L"getzxc"]=                  &NONS_ScriptInterpreter::command_getzxc                               |ALLOW_IN_RUN;
	this->commandList[L"globalon"]=                &NONS_ScriptInterpreter::command_globalon             |ALLOW_IN_DEFINE             ;
	this->commandList[L"gosub"]=                   &NONS_ScriptInterpreter::command_gosub                |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"goto"]=                    &NONS_ScriptInterpreter::command_goto                                 |ALLOW_IN_RUN;
	this->commandList[L"humanorder"]=              &NONS_ScriptInterpreter::command_humanorder                           |ALLOW_IN_RUN;
	this->commandList[L"humanz"]=                  &NONS_ScriptInterpreter::command_humanz               |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"if"]=                      &NONS_ScriptInterpreter::command_if                   |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"inc"]=                     &NONS_ScriptInterpreter::command_inc                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"indent"]=                  &NONS_ScriptInterpreter::command_indent                               |ALLOW_IN_RUN;
	this->commandList[L"input"]=                   &NONS_ScriptInterpreter::command_unimplemented                        |ALLOW_IN_RUN;
	this->commandList[L"insertmenu"]=              &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE             ;
	this->commandList[L"intlimit"]=                &NONS_ScriptInterpreter::command_intlimit             |ALLOW_IN_DEFINE             ;
	this->commandList[L"isdown"]=                  &NONS_ScriptInterpreter::command_isdown                               |ALLOW_IN_RUN;
	this->commandList[L"isfull"]=                  &NONS_ScriptInterpreter::command_isfull                               |ALLOW_IN_RUN;
	this->commandList[L"ispage"]=                  &NONS_ScriptInterpreter::command_ispage                               |ALLOW_IN_RUN;
	this->commandList[L"isskip"]=                  &NONS_ScriptInterpreter::command_unimplemented                        |ALLOW_IN_RUN;
	this->commandList[L"itoa"]=                    &NONS_ScriptInterpreter::command_itoa                 |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"itoa2"]=                   &NONS_ScriptInterpreter::command_itoa                 |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"jumpb"]=                   &NONS_ScriptInterpreter::command_jumpf                                |ALLOW_IN_RUN;
	this->commandList[L"jumpf"]=                   &NONS_ScriptInterpreter::command_jumpf                                |ALLOW_IN_RUN;
	this->commandList[L"kidokumode"]=              &NONS_ScriptInterpreter::command_unimplemented                        |ALLOW_IN_RUN;
	this->commandList[L"kidokuskip"]=              &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE             ;
	this->commandList[L"labellog"]=                &NONS_ScriptInterpreter::command_labellog             |ALLOW_IN_DEFINE             ;
	this->commandList[L"layermessage"]=            &NONS_ScriptInterpreter::command_unimplemented                        |ALLOW_IN_RUN;
	this->commandList[L"ld"]=                      &NONS_ScriptInterpreter::command_ld                                   |ALLOW_IN_RUN;
	this->commandList[L"len"]=                     &NONS_ScriptInterpreter::command_len                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"linepage"]=                &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE             ;
	this->commandList[L"linepage2"]=               &NONS_ScriptInterpreter::command_unimplemented                        |ALLOW_IN_RUN;
	this->commandList[L"loadgame"]=                &NONS_ScriptInterpreter::command_loadgame                             |ALLOW_IN_RUN;
	this->commandList[L"loadgosub"]=               &NONS_ScriptInterpreter::command_loadgosub            |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"locate"]=                  &NONS_ScriptInterpreter::command_locate                               |ALLOW_IN_RUN;
	this->commandList[L"logsp"]=                   0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"logsp2"]=                  0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"lookbackbutton"]=          &NONS_ScriptInterpreter::command_lookbackbutton       |ALLOW_IN_DEFINE             ;
	this->commandList[L"lookbackcolor"]=           &NONS_ScriptInterpreter::command_lookbackcolor        |ALLOW_IN_DEFINE             ;
	this->commandList[L"lookbackflush"]=           &NONS_ScriptInterpreter::command_lookbackflush                        |ALLOW_IN_RUN;
	this->commandList[L"lookbacksp"]=              0                                                     |ALLOW_IN_DEFINE             ;
	this->commandList[L"loopbgm"]=                 &NONS_ScriptInterpreter::command_play                                 |ALLOW_IN_RUN;
	this->commandList[L"loopbgmstop"]=             &NONS_ScriptInterpreter::command_playstop                             |ALLOW_IN_RUN;
	this->commandList[L"lr_trap"]=                 &NONS_ScriptInterpreter::command_trap                                 |ALLOW_IN_RUN;
	this->commandList[L"lr_trap2"]=                &NONS_ScriptInterpreter::command_trap                                 |ALLOW_IN_RUN;
	this->commandList[L"lsp"]=                     &NONS_ScriptInterpreter::command_lsp                                  |ALLOW_IN_RUN;
	//this->commandList[L"lsp2"]=                  &NONS_ScriptInterpreter::command_lsp2                                 |ALLOW_IN_RUN;
	this->commandList[L"lsph"]=                    &NONS_ScriptInterpreter::command_lsp                                  |ALLOW_IN_RUN;
	this->commandList[L"maxkaisoupage"]=           &NONS_ScriptInterpreter::command_maxkaisoupage        |ALLOW_IN_DEFINE             ;
	this->commandList[L"menu_automode"]=           &NONS_ScriptInterpreter::command_undocumented                         |ALLOW_IN_RUN;
	this->commandList[L"menu_full"]=               &NONS_ScriptInterpreter::command_menu_full                            |ALLOW_IN_RUN;
	this->commandList[L"menu_window"]=             &NONS_ScriptInterpreter::command_menu_full                            |ALLOW_IN_RUN;
	this->commandList[L"menuselectcolor"]=         &NONS_ScriptInterpreter::command_menuselectcolor      |ALLOW_IN_DEFINE             ;
	this->commandList[L"menuselectvoice"]=         &NONS_ScriptInterpreter::command_menuselectvoice      |ALLOW_IN_DEFINE             ;
	this->commandList[L"menusetwindow"]=           &NONS_ScriptInterpreter::command_menusetwindow        |ALLOW_IN_DEFINE             ;
	this->commandList[L"mid"]=                     &NONS_ScriptInterpreter::command_mid                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"mod"]=                     &NONS_ScriptInterpreter::command_add                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"mode_ext"]=                0                                                     |ALLOW_IN_DEFINE             ;
	this->commandList[L"mode_saya"]=               0                                                     |ALLOW_IN_DEFINE             ;
	this->commandList[L"monocro"]=                 &NONS_ScriptInterpreter::command_monocro                              |ALLOW_IN_RUN;
	this->commandList[L"mov"]=                     &NONS_ScriptInterpreter::command_mov                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"mov3"]=                    &NONS_ScriptInterpreter::command_movN                 |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"mov4"]=                    &NONS_ScriptInterpreter::command_movN                 |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"mov5"]=                    &NONS_ScriptInterpreter::command_movN                 |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"mov6"]=                    &NONS_ScriptInterpreter::command_movN                 |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"mov7"]=                    &NONS_ScriptInterpreter::command_movN                 |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"mov8"]=                    &NONS_ScriptInterpreter::command_movN                 |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"mov9"]=                    &NONS_ScriptInterpreter::command_movN                 |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"mov10"]=                   &NONS_ScriptInterpreter::command_movN                 |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"movemousecursor"]=         0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"movl"]=                    &NONS_ScriptInterpreter::command_movl                 |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"mp3"]=                     &NONS_ScriptInterpreter::command_play                                 |ALLOW_IN_RUN;
	this->commandList[L"mp3fadeout"]=              &NONS_ScriptInterpreter::command_mp3fadeout           |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"mp3loop"]=                 &NONS_ScriptInterpreter::command_play                                 |ALLOW_IN_RUN;
	this->commandList[L"mp3save"]=                 &NONS_ScriptInterpreter::command_play                                 |ALLOW_IN_RUN;
	this->commandList[L"mp3stop"]=                 &NONS_ScriptInterpreter::command_playstop                             |ALLOW_IN_RUN;
	this->commandList[L"mp3vol"]=                  &NONS_ScriptInterpreter::command_mp3vol                               |ALLOW_IN_RUN;
	this->commandList[L"mpegplay"]=                &NONS_ScriptInterpreter::command_avi                                  |ALLOW_IN_RUN;
	this->commandList[L"msp"]=                     &NONS_ScriptInterpreter::command_msp                                  |ALLOW_IN_RUN;
	this->commandList[L"mul"]=                     &NONS_ScriptInterpreter::command_add                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"nega"]=                    &NONS_ScriptInterpreter::command_nega                                 |ALLOW_IN_RUN;
	this->commandList[L"next"]=                    &NONS_ScriptInterpreter::command_next                 |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"notif"]=                   &NONS_ScriptInterpreter::command_if                   |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"nsa"]=                     &NONS_ScriptInterpreter::command_nsa                  |ALLOW_IN_DEFINE             ;
	this->commandList[L"ns2"]=                     &NONS_ScriptInterpreter::command_nsa                  |ALLOW_IN_DEFINE             ;
	this->commandList[L"ns3"]=                     &NONS_ScriptInterpreter::command_nsa                  |ALLOW_IN_DEFINE             ;
	this->commandList[L"nsadir"]=                  &NONS_ScriptInterpreter::command_nsadir               |ALLOW_IN_DEFINE             ;
	this->commandList[L"numalias"]=                &NONS_ScriptInterpreter::command_alias                |ALLOW_IN_DEFINE             ;
	this->commandList[L"ofscopy"]=                 &NONS_ScriptInterpreter::command_undocumented                         |ALLOW_IN_RUN;
	this->commandList[L"ofscpy"]=                  &NONS_ScriptInterpreter::command_undocumented                         |ALLOW_IN_RUN;
	this->commandList[L"play"]=                    &NONS_ScriptInterpreter::command_play                                 |ALLOW_IN_RUN;
	this->commandList[L"playonce"]=                &NONS_ScriptInterpreter::command_play                                 |ALLOW_IN_RUN;
	this->commandList[L"playstop"]=                &NONS_ScriptInterpreter::command_playstop                             |ALLOW_IN_RUN;
	this->commandList[L"pretextgosub"]=            0                                                     |ALLOW_IN_DEFINE             ;
	this->commandList[L"print"]=                   &NONS_ScriptInterpreter::command_print                                |ALLOW_IN_RUN;
	this->commandList[L"prnum"]=                   &NONS_ScriptInterpreter::command_unimplemented                        |ALLOW_IN_RUN;
	this->commandList[L"prnumclear"]=              &NONS_ScriptInterpreter::command_unimplemented                        |ALLOW_IN_RUN;
	this->commandList[L"puttext"]=                 &NONS_ScriptInterpreter::command_literal_print                        |ALLOW_IN_RUN;
	this->commandList[L"quake"]=                   &NONS_ScriptInterpreter::command_quake                                |ALLOW_IN_RUN;
	this->commandList[L"quakex"]=                  &NONS_ScriptInterpreter::command_sinusoidal_quake                     |ALLOW_IN_RUN;
	this->commandList[L"quakey"]=                  &NONS_ScriptInterpreter::command_sinusoidal_quake                     |ALLOW_IN_RUN;
	this->commandList[L"repaint"]=                 &NONS_ScriptInterpreter::command_repaint                              |ALLOW_IN_RUN;
	this->commandList[L"reset"]=                   &NONS_ScriptInterpreter::command_reset                                |ALLOW_IN_RUN;
	this->commandList[L"resetmenu"]=               &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE             ;
	this->commandList[L"resettimer"]=              &NONS_ScriptInterpreter::command_resettimer                           |ALLOW_IN_RUN;
	this->commandList[L"return"]=                  &NONS_ScriptInterpreter::command_return               |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"rmenu"]=                   &NONS_ScriptInterpreter::command_rmenu                |ALLOW_IN_DEFINE             ;
	this->commandList[L"rmode"]=                   &NONS_ScriptInterpreter::command_rmode                                |ALLOW_IN_RUN;
	this->commandList[L"rnd"]=                     &NONS_ScriptInterpreter::command_rnd                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"rnd2"]=                    &NONS_ScriptInterpreter::command_rnd                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"roff"]=                    &NONS_ScriptInterpreter::command_rmode                |ALLOW_IN_DEFINE             ;
	this->commandList[L"rubyoff"]=                 &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"rubyon"]=                  &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"sar"]=                     &NONS_ScriptInterpreter::command_nsa                                  |ALLOW_IN_RUN;
	this->commandList[L"savefileexist"]=           &NONS_ScriptInterpreter::command_savefileexist                        |ALLOW_IN_RUN;
	this->commandList[L"savegame"]=                &NONS_ScriptInterpreter::command_savegame                             |ALLOW_IN_RUN;
	this->commandList[L"savename"]=                &NONS_ScriptInterpreter::command_savename             |ALLOW_IN_DEFINE             ;
	this->commandList[L"savenumber"]=              &NONS_ScriptInterpreter::command_savenumber           |ALLOW_IN_DEFINE             ;
	this->commandList[L"saveoff"]=                 &NONS_ScriptInterpreter::command_unimplemented                        |ALLOW_IN_RUN;
	this->commandList[L"saveon"]=                  &NONS_ScriptInterpreter::command_unimplemented                        |ALLOW_IN_RUN;
	this->commandList[L"savescreenshot"]=          &NONS_ScriptInterpreter::command_savescreenshot                       |ALLOW_IN_RUN;
	this->commandList[L"savescreenshot2"]=         &NONS_ScriptInterpreter::command_savescreenshot                       |ALLOW_IN_RUN;
	this->commandList[L"savetime"]=                &NONS_ScriptInterpreter::command_savetime                             |ALLOW_IN_RUN;
	this->commandList[L"savetime2"]=               &NONS_ScriptInterpreter::command_savetime2                            |ALLOW_IN_RUN;
	this->commandList[L"select"]=                  &NONS_ScriptInterpreter::command_select                               |ALLOW_IN_RUN;
	this->commandList[L"selectbtnwait"]=           0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"selectcolor"]=             &NONS_ScriptInterpreter::command_selectcolor          |ALLOW_IN_DEFINE             ;
	this->commandList[L"selectvoice"]=             &NONS_ScriptInterpreter::command_selectvoice          |ALLOW_IN_DEFINE             ;
	this->commandList[L"selgosub"]=                &NONS_ScriptInterpreter::command_select                               |ALLOW_IN_RUN;
	this->commandList[L"selnum"]=                  &NONS_ScriptInterpreter::command_select                               |ALLOW_IN_RUN;
	this->commandList[L"setcursor"]=               &NONS_ScriptInterpreter::command_setcursor                            |ALLOW_IN_RUN;
	this->commandList[L"setlayer"]=                &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE             ;
	this->commandList[L"setwindow"]=               &NONS_ScriptInterpreter::command_setwindow                            |ALLOW_IN_RUN;
	this->commandList[L"setwindow2"]=              0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"setwindow3"]=              0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"sevol"]=                   0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"shadedistance"]=           &NONS_ScriptInterpreter::command_shadedistance        |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"sin"]=                     &NONS_ScriptInterpreter::command_add                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"skip"]=                    &NONS_ScriptInterpreter::command_skip                 |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"skipoff"]=                 &NONS_ScriptInterpreter::command_unimplemented                        |ALLOW_IN_RUN;
	this->commandList[L"soundpressplgin"]=         &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE             ;
	this->commandList[L"sp_rgb_gradation"]=        &NONS_ScriptInterpreter::command_undocumented                         |ALLOW_IN_RUN;
	this->commandList[L"spbtn"]=                   0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"spclclk"]=                 0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"spi"]=                     &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE             ;
	this->commandList[L"split"]=                   &NONS_ScriptInterpreter::command_split                |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"splitstring"]=             0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"spreload"]=                0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"spstr"]=                   0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"stop"]=                    &NONS_ScriptInterpreter::command_stop                                 |ALLOW_IN_RUN;
	this->commandList[L"stralias"]=                &NONS_ScriptInterpreter::command_alias                |ALLOW_IN_DEFINE             ;
	this->commandList[L"strsp"]=                   0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"sub"]=                     &NONS_ScriptInterpreter::command_add                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"systemcall"]=              &NONS_ScriptInterpreter::command_systemcall                           |ALLOW_IN_RUN;
	this->commandList[L"tablegoto"]=               &NONS_ScriptInterpreter::command_tablegoto            |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"tal"]=                     &NONS_ScriptInterpreter::command_tal                                  |ALLOW_IN_RUN;
	this->commandList[L"tan"]=                     &NONS_ScriptInterpreter::command_add                  |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"tateyoko"]=                &NONS_ScriptInterpreter::command_unimplemented                        |ALLOW_IN_RUN;
	this->commandList[L"texec"]=                   0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"textbtnwait"]=             0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"textclear"]=               &NONS_ScriptInterpreter::command_textclear                            |ALLOW_IN_RUN;
	this->commandList[L"textgosub"]=               &NONS_ScriptInterpreter::command_textgosub            |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"texthide"]=                0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"textoff"]=                 &NONS_ScriptInterpreter::command_textonoff                            |ALLOW_IN_RUN;
	this->commandList[L"texton"]=                  &NONS_ScriptInterpreter::command_textonoff                            |ALLOW_IN_RUN;
	this->commandList[L"textshow"]=                0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"textspeed"]=               &NONS_ScriptInterpreter::command_textspeed                            |ALLOW_IN_RUN;
	this->commandList[L"time"]=                    &NONS_ScriptInterpreter::command_time                                 |ALLOW_IN_RUN;
	this->commandList[L"transmode"]=               &NONS_ScriptInterpreter::command_transmode            |ALLOW_IN_DEFINE             ;
	this->commandList[L"trap"]=                    &NONS_ScriptInterpreter::command_trap                                 |ALLOW_IN_RUN;
	this->commandList[L"trap2"]=                   &NONS_ScriptInterpreter::command_trap                                 |ALLOW_IN_RUN;
	this->commandList[L"underline"]=               &NONS_ScriptInterpreter::command_underline            |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"useescspc"]=               &NONS_ScriptInterpreter::command_useescspc            |ALLOW_IN_DEFINE             ;
	this->commandList[L"usewheel"]=                &NONS_ScriptInterpreter::command_usewheel             |ALLOW_IN_DEFINE             ;
	this->commandList[L"versionstr"]=              &NONS_ScriptInterpreter::command_versionstr           |ALLOW_IN_DEFINE             ;
	this->commandList[L"voicevol"]=                0                                                                     |ALLOW_IN_RUN;
	this->commandList[L"vsp"]=                     &NONS_ScriptInterpreter::command_vsp                                  |ALLOW_IN_RUN;
	this->commandList[L"wait"]=                    &NONS_ScriptInterpreter::command_wait                                 |ALLOW_IN_RUN;
	this->commandList[L"waittimer"]=               &NONS_ScriptInterpreter::command_waittimer                            |ALLOW_IN_RUN;
	this->commandList[L"wave"]=                    &NONS_ScriptInterpreter::command_wave                                 |ALLOW_IN_RUN;
	this->commandList[L"waveloop"]=                &NONS_ScriptInterpreter::command_wave                                 |ALLOW_IN_RUN;
	this->commandList[L"wavestop"]=                &NONS_ScriptInterpreter::command_wavestop                             |ALLOW_IN_RUN;
	this->commandList[L"windowback"]=              &NONS_ScriptInterpreter::command_unimplemented        |ALLOW_IN_DEFINE             ;
	this->commandList[L"windoweffect"]=            &NONS_ScriptInterpreter::command_windoweffect         |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"zenkakko"]=                &NONS_ScriptInterpreter::command_unimplemented                        |ALLOW_IN_RUN;
	this->commandList[L"date2"]=                   &NONS_ScriptInterpreter::command_date                                 |ALLOW_IN_RUN;
	this->commandList[L"getini"]=                  &NONS_ScriptInterpreter::command_getini               |ALLOW_IN_DEFINE|ALLOW_IN_RUN;
	this->commandList[L"new_set_window"]=          &NONS_ScriptInterpreter::command_new_set_window                       |ALLOW_IN_RUN;
	this->commandList[L"set_default_font_size"]=   &NONS_ScriptInterpreter::command_set_default_font_size                |ALLOW_IN_RUN;
	this->commandList[L"unalias"]=                 &NONS_ScriptInterpreter::command_unalias                              |ALLOW_IN_RUN;
	this->commandList[L"literal_print"]=           &NONS_ScriptInterpreter::command_literal_print                        |ALLOW_IN_RUN;
	this->commandList[L"use_new_if"]=              &NONS_ScriptInterpreter::command_use_new_if                           |ALLOW_IN_RUN;
	this->commandList[L"centerh"]=                 &NONS_ScriptInterpreter::command_centerh                              |ALLOW_IN_RUN;
	this->commandList[L"centerv"]=                 &NONS_ScriptInterpreter::command_centerv                              |ALLOW_IN_RUN;
	this->commandList[L"killmenu"]=                &NONS_ScriptInterpreter::command_unimplemented                        |ALLOW_IN_RUN;
	this->commandList[L"command_syntax_example"]=  &NONS_ScriptInterpreter::command_unimplemented                        |ALLOW_IN_RUN;
	this->commandList[L"stdout"]=                  &NONS_ScriptInterpreter::command_stdout                               |ALLOW_IN_RUN;
	this->commandList[L"stderr"]=                  &NONS_ScriptInterpreter::command_stdout                               |ALLOW_IN_RUN;
	this->commandList[L""]=                        &NONS_ScriptInterpreter::command___userCommandCall__                  |ALLOW_IN_RUN;
	this->commandList[L"async_effect"]=            &NONS_ScriptInterpreter::command_async_effect                         |ALLOW_IN_RUN;
	this->commandList[L"add_overall_filter"]=      &NONS_ScriptInterpreter::command_add_filter                           |ALLOW_IN_RUN;
	this->commandList[L"add_filter"]=              &NONS_ScriptInterpreter::command_add_filter                           |ALLOW_IN_RUN;
	this->commandList[L"base_resolution"]=         &NONS_ScriptInterpreter::command_base_resolution      |ALLOW_IN_DEFINE             ;
	this->commandList[L"use_nice_svg"]=            &NONS_ScriptInterpreter::command_use_nice_svg                         |ALLOW_IN_RUN;
	this->commandList[L"clock"]=                   &NONS_ScriptInterpreter::command_clock                                |ALLOW_IN_RUN;
	/*
	this->commandList[L""]=&NONS_ScriptInterpreter::command_;
	*/
	this->allowedCommandList.insert(L"add");
	this->allowedCommandList.insert(L"atoi");
	this->allowedCommandList.insert(L"cmp");
	this->allowedCommandList.insert(L"cos");
	this->allowedCommandList.insert(L"date");
	this->allowedCommandList.insert(L"dec");
	this->allowedCommandList.insert(L"dim");
	this->allowedCommandList.insert(L"div");
	this->allowedCommandList.insert(L"effect");
	this->allowedCommandList.insert(L"fileexist");
	this->allowedCommandList.insert(L"getbgmvol");
	this->allowedCommandList.insert(L"getbtntimer");
	this->allowedCommandList.insert(L"getcursorpos");
	this->allowedCommandList.insert(L"getlog");
	this->allowedCommandList.insert(L"getmp3vol");
	this->allowedCommandList.insert(L"gettext");
	this->allowedCommandList.insert(L"gettimer");
	this->allowedCommandList.insert(L"getversion");
	this->allowedCommandList.insert(L"inc");
	this->allowedCommandList.insert(L"intlimit");
	this->allowedCommandList.insert(L"isfull");
	this->allowedCommandList.insert(L"ispage");
	this->allowedCommandList.insert(L"itoa");
	this->allowedCommandList.insert(L"itoa2");
	this->allowedCommandList.insert(L"len");
	this->allowedCommandList.insert(L"mid");
	this->allowedCommandList.insert(L"mod");
	this->allowedCommandList.insert(L"mov");
	this->allowedCommandList.insert(L"mov3");
	this->allowedCommandList.insert(L"mov4");
	this->allowedCommandList.insert(L"mov5");
	this->allowedCommandList.insert(L"mov6");
	this->allowedCommandList.insert(L"mov7");
	this->allowedCommandList.insert(L"mov8");
	this->allowedCommandList.insert(L"mov9");
	this->allowedCommandList.insert(L"mov10");
	this->allowedCommandList.insert(L"movl");
	this->allowedCommandList.insert(L"mul");
	this->allowedCommandList.insert(L"rnd");
	this->allowedCommandList.insert(L"rnd2");
	this->allowedCommandList.insert(L"sin");
	this->allowedCommandList.insert(L"split");
	this->allowedCommandList.insert(L"tan");
	this->allowedCommandList.insert(L"time");
	this->allowedCommandList.insert(L"date2");
	this->allowedCommandList.insert(L"getini");
	ulong total=this->totalCommands(),
		implemented=this->implementedCommands();
	std::cout <<"ONSlaught script interpreter v"<<float(implemented*100/total)/100.0f<<std::endl;
	if (CLOptions.listImplementation)
		this->listImplementation();
}

NONS_ScriptInterpreter::~NONS_ScriptInterpreter(){
	if (this->was_initialized)
		this->uninit();
	delete this->screen;
	delete this->main_font;
	delete this->script;
	while (this->commandQueue.size()){
		delete this->commandQueue.front();
		this->commandQueue.pop();
	}
}

void NONS_ScriptInterpreter::init(){
	this->defaultfs=18;
	this->base_size[0]=this->virtual_size[0]=CLOptions.virtualWidth;
	this->base_size[1]=this->virtual_size[1]=CLOptions.virtualHeight;
	if (!CLOptions.play.size()){
		std::wstring fontfile=getDefaultFontFilename();
		if (!this->main_font)
			this->main_font=init_font(fontfile);
		if (!this->main_font || !this->main_font->good()){
			delete this->main_font;
			if (!this->main_font)
				o_stderr <<"FATAL ERROR: Could not find \""<<fontfile<<"\". If your system is case-sensitive, "
					"make sure the file name is capitalized correctly.\n";
			else
				o_stderr <<"FATAL ERROR: \""<<fontfile<<"\" is not a valid font file.\n";
			exit(0);
		}
		SDL_Color white={255,255,255,255},
			black={0,0,0,255};
		ulong fs=this->defaultfs*this->virtual_size[1]/this->base_size[1];
		this->font_cache=new NONS_FontCache(*this->main_font,fs,white,0,0,0,black FONTCACHE_DEBUG_PARAMETERS);
	}
	if (!CLOptions.play.size()){
		if (!this->script){
			ErrorCode error=NONS_NO_ERROR;
			if (CLOptions.scriptPath.size())
				error=init_script(this->script,CLOptions.scriptPath,CLOptions.scriptencoding,CLOptions.scriptEncryption);
			else
				error=init_script(this->script,CLOptions.scriptencoding);
			if (error!=NONS_NO_ERROR){
				handleErrors(error,-1,"NONS_ScriptInterpreter::NONS_ScriptInterpreter",0);
				exit(error);
			}
		}
		this->thread=new NONS_ScriptThread(this->script);
		this->saveGame=new NONS_SaveFile;
		this->saveGame->format='N';
		memcpy(this->saveGame->hash,this->script->hash,sizeof(unsigned)*5);
	}
	{
		labellog.init(L"NScrllog.dat",L"nonsllog.dat");
		ImageLoader.init();
		o_stdout <<"Global files go in \""<<config_directory<<"\".\n";
		o_stdout <<"Local files go in \""<<save_directory<<"\".\n";
		this->audio=new NONS_Audio(CLOptions.musicDirectory);
		if (CLOptions.musicFormat.size())
			this->audio->musicFormat=CLOptions.musicFormat;
		if (!this->screen)
			this->screen=init_screen(*this->font_cache);
	}
	this->store=new NONS_VariableStore();
	this->interpreter_mode=UNDEFINED_MODE;
	this->nsadir="./";
	this->default_speed=0;
	this->default_speed_slow=0;
	this->default_speed_med=0;
	this->default_speed_fast=0;

	if (settings.exists(L"textSpeedMode"))
		this->current_speed_setting=(char)settings.getInt(L"textSpeedMode");
	else
		this->current_speed_setting=1;

	this->legacy_set_window=1;
	this->arrowCursor=new NONS_Cursor(L":l/3,160,2;cursor0.bmp",0,0,0,this->screen);
	if (!this->arrowCursor->data){
		delete this->arrowCursor;
		this->arrowCursor=new NONS_Cursor(this->screen);
	}
	this->pageCursor=new NONS_Cursor(L":l/3,160,2;cursor1.bmp",0,0,0,this->screen);
	if (!this->pageCursor->data){
		delete this->pageCursor;
		this->pageCursor=new NONS_Cursor(this->screen);
	}
	this->gfx_store=this->screen->gfx_store;
	this->hideTextDuringEffect=1;
	this->selectOn.r=0xFF;
	this->selectOn.g=0xFF;
	this->selectOn.b=0xFF;
	this->selectOff.r=0xA9;
	this->selectOff.g=0xA9;
	this->selectOff.b=0xA9;
	this->autoclick=0;
	this->timer=SDL_GetTicks();
	this->menu=new NONS_Menu(this);
	this->imageButtons=0;
	this->new_if=0;
	this->btnTimer=0;
	this->imageButtonExpiration=0;
	this->printed_lines.clear();
	this->screen->char_baseline=this->screen->screen->inRect.h-1;
	this->useWheel=0;
	this->useEscapeSpace=0;
	this->screenshot=0;
}

void NONS_ScriptInterpreter::uninit(){
	delete this->store;
	for (INIcacheType::iterator i=this->INIcache.begin();i!=this->INIcache.end();i++)
		delete i->second;
	delete this->font_cache;
	this->INIcache.clear();
	delete this->arrowCursor;
	delete this->pageCursor;
	delete this->menu;
	this->selectVoiceClick.clear();
	this->selectVoiceEntry.clear();
	this->selectVoiceMouseOver.clear();
	this->clickStr.clear();
	delete this->imageButtons;
	delete this->saveGame;
	delete this->thread;
	delete this->audio;

	settings.assignInt(L"textSpeedMode",this->current_speed_setting);
	settings.writeOut(config_directory+settings_filename);

	this->textgosub.clear();
	if (!!this->screenshot)
		SDL_FreeSurface(this->screenshot);
}

void NONS_ScriptInterpreter::listImplementation(){
	std::vector<std::wstring> implemented,
		notyetimplemented,
		undocumented,
		unimplemented;
	for (commandMapType::iterator i=this->commandList.begin();i!=this->commandList.end();i++){
		if (!i->second.function)
			notyetimplemented.push_back(i->first);
		else if (i->second.function==&NONS_ScriptInterpreter::command_undocumented)
			undocumented.push_back(i->first);
		else if (i->second.function==&NONS_ScriptInterpreter::command_unimplemented)
			unimplemented.push_back(i->first);
		else if (i->second.function==&NONS_ScriptInterpreter::command___userCommandCall__)
			implemented.push_back(L"<user commands>");
		else
			implemented.push_back(i->first);
	}
	o_stdout.redirect();
	o_stdout <<"Implemented commands:\n";
	o_stdout.indent(1);
	for (ulong a=0;a<implemented.size();a++)
		o_stdout <<implemented[a]<<"\n";
	o_stdout.indent(-1);
	o_stdout <<"Count: "<<(ulong)implemented.size()<<"\n";
	o_stdout <<"\nNot yet implemented commands (commands that will probably be implemented at some point):\n";
	o_stdout.indent(1);
	for (ulong a=0;a<notyetimplemented.size();a++)
		o_stdout <<notyetimplemented[a]<<"\n";
	o_stdout.indent(-1);
	o_stdout <<"Count: "<<(ulong)notyetimplemented.size()<<"\n";
	o_stdout <<"\nUndocumented commands (commands whose purpose is presently unknown):\n";
	o_stdout.indent(1);
	for (ulong a=0;a<undocumented.size();a++)
		o_stdout <<undocumented[a]<<"\n";
	o_stdout.indent(-1);
	o_stdout <<"Count: "<<(ulong)undocumented.size()<<"\n";
	o_stdout <<"\nUnimplemented commands (commands that will probably remain so):\n";
	o_stdout.indent(1);
	for (ulong a=0;a<unimplemented.size();a++)
		o_stdout <<unimplemented[a]<<"\n";
	o_stdout.indent(-1);
	o_stdout <<"Count: "<<(ulong)unimplemented.size()<<"\n";
}

void NONS_ScriptInterpreter::stop(){
	this->stop_interpreting=1;
	forceSkip=1;
}

void NONS_ScriptInterpreter::getCommandListing(std::vector<std::wstring> &vector){
	for (commandMapType::iterator i=this->commandList.begin();i!=this->commandList.end();i++)
		if (i->first.size())
			vector.push_back(i->first);
}

void NONS_ScriptInterpreter::getSymbolListing(std::vector<std::wstring> &vector){
	for (constants_map_T::iterator i=this->store->constants.begin();i!=this->store->constants.end();i++)
		if (std::find(vector.begin(),vector.end(),i->first)==vector.end())
			vector.push_back(i->first);
}

std::wstring NONS_ScriptInterpreter::getValue(const std::wstring &str){
	long l;
	if (this->store->getIntValue(str,l,0)!=NONS_NO_ERROR){
		std::wstring str2;
		if (this->store->getWcsValue(str,str2,0)!=NONS_NO_ERROR){
			handleErrors(NONS_NO_ERROR,0,"",0);
			return L"Interpreter said: \"Could not make sense of argument\"";
		}
		handleErrors(NONS_NO_ERROR,0,"",0);
		return str2;
	}
	return itoaw(l);
}

std::wstring NONS_ScriptInterpreter::interpretFromConsole(const std::wstring &str){
	NONS_ScriptLine *l=new NONS_ScriptLine(0,str,0,1);
	std::wstring ret;
	bool enqueue=0;
	for (ulong a=0;a<l->statements.size();a++){
		l->statements[a]->parse(this->script);
		if (!enqueue){
			if (l->statements[a]->type!=StatementType::COMMAND){
				enqueue=1;
				ret=L"Non-commands are not allowed to be ran from console. "
					L"The entire line will be queued to run after the current command ends.";
			}else if (this->allowedCommandList.find(l->statements[a]->commandName)==this->allowedCommandList.end()){
				enqueue=1;
				ret=L"Command \""+l->statements[a]->commandName+L"\" is not allowed to be ran from console. "
					L"The entire line will be queued to run after the current command ends.";
			}
		}
	}
	if (enqueue){
		this->queue(l);
		return ret;
	}
	for (ulong a=0;a<l->statements.size();a++){
		if (!CHECK_FLAG(this->interpretString(*l->statements[a],l,0),NONS_NO_ERROR_FLAG)){
			if (ret.size())
				ret.push_back('\n');
			ret.append(L"Call [");
			ret.append(l->statements[a]->stmt);
			ret.append(L"] failed.");
		}
	}
	return ret;
}

void NONS_ScriptInterpreter::queue(NONS_ScriptLine *line){
	this->commandQueue.push(line);
}

#include "../video_player.h"

struct playback_input_thread_params{
	bool allow_quit;
	int *stop;
	int *toggle_fullscreen;
	int *take_screenshot;
};

extern bool video_playback;

void playback_input_thread(void *p){
	playback_input_thread_params params=*(playback_input_thread_params *)p;
	NONS_EventQueue queue;
	video_playback=1;
	while (!*params.stop){
		while (!queue.empty() && !*params.stop){
			SDL_Event event=queue.pop();
			switch (event.type){
				case SDL_QUIT:
					*params.stop=1;
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym){
						case SDLK_ESCAPE:
							if (params.allow_quit)
								*params.stop=1;
							break;
						case SDLK_f:
							*params.toggle_fullscreen=1;
							break;
						case SDLK_RETURN:
							if (CHECK_FLAG(event.key.keysym.mod,KMOD_ALT))
								*params.toggle_fullscreen=1;
							break;
						case SDLK_F12:
							*params.take_screenshot=1;
						default:
							break;
					}
				default:
					break;
			}
		}
		SDL_Delay(10);
	}
	video_playback=0;
}

struct video_playback_params{
	NONS_VirtualScreen *vs;
};

SDL_Surface *playback_fullscreen_callback(volatile SDL_Surface *screen,void *user_data){
	video_playback_params params=*(video_playback_params *)user_data;
	return params.vs->toggleFullscreenFromVideo();
}

SDL_Surface *playback_screenshot_callback(volatile SDL_Surface *screen,void *user_data){
	video_playback_params params=*(video_playback_params *)user_data;
	params.vs->takeScreenshotFromVideo();
	return (SDL_Surface *)screen;
}

struct ff_file{
	NONS_DataStream *stream;
	ff_file():stream(0){}
	~ff_file(){
		general_archive.close(this->stream);
	}
	static int open(void *p,const char *a){
		ff_file *_this=(ff_file *)p;
		_this->stream=general_archive.open(UniFromUTF8(std::string(a)));
		return (_this->stream)?0:-1;
	}
	static int close(void *p){
		ff_file *_this=(ff_file *)p;
		general_archive.close(_this->stream);
		_this->stream=0;
		return 0;
	}
	static int read(void *p,uint8_t *a,int b){
		ff_file *_this=(ff_file *)p;
		if (b<0)
			return -1;
		size_t l=(size_t)b;
		if (!_this->stream->read(a,l,l))
			return -1;
		return l;
	}
	static int64_t seek(void *p,int64_t a,int b){
		ff_file *_this=(ff_file *)p;
		if (b<-1 || b>2)
			return -1;
		if (b!=2)
			return _this->stream->seek(a,b);
		return _this->stream->get_size();
	}
};

ErrorCode NONS_ScriptInterpreter::play_video(const std::wstring &filename,bool skippable){
	NONS_LibraryLoader video_player("video_player",0);
#define play_video_TRY_GET_FUNCTION(type,name,string)\
	type name=(type)video_player.getFunction(string);\
	if (!name){\
		switch (video_player.error){\
			case NONS_LibraryLoader::LIBRARY_NOT_FOUND:\
				return NONS_LIBRARY_NOT_FOUND;\
			case NONS_LibraryLoader::FUNCTION_NOT_FOUND:\
				return NONS_FUNCTION_NOT_FOUND;\
		}\
	}
	play_video_TRY_GET_FUNCTION(
		video_playback_fp,
		C_play_video,
		PLAYBACK_FUNCTION_NAME_STRING
	);
	play_video_TRY_GET_FUNCTION(
		player_type_fp,
		get_player_type,
		PLAYER_TYPE_FUNCTION_NAME_STRING
	);
	play_video_TRY_GET_FUNCTION(
		player_version_fp,
		get_player_version,
		PLAYER_VERSION_FUNCTION_NAME_STRING
	);
	o_stdout <<"Loaded video player based on "<<get_player_type()<<" (v"<<get_player_version()<<").\n";
	bool success;
	std::string exception_string(10000,0);
	std::string utf8_filename=UniToUTF8(filename);
	file_protocol fp;
	ff_file file;
	fp.data=&file;
	fp.close=ff_file::close;
	fp.open=ff_file::open;
	fp.read=ff_file::read;
	fp.seek=ff_file::seek;
	if (file.open(&file,utf8_filename.c_str())<0){
		success=0;
		exception_string="File not found.";
	}else{
		NONS_MutexLocker ml(screenMutex);
		SDL_Surface *screen=this->screen->screen->screens[REAL];
		SDL_FillRect(screen,0,0);
		int stop=0,
			toggle_fullscreen=0,
			take_screenshot=0;
		playback_input_thread_params thread_params={
			skippable,
			&stop,
			&toggle_fullscreen,
			&take_screenshot
		};
		video_playback_params playback_params={ this->screen->screen };
		NONS_Thread input_thread(playback_input_thread,&thread_params);
		C_play_video_params::trigger_callback_pair pairs[]={
			{&toggle_fullscreen,playback_fullscreen_callback},
			{&take_screenshot,playback_screenshot_callback}
		};
		std::string utf8_filename_copy=utf8_filename;
		if (get_player_type()==std::string("libVLC")){
			utf8_filename_copy.push_back('/');
			utf8_filename_copy.append(itoac(&fp));
		}
		C_play_video_params parameters={
			C_PLAY_VIDEO_PARAMS_VERSION,
			screen,
			&utf8_filename_copy[0],
			&playback_params,
			sizeof(pairs)/sizeof(*pairs),
			pairs,
			&stop,
			CLOptions.verbosity>=255,
			&exception_string[0],
			exception_string.size(),
			fp
		};
		success=!!C_play_video(&parameters);
		exception_string.resize(strlen(exception_string.c_str()));
		stop=1;
		file.close(&file);
	}
	if (!success){
		if (exception_string.size())
			o_stderr <<"NONS_ScriptInterpreter::play_video( \""<<utf8_filename<<"\" ): "<<exception_string<<"\n";
		else
			o_stderr <<"NONS_ScriptInterpreter::play_video( \""<<utf8_filename<<"\" ): Unknown error.\n";
		return NONS_UNDEFINED_ERROR;
	}
	return NONS_NO_ERROR;
}

#define generic_play_loop(condition) {\
	NONS_EventQueue queue;\
	bool _break=0;\
	while ((condition) && !_break){\
		while (!queue.empty() && !_break){\
			SDL_Event event=queue.pop();\
			if (event.type==SDL_KEYDOWN ||\
					event.type==SDL_MOUSEBUTTONDOWN ||\
					event.type==SDL_QUIT)\
				_break=1;\
		}\
		SDL_Delay(50);\
	}\
}

const wchar_t *image_formats[]={
	L"bmp",
	L"gif",
	L"iff",
	L"jpeg",
	L"jpg",
	L"lbm",
	L"pbm",
	L"pcx",
	L"png",
	L"ppm",
	L"tga",
	L"tif",
	L"tiff",
	L"xcf",
	L"xpm",
	L"svg",
	0
};

const wchar_t *sound_formats[]={
	L"ogg",
	L"mp3",
	L"mid",
	L"it",
	L"xm",
	L"s3m",
	L"mod",
	L"aiff",
	L"flac",
	L"669",
	L"med",
	L"voc",
	L"wav",
	0
};

//0: unknown
//1: image
//2: sound
//3: video
ulong get_file_type(const std::wstring &filename){
	ulong dot=filename.rfind('.');
	if (dot==filename.npos)
		return 0;
	std::wstring extension=filename.substr(dot+1);
	tolower(extension);
	for (ulong a=0;image_formats[a];a++)
		if (extension==image_formats[a])
			return 1;
	for (ulong a=0;sound_formats[a];a++)
		if (extension==sound_formats[a])
			return 2;
	return 3;
}

bool NONS_ScriptInterpreter::generic_play(const std::wstring &filename,bool from_archive){
	NONS_ScreenSpace *scr=this->screen;
	ulong type=get_file_type(filename);
	switch (type){
		case 0:
			return 0;
		case 1:
			if (scr->Background->load(&filename)){
				NONS_MutexLocker ml(screenMutex);
				scr->Background->position.x=(scr->screen->screens[VIRTUAL]->w-scr->Background->clip_rect.w)/2;
				scr->Background->position.y=(scr->screen->screens[VIRTUAL]->h-scr->Background->clip_rect.h)/2;
				scr->BlendOnlyBG(1);
				generic_play_loop(1);
			}
			return 1;
		case 2:
			if (!CHECK_FLAG(this->audio->playMusic(&filename,1),NONS_NO_ERROR_FLAG))
				return 0;
			generic_play_loop(this->audio->music->is_playing());
			return 1;
		case 3:
			return !CHECK_FLAG(handleErrors(
					this->play_video(filename,1),
					ULONG_MAX,
					"NONS_ScriptInterpreter::generic_play",
					0
				),NONS_NO_ERROR_FLAG);
	}
	return 0;
}

ulong NONS_ScriptInterpreter::totalCommands(){
	return this->commandList.size()-1;
}

ulong NONS_ScriptInterpreter::implementedCommands(){
	ulong res=0;
	for (commandMapType::iterator i=this->commandList.begin();i!=this->commandList.end();i++)
		if (i->first!=L"" && i->second.function)
			res++;
	return res;
}

void NONS_ScriptInterpreter::handleKeys(SDL_Event &event){
	if (event.type==SDL_KEYDOWN){
		float def=(float)this->default_speed,
			cur=(float)this->screen->output->display_speed;
		if (event.key.keysym.sym==SDLK_F5){
			this->default_speed=this->default_speed_slow;
			this->current_speed_setting=0;
			this->screen->output->display_speed=ulong(cur/def*float(this->default_speed));
		}else if (event.key.keysym.sym==SDLK_F6){
			this->default_speed=this->default_speed_med;
			this->current_speed_setting=1;
			this->screen->output->display_speed=ulong(cur/def*float(this->default_speed));
		}else if (event.key.keysym.sym==SDLK_F7){
			this->default_speed=this->default_speed_fast;
			this->current_speed_setting=2;
			this->screen->output->display_speed=ulong(cur/def*float(this->default_speed));
		}
	}
}

void print_command(NONS_RedirectedOutput &ro,ulong current_line,const std::wstring &commandName,const std::vector<std::wstring> &parameters,ulong mode){
	if (mode<2){
		ro <<"{\n";
		ro.indent(1);
		if (!current_line)
			ro <<"Line "<<current_line<<":\n";
		ro <<"["<<commandName<<"] ";
		if (parameters.size()){
			ro <<"(\n";
			ro.indent(1);
			for (ulong a=0;;a++){
				ro <<"["<<parameters[a]<<"]";
				if (a==parameters.size()-1){
					ro <<"\n";
					break;
				}
				ro <<",\n";
			}
			ro.indent(-1);
			ro <<")\n";
		}else
			ro <<"{NO PARAMETERS}\n";
	}
	if (mode!=1){
		o_stderr.indent(-1);
		o_stderr <<"}\n";
	}
}

extern uchar trapFlag;

bool NONS_ScriptInterpreter::interpretNextLine(){
	if (trapFlag){
		if (!CURRENTLYSKIPPING || (CURRENTLYSKIPPING && !(trapFlag>2))){
			bool end=0;
			while (!this->inputQueue.empty() && !end){
				SDL_Event event=this->inputQueue.pop();
				if (event.type==SDL_MOUSEBUTTONDOWN && (event.button.which==SDL_BUTTON_LEFT || !(trapFlag%2)))
					end=1;
				else
					this->handleKeys(event);
			}
			if (end){
				this->thread->gotoLabel(this->trapLabel);
				this->thread->advanceToNextStatement();
				this->trapLabel.clear();
				trapFlag=0;
			}
		}
	}else{
		while (!this->inputQueue.empty()){
			SDL_Event event=this->inputQueue.pop();
			this->handleKeys(event);
		}
	}

	NONS_Statement *stmt=this->thread->getCurrentStatement();
	if (!stmt)
		return 0;
	stmt->parse(this->script);
	ulong current_line=stmt->lineOfOrigin->lineNumber;
	if (CLOptions.verbosity>=1 && CLOptions.verbosity<255)
		o_stderr <<"Interpreting line "<<current_line<<"\n";
	if (CLOptions.verbosity>=3 && CLOptions.verbosity<255 && stmt->type==StatementType::COMMAND)
		print_command(o_stderr,0,stmt->commandName,stmt->parameters,0);
	this->saveGame->textX=this->screen->output->x;
	this->saveGame->textY=this->screen->output->y;
	this->saveGame->italic=this->screen->output->get_italic();
	this->saveGame->bold=this->screen->output->get_bold();
#if defined _DEBUG && defined STOP_AT_LINE && STOP_AT_LINE>0
	//Reserved for debugging:
	bool break_at_this_line=0;
	if (stmt->lineOfOrigin->lineNumber==STOP_AT_LINE)
		break_at_this_line=1;
#endif
	switch (stmt->type){
		case StatementType::BLOCK:
			this->saveGame->currentLabel=stmt->commandName;
			labellog.addString(stmt->commandName);
			if (!stdStrCmpCI(stmt->commandName,L"define"))
				this->interpreter_mode=DEFINE_MODE;
			else if (!stdStrCmpCI(stmt->commandName,L"start"))
				this->interpreter_mode=RUN_MODE;
			break;
		case StatementType::JUMP:
		case StatementType::COMMENT:
			break;
		case StatementType::PRINTER:
			if (this->interpreter_mode!=RUN_MODE){
				handleErrors(NONS_NOT_ALLOWED_IN_DEFINE_MODE,current_line,"NONS_ScriptInterpreter::interpretNextLine",0);
			}else{
				if (this->printed_lines.find(current_line)==this->printed_lines.end()){
					//softwareCtrlIsPressed=0;
					this->printed_lines.insert(current_line);
				}
				this->Printer(stmt->stmt);
			}
			break;
		case StatementType::COMMAND:
			{
				commandMapType::iterator i=this->commandList.find(stmt->commandName);
				//bool is_user_command=0;
				if (i==this->commandList.end()){
					if (this->userCommandList.find(stmt->commandName)!=this->userCommandList.end())
						i=this->commandList.find(L"");
				}
				if (i!=this->commandList.end()){
					if (!i->second.allow_define && this->interpreter_mode==DEFINE_MODE){
						handleErrors(NONS_NOT_ALLOWED_IN_DEFINE_MODE,current_line,"NONS_ScriptInterpreter::interpretNextLine",0);
						break;
					}else if (!i->second.allow_run && this->interpreter_mode==RUN_MODE){
						handleErrors(NONS_NOT_ALLOWED_IN_RUN_MODE,current_line,"NONS_ScriptInterpreter::interpretNextLine",0);
						break;
					}
					commandFunctionPointer function=i->second.function;
					if (!function){
						if (this->implementationErrors.find(i->first)==this->implementationErrors.end()){
							o_stderr <<"NONS_ScriptInterpreter::interpretNextLine(): ";
							if (current_line==ULONG_MAX)
								o_stderr <<"Error";
							else
								o_stderr <<"Error near line "<<current_line;
							o_stderr <<". Command \""<<stmt->commandName<<"\" is not implemented yet.\n"
								"    Implementation errors are reported only once.\n";
							this->implementationErrors.insert(i->first);
						}
						if (CLOptions.stopOnFirstError)
							return 0;
						break;
					}
					ErrorCode error;

					//After the next if, 'stmt' is no longer guaranteed to remain valid, so we
					//should copy now all the data we may need.
					std::wstring commandName=stmt->commandName;
					std::vector<std::wstring> parameters=stmt->parameters;

					if (CHECK_FLAG(stmt->error,NONS_NO_ERROR_FLAG)){
#if defined _DEBUG && defined STOP_AT_LINE && STOP_AT_LINE>0
						if (break_at_this_line)
							std::cout <<"TRIGGERED!"<<std::endl;
#endif
						error=(this->*function)(*stmt);
					}else
						error=stmt->error;
					bool there_was_an_error=!CHECK_FLAG(error,NONS_NO_ERROR_FLAG);
					if (there_was_an_error)
						print_command(o_stderr,current_line,commandName,parameters,1);
					handleErrors(error,current_line,"NONS_ScriptInterpreter::interpretNextLine",0);
					if (there_was_an_error)
						print_command(o_stderr,current_line,commandName,parameters,2);
					if (CLOptions.stopOnFirstError && error!=NONS_UNIMPLEMENTED_COMMAND || error==NONS_END)
						return 0;
				}else{
					o_stderr <<"NONS_ScriptInterpreter::interpretNextLine(): ";
					if (current_line==ULONG_MAX)
						o_stderr <<"Error";
					else
						o_stderr <<"Error near line "<<current_line;
					o_stderr <<". Command \""<<stmt->commandName<<"\" could not be recognized.\n";
					if (CLOptions.stopOnFirstError)
						return 0;
				}
			}
			break;
		default:;
	}
	while (this->commandQueue.size()){
		NONS_ScriptLine *line=this->commandQueue.front();
		for (ulong a=0;a<line->statements.size();a++)
			if (this->interpretString(*line->statements[a],line,0)==NONS_END)
				return 0;
		this->commandQueue.pop();
		delete line;
	}
	if (!this->thread->advanceToNextStatement() || this->stop_interpreting){
		this->command_end(*stmt);
		return 0;
	}
	return 1;
}

ErrorCode NONS_ScriptInterpreter::interpretString(const std::wstring &str,NONS_ScriptLine *line,ulong offset){
	NONS_ScriptLine l(0,str,0,1);
	ErrorCode ret=NONS_NO_ERROR;
	for (ulong a=0;a<l.statements.size();a++){
		ErrorCode error=this->interpretString(*l.statements[a],line,offset);
		if (!CHECK_FLAG(error,NONS_NO_ERROR_FLAG))
			ret=NONS_UNDEFINED_ERROR;
	}
	return ret;
}

ErrorCode NONS_ScriptInterpreter::interpretString(NONS_Statement &stmt,NONS_ScriptLine *line,ulong offset){
	stmt.parse(this->script);
	stmt.lineOfOrigin=line;
	stmt.fileOffset=offset;
	if (CLOptions.verbosity>=3 && CLOptions.verbosity<255 && stmt.type==StatementType::COMMAND){
		o_stderr <<"String: ";
		print_command(o_stderr,0,stmt.commandName,stmt.parameters,0);
	}
	switch (stmt.type){
		case StatementType::COMMENT:
			break;
		case StatementType::PRINTER:
			if (this->interpreter_mode!=RUN_MODE){
				handleErrors(NONS_NOT_ALLOWED_IN_DEFINE_MODE,0,"NONS_ScriptInterpreter::interpretString",0);
			}else{
				if (!stmt.lineOfOrigin || this->printed_lines.find(stmt.lineOfOrigin->lineNumber)==this->printed_lines.end()){
					//softwareCtrlIsPressed=0;
					if (!!stmt.lineOfOrigin)
						this->printed_lines.insert(stmt.lineOfOrigin->lineNumber);
				}
				this->Printer(stmt.stmt);
			}
			break;
		case StatementType::COMMAND:
			{
				ulong current_line=(!!stmt.lineOfOrigin)?stmt.lineOfOrigin->lineNumber:0;
				commandMapType::iterator i=this->commandList.find(stmt.commandName);
				if (i!=this->commandList.end()){
					if (!i->second.allow_define && this->interpreter_mode==DEFINE_MODE)
						return handleErrors(NONS_NOT_ALLOWED_IN_DEFINE_MODE,current_line,"NONS_ScriptInterpreter::interpretNextLine",1);
					if (!i->second.allow_run && this->interpreter_mode==RUN_MODE)
						return handleErrors(NONS_NOT_ALLOWED_IN_RUN_MODE,current_line,"NONS_ScriptInterpreter::interpretNextLine",1);
					commandFunctionPointer function=i->second.function;
					if (!function){
						if (this->implementationErrors.find(i->first)!=this->implementationErrors.end()){
							o_stderr <<"NONS_ScriptInterpreter::interpretNextLine(): "
								"Error. Command \""<<stmt.commandName<<"\" is not implemented.\n"
								"    Implementation errors are reported only once.\n";
							this->implementationErrors.insert(i->first);
						}
						return NONS_NOT_IMPLEMENTED;
					}
					return handleErrors((this->*function)(stmt),current_line,"NONS_ScriptInterpreter::interpretString",1);
				}else{
					o_stderr <<"NONS_ScriptInterpreter::interpretString(): "
						"Error. Command \""<<stmt.commandName<<"\" could not be recognized.\n";
					return NONS_UNRECOGNIZED_COMMAND;
				}
			}
			break;
		default:;
	}
	return NONS_NO_ERROR;
}

std::wstring insertIntoString(const std::wstring &dst,ulong from,ulong l,const std::wstring &src){
	return dst.substr(0,from)+src+dst.substr(from+l);
}

std::wstring insertIntoString(const std::wstring &dst,ulong from,ulong l,long src){
	std::wstringstream temp;
	temp <<src;
	return insertIntoString(dst,from,l,temp.str());
}

std::wstring getInlineExpression(const std::wstring &string,ulong off,ulong *len){
	ulong l=off;
	while (multicomparison(string[l],"_+-*/|&%$?<>=()[]\"`") || NONS_isalnum(string[l])){
		if (string[l]=='\"' || string[l]=='`'){
			wchar_t quote=string[l];
			ulong l2=l+1;
			for (;l2<string.size() && string[l2]!=quote;l2++);
			if (string[l2]!=quote)
				break;
			else
				l=l2;
		}
		l++;
	}
	if (!!len)
		*len=l-off;
	return std::wstring(string,off,l-off);
}

void NONS_ScriptInterpreter::reduceString(
		const std::wstring &src,
		std::wstring &dst,
		std::set<NONS_VariableMember *> *visited,
		std::vector<std::pair<std::wstring,NONS_VariableMember *> > *stack){
	for (ulong off=0;off<src.size();){
		switch (src[off]){
			case '!':
				if (src.find(L"!nl",off)==off){
					dst.push_back('\n');
					off+=3;
					break;
				}
			case '%':
			case '$':
			case '?':
				{
					ulong l;
					std::wstring expr=getInlineExpression(src,off,&l);
					if (expr.size()){
						NONS_VariableMember *var=this->store->retrieve(expr,0);
						if (!!var){
							off+=l;
							if (var->getType()==INTEGER){
								std::wstringstream stream;
								stream <<var->getInt();
								dst.append(stream.str());
							}else if (var->getType()==STRING){
								std::wstring copy=var->getWcs();
								if (!!visited && visited->find(var)!=visited->end()){
									o_stderr <<"NONS_ScriptInterpreter::reduceString(): WARNING: Infinite recursion avoided.\n"
										"    Reduction stack contents:\n";
									for (std::vector<std::pair<std::wstring,NONS_VariableMember *> >::iterator i=stack->begin();i!=stack->end();i++)
										o_stderr <<"        ["<<i->first<<"] = \""<<i->second->getWcs()<<"\"\n";
									o_stderr <<" (last) ["<<expr<<"] = \""<<copy<<"\"\n";
									dst.append(copy);
								}else{
									std::set<NONS_VariableMember *> *temp_visited;
									std::vector<std::pair<std::wstring,NONS_VariableMember *> > *temp_stack;
									if (!visited)
										temp_visited=new std::set<NONS_VariableMember *>;
									else
										temp_visited=visited;
									temp_visited->insert(var);
									if (!stack)
										temp_stack=new std::vector<std::pair<std::wstring,NONS_VariableMember *> >;
									else
										temp_stack=stack;
									temp_stack->push_back(std::pair<std::wstring,NONS_VariableMember *>(expr,var));
									reduceString(copy,dst,temp_visited,temp_stack);
									if (!visited)
										delete temp_visited;
									else
										temp_visited->erase(var);
									if (!stack)
										delete temp_stack;
									else
										temp_stack->pop_back();
								}
							}
							break;
						}
					}
				}
			default:
				dst.push_back(src[off]);
				off++;
		}
	}
}

void findStops(const std::wstring &src,std::vector<std::pair<ulong,ulong> > &stopping_points,std::wstring &dst){
	dst.clear();
	for (ulong a=0,size=src.size();a<size;a++){
		switch (src[a]){
			case '\\':
			case '@':
				{
					std::pair<ulong,ulong> push(dst.size(),a);
					stopping_points.push_back(push);
					continue;
				}
			case '!':
				if (firstcharsCI(src,a,L"!sd")){
					stopping_points.push_back(std::make_pair(dst.size(),a));
					a+=2;
					continue;
				}else if (firstcharsCI(src,a,L"!s") || firstcharsCI(src,a,L"!d") || firstcharsCI(src,a,L"!w")){
					stopping_points.push_back(std::make_pair(dst.size(),a));
					ulong l=2;
					for (;isdigit(src[a+l]);l++);
					a+=l-1;
					continue;
				}else if (firstcharsCI(src,a,L"!i") || firstcharsCI(src,a,L"!b")){
					stopping_points.push_back(std::make_pair(dst.size(),a));
					a++;
					continue;
				}
			case '#':
				if (src[a]=='#'){
					if (src.size()-a-1>=6){
						a++;
						short b;
						for (b=0;b<6 && NONS_ishexa(src[a+b]);b++);
						if (b!=6)
							a--;
						else{
							std::pair<ulong,ulong> push(dst.size(),a-1);
							stopping_points.push_back(push);
							a+=5;
							continue;
						}
					}
				}
			default:
				dst.push_back(src[a]);
		}
		if (src[a]=='\\')
			break;
	}
	std::pair<ulong,ulong> push(dst.size(),src.size());
	stopping_points.push_back(push);
}

bool NONS_ScriptInterpreter::Printer_support(std::vector<printingPage> &pages,ulong *totalprintedchars,bool *justTurnedPage,ErrorCode *error){
	NONS_StandardOutput *out=this->screen->output;
	this->screen->showText();
	std::wstring *str;
	bool justClicked;
	for (std::vector<printingPage>::iterator i=pages.begin();i!=pages.end();i++){
		bool clearscr=out->prepareForPrinting(i->print);
		if (clearscr){
			if (this->pageCursor->animate(this->menu,this->autoclick)<0){
				if (!!error)
					*error=NONS_NO_ERROR;
				return 1;
			}
			this->screen->clearText();
		}
		str=&i->reduced;
		for (ulong reduced=0,printed=0,stop=0;stop<i->stops.size();stop++){
			ulong printedChars=0;
			while (justClicked=out->print(printed,i->stops[stop].first,this->screen->screen,&printedChars)){
				if (this->pageCursor->animate(this->menu,this->autoclick)<0){
					if (!!error)
						*error=NONS_NO_ERROR;
					return 1;
				}
				this->screen->clearText();
				justClicked=1;
			}
			if (printedChars>0){
				if (!!totalprintedchars)
					(*totalprintedchars)+=printedChars;
				if (!!justTurnedPage)
					*justTurnedPage=0;
				justClicked=0;
			}
			reduced=i->stops[stop].second;
			switch ((*str)[reduced]){
				case '\\':
					if (this->textgosub.size() && (this->textgosubRecurses || !this->insideTextgosub())){
						std::vector<printingPage>::iterator i2=i;
						std::vector<printingPage> temp;
						printingPage temp2(*i2);
						temp2.print.erase(temp2.print.begin(),temp2.print.begin()+temp2.stops[stop].first);
						temp2.reduced.erase(temp2.reduced.begin(),temp2.reduced.begin()+temp2.stops[stop].second+1);
						std::pair<ulong,ulong> takeOut;
						takeOut.first=temp2.stops[stop].first+1;
						takeOut.second=temp2.stops[stop].second+1;
						temp2.stops.erase(temp2.stops.begin(),temp2.stops.begin()+stop+1);
						for (std::vector<std::pair<ulong,ulong> >::iterator i2=temp2.stops.begin();i2<temp2.stops.end();++i2){
							i2->first-=takeOut.first;
							i2->second-=takeOut.second;
						}
						temp.push_back(temp2);
						for (i2++;i2!=pages.end();i2++)
							temp.push_back(*i2);
						NONS_StackElement *pusher=new NONS_StackElement(temp,'\\',this->insideTextgosub()+1);
						this->callStack.push_back(pusher);
						this->goto_label(this->textgosub);
						out->endPrinting();
						if (!!error)
							*error=NONS_NO_ERROR;
						return 1;
					}else if (!justClicked && this->pageCursor->animate(this->menu,this->autoclick)<0){
						if (!!error)
							*error=NONS_NO_ERROR;
						return 1;
					}
					out->endPrinting();
					this->screen->clearText();
					reduced++;
					if (!!justTurnedPage)
						*justTurnedPage=1;
					break;
				case '@':
					if (this->textgosub.size() && (this->textgosubRecurses || !this->insideTextgosub())){
						std::vector<printingPage>::iterator i2=i;
						std::vector<printingPage> temp;
						printingPage temp2(*i2);
						temp2.print.erase(temp2.print.begin(),temp2.print.begin()+temp2.stops[stop].first);
						temp2.reduced.erase(temp2.reduced.begin(),temp2.reduced.begin()+temp2.stops[stop].second+1);
						std::pair<ulong,ulong> takeOut;
						takeOut.first=temp2.stops[stop].first;
						takeOut.second=temp2.stops[stop].second+1;
						temp2.stops.erase(temp2.stops.begin(),temp2.stops.begin()+stop+1);
						for (std::vector<std::pair<ulong,ulong> >::iterator i2=temp2.stops.begin();i2<temp2.stops.end();++i2){
							i2->first-=takeOut.first;
							i2->second-=takeOut.second;
						}
						temp.push_back(temp2);
						for (i2++;i2!=pages.end();i2++)
							temp.push_back(*i2);
						NONS_StackElement *pusher=new NONS_StackElement(temp,'@',this->insideTextgosub()+1);
						this->callStack.push_back(pusher);
						this->goto_label(this->textgosub);
						out->endPrinting();
						if (!!error)
							*error=NONS_NO_ERROR;
						return 1;
					}else if (!justClicked && this->arrowCursor->animate(this->menu,this->autoclick)<0){
						if (!!error)
							*error=NONS_NO_ERROR;
						return 1;
					}
					reduced++;
					break;
				case '!':
					if (firstcharsCI(*str,reduced,L"!sd")){
						out->display_speed=this->default_speed;
						reduced+=3;
						break;
					}else{
						bool notess=firstcharsCI(*str,reduced,L"!s"),
							notdee=firstcharsCI(*str,reduced,L"!d"),
							notdu=firstcharsCI(*str,reduced,L"!w");
						if (notess || notdee || notdu){
							reduced+=2;
							ulong l=0;
							for (;NONS_isdigit((*str)[reduced+l]);l++);
							if (l>0){
								long s=0;
								{
									std::wstringstream stream(str->substr(reduced,l));
									stream >>s;
								}
								if (notess){
									switch (this->current_speed_setting){
										case 0:
											this->screen->output->display_speed=s*2;
											break;
										case 1:
											this->screen->output->display_speed=s;
											break;
										case 2:
											this->screen->output->display_speed=s/2;
									}
								}else if (notdee)
									waitCancellable(s);
								else
									waitNonCancellable(s);
								reduced+=l;
								break;
							}else
								reduced-=2;
						}else{
							bool noti=firstcharsCI(*str,reduced,L"!i"),
								notbee=firstcharsCI(*str,reduced,L"!b");
							if (noti || notbee){
								reduced+=2;
								NONS_FontCache *caches[2];
								caches[0]=out->foregroundLayer->fontCache;
								if (out->shadowLayer)
									caches[1]=out->shadowLayer->fontCache;
								if (noti)
									out->set_italic(!out->get_italic());
								else
									out->set_bold(!out->get_bold());
								break;
							}
						}
					}
				case '#':
					if ((*str)[reduced]=='#'){
						ulong len=str->size()-reduced-1;
						if (len>=6){
							reduced++;
							Uint32 parsed=0;
							short a;
							for (a=0;a<6;a++){
								int hex=(*str)[reduced+a];
								if (!NONS_ishexa(hex))
									break;
								parsed<<=4;
								parsed|=HEX2DEC(hex);
							}
							if (a==6){
								SDL_Color color=this->screen->output->foregroundLayer->fontCache->get_color();
								color.r=parsed>>16;
								color.g=(parsed&0xFF00)>>8;
								color.b=(parsed&0xFF);
								this->screen->output->foregroundLayer->fontCache->setColor(color);
								reduced+=6;
								break;
							}
							reduced--;
						}
					}
			}
			printed=i->stops[stop].first;
			justClicked=0;
		}
		out->endPrinting();
	}
	return 0;
}

ErrorCode NONS_ScriptInterpreter::Printer(const std::wstring &line){
	this->currentBuffer=this->screen->output->currentBuffer;
	NONS_StandardOutput *out=this->screen->output;
	if (!line.size()){
		if (out->NewLine()){
			if (this->pageCursor->animate(this->menu,this->autoclick)<0)
				return NONS_NO_ERROR;
			out->NewLine();
		}
		return NONS_NO_ERROR;
	}
	bool skip=line[0]=='`';
	std::wstring str=line.substr(skip);
	bool justTurnedPage=0;
	std::wstring reducedString;
	reduceString(str,reducedString);
	std::vector<printingPage> pages;
	ulong totalprintedchars=0;
	for (ulong a=0;a<reducedString.size();){
		ulong p=reducedString.find('\\',a);
		if (p==reducedString.npos)
			p=reducedString.size();
		else
			p++;
		std::wstring str3(reducedString.begin()+a,reducedString.begin()+p);
		a=p;
		pages.push_back(printingPage());
		printingPage &page=pages.back();
		page.reduced=str3;
		findStops(page.reduced,page.stops,page.print);
	}
	ErrorCode error;
	if (this->Printer_support(pages,&totalprintedchars,&justTurnedPage,&error))
		return error;
	if (!justTurnedPage && totalprintedchars && !this->insideTextgosub() && out->NewLine() &&
			this->pageCursor->animate(this->menu,this->autoclick)>=0){
		this->screen->clearText();
		//out->NewLine();
	}
	return NONS_NO_ERROR;
}

std::wstring NONS_ScriptInterpreter::convertParametersToString(NONS_Statement &stmt){
	std::wstring string;
	for (ulong a=0;a<stmt.parameters.size();a++){
		std::wstring &str=stmt.parameters[a];
		NONS_Expression::Value *val=this->store->evaluate(str,0);
		if (val->is_err()){
			delete val;
			continue;
		}
		string.append((val->is_int())?itoaw(val->integer):val->string);
	}
	return string;
}

ulong NONS_ScriptInterpreter::insideTextgosub(){
	return (this->callStack.size() && this->callStack.back()->textgosubLevel)?this->callStack.back()->textgosubLevel:0;
}

bool NONS_ScriptInterpreter::goto_label(const std::wstring &label){
	if (!this->thread->gotoLabel(label))
		return 0;
	labellog.addString(label);
	return 1;
}

bool NONS_ScriptInterpreter::gosub_label(const std::wstring &label){
	NONS_StackElement *el=new NONS_StackElement(this->thread->getNextStatementPair(),NONS_ScriptLine(),0,this->insideTextgosub());
	if (!this->goto_label(label)){
		delete el;
		return 0;
	}
	this->callStack.push_back(el);
	return 1;
}

ErrorCode NONS_ScriptInterpreter::load(int file){
	NONS_SaveFile save;
	save.load(save_directory+L"save"+itoaw(file)+L".dat");
	if (save.error!=NONS_NO_ERROR)
		return NONS_NO_SUCH_SAVEGAME;
	//**********************************************************************
	//NONS save file
	//**********************************************************************
	if (save.version>NONS_SAVEFILE_VERSION)
		return NONS_UNSUPPORTED_SAVEGAME_VERSION;
	for (ulong a=0;a<5;a++)
		if (save.hash[a]!=this->script->hash[a])
			return NONS_HASH_DOES_NOT_MATCH;
	//stack
	//flush
	while (!this->callStack.empty()){
		NONS_StackElement *p=this->callStack.back();
		delete p;
		this->callStack.pop_back();
	}
	for (ulong a=0;a<save.stack.size();a++){
		NONS_SaveFile::stackEl *el=save.stack[a];
		NONS_StackElement *push;
		std::pair<ulong,ulong> pair(this->script->blockFromLabel(el->label)->first_line+el->linesBelow,el->statementNo);
		switch (el->type){
			case StackFrameType::SUBROUTINE_CALL:
				push=new NONS_StackElement(
					pair,
					NONS_ScriptLine(0,el->leftovers,0,1),
					0,
					el->textgosubLevel);
				break;
			case StackFrameType::FOR_NEST:
				push=new NONS_StackElement(
					this->store->retrieve(el->variable,0)->intValue,
					pair,
					0,
					el->to,
					el->step,
					el->textgosubLevel);
				break;
			//To be implemented in the future:
			/*case TEXTGOSUB_CALL:
				push=new NONS_StackElement(el->pages,el->trigger,el->textgosubLevel);*/
			case StackFrameType::USERCMD_CALL:
				push=new NONS_StackElement(
					pair,
					NONS_ScriptLine(0,el->leftovers,0,1),
					0,
					el->textgosubLevel);
				{
					NONS_StackElement *temp=new NONS_StackElement(push,el->parameters);
					delete push;
					push=temp;
				}
		}
		this->callStack.push_back(push);
	}
	std::pair<ulong,ulong> pair(this->script->blockFromLabel(save.currentLabel)->first_line+save.linesBelow,save.statementNo);
	this->thread->gotoPair(pair);
	this->saveGame->currentLabel=save.currentLabel;
	this->loadgosub=save.loadgosub;
	//variables
	variables_map_T::iterator first=this->store->variables.begin(),last=first;
	if (first->first<200){
		for (;last!=this->store->variables.end() && last->first<200;last++)
			delete last->second;
		this->store->variables.erase(first,last);
	}
	for (variables_map_T::iterator i=save.variables.begin();i!=save.variables.end();i++){
		NONS_Variable *var=i->second;
		NONS_Variable *dst=this->store->retrieve(i->first,0);
		(*dst)=(*var);
	}
	for (arrays_map_T::iterator i=this->store->arrays.begin();i!=this->store->arrays.end();i++)
		delete i->second;
	this->store->arrays.clear();
	for (arrays_map_T::iterator i=save.arrays.begin();i!=save.arrays.end();i++)
		this->store->arrays[i->first]=new NONS_VariableMember(*(i->second));
	//screen
	//window
	NONS_ScreenSpace *scr=this->screen;
	this->font_cache->set_size(save.fontSize);
	this->font_cache->spacing=save.spacing;
	this->font_cache->line_skip=save.lineSkip;
	scr->resetParameters(&save.textWindow,&save.windowFrame,*this->font_cache,save.fontShadow);
	NONS_StandardOutput *out=scr->output;
	/*out->shadeLayer->clip_rect=save.textWindow;
	out->x0=save.windowFrame.x;
	out->y0=save.windowFrame.y;
	out->w=save.windowFrame.w;
	out->h=save.windowFrame.h;*/
	out->shadeLayer->setShade(save.windowColor.r,save.windowColor.g,save.windowColor.b);
	out->shadeLayer->Clear();
	out->transition->effect=save.windowTransition;
	out->transition->duration=save.windowTransitionDuration;
	out->transition->rule=save.windowTransitionRule;
	this->hideTextDuringEffect=save.hideWindow;
	out->foregroundLayer->fontCache->setColor(save.windowTextColor);
	out->set_italic(save.italic);
	out->set_bold(save.bold);
	out->display_speed=save.textSpeed;
	if (save.version>2){
		out->shadowPosX=save.shadowPosX;
		out->shadowPosY=save.shadowPosY;
	}else{
		out->shadowPosX=1;
		out->shadowPosY=1;
	}
	out->log.clear();
	for (ulong a=0;a<save.logPages.size();a++)
		out->log.push_back(save.logPages[a]);
	out->currentBuffer=save.currentBuffer;
	out->indentationLevel=save.indentationLevel;
	out->x=save.textX;
	out->y=save.textY;
	if (this->arrowCursor)
		delete this->arrowCursor;
	if (this->pageCursor)
		delete this->pageCursor;
	if (!save.arrow.string.size())
		//this->arrowCursor=new NONS_Cursor(this->screen);
		this->arrowCursor=new NONS_Cursor(L":l/3,160,2;cursor0.bmp",0,0,0,this->screen);
	else
		this->arrowCursor=new NONS_Cursor(
			save.arrow.string,
			save.arrow.x,
			save.arrow.y,
			save.arrow.absolute,
			this->screen);
	if (!save.page.string.size())
		//this->pageCursor=new NONS_Cursor(this->screen);
		this->pageCursor=new NONS_Cursor(L":l/3,160,2;cursor1.bmp",0,0,0,this->screen);
	else
		this->pageCursor=new NONS_Cursor(
			save.page.string,
			save.page.x,
			save.page.y,
			save.page.absolute,
			this->screen);
	//graphics
	if (save.background.size()){
		if (!scr->Background)
			scr->Background=new NONS_Layer(&save.background);
		else
			scr->Background->load(&save.background);
	}else{
		if (!scr->Background){
			unsigned rgb=(save.bgColor.r<<rshift)|(save.bgColor.g<<gshift)|(save.bgColor.b<<bshift);
			scr->Background=new NONS_Layer(&scr->screen->inRect,rgb);
		}else{
			scr->Background->setShade(save.bgColor.r,save.bgColor.g,save.bgColor.b);
			scr->Background->Clear();
		}
	}
	if (save.version>1)
		scr->char_baseline=save.char_baseline;
	else
		scr->char_baseline=scr->screenBuffer->clip_rect.h-1;
	NONS_Layer **characters[]={&scr->leftChar,&scr->centerChar,&scr->rightChar};
	for (int a=0;a<3;a++){
		(*characters[a])->unload();
		if (!save.characters[a].string.size())
			continue;
		if (!*characters[a])
			*characters[a]=new NONS_Layer(&save.characters[a].string);
		else
			(*characters[a])->load(&save.characters[a].string);
		(*characters[a])->position.x=(Sint16)save.characters[a].x;
		(*characters[a])->position.y=(Sint16)save.characters[a].y;
		(*characters[a])->visible=save.characters[a].visibility;
		(*characters[a])->alpha=save.characters[a].alpha;
		if ((*characters[a])->animated())
			(*characters[a])->animation.animation_time_offset=save.characters[a].animOffset;
	}
	if (save.version>2){
		scr->charactersBlendOrder.clear();
		for (ulong a=0;a<3 && save.charactersBlendOrder[a]!=255;a++)
			scr->charactersBlendOrder.push_back(save.charactersBlendOrder[a]);
	}

	scr->blendSprites=save.blendSprites;
	for (ulong a=0;a<scr->layerStack.size();a++){
		if (!scr->layerStack[a])
			continue;
		scr->layerStack[a]->unload();
	}
	for (ulong a=0;a<save.sprites.size();a++){
		NONS_SaveFile::Sprite *spr=save.sprites[a];
		if (spr)
			scr->loadSprite(a,spr->string,spr->x,spr->y,0xFF,spr->visibility);
	}
	scr->sprite_priority=save.spritePriority;
	this->screen->apply_monochrome_first=save.nega_parameter;
	this->screen->filterPipeline=save.pipelines[0];
	//Preparations for audio
	NONS_Audio *au=this->audio;
	au->stopAllSound();
	out->ephemeralOut(&out->currentBuffer,0,0,1,0);
	{
		ulong w,h;
		{
			NONS_MutexLocker ml(screenMutex);
			w=scr->screen->screens[VIRTUAL]->w;
			h=scr->screen->screens[VIRTUAL]->h;
		}
		SDL_Surface *srf=makeSurface(w,h,32);
		SDL_FillRect(srf,0,amask);
		NONS_GFX::callEffect(10,1000,0,srf,0,scr->screen);
		SDL_FreeSurface(srf);
	}
	SDL_Delay(1500);
	scr->BlendNoCursor(10,1000,0);
	this->screen->screen->applyFilter(0,SDL_Color(),L"");
	for (ulong a=0;a<save.pipelines[1].size();a++){
		pipelineElement &el=save.pipelines[1][a];
		this->screen->screen->applyFilter(el.effectNo+1,el.color,el.ruleStr);
	}
	this->screen->screen->stopEffect();
	if (save.asyncEffect_no)
		this->screen->screen->callEffect(save.asyncEffect_no,save.asyncEffect_freq);
	scr->showText();
	//audio
	if (save.musicTrack>=0){
		std::wstring temp=L"track";
		temp+=itoaw(save.musicTrack,2);
		au->playMusic(&temp,save.loopMp3?-1:0);
	}else if (save.music.size())
		this->audio->playMusic(&save.music,save.loopMp3?-1:0);
	au->musicVolume(save.musicVolume);
	for (ushort a=0;a<save.channels.size();a++){
		NONS_SaveFile::Channel *c=save.channels[a];
		if (!c->name.size())
			continue;
		au->playSoundAsync(&c->name,a,c->loop?-1:0);
	}
	if (this->loadgosub.size())
		this->gosub_label(this->loadgosub);
	return NONS_NO_ERROR;
}

bool NONS_ScriptInterpreter::save(int file){
	if (this->insideTextgosub())
		return 0;
	for (ulong a=0;a<this->saveGame->stack.size();a++)
		delete this->saveGame->stack[a];
	this->saveGame->stack.clear();
	for (variables_map_T::iterator i=this->saveGame->variables.begin();i!=this->saveGame->variables.end();i++)
		delete i->second;
	this->saveGame->variables.clear();
	for (arrays_map_T::iterator i=this->saveGame->arrays.begin();i!=this->saveGame->arrays.end();i++)
		delete i->second;
	this->saveGame->arrays.clear();
	this->saveGame->logPages.clear();
	for (ulong a=0;a<this->saveGame->sprites.size();a++)
		if (this->saveGame->sprites[a])
			delete this->saveGame->sprites[a];
	this->saveGame->sprites.clear();
	for (ulong a=0;a<this->saveGame->channels.size();a++)
		if (this->saveGame->channels[a])
			delete this->saveGame->channels[a];
	this->saveGame->channels.clear();
	//stack
	{
		for (std::vector<NONS_StackElement *>::iterator i=this->callStack.begin();i!=this->callStack.end();i++){
			NONS_SaveFile::stackEl *el=new NONS_SaveFile::stackEl();
			NONS_StackElement *el0=*i;
			el->type=el0->type;
			NONS_ScriptBlock *block=this->script->blockFromLine(el0->returnTo.line);
			el->label=block->name;
			el->linesBelow=el0->returnTo.line-block->first_line;
			el->statementNo=el0->returnTo.statement;
			el->textgosubLevel=el0->textgosubLevel;
			switch (el->type){
				case StackFrameType::SUBROUTINE_CALL:
					el->leftovers=el0->interpretAtReturn.toString();
					break;
				case StackFrameType::FOR_NEST:
					el->variable=0;
					for (variables_map_T::iterator i2=this->store->variables.begin();i2!=this->store->variables.end() && !el->variable;++i2)
						if (i2->second->intValue==el0->var)
							el->variable=i2->first;
					el->to=el0->to;
					el->step=el0->step;
					break;
				/*case StackFrameType::TEXTGOSUB_CALL:
					break;*/
				case StackFrameType::USERCMD_CALL:
					el->leftovers=el0->interpretAtReturn.toString();
					el->parameters=el0->parameters;
					break;
			}
			this->saveGame->stack.push_back(el);
		}
		{
			const NONS_ScriptBlock *block=this->thread->currentBlock;
			this->saveGame->currentLabel=block->name;
			NONS_Statement *stmt=this->thread->getCurrentStatement();
			this->saveGame->linesBelow=stmt->lineOfOrigin->lineNumber-block->first_line;
			this->saveGame->statementNo=stmt->statementNo;
		}
		this->saveGame->loadgosub=this->loadgosub;
	}
	//variables
	{
		variables_map_T *varStack=&this->store->variables;
		for (variables_map_T::iterator i=varStack->begin();i!=varStack->end() && i->first<200;i++)
			if (!VARIABLE_HAS_NO_DATA(i->second))
				this->saveGame->variables[i->first]=new NONS_Variable(*i->second);;
		for (arrays_map_T::iterator i=this->store->arrays.begin();
				i!=this->store->arrays.end();i++){
			this->saveGame->arrays[i->first]=new NONS_VariableMember(*(i->second));
		}
	}
	//screen
	{
		//window
		NONS_ScreenSpace *scr=this->screen;
		NONS_StandardOutput *out=scr->output;
		this->saveGame->textWindow=out->shadeLayer->clip_rect.to_SDL_Rect();
		this->saveGame->windowFrame.x=out->x0;
		this->saveGame->windowFrame.y=out->y0;
		this->saveGame->windowFrame.w=out->w;
		this->saveGame->windowFrame.h=out->h;
		ulong color=out->shadeLayer->defaultShade;
		this->saveGame->windowColor.r=(color>>rshift)&0xFF;
		this->saveGame->windowColor.g=(color>>gshift)&0xFF;
		this->saveGame->windowColor.b=(color>>bshift)&0xFF;
		this->saveGame->windowTransition=out->transition->effect;
		this->saveGame->windowTransitionDuration=out->transition->duration;
		this->saveGame->windowTransitionRule=out->transition->rule;
		this->saveGame->hideWindow=this->hideTextDuringEffect;
		this->saveGame->fontSize=(ushort)out->foregroundLayer->fontCache->get_size();
		this->saveGame->windowTextColor=out->foregroundLayer->fontCache->get_color();
		this->saveGame->textSpeed=out->display_speed;
		this->saveGame->fontShadow=!!out->shadowLayer;
		this->saveGame->shadowPosX=out->shadowPosX;
		this->saveGame->shadowPosY=out->shadowPosY;
		this->saveGame->spacing=out->foregroundLayer->fontCache->spacing;
		this->saveGame->lineSkip=(ushort)out->foregroundLayer->fontCache->line_skip;
		for (ulong a=0;a<out->log.size();a++)
			this->saveGame->logPages.push_back(out->log[a]);
		this->saveGame->currentBuffer=out->currentBuffer;
		this->saveGame->indentationLevel=out->indentationLevel;
		if (!this->arrowCursor || !this->arrowCursor->data)
			this->saveGame->arrow.string.clear();
		else{
			this->saveGame->arrow.string=this->arrowCursor->data->animation.getString();
			this->saveGame->arrow.x=this->arrowCursor->xpos;
			this->saveGame->arrow.y=this->arrowCursor->ypos;
			this->saveGame->arrow.absolute=this->arrowCursor->absolute;
		}
		if (!this->pageCursor || !this->pageCursor->data)
			this->saveGame->page.string.clear();
		else{
			this->saveGame->page.string=this->pageCursor->data->animation.getString();
			this->saveGame->page.x=this->pageCursor->xpos;
			this->saveGame->page.y=this->pageCursor->ypos;
			this->saveGame->page.absolute=this->pageCursor->absolute;
		}
		//graphic
		{
			NONS_Image *i=ImageLoader.elementFromSurface(scr->Background->data);
			if (i){
				this->saveGame->background=i->animation.getString();
			}else{
				this->saveGame->background.clear();
				color=scr->Background->defaultShade;
				this->saveGame->bgColor.r=(color>>rshift)&0xFF;
				this->saveGame->bgColor.g=(color>>gshift)&0xFF;
				this->saveGame->bgColor.b=(color>>bshift)&0xFF;
			}
		}
		this->saveGame->char_baseline=scr->char_baseline;
		NONS_Layer **characters[]={&scr->leftChar,&scr->centerChar,&scr->rightChar};
		for (int a=0;a<3;a++){
			if (!!*characters[a] && !!(*characters[a])->data){
				this->saveGame->characters[a].string=(*characters[a])->animation.getString();
				this->saveGame->characters[a].x=long((*characters[a])->position.x);
				this->saveGame->characters[a].y=long((*characters[a])->position.y);
				this->saveGame->characters[a].visibility=(*characters[a])->visible;
				this->saveGame->characters[a].alpha=(*characters[a])->alpha;
			}else
				this->saveGame->characters[a].string.clear();
		}
		//std::copy(scr->charactersBlendOrder.begin(),scr->charactersBlendOrder.end(),this->saveGame->charactersBlendOrder);
		for (size_t a=0;a<scr->charactersBlendOrder.size();a++)
			this->saveGame->charactersBlendOrder[a]=(uchar)scr->charactersBlendOrder[a];
		//std::fill(this->saveGame->charactersBlendOrder+scr->charactersBlendOrder.size(),this->saveGame->charactersBlendOrder+3,255);
		for (size_t a=scr->charactersBlendOrder.size();a<3;a++)
			this->saveGame->charactersBlendOrder[a]=255;
		//update sprite record
		this->saveGame->blendSprites=scr->blendSprites;
		for (ulong a=0;a<scr->layerStack.size();a++){
			if (this->saveGame->sprites.size()==a)
				this->saveGame->sprites.push_back(0);
			else if (this->saveGame->sprites.size()<a)
				this->saveGame->sprites.resize(a+1,0);
			NONS_SaveFile::Sprite *b=this->saveGame->sprites[a];
			NONS_Layer *c=scr->layerStack[a];
			if (!c || !c->data){
				delete b;
				this->saveGame->sprites[a]=0;
			}else{
				if (!b){
					NONS_SaveFile::Sprite *spr=new NONS_SaveFile::Sprite();
					NONS_Image *i=ImageLoader.elementFromSurface(c->data);
					if (i){
						spr->string=i->animation.getString();
						this->saveGame->sprites[a]=spr;
						b=spr;
					}else
						delete spr;
				}
				if (b){
					b->x=(long)c->position.x;
					b->y=(long)c->position.y;
					b->visibility=c->visible;
					b->alpha=c->alpha;
				}else
					o_stderr <<"NONS_ScriptInterpreter::save(): unresolvable inconsistent internal state.\n";
			}
		}
		this->saveGame->spritePriority=this->screen->sprite_priority;
		this->saveGame->nega_parameter=this->screen->apply_monochrome_first;
		this->saveGame->pipelines[0]=this->screen->filterPipeline;
		if (this->screen->screen->usingFeature[ASYNC_EFFECT]){
			this->saveGame->asyncEffect_no=this->screen->screen->aeffect_no;
			this->saveGame->asyncEffect_freq=this->screen->screen->aeffect_freq;
		}
		this->saveGame->pipelines[1]=this->screen->screen->filterPipeline;
	}
	{
		NONS_Audio *au=this->audio;
		if (!Mix_PlayingMusic()){
			this->saveGame->musicTrack=-1;
			this->saveGame->music.clear();
		}else
			this->saveGame->loopMp3=this->mp3_loop;
		int vol=au->musicVolume(-1);
		this->saveGame->musicVolume=vol<0?100:vol;
		if (au->isInitialized()){
			NONS_MutexLocker ml(au->soundcache->mutex);
			for (std::list<NONS_SoundEffect *>::iterator i=au->soundcache->channelWatch.begin();i!=au->soundcache->channelWatch.end();i++){
				NONS_SoundEffect *ch=*i;
				if (!ch || !ch->sound || !ch->isplaying)
					continue;
				NONS_SaveFile::Channel *cha=new NONS_SaveFile::Channel();
				cha->name=ch->sound->name;
				cha->loop=!!ch->loops;
				if (ch->channel>=ch->channel)
					this->saveGame->channels.resize(ch->channel+1,0);
				this->saveGame->channels[ch->channel]=cha;
			}
		}
	}
	bool ret=this->saveGame->save(save_directory+L"save"+itoaw(file)+L".dat");
	//Also save user data
	this->store->saveData();
	ImageLoader.filelog->writeOut();
	return ret;
}

ErrorCode NONS_ScriptInterpreter::command___userCommandCall__(NONS_Statement &stmt){
	if (!this->gosub_label(stmt.commandName))
		return NONS_UNDEFINED_ERROR;
	NONS_StackElement *el=new NONS_StackElement(this->callStack.back(),stmt.parameters);
	delete this->callStack.back();
	this->callStack.back()=el;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_add(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	NONS_VariableMember *var;
	GET_INT_VARIABLE(var,0);
	long val;
	GET_INT_VALUE(val,1);
	while (1){
		if (!stdStrCmpCI(stmt.commandName,L"add")){
			var->add(val);
			break;
		}
		if (!stdStrCmpCI(stmt.commandName,L"sub")){
			var->sub(val);
			break;
		}
		if (!stdStrCmpCI(stmt.commandName,L"mul")){
			var->mul(val);
			break;
		}
		if (!stdStrCmpCI(stmt.commandName,L"div")){
			if (!val)
				return NONS_DIVISION_BY_ZERO;
			var->div(val);
			break;
		}
		if (!stdStrCmpCI(stmt.commandName,L"mod")){
			if (!val)
				return NONS_DIVISION_BY_ZERO;
			var->mod(val);
			break;
		}
		if (!stdStrCmpCI(stmt.commandName,L"sin")){
			var->set(long(sin(M_PI*double(val)/180.0)*1000.0));
			break;
		}
		if (!stdStrCmpCI(stmt.commandName,L"cos")){
			var->set(long(cos(M_PI*double(val)/180.0)*1000.0));
			break;
		}
		if (!stdStrCmpCI(stmt.commandName,L"tan")){
			var->set(long(tan(M_PI*double(val)/180.0)*1000.0));
			break;
		}
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_add_filter(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long effect,rgb;
	std::wstring rule;
	GET_INT_VALUE(effect,0);
	if (stdStrCmpCI(stmt.commandName,L"add_filter")){
		if (!effect)
			this->screen->screen->applyFilter(0,SDL_Color(),L"");
		else{
			MINIMUM_PARAMETERS(3);
			GET_INT_VALUE(rgb,1);
			GET_STR_VALUE(rule,2);
			ErrorCode error=this->screen->screen->applyFilter(effect,rgb2SDLcolor(rgb),rule);
			if (error!=NONS_NO_ERROR)
				return error;
		}
	}else{
		if (!effect)
			this->screen->filterPipeline.clear();
		else{
			MINIMUM_PARAMETERS(3);
			GET_INT_VALUE(rgb,1);
			GET_STR_VALUE(rule,2);
			effect--;
			if ((ulong)effect>=NONS_GFX::filters.size())
				return NONS_NO_EFFECT;
			this->screen->filterPipeline.push_back(pipelineElement(effect,rgb2SDLcolor(rgb),rule,0));
		}
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_alias(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	NONS_VariableMember *var=this->store->retrieve(stmt.parameters[0],0);
	if (!var){
		if (!isValidIdentifier(stmt.parameters[0]))
			return NONS_INVALID_ID_NAME;
		NONS_VariableMember *val;
		if (!stdStrCmpCI(stmt.commandName,L"numalias")){
			val=new NONS_VariableMember(INTEGER);
			if (stmt.parameters.size()>1){
				long temp;
				GET_INT_VALUE(temp,1);
				val->set(temp);
			}
		}else{
			val=new NONS_VariableMember(STRING);
			if (stmt.parameters.size()>1){
				std::wstring temp;
				GET_STR_VALUE(temp,1);
				val->set(temp);
			}
		}
		val->makeConstant();
		this->store->constants[stmt.parameters[0]]=val;
		return NONS_NO_ERROR;
	}
	if (var->isConstant())
		return NONS_DUPLICATE_CONSTANT_DEFINITION;
	return NONS_INVALID_ID_NAME;
}

ErrorCode NONS_ScriptInterpreter::command_allsphide(NONS_Statement &stmt){
	this->screen->blendSprites=!stdStrCmpCI(stmt.commandName,L"allspresume");
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_async_effect(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long effectNo,
		frequency;
	GET_INT_VALUE(effectNo,0);
	if (!effectNo){
		this->screen->screen->stopEffect();
		return NONS_NO_ERROR;
	}
	MINIMUM_PARAMETERS(2);
	GET_INT_VALUE(frequency,1);
	if (effectNo<0 || frequency<=0)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	this->screen->screen->callEffect(effectNo-1,frequency);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_atoi(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	NONS_VariableMember *dst;
	GET_INT_VARIABLE(dst,0);
	std::wstring val;
	GET_STR_VALUE(val,1);
	dst->atoi(val);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_autoclick(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long ms;
	GET_INT_VALUE(ms,0);
	if (ms<0)
		ms=0;
	this->autoclick=ms;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_avi(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	std::wstring filename;
	long skippable;
	GET_STR_VALUE(filename,0);
	GET_INT_VALUE(skippable,1);
	return this->play_video(filename,!!skippable);
}

ErrorCode NONS_ScriptInterpreter::command_base_resolution(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	long w,h;
	GET_INT_VALUE(w,0);
	GET_INT_VALUE(h,1);
	this->base_size[0]=w;
	this->base_size[1]=h;
	for (int a=0;a<2;a++)
		ImageLoader.base_scale[a]=double(this->virtual_size[a])/double(this->base_size[a]);
	ulong size=this->defaultfs*this->virtual_size[1]/this->base_size[1];
	this->font_cache->set_size(size);
	this->screen->output->set_size(size);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_bg(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	NONS_ScreenSpace *scr=this->screen;
	long color=0;
	scr->hideText();
	if (!stdStrCmpCI(stmt.parameters[0],L"white")){
		scr->Background->setShade(-1,-1,-1);
		scr->Background->Clear();
	}else if (!stdStrCmpCI(stmt.parameters[0],L"black")){
		scr->Background->setShade(0,0,0);
		scr->Background->Clear();
	}else if (this->store->getIntValue(stmt.parameters[0],color,0)==NONS_NO_ERROR){
		char r=char((color&0xFF0000)>>16),
			g=(color&0xFF00)>>8,
			b=(color&0xFF);
		scr->Background->setShade(r,g,b);
		scr->Background->Clear();
	}else{
		std::wstring filename;
		GET_STR_VALUE(filename,0);
		scr->Background->load(&filename);
		NONS_MutexLocker ml(screenMutex);
		scr->Background->position.x=(scr->screen->screens[VIRTUAL]->w-scr->Background->clip_rect.w)/2;
		scr->Background->position.y=(scr->screen->screens[VIRTUAL]->h-scr->Background->clip_rect.h)/2;
	}
	scr->leftChar->unload();
	scr->rightChar->unload();
	scr->centerChar->unload();
	long number,duration;
	ErrorCode ret;
	GET_INT_VALUE(number,1);
	if (stmt.parameters.size()>2){
		std::wstring rule;
		GET_INT_VALUE(duration,2);
		if (stmt.parameters.size()>3)
			GET_STR_VALUE(rule,3);
		ret=scr->BlendNoCursor(number,duration,&rule);
	}else
		ret=scr->BlendNoCursor(number);
	return ret;
}

ErrorCode NONS_ScriptInterpreter::command_bgcopy(NONS_Statement &stmt){
	NONS_MutexLocker ml(screenMutex);
	this->screen->Background->load(this->screen->screen->screens[VIRTUAL]);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_blt(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(8);
	if (!this->imageButtons)
		return NONS_NO_BUTTON_IMAGE;
	float screenX,screenY,screenW,screenH,
		imgX,imgY,imgW,imgH;
	GET_COORDINATE(screenX,0,0);
	GET_COORDINATE(screenY,1,1);
	GET_COORDINATE(screenW,0,2);
	GET_COORDINATE(screenH,1,3);
	GET_COORDINATE(imgX,0,4);
	GET_COORDINATE(imgY,1,5);
	GET_COORDINATE(imgW,0,6);
	GET_COORDINATE(imgH,1,7);
	NONS_Rect dstRect={
			screenX,
			screenY,
			screenW,
			screenH
		},
		srcRect={
			imgX,
			imgY,
			imgW,
			imgH
	};
	void (*interpolationFunction)(SDL_Surface *,SDL_Rect *,SDL_Surface *,SDL_Rect *,ulong,ulong)=&nearestNeighborInterpolation;
	ulong x_multiplier=1,y_multiplier=1;
	if (imgW==screenW && imgH==screenH){
		NONS_MutexLocker ml(screenMutex);
		manualBlit(this->imageButtons->loadedGraphic,&srcRect.to_SDL_Rect(),this->screen->screen->screens[VIRTUAL],&dstRect.to_SDL_Rect());
	}else{
		x_multiplier=(ulong(screenW)<<8)/ulong(imgW);
		y_multiplier=(ulong(screenH)<<8)/ulong(imgH);
		NONS_MutexLocker ml(screenMutex);
		interpolationFunction(
			this->imageButtons->loadedGraphic,&srcRect.to_SDL_Rect(),
			this->screen->screen->screens[VIRTUAL],
			&dstRect.to_SDL_Rect(),x_multiplier,y_multiplier
		);
	}
	this->screen->screen->updateScreen((ulong)dstRect.x,(ulong)dstRect.y,(ulong)dstRect.w,(ulong)dstRect.h);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_br(NONS_Statement &stmt){
	return this->Printer(L"");
}

ErrorCode NONS_ScriptInterpreter::command_break(NONS_Statement &stmt){
	if (this->callStack.empty())
		return NONS_EMPTY_CALL_STACK;
	NONS_StackElement *element=this->callStack.back();
	if (element->type!=StackFrameType::FOR_NEST)
		return NONS_UNEXPECTED_NEXT;
	if (element->end!=element->returnTo){
		this->thread->gotoPair(element->returnTo.toPair());
		delete element;
		this->callStack.pop_back();
		return NONS_NO_ERROR;
	}
	std::pair<ulong,ulong> next=this->thread->getNextStatementPair();
	bool valid=0;
	while (!!(valid=this->thread->advanceToNextStatement())){
		NONS_Statement *pstmt=this->thread->getCurrentStatement();
		pstmt->parse(this->script);
		if (!stdStrCmpCI(pstmt->commandName,L"next"))
			break;
	}
	if (!valid){
		this->thread->gotoPair(next);
		return NONS_NO_NEXT;
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_btn(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(7);
	if (!this->imageButtons)
		return NONS_NO_BUTTON_IMAGE;
	long index;
	float butX,butY,width,height,srcX,srcY;
	GET_INT_VALUE(index,0);
	GET_COORDINATE(butX,0,1);
	GET_COORDINATE(butY,1,2);
	GET_COORDINATE(width,0,3);
	GET_COORDINATE(height,1,4);
	GET_COORDINATE(srcX,0,5);
	GET_COORDINATE(srcY,1,6);
	if (index<=0)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	index--;
	this->imageButtons->addImageButton(index,(int)butX,(int)butY,(int)width,(int)height,(int)srcX,(int)srcY);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_btndef(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	if (this->imageButtons)
		delete this->imageButtons;
	this->imageButtons=0;
	if (!stdStrCmpCI(stmt.parameters[0],L"clear"))
		return NONS_NO_ERROR;
	std::wstring filename;
	GET_STR_VALUE(filename,0);
	if (!filename.size()){
		SDL_Surface *tmpSrf;
		{
			NONS_MutexLocker ml(screenMutex);
			tmpSrf=makeSurface(
				this->screen->screen->screens[VIRTUAL]->w,
				this->screen->screen->screens[VIRTUAL]->h,
				32);
		}
		this->imageButtons=new NONS_ButtonLayer(tmpSrf,this->screen);
		this->imageButtons->inputOptions.Wheel=this->useWheel;
		this->imageButtons->inputOptions.EscapeSpace=this->useEscapeSpace;
		return NONS_NO_ERROR;
	}
	SDL_Surface *img;
	if (!ImageLoader.fetchSprite(img,filename)){
		ImageLoader.unfetchImage(img);
		return NONS_FILE_NOT_FOUND;
	}
	this->imageButtons=new NONS_ButtonLayer(img,this->screen);
	this->imageButtons->inputOptions.Wheel=this->useWheel;
	this->imageButtons->inputOptions.EscapeSpace=this->useEscapeSpace;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_btntime(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long time;
	GET_INT_VALUE(time,0);
	if (time<0)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	this->imageButtonExpiration=time;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_btnwait(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	if (!this->imageButtons)
		return NONS_NO_BUTTON_IMAGE;
	NONS_VariableMember *var;
	GET_INT_VARIABLE(var,0);
	int choice=this->imageButtons->getUserInput(this->imageButtonExpiration);
	if (choice==INT_MIN)
		return NONS_END;
	var->set(choice+1);
	this->btnTimer=SDL_GetTicks();
	if (choice>=0 && stdStrCmpCI(stmt.commandName,L"btnwait2")){
		delete this->imageButtons;
		this->imageButtons=0;
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_caption(NONS_Statement &stmt){
	if (!stmt.parameters.size())
		SDL_WM_SetCaption("",0);
	else{
		std::wstring temp;
		GET_STR_VALUE(temp,0);
#if !NONS_SYS_WINDOWS
		SDL_WM_SetCaption(UniToUTF8(temp).c_str(),0);
#else
		SetWindowText(mainWindow,temp.c_str());
#endif
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_cell(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	long sprt,
		cell;
	GET_INT_VALUE(sprt,0);
	GET_INT_VALUE(cell,1);
	if (sprt<0 || cell<0 || (ulong)sprt>=this->screen->layerStack.size())
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	NONS_Layer *layer=this->screen->layerStack[sprt];
	if (!layer || !layer->data)
		return NONS_NO_SPRITE_LOADED_THERE;
	if ((ulong)cell>=layer->animation.animation_length)
		cell=layer->animation.animation_length-1;
	layer->animation.resetAnimation();
	if (!cell)
		return NONS_NO_ERROR;
	if (layer->animation.frame_ends.size()==1)
		layer->animation.advanceAnimation(layer->animation.frame_ends[0]*cell);
	else
		layer->animation.advanceAnimation(layer->animation.frame_ends[cell-1]);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_centerh(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long fraction;
	GET_INT_VALUE(fraction,0);
	this->screen->output->setCenterPolicy('h',fraction);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_centerv(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long fraction;
	GET_INT_VALUE(fraction,0);
	this->screen->output->setCenterPolicy('v',fraction);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_checkpage(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	NONS_VariableMember *dst;
	GET_INT_VARIABLE(dst,0);
	long page;
	GET_INT_VALUE(page,1);
	if (page<0)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	dst->set(this->screen->output->log.size()>=(ulong)page);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_cl(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	switch (NONS_tolower(stmt.parameters[0][0])){
		case 'l':
			if (this->hideTextDuringEffect)
				this->screen->hideText();
			this->screen->leftChar->unload();
			break;
		case 'r':
			if (this->hideTextDuringEffect)
				this->screen->hideText();
			this->screen->rightChar->unload();
			break;
		case 'c':
			if (this->hideTextDuringEffect)
				this->screen->hideText();
			this->screen->centerChar->unload();
			break;
		case 'a':
			if (this->hideTextDuringEffect)
				this->screen->hideText();
			this->screen->leftChar->unload();
			this->screen->rightChar->unload();
			this->screen->centerChar->unload();
			break;
		default:
			return NONS_INVALID_PARAMETER;
	}
	long number,duration;
	ErrorCode ret;
	GET_INT_VALUE(number,1);
	if (stmt.parameters.size()>2){
		std::wstring rule;
		GET_INT_VALUE(duration,2);
		if (stmt.parameters.size()>3)
			GET_STR_VALUE(rule,3);
		ret=this->screen->BlendNoCursor(number,duration,&rule);
	}else
		ret=this->screen->BlendNoCursor(number);
	return ret;
}

ErrorCode NONS_ScriptInterpreter::command_click(NONS_Statement &stmt){
	waitUntilClick();
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_clickstr(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	GET_STR_VALUE(this->clickStr,0);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_clock(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	NONS_VariableMember *dst;
	GET_INT_VARIABLE(dst,0);
	dst->set(SDL_GetTicks());
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_cmp(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(3);
	NONS_VariableMember *var;
	GET_INT_VARIABLE(var,0);
	std::wstring opA,opB;
	GET_STR_VALUE(opA,1);
	GET_STR_VALUE(opB,2);
	var->set(wcscmp(opA.c_str(),opB.c_str()));
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_csp(NONS_Statement &stmt){
	long n=-1;
	if (stmt.parameters.size()>0)
		GET_INT_VALUE(n,0);
	if (n>0 && ulong(n)>=this->screen->layerStack.size())
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	if (n<0){
		for (ulong a=0;a<this->screen->layerStack.size();a++)
			if (this->screen->layerStack[a] && this->screen->layerStack[a]->data)
				this->screen->layerStack[a]->unload();
	}else if (this->screen->layerStack[n] && this->screen->layerStack[n]->data)
		this->screen->layerStack[n]->unload();
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_date(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(3);
	NONS_VariableMember *year,*month,*day;
	GET_INT_VARIABLE(year,0);
	GET_INT_VARIABLE(month,1);
	GET_INT_VARIABLE(day,2);
	time_t t=time(0);
	tm *time=localtime(&t);
	if (stdStrCmpCI(stmt.commandName,L"date2"))
		year->set(time->tm_year%100);
	else
		year->set(time->tm_year+1900);
	month->set(time->tm_mon+1);
	day->set(time->tm_mday);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_defaultspeed(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(3);
	long slow,med,fast;
	GET_INT_VALUE(slow,0);
	GET_INT_VALUE(med,1);
	GET_INT_VALUE(fast,2);
	if (slow<0 || med<0 || fast<0)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	this->default_speed_slow=slow;
	this->default_speed_med=med;
	this->default_speed_fast=fast;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_defsub(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	std::wstring name=stmt.parameters[0];
	trim_string(name);
	if (name[0]=='*')
		name=name.substr(name.find_first_not_of('*'));
	if (!isValidIdentifier(name))
		return NONS_INVALID_COMMAND_NAME;
	if (this->commandList.find(name)!=this->commandList.end())
		return NONS_DUPLICATE_COMMAND_DEFINITION_BUILTIN;
	if (this->userCommandList.find(name)!=this->userCommandList.end())
		return NONS_DUPLICATE_COMMAND_DEFINITION_USER;
	if (!this->script->blockFromLabel(name))
		return NONS_NO_SUCH_BLOCK;
	this->userCommandList.insert(name);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_delay(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long delay;
	GET_INT_VALUE(delay,0);
	waitCancellable(delay);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_deletescreenshot(NONS_Statement &stmt){
	if (!!this->screenshot)
		SDL_FreeSurface(this->screenshot);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_dim(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	std::vector<long> indices;
	HANDLE_POSSIBLE_ERRORS(this->store->array_declaration(indices,stmt.parameters[0]));
	if (indices[0]>1)
		return NONS_UNDEFINED_ARRAY;
	for (size_t a=1;a<indices.size();a++)
		if (indices[a]<0)
			return NONS_NEGATIVE_INDEX_IN_ARRAY_DECLARATION;
	this->store->arrays[indices[0]]=new NONS_VariableMember(indices,1);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_draw(NONS_Statement &stmt){
	this->screen->screen->blitToScreen(this->screen->screenBuffer,0,0);
	NONS_MutexLocker ml(screenMutex);
	this->screen->screen->updateWithoutLock();
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_drawbg(NONS_Statement &stmt){
	if (!this->screen->Background || !this->screen->Background->data)
		SDL_FillRect(this->screen->screenBuffer,0,this->screen->screenBuffer->format->Amask);
	else if (!stdStrCmpCI(stmt.commandName,L"drawbg"))
		manualBlit(
			this->screen->Background->data,
			0,
			this->screen->screenBuffer,
			&this->screen->Background->position.to_SDL_Rect());
	else{
		MINIMUM_PARAMETERS(5);
		float x,y;
		long xscale,yscale,angle;
		GET_COORDINATE(x,0,0);
		GET_COORDINATE(y,1,1);
		GET_INT_VALUE(xscale,2);
		GET_INT_VALUE(yscale,3);
		GET_INT_VALUE(angle,4);
		if (!(xscale*yscale))
			SDL_FillRect(this->screen->screenBuffer,0,this->screen->screenBuffer->format->Amask);
		else{
			SDL_Surface *src=this->screen->Background->data;
			NONS_Image *img=ImageLoader.elementFromSurface(src);
			bool freeSrc=0;

			if (xscale<0 || yscale<0){
				SDL_Surface *dst=makeSurface(src->w,src->h,32);
				if (yscale>0)
					FlipSurfaceH(src,dst);
				else if (xscale>0)
					FlipSurfaceV(src,dst);
				else
					FlipSurfaceHV(src,dst);
				xscale=ABS(xscale);
				yscale=ABS(yscale);
				src=dst;
				freeSrc=1;
			}

			if (src->format->BitsPerPixel!=32){
				SDL_Surface *dst=makeSurface(src->w,src->h,32);
				manualBlit(src,0,dst,0);
				freeSrc=1;
			}
			if (img->svg_source){
				ImageLoader.svg_functions.SVG_set_scale(img->svg_source,double(xscale)/100.0,double(yscale)/100.0);
				ImageLoader.svg_functions.SVG_set_rotation(img->svg_source,-double(angle));
				ImageLoader.svg_functions.SVG_add_scale(img->svg_source,ImageLoader.base_scale[0],ImageLoader.base_scale[1]);
				SDL_Surface *dst=ImageLoader.svg_functions.SVG_render(img->svg_source);
				if (freeSrc)
					SDL_FreeSurface(src);
				src=dst;
			}else{
				if (xscale!=100 || yscale!=100){
					SDL_Surface *dst=resizeFunction(src,src->w*xscale/100,src->h*yscale/100);
					if (freeSrc)
						SDL_FreeSurface(src);
					src=dst;
				}
				if (angle!=0){
					SDL_Surface *dst=rotationFunction(src,double(angle)/180*M_PI);
					SDL_FreeSurface(src);
					src=dst;
				}
			}
			SDL_Rect dstR={
				Sint16(-long(src->clip_rect.w/2)+x),
				Sint16(-long(src->clip_rect.h/2)+y),
				0,0
			};
			manualBlit(src,0,this->screen->screenBuffer,&dstR);
			SDL_FreeSurface(src);
		}
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_drawclear(NONS_Statement &stmt){
	SDL_LockSurface(this->screen->screenBuffer);
	surfaceData sd=this->screen->screenBuffer;
	Uint32 *p=(Uint32 *)sd.pixels;
	Uint32 mask=this->screen->screenBuffer->format->Amask;
	for (ulong n=sd.w*sd.h;n;n--)
		*p++=mask;
	SDL_UnlockSurface(this->screen->screenBuffer);
	//SDL_FillRect(this->screen->screenBuffer,0,this->screen->screenBuffer->format->Amask);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_drawfill(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(3);
	long r,g,b;
	GET_INT_VALUE(r,0);
	GET_INT_VALUE(g,1);
	GET_INT_VALUE(b,2);
	r=ulong(r)&0xFF;
	g=ulong(g)&0xFF;
	b=ulong(b)&0xFF;
	SDL_Surface *dst=this->screen->screenBuffer;
	surfaceData sd(dst);
	SDL_FillRect(dst,0,r<<sd.Roffset|g<<sd.Goffset|b<<sd.Boffset|0xFF<<sd.Aoffset);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_drawsp(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(5);
	long spriteno,
		cell,
		alpha;
	float x,y;
	long xscale=0,yscale=0,
		rotation,
		matrix_00=0,matrix_01=0,
		matrix_10=0,matrix_11=0;
	GET_INT_VALUE(spriteno,0);
	GET_INT_VALUE(cell,1);
	GET_INT_VALUE(alpha,2);
	GET_COORDINATE(x,0,3);
	GET_COORDINATE(y,1,4);
	ulong functionVersion=1;
	if (!stdStrCmpCI(stmt.commandName,L"drawsp2"))
		functionVersion=2;
	else if (!stdStrCmpCI(stmt.commandName,L"drawsp3"))
		functionVersion=3;
	switch (functionVersion){
		case 2:
			MINIMUM_PARAMETERS(8);
			GET_INT_VALUE(xscale,5);
			GET_INT_VALUE(yscale,6);
			GET_INT_VALUE(rotation,7);
			break;
		case 3:
			MINIMUM_PARAMETERS(9);
			GET_INT_VALUE(matrix_00,5);
			GET_INT_VALUE(matrix_01,6);
			GET_INT_VALUE(matrix_10,7);
			GET_INT_VALUE(matrix_11,8);
			break;
	}


	std::vector<NONS_Layer *> &sprites=this->screen->layerStack;
	if (spriteno<0 || (ulong)spriteno>sprites.size())
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	NONS_Layer *sprite=sprites[spriteno];
	if (!sprite || !sprite->data)
		return NONS_NO_SPRITE_LOADED_THERE;
	SDL_Surface *src=sprite->data;
	NONS_Image *img=ImageLoader.elementFromSurface(src);
	if (cell<0 || (ulong)cell>=sprite->animation.animation_length)
		return NONS_NO_ERROR;
	if (functionVersion==2 && !(xscale*yscale))
		return NONS_NO_ERROR;


	SDL_Rect srcRect=sprite->clip_rect.to_SDL_Rect();
	srcRect.x=Sint16(src->w/sprite->animation.animation_length*cell);
	srcRect.y=0;
	SDL_Rect dstRect={(Sint16)x,(Sint16)y,0,0};


	bool freeSrc=0;
	if (functionVersion>1){
		SDL_Surface *temp=makeSurface(srcRect.w,srcRect.h,32);
		manualBlit(src,&srcRect,temp,0);
		src=temp;
		freeSrc=1;
	}
	switch (functionVersion){
		case 2:
			{
				SDL_Surface *dst;
				if (xscale<0 || yscale<0){
					dst=makeSurface(srcRect.w,srcRect.h,32);
					if (yscale>0)
						FlipSurfaceH(src,dst);
					else if (xscale>0)
						FlipSurfaceV(src,dst);
					else
						FlipSurfaceHV(src,dst);
					SDL_FreeSurface(src);
					xscale=ABS(xscale);
					yscale=ABS(yscale);
					src=dst;
				}
				if (img->svg_source){
					ImageLoader.svg_functions.SVG_set_scale(img->svg_source,double(xscale)/100.0,double(yscale)/100.0);
					ImageLoader.svg_functions.SVG_set_rotation(img->svg_source,-double(rotation));
					ImageLoader.svg_functions.SVG_add_scale(img->svg_source,ImageLoader.base_scale[0],ImageLoader.base_scale[1]);
					dst=ImageLoader.svg_functions.SVG_render(img->svg_source);
					SDL_FreeSurface(src);
					src=dst;
				}else{
					if (xscale!=100 || yscale!=100 || this->virtual_size[0]!=this->base_size[0] || this->virtual_size[1]!=this->base_size[1]){
						dst=resizeFunction(src,src->w*xscale/100,src->h*yscale/100);
						SDL_FreeSurface(src);
						src=dst;
					}
					if (rotation){
						dst=rotationFunction(src,double(rotation)/180*M_PI);
						SDL_FreeSurface(src);
						src=dst;
					}
				}
			}
			break;
		case 3:
			if (img->svg_source){
				double matrix_safe[]={1,1,1,1};
				double matrix[]={
					double(matrix_00)/1000.0,
					double(matrix_01)/1000.0,
					double(matrix_10)/1000.0,
					double(matrix_11)/1000.0
				};
				ImageLoader.svg_functions.SVG_set_matrix(img->svg_source,matrix_safe);
				ImageLoader.svg_functions.SVG_set_matrix(img->svg_source,matrix);
				SDL_Surface *dst=ImageLoader.svg_functions.SVG_render(img->svg_source);
				SDL_FreeSurface(src);
				src=dst;
			}else{
				float matrix[]={
					float(matrix_00)/1000.0f,
					float(matrix_01)/1000.0f,
					float(matrix_10)/1000.0f,
					float(matrix_11)/1000.0f
				};
				SDL_Surface *dst=applyTransformationMatrix(src,matrix);
				if (!dst)
					return NONS_BAD_MATRIX;
				SDL_FreeSurface(src);
				src=dst;
			}
			break;
	}
	if (functionVersion>1){
		srcRect.x=0;
		srcRect.y=0;
		srcRect.w=src->w;
		srcRect.h=src->h;
		dstRect.x-=srcRect.w/2;
		dstRect.y-=srcRect.h/2;
	}


	manualBlit(src,&srcRect,this->screen->screenBuffer,&dstRect,(alpha<-0xFF)?-0xFF:((alpha>0xFF)?0xFF:alpha));


	if (freeSrc)
		SDL_FreeSurface(src);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_drawtext(NONS_Statement &stmt){
	NONS_ScreenSpace *scr=this->screen;
	if (!scr->output->shadeLayer->useDataAsDefaultShade)
		multiplyBlend(
			scr->output->shadeLayer->data,0,
			scr->screenBuffer,
			&scr->output->shadeLayer->clip_rect.to_SDL_Rect());
	else
		manualBlit(
			scr->output->shadeLayer->data,0,
			scr->screenBuffer,
			&scr->output->shadeLayer->clip_rect.to_SDL_Rect());
	if (scr->output->shadowLayer)
		manualBlit(
			scr->output->shadowLayer->data,0,
			scr->screenBuffer,
			&scr->output->shadowLayer->clip_rect.to_SDL_Rect(),
			scr->output->shadowLayer->alpha);
	manualBlit(
		scr->output->foregroundLayer->data,0,
		scr->screenBuffer,
		&scr->output->foregroundLayer->clip_rect.to_SDL_Rect(),
		scr->output->foregroundLayer->alpha);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_dwave(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	long channel;
	GET_INT_VALUE(channel,0);
	if (channel<0 || channel>7)
		return NONS_INVALID_CHANNEL_INDEX;
	std::wstring name;
	GET_STR_VALUE(name,1);
	tolower(name);
	toforwardslash(name);
	long loop=!stdStrCmpCI(stmt.commandName,L"dwave")?0:-1;
	return this->audio->playSoundAsync(&name,channel,loop);
}

ErrorCode NONS_ScriptInterpreter::command_dwaveload(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	long channel;
	GET_INT_VALUE(channel,0);
	if (channel<0)
		return NONS_INVALID_CHANNEL_INDEX;
	std::wstring name;
	GET_STR_VALUE(name,1);
	tolower(name);
	toforwardslash(name);
	ErrorCode error=NONS_NO_ERROR;
	if (!this->audio->bufferIsLoaded(name))
		this->audio->loadAsyncBuffer(name,channel);
	return error;
}

ErrorCode NONS_ScriptInterpreter::command_effect(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	long code,effect,timing=0;
	std::wstring rule;
	GET_INT_VALUE(code,0);
	if (this->gfx_store->retrieve(code))
		return NONS_DUPLICATE_EFFECT_DEFINITION;
	GET_INT_VALUE(effect,1);
	if (stmt.parameters.size()>2)
		GET_INT_VALUE(timing,2);
	if (stmt.parameters.size()>3)
		GET_STR_VALUE(rule,3);
	NONS_GFX *gfx=this->gfx_store->add(code,effect,timing,&rule);
	gfx->stored=1;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_effectblank(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long a;
	GET_INT_VALUE(a,0);
	if (a<0)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	NONS_GFX::effectblank=a;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_end(NONS_Statement &stmt){
	return NONS_END;
}

ErrorCode NONS_ScriptInterpreter::command_erasetextwindow(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long yesno;
	GET_INT_VALUE(yesno,0);
	this->hideTextDuringEffect=!!yesno;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_fileexist(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	NONS_VariableMember *dst;
	GET_INT_VARIABLE(dst,0);
	std::wstring filename;
	GET_STR_VALUE(filename,1);
	dst->set(general_archive.exists(filename));
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_filelog(NONS_Statement &stmt){
	ImageLoader.filelog->commit=1;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_for(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(3);
	NONS_VariableMember *var;
	GET_INT_VARIABLE(var,0);
	long from,to,step=1;
	GET_INT_VALUE(from,1);
	GET_INT_VALUE(to,2);
	if (stmt.parameters.size()>3)
		GET_INT_VALUE(step,3);
	if (!step)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	var->set(from);
	NONS_StackElement *element=new NONS_StackElement(var,this->thread->getNextStatementPair(),from,to,step,this->insideTextgosub());
	this->callStack.push_back(element);
	if (step>0 && from>to || step<0 && from<to)
		return this->command_break(stmt);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_game(NONS_Statement &stmt){
	//this->interpreter_mode=NORMAL;
	if (!this->thread->gotoLabel(L"start"))
		return NONS_NO_START_LABEL;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_getbtntimer(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	NONS_VariableMember *var;
	GET_INT_VARIABLE(var,0);
	var->set(SDL_GetTicks()-this->btnTimer);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_getcursor(NONS_Statement &stmt){
	if (!this->imageButtons)
		return NONS_NO_BUTTON_IMAGE;
	this->imageButtons->inputOptions.Cursor=1;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_getcursorpos(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	NONS_VariableMember *x,*y;
	GET_INT_VARIABLE(x,0);
	GET_INT_VARIABLE(y,1);
	x->set(this->screen->output->x);
	y->set(this->screen->output->y);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_getenter(NONS_Statement &stmt){
	if (!this->imageButtons)
		return NONS_NO_BUTTON_IMAGE;
	this->imageButtons->inputOptions.Enter=1;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_getfunction(NONS_Statement &stmt){
	if (!this->imageButtons)
		return NONS_NO_BUTTON_IMAGE;
	this->imageButtons->inputOptions.Function=1;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_getini(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(4);
	NONS_VariableMember *dst;
	GET_VARIABLE(dst,0);
	std::wstring section,
		filename,
		key;
	GET_STR_VALUE(filename,0);
	GET_STR_VALUE(section,1);
	GET_STR_VALUE(key,2);
	INIcacheType::iterator i=this->INIcache.find(filename);
	INIfile *file=0;
	if (i==this->INIcache.end()){
		NONS_DataStream *stream=general_archive.open(filename);
		if (!stream)
			return NONS_FILE_NOT_FOUND;
		std::vector<uchar> buffer;
		stream->read_all(buffer);
		this->INIcache[filename]=new INIfile(buffer,CLOptions.scriptencoding);
	}else
		file=i->second;
	INIsection *sec=file->getSection(section);
	if (!sec)
		return NONS_INI_SECTION_NOT_FOUND;
	INIvalue *val=sec->getValue(key);
	if (!val)
		return NONS_INI_KEY_NOT_FOUND;
	switch (dst->getType()){
		case INTEGER:
			dst->set(val->getIntValue());
			break;
		case STRING:
			dst->set(val->getStrValue());
			break;
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_getinsert(NONS_Statement &stmt){
	if (!this->imageButtons)
		return NONS_NO_BUTTON_IMAGE;
	this->imageButtons->inputOptions.Insert=1;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_getlog(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	NONS_VariableMember *dst;
	GET_STR_VARIABLE(dst,0);
	long page;
	GET_INT_VALUE(page,1);
	NONS_StandardOutput *out=this->screen->output;
	if (page<0)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	if (!page)
		return this->command_gettext(stmt);
	if (out->log.size()<(ulong)page)
		return NONS_NOT_ENOUGH_LOG_PAGES;
	std::wstring text=removeTags(out->log[out->log.size()-page]);
	dst->set(text);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_getmp3vol(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	NONS_VariableMember *dst;
	GET_INT_VARIABLE(dst,0);
	dst->set(this->audio->musicVolume(-1));
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_getpage(NONS_Statement &stmt){
	if (!this->imageButtons)
		return NONS_NO_BUTTON_IMAGE;
	this->imageButtons->inputOptions.PageUpDown=1;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_getparam(NONS_Statement &stmt){
	std::vector<std::wstring> *parameters=0;
	for (ulong a=this->callStack.size()-1;a<this->callStack.size() && !parameters;a--)
		if (this->callStack[a]->type==StackFrameType::USERCMD_CALL)
			parameters=&this->callStack[a]->parameters;
	if (!parameters)
		return NONS_NOT_IN_A_USER_COMMAND_CALL;
	ErrorCode error;
	std::vector<std::pair<NONS_VariableMember *,std::pair<long,std::wstring> > > actions;
	for (ulong a=0;a<parameters->size() && a<stmt.parameters.size();a++){
		wchar_t c=NONS_tolower(stmt.parameters[a][0]);
		if (c=='i' || c=='s'){
			NONS_VariableMember *src=this->store->retrieve((*parameters)[a],&error),
				*dst;

			if (!src)
				return error;
			if (src->isConstant())
				return NONS_EXPECTED_VARIABLE;
			if (src->getType()==INTEGER_ARRAY)
				return NONS_EXPECTED_SCALAR;

			dst=this->store->retrieve(stmt.parameters[a].substr(1),&error);
			if (!dst)
				return error;
			if (dst->isConstant())
				return NONS_EXPECTED_VARIABLE;
			if (dst->getType()==INTEGER_ARRAY)
				return NONS_EXPECTED_SCALAR;
			if (dst->getType()!=INTEGER)
				return NONS_EXPECTED_NUMERIC_VARIABLE;

			Sint32 index=this->store->getVariableIndex(src);
			actions.resize(actions.size()+1);
			actions.back().first=dst;
			actions.back().second.first=index;
		}else{
			NONS_VariableMember *dst=this->store->retrieve(stmt.parameters[a],&error);

			if (!dst)
				return error;
			if (dst->isConstant())
				return NONS_EXPECTED_VARIABLE;
			if (dst->getType()==INTEGER_ARRAY)
				return NONS_EXPECTED_SCALAR;
			if (dst->getType()==INTEGER){
				long val;
				HANDLE_POSSIBLE_ERRORS(this->store->getIntValue((*parameters)[a],val,0));
				actions.resize(actions.size()+1);
				actions.back().first=dst;
				actions.back().second.first=val;
			}else{
				std::wstring val;
				HANDLE_POSSIBLE_ERRORS(this->store->getWcsValue((*parameters)[a],val,0));
				actions.resize(actions.size()+1);
				actions.back().first=dst;
				actions.back().second.second=val;
			}
		}
	}

	for (ulong a=0;a<actions.size();a++){
		if (actions[a].first->getType()==INTEGER)
			actions[a].first->set(actions[a].second.first);
		else
			actions[a].first->set(actions[a].second.second);
	}

	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_getscreenshot(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	long w,h;
	GET_INT_VALUE(w,0);
	GET_INT_VALUE(h,1);
	if (w<=0 || h<=0)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	if (!!this->screenshot)
		SDL_FreeSurface(this->screenshot);
	screenMutex.lock();
	SDL_Surface *scr=this->screen->screen->screens[VIRTUAL];
	if (scr->format->BitsPerPixel<32){
		SDL_Surface *temp=makeSurface(scr->w,scr->h,32);
		manualBlit(scr,0,temp,0);
		screenMutex.unlock();
		this->screenshot=SDL_ResizeSmooth(temp,w,h);
		SDL_FreeSurface(temp);
	}else{
		this->screenshot=SDL_ResizeSmooth(scr,w,h);
		screenMutex.unlock();
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_gettab(NONS_Statement &stmt){
	if (!this->imageButtons)
		return NONS_NO_BUTTON_IMAGE;
	this->imageButtons->inputOptions.Tab=1;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_gettext(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	NONS_VariableMember *dst;
	GET_STR_VARIABLE(dst,0);
	std::wstring text=removeTags(this->screen->output->currentBuffer);
	dst->set(text);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_gettimer(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	NONS_VariableMember *var;
	GET_INT_VARIABLE(var,0);
	var->set(this->timer);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_getversion(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	NONS_VariableMember *dst;
	GET_VARIABLE(dst,0);
	if (dst->getType()==INTEGER)
		dst->set(ONSLAUGHT_BUILD_VERSION);
	else
		dst->set(ONSLAUGHT_BUILD_VERSION_WSTR);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_getzxc(NONS_Statement &stmt){
	if (!this->imageButtons)
		return NONS_NO_BUTTON_IMAGE;
	this->imageButtons->inputOptions.ZXC=1;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_globalon(NONS_Statement &stmt){
	this->store->commitGlobals=1;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_gosub(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	std::wstring label;
	GET_LABEL(label,0);
	if (!this->gosub_label(label)){
		handleErrors(NONS_NO_SUCH_BLOCK,stmt.lineOfOrigin->lineNumber,"NONS_ScriptInterpreter::command_gosub",1);
		return NONS_NO_SUCH_BLOCK;
	}
	return NONS_GOSUB;
}

ErrorCode NONS_ScriptInterpreter::command_goto(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	std::wstring label;
	GET_LABEL(label,0);
	if (!this->goto_label(label))
		return NONS_NO_SUCH_BLOCK;
	return NONS_NO_ERROR_BUT_BREAK;
}

ErrorCode NONS_ScriptInterpreter::command_humanorder(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	std::wstring order;
	GET_STR_VALUE(order,0);
	std::vector<ulong> porder;
	bool found[3]={0};
	ulong offsets[26];
	offsets['l'-'a']=0;
	offsets['c'-'a']=1;
	offsets['r'-'a']=2;
	for (ulong a=0;a<order.size();a++){
		wchar_t c=NONS_tolower(order[a]);
		switch (c){
			case 'l':
			case 'c':
			case 'r':
				if (found[offsets[c-'a']])
					break;
				porder.push_back(offsets[c-'a']);
				found[offsets[c-'a']]=1;
				break;
			default:;
		}
	}
	std::reverse(porder.begin(),porder.end());
	if (stmt.parameters.size()==1){
		this->screen->charactersBlendOrder=porder;
		return NONS_NO_ERROR;
	}
	long number,duration;
	ErrorCode ret;
	GET_INT_VALUE(number,1);
	if (stmt.parameters.size()>2){
		GET_INT_VALUE(duration,2);
		std::wstring rule;
		if (stmt.parameters.size()>3)
			GET_STR_VALUE(rule,3);
		this->screen->hideText();
		this->screen->charactersBlendOrder=porder;
		ret=this->screen->BlendNoCursor(number,duration,&rule);
	}else{
		this->screen->hideText();
		this->screen->charactersBlendOrder=porder;
		ret=this->screen->BlendNoCursor(number);
	}
	return ret;
}

ErrorCode NONS_ScriptInterpreter::command_humanz(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long z;
	GET_INT_VALUE(z,0);
	if (z<-1)
		z=-1;
	else if (ulong(z)>=this->screen->layerStack.size())
		z=this->screen->layerStack.size()-1;
	this->screen->sprite_priority=z;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_if(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	long res=0;
	bool notif=!stdStrCmpCI(stmt.commandName,L"notif");
	HANDLE_POSSIBLE_ERRORS(this->store->getIntValue(stmt.parameters[0],res,notif && !this->new_if));
	if (notif && this->new_if)
		res=!res;
	if (!res)
		return NONS_NO_ERROR;
	ErrorCode ret=NONS_NO_ERROR;
	NONS_ScriptLine line(0,stmt.parameters[1],0,1);
	for (ulong a=0;a<line.statements.size();a++){
		ErrorCode error=this->interpretString(*line.statements[a],stmt.lineOfOrigin,stmt.fileOffset);
		if (error==NONS_END)
			return NONS_END;
		if (error==NONS_GOSUB)
			this->callStack.back()->interpretAtReturn=NONS_ScriptLine(line,a+1);
		if (!CHECK_FLAG(error,NONS_NO_ERROR_FLAG)){
			handleErrors(error,-1,"NONS_ScriptInterpreter::command_if",1);
			ret=NONS_UNDEFINED_ERROR;
		}
		if (CHECK_FLAG(error,NONS_BREAK_WORTHY_ERROR))
			break;
	}
	return ret;
}

ErrorCode NONS_ScriptInterpreter::command_inc(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	NONS_VariableMember *var;
	GET_INT_VARIABLE(var,0);
	if (!stdStrCmpCI(stmt.commandName,L"inc"))
		var->inc();
	else
		var->dec();
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_indent(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long indent;
	GET_INT_VALUE(indent,0);
	if (indent<0)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	this->screen->output->indentationLevel=indent;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_intlimit(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(3);
	NONS_VariableMember *dst;
	GET_INT_VARIABLE(dst,0);
	long lower,upper;
	GET_INT_VALUE(lower,1);
	GET_INT_VALUE(upper,2);
	dst->setlimits(lower,upper);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_isdown(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	NONS_VariableMember *var;
	GET_INT_VARIABLE(var,0);
	var->set(CHECK_FLAG(SDL_GetMouseState(0,0),SDL_BUTTON(SDL_BUTTON_LEFT)));
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_isfull(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	NONS_VariableMember *var;
	GET_INT_VARIABLE(var,0);
	NONS_MutexLocker ml(screenMutex);
	var->set(this->screen->screen->fullscreen);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_ispage(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	NONS_VariableMember *dst;
	GET_INT_VARIABLE(dst,0);
	if (!this->insideTextgosub())
		dst->set(0);
	else{
		std::vector<NONS_StackElement *>::reverse_iterator i=this->callStack.rbegin();
		for (;i!=this->callStack.rend() && (*i)->type!=StackFrameType::TEXTGOSUB_CALL;i++);
		dst->set((*i)->textgosubTriggeredBy=='\\');
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_itoa(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	NONS_VariableMember *dst;
	GET_STR_VARIABLE(dst,0);
	long src;
	GET_INT_VALUE(src,1);
	std::wstringstream stream;
	stream <<src;
	std::wstring str=stream.str();
	if (!stdStrCmpCI(stmt.commandName,L"itoa2")){
		for (ulong a=0;a<str.size();a++)
			if (NONS_isdigit(str[a]))
				str[a]+=0xFEE0;
	}
	dst->set(str);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_jumpf(NONS_Statement &stmt){
	if (!stdStrCmpCI(stmt.commandName,L"jumpb")){
		if (!this->thread->gotoJumpBackwards(stmt.fileOffset))
			return NONS_NO_JUMPS;
		return NONS_NO_ERROR_BUT_BREAK;
	}else{
		if (!this->thread->gotoJumpForward(stmt.fileOffset))
			return NONS_NO_JUMPS;
		return NONS_NO_ERROR_BUT_BREAK;
	}
}

ErrorCode NONS_ScriptInterpreter::command_labellog(NONS_Statement &stmt){
	labellog.commit=1;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_ld(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(3);
	std::wstring name;
	GET_STR_VALUE(name,1);
	NONS_Layer **l=0;
	long off;
	int width;
	{
		NONS_MutexLocker ml(screenMutex);
		width=this->screen->screen->screens[VIRTUAL]->w;
	}
	switch (stmt.parameters[0][0]){
		case 'l':
			l=&this->screen->leftChar;
			off=width/4;
			break;
		case 'c':
			l=&this->screen->centerChar;
			off=width/2;
			break;
		case 'r':
			l=&this->screen->rightChar;
			off=width/4*3;
			break;
		default:
			return NONS_INVALID_PARAMETER;
	}
	if (this->hideTextDuringEffect)
		this->screen->hideText();
	if (!*l)
		*l=new NONS_Layer(&name);
	else if (!(*l)->load(&name))
		return NONS_FILE_NOT_FOUND;
	if (!(*l)->data)
		return NONS_FILE_NOT_FOUND;
	(*l)->centerAround(off);
	(*l)->useBaseline(this->screen->char_baseline);
	long number,duration;
	ErrorCode ret;
	GET_INT_VALUE(number,2);
	if (stmt.parameters.size()>3){
		GET_INT_VALUE(duration,3);
		std::wstring rule;
		if (stmt.parameters.size()>4)
			GET_STR_VALUE(rule,4);
		ret=this->screen->BlendNoCursor(number,duration,&rule);
	}else
		ret=this->screen->BlendNoCursor(number);
	return ret;
}

ErrorCode NONS_ScriptInterpreter::command_len(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	NONS_VariableMember *dst;
	GET_INT_VARIABLE(dst,0);
	std::wstring src;
	GET_STR_VALUE(src,1);
	dst->set(src.size());
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_literal_print(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	std::wstring string=this->convertParametersToString(stmt);
	if (string.size()){
		this->screen->showText();
		if (this->screen->output->prepareForPrinting(string.c_str())){
			if (this->pageCursor->animate(this->menu,this->autoclick)<0)
				return NONS_NO_ERROR;
			this->screen->clearText();
		}
		while (this->screen->output->print(0,string.size(),this->screen->screen)){
			if (this->pageCursor){
				if (this->pageCursor->animate(this->menu,this->autoclick)<0)
					return NONS_NO_ERROR;
			}else
				waitUntilClick();
			this->screen->clearText();
		}
		this->screen->output->endPrinting();
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_loadgame(NONS_Statement &stmt){
	long file;
	GET_INT_VALUE(file,0);
	if (file<1)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	return this->load(file);
}

ErrorCode NONS_ScriptInterpreter::command_loadgosub(NONS_Statement &stmt){
	if (!stmt.parameters.size()){
		this->loadgosub.clear();
		return NONS_NO_ERROR;
	}
	if (!this->script->blockFromLabel(stmt.parameters[0]))
		return NONS_NO_SUCH_BLOCK;
	this->loadgosub=stmt.parameters[0];
	trim_string(this->loadgosub);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_locate(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	float x,y;
	GET_COORDINATE(x,0,0);
	GET_COORDINATE(y,1,1);
	if (x<0 || y<0)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	this->screen->output->setPosition((int)x,(int)y);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_lookbackbutton(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(4);
	std::wstring A,B,C,D;
	GET_STR_VALUE(A,0);
	GET_STR_VALUE(B,1);
	GET_STR_VALUE(C,2);
	GET_STR_VALUE(D,3);
	NONS_AnimationInfo anim;
	anim.parse(A);
	A=L":l;";
	A.append(anim.getFilename());
	anim.parse(B);
	B=L":l;";
	B.append(anim.getFilename());
	anim.parse(C);
	C=L":l;";
	C.append(anim.getFilename());
	anim.parse(D);
	D=L":l;";
	D.append(anim.getFilename());
	bool ret=this->screen->lookback->setUpButtons(A,B,C,D);
	return ret?NONS_NO_ERROR:NONS_FILE_NOT_FOUND;
}

ErrorCode NONS_ScriptInterpreter::command_lookbackcolor(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long a;
	GET_INT_VALUE(a,0);
	SDL_Color col={Sint8((a&0xFF0000)>>16),(a&0xFF00)>>8,a&0xFF,0};
	this->screen->lookback->foreground=col;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_lookbackflush(NONS_Statement &stmt){
	this->screen->output->log.clear();
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_lsp(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(4);
	long spriten;
	float x,y;
	long alpha=255;
	std::wstring str;
	GET_INT_VALUE(spriten,0);
	GET_COORDINATE(x,0,2);
	GET_COORDINATE(y,1,3);
	if (stmt.parameters.size()>4)
		GET_INT_VALUE(alpha,4);
	GET_STR_VALUE(str,1);
	if (alpha>255)
		alpha=255;
	if (alpha<0)
		alpha=0;
	HANDLE_POSSIBLE_ERRORS(this->screen->loadSprite(spriten,str,(long)x,(long)y,(uchar)alpha,!stdStrCmpCI(stmt.commandName,L"lsp")));
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_maxkaisoupage(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long max;
	GET_INT_VALUE(max,0);
	if (max<=0)
		max=-1;
	this->screen->output->maxLogPages=max;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_menu_full(NONS_Statement &stmt){
	this->screen->screen->toggleFullscreen(!stdStrCmpCI(stmt.commandName,L"menu_full"));
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_menuselectcolor(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(3);
	long on,off,nofile;
	GET_INT_VALUE(on,0);
	GET_INT_VALUE(off,1);
	GET_INT_VALUE(nofile,2);
	SDL_Color coloron={Uint8((on&0xFF0000)>>16),(on&0xFF00)>>8,on&0xFF,0},
		coloroff={Uint8((off&0xFF0000)>>16),(off&0xFF00)>>8,off&0xFF,0},
		colornofile={Uint8((nofile&0xFF0000)>>16),(nofile&0xFF00)>>8,nofile&0xFF,0};
	this->menu->on=coloron;
	this->menu->off=coloroff;
	this->menu->nofile=colornofile;
	this->menu->reset();
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_menuselectvoice(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(7);
	std::wstring entry,
		cancel,
		mouse,
		click,
		yes,
		no;
	GET_STR_VALUE(entry,0);
	GET_STR_VALUE(cancel,1);
	GET_STR_VALUE(mouse,2);
	GET_STR_VALUE(click,3);
	GET_STR_VALUE(yes,5);
	GET_STR_VALUE(no,6);
	this->menu->voiceEntry=entry;
	this->menu->voiceCancel=cancel;
	this->menu->voiceMO=mouse;
	this->menu->voiceClick=click;
	this->menu->voiceYes=yes;
	this->menu->voiceNo=no;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_menusetwindow(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(7);
	float fontX,fontY,spacingX,spacingY;
	long shadow,hexcolor;
	GET_COORDINATE(fontX,0,0);
	GET_COORDINATE(fontY,1,1);
	GET_COORDINATE(spacingX,0,2);
	GET_COORDINATE(spacingY,1,3);
	GET_INT_VALUE(shadow,5);
	GET_INT_VALUE(hexcolor,6);
	SDL_Color color={
		Uint8((hexcolor&0xFF0000)>>16),
		(hexcolor&0xFF00)>>8,
		hexcolor&0xFF,
		0
	};
	this->menu->fontsize=(long)fontX;
	this->menu->lineskip=long(fontY+spacingY);
	this->menu->spacing=(long)spacingX;
	this->menu->shadow=!!shadow;
	this->menu->shadeColor=color;
	this->menu->reset();
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_mid(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(3);
	NONS_VariableMember *dst;
	GET_STR_VARIABLE(dst,0);
	long start,len;
	GET_INT_VALUE(start,2);
	std::wstring src;
	GET_STR_VALUE(src,1);
	len=src.size();
	if ((ulong)start>=src.size()){
		dst->set(L"");
		return NONS_NO_ERROR;
	}
	if (stmt.parameters.size()>3){
		GET_INT_VALUE(len,3);
	}
	if ((ulong)start+len>src.size())
		len=src.size()-start;
	dst->set(src.substr(start,len));
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_monocro(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long color;
	if (!stdStrCmpCI(stmt.parameters[0],L"off")){
		if (this->screen->filterPipeline.size() && this->screen->filterPipeline[0].effectNo==pipelineElement::MONOCHROME)
			this->screen->filterPipeline.erase(this->screen->filterPipeline.begin());
		else if (this->screen->filterPipeline.size()>1 && this->screen->filterPipeline[1].effectNo==pipelineElement::MONOCHROME)
			this->screen->filterPipeline.erase(this->screen->filterPipeline.begin()+1);
		return NONS_NO_ERROR;
	}
	GET_INT_VALUE(color,0);

	if (this->screen->filterPipeline.size()){
		if (this->screen->apply_monochrome_first && this->screen->filterPipeline[0].effectNo==pipelineElement::NEGATIVE){
			this->screen->filterPipeline.push_back(this->screen->filterPipeline[0]);
			this->screen->filterPipeline[0].effectNo=pipelineElement::MONOCHROME;
			this->screen->filterPipeline[0].color=rgb2SDLcolor(color);
		}else if (this->screen->filterPipeline[0].effectNo==pipelineElement::MONOCHROME)
			this->screen->filterPipeline[0].color=rgb2SDLcolor(color);
		else
			this->screen->filterPipeline.push_back(pipelineElement(pipelineElement::MONOCHROME,rgb2SDLcolor(color),L"",0));
	}else
		this->screen->filterPipeline.push_back(pipelineElement(pipelineElement::MONOCHROME,rgb2SDLcolor(color),L"",0));
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_mov(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	NONS_VariableMember *var;
	GET_VARIABLE(var,0);
	if (var->getType()==INTEGER){
		long val;
		GET_INT_VALUE(val,1);
		var->set(val);
	}else{
		std::wstring val;
		GET_STR_VALUE(val,1);
		var->set(val);
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_movl(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	NONS_VariableMember *dst;
	ErrorCode error;
	dst=this->store->retrieve(stmt.parameters[0],&error);
	if (!CHECK_FLAG(error,NONS_NO_ERROR_FLAG))
		return error;
	if (dst->getType()!=INTEGER_ARRAY)
		return NONS_EXPECTED_ARRAY;
	if (stmt.parameters.size()-1>dst->dimensionSize)
		handleErrors(NONS_TOO_MANY_PARAMETERS,stmt.lineOfOrigin->lineNumber,"NONS_ScriptInterpreter::command_movl",1);
	for (ulong a=0;a<dst->dimensionSize && a<stmt.parameters.size()-1;a++){
		long temp;
		GET_INT_VALUE(temp,a+1);
		dst->dimension[a]->set(temp);
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_movN(NONS_Statement &stmt){
	ulong functionVersion=atoi(stmt.commandName.substr(3));
	MINIMUM_PARAMETERS(functionVersion+1);
	NONS_VariableMember *first;
	GET_VARIABLE(first,0);
	Sint32 index=this->store->getVariableIndex(first);
	if (Sint32(index+functionVersion)>NONS_VariableStore::indexUpperLimit)
		return NONS_NOT_ENOUGH_VARIABLE_INDICES;
	std::vector<long> intvalues;
	std::vector<std::wstring> strvalues;
	for (ulong a=0;a<functionVersion;a++){
		if (first->getType()==INTEGER){
			long val;
			GET_INT_VALUE(val,a+1);
			intvalues.push_back(val);
		}else{
			std::wstring val;
			GET_STR_VALUE(val,a+1);
			strvalues.push_back(val);
		}
	}
	for (ulong a=0;a<functionVersion;a++){
		if (first->getType()==INTEGER){
			NONS_Variable *var=this->store->retrieve(index+a,0);
			var->intValue->set(intvalues[a]);
		}else{
			NONS_Variable *var=this->store->retrieve(index+a,0);
			var->wcsValue->set(strvalues[a]);
		}
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_mp3fadeout(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long ms;
	GET_INT_VALUE(ms,0);
	if (ms<25){
		this->audio->stopMusic();
		return NONS_NO_ERROR;
	}
	float original_vol=(float)this->audio->musicVolume(-1);
	float advance=original_vol/(float(ms)/25.0f);
	float current_vol=original_vol;
	while (current_vol>0){
		SDL_Delay(25);
		current_vol-=advance;
		if (current_vol<0)
			current_vol=0;
		this->audio->musicVolume((int)current_vol);
	}
	HANDLE_POSSIBLE_ERRORS(this->audio->stopMusic());
	this->audio->musicVolume((int)original_vol);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_mp3vol(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long vol;
	GET_INT_VALUE(vol,0);
	this->audio->musicVolume(vol);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_msp(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(4);
	long spriten,alpha;
	float x,y;
	GET_INT_VALUE(spriten,0);
	GET_COORDINATE(x,0,1);
	GET_COORDINATE(y,1,2);
	GET_INT_VALUE(alpha,3);
	if (ulong(spriten)>this->screen->layerStack.size())
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	NONS_Layer *l=this->screen->layerStack[spriten];
	if (!l)
		return NONS_NO_SPRITE_LOADED_THERE;
	if (stdStrCmpCI(stmt.commandName,L"amsp")){
		l->position.x+=x;
		l->position.y+=y;
		if (long(l->alpha)+alpha>255)
			l->alpha=255;
		else if (long(l->alpha)+alpha<0)
			l->alpha=0;
		else
			l->alpha+=(uchar)alpha;
	}else{
		l->position.x=x;
		l->position.y=y;
		if (alpha>255)
			alpha=255;
		else if (alpha<0)
			alpha=0;
		l->alpha=(uchar)alpha;
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_nega(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long onoff;
	GET_INT_VALUE(onoff,0);
	std::vector<pipelineElement> &v=this->screen->filterPipeline;
	if (onoff){
		onoff=onoff==1;
		this->screen->apply_monochrome_first=!onoff;
		bool push=0;
		switch (v.size()){
			case 0:
				push=1;
				break;
			case 1:
				if (v[0].effectNo==pipelineElement::MONOCHROME){
					if (onoff){
						v.push_back(v[0]);
						v[0].effectNo=pipelineElement::NEGATIVE;
					}else
						push=1;
				}
				break;
			case 2:
				if (v.size() && v[0].effectNo==pipelineElement::NEGATIVE){
					if (!onoff){
						v.push_back(v[0]);
						v.erase(v.begin());
					}
				}else if (v.size()>1 && v[1].effectNo==pipelineElement::NEGATIVE){
					if (onoff){
						v.insert(v.begin(),v.back());
						v.pop_back();
					}
				}
		}
		if (push)
			v.push_back(pipelineElement(pipelineElement::NEGATIVE,SDL_Color(),L"",0));
	}else{
		if (v.size() && v[0].effectNo==pipelineElement::NEGATIVE)
			v.erase(v.begin());
		else if (v.size()>1 && v[1].effectNo==pipelineElement::NEGATIVE)
			v.erase(v.begin()+1);
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_new_set_window(NONS_Statement &stmt){
	this->legacy_set_window=0;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_next(NONS_Statement &stmt){
	if (this->callStack.empty())
		return NONS_EMPTY_CALL_STACK;
	NONS_StackElement *element=this->callStack.back();
	if (element->type!=StackFrameType::FOR_NEST)
		return NONS_UNEXPECTED_NEXT;
	element->var->add(element->step);
	if (element->step>0 && element->var->getInt()>element->to || element->step<0 && element->var->getInt()<element->to){
		delete element;
		this->callStack.pop_back();
		return NONS_NO_ERROR;
	}
	NONS_Statement *cstmt=this->thread->getCurrentStatement();
	element->end.line=cstmt->lineOfOrigin->lineNumber;
	element->end.statement=cstmt->statementNo;
	this->thread->gotoPair(element->returnTo.toPair());
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_nsa(NONS_Statement &stmt){
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_nsadir(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	std::wstring temp;
	GET_STR_VALUE(temp,0);
	this->nsadir=UniToUTF8(temp);
	tolower(this->nsadir);
	toforwardslash(this->nsadir);
	if (this->nsadir[this->nsadir.size()-1]!='/')
		this->nsadir.push_back('/');
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_play(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	std::wstring name;
	GET_STR_VALUE(name,0);
	ErrorCode error=NONS_UNDEFINED_ERROR;
	this->mp3_loop=0;
	this->mp3_save=0;
	if (!stdStrCmpCI(stmt.commandName,L"play") || !stdStrCmpCI(stmt.commandName,L"loopbgm"))
		this->mp3_loop=1;
	else if (!stdStrCmpCI(stmt.commandName,L"mp3save"))
		this->mp3_save=1;
	else if (!stdStrCmpCI(stmt.commandName,L"mp3loop") || !stdStrCmpCI(stmt.commandName,L"bgm")){
		this->mp3_loop=1;
		this->mp3_save=1;
	}
	if (name[0]=='*'){
		int track=atoi(name.substr(1));
		std::wstring temp=L"track";
		temp+=itoaw(track,2);
		error=this->audio->playMusic(&temp,this->mp3_loop?-1:0);
		if (error==NONS_NO_ERROR)
			this->saveGame->musicTrack=track;
		else
			this->saveGame->musicTrack=-1;
	}else if ((error=this->audio->playMusic(&name,this->mp3_loop?-1:0))==NONS_NO_ERROR)
		this->saveGame->music=name;
	else
		this->saveGame->music.clear();
	return error;
}

ErrorCode NONS_ScriptInterpreter::command_playstop(NONS_Statement &stmt){
	this->mp3_loop=0;
	this->mp3_save=0;
	return this->audio->stopMusic();
}

ErrorCode NONS_ScriptInterpreter::command_print(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long number,duration;
	ErrorCode ret;
	GET_INT_VALUE(number,0);
	if (stmt.parameters.size()>1){
		GET_INT_VALUE(duration,1);
		std::wstring rule;
		if (stmt.parameters.size()>2)
			GET_STR_VALUE(rule,2);
		this->screen->hideText();
		ret=this->screen->BlendNoCursor(number,duration,&rule);
	}else{
		this->screen->hideText();
		ret=this->screen->BlendNoCursor(number);
	}
	return ret;
}

void shake(SDL_Surface *dst,long amplitude,ulong duration){
	SDL_Rect srcrect=dst->clip_rect,
		dstrect=srcrect;
	SDL_Surface *copyDst=makeSurface(dst->w,dst->h,32);
	manualBlit(dst,0,copyDst,0);
	ulong start=SDL_GetTicks();
	SDL_Rect last=dstrect;
	while (SDL_GetTicks()-start<duration){
		SDL_FillRect(dst,&srcrect,0);
		do{
			dstrect.x=Sint16((rand()%2)?amplitude:-amplitude);
			dstrect.y=Sint16((rand()%2)?amplitude:-amplitude);
		}while (dstrect.x==last.x && dstrect.y==last.y);
		last=dstrect;
		manualBlit(copyDst,&srcrect,dst,&dstrect);
		SDL_UpdateRect(dst,0,0,0,0);
	}
	manualBlit(copyDst,0,dst,0);
	SDL_UpdateRect(dst,0,0,0,0);
	SDL_FreeSurface(copyDst);
}

ErrorCode NONS_ScriptInterpreter::command_quake(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	long amplitude;
	long duration;
	GET_INT_COORDINATE(amplitude,0,0);
	GET_INT_VALUE(duration,1);
	if (amplitude<0 || duration<0)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	amplitude*=2;
	amplitude=this->screen->screen->convertW(amplitude);
	NONS_MutexLocker ml(screenMutex);
	shake(this->screen->screen->screens[REAL],amplitude,duration);
	return 0;
}

ErrorCode NONS_ScriptInterpreter::command_repaint(NONS_Statement &stmt){
	if (this->screen)
		this->screen->BlendNoCursor(1);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_reset(NONS_Statement &stmt){
	this->uninit();
	this->init();
	this->screen->clear();
	delete this->gfx_store;
	this->gfx_store=new NONS_GFXstore();
	this->screen->gfx_store=this->gfx_store;
	this->audio->stopAllSound();
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_resettimer(NONS_Statement &stmt){
	this->timer=SDL_GetTicks();
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_return(NONS_Statement &stmt){
	if (this->callStack.empty())
		return NONS_EMPTY_CALL_STACK;
	NONS_StackElement *popped;
	do{
		popped=this->callStack.back();
		this->callStack.pop_back();
	}while (
		popped->type!=StackFrameType::SUBROUTINE_CALL &&
		popped->type!=StackFrameType::TEXTGOSUB_CALL &&
		popped->type!=StackFrameType::USERCMD_CALL);
	this->thread->gotoPair(popped->returnTo.toPair());
	if (popped->type==StackFrameType::TEXTGOSUB_CALL){
		this->Printer_support(popped->pages,0,0,0);
		delete popped;
		return NONS_NO_ERROR;
	}
	NONS_ScriptLine &line=popped->interpretAtReturn;
	ErrorCode ret=NONS_NO_ERROR_BUT_BREAK;
	for (ulong a=0;a<line.statements.size();a++){
		ErrorCode error=this->interpretString(*line.statements[a],stmt.lineOfOrigin,stmt.fileOffset);
		if (error==NONS_END)
			return NONS_END;
		if (error==NONS_GOSUB)
			this->callStack.back()->interpretAtReturn=NONS_ScriptLine(line,a+1);
		if (!CHECK_FLAG(error,NONS_NO_ERROR_FLAG)){
			handleErrors(error,-1,"NONS_ScriptInterpreter::command_if",1);
			ret=NONS_UNDEFINED_ERROR;
		}
		if (CHECK_FLAG(error,NONS_BREAK_WORTHY_ERROR))
			break;
	}
	return ret;
}

ErrorCode NONS_ScriptInterpreter::command_rmenu(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	if (stmt.parameters.size()%2)
		return NONS_INSUFFICIENT_PARAMETERS;
	std::vector<std::wstring> items;
	for (ulong a=0;a<stmt.parameters.size();a++){
		std::wstring s;
		GET_STR_VALUE(s,a);
		a++;
		items.push_back(s);
		items.push_back(stmt.parameters[a]);
	}
	this->menu->resetStrings(&items);
	this->menu->rightClickMode=1;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_rmode(NONS_Statement &stmt){
	if (!stdStrCmpCI(stmt.commandName,L"roff")){
		this->menu->rightClickMode=0;
		return NONS_NO_ERROR;
	}
	MINIMUM_PARAMETERS(1);
	long a;
	GET_INT_VALUE(a,0);
	if (!a)
		this->menu->rightClickMode=0;
	else
		this->menu->rightClickMode=1;
	return NONS_NO_ERROR;
}

/*
Behavior notes:
rnd %a,%n ;a=[0;n)
rnd2 %a,%min,%max ;a=[min;max]
*/
ErrorCode NONS_ScriptInterpreter::command_rnd(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	NONS_VariableMember *dst;
	GET_INT_VARIABLE(dst,0);
	long min=0,max;
	if (!stdStrCmpCI(stmt.commandName,L"rnd")){
		GET_INT_VALUE(max,1);
		max--;
	}else{
		MINIMUM_PARAMETERS(3);
		GET_INT_VALUE(max,2);
		GET_INT_VALUE(min,1);
	}
	//lower+int(double(upper-lower+1)*rand()/(RAND_MAX+1.0))
	dst->set(min+(rand()*(max-min))/RAND_MAX);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_savefileexist(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	NONS_VariableMember *dst;
	GET_INT_VARIABLE(dst,0);
	long file;
	GET_INT_VALUE(file,1);
	if (file<1)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	std::wstring path=save_directory+L"save"+itoaw(file)+L".dat";
	dst->set(NONS_File::file_exists(path));
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_savegame(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long file;
	GET_INT_VALUE(file,0);
	if (file<1)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	return this->save(file)?NONS_NO_ERROR:NONS_UNDEFINED_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_savename(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(3);
	std::wstring save,
		load,
		slot;
	GET_STR_VALUE(save,0);
	GET_STR_VALUE(load,1);
	GET_STR_VALUE(slot,2);
	this->menu->stringSave=save;
	this->menu->stringLoad=load;
	this->menu->stringSlot=slot;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_savenumber(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long n;
	GET_INT_VALUE(n,0);
	if (n<1 || n>20)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	this->menu->slots=(ushort)n;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_savescreenshot(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	std::wstring filename;
	GET_STR_VALUE(filename,0);
	if (!this->screenshot){
		NONS_MutexLocker ml(screenMutex);
		SDL_SaveBMP(this->screen->screen->screens[VIRTUAL],UniToUTF8(filename).c_str());
	}else{
		SDL_SaveBMP(this->screenshot,UniToUTF8(filename).c_str());
		if (!stdStrCmpCI(stmt.commandName,L"savescreenshot")){
			SDL_FreeSurface(this->screenshot);
			this->screenshot=0;
		}
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_savetime(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(5);
	NONS_VariableMember *month,*day,*hour,*minute;
	GET_INT_VARIABLE(month,1);
	GET_INT_VARIABLE(day,2);
	GET_INT_VARIABLE(hour,3);
	GET_INT_VARIABLE(minute,4);
	long file;
	GET_INT_VALUE(file,0);
	if (file<1)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	std::wstring path=save_directory+L"save"+itoaw(file)+L".dat";
	if (!NONS_File::file_exists(path)){
		day->set(0);
		month->set(0);
		hour->set(0);
		minute->set(0);
	}else{
		tm *date=getDate(path);
		day->set(date->tm_mon+1);
		month->set(date->tm_mday);
		hour->set(date->tm_hour);
		minute->set(date->tm_min);
		delete date;
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_savetime2(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(7);
	NONS_VariableMember *year,*month,*day,*hour,*minute,*second;
	GET_INT_VARIABLE(year,1);
	GET_INT_VARIABLE(month,2);
	GET_INT_VARIABLE(day,3);
	GET_INT_VARIABLE(hour,4);
	GET_INT_VARIABLE(minute,5);
	GET_INT_VARIABLE(second,6);
	long file;
	GET_INT_VALUE(file,0);
		if (file<1)
			return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	std::wstring path=save_directory+L"save"+itoaw(file)+L".dat";
	if (!NONS_File::file_exists(path)){
		year->set(0);
		month->set(0);
		day->set(0);
		hour->set(0);
		minute->set(0);
		second->set(0);
	}else{
		tm *date=getDate(path.c_str());
		year->set(date->tm_year+1900);
		month->set(date->tm_mday);
		day->set(date->tm_mon+1);
		hour->set(date->tm_hour);
		minute->set(date->tm_min);
		second->set(date->tm_sec);
		delete date;
	}
	return NONS_NO_ERROR;
}

extern std::ofstream textDumpFile;

ErrorCode NONS_ScriptInterpreter::command_select(NONS_Statement &stmt){
	bool selnum;
	if (!stdStrCmpCI(stmt.commandName,L"selnum")){
		MINIMUM_PARAMETERS(2);
		selnum=1;
	}else{
		MINIMUM_PARAMETERS(3);
		if (stmt.parameters.size()%2)
			return NONS_INSUFFICIENT_PARAMETERS;
		selnum=0;
	}
	NONS_VariableMember *var=0;
	if (selnum)
		GET_INT_VARIABLE(var,0);
	std::vector<std::wstring> strings,jumps;
	for (ulong a=selnum;a<stmt.parameters.size();a++){
		std::wstring temp;
		GET_STR_VALUE(temp,a);
		strings.push_back(temp);
		if (!selnum){
			a++;
			GET_LABEL(temp,a);
			jumps.push_back(temp);
		}
	}
	NONS_ButtonLayer layer(*this->font_cache,this->screen,0,this->menu);
	layer.makeTextButtons(
		strings,
		this->selectOn,
		this->selectOff,
		!!this->screen->output->shadowLayer,
		&this->selectVoiceEntry,
		&this->selectVoiceMouseOver,
		&this->selectVoiceClick,
		this->audio,
		this->screen->output->w,
		this->screen->output->h);
	ctrlIsPressed=0;
	//softwareCtrlIsPressed=0;
	this->screen->showText();
	int choice=layer.getUserInput(this->screen->output->x,this->screen->output->y);
	if (choice==-2){
		this->screen->clearText();
		choice=layer.getUserInput(this->screen->output->x,this->screen->output->y);
		if (choice==-2)
			return NONS_SELECT_TOO_BIG;
	}
	if (choice==INT_MIN)
		return NONS_END;
	if (choice==-3)
		return NONS_NO_ERROR;
	this->screen->clearText();
	if (textDumpFile.is_open())
		textDumpFile <<"    "<<UniToUTF8(strings[choice])<<std::endl;
	if (selnum)
		var->set(choice);
	else{
		if (!this->script->blockFromLabel(jumps[choice]))
			return NONS_NO_SUCH_BLOCK;
		if (!stdStrCmpCI(stmt.commandName,L"selgosub")){
			NONS_StackElement *p=new NONS_StackElement(this->thread->getNextStatementPair(),NONS_ScriptLine(),0,this->insideTextgosub());
			this->callStack.push_back(p);
		}
		this->thread->gotoLabel(jumps[choice]);
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_selectcolor(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	long on,off;
	GET_INT_VALUE(on,0);
	GET_INT_VALUE(off,1);
	this->selectOn.r=Uint8((on&0xFF0000)>>16);
	this->selectOn.g=(on&0xFF00)>>8;
	this->selectOn.b=on&0xFF;
	this->selectOff.r=Uint8((off&0xFF0000)>>16);
	this->selectOff.g=(off&0xFF00)>>8;
	this->selectOff.b=off&0xFF;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_selectvoice(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(3);
	std::wstring strings[3];
	for (int a=0;a<3;a++){
		GET_STR_VALUE(strings[a],a);
		tolower(strings[a]);
		if (strings[a].size() && !general_archive.exists(strings[a]))
			return NONS_FILE_NOT_FOUND;
	}
	this->selectVoiceEntry=strings[0];
	this->selectVoiceMouseOver=strings[1];
	this->selectVoiceClick=strings[2];
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_set_default_font_size(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long a;
	GET_INT_VALUE(a,0);
	this->defaultfs=a;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_setcursor(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(4);
	long which;
	float x,y;
	std::wstring string;
	GET_INT_VALUE(which,0);
	GET_COORDINATE(x,0,2);
	GET_COORDINATE(y,1,3);
	GET_STR_VALUE(string,1);
	bool absolute=stdStrCmpCI(stmt.commandName,L"abssetcursor")==0;
	NONS_Cursor *new_cursor=new NONS_Cursor(string,(long)x,(long)y,absolute,this->screen),
		**dst=&((!which)?this->arrowCursor:this->pageCursor);
	if (*dst)
		delete *dst;
	*dst=new_cursor;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_setwindow(NONS_Statement &stmt){
	float frameXstart,
		frameYstart;
	long frameXend,
		frameYend;
	float fontsize,
		spacingX,
		spacingY;
	long speed,
		bold,
		shadow,
		color;
	float windowXstart,
		windowYstart,
		windowXend,
		windowYend;
	std::wstring filename;
	bool syntax=0;
	int forceLineSkip=0;
	if (this->legacy_set_window){
		float fontsizeY;
		MINIMUM_PARAMETERS(14);
		GET_COORDINATE(frameXstart,	0,0);
		GET_COORDINATE(frameYstart,	1,1);
		GET_INT_VALUE(frameXend,2);
		GET_INT_VALUE(frameYend,3);
		GET_COORDINATE(fontsize,	0,4);
		GET_COORDINATE(fontsizeY,	1,5);
		GET_COORDINATE(spacingX,	0,6);
		GET_COORDINATE(spacingY,	1,7);
		GET_INT_VALUE(speed,8);
		GET_INT_VALUE(bold,9);
		GET_INT_VALUE(shadow,10);
		GET_COORDINATE(windowXstart,0,12);
		GET_COORDINATE(windowYstart,1,13);
		if (this->store->getIntValue(stmt.parameters[11],color,0)!=NONS_NO_ERROR){
			syntax=1;
			GET_STR_VALUE(filename,11);
			windowXend=windowXstart+1;
			windowYend=windowYstart+1;
		}else{
			GET_COORDINATE(windowXend,0,14);
			GET_COORDINATE(windowYend,1,15);
		}
		frameXend*=long(fontsize+spacingX);
		frameXend+=(long)frameXstart;
		fontsize=float(this->defaultfs*this->virtual_size[1]/this->base_size[1]);
		forceLineSkip=int(fontsizeY+spacingY);
		frameYend*=long(fontsizeY+spacingY);
		frameYend+=(long)frameYstart;
	}else{
		MINIMUM_PARAMETERS(15);
		GET_COORDINATE(frameXstart	,0,0);
		GET_COORDINATE(frameYstart	,1,1);
		{
			float temp;
			GET_COORDINATE(temp,0,2);	
			frameXend=(long)temp;
		}
		{
			float temp;
			GET_COORDINATE(temp,1,3);
			frameYend=(long)temp;
		}
		GET_COORDINATE(fontsize		,0,4);
		GET_COORDINATE(spacingX		,0,5);
		GET_COORDINATE(spacingY		,1,6);
		GET_INT_VALUE(speed,7);
		GET_INT_VALUE(bold,8);
		GET_INT_VALUE(shadow,9);
		GET_COORDINATE(windowXstart,0,11);
		GET_COORDINATE(windowYstart,1,12);
		GET_COORDINATE(windowXend,0,13);
		GET_COORDINATE(windowYend,1,14);
		if (this->store->getIntValue(stmt.parameters[10],color,0)!=NONS_NO_ERROR){
			syntax=1;
			GET_STR_VALUE(filename,10);
		}
	}
	bold=0;
	if (windowXstart<0 || windowXend<0 || windowXstart<0 || windowYend<0 ||
			frameXstart<0 || frameXend<0 || frameXstart<0 || frameYend<0 ||
			windowXstart>=windowXend ||
			windowYstart>=windowYend ||
			frameXstart>=frameXend ||
			frameYstart>=frameYend ||
			fontsize<1){
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	}
	NONS_Rect windowRect={
		windowXstart,
		windowYstart,
		windowXend-windowXstart+1,
		windowYend-windowYstart+1
	},
	frameRect={
		frameXstart,
		frameYstart,
		frameXend-frameXstart,
		frameYend-frameYstart
	};
	{
		NONS_MutexLocker ml(screenMutex);
		SDL_Surface *scr=this->screen->screen->screens[VIRTUAL];
		if (frameRect.x+frameRect.w>scr->w || frameRect.y+frameRect.h>scr->h)
			o_stderr <<"Warning: The text frame is larger than the screen\n";
		if (this->screen->output->shadeLayer->useDataAsDefaultShade){
			ImageLoader.unfetchImage(this->screen->output->shadeLayer->data);
			this->screen->output->shadeLayer->data=0;
		}
		{
			NONS_FontCache *fc[]={
				this->font_cache,
				this->screen->output->foregroundLayer->fontCache,
				this->screen->output->shadowLayer->fontCache
			};
			for (int a=0;a<3;a++){
				fc[a]->set_size((ulong)fontsize);
				fc[a]->spacing=(long)spacingX;
				if (forceLineSkip)
					fc[a]->line_skip=forceLineSkip;
			}
		}
		SDL_Surface *pic;
		if (!syntax){
			this->screen->resetParameters(&windowRect.to_SDL_Rect(),&frameRect.to_SDL_Rect(),*this->font_cache,shadow!=0);
			this->screen->output->shadeLayer->setShade(uchar((color&0xFF0000)>>16),(color&0xFF00)>>8,color&0xFF);
			this->screen->output->shadeLayer->Clear();
		}else{
			ImageLoader.fetchSprite(pic,filename);
			windowRect.w=(float)pic->w;
			windowRect.h=(float)pic->h;
			this->screen->resetParameters(&windowRect.to_SDL_Rect(),&frameRect.to_SDL_Rect(),*this->font_cache,shadow!=0);
			this->screen->output->shadeLayer->usePicAsDefaultShade(pic);
		}
	}
	this->screen->output->extraAdvance=(int)spacingX;
	//this->screen->output->extraLineSkip=0;
	this->default_speed_slow=speed*2;
	this->default_speed_med=speed;
	this->default_speed_fast=speed/2;
	switch (this->current_speed_setting){
		case 0:
			this->default_speed=this->default_speed_slow;
			break;
		case 1:
			this->default_speed=this->default_speed_med;
			break;
		case 2:
			this->default_speed=this->default_speed_fast;
			break;
	}
	this->screen->output->display_speed=this->default_speed;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_shadedistance(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	long x,y;
	GET_INT_COORDINATE(x,0,0);
	GET_INT_COORDINATE(y,1,1);
	this->screen->output->shadowPosX=x;
	this->screen->output->shadowPosY=y;
	return NONS_NO_ERROR;
}

void quake(SDL_Surface *dst,char axis,ulong amplitude,ulong duration){
	float length=(float)duration,
		amp=(float)amplitude;
	SDL_Rect srcrect=dst->clip_rect,
		dstrect=srcrect;
	SDL_Surface *copyDst=makeSurface(dst->w,dst->h,32);
	manualBlit(dst,0,copyDst,0);
	ulong start=SDL_GetTicks();
	while (1){
		float x=float(SDL_GetTicks()-start);
		if (x>duration)
			break;
		float y=(float)sin(x*(20/length)*M_PI)*((amp/-length)*x+amplitude);
		SDL_FillRect(dst,&srcrect,0);
		if (axis=='x')
			dstrect.x=(Sint16)y;
		else
			dstrect.y=(Sint16)y;
		manualBlit(copyDst,&srcrect,dst,&dstrect);
		SDL_UpdateRect(dst,0,0,0,0);
	}
	SDL_FreeSurface(copyDst);
}

ErrorCode NONS_ScriptInterpreter::command_sinusoidal_quake(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	long amplitude;
	long duration;
	GET_INT_COORDINATE(amplitude,0,0);
	GET_INT_VALUE(duration,1);
	if (amplitude<0 || duration<0)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	amplitude*=10;
	if (stmt.commandName[5]=='x')
		amplitude=this->screen->screen->convertW(amplitude);
	else
		amplitude=this->screen->screen->convertH(amplitude);
	NONS_MutexLocker ml(screenMutex);
	quake(this->screen->screen->screens[REAL],(char)stmt.commandName[5],amplitude,duration);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_skip(NONS_Statement &stmt){
	long count=2;
	if (stmt.parameters.size())
		GET_INT_VALUE(count,0);
	if (!count && !this->thread->getCurrentStatement()->statementNo)
		return NONS_ZERO_VALUE_IN_SKIP;
	if (!this->thread->skip(count))
		return NONS_NOT_ENOUGH_LINES_TO_SKIP;
	return NONS_NO_ERROR_BUT_BREAK;
}

ErrorCode NONS_ScriptInterpreter::command_split(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	std::wstring srcStr,
		searchStr;
	GET_STR_VALUE(srcStr,0);
	GET_STR_VALUE(searchStr,1);
	std::vector <NONS_VariableMember *> dsts;
	for (ulong a=2;a<stmt.parameters.size();a++){
		NONS_VariableMember *var;
		GET_VARIABLE(var,a);
		dsts.push_back(var);
	}
	ulong middle=0;
	for (ulong a=0;a<dsts.size();a++){
		ulong next=srcStr.find(searchStr,middle);
		bool _break=(next==srcStr.npos);
		std::wstring copy=(!_break)?srcStr.substr(middle,next-middle):srcStr.substr(middle);
		if (dsts[a]->getType()==INTEGER)
			dsts[a]->set(atoi(copy));
		else
			dsts[a]->set(copy);
		if (_break)
			break;
		middle=next+searchStr.size();
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_stdout(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	std::wstring text=this->convertParametersToString(stmt);
	if (!stdStrCmpCI(stmt.stmt,L"stdout"))
		o_stdout <<text<<"\n";
	else //if (!stdStrCmpCI(stmt.stmt,L"stderr"))
		o_stderr <<text<<"\n";
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_stop(NONS_Statement &stmt){
	this->mp3_loop=0;
	this->mp3_save=0;
	this->wav_loop=0;
	return this->audio->stopAllSound();
}

ErrorCode NONS_ScriptInterpreter::command_systemcall(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	if (!stdStrCmpCI(stmt.parameters[0],L"rmenu") && this->menu->callMenu()==INT_MIN)
		return NONS_END;
	else
		this->menu->call(stmt.parameters[0]);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_tablegoto(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	long val;
	GET_INT_VALUE(val,0);
	if (val<0)
		return NONS_NEGATIVE_GOTO_INDEX;
	std::vector<std::wstring> labels(stmt.parameters.size()-1);
	for (ulong a=1;a<stmt.parameters.size();a++)
		GET_LABEL(labels[a-1],a);
	if ((ulong)val>labels.size())
		return NONS_NOT_ENOUGH_LABELS;
	if (!this->script->blockFromLabel(labels[val]))
		return NONS_NO_SUCH_BLOCK;
	this->goto_label(labels[val]);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_tal(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	long newalpha;
	GET_INT_VALUE(newalpha,1);
	switch (stmt.parameters[0][0]){
		case 'l':
			if (this->hideTextDuringEffect)
				this->screen->hideText();
			this->screen->leftChar->alpha=(uchar)newalpha;
			break;
		case 'r':
			if (this->hideTextDuringEffect)
				this->screen->hideText();
			this->screen->rightChar->alpha=(uchar)newalpha;
			break;
		case 'c':
			if (this->hideTextDuringEffect)
				this->screen->hideText();
			this->screen->centerChar->alpha=(uchar)newalpha;
			break;
		case 'a':
			if (this->hideTextDuringEffect)
				this->screen->hideText();
			this->screen->leftChar->alpha=(uchar)newalpha;
			this->screen->rightChar->alpha=(uchar)newalpha;
			this->screen->centerChar->alpha=(uchar)newalpha;
			break;
		default:
			return NONS_INVALID_PARAMETER;
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_textclear(NONS_Statement &stmt){
	if (this->screen)
		this->screen->clearText();
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_textgosub(NONS_Statement &stmt){
	if (!stmt.parameters.size())
		return NONS_NO_ERROR;
	std::wstring label;
	GET_LABEL(label,0);
	if (!this->script->blockFromLabel(label))
		return NONS_NO_SUCH_BLOCK;
	if (stmt.parameters.size()<2)
		this->textgosubRecurses=0;
	else{
		long rec;
		GET_INT_VALUE(rec,1);
		this->textgosubRecurses=(rec!=0);
	}
	this->textgosub=label;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_textonoff(NONS_Statement &stmt){
	if (!stdStrCmpCI(stmt.commandName,L"texton"))
		this->screen->showText();
	else
		this->screen->hideText();
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_textspeed(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long speed;
	GET_INT_VALUE(speed,0);
	if (speed<0)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	this->screen->output->display_speed=speed;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_time(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(3);
	NONS_VariableMember *h,*m,*s;
	GET_INT_VARIABLE(h,0);
	GET_INT_VARIABLE(m,1);
	GET_INT_VARIABLE(s,2);
	time_t t=time(0);
	tm *time=localtime(&t);
	h->set(time->tm_hour);
	m->set(time->tm_min);
	s->set(time->tm_sec);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_transmode(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	std::wstring param=stmt.parameters[0];
	tolower(param);
	if (param==L"leftup"){
		NONS_AnimationInfo::default_trans=NONS_AnimationInfo::LEFT_UP;
	}else if (param==L"copy"){
		NONS_AnimationInfo::default_trans=NONS_AnimationInfo::COPY_TRANS;
	}else if (param==L"alpha"){
		NONS_AnimationInfo::default_trans=NONS_AnimationInfo::PARALLEL_MASK;
	}else
		return NONS_INVALID_PARAMETER;
	return NONS_NO_ERROR;
}

uchar trapFlag=0;

ErrorCode NONS_ScriptInterpreter::command_trap(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	if (!stdStrCmpCI(stmt.parameters[0],L"off")){
		if (!trapFlag)
			return NONS_NO_TRAP_SET;
		trapFlag=0;
		this->trapLabel.clear();
		return NONS_NO_ERROR;
	}
	long kind;
	if (!stdStrCmpCI(stmt.commandName,L"trap"))
		kind=1;
	else if (!stdStrCmpCI(stmt.commandName,L"lr_trap"))
		kind=2;
	else if (!stdStrCmpCI(stmt.commandName,L"trap2"))
		kind=3;
	else
		kind=4;
	std::wstring label;
	GET_LABEL(label,0);
	if (!this->script->blockFromLabel(label))
		return NONS_NO_SUCH_BLOCK;
	this->trapLabel=label;
	trapFlag=(uchar)kind;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_unalias(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	std::wstring name=stmt.parameters[0];
	constants_map_T::iterator i=this->store->constants.find(name);
	if (i==this->store->constants.end())
		return NONS_UNDEFINED_CONSTANT;
	delete i->second;
	this->store->constants.erase(i);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_underline(NONS_Statement &stmt){
	if (!stmt.parameters.size())
		return 0;
	long a;
	GET_INT_COORDINATE(a,1,0);
	this->screen->char_baseline=a;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_undocumented(NONS_Statement &stmt){
	return NONS_UNDOCUMENTED_COMMAND;
}

ErrorCode NONS_ScriptInterpreter::command_unimplemented(NONS_Statement &stmt){
	return NONS_UNIMPLEMENTED_COMMAND;
}

ErrorCode NONS_ScriptInterpreter::command_use_new_if(NONS_Statement &stmt){
	this->new_if=1;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_use_nice_svg(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long a;
	GET_INT_VALUE(a,0);
	ImageLoader.fast_svg=!a;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_useescspc(NONS_Statement &stmt){
	this->useEscapeSpace=1;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_usewheel(NONS_Statement &stmt){
	this->useWheel=1;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_versionstr(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	std::wstring str1,str2;
	GET_STR_VALUE(str1,0);
	GET_STR_VALUE(str2,1);
	o_stdout <<"-------------------------------------------------------------------------------\n"
		"versionstr says:\n"
		<<str1<<"\n"
		<<str2<<"\n"
		"-------------------------------------------------------------------------------\n";
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_vsp(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(2);
	long n=-1,visibility;
	GET_INT_VALUE(n,0);
	GET_INT_VALUE(visibility,1);
	if (n>0 && ulong(n)>=this->screen->layerStack.size())
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	if (this->screen->layerStack[n] && this->screen->layerStack[n]->data)
		this->screen->layerStack[n]->visible=!!visibility;
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_wait(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long ms;
	GET_INT_VALUE(ms,0);
	waitNonCancellable(ms);
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_waittimer(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long ms;
	GET_INT_VALUE(ms,0);
	if (ms<0)
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	ulong now=SDL_GetTicks();
	if (ulong(ms)>now-this->timer){
		long delay=ms-(now-this->timer);
		while (delay>0 && !forceSkip){
			SDL_Delay(10);
			delay-=10;
		}
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScriptInterpreter::command_wave(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	std::wstring name;
	GET_STR_VALUE(name,0);
	tolower(name);
	toforwardslash(name);
	ErrorCode error;
	this->wav_loop=!!stdStrCmpCI(stmt.commandName,L"wave");
	error=this->audio->playSoundAsync(&name,0,this->wav_loop?-1:0);
	return error;
}

ErrorCode NONS_ScriptInterpreter::command_wavestop(NONS_Statement &stmt){
	this->wav_loop=0;
	return this->audio->stopSoundAsync(0);
}

ErrorCode NONS_ScriptInterpreter::command_windoweffect(NONS_Statement &stmt){
	MINIMUM_PARAMETERS(1);
	long number,duration;
	std::wstring rule;
	GET_INT_VALUE(number,0);
	if (stmt.parameters.size()>1){
		GET_INT_VALUE(duration,1);
		if (stmt.parameters.size()>2)
			GET_STR_VALUE(rule,2);
		if (!this->screen->output->transition->stored)
			delete this->screen->output->transition;
		this->screen->output->transition=new NONS_GFX(number,duration,&rule);
	}else{
		NONS_GFX *effect=this->gfx_store->retrieve(number);
		if (!effect)
			return NONS_UNDEFINED_EFFECT;
		if (!this->screen->output->transition->stored)
			delete this->screen->output->transition;
		this->screen->output->transition=effect;
	}
	return NONS_NO_ERROR;
}
