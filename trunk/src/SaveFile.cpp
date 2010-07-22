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

#include "SaveFile.h"
#include "IOFunctions.h"
#include "CommandLineOptions.h"
#include "VirtualScreen.h"

#if NONS_SYS_WINDOWS
#include <windows.h>
#elif NONS_SYS_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

std::wstring save_directory;
std::wstring config_directory;
const wchar_t *settings_filename=L"settings.cfg";

tm *getDate(const std::wstring &filename){
	tm *res=new tm();
#if NONS_SYS_WINDOWS
	FILETIME time;
	SYSTEMTIME time2;
#ifdef UNICODE
	HANDLE h=CreateFile(filename.c_str(),FILE_READ_DATA,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
#else
	HANDLE h=CreateFile(UniToISO88591(filename).c_str(),FILE_READ_DATA,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
#endif
	GetFileTime(h,0,0,&time);
	CloseHandle(h);
	FileTimeToSystemTime((const FILETIME *)&time,&time2);
	SystemTimeToTzSpecificLocalTime(0,&time2,&time2);
	res->tm_year=time2.wYear-1900;
	res->tm_mon=time2.wMonth-1;
	res->tm_mday=time2.wDay;
	res->tm_hour=time2.wHour;
	res->tm_min=time2.wMinute;
	res->tm_sec=time2.wSecond;
#elif NONS_SYS_UNIX
	struct stat buf;
	stat(UniToUTF8(filename).c_str(),&buf);
	*res=*localtime(&(buf.st_mtime));
#else
	res->tm_year=2000;
	res->tm_mon=0;
	res->tm_mday=1;
	res->tm_hour=0;
	res->tm_min=0;
	res->tm_sec=0;
#endif
	return res;
}

std::vector<tm *> existing_files(const std::wstring &location){
	std::vector<tm *> res;
	res.reserve(20);
	std::wstring path=location;
	toforwardslash(path);
	if (path[path.size()-1]!='/')
		path.push_back('/');
	for (short a=1;a<21;a++){
		std::wstring filename=path+L"save"+itoaw(a)+L".dat";
		if (!NONS_File::file_exists(filename))
			res.push_back(0);
		else
			res.push_back(getDate(filename));
	}
	return res;
}

#if NONS_SYS_WINDOWS
#ifndef UNICODE
#define UNICODE
#include <windows.h>
#endif

DECLARE_ENUM(WINDOWS_VERSION)
	ERR=0,
	//9x kernel
	V95=1,
	V98=2,
	VME=3,
	//NT kernel
	V2K=4,
	VXP=5,
	VVI=6,
	VW7=7
DECLARE_ENUM_CLOSE;

WINDOWS_VERSION::WINDOWS_VERSION getWindowsVersion(){
	//First try with the 9x kernel
	HKEY k;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\0"),0,KEY_READ,&k)!=ERROR_SUCCESS)
		return WINDOWS_VERSION::ERR;
	DWORD type,size;
	WINDOWS_VERSION::WINDOWS_VERSION ret;
	if (RegQueryValueEx(k,TEXT("Version"),0,&type,0,&size)!=ERROR_SUCCESS || type!=REG_SZ){
		//Not the 9x kernel
		RegCloseKey(k);
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),0,KEY_READ,&k)!=ERROR_SUCCESS)
			return WINDOWS_VERSION::ERR;
		if (RegQueryValueEx(k,TEXT("CurrentVersion"),0,&type,0,&size)!=ERROR_SUCCESS || type!=REG_SZ)
			return WINDOWS_VERSION::ERR;
		std::string str(size,0);
		RegQueryValueEx(k,TEXT("CurrentVersion"),0,&type,(LPBYTE)&str[0],&size);
		RegCloseKey(k);
		switch (str[0]){
			case '5':
				ret=WINDOWS_VERSION::VXP;
				break;
			case '6':
				ret=WINDOWS_VERSION::VVI;
				break;
			case '7':
				ret=WINDOWS_VERSION::VW7;
				break;
			default:
				ret=WINDOWS_VERSION::ERR;
		}
	}else{
		std::string str(size,0);
		RegQueryValueEx(k,TEXT("VersionNumber"),0,&type,(LPBYTE)&str[0],&size);
		RegCloseKey(k);
		switch (str[2]){
			case '0':
				ret=WINDOWS_VERSION::V95;
				break;
			case '1':
				ret=WINDOWS_VERSION::V98;
				break;
			case '9':
				ret=WINDOWS_VERSION::VME;
				break;
			default:
				ret=WINDOWS_VERSION::ERR;
		}
	}
	return ret;
}

