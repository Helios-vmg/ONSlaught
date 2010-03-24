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

#include "GUI.h"
#include "ScreenSpace.h"
#include "ScriptInterpreter.h"
#include "IOFunctions.h"
#include <iostream>
#include <freetype/ftoutln.h>

NONS_DebuggingConsole console;

template <typename T>
inline T FT_FLOOR(T x){
	return (x&-64)/64;
}

template <typename T>
inline T FT_CEIL(T x){
	return ((x+63)&-64)/64;
}

NONS_Lookback::NONS_Lookback(NONS_StandardOutput *output,uchar r,uchar g,uchar b){
	this->output=output;
	this->foreground.r=r;
	this->foreground.g=g;
	this->foreground.b=b;
	this->up=new NONS_Button();
	this->down=new NONS_Button();
	SDL_Rect temp=this->output->foregroundLayer->clip_rect.to_SDL_Rect();
	int thirdofscreen=temp.h/3;
	temp.y=thirdofscreen;
	this->up->onLayer=new NONS_Layer(&temp,0);
	this->up->offLayer=new NONS_Layer(&temp,0);
	this->down->onLayer=new NONS_Layer(&temp,0);
	this->down->offLayer=new NONS_Layer(&temp,0);
	this->down->posy=thirdofscreen*2;
	this->up->box=temp;
	this->up->box.x=0;
	this->up->box.y=0;
	this->down->box=temp;
	this->down->box.x=0;
	this->down->box.y=0;
	this->sUpon=0;
	this->sUpoff=0;
	this->sDownon=0;
	this->sDownoff=0;
}

NONS_Lookback::~NONS_Lookback(){
	delete this->up;
	delete this->down;
	if (!!this->sUpon)
		SDL_FreeSurface(this->sUpon);
	if (!!this->sUpoff)
		SDL_FreeSurface(this->sUpoff);
	if (!!this->sDownon)
		SDL_FreeSurface(this->sDownon);
	if (!!this->sDownoff)
		SDL_FreeSurface(this->sDownoff);
}

bool NONS_Lookback::setUpButtons(const std::wstring &upon,const std::wstring &upoff,const std::wstring &downon,const std::wstring &downoff){
	const std::wstring *srcs[4]={&upon,&upoff,&downon,&downoff};
	SDL_Surface *temp[4];
	for (int a=0;a<4;a++){
		if (!ImageLoader->fetchSprite(temp[a],*srcs[a])){
			for (;a>=0;a--)
				ImageLoader->unfetchImage(temp[a]);
			return 0;
		}
	}
	this->sUpon=temp[0];
	this->sUpoff=temp[1];
	this->sDownon=temp[2];
	this->sDownoff=temp[3];
	
	SDL_Rect src={0,0,this->sUpon->w,this->sUpon->h},
		dst={Sint16(this->up->onLayer->clip_rect.w-this->sUpon->w),0,0,0};
	manualBlit(this->sUpon,&src,this->up->onLayer->data,&dst);
	dst.x=Sint16(this->up->onLayer->clip_rect.w-this->sUpoff->w);
	manualBlit(this->sUpoff,&src,this->up->offLayer->data,&dst);
	dst.x=Sint16(this->down->onLayer->clip_rect.w-this->sDownon->w);
	dst.y=Sint16(this->down->onLayer->clip_rect.h-this->sDownon->h);
	manualBlit(this->sDownon,&src,this->down->onLayer->data,&dst);
	dst.x=Sint16(this->down->offLayer->clip_rect.w-this->sDownoff->w);
	dst.y=Sint16(this->down->offLayer->clip_rect.h-this->sDownoff->h);
	manualBlit(this->sDownoff,&src,this->down->offLayer->data,&dst);
	return 1;
}

void NONS_Lookback::reset(NONS_StandardOutput *output){
	delete (NONS_Button *)this->up;
	delete (NONS_Button *)this->down;
	this->output=output;
	SDL_Rect temp=this->output->foregroundLayer->clip_rect.to_SDL_Rect();
	int thirdofscreen=temp.h/3;
	temp.h=thirdofscreen;
	this->up=new NONS_Button();
	this->down=new NONS_Button();
	this->up->onLayer=new NONS_Layer(&temp,0);
	this->up->offLayer=new NONS_Layer(&temp,0);
	this->down->onLayer=new NONS_Layer(&temp,0);
	this->down->offLayer=new NONS_Layer(&temp,0);
	this->down->posy=thirdofscreen*2;
	this->up->box=temp;
	this->up->box.x=0;
	this->up->box.y=0;
	this->down->box=temp;
	this->down->box.x=0;
	this->down->box.y=0;
	this->up->posx+=temp.x;
	this->up->posy+=temp.y;
	this->down->posx+=temp.x;
	this->down->posy+=temp.y;
	if (!this->sUpon)
		return;
	SDL_Rect src={0,0,this->sUpon->w,this->sUpon->h},
		dst={Sint16(this->up->onLayer->clip_rect.w-this->sUpon->w),0,0,0};
	manualBlit(this->sUpon,&src,this->up->onLayer->data,&dst);
	dst.x=Sint16(this->up->onLayer->clip_rect.w-this->sUpoff->w);
	manualBlit(this->sUpoff,&src,this->up->offLayer->data,&dst);
	//this->up->posx=this->up->onLayer->clip_rect.x;
	//this->up->posy=this->up->onLayer->clip_rect.y;
	dst.x=Sint16(this->down->onLayer->clip_rect.w-this->sDownon->w);
	dst.y=Sint16(this->down->onLayer->clip_rect.h-this->sDownon->h);
	manualBlit(this->sDownon,&src,this->down->onLayer->data,&dst);
	dst.x=Sint16(this->down->offLayer->clip_rect.w-this->sDownoff->w);
	dst.y=Sint16(this->down->offLayer->clip_rect.h-this->sDownoff->h);
	manualBlit(this->sDownoff,&src,this->down->offLayer->data,&dst);
	//this->down->posx=this->down->onLayer->clip_rect.x;
	//this->down->posy=this->down->onLayer->clip_rect.y/*+thirdofscreen*2*/;
}