#elif NONS_SYS_UNIX
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#endif

std::wstring getConfigLocation(){
#if NONS_SYS_WINDOWS
	if (getWindowsVersion()<WINDOWS_VERSION::V2K)
		return L"./";
	HKEY k;
	if (RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",0,KEY_READ,&k)!=ERROR_SUCCESS)
		return L"./";
	DWORD type,size;
	if (RegQueryValueEx(k,L"Personal",0,&type,0,&size)!=ERROR_SUCCESS || type!=REG_SZ){
		RegCloseKey(k);
		return L"./";
	}
	std::wstring path(size/sizeof(TCHAR),0);
	RegQueryValueEx(k,L"Personal",0,&type,(LPBYTE)&path[0],&size);
	RegCloseKey(k);
	path.resize(wcslen(path.c_str()));
	toforwardslash(path);
	if (path[path.size()-1]!='/')
		path.append(L"/.ONSlaught");
	else
		path.append(L".ONSlaught");
	if (!CreateDirectory((LPCTSTR)path.c_str(),0) && GetLastError()!=ERROR_ALREADY_EXISTS){
		return L"./";
	}
	path.push_back('/');
	return path;
#elif NONS_SYS_UNIX
	passwd* pwd=getpwuid(getuid());
	if (!pwd)
		return L"./";
	std::string res=pwd->pw_dir;
	if (res[res.size()-1]!='/')
		res.append("/.ONSlaught");
	else
		res.append(".ONSlaught");
	if (mkdir(res.c_str(),~0) && errno!=EEXIST){
		return L"./";
	}
	res.push_back('/');
	return UniFromUTF8(res);
#else
	return L"./";
#endif
}

std::wstring getSaveLocation(unsigned hash[5]){
#if NONS_SYS_WINDOWS
	if (getWindowsVersion()<WINDOWS_VERSION::V2K)
		return L"./";
#endif
	std::wstring root=config_directory;
#if NONS_SYS_WINDOWS
#ifdef UNICODE
	std::wstring path=root;
#else
	std::string path=UniToISO88591(root);
#endif
#elif NONS_SYS_UNIX
	std::wstring path=root;
#else
	return root;
#endif
#if NONS_SYS_WINDOWS || NONS_SYS_UNIX
	if (!CLOptions.savedir.size()){
		path.append(itohexw(hash[0],8));
		path.push_back(' ');
		path.append(itohexw(hash[1],8));
	}else
		path.append(CLOptions.savedir);
#endif
#if NONS_SYS_WINDOWS
	if (!CreateDirectory((LPCTSTR)path.c_str(),0) && GetLastError()!=ERROR_ALREADY_EXISTS)
		return root;
	path.push_back('/');
	return path;
#elif NONS_SYS_UNIX
	if (mkdir(UniToUTF8(path).c_str(),~0) && errno!=EEXIST)
		return root;
	path.push_back('/');
	return path;
#endif
}

NONS_VariableMember *readArray(void *buffer,ulong &offset){
	NONS_VariableMember *var;
	Uint32 dim=readDWord(buffer,offset);
	if (dim){
		var=new NONS_VariableMember(INTEGER_ARRAY);
		var->dimensionSize=dim;
		var->dimension=new NONS_VariableMember*[dim];
		for (ulong a=0;a<dim;a++)
			var->dimension[a]=readArray(buffer,offset);
	}else{
		var=new NONS_VariableMember(INTEGER);
		var->set(readSignedDWord(buffer,offset));
	}
	return var;
}

void NONS_SaveFile::load(std::wstring filename){
	size_t l;
	uchar *buffer=NONS_File::read(filename.c_str(),l);
	if (!buffer)
		return;
	ulong offset=0;
	bool compressed=firstcharsCI((char *)buffer,"BZh");
	bool nons_file=compressed || firstcharsCI((char *)buffer,"NONS");
	if (nons_file){
		this->error=NONS_NO_ERROR;
		this->format='N';
		if (compressed){
			uchar *temp=decompressBuffer_BZ2(buffer,l,l);
			delete[] buffer;
			buffer=temp;
		}
		offset+=5;
		this->version=readWord(buffer,offset);
		for (ulong a=0;a<5;a++)
			this->hash[a]=readDWord(buffer,offset);
		ulong header[4];
		for (ulong a=0;a<4;a++)
			header[a]=readDWord(buffer,offset);
		//stack
		offset=*header;
		{
			ulong n=readDWord(buffer,offset);
			for (ulong a=0;a<n;a++){
				stackEl *el=new stackEl();
				el->type=(StackFrameType::StackFrameType)readByte(buffer,offset);
				el->label=UniFromUTF8(readString(buffer,offset));
				if (this->version<2)
					el->offset_deprecated=readDWord(buffer,offset);
				else{
					el->linesBelow=readDWord(buffer,offset);
					el->statementNo=readDWord(buffer,offset);
					el->textgosubLevel=readDWord(buffer,offset);
				}
				switch (el->type){
					case StackFrameType::SUBROUTINE_CALL:
						el->leftovers=UniFromUTF8(readString(buffer,offset));
						break;
					case StackFrameType::FOR_NEST:
						el->variable=readDWord(buffer,offset);
						el->to=readSignedDWord(buffer,offset);
						el->step=readSignedDWord(buffer,offset);
						break;
					case StackFrameType::TEXTGOSUB_CALL:
						break;
					case StackFrameType::USERCMD_CALL:
						el->leftovers=UniFromUTF8(readString(buffer,offset));
						el->parameters.resize(readDWord(buffer,offset));
						for (ulong b=0;b<el->parameters.size();b++)
							el->parameters[b]=UniFromUTF8(readString(buffer,offset));
						break;

				}
				this->stack.push_back(el);
			}
			this->currentLabel=UniFromUTF8(readString(buffer,offset));
			if (this->version<2)
				this->currentOffset_deprecated=readDWord(buffer,offset);
			else{
				this->linesBelow=readDWord(buffer,offset);
				this->statementNo=readDWord(buffer,offset);
			}
			if (this->version>2)
				this->loadgosub=UniFromUTF8(readString(buffer,offset));
		}
		//variables
		offset=header[1];
		{
			ulong n=readDWord(buffer,offset);
			std::vector<Sint32> intervals;
			//read intervals----------------------------------------------------
			for (ulong a=0;a<n;a++){
				Uint32 b=readDWord(buffer,offset);
				if (b&0x80000000){
					//compensate for negatives
					if (b&0x40000000)
						b|=0x80000000;
					else
						b&=0x7FFFFFFF;
					intervals.push_back((Sint32)b);
					intervals.push_back(1);
				}else{
					if (b&0x40000000)
						b|=0x80000000;
					intervals.push_back((Sint32)b);
					intervals.push_back(readDWord(buffer,offset));
				}
			}
			//------------------------------------------------------------------
			for (ulong currentInterval=0;currentInterval<intervals.size();){
				ulong a=intervals[currentInterval],
					b=intervals[currentInterval+1];
				currentInterval+=2;
				for (ulong c=0;c<b;c++){
					NONS_Variable *var=new NONS_Variable;
					var->intValue->set(readSignedDWord(buffer,offset));
					var->wcsValue->set(UniFromUTF8(readString(buffer,offset)));
					this->variables[a++]=var;
				}
			}
			intervals.clear();
			n=readDWord(buffer,offset);
			//read intervals----------------------------------------------------
			for (ulong a=0;a<n;a++){
				Uint32 b=readDWord(buffer,offset);
				if (b&0x80000000){
					if (b&0x40000000)
						b|=0x80000000;
					else
						b&=0x7FFFFFFF;
					intervals.push_back((Sint32)b);
					intervals.push_back(1);
				}else{
					if (b&0x40000000)
						b|=0x80000000;
					intervals.push_back((Sint32)b);
					intervals.push_back(readDWord(buffer,offset));
				}
			}
			//------------------------------------------------------------------
			for (ulong currentInterval=0;currentInterval<intervals.size();){
				ulong a=intervals[currentInterval],
					b=intervals[currentInterval+1];
				currentInterval+=2;
				for (ulong c=0;c<b;c++){
					NONS_VariableMember *var=readArray(buffer,offset);
					this->arrays[a++]=var;
				}
			}
		}
		//screen
		offset=header[2];
		{
			ulong subheader[2];
			subheader[0]=readDWord(buffer,offset);
			subheader[1]=readDWord(buffer,offset);
			offset=*subheader;
			this->textWindow.x=readSignedWord(buffer,offset);
			this->textWindow.y=readSignedWord(buffer,offset);
			this->textWindow.w=readSignedWord(buffer,offset);
			this->textWindow.h=readSignedWord(buffer,offset);
			this->windowFrame.x=readSignedWord(buffer,offset);
			this->windowFrame.y=readSignedWord(buffer,offset);
			this->windowFrame.w=readSignedWord(buffer,offset);
			this->windowFrame.h=readSignedWord(buffer,offset);
			this->windowColor.r=readByte(buffer,offset);
			this->windowColor.g=readByte(buffer,offset);
			this->windowColor.b=readByte(buffer,offset);
			this->windowTransition=readDWord(buffer,offset);
			this->windowTransitionDuration=readDWord(buffer,offset);
			this->windowTransitionRule=UniFromUTF8(readString(buffer,offset));
			this->hideWindow=!!readByte(buffer,offset);
			this->fontSize=readWord(buffer,offset);
			this->windowTextColor.r=readByte(buffer,offset);
			this->windowTextColor.g=readByte(buffer,offset);
			this->windowTextColor.b=readByte(buffer,offset);
			{
				uchar style=readByte(buffer,offset);
				this->italic=(style&1)==1;
				this->bold=(style&2)==2;
			}
			this->textSpeed=readDWord(buffer,offset);
			this->fontShadow=!!readByte(buffer,offset);
			if (this->version>2){
				this->shadowPosX=readSignedDWord(buffer,offset);
				this->shadowPosY=readSignedDWord(buffer,offset);
			}
			this->spacing=readWord(buffer,offset);
			this->lineSkip=readSignedWord(buffer,offset);
			ulong n=readDWord(buffer,offset);
			for (ulong a=0;a<n;a++)
				this->logPages.push_back(UniFromUTF8(readString(buffer,offset)));
			this->currentBuffer=UniFromUTF8(readString(buffer,offset));
			if (this->version>2)
				this->indentationLevel=readDWord(buffer,offset);
			else
				this->indentationLevel=0;
			this->textX=readWord(buffer,offset);
			this->textY=readWord(buffer,offset);
			this->arrow.string=UniFromUTF8(readString(buffer,offset));
			this->arrow.absolute=!!readByte(buffer,offset);
			this->arrow.x=readSignedWord(buffer,offset);
			this->arrow.y=readSignedWord(buffer,offset);
			this->page.string=UniFromUTF8(readString(buffer,offset));
			this->page.absolute=!!readByte(buffer,offset);
			this->page.x=readSignedWord(buffer,offset);
			this->page.y=readSignedWord(buffer,offset);

			offset=subheader[1];
			this->background=UniFromUTF8(readString(buffer,offset));
			if (this->background.size()){
				this->bgColor.r=readByte(buffer,offset);
				this->bgColor.g=readByte(buffer,offset);
				this->bgColor.b=readByte(buffer,offset);
			}else{
				this->bgColor.r=0;
				this->bgColor.g=0;
				this->bgColor.b=0;
			}
			if (this->version>1){
				this->char_baseline=readDWord(buffer,offset);
				for (int a=0;a<3;a++){
					this->characters[a].string=UniFromUTF8(readString(buffer,offset));
					if (this->characters[a].string.size()){
						this->characters[a].x=readSignedDWord(buffer,offset);
						this->characters[a].y=readSignedDWord(buffer,offset);
						this->characters[a].visibility=!!readByte(buffer,offset);
						this->characters[a].alpha=readByte(buffer,offset);
					}
				}
			}else
				for (int a=0;a<3;a++)
					this->characters[a].string=UniFromUTF8(readString(buffer,offset));
			if (this->version>2){
				for (ulong a=0;a<3;a++)
					this->charactersBlendOrder[a]=readByte(buffer,offset);

				this->blendSprites=!!readByte(buffer,offset);
			}else
				this->blendSprites=1;
			n=readDWord(buffer,offset);
			std::vector<ulong> intervals;
			for (ulong a=0;a<n;a++){
				ulong b=readDWord(buffer,offset);
				if (b&0x80000000){
					intervals.push_back(b&0x7FFFFFFF);
					intervals.push_back(1);
				}else{
					intervals.push_back(b);
					intervals.push_back(readDWord(buffer,offset));
				}
			}
			for (ulong currentInterval=0;currentInterval<intervals.size();){
				ulong a=intervals[currentInterval],
					b=intervals[currentInterval+1];
				currentInterval+=2;
				if (this->sprites.size()<a+1)
					this->sprites.resize(a+b,0);
				for (ulong c=0;c<b;c++){
					Sprite *spr=new Sprite();
					spr->string=UniFromUTF8(readString(buffer,offset));
					spr->x=readSignedDWord(buffer,offset);
					spr->y=readSignedDWord(buffer,offset);
					spr->visibility=!!readByte(buffer,offset);
					spr->alpha=readByte(buffer,offset);
					this->sprites[a++]=spr;
				}
			}
			this->spritePriority=readDWord(buffer,offset);
			this->pipelines[0].clear();
			this->pipelines[1].clear();
			if (this->version<4){
				bool monochrome=!!readByte(buffer,offset);
				SDL_Color monochromeColor={
					readByte(buffer,offset),
					readByte(buffer,offset),
					readByte(buffer,offset),
					0
				};
				bool negative=!!readByte(buffer,offset);
				if (monochrome)
					this->pipelines->push_back(pipelineElement(0,monochromeColor,L"",0));
				if (negative)
					this->pipelines->push_back(pipelineElement(1,SDL_Color(),L"",0));
			}else{
				this->nega_parameter=!!readByte(buffer,offset);

				this->pipelines->resize(readDWord(buffer,offset));
				for (ulong a=0;a<this->pipelines->size();a++){
					pipelineElement &el=this->pipelines[0][a];
					el.effectNo=readDWord(buffer,offset);
					el.color.r=readByte(buffer,offset);
					el.color.g=readByte(buffer,offset);
					el.color.b=readByte(buffer,offset);
					el.ruleStr=UniFromUTF8(readString(buffer,offset));
				}

				this->asyncEffect_no=readDWord(buffer,offset);
				this->asyncEffect_freq=readDWord(buffer,offset);

				this->pipelines[1].resize(readDWord(buffer,offset));
				for (ulong a=0;a<this->pipelines[1].size();a++){
					pipelineElement &el=this->pipelines[1][a];
					el.effectNo=readDWord(buffer,offset);
					el.color.r=readByte(buffer,offset);
					el.color.g=readByte(buffer,offset);
					el.color.b=readByte(buffer,offset);
					el.ruleStr=UniFromUTF8(readString(buffer,offset));
				}
			}
		}
		//audio
		offset=header[3];
		{
			this->musicTrack=(char)readByte(buffer,offset);
			if (this->musicTrack<0)
				this->music=UniFromUTF8(readString(buffer,offset));
			uchar vol=readByte(buffer,offset);
			/*if (vol>127)
				vol=127;*/
			this->musicVolume=(vol&0x7F);
			this->loopMp3=CHECK_FLAG(vol,0x80);
			this->channels.resize(readWord(buffer,offset),0);
			for (ushort a=0;a<this->channels.size();a++){
				Channel *b=new Channel();
				b->name=UniFromUTF8(readString(buffer,offset));
				vol=readByte(buffer,offset);
				b->volume=(vol&0x7F);
				b->loop=!!(vol&0x80);
				this->channels[a]=b;
			}
		}
	}else
		this->error=NONS_UNSUPPORTED_SAVEGAME_VERSION;
	delete[] buffer;
}

NONS_SaveFile::NONS_SaveFile(){
	memset(this->hash,0,sizeof(unsigned)*5);
}

NONS_SaveFile::~NONS_SaveFile(){
	for (ulong a=0;a<this->sprites.size();a++)
		if (this->sprites[a])
			delete this->sprites[a];
	for (ulong a=0;a<this->stack.size();a++)
		if (this->stack[a])
			delete this->stack[a];
	for (variables_map_T::iterator i=this->variables.begin();i!=this->variables.end();i++)
		if (!!i->second)
			delete i->second;
	for (arrays_map_T::iterator i=this->arrays.begin();i!=this->arrays.end();i++)
		if (!!i->second)
			delete i->second;
	for (ulong a=0;a<this->channels.size();a++)
		if (this->channels[a])
			delete this->channels[a];
}

void writeArray(NONS_VariableMember *var,std::vector<uchar> &buffer){
	writeDWord(var->dimensionSize,buffer);
	if (var->dimensionSize){
		for (ulong a=0;a<var->dimensionSize;a++)
			writeArray(var->dimension[a],buffer);
	}else
		writeDWord(var->getInt(),buffer);
}

bool NONS_SaveFile::save(std::wstring filename){
	if (this->format!='N')
		return 0;
	std::vector<uchar> buffer;
	writeString(L"NONS",buffer);
	writeWord(NONS_SAVEFILE_VERSION,buffer);
	for (ulong a=0;a<5;a++)
		writeDWord(this->hash[a],buffer);
	ulong header[4];
	for (ulong a=0;a<4;a++){
		header[a]=buffer.size();
		writeDWord(0,buffer);
	}
	//stack
	writeDWord(buffer.size(),buffer,*header);
	{
		writeDWord(this->stack.size(),buffer);
		for (ulong a=0;a<this->stack.size();a++){
			stackEl *el=this->stack[a];
			writeByte(el->type,buffer);
			writeString(el->label,buffer);
			writeDWord(el->linesBelow,buffer);
			writeDWord(el->statementNo,buffer);
			writeDWord(el->textgosubLevel,buffer);
			switch (el->type){
				case StackFrameType::SUBROUTINE_CALL:
					writeString(el->leftovers,buffer);
					break;
				case StackFrameType::FOR_NEST:
					writeDWord(el->variable,buffer);
					writeDWord(el->to,buffer);
					writeDWord(el->step,buffer);
					break;
				case StackFrameType::TEXTGOSUB_CALL:
					break;
				case StackFrameType::USERCMD_CALL:
					writeString(el->leftovers,buffer);
					writeDWord(el->parameters.size(),buffer);
					for (ulong b=0;a<el->parameters.size();b++)
						writeString(el->parameters[b],buffer);
					break;
			}
		}
		writeString(this->currentLabel,buffer);
		writeDWord(this->linesBelow,buffer);
		writeDWord(this->statementNo,buffer);
		writeString(this->loadgosub,buffer);
	}
	//variables
	writeDWord(buffer.size(),buffer,header[1]);
	{
		//variables
		bool set=!!this->variables.size();
		if (!set)
			writeDWord(0,buffer);
		else{
			variables_map_T::iterator i=this->variables.begin();
			if (i->first>=200)
				writeDWord(0,buffer);
			else{
				variables_map_T::iterator i2=i;
				std::vector<Sint32> intervals=getIntervals<NONS_Variable *>(i,this->variables.end());
				writeDWord(intervals.size()/2,buffer);
				for (ulong a=0;a<intervals.size();){
					if (intervals[a+1]>1){
						writeDWord(intervals[a++],buffer);
						writeDWord(intervals[a++],buffer);
					}else{
						writeDWord(intervals[a]|0x80000000,buffer);
						a+=2;
					}
				}
				for (i=i2;i!=this->variables.end();i++){
					writeDWord(i->second->intValue->getInt(),buffer);
					writeString(i->second->wcsValue->getWcs(),buffer);
				}
			}
		}
		//arrays
		if (!this->arrays.size())
			writeDWord(0,buffer);
		else{
			std::vector<Sint32> intervals=getIntervals<NONS_VariableMember *>(this->arrays.begin(),this->arrays.end());
			writeDWord(intervals.size()/2,buffer);
			for (ulong a=0;a<intervals.size();){
				if (intervals[a+1]>1){
					writeDWord(intervals[a++],buffer);
					writeDWord(intervals[a++],buffer);
				}else{
					writeDWord(intervals[a]|0x80000000,buffer);
					a+=2;
				}
			}
			for (arrays_map_T::iterator i=this->arrays.begin();i!=this->arrays.end();i++)
				writeArray(i->second,buffer);
		}
	}
	//screen
	writeDWord(buffer.size(),buffer,header[2]);
	{
		ulong screenHeader[2];
		for (ulong a=0;a<2;a++){
			screenHeader[a]=buffer.size();
			writeDWord(0,buffer);
		}
		//window
		writeDWord(buffer.size(),buffer,*screenHeader);
		writeWord(this->textWindow.x,buffer);
		writeWord(this->textWindow.y,buffer);
		writeWord(this->textWindow.w,buffer);
		writeWord(this->textWindow.h,buffer);
		writeWord(this->windowFrame.x,buffer);
		writeWord(this->windowFrame.y,buffer);
		writeWord(this->windowFrame.w,buffer);
		writeWord(this->windowFrame.h,buffer);
		writeByte(this->windowColor.r,buffer);
		writeByte(this->windowColor.g,buffer);
		writeByte(this->windowColor.b,buffer);
		writeDWord(this->windowTransition,buffer);
		writeDWord(this->windowTransitionDuration,buffer);
		writeString(this->windowTransitionRule,buffer);
		writeByte(this->hideWindow,buffer);
		writeWord(this->fontSize,buffer);
		writeByte(this->windowTextColor.r,buffer);
		writeByte(this->windowTextColor.g,buffer);
		writeByte(this->windowTextColor.b,buffer);
		writeByte((uchar(this->bold)<<1)|uchar(this->italic),buffer);
		writeDWord(this->textSpeed,buffer);
		writeByte(this->fontShadow,buffer);
		writeDWord(this->shadowPosX,buffer);
		writeDWord(this->shadowPosY,buffer);
		writeWord((Uint16)this->spacing,buffer);
		writeWord(this->lineSkip,buffer);
		writeDWord(this->logPages.size(),buffer);
		for (ulong a=0;a<this->logPages.size();a++)
			writeString(this->logPages[a],buffer);
		writeString(this->currentBuffer,buffer);
		writeDWord(this->indentationLevel,buffer);
		writeWord(this->textX,buffer);
		writeWord(this->textY,buffer);
		writeString(this->arrow.string,buffer);
		writeByte(this->arrow.absolute,buffer);
		writeWord((Uint16)this->arrow.x,buffer);
		writeWord((Uint16)this->arrow.y,buffer);
		writeString(this->page.string,buffer);
		writeByte(this->page.absolute,buffer);
		writeWord((Uint16)this->page.x,buffer);
		writeWord((Uint16)this->page.y,buffer);
		//graphics
		writeDWord(buffer.size(),buffer,screenHeader[1]);
		writeString(this->background,buffer);
		if (this->background.size()){
			writeByte(this->bgColor.r,buffer);
			writeByte(this->bgColor.g,buffer);
			writeByte(this->bgColor.b,buffer);
		}
		writeDWord(this->char_baseline,buffer);
		for (int a=0;a<3;a++){
			writeString(this->characters[a].string,buffer);
			if (this->characters[a].string.size()){
				writeDWord(this->characters[a].x,buffer);
				writeDWord(this->characters[a].y,buffer);
				writeByte(this->characters[a].visibility,buffer);
				writeByte(this->characters[a].alpha,buffer);
			}
		}
		for (ulong a=0;a<3;a++)
			writeByte(this->charactersBlendOrder[a],buffer);

		writeByte(this->blendSprites,buffer);
		std::vector<ulong> intervals;
		ulong last=0;
		bool set=0;
		for (ulong a=0;a<this->sprites.size() && !set;a++){
			if (!!this->sprites[a]){
				last=a;
				set=1;
			}
		}
		if (set){
			intervals.push_back(last++);
			for (ulong a=last;a<this->sprites.size();a++){
				if (!this->sprites[a]){
					intervals.push_back(last-intervals.back());
					for (a++;a<this->sprites.size() && !this->sprites[a];a++);
					if (a>=this->sprites.size())
						break;
					intervals.push_back(a);
					last=a+1;
				}else
					last++;
			}
			if (intervals.size()%2)
				intervals.push_back(last-intervals.back());
			writeDWord(intervals.size()/2,buffer);
			for (ulong a=0;a<intervals.size();){
				long start=intervals[a++];
				ulong size=intervals[a++];
				if (size==1)
					writeDWord(start|0x80000000,buffer);
				else{
					writeDWord(start,buffer);
					writeDWord(size,buffer);
				}
			}
			for (ulong a=0;a<this->sprites.size();a++){
				Sprite *spr=this->sprites[a];
				if (!spr)
					continue;
				writeString(spr->string,buffer);
				writeDWord(spr->x,buffer);
				writeDWord(spr->y,buffer);
				writeByte(spr->visibility,buffer);
				writeByte(spr->alpha,buffer);
			}
		}else
			writeDWord(0,buffer);
		writeDWord(this->spritePriority,buffer);

		writeByte(this->nega_parameter,buffer);

		writeDWord(this->pipelines->size(),buffer);
		for (ulong a=0;a<this->pipelines->size();a++){
			pipelineElement &el=this->pipelines[0][a];
			writeDWord(el.effectNo,buffer);
			writeByte(el.color.r,buffer);
			writeByte(el.color.g,buffer);
			writeByte(el.color.b,buffer);
			writeString(el.ruleStr,buffer);
		}

		writeDWord(this->asyncEffect_no,buffer);
		writeDWord(this->asyncEffect_freq,buffer);

		writeDWord(this->pipelines[1].size(),buffer);
		for (ulong a=0;a<this->pipelines[1].size();a++){
			pipelineElement &el=this->pipelines[1][a];
			writeDWord(el.effectNo,buffer);
			writeByte(el.color.r,buffer);
			writeByte(el.color.g,buffer);
			writeByte(el.color.b,buffer);
			writeString(el.ruleStr,buffer);
		}
	}
	//audio
	writeDWord(buffer.size(),buffer,header[3]);
	{
		writeByte((Uint8)this->musicTrack,buffer);
		if (this->musicTrack<0)
			writeString(this->music,buffer);
		if (this->musicVolume>127)
			this->musicVolume=127;
		writeByte(this->musicVolume|(this->loopMp3?0x80:0),buffer);
		writeWord(this->channels.size(),buffer);
		for (ulong a=0;a<this->channels.size();a++){
			Channel *c=this->channels[a];
			writeString(c->name,buffer);
			writeByte(c->volume|(c->loop?0:0x80),buffer);
		}
	}

	size_t l;
	uchar *writebuffer=compressBuffer_BZ2(&buffer[0],buffer.size(),l);
	bool ret=!NONS_File::write(filename,writebuffer,l);
	delete[] writebuffer;
	return ret;
	//return !writefile(filename,(char *)buffer.c_str(),buffer.size());
}