int NONS_Lookback::display(NONS_VirtualScreen *dst){
	int ret=0;
	if (!this->output->log.size())
		return ret;
	NONS_EventQueue queue;
	SDL_Surface *copyDst,
		*preBlit;
	{
		NONS_MutexLocker ml(screenMutex);
		SDL_Surface *screen=dst->screens[VIRTUAL];
		int w=screen->w,
			h=screen->h;
		copyDst=makeSurface(w,h,32);
		preBlit=makeSurface(w,h,32);
		manualBlit(screen,0,copyDst,0);
	}
	long end=this->output->log.size(),
		currentPage=end-1;
	this->output->ephemeralOut(&this->output->log[currentPage],dst,0,0,&this->foreground);
	{
		NONS_MutexLocker ml(screenMutex);
		manualBlit(dst->screens[VIRTUAL],0,preBlit,0);
	}
	int mouseOver=-1;
	int x,y;
	uchar visibility=(!!currentPage)<<1;
	if (visibility){
		getCorrectedMousePosition(dst,&x,&y);
		if (this->up->MouseOver(x,y)){
			mouseOver=0;
			this->up->mergeWithoutUpdate(dst,preBlit,1,1);
		}else{
			mouseOver=-1;
			this->up->mergeWithoutUpdate(dst,preBlit,0,1);
		}
	}else
		mouseOver=-1;
	dst->updateWholeScreen();
	while (1){
		queue.WaitForEvent(10);
		while (!queue.empty()){
			SDL_Event event=queue.pop();
			switch (event.type){
				case SDL_QUIT:
					ret=INT_MIN;
					goto callLookback_000;
				case SDL_MOUSEMOTION:
					{
						if (visibility){
							if (visibility&2 && this->up->MouseOver(&event)){
								if (visibility&1)
									this->down->merge(dst,preBlit,0);
								mouseOver=0;
								this->up->merge(dst,preBlit,1);
							}else{
								if (visibility&2)
									this->up->merge(dst,preBlit,0);
								if (visibility&1 && this->down->MouseOver(&event)){
									mouseOver=1;
									this->down->merge(dst,preBlit,1);
								}else{
									mouseOver=-1;
									if (visibility&1)
										this->down->merge(dst,preBlit,0);
								}
							}
						}
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					{
						if (event.button.button==SDL_BUTTON_LEFT){
							if (mouseOver<0 || !visibility)
								break;
							{
								NONS_MutexLocker ml(screenMutex);
								manualBlit(copyDst,0,dst->screens[VIRTUAL],0);
							}
							int dir;
							if (!mouseOver)
								dir=-1;
							else
								dir=1;
							if (!this->changePage(dir,currentPage,copyDst,dst,preBlit,visibility,mouseOver))
								goto callLookback_000;
						}else if (event.button.button==SDL_BUTTON_WHEELUP || event.button.button==SDL_BUTTON_WHEELDOWN){
							int dir;
							if (event.button.button==SDL_BUTTON_WHEELUP)
								dir=-1;
							else
								dir=1;
							if (!this->changePage(dir,currentPage,copyDst,dst,preBlit,visibility,mouseOver))
								goto callLookback_000;
						}
					}
					break;
				case SDL_KEYDOWN:
					{
						int dir=0;
						switch (event.key.keysym.sym){
							case SDLK_UP:
							case SDLK_PAGEUP:
								dir=-1;
								break;
							case SDLK_DOWN:
							case SDLK_PAGEDOWN:
								dir=1;
								break;
							case SDLK_ESCAPE:
								dir=end-currentPage;
							default:
								break;
						}
						if (!this->changePage(dir,currentPage,copyDst,dst,preBlit,visibility,mouseOver))
							goto callLookback_000;
					}
					break;
			}
		}
		SDL_Delay(10);
	}
callLookback_000:
	SDL_FreeSurface(copyDst);
	SDL_FreeSurface(preBlit);
	return ret;
}

bool NONS_Lookback::changePage(int dir,long &currentPage,SDL_Surface *copyDst,NONS_VirtualScreen *dst,SDL_Surface *preBlit,uchar &visibility,int &mouseOver){
	long end=this->output->log.size();
	if (!dir || -dir>currentPage)
		return 1;
	{
		NONS_MutexLocker ml(screenMutex);
		manualBlit(copyDst,0,dst->screens[VIRTUAL],0);
	}
	currentPage+=dir;
	if (currentPage==end)
		return 0;
	this->output->ephemeralOut(&this->output->log[currentPage],dst,0,0,&this->foreground);
	{
		NONS_MutexLocker ml(screenMutex);
		manualBlit(dst->screens[VIRTUAL],0,preBlit,0);
	}
	bool visibilitya[]={
		!!currentPage,
		currentPage!=end-1
	};
	visibility=(visibilitya[0]<<1)|int(visibilitya[1]);
	NONS_Button *buttons[]={
		(NONS_Button *)this->up,
		(NONS_Button *)this->down
	};
	int x,y;
	getCorrectedMousePosition(dst,&x,&y);
	mouseOver=-1;
	if (visibilitya[0] || visibilitya[1]){
		for (int a=0;a<2;a++){
			if (mouseOver<0 && buttons[a]->MouseOver(x,y))
				mouseOver=a;
			if (visibilitya[a])
				buttons[a]->mergeWithoutUpdate(dst,preBlit,mouseOver==a,1);
		}
	}
	//SDL_UpdateRect(dst,0,0,0,0);
	dst->updateWholeScreen();
	return 1;
}

NONS_Cursor::NONS_Cursor(NONS_ScreenSpace *screen){
	this->data=0;
	this->xpos=0;
	this->ypos=0;
	this->absolute=0;
	this->screen=screen;
}

NONS_Cursor::NONS_Cursor(const std::wstring &str,long x,long y,long absolute,NONS_ScreenSpace *screen){
	this->data=0;
	this->xpos=x;
	this->ypos=y;
	this->absolute=!!absolute;
	this->screen=screen;
	this->data=new NONS_Layer(&str);
}

NONS_Cursor::~NONS_Cursor(){
	if (this->data)
		delete this->data;
}

int NONS_Cursor::animate(NONS_Menu *menu,ulong expiration){
	if (CURRENTLYSKIPPING)
		return 0;
	this->screen->cursor=this->data;
	this->data->position.x=float(this->xpos+(!this->absolute)?this->screen->output->x:0);
	this->data->position.y=float(this->ypos+(!this->absolute)?this->screen->output->y:0);
	bool done=0;
	NONS_EventQueue queue;
	const long delayadvance=25;
	ulong expire=expiration?expiration:ULONG_MAX;
	int ret=0;
	std::vector<SDL_Rect> rects;
	if (this->data->animated())
		this->screen->BlendAll(1);
	while (!done && !CURRENTLYSKIPPING && expire>0){
		for (ulong a=0;!done && !CURRENTLYSKIPPING && expire>0;a+=delayadvance){
			while (!queue.empty()){
				SDL_Event event=queue.pop();
				switch (event.type){
					case SDL_QUIT:
						ret=-1;
						goto animate_000;
					case SDL_KEYDOWN:
						if (event.key.keysym.sym==SDLK_PAUSE){
							console.enter(this->screen);
							if (queue.emptify()){
								ret=-1;
								goto animate_000;
							}
							break;
						}
						if (!menu)
							break;
						switch (event.key.keysym.sym){
							case SDLK_ESCAPE:
								if (!this->callMenu(menu,&queue)){
									ret=-1;
									goto animate_000;
								}
								break;
							case SDLK_UP:
							case SDLK_PAGEUP:
								if (!this->callLookback(&queue)){
									ret=-1;
									goto animate_000;
								}
								break;
							case SDLK_MENU:
								break;
							default:
								goto animate_000;
						}
						break;
					case SDL_MOUSEBUTTONDOWN:
						if (event.button.button==SDL_BUTTON_RIGHT){
							if (!this->callMenu(menu,&queue)){
								ret=-1;
								goto animate_000;
							}
						}else if (event.button.button==SDL_BUTTON_WHEELUP && !this->callLookback(&queue)){
							ret=-1;
							goto animate_000;
						}else
							done=1;
						break;
					default:
						break;
				}
			}
			ulong t0=SDL_GetTicks();
			if (!!this->screen && this->screen->advanceAnimations(delayadvance,rects))
				this->screen->BlendOptimized(rects);
			ulong t1=SDL_GetTicks()-t0;
			long delay=delayadvance-t1;
			if (delay>0)
				SDL_Delay(delay);
			expire-=delayadvance;
		}
	}
animate_000:
	if (ret!=-1){
		this->screen->BlendNoCursor(1);
		this->screen->cursor=0;
	}
	return ret;
}

bool NONS_Cursor::callMenu(NONS_Menu *menu,NONS_EventQueue *queue){
	if (menu && menu->rightClickMode==1 && menu->buttons){
		//this->screen->BlendNoText(1);
		int ret=menu->callMenu();
		if (ret==-1 || ret==INT_MIN || queue->emptify())
			return 0;
		if (this->data->animated())
			this->screen->BlendAll(1);
		else
			this->screen->BlendNoCursor(1);
	}
	return 1;
}

bool NONS_Cursor::callLookback(NONS_EventQueue *queue){
	this->screen->BlendNoText(0);
	{
		NONS_MutexLocker ml(screenMutex);
		manualBlit(this->screen->screenBuffer,0,this->screen->screen->screens[VIRTUAL],0);
		multiplyBlend(
			this->screen->output->shadeLayer->data,
			0,
			this->screen->screen->screens[VIRTUAL],
			&this->screen->output->shadeLayer->clip_rect.to_SDL_Rect()
		);
	}
	if (this->screen->lookback->display(this->screen->screen)==INT_MIN || queue->emptify())
		return 0;
	if (this->data->animated())
		this->screen->BlendAll(1);
	else
		this->screen->BlendNoCursor(1);
	return 1;
}

NONS_Button::NONS_Button(){
	this->offLayer=0;
	this->onLayer=0;
	this->shadowLayer=0;
	this->box.x=0;
	this->box.y=0;
	this->box.h=0;
	this->box.w=0;
	this->font_cache=0;
	this->status=0;
	this->posx=0;
	this->posy=0;
}

NONS_Button::NONS_Button(const NONS_FontCache &fc){
	this->offLayer=0;
	this->onLayer=0;
	this->shadowLayer=0;
	this->box.x=0;
	this->box.y=0;
	this->box.h=0;
	this->box.w=0;
	this->font_cache=new NONS_FontCache(fc FONTCACHE_DEBUG_PARAMETERS);
	this->status=0;
	this->posx=0;
	this->posy=0;
}

NONS_Button::~NONS_Button(){
	delete this->offLayer;
	delete this->onLayer;
	delete this->shadowLayer;
	delete this->font_cache;
}

void NONS_Button::makeTextButton(const std::wstring &text,const NONS_FontCache &fc_,float center,const SDL_Color &on,const SDL_Color &off,bool shadow,int limitX,int limitY){
	NONS_FontCache fc(fc_ FONTCACHE_DEBUG_PARAMETERS);
	SDL_Color black={0,0,0,0};
	this->limitX=limitX;
	this->limitY=limitY;
	this->box=this->GetBoundingBox(text,&fc,limitX,limitY);
	this->offLayer=new NONS_Layer(&this->box,0);
	this->offLayer->MakeTextLayer(*this->font_cache,off);
	this->onLayer=new NONS_Layer(&this->box,0);
	this->onLayer->MakeTextLayer(*this->font_cache,on);
	if (shadow){
		this->shadowLayer=new NONS_Layer(&this->box,0);
		this->shadowLayer->MakeTextLayer(*this->font_cache,black);
	}
	this->write(text,center);
}

SDL_Rect NONS_Button::GetBoundingBox(const std::wstring &str,NONS_FontCache *cache,int limitX,int limitY){
	std::vector<NONS_Glyph *> outputBuffer;
	long lastSpace=-1;
	int x0=0,
		y0=0,
		wordL=0,
		width=0,
		minheight=INT_MAX,
		height=0,
		lineSkip=this->font_cache->line_skip,
		fontLineSkip=this->font_cache->get_font().line_skip;
	if (!cache)
		cache=this->offLayer->fontCache;
	SDL_Rect frame={0,0,this->limitX,this->limitY};
	for (size_t a=0;a<str.size();a++){
		wchar_t c=str[a];
		NONS_Glyph *glyph=cache->getGlyph(c);
		if (c=='\n'){
			outputBuffer.push_back(0);
			if (x0+wordL>=frame.w && lastSpace>=0){
				if (isbreakspace(outputBuffer[lastSpace]->get_codepoint())){
					outputBuffer[lastSpace]->done();
					outputBuffer[lastSpace]=0;
				}else
					outputBuffer.insert(outputBuffer.begin()+lastSpace+1,0);
				lastSpace=-1;
				x0=0;
				y0+=lineSkip;
			}
			lastSpace=-1;
			x0=0;
			y0+=lineSkip;
			wordL=0;
		}else if (isbreakspace(c)){
			if (x0+wordL>=frame.w && lastSpace>=0){
				if (isbreakspace(outputBuffer[lastSpace]->get_codepoint())){
					outputBuffer[lastSpace]->done();
					outputBuffer[lastSpace]=0;
				}else
					outputBuffer.insert(outputBuffer.begin()+lastSpace+1,0);
				lastSpace=-1;
				x0=0;
				y0+=lineSkip;
			}
			x0+=wordL;
			lastSpace=outputBuffer.size();
			wordL=glyph->get_advance();
			outputBuffer.push_back(glyph);
		}else if (c){
			wordL+=glyph->get_advance();
			outputBuffer.push_back(glyph);
		}
	}
	if (x0+wordL>=frame.w && lastSpace>=0){
		outputBuffer[lastSpace]->done();
		outputBuffer[lastSpace]=0;
	}
	x0=0;
	y0=0;
	for (ulong a=0;a<outputBuffer.size();a++){
		if (!outputBuffer[a]){
			if (x0>width)
				width=x0;
			x0=0;
			y0+=lineSkip;
			continue;
		}
		SDL_Rect tempRect=outputBuffer[a]->get_bounding_box();
		int temp=tempRect.y+tempRect.h;
		if (height<temp)
			height=temp;
		if (tempRect.y<minheight)
			minheight=tempRect.y;
		int advance=outputBuffer[a]->get_advance();
		if (x0+advance>frame.w){
			if (x0>width)
				width=x0;
			x0=0;
			y0+=lineSkip;
		}
		x0+=advance;
		outputBuffer[a]->done();
	}
	if (x0>width)
		width=x0;
	SDL_Rect res={0,0,width,y0+fontLineSkip};
	return res;
}

void NONS_Button::write(const std::wstring &str,float center){
	std::vector<NONS_Glyph *> outputBuffer;
	std::vector<NONS_Glyph *> outputBuffer2;
	std::vector<NONS_Glyph *> outputBuffer3;
	long lastSpace=-1;
	int x0=0,y0=0;
	int wordL=0;
	SDL_Rect frame={0,-this->box.y,this->box.w,this->box.h};
	int lineSkip=this->offLayer->fontCache->get_font().line_skip;
	SDL_Rect screenFrame={0,0,this->limitX,this->limitY};
	for (size_t a=0;a<str.size();a++){
		wchar_t c=str[a];
		NONS_Glyph *glyph=this->offLayer->fontCache->getGlyph(c);
		NONS_Glyph *glyph2=this->onLayer->fontCache->getGlyph(c);
		NONS_Glyph *glyph3=0;
		glyph3=(this->shadowLayer)?this->shadowLayer->fontCache->getGlyph(c):0;
		if (c=='\n'){
			outputBuffer.push_back(0);
			outputBuffer2.push_back(0);
			outputBuffer3.push_back(0);
			if (x0+wordL>=screenFrame.w && lastSpace>=0){
				if (isbreakspace(outputBuffer[lastSpace]->get_codepoint())){
					outputBuffer[lastSpace]->done();
					outputBuffer2[lastSpace]->done();
					outputBuffer3[lastSpace]->done();
					outputBuffer[lastSpace]=0;
					outputBuffer2[lastSpace]=0;
					outputBuffer3[lastSpace]=0;
				}else{
					outputBuffer.insert(outputBuffer.begin()+lastSpace+1,0);
					outputBuffer2.insert(outputBuffer2.begin()+lastSpace+1,0);
					outputBuffer3.insert(outputBuffer3.begin()+lastSpace+1,0);
				}
				lastSpace=-1;
				y0+=lineSkip;
			}
			lastSpace=-1;
			x0=0;
			y0+=lineSkip;
			wordL=0;
		}else if (isbreakspace(c)){
			if (x0+wordL>=screenFrame.w && lastSpace>=0){
				if (isbreakspace(outputBuffer[lastSpace]->get_codepoint())){
					outputBuffer[lastSpace]->done();
					outputBuffer2[lastSpace]->done();
					outputBuffer3[lastSpace]->done();
					outputBuffer[lastSpace]=0;
					outputBuffer2[lastSpace]=0;
					outputBuffer3[lastSpace]=0;
				}else{
					outputBuffer.insert(outputBuffer.begin()+lastSpace+1,0);
					outputBuffer2.insert(outputBuffer2.begin()+lastSpace+1,0);
					outputBuffer3.insert(outputBuffer3.begin()+lastSpace+1,0);
				}
				lastSpace=-1;
				x0=0;
				y0+=lineSkip;
			}
			x0+=wordL;
			lastSpace=outputBuffer.size();
			wordL=glyph->get_advance();
			outputBuffer.push_back(glyph);
			outputBuffer2.push_back(glyph2);
			outputBuffer3.push_back(glyph3);
		}else if (c){
			wordL+=glyph->get_advance();
			outputBuffer.push_back(glyph);
			outputBuffer2.push_back(glyph2);
			outputBuffer3.push_back(glyph3);
		}
	}
	if (x0+wordL>=screenFrame.w && lastSpace>=0){
		outputBuffer[lastSpace]->done();
		outputBuffer2[lastSpace]->done();
		outputBuffer3[lastSpace]->done();
		outputBuffer[lastSpace]=0;
		outputBuffer2[lastSpace]=0;
		outputBuffer3[lastSpace]=0;
	}
	x0=this->setLineStart(&outputBuffer,0,&screenFrame,center);
	y0=0;
	for (ulong a=0;a<outputBuffer.size();a++){
		if (!outputBuffer[a]){
			x0=this->setLineStart(&outputBuffer,a,&screenFrame,center);
			y0+=lineSkip;
			continue;
		}
		int advance=outputBuffer[a]->get_advance();
		if (x0+advance>screenFrame.w){
			x0=this->setLineStart(&outputBuffer,a,&frame,center);
			y0+=lineSkip;
		}
		outputBuffer[a]->put(this->offLayer->data,x0,y0);
		outputBuffer2[a]->put(this->onLayer->data,x0,y0);
		outputBuffer[a]->done();
		outputBuffer2[a]->done();
		if (this->shadowLayer){
			outputBuffer3[a]->put(this->shadowLayer->data,x0,y0);
			outputBuffer3[a]->done();
		}
		x0+=advance;
	}
}

int NONS_Button::setLineStart(std::vector<NONS_Glyph *> *arr,long start,SDL_Rect *frame,float center){
	while (!(*arr)[start])
		start++;
	int width=this->predictLineLength(arr,start,frame->w);
	float factor=(center<=0.5f)?center:1.0f-center;
	int pixelcenter=int(float(frame->w)*factor);
	return int((width/2.0f>pixelcenter)?(frame->w-width)*(center>0.5f):frame->w*center-width/2.0f);
}

int NONS_Button::predictLineLength(std::vector<NONS_Glyph *> *arr,long start,int width){
	int res=0;
	for (ulong a=start;a<arr->size() && (*arr)[a] && res+(*arr)[a]->get_advance()<=width;a++)
		res+=(*arr)[a]->get_advance();
	return res;
}

void NONS_Button::makeGraphicButton(SDL_Surface *src,int posx,int posy,int width,int height,int originX,int originY){
	SDL_Rect dst={0,0,width,height},
		srcRect={originX,originY,width,height};
	this->onLayer=new NONS_Layer(&dst,0);
	manualBlit(src,&srcRect,this->onLayer->data,&dst);
	this->posx=posx;
	this->posy=posy;
	this->box.w=width;
	this->box.h=height;
}

void NONS_Button::mergeWithoutUpdate(NONS_VirtualScreen *dst,SDL_Surface *original,bool status,bool force){
	if (!force && this->status==status)
		return;
	SDL_Rect rect=this->box;
	rect.x=this->posx;
	rect.y=this->posy;

	NONS_MutexLocker ml(screenMutex);
	manualBlit(original,&rect,dst->screens[VIRTUAL],&rect);
	if (this->shadowLayer){
		rect.x++;
		rect.y++;
		manualBlit(this->shadowLayer->data,0,dst->screens[VIRTUAL],&rect);
		rect.x--;
		rect.y--;
	}
	this->status=status;
	if (this->status)
		manualBlit(this->onLayer->data,0,dst->screens[VIRTUAL],&rect);
	else if (this->offLayer)
		manualBlit(this->offLayer->data,0,dst->screens[VIRTUAL],&rect);
}

void NONS_Button::merge(NONS_VirtualScreen *dst,SDL_Surface *original,bool status,bool force){
	if (!force && this->status==status)
		return;
	SDL_Rect rect=this->box;
	rect.x=this->posx;
	rect.y=this->posy;
	this->mergeWithoutUpdate(dst,original,status,force);
	int w,h;
	{
		NONS_MutexLocker ml(screenMutex);
		w=(rect.w+rect.x>dst->screens[VIRTUAL]->w)?(dst->screens[VIRTUAL]->w-rect.x):(rect.w);
		h=(rect.h+rect.y>dst->screens[VIRTUAL]->h)?(dst->screens[VIRTUAL]->h-rect.y):(rect.h);
	}
	dst->updateScreen(rect.x,rect.y,w,h);
}

bool NONS_Button::MouseOver(SDL_Event *event){
	if (event->type!=SDL_MOUSEMOTION)
		return 0;
	int x=event->motion.x,y=event->motion.y;
	int startx=this->posx+this->box.x,starty=this->posy+this->box.y;
	return (x>=startx && x<=startx+this->box.w && y>=starty && y<=starty+this->box.h);
}

bool NONS_Button::MouseOver(int x,int y){
	int startx=this->posx+this->box.x,starty=this->posy+this->box.y;
	return (x>=startx && x<=startx+this->box.w && y>=starty && y<=starty+this->box.h);
}

NONS_ButtonLayer::NONS_ButtonLayer(const NONS_FontCache &fc,NONS_ScreenSpace *screen,bool exitable,NONS_Menu *menu){
	this->font_cache=new NONS_FontCache(fc FONTCACHE_DEBUG_PARAMETERS);
	this->screen=screen;
	this->exitable=exitable;
	this->menu=menu;
	this->loadedGraphic=0;
	this->inputOptions.btnArea=0;
	this->inputOptions.Cursor=0;
	this->inputOptions.Enter=0;
	this->inputOptions.EscapeSpace=0;
	this->inputOptions.Function=0;
	this->inputOptions.Wheel=0;
	this->inputOptions.Insert=0;
	this->inputOptions.PageUpDown=0;
	this->inputOptions.Tab=0;
	this->inputOptions.ZXC=0;
}

NONS_ButtonLayer::NONS_ButtonLayer(SDL_Surface *img,NONS_ScreenSpace *screen){
	this->font_cache=0;
	this->loadedGraphic=img;
	this->screen=screen;
	this->inputOptions.btnArea=0;
	this->inputOptions.Cursor=0;
	this->inputOptions.Enter=0;
	this->inputOptions.EscapeSpace=0;
	this->inputOptions.Function=0;
	this->inputOptions.Wheel=0;
	this->inputOptions.Insert=0;
	this->inputOptions.PageUpDown=0;
	this->inputOptions.Tab=0;
	this->inputOptions.ZXC=0;
}

NONS_ButtonLayer::~NONS_ButtonLayer(){
	for (ulong a=0;a<this->buttons.size();a++)
		if (this->buttons[a])
			delete this->buttons[a];
	if (this->loadedGraphic && !ImageLoader->unfetchImage(this->loadedGraphic))
		SDL_FreeSurface(this->loadedGraphic);
	delete this->font_cache;
}

void NONS_ButtonLayer::makeTextButtons(const std::vector<std::wstring> &arr,
		const SDL_Color &on,
		const SDL_Color &off,
		bool shadow,
		std::wstring *entry,
		std::wstring *mouseover,
		std::wstring *click,
		NONS_Audio *audio,
		NONS_GeneralArchive *archive,
		int width,
		int height){
	if (!this->font_cache)
		return;
	for (ulong a=0;a<this->buttons.size();a++)
		delete this->buttons[a];
	this->buttons.clear();
	this->archive=archive;
	this->audio=audio;
	if (entry)
		this->voiceEntry=*entry;
	if (click)
		this->voiceClick=*click;
	if (mouseover)
		this->voiceMouseOver=*mouseover;
	this->boundingBox.x=0;
	this->boundingBox.y=0;
	this->boundingBox.w=0;
	this->boundingBox.h=0;
	for (ulong a=0;a<arr.size();a++){
		NONS_Button *button=new NONS_Button(*this->font_cache);
		this->buttons.push_back(button);
		button->makeTextButton(arr[a],*this->font_cache,0,on,off,shadow,width,height);
		this->boundingBox.h+=button->box.h;
		if (button->box.w>this->boundingBox.w)
			this->boundingBox.w=button->box.w;
	}
}

int NONS_ButtonLayer::getUserInput(int x,int y){
	if (!this->buttons.size())
		return -1;
	for (ulong a=0;a<this->buttons.size();a++){
		NONS_Button *cB=this->buttons[a];
		cB->posx=x;
		cB->posy=y;
		y+=cB->box.y+cB->box.h;
	}
	if (y>this->screen->output->y0+this->screen->output->h)
		return -2;
	if (this->voiceEntry.size()){
		if (this->audio->bufferIsLoaded(this->voiceEntry))
			this->audio->playSoundAsync(&this->voiceEntry,0,0,7,0);
		else{
			ulong l;
			char *buffer=(char *)this->archive->getFileBuffer(this->voiceEntry,l);
			if (this->audio->playSoundAsync(&this->voiceClick,buffer,l,7,0)!=NONS_NO_ERROR)
				delete[] buffer;
		}
	}
	if (this->voiceMouseOver.size()){
		if (this->audio->bufferIsLoaded(this->voiceMouseOver)){
			ulong l;
			char *buffer=(char *)this->archive->getFileBuffer(this->voiceMouseOver,l);
			this->audio->loadAsyncBuffer(this->voiceMouseOver,buffer,l,7);
		}
	}
	NONS_EventQueue queue;
	SDL_Surface *screenCopy=makeSurface(this->screen->screen->inRect.w,this->screen->screen->inRect.h,32);
	{
		NONS_MutexLocker ml(screenMutex);
		manualBlit(this->screen->screen->screens[VIRTUAL],0,screenCopy,0);
	}
	int mouseOver=-1;
	getCorrectedMousePosition(this->screen->screen,&x,&y);
	for (ulong a=0;a<this->buttons.size();a++){
		NONS_Button *b=this->buttons[a];
		if (b){
			if (b->MouseOver(x,y) && mouseOver<0){
				mouseOver=a;
				b->mergeWithoutUpdate(this->screen->screen,screenCopy,1,1);
			}else
				this->buttons[a]->mergeWithoutUpdate(this->screen->screen,screenCopy,0,1);
		}
	}
	this->screen->screen->updateWholeScreen();
	while (1){
		queue.WaitForEvent(10);
		SDL_Event event=queue.pop();
		//Handle entering to lookback.
		if (event.type==SDL_KEYDOWN && (event.key.keysym.sym==SDLK_UP || event.key.keysym.sym==SDLK_PAGEUP) ||
				event.type==SDL_MOUSEBUTTONDOWN && (event.button.button==SDL_BUTTON_WHEELUP || event.button.button==SDL_BUTTON_WHEELDOWN)){
			this->screen->BlendNoText(0);
			this->screen->screen->blitToScreen(this->screen->screenBuffer,0,0);
			{
				NONS_MutexLocker ml(screenMutex);
				multiplyBlend(
					this->screen->output->shadeLayer->data,
					&this->screen->output->shadeLayer->clip_rect.to_SDL_Rect(),
					this->screen->screen->screens[VIRTUAL],
					0);
				if (this->screen->lookback->display(this->screen->screen)==INT_MIN || queue.emptify()){
					SDL_FreeSurface(screenCopy);
					return INT_MIN;
				}
				manualBlit(screenCopy,0,this->screen->screen->screens[VIRTUAL],0);
			}
			getCorrectedMousePosition(this->screen->screen,&x,&y);
			for (ulong a=0;a<this->buttons.size();a++){
				NONS_Button *b=this->buttons[a];
				if (b){
					if (b->MouseOver(x,y)){
						mouseOver=a;
						b->mergeWithoutUpdate(this->screen->screen,screenCopy,1,1);
					}else
						this->buttons[a]->mergeWithoutUpdate(this->screen->screen,screenCopy,0,1);
				}
			}
			this->screen->screen->updateWholeScreen();
			continue;
		}
		switch (event.type){
			case SDL_QUIT:
				SDL_FreeSurface(screenCopy);
				return INT_MIN;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym){
					case SDLK_ESCAPE:
						if (this->exitable){
							this->screen->screen->blitToScreen(screenCopy,0,0);
							this->screen->screen->updateWholeScreen();
							SDL_FreeSurface(screenCopy);
							return -1;
						}else if (this->menu){
							this->screen->screen->blitToScreen(screenCopy,0,0);
							int ret=this->menu->callMenu();
							if (ret<0){
								SDL_FreeSurface(screenCopy);
								return (ret==INT_MIN)?INT_MIN:-3;
							}
							if (queue.emptify()){
								SDL_FreeSurface(screenCopy);
								return INT_MIN;
							}
							this->screen->screen->blitToScreen(screenCopy,0,0);
							getCorrectedMousePosition(this->screen->screen,&x,&y);
							for (ulong a=0;a<this->buttons.size();a++){
								NONS_Button *b=this->buttons[a];
								if (b){
									if (b->MouseOver(x,y)){
										mouseOver=a;
										b->mergeWithoutUpdate(this->screen->screen,screenCopy,1,1);
									}else
										this->buttons[a]->mergeWithoutUpdate(this->screen->screen,screenCopy,0,1);
								}
							}
							this->screen->screen->updateWholeScreen();
						}
						break;
					//Will never happen:
					/*
					case SDLK_UP:
					case SDLK_PAGEUP:
						{
						}
					*/
					case SDLK_PAUSE:
						console.enter(this->screen);
						if (!queue.emptify()){
							SDL_FreeSurface(screenCopy);
							return INT_MIN;
						}
						break;
					default:
						break;
				}
			case SDL_MOUSEMOTION:
				{
					if (mouseOver>=0 && this->buttons[mouseOver]->MouseOver(&event))
						break;
					int tempMO=-1;
					for (ulong a=0;a<this->buttons.size() && tempMO==-1;a++)
						if (this->buttons[a]->MouseOver(&event))
							tempMO=a;
					if (tempMO<0){
						if (mouseOver>=0)
							this->buttons[mouseOver]->merge(this->screen->screen,screenCopy,0);
						mouseOver=-1;
						break;
					}else{
						if (mouseOver>=0)
							this->buttons[mouseOver]->merge(this->screen->screen,screenCopy,0);
						mouseOver=tempMO;
						this->buttons[mouseOver]->merge(this->screen->screen,screenCopy,1);
						if (this->voiceMouseOver.size()){
							if (this->audio->bufferIsLoaded(this->voiceMouseOver))
								this->audio->playSoundAsync(&this->voiceMouseOver,0,0,7,0);
							else{
								ulong l;
								char *buffer=(char *)this->archive->getFileBuffer(this->voiceMouseOver,l);
								if (this->audio->playSoundAsync(&this->voiceMouseOver,buffer,l,7,0)!=NONS_NO_ERROR)
									delete[] buffer;
							}
						}
						//o_stdout <<"ButtonLayer::getUserInput(): "<<mouseOver<<std::endl;
					}
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				{
					if (mouseOver<0)
						break;
					else{
						if (this->voiceClick.size()){
							if (this->audio->bufferIsLoaded(this->voiceClick))
								this->audio->playSoundAsync(&this->voiceClick,0,0,7,0);
							else{
								ulong l;
								char *buffer=(char *)this->archive->getFileBuffer(this->voiceClick,l);
								if (this->audio->playSoundAsync(&this->voiceClick,buffer,l,7,0)!=NONS_NO_ERROR)
									delete[] buffer;
							}
						}
						{
							NONS_MutexLocker ml(screenMutex);
							manualBlit(screenCopy,0,this->screen->screen->screens[VIRTUAL],0);
						}
						this->screen->screen->updateWholeScreen();
						SDL_FreeSurface(screenCopy);
						return mouseOver;
					}
				}
				break;
		}
	}
}

void NONS_ButtonLayer::addImageButton(ulong index,int posx,int posy,int width,int height,int originX,int originY){
	if (this->buttons.size()<index+1)
		this->buttons.resize(index+1,0);
	NONS_Button *b=new NONS_Button();
	b->makeGraphicButton(this->loadedGraphic,posx,posy,width,height,originX,originY);
	this->buttons[index]=b;
}

ulong NONS_ButtonLayer::countActualButtons(){
	ulong res=0;
	for (ulong a=0;a<this->buttons.size();a++)
		if (this->buttons[a])
			res++;
	return res;
}

int NONS_ButtonLayer::getUserInput(ulong expiration){
	if (!this->countActualButtons()){
		this->addImageButton(0,0,0,this->loadedGraphic->w,this->loadedGraphic->h,0,0);
		//return LONG_MIN;
	}
	NONS_EventQueue queue;
	SDL_Surface *screenCopy;
	{
		NONS_MutexLocker ml(screenMutex);
		screenCopy=makeSurface(this->screen->screen->screens[VIRTUAL]->w,this->screen->screen->screens[VIRTUAL]->h,32);
		manualBlit(this->screen->screen->screens[VIRTUAL],0,screenCopy,0);
	}
	int mouseOver=-1;
	int x,y;
	getCorrectedMousePosition(this->screen->screen,&x,&y);
	for (ulong a=0;a<this->buttons.size();a++){
		NONS_Button *b=this->buttons[a];
		if (b){
			if (b->MouseOver(x,y) && mouseOver<0){
				mouseOver=a;
				b->mergeWithoutUpdate(this->screen->screen,screenCopy,1,1);
			}else
				this->buttons[a]->mergeWithoutUpdate(this->screen->screen,screenCopy,0,1);
		}
	}
	this->screen->screen->updateWholeScreen();
	long expire=expiration;


	std::map<SDLKey,std::pair<int,bool *> > key_bool_map;
	key_bool_map[SDLK_ESCAPE]=  std::make_pair(-10,&this->inputOptions.EscapeSpace);
	key_bool_map[SDLK_SPACE]=   std::make_pair(-11,&this->inputOptions.EscapeSpace);
	key_bool_map[SDLK_PAGEUP]=  std::make_pair(-12,&this->inputOptions.PageUpDown);
	key_bool_map[SDLK_PAGEDOWN]=std::make_pair(-13,&this->inputOptions.PageUpDown);
	key_bool_map[SDLK_RETURN]=  std::make_pair(-19,&this->inputOptions.Enter);
	key_bool_map[SDLK_TAB]=     std::make_pair(-20,&this->inputOptions.Tab);
	key_bool_map[SDLK_F1]=      std::make_pair(-21,&this->inputOptions.Function);
	key_bool_map[SDLK_F2]=      std::make_pair(-22,&this->inputOptions.Function);
	key_bool_map[SDLK_F3]=      std::make_pair(-23,&this->inputOptions.Function);
	key_bool_map[SDLK_F4]=      std::make_pair(-24,&this->inputOptions.Function);
	key_bool_map[SDLK_F5]=      std::make_pair(-25,&this->inputOptions.Function);
	key_bool_map[SDLK_F6]=      std::make_pair(-26,&this->inputOptions.Function);
	key_bool_map[SDLK_F7]=      std::make_pair(-27,&this->inputOptions.Function);
	key_bool_map[SDLK_F8]=      std::make_pair(-28,&this->inputOptions.Function);
	key_bool_map[SDLK_F9]=      std::make_pair(-29,&this->inputOptions.Function);
	key_bool_map[SDLK_F10]=     std::make_pair(-30,&this->inputOptions.Function);
	key_bool_map[SDLK_F11]=     std::make_pair(-31,&this->inputOptions.Function);
	key_bool_map[SDLK_F12]=     std::make_pair(-32,&this->inputOptions.Function);
	key_bool_map[SDLK_UP]=      std::make_pair(-40,&this->inputOptions.Cursor);
	key_bool_map[SDLK_RIGHT]=   std::make_pair(-41,&this->inputOptions.Cursor);
	key_bool_map[SDLK_DOWN]=    std::make_pair(-42,&this->inputOptions.Cursor);
	key_bool_map[SDLK_LEFT]=    std::make_pair(-43,&this->inputOptions.Cursor);
	key_bool_map[SDLK_INSERT]=  std::make_pair(-50,&this->inputOptions.Insert);
	key_bool_map[SDLK_z]=       std::make_pair(-51,&this->inputOptions.ZXC);
	key_bool_map[SDLK_x]=       std::make_pair(-52,&this->inputOptions.ZXC);
	key_bool_map[SDLK_c]=       std::make_pair(-53,&this->inputOptions.ZXC);

	//Is this the same as while (1)? I have no idea, but don't touch it, just in case.
	while (expiration && expire>0 || !expiration){
		while (!queue.empty()){
			SDL_Event event=queue.pop();
			switch (event.type){
				case SDL_QUIT:
					SDL_FreeSurface(screenCopy);
					return INT_MIN;
				case SDL_MOUSEMOTION:
					{
						if (mouseOver>=0 && this->buttons[mouseOver]->MouseOver(&event))
							break;
						int tempMO=-1;
						for (ulong a=0;a<this->buttons.size() && tempMO==-1;a++)
							if (this->buttons[a] && this->buttons[a]->MouseOver(&event))
								tempMO=a;
						if (tempMO<0){
							if (mouseOver>=0)
								this->buttons[mouseOver]->merge(this->screen->screen,screenCopy,0);
							mouseOver=-1;
							break;
						}else{
							if (mouseOver>=0)
								this->buttons[mouseOver]->merge(this->screen->screen,screenCopy,0);
							mouseOver=tempMO;
							this->buttons[mouseOver]->merge(this->screen->screen,screenCopy,1);
						}
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					{
						int button;
						switch (event.button.button){
							case SDL_BUTTON_LEFT:
								button=1;
								break;
							case SDL_BUTTON_RIGHT:
								button=2;
								break;
							case SDL_BUTTON_WHEELUP:
								button=3;
								break;
							case SDL_BUTTON_WHEELDOWN:
								button=4;
								break;
							default:
								button=-1;
						}
						if (button<0)
							break;
						SDL_FreeSurface(screenCopy);
						this->screen->screen->updateWholeScreen();
						if (button==1){
							if (mouseOver<0)
								return -1;
							return mouseOver;
						}else
							return -button;
					}
					break;
				case SDL_KEYDOWN:
					if (event.key.keysym.sym==SDLK_PAUSE){
						console.enter(this->screen);
						if (!queue.emptify()){
							SDL_FreeSurface(screenCopy);
							return INT_MIN;
						}
					}else{
						SDLKey key=event.key.keysym.sym;
						std::map<SDLKey,std::pair<int,bool *> >::iterator i=key_bool_map.find(key);
						int ret=0;
						if (i!=key_bool_map.end()){
							if (*(i->second.second))
								ret=i->second.first-1;
							else if (key==SDLK_ESCAPE)
								ret=-2;
						}
						if (ret){
							SDL_FreeSurface(screenCopy);
							this->screen->screen->updateWholeScreen();
							return ret;
						}
					}
					break;
			}
		}
		SDL_Delay(10);
		expire-=10;
	}
	SDL_FreeSurface(screenCopy);
	return (this->inputOptions.Wheel)?-5:-2;
}

NONS_Menu::NONS_Menu(NONS_ScriptInterpreter *interpreter){
	this->interpreter=interpreter;
	this->on.r=0xFF;
	this->on.g=0xFF;
	this->on.b=0xFF;
	this->off.r=0xAA;
	this->off.g=0xAA;
	this->off.b=0xAA;
	this->nofile=this->off;
	this->shadow=1;
	this->font_cache=0;
	this->default_font_cache=interpreter->font_cache;
	this->buttons=0;
	NONS_ScreenSpace *scr=interpreter->screen;
	{
		NONS_MutexLocker ml(screenMutex);
		this->shade=new NONS_Layer(&(scr->screen->screens[VIRTUAL]->clip_rect),0xCCCCCCCC|amask);
	}
	this->slots=10;
	this->audio=interpreter->audio;
	this->archive=interpreter->archive;
	this->rightClickMode=1;
}

NONS_Menu::NONS_Menu(std::vector<std::wstring> *options,NONS_ScriptInterpreter *interpreter){
	this->interpreter=interpreter;
	for (ulong a=0;a<options->size();a++){
		this->strings.push_back((*options)[a++]);
		this->commands.push_back((*options)[a]);
	}
	this->on.r=0xFF;
	this->on.g=0xFF;
	this->on.b=0xFF;
	this->off.r=0xA9;
	this->off.g=0xA9;
	this->off.b=0xA9;
	this->shadow=1;
	this->font_cache=0;
	NONS_ScreenSpace *scr=interpreter->screen;
	this->default_font_cache=interpreter->font_cache;
	this->buttons=new NONS_ButtonLayer(*this->default_font_cache,scr,1,0);
	int w,h;
	{
		NONS_MutexLocker ml(screenMutex);
		w=scr->screen->screens[VIRTUAL]->w;
		h=scr->screen->screens[VIRTUAL]->h;
	}
	this->audio=interpreter->audio;
	this->archive=interpreter->archive;
	this->buttons->makeTextButtons(
		this->strings,
		this->on,
		this->off,
		this->shadow,
		&this->voiceEntry,
		&this->voiceMO,
		&this->voiceClick,
		this->audio,
		this->archive,
		w,
		h);
	this->x=(w-this->buttons->boundingBox.w)/2;
	this->y=(h-this->buttons->boundingBox.h)/2;
	{
		NONS_MutexLocker ml(screenMutex);
		this->shade=new NONS_Layer(&(scr->screen->screens[VIRTUAL]->clip_rect),0xCCCCCCCC|amask);
	}
	this->rightClickMode=1;
}

NONS_Menu::~NONS_Menu(){
	delete this->buttons;
	delete this->font_cache;
	delete this->shade;
}

int NONS_Menu::callMenu(){
	this->interpreter->screen->BlendNoText(0);
	multiplyBlend(this->shade->data,0,this->interpreter->screen->screenBuffer,0);
	{
		NONS_MutexLocker ml(screenMutex);
		manualBlit(this->interpreter->screen->screenBuffer,0,
			this->interpreter->screen->screen->screens[VIRTUAL],0);
	}
	int choice=this->buttons->getUserInput(this->x,this->y);
	if (choice<0){
		if (choice!=INT_MIN && this->voiceCancel.size()){
			if (this->audio->bufferIsLoaded(this->voiceCancel))
				this->audio->playSoundAsync(&this->voiceCancel,0,0,7,0);
			else{
				ulong l;
				char *buffer=(char *)this->archive->getFileBuffer(this->voiceCancel,l);
				if (this->audio->playSoundAsync(&this->voiceCancel,buffer,l,7,0)!=NONS_NO_ERROR)
					delete[] buffer;
			}
		}
		return 0;
	}
	return this->call(this->commands[choice]);
}

std::string getDefaultFontFilename();

void NONS_Menu::reset(){
	delete this->buttons;
	delete this->font_cache;
	this->font_cache=new NONS_FontCache(*this->default_font_cache FONTCACHE_DEBUG_PARAMETERS);
	this->font_cache->spacing=this->spacing;
	this->font_cache->line_skip=this->lineskip;
	this->shade->setShade(this->shadeColor.r,this->shadeColor.g,this->shadeColor.b);
	NONS_ScreenSpace *scr=this->interpreter->screen;
	this->buttons=new NONS_ButtonLayer(*this->font_cache,scr,1,0);
	int w,h;
	{
		NONS_MutexLocker ml(screenMutex);
		w=scr->screen->screens[VIRTUAL]->w;
		h=scr->screen->screens[VIRTUAL]->h;
	}
	this->buttons->makeTextButtons(
		this->strings,
		this->on,
		this->off,
		this->shadow,
		0,0,0,0,0,
		w,h);
	this->x=(w-this->buttons->boundingBox.w)/2;
	this->y=(h-this->buttons->boundingBox.h)/2;
}

void NONS_Menu::resetStrings(std::vector<std::wstring> *options){
	if (this->buttons)
		delete this->buttons;
	this->strings.clear();
	this->commands.clear();
	for (ulong a=0;a<options->size();a++){
		this->strings.push_back((*options)[a++]);
		this->commands.push_back((*options)[a]);
	}
	NONS_ScreenSpace *scr=interpreter->screen;
	this->buttons=new NONS_ButtonLayer(this->get_font_cache(),scr,1,0);
	int w,h;
	{
		NONS_MutexLocker ml(screenMutex);
		w=scr->screen->screens[VIRTUAL]->w;
		h=scr->screen->screens[VIRTUAL]->h;
	}
	this->buttons->makeTextButtons(
		this->strings,
		this->on,
		this->off,
		this->shadow,
		0,0,0,0,0,
		w,h);
	this->x=(w-this->buttons->boundingBox.w)/2;
	this->y=(h-this->buttons->boundingBox.h)/2;
}

int NONS_Menu::write(const std::wstring &txt,int y){
	NONS_FontCache tempCacheForeground(this->get_font_cache() FONTCACHE_DEBUG_PARAMETERS),
		tempCacheShadow(tempCacheForeground FONTCACHE_DEBUG_PARAMETERS);
	if (this->shadow){
		SDL_Color black={0,0,0,0};
		tempCacheShadow.setColor(black);
	}
	
	std::vector<NONS_Glyph *> outputBuffer;
	std::vector<NONS_Glyph *> outputBuffer2;
	ulong width=0;
	for (size_t a=0;a<txt.size();a++){
		NONS_Glyph *glyph=tempCacheForeground.getGlyph(txt[a]);
		width+=glyph->get_advance();
		outputBuffer.push_back(glyph);
		if (this->shadow)
			outputBuffer2.push_back(tempCacheShadow.getGlyph(txt[a]));
	}
	NONS_ScreenSpace *scr=interpreter->screen;
	int w;
	{
		NONS_MutexLocker ml(screenMutex);
		w=scr->screen->screens[VIRTUAL]->w;
	}
	int x=(w-width)/2;
	for (ulong a=0;a<outputBuffer.size();a++){
		int advance=outputBuffer[a]->get_advance();
		{
			NONS_MutexLocker ml(screenMutex);
			if (this->shadow){
				outputBuffer2[a]->put(scr->screen->screens[VIRTUAL],x+1,y+1);
				outputBuffer2[a]->done();
			}
			outputBuffer[a]->put(scr->screen->screens[VIRTUAL],x,y);
			outputBuffer[a]->done();
		}
		x+=advance;
	}
	return this->get_font_cache().line_skip;
}

extern std::wstring save_directory;

int NONS_Menu::save(){
	int y0;
	if (this->stringSave.size())
		y0=this->write(this->stringSave,20);
	else
		y0=this->write(L"~~ Save File ~~",20);
	NONS_ScreenSpace *scr=interpreter->screen;
	NONS_ButtonLayer files(this->get_font_cache(),scr,1,0);
	std::vector<tm *> times=existing_files(save_directory);
	int choice,
		ret;
	while (1){
		std::vector<std::wstring> strings;
		std::wstring pusher;
		for (ulong a=0;a<slots;a++){
			tm *t=times[a];
			if (this->stringSlot.size())
				pusher=this->stringSlot;
			else
				pusher=L"Slot";
			pusher+=L" "+itoaw(a+1,2)+L"    ";
			if (t)
				pusher.append(getTimeString<wchar_t>(t));
			else
				pusher+=L"-------------------";
			strings.push_back(pusher);
		}
		int w,h;
		{
			NONS_MutexLocker ml(screenMutex);
			w=scr->screen->screens[VIRTUAL]->w;
			h=scr->screen->screens[VIRTUAL]->h;
		}
		files.makeTextButtons(
			strings,
			this->on,
			this->off,
			this->shadow,
			&this->voiceEntry,
			&this->voiceMO,
			&this->voiceClick,
			this->audio,
			this->archive,w,h);
		choice=files.getUserInput((w-files.boundingBox.w)/2,y0*2+20);
		if (choice==INT_MIN)
			ret=INT_MIN;
		else{
			if (choice==-2){
				this->slots--;
				continue;
			}
			if (choice<0){
				if (this->voiceCancel.size()){
					if (this->audio->bufferIsLoaded(this->voiceCancel))
						this->audio->playSoundAsync(&this->voiceCancel,0,0,7,0);
					else{
						ulong l;
						char *buffer=(char *)this->archive->getFileBuffer(this->voiceCancel,l);
						if (this->audio->playSoundAsync(&this->voiceCancel,buffer,l,7,0)!=NONS_NO_ERROR)
							delete[] buffer;
					}
				}
			}
			ret=choice+1;
		}
		break;
	}
	for (ulong a=0;a<times.size();a++)
		if (times[a])
			delete times[a];
	return ret;
}

int NONS_Menu::load(){
	int y0;
	if (this->stringLoad.size())
		y0=this->write(this->stringLoad,20);
	else
		y0=this->write(L"~~ Load File ~~",20);
	NONS_ScreenSpace *scr=interpreter->screen;
	NONS_ButtonLayer files(this->get_font_cache(),scr,1,0);
	std::vector<tm *> times=existing_files(save_directory);
	int choice,
		ret;
	std::vector<std::wstring> strings;
	{
		std::wstring pusher;
		for (ulong a=0;a<slots;a++){
			tm *t=times[a];
			if (this->stringSlot.size())
				pusher=this->stringSlot;
			else
				pusher=L"Slot";
			pusher+=L" "+itoaw(a+1,2)+L"    ";
			if (t)
				pusher.append(getTimeString<wchar_t>(t));
			else
				pusher+=L"-------------------";
			strings.push_back(pusher);
		}
	}
	while (1){
		int w,h;
		{
			NONS_MutexLocker ml(screenMutex);
			w=scr->screen->screens[VIRTUAL]->w;
			h=scr->screen->screens[VIRTUAL]->h;
		}
		files.makeTextButtons(
			strings,
			this->on,
			this->off,
			this->shadow,
			&this->voiceEntry,
			&this->voiceMO,
			&this->voiceClick,
			this->audio,
			this->archive,
			w,h);
		choice=files.getUserInput((w-files.boundingBox.w)/2,y0*2+20);
		if (choice==INT_MIN)
			ret=INT_MIN;
		else{
			if (choice==-2){
				this->slots--;
				strings.pop_back();
				continue;
			}
			if (choice<0 && this->voiceCancel.size()){
				if (this->audio->bufferIsLoaded(this->voiceCancel))
					this->audio->playSoundAsync(&this->voiceCancel,0,0,7,0);
				else{
					ulong l;
					char *buffer=(char *)this->archive->getFileBuffer(this->voiceCancel,l);
					if (this->audio->playSoundAsync(&this->voiceCancel,buffer,l,7,0)!=NONS_NO_ERROR)
						delete[] buffer;
				}
			}
			ret=choice+1;
		}
		break;
	}
	for (ulong a=0;a<times.size();a++)
		if (times[a])
			delete times[a];
	return ret;
}

int NONS_Menu::windowerase(){
	this->interpreter->screen->BlendNoText(1);
	NONS_EventQueue queue;
	while (1){
		queue.WaitForEvent();
		while (!queue.empty()){
			SDL_Event event=queue.pop();
			if (event.type==SDL_KEYDOWN || event.type==SDL_MOUSEBUTTONDOWN)
				return 0;
			if (event.type==SDL_QUIT)
				return INT_MIN;
		}
	}
}

int NONS_Menu::skip(){
	ctrlIsPressed=1;
	return 0;
}

int NONS_Menu::call(const std::wstring &string){
	int ret=0;
	if (string==L"reset")
		ret=-1;
	if (string==L"save"){
		int save=this->save();
		if (save>0){
			this->interpreter->save(save);
			//ret=-1;
		}else if (save==INT_MIN)
			ret=INT_MIN;
	}else if (string==L"load"){
		int load=this->load();
		if (load>0 && this->interpreter->load(load))
			ret=-1;
	}else if (string==L"windowerase"){
		ret=this->windowerase();
	}else if (string==L"lookback"){
		NONS_ScreenSpace *scr=this->interpreter->screen;
		scr->BlendNoText(0);
		{
			NONS_MutexLocker ml(screenMutex);
			manualBlit(scr->screenBuffer,0,scr->screen->screens[VIRTUAL],0);
			multiplyBlend(
				scr->output->shadeLayer->data,
				&scr->output->shadeLayer->clip_rect.to_SDL_Rect(),
				scr->screen->screens[VIRTUAL],
				0);
		}
		ret=scr->lookback->display(scr->screen);
	}else if (string==L"skip"){
		this->skip();
	}else{
		ErrorCode error=this->interpreter->interpretString(string,0,0);
		if (error==NONS_END)
			ret=INT_MIN;
		else if (error!=NONS_NO_ERROR)
			handleErrors(error,-1,"NONS_Menu::call",1);
	}
	return ret;
}

NONS_FreeType_Lib NONS_FreeType_Lib::instance;

NONS_FreeType_Lib::NONS_FreeType_Lib(){
	if (FT_Init_FreeType(&this->library))
		throw std::string("FT_Init_FreeType() has failed!");
}

NONS_FreeType_Lib::~NONS_FreeType_Lib(){
	FT_Done_FreeType(this->library);
}

NONS_Font::NONS_Font(const std::string &filename){
	this->buffer=0;
	this->error=FT_New_Face(NONS_FreeType_Lib::instance.get_lib(),filename.c_str(),0,&this->ft_font);
	if (!this->good())
		return;
	this->size=0;
}

NONS_Font::NONS_Font(uchar *buffer,size_t size){
	this->buffer=buffer;
	this->error=FT_New_Memory_Face(NONS_FreeType_Lib::instance.get_lib(),(const FT_Byte *)buffer,size,0,&this->ft_font);
	if (!this->good())
		return;
	this->size=0;
}

NONS_Font::~NONS_Font(){
	if (this->good())
		FT_Done_Face(this->ft_font);
	delete[] this->buffer;
}

void NONS_Font::set_size(ulong size){
	if (this->size==size)
		return;
	this->size=size;
	FT_Set_Pixel_Sizes(this->ft_font,0,size);
	FT_Fixed scale=this->ft_font->size->metrics.y_scale;
	this->ascent=FT_CEIL(FT_MulFix(this->ft_font->ascender,scale));
	this->line_skip=FT_CEIL(FT_MulFix(this->ft_font->height,scale));
}

bool NONS_Font::is_monospace() const{
	return this->check_flag(FT_FACE_FLAG_FIXED_WIDTH);
}

FT_GlyphSlot NONS_Font::render_glyph(wchar_t codepoint,bool italic,bool bold) const{
	FT_Load_Glyph(this->ft_font,FT_Get_Char_Index(this->ft_font,codepoint),FT_LOAD_FORCE_AUTOHINT);
	if (italic){
		FT_Matrix shear;
		shear.xx=0x10000;
		shear.xy=0x34FE; //~0.207
		shear.yx=0;
		shear.yy=0x10000;
		FT_Outline_Transform(&this->ft_font->glyph->outline,&shear);
	}
	if (bold)
		FT_Outline_Embolden(&this->ft_font->glyph->outline,FT_Pos(this->size*16/10));
	FT_Render_Glyph(this->ft_font->glyph,FT_RENDER_MODE_LIGHT);
	return this->ft_font->glyph;
}

NONS_Glyph::NONS_Glyph(NONS_FontCache &fc,wchar_t codepoint,ulong size,const SDL_Color &color,bool italic,bool bold):fc(fc){
	this->codepoint=codepoint;
	this->size=size;
	this->color=color;
	this->refCount=0;
	this->italic=italic;
	this->bold=bold;
	NONS_Font &font=fc.get_font();
	font.set_size(size);
	FT_GlyphSlot glyph=font.render_glyph(codepoint,italic,bold);
	FT_Bitmap &bitmap=glyph->bitmap;
	ulong width=bitmap.width,
		height=bitmap.rows;
	uchar *dst=this->base_bitmap=new uchar[1+width*height],
		*src=bitmap.buffer;
	for (ulong y=0;y<(ulong)bitmap.rows;y++){
		uchar *src=bitmap.buffer+y*bitmap.pitch;
		for (ulong x=0;x<(ulong)bitmap.width;x++)
			*dst++=*src++;
	}

	this->bounding_box.x=glyph->bitmap_left;
	this->bounding_box.y=Sint16(font.ascent-glyph->bitmap_top);
	this->bounding_box.w=(Uint16)width;
	this->bounding_box.h=(Uint16)height;
	this->advance=FT_CEIL(glyph->metrics.horiAdvance);
}

NONS_Glyph::~NONS_Glyph(){
	delete[] this->base_bitmap;
}

inline bool operator==(const SDL_Color &a,const SDL_Color &b){
	return a.r==b.r && a.g==b.g && a.b==b.b;
}
inline bool operator!=(const SDL_Color &a,const SDL_Color &b){ return !(a==b); }

int NONS_Glyph::compare_properties(ulong size,const SDL_Color &color,bool italic,bool bold) const{
	if (this->color!=color)
		return 1;
	if (this->size!=size || this->italic!=italic || this->bold!=bold)
		return 2;
	return 0;
}

long NONS_Glyph::get_advance(){
	return long(this->advance)+this->fc.spacing;
}

void NONS_Glyph::put(SDL_Surface *dst,int x,int y,uchar alpha){
	x+=this->bounding_box.x;
	y+=this->bounding_box.y;
	int x0=0,
		y0=0;
	if (x<0){
		x0=-x;
		x=0;
	}
	if (y<0){
		y0=-y;
		y=0;
	}

	SDL_LockSurface(dst);
	surfaceData sd(dst);
	uchar *src=this->base_bitmap;
	uchar r0=this->color.r,
		g0=this->color.g,
		b0=this->color.b;
	for (ulong src_y=y0,dst_y=y;src_y<this->bounding_box.h && dst_y<sd.h;src_y++,dst_y++){
		uchar *dst=sd.pixels+dst_y*sd.pitch+x*sd.advance;
		src+=x0;
		for (ulong src_x=x0,dst_x=x;src_x<this->bounding_box.w && dst_x<sd.w;src_x++,dst_x++){
			uchar a0=*src,
				*r1=dst+sd.Roffset,
				*g1=dst+sd.Goffset,
				*b1=dst+sd.Boffset,
				*a1=dst+sd.Aoffset;

			do_alpha_blend(r1,g1,b1,a1,r0,g0,b0,a0,sd.alpha,1,alpha);

			src++;
			dst+=sd.advance;
		}
	}
	SDL_UnlockSurface(dst);
}

void NONS_Glyph::done(){
	if (this)
		this->fc.done(this);
}
#ifndef _DEBUG
NONS_FontCache::NONS_FontCache(NONS_Font &f,ulong size,const SDL_Color &color,bool italic,bool bold):font(f){
#else
NONS_FontCache::NONS_FontCache(NONS_Font &f,ulong size,const SDL_Color &color,bool italic,bool bold,const char *file,ulong line):font(f){
	this->declared_in=file;
	this->line=line;
#endif
	this->setColor(color);
	this->resetStyle(size,italic,bold);
	this->spacing=0;
	this->line_skip=font.line_skip;
}

#ifndef _DEBUG
NONS_FontCache::NONS_FontCache(const NONS_FontCache &fc):font(fc.font){
#else
NONS_FontCache::NONS_FontCache(const NONS_FontCache &fc,const char *file,ulong line):font(fc.font){
	this->declared_in=file;
	this->line=line;
#endif
	this->setColor(fc.color);
	this->resetStyle(fc.size,fc.italic,fc.bold);
	this->spacing=fc.spacing;
}

NONS_FontCache::~NONS_FontCache(){
	ulong count=0;
	for (std::map<wchar_t,NONS_Glyph *>::iterator i=this->glyphs.begin(),end=this->glyphs.end();i!=end;i++){
		count+=i->second->refCount;
		delete i->second;
	}
	for (std::set<NONS_Glyph *>::iterator i=this->garbage.begin(),end=this->garbage.end();i!=end;i++){
		count+=(*i)->refCount;
		delete *i;
	}
	if (count){
		o_stderr <<"NONS_FontCache::~NONS_FontCache(): Warning: "<<count<<" possible dangling references.\n";
#ifdef _DEBUG
		o_stderr <<"The cache was created in "<<this->declared_in<<", line "<<this->line<<"\n";
#endif
	}
}

void NONS_FontCache::set_size(ulong size){
	this->size=size;
	this->font.set_size(size);
	this->line_skip=this->font.line_skip;
}

void NONS_FontCache::resetStyle(ulong size,bool italic,bool bold){
	this->set_size(size);
	this->set_italic(italic);
	this->set_bold(bold);
}

NONS_Glyph *NONS_FontCache::getGlyph(wchar_t c){
	if (c<32)
		return 0;
	bool must_render=(this->glyphs.find(c)==this->glyphs.end());
	NONS_Glyph *&g=this->glyphs[c];
	if (!must_render && g->compare_properties(this->size,this->color,this->italic,this->bold)==2){
		if (!g->refCount)
			delete g;
		else
			this->garbage.insert(g);
		must_render=1;
	}
	if (must_render)
		g=new NONS_Glyph(*this,c,this->size,this->color,this->italic,this->bold);
	g->setColor(this->color);
	g->refCount++;
	return g;
}

void NONS_FontCache::done(NONS_Glyph *g){
	if (!g)
		return;
	std::map<wchar_t,NONS_Glyph *>::iterator i=this->glyphs.find(g->get_codepoint());
	if (i!=this->glyphs.end()){
		if (i->second!=g){
			std::set<NONS_Glyph *>::iterator i2=this->garbage.find(g);
			if (i2!=this->garbage.end()){
				(*i2)->refCount--;
				if (!(*i2)->refCount){
					delete *i2;
					this->garbage.erase(i2);
				}
			}
		}else
			i->second->refCount--;
	}
	//otherwise, the glyph doesn't belong to this cache
}

NONS_Font *init_font(NONS_GeneralArchive *archive,const std::string &filename){
	NONS_Font *font=new NONS_Font(filename);
	if (!font->good()){
		delete font;
		ulong l;
		uchar *buffer=archive->getFileBuffer(UniFromISO88591(filename),l);
		if (!buffer)
			return 0;
		font=new NONS_Font(buffer,l);
	}
	return font;
}

NONS_DebuggingConsole::NONS_DebuggingConsole():font(0),cache(0),print_prompt(1){}

NONS_DebuggingConsole::~NONS_DebuggingConsole(){
	if (this->font)
		delete this->font;
}

ulong getGlyphWidth(NONS_FontCache *cache){
	ulong res=0;
	for (wchar_t a='A';a<='Z';a++){
		NONS_Glyph *g=cache->getGlyph(a);
		ulong w=g->get_advance();
		g->done();
		if (w>res)
			res=w;
	}
	return res;
}

extern ConfigFile settings;

std::string getConsoleFontFilename(){
	if (!settings.exists(L"console font"))
		settings.assignWString(L"console font",L"cour.ttf");
	return UniToUTF8(settings.getWString(L"console font"));
}

void NONS_DebuggingConsole::init(NONS_GeneralArchive *archive){
	if (!this->font){
		std::string font=getConsoleFontFilename();
		this->font=init_font(archive,font.c_str());
		if (!this->font){
			o_stderr <<"The font \""<<font<<"\" could not be found. The debugging console will not be available.\n";
		}else{
			SDL_Color color={0xFF,0xFF,0xFF,0xFF};
			this->cache=new NONS_FontCache(*this->font,15,color,0,0 FONTCACHE_DEBUG_PARAMETERS);
			this->screenW=this->screenH=0;
		}
	}
	this->autocompleteVector.push_back(L"quit");
	this->autocompleteVector.push_back(L"_get");
	gScriptInterpreter->getCommandListing(this->autocompleteVector);
}

extern bool useDebugMode;

void NONS_DebuggingConsole::enter(NONS_ScreenSpace *dst){
	if (!this->font)
		return;
	SDL_Surface *dstCopy;
	{
		NONS_MutexLocker ml(screenMutex);
		if (!this->screenW){
			this->characterWidth=getGlyphWidth(this->cache);
			this->characterHeight=this->font->line_skip;
			this->screenW=dst->screen->screens[VIRTUAL]->w/this->characterWidth;
			this->screenH=dst->screen->screens[VIRTUAL]->h/this->characterHeight;
			this->screen.resize(this->screenW*this->screenH);
			this->cursorX=this->cursorY=0;
		}
		dstCopy=copySurface(dst->screen->screens[VIRTUAL]);
		SDL_FillRect(dst->screen->screens[VIRTUAL],0,amask);
		gScriptInterpreter->getSymbolListing(this->autocompleteVector);
		std::sort(this->autocompleteVector.begin(),this->autocompleteVector.end());
		dst->screen->updateWithoutLock();
	}
	bool ret=1;
	std::wstring inputLine=this->partial;
	while (this->input(inputLine,dst)){
		if (!stdStrCmpCI(inputLine,L"quit")){
			gScriptInterpreter->stop();
			break;
		}
		this->output(inputLine+L"\n",dst);
		this->pastInputs.push_back(inputLine);
		if (firstcharsCI(inputLine,0,L"_get"))
			this->output(gScriptInterpreter->getValue(inputLine.substr(4))+L"\n",dst);
		else if (!stdStrCmpCI(inputLine,L"cls")){
			this->screen.clear();
			this->screen.resize(this->screenH*this->screenW);
			this->cursorX=0;
			this->cursorY=0;
		}else
			this->output(gScriptInterpreter->interpretFromConsole(inputLine)+L"\n",dst);
		inputLine.clear();
		this->print_prompt=1;
	}
	this->partial=inputLine;
	
	dst->screen->blitToScreen(dstCopy,0,0);
	dst->screen->updateWholeScreen();
	SDL_FreeSurface(dstCopy);
}

void NONS_DebuggingConsole::output(const std::wstring &str,NONS_ScreenSpace *dst){
	ulong lastY=this->cursorY;
	for (ulong a=0;a<str.size();a++){
		switch (str[a]){
			case '\n':
				this->cursorX=0;
				this->cursorY++;
				break;
			case '\t':
				this->cursorX+=4-this->cursorX%4;
				if (this->cursorX>=this->screenW){
					this->cursorX=0;
					this->cursorY++;
				}
				break;
			default:
				this->locate(this->cursorX,this->cursorY)=str[a];
				this->cursorX++;
				if (this->cursorX>=this->screenW){
					this->cursorX=0;
					this->cursorY++;
				}
		}
		while (this->cursorY*this->screenW>=this->screen.size())
			this->screen.resize(this->screen.size()+this->screenW);
		if (dst && this->cursorY!=lastY){
			lastY=this->cursorY;
			NONS_MutexLocker ml(screenMutex);
			this->redraw(dst,0,0);
		}
	}
}

void NONS_DebuggingConsole::autocomplete(std::vector<std::wstring> &dst,const std::wstring &line,std::wstring &suggestion,ulong cursor,ulong &space){
	dst.clear();
	std::wstring first,
		second,
		third=line.substr(cursor);
	ulong cutoff=line.find_last_of(WCS_WHITESPACE,cursor);
	if (cutoff==line.npos)
		cutoff=0;
	else
		cutoff++;
	space=cutoff;
	first=line.substr(0,cutoff);
	second=line.substr(cutoff,cursor-cutoff);
	for (ulong a=0;a<this->autocompleteVector.size();a++)
		if (firstcharsCI(this->autocompleteVector[a],0,second))
			dst.push_back(this->autocompleteVector[a]);
	if (!dst.size())
		return;
	ulong max;
	for (max=0;max<dst.front().size();max++){
		wchar_t c=dst.front()[max];
		bool _break=0;
		for (ulong a=0;a<dst.size() && !_break;a++)
			if (dst[a][max]!=c)
				_break=1;
		if (_break)
			break;
	}
	suggestion=dst.front().substr(0,max);
}

void NONS_DebuggingConsole::outputMatches(const std::vector<std::wstring> &matches,NONS_ScreenSpace *dst/*,long startFromLine,ulong cursor,const std::wstring &line*/){
	this->output(L"\n",dst);
	for (ulong a=0;a<matches.size();a++)
		this->output(matches[a]+L"\n",dst);
}

bool NONS_DebuggingConsole::input(std::wstring &input,NONS_ScreenSpace *dst){
	const std::wstring prompt=L"input:>";
	if (this->print_prompt){
		this->output(prompt,dst);
		print_prompt=0;
	}
	ulong cursor=input.size();
	this->redraw(dst,0,cursor,input);
	bool ret=1;
	useDebugMode=1;
	std::vector<std::wstring> inputs=this->pastInputs;
	std::wstring inputLine=input;
	inputs.push_back(inputLine);
	ulong currentlyEditing=inputs.size()-1;
	NONS_EventQueue queue;
	bool _break=0;
	long screenOffset=0;
	while (!_break){
		queue.WaitForEvent(10);
		while (!queue.empty() && !_break){
			SDL_Event event=queue.pop();
			bool refresh=0;
			switch (event.type){
				case SDL_QUIT:
					ret=0;
					_break=1;
					break;
				case SDL_KEYDOWN:
					{
						switch (event.key.keysym.sym){
							case SDLK_UP:
								if (!(event.key.keysym.mod&KMOD_CTRL)){
									if (currentlyEditing){
										inputs[currentlyEditing--]=inputLine;
										inputLine=inputs[currentlyEditing];
										cursor=inputLine.size();
										refresh=1;
									}
								}else if (this->screen.size()/this->screenW-this->screenH+screenOffset>0){
									screenOffset--;
									refresh=1;
								}
								break;
							case SDLK_DOWN:
								if (!(event.key.keysym.mod&KMOD_CTRL)){
									if (currentlyEditing<inputs.size()-1){
										inputs[currentlyEditing++]=inputLine;
										inputLine=inputs[currentlyEditing];
										cursor=inputLine.size();
										refresh=1;
									}
								}else if (screenOffset<0){
									screenOffset++;
									refresh=1;
								}
								break;
							case SDLK_LEFT:
								if (cursor){
									cursor--;
									refresh=1;
								}
								break;
							case SDLK_RIGHT:
								if (cursor<inputLine.size()){
									cursor++;
									refresh=1;
								}
								break;
							case SDLK_HOME:
								if (cursor){
									cursor=0;
									refresh=1;
								}
								break;
							case SDLK_END:
								if (cursor<inputLine.size()){
									cursor=inputLine.size();
									refresh=1;
								}
								break;
							case SDLK_RETURN:
							case SDLK_KP_ENTER:
								_break=1;
								break;
							case SDLK_PAUSE:
							case SDLK_ESCAPE:
								ret=0;
								_break=1;
								break;
							case SDLK_BACKSPACE:
								if (inputLine.size() && cursor){
									inputLine.erase(cursor-1,1);
									cursor--;
									refresh=1;
								}
								break;
							case SDLK_DELETE:
								if (cursor<inputLine.size()){
									inputLine.erase(cursor,1);
									refresh=1;
								}
								break;
							case SDLK_TAB:
								if (inputLine.size()){
									std::vector<std::wstring> matches;
									std::wstring suggestion;
									ulong space;
									this->autocomplete(matches,inputLine,suggestion,cursor,space);
									if (matches.size()==1){
										inputLine=inputLine.substr(0,space)+matches.front()+L" "+inputLine.substr(cursor);
										cursor=space+matches.front().size()+1;
										refresh=1;
									}else if (matches.size()>1){
										inputLine=inputLine.substr(0,space)+suggestion+inputLine.substr(cursor);
										cursor=space+suggestion.size();
										this->outputMatches(matches,dst);
										this->output(prompt,dst);
										refresh=1;
									}
								}
								break;
							default:
								{
									wchar_t c=event.key.keysym.unicode;
									if (c<32)
										break;
									inputLine.insert(inputLine.begin()+cursor++,c);
									refresh=1;
								}
						}
						break;
					}
				default:
					break;
			}
			if (refresh)
				this->redraw(dst,screenOffset,cursor,inputLine);
		}
	}
	useDebugMode=0;
	input=inputLine;
	return ret;
}

void NONS_DebuggingConsole::redraw(NONS_ScreenSpace *dst,long startFromLine,ulong lineHeight){
	static SDL_Color white={0xFF,0xFF,0xFF,0xFF};
	{
		NONS_MutexLocker ml(screenMutex);
		SDL_FillRect(dst->screen->screens[VIRTUAL],0,amask);
	}
	long startAt=this->screen.size()/this->screenW-this->screenH;
	if (this->cursorY+lineHeight>=this->screenH)
		startAt+=startFromLine+lineHeight;
	if (startAt<0)
		startAt=0;
	this->cache->setColor(white);
	for (ulong y=0;y<this->screenH;y++){
		for (ulong x=0;x<this->screenW;x++){
			if (CONLOCATE(x,y+startAt)>=this->screen.size())
				continue;
			wchar_t c=this->locate(x,y+startAt);
			if (!c)
				continue;
			NONS_Glyph *g=this->cache->getGlyph(c);
			{
				NONS_MutexLocker ml(screenMutex);
				g->put(dst->screen->screens[VIRTUAL],x*this->characterWidth,y*this->characterHeight);
			}
			g->done();
		}
	}
}

void NONS_DebuggingConsole::redraw(NONS_ScreenSpace *dst,long startFromLine,ulong cursor,const std::wstring &line){
	static SDL_Color white={0xFF,0xFF,0xFF,0xFF},
		black={0,0,0,0xFF};
	NONS_MutexLocker ml(screenMutex);
	ulong lineHeight=(line.size()+this->cursorX)/this->screenW;
	this->redraw(dst,startFromLine,lineHeight);
	long cursorX=this->cursorX,
		cursorY=this->cursorY;
	if (cursorY+lineHeight>=this->screenH)
		cursorY=this->screenH-1-lineHeight-startFromLine;
	//cursorY-=startFromLine;
	if (cursor<line.size()){
		for (ulong a=0;a<line.size();a++){
			if (cursorY<(long)this->screenH){
				if (a==cursor){
					SDL_Rect rect={
						Sint16(cursorX*this->characterWidth),
						Sint16(cursorY*this->characterHeight),
						(Uint16)this->characterWidth,
						(Uint16)this->characterHeight
					};
					SDL_FillRect(dst->screen->screens[VIRTUAL],&rect,rmask|gmask|bmask|amask);
				}
				this->cache->setColor((a!=cursor)?white:black);
				NONS_Glyph *g=this->cache->getGlyph(line[a]);
				g->put(
					dst->screen->screens[VIRTUAL],
					cursorX*this->characterWidth,
					cursorY*this->characterHeight
				);
				g->done();
			}
			cursorX++;
			if (cursorX>=(long)this->screenW){
				cursorX=0;
				cursorY++;
			}
		}
	}else{
		this->cache->setColor(white);
		for (ulong a=0;a<line.size();a++){
			if (cursorY<(long)this->screenH){
				NONS_Glyph *g=this->cache->getGlyph(line[a]);
				g->put(
					dst->screen->screens[VIRTUAL],
					cursorX*this->characterWidth,
					cursorY*this->characterHeight
				);
				g->done();
			}
			cursorX++;
			if (cursorX>=(long)this->screenW){
				cursorX=0;
				cursorY++;
			}
		}
		SDL_Rect rect={
			Sint16(cursorX*this->characterWidth),
			Sint16(cursorY*this->characterHeight),
			(Uint16)this->characterWidth,
			(Uint16)this->characterHeight
		};
		SDL_FillRect(dst->screen->screens[VIRTUAL],&rect,rmask|gmask|bmask|amask);
	}
	dst->screen->updateWithoutLock();
}
