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
#include "CommandLineOptions.h"
#include <iostream>
#include <cassert>
#include <freetype/ftoutln.h>
#include <freetype/ftstroke.h>

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
	this->up=0;
	this->down=0;
	this->resetButtons();
	memset(this->surfaces,0,sizeof(this->surfaces));
}

void NONS_Lookback::resetButtons(){
	NONS_Rect temp=this->output->foregroundLayer->clip_rect;
	temp.y=temp.h;
	temp.h/=3.f;
	temp.y-=temp.h;
	NONS_GraphicButton *up=new NONS_GraphicButton(0,0,0,(int)temp.w,(int)temp.h,0,0),
		*down=new NONS_GraphicButton(0,0,(int)temp.y,(int)temp.w,(int)temp.h,0,0);
	delete this->up;
	delete this->down;
	this->up=up;
	this->down=down;
}

NONS_Lookback::~NONS_Lookback(){
	delete this->up;
	delete this->down;
	for (int a=0;a<4;a++)
		ImageLoader.unfetchImage(this->surfaces[a]);
}

bool NONS_Lookback::setUpButtons(const std::wstring &upon,const std::wstring &upoff,const std::wstring &downon,const std::wstring &downoff){
	const std::wstring *srcs[4]={&upon,&upoff,&downon,&downoff};
	for (int a=0;a<4;a++){
		if (!ImageLoader.fetchSprite(this->surfaces[a],*srcs[a])){
			for (;a>=0;a--){
				ImageLoader.unfetchImage(this->surfaces[a]);
				this->surfaces[a]=0;
			}
			return 0;
		}
	}
	this->setUpButtons();
	return 1;
}

bool NONS_Lookback::setUpButtons(ulong up,ulong down,NONS_ScreenSpace *screen){
	delete this->up;
	delete this->down;
	this->up=new NONS_SpriteButton(up,screen);
	this->down=new NONS_SpriteButton(down,screen);
	return 1;
}

void NONS_Lookback::setUpButtons(){
	NONS_GraphicButton *up=dynamic_cast<NONS_GraphicButton *>(this->up),
		*down=dynamic_cast<NONS_GraphicButton *>(this->down);
	assert(up!=0);
	up->allocateLayer(up->setOffLayer(),0,0,0,(int)up->getBox().w,(int)up->getBox().h,0,0);
	down->allocateLayer(down->setOffLayer(),0,0,0,(int)down->getBox().w,(int)down->getBox().h,0,0);
	NONS_Rect srcRect(0,0,(float)this->surfaces[0]->w,(float)this->surfaces[0]->h),
		dstRect[4];
	NONS_Layer *dst[]={
		up->setOnLayer(),
		up->setOffLayer(),
		down->setOnLayer(),
		down->setOffLayer()
	};
	for (int a=0;a<4;a++){
		dstRect[a].x=dst[a]->clip_rect.w-this->surfaces[a]->w;
		if (a<2)
			continue;
		dstRect[a].y=dst[a]->clip_rect.h-this->surfaces[a]->h;
	}
	for (int a=0;a<4;a++)
		manualBlit(this->surfaces[a],&srcRect.to_SDL_Rect(),dst[a]->data,&dstRect[a].to_SDL_Rect());
}

void NONS_Lookback::reset(NONS_StandardOutput *output){
	this->output=output;
	if (this->up && !dynamic_cast<NONS_SurfaceButton *>(this->up)){
		this->resetButtons();
		if (!this->surfaces[0])
			return;
		this->setUpButtons();
	}
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
	this->data->position.x=float(this->xpos+((!this->absolute)?this->screen->output->x:0));
	this->data->position.y=float(this->ypos+((!this->absolute)?this->screen->output->y:0));
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

#define NONS_BUTTON_MERGE_COMMON										\
	if (!force && this->status==status)									\
		return;															\
	SDL_Rect srcRect=nrect.to_SDL_Rect(),								\
		dstRect=srcRect;												\
	dstRect.x=posx;														\
	dstRect.y=posy;														\
	this->status=status;												\
	{																	\
		NONS_MutexLocker ml(screenMutex);								\
		manualBlit(original,&dstRect,dst->screens[VIRTUAL],&dstRect);	\
		this->mergeWithoutUpdate_inner(dst,&dstRect,&srcRect);			\
	}

bool NONS_Button::MouseOver(SDL_Event *event){
	if (event->type!=SDL_MOUSEMOTION)
		return 0;
	int x=event->motion.x,
		y=event->motion.y;
	return this->MouseOver(x,y);
}

void NONS_Button::merge(
		NONS_VirtualScreen *dst,
		NONS_Rect &nrect,
		int posx,
		int posy,
		SDL_Surface *original,
		bool status,
		bool force){
	NONS_BUTTON_MERGE_COMMON;
	int w,h;
	{
		NONS_MutexLocker ml(screenMutex);
		w=(dstRect.w+dstRect.x>dst->screens[VIRTUAL]->w)?(dst->screens[VIRTUAL]->w-dstRect.x):(dstRect.w);
		h=(dstRect.h+dstRect.y>dst->screens[VIRTUAL]->h)?(dst->screens[VIRTUAL]->h-dstRect.y):(dstRect.h);
	}
	dst->updateScreen(dstRect.x,dstRect.y,w,h);
}

void NONS_Button::mergeWithoutUpdate(
		NONS_VirtualScreen *dst,
		NONS_Rect &nrect,
		int posx,
		int posy,
		SDL_Surface *original,
		bool status,
		bool force){
	NONS_BUTTON_MERGE_COMMON;
}

NONS_SurfaceButton::~NONS_SurfaceButton(){
	delete this->getOffLayer();
	delete this->getOnLayer();
}

void NONS_SurfaceButton::mergeWithoutUpdate(NONS_VirtualScreen *dst,SDL_Surface *original,bool status,bool force){
	NONS_Button::mergeWithoutUpdate(dst,this->box,this->posx,this->posy,original,status,force);
}

NONS_Rect NONS_SurfaceButton::get_dimensions(){
	NONS_Rect rect=this->box;
	rect.x=(float)posx;
	rect.y=(float)posy;
	return rect;
}

void NONS_SurfaceButton::mergeWithoutUpdate_inner(NONS_VirtualScreen *dst,SDL_Rect *dstRect,SDL_Rect *srcRect){
	NONS_TextButton *_this=dynamic_cast<NONS_TextButton *>(this);
	if (_this && _this->getShadowLayer()){
		dstRect->x++;
		dstRect->y++;
		manualBlit(_this->setShadowLayer()->data,0,dst->screens[VIRTUAL],dstRect);
		dstRect->x--;
		dstRect->y--;
	}
	if (this->status)
		manualBlit(this->getOnLayer()->data,0,dst->screens[VIRTUAL],dstRect);
	else if (this->getOffLayer())
		manualBlit(this->getOffLayer()->data,0,dst->screens[VIRTUAL],dstRect);
}

void NONS_SurfaceButton::merge(NONS_VirtualScreen *dst,SDL_Surface *original,bool status,bool force){
	NONS_Button::merge(dst,this->box,this->posx,this->posy,original,status,force);
}

NONS_GraphicButton::NONS_GraphicButton(SDL_Surface *src,int posx,int posy,int width,int height,int originX,int originY)
		:NONS_SurfaceButton(){
	this->allocateLayer(this->setOnLayer(),src,posx,posy,width,height,originX,originY);
	this->posx=posx;
	this->posy=posy;
	this->box.w=(float)width;
	this->box.h=(float)height;
}

void NONS_GraphicButton::allocateLayer(
		NONS_Layer *&layer,
		SDL_Surface *src,
		int posx,
		int posy,
		int width,
		int height,
		int originX,
		int originY){
	SDL_Rect dst={0,0,width,height},
		srcRect={originX,originY,width,height};
	delete layer;
	layer=new NONS_Layer(&dst,0);
	manualBlit(src,&srcRect,layer->data,&dst);
}

NONS_TextButton::NONS_TextButton(
		const std::wstring &text,
		const NONS_FontCache &fc,
		float center,
		const SDL_Color &on,
		const SDL_Color &off,
		bool shadow,
		int limitX,
		int limitY)
		:NONS_SurfaceButton(),font_cache(fc FONTCACHE_DEBUG_PARAMETERS){
	SDL_Color black={0,0,0,0};
	this->limitX=limitX;
	this->limitY=limitY;
	int offsetX,
		offsetY;
	this->box=this->GetBoundingBox(text,&this->font_cache,limitX,limitY,offsetX,offsetY);
	this->setOffLayer()=new NONS_Layer(&this->box.to_SDL_Rect(),0);
	this->setOffLayer()->MakeTextLayer(this->font_cache,off);
	this->setOnLayer()=new NONS_Layer(&this->box.to_SDL_Rect(),0);
	this->setOnLayer()->MakeTextLayer(this->font_cache,on);
	if (shadow){
		this->setShadowLayer()=new NONS_Layer(&this->box.to_SDL_Rect(),0);
		this->setShadowLayer()->MakeTextLayer(this->font_cache,black);
		this->box.w++;
		this->box.h++;
	}else
		this->setShadowLayer()=0;
	this->write(text,offsetX,offsetY,center);
}

NONS_TextButton::~NONS_TextButton(){
	delete this->getShadowLayer();
}

SDL_Rect NONS_TextButton::GetBoundingBox(const std::wstring &str,NONS_FontCache *cache,int limitX,int limitY,int &offsetX,int &offsetY){
	std::vector<NONS_Glyph *> outputBuffer;
	long lastSpace=-1;
	int x0=0,
		y0=0,
		wordL=0,
		//width=0,
		//minheight=INT_MAX,
		//height=0,
		lineSkip=this->font_cache.font_line_skip/*,
		fontLineSkip=this->font_cache->font_line_skip*/;
	if (!cache)
		cache=this->getOffLayer()->fontCache;
	SDL_Rect frame={0,0,this->limitX,this->limitY};
	int minx=INT_MAX,
		miny=INT_MAX,
		maxx=INT_MIN,
		maxy=INT_MIN;
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
					outputBuffer.insert(outputBuffer.begin()+lastSpace+1,(NONS_Glyph *)0);
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
					outputBuffer.insert(outputBuffer.begin()+lastSpace+1,(NONS_Glyph *)0);
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
			x0=0;
			y0+=lineSkip;
			continue;
		}
		SDL_Rect r=outputBuffer[a]->get_put_bounding_box(x0,y0);
		int advance=outputBuffer[a]->get_advance();

		minx=std::min(minx,(int)r.x);
		miny=std::min(miny,(int)r.y);
		maxx=std::max(maxx,(int)r.x+(int)r.w);
		maxx=std::max(maxx,x0+advance);
		maxy=std::max(maxy,(int)r.y+(int)r.h);
		maxy=std::max(maxy,y0+lineSkip);

		if (x0+advance>frame.w){
			x0=0;
			y0+=lineSkip;
		}
		x0+=advance;
		outputBuffer[a]->done();
	}
	offsetX=(minx<0)?-minx:0;
	offsetY=(miny<0)?-miny:0;
	SDL_Rect res={0,0,maxx+offsetX,maxy+offsetY};
	return res;
}

#define CHECK_POINTER_AND_CALL(p,c) if (p) p->c

void NONS_TextButton::write(const std::wstring &str,int offsetX,int offsetY,float center){
	/*SDL_FillRect(this->getOnLayer()->data,0,gmask|amask);
	SDL_FillRect(this->getOffLayer()->data,0,gmask|amask);
	if (this->getShadowLayer())
		SDL_FillRect(this->getShadowLayer()->data,0,gmask|amask);*/
	std::vector<NONS_Glyph *> outputBuffer;
	std::vector<NONS_Glyph *> outputBuffer2;
	std::vector<NONS_Glyph *> outputBuffer3;
	long lastSpace=-1;
	int x0=offsetX,
		y0=offsetY;
	int wordL=0;
	SDL_Rect frame={
		0,
		(Sint16)-this->box.y,
		(Uint16)this->box.w,
		(Uint16)this->box.h
	};
	int lineSkip=this->getOffLayer()->fontCache->line_skip;
	SDL_Rect screenFrame={
		0,0,
		(Uint16)this->limitX,
		(Uint16)this->limitY
	};
	for (size_t a=0;a<str.size();a++){
		wchar_t c=str[a];
		NONS_Glyph *glyph=this->getOffLayer()->fontCache->getGlyph(c);
		NONS_Glyph *glyph2=this->getOnLayer()->fontCache->getGlyph(c);
		NONS_Glyph *glyph3=0;
		glyph3=(this->getShadowLayer())?this->getShadowLayer()->fontCache->getGlyph(c):0;
		if (c=='\n'){
			outputBuffer.push_back(0);
			outputBuffer2.push_back(0);
			outputBuffer3.push_back(0);
			if (x0+wordL>=screenFrame.w && lastSpace>=0){
				if (isbreakspace(outputBuffer[lastSpace]->get_codepoint())){
					outputBuffer[lastSpace]->done();
					outputBuffer2[lastSpace]->done();
					CHECK_POINTER_AND_CALL(outputBuffer3[lastSpace],done());
					outputBuffer[lastSpace]=0;
					outputBuffer2[lastSpace]=0;
					outputBuffer3[lastSpace]=0;
				}else{
					outputBuffer.insert(outputBuffer.begin()+lastSpace+1,(NONS_Glyph *)0);
					outputBuffer2.insert(outputBuffer2.begin()+lastSpace+1,(NONS_Glyph *)0);
					outputBuffer3.insert(outputBuffer3.begin()+lastSpace+1,(NONS_Glyph *)0);
				}
				lastSpace=-1;
				y0+=lineSkip;
			}
			lastSpace=-1;
			x0=offsetX;
			y0+=lineSkip;
			wordL=0;
		}else if (isbreakspace(c)){
			if (x0+wordL>=screenFrame.w && lastSpace>=0){
				if (isbreakspace(outputBuffer[lastSpace]->get_codepoint())){
					outputBuffer[lastSpace]->done();
					outputBuffer2[lastSpace]->done();
					CHECK_POINTER_AND_CALL(outputBuffer3[lastSpace],done());
					outputBuffer[lastSpace]=0;
					outputBuffer2[lastSpace]=0;
					outputBuffer3[lastSpace]=0;
				}else{
					outputBuffer.insert(outputBuffer.begin()+lastSpace+1,(NONS_Glyph *)0);
					outputBuffer2.insert(outputBuffer2.begin()+lastSpace+1,(NONS_Glyph *)0);
					outputBuffer3.insert(outputBuffer3.begin()+lastSpace+1,(NONS_Glyph *)0);
				}
				lastSpace=-1;
				x0=offsetX;
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
		CHECK_POINTER_AND_CALL(outputBuffer3[lastSpace],done());
		outputBuffer[lastSpace]=0;
		outputBuffer2[lastSpace]=0;
		outputBuffer3[lastSpace]=0;
	}
	x0=this->setLineStart(&outputBuffer,0,&screenFrame,center,offsetX);
	y0=0;
	for (ulong a=0;a<outputBuffer.size();a++){
		if (!outputBuffer[a]){
			x0=this->setLineStart(&outputBuffer,a,&screenFrame,center,offsetX);
			y0+=lineSkip;
			continue;
		}
		int advance=outputBuffer[a]->get_advance();
		if (x0+advance>screenFrame.w){
			x0=this->setLineStart(&outputBuffer,a,&frame,center,offsetX);
			y0+=lineSkip;
		}
		outputBuffer[a]->put(this->getOffLayer()->data,x0,y0);
		outputBuffer2[a]->put(this->getOnLayer()->data,x0,y0);
		outputBuffer[a]->done();
		outputBuffer2[a]->done();
		if (this->getShadowLayer()){
			outputBuffer3[a]->put(this->getShadowLayer()->data,x0,y0);
			outputBuffer3[a]->done();
		}
		x0+=advance;
	}
}

int NONS_TextButton::setLineStart(std::vector<NONS_Glyph *> *arr,long start,SDL_Rect *frame,float center,int offsetX){
	while (!(*arr)[start])
		start++;
	int width=this->predictLineLength(arr,start,frame->w,offsetX);
	float factor=(center<=0.5f)?center:1.0f-center;
	int pixelcenter=int(float(frame->w)*factor);
	return int((width/2.f>pixelcenter)?(frame->w-width)*(center>0.5f):frame->w*center-width/2.f)+offsetX;
}

int NONS_TextButton::predictLineLength(std::vector<NONS_Glyph *> *arr,long start,int width,int offsetX){
	int res=0;
	for (ulong a=start;a<arr->size() && (*arr)[a] && res+(*arr)[a]->get_advance()<=width;a++)
		res+=(*arr)[a]->get_advance();
	return res;
}

void NONS_SpriteButton::mergeWithoutUpdate_inner(NONS_VirtualScreen *dst,SDL_Rect *dstRect,SDL_Rect *srcRect){
	srcRect->x=(!this->status)?0:srcRect->w;
	NONS_Layer *layer=this->screen->layerStack[this->sprite];
	SDL_Surface *surface=layer->data;
	manualBlit(surface,srcRect,dst->screens[VIRTUAL],dstRect,layer->alpha);
}

void NONS_SpriteButton::mergeWithoutUpdate(NONS_VirtualScreen *dst,SDL_Surface *original,bool status,bool force){
	NONS_Layer *layer=this->screen->layerStack[this->sprite];
	NONS_Button::mergeWithoutUpdate(dst,layer->clip_rect,(int)layer->position.x,(int)layer->position.y,original,status,force);
}

void NONS_SpriteButton::merge(NONS_VirtualScreen *dst,SDL_Surface *original,bool status,bool force){
	NONS_Layer *layer=this->screen->layerStack[this->sprite];
	NONS_Button::merge(dst,layer->clip_rect,(int)layer->position.x,(int)layer->position.y,original,status,force);
}

bool NONS_SpriteButton::MouseOver(int x,int y){
	NONS_Layer *layer=this->screen->layerStack[this->sprite];
	return MOUSE_OVER(x,y,layer->position.x,layer->position.y,layer->clip_rect.w,layer->clip_rect.h);
}

NONS_Rect NONS_SpriteButton::get_dimensions(){
	NONS_Layer *layer=this->screen->layerStack[this->sprite];
	return NONS_Rect(layer->position.x,layer->position.y,layer->clip_rect.w,layer->clip_rect.h);
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
	this->return_on_down=0;
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
	this->return_on_down=0;
}

NONS_ButtonLayer::~NONS_ButtonLayer(){
	for (ulong a=0;a<this->buttons.size();a++)
		if (this->buttons[a])
			delete this->buttons[a];
	if (this->loadedGraphic && !ImageLoader.unfetchImage(this->loadedGraphic))
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
		int width,
		int height){
	if (!this->font_cache)
		return;
	for (ulong a=0;a<this->buttons.size();a++)
		delete this->buttons[a];
	this->buttons.clear();
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
		NONS_TextButton *button=new NONS_TextButton(arr[a],*this->font_cache,0,on,off,shadow,width,height);
		this->buttons.push_back(button);
		this->boundingBox.h+=(Uint16)button->getBox().h;
		if (button->getBox().w>this->boundingBox.w)
			this->boundingBox.w=(Uint16)button->getBox().w;
	}
}

int NONS_ButtonLayer::getUserInput(int x,int y){
	if (!this->buttons.size())
		return -1;
	for (ulong a=0;a<this->buttons.size();a++){
		NONS_TextButton *cB=(NONS_TextButton *)this->buttons[a];
		cB->setPosx()=x;
		cB->setPosy()=y;
		y+=int(cB->getBox().y+cB->getBox().h);
	}
	if (y>this->screen->output->y0+this->screen->output->h)
		return -2;
	if (this->voiceEntry.size())
		this->audio->playSoundAsync(&this->voiceEntry,7,0);
	if (this->voiceMouseOver.size())
		this->audio->loadAsyncBuffer(this->voiceEntry,7);
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
		if (event.type==SDL_KEYDOWN && (/*event.key.keysym.sym==SDLK_UP || */event.key.keysym.sym==SDLK_PAGEUP) ||
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
					case SDLK_UP:
					case SDLK_DOWN:
						this->react_to_updown(mouseOver,event.key.keysym.sym,screenCopy);
						if (this->voiceMouseOver.size())
							this->audio->playSoundAsync(&this->voiceMouseOver,7,0);
						break;
					case SDLK_RETURN:
						if (this->react_to_click(mouseOver,screenCopy))
							return mouseOver;
						break;
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
				break;
			case SDL_MOUSEMOTION:
				if (this->react_to_movement(mouseOver,&event,screenCopy) && this->voiceMouseOver.size())
					this->audio->playSoundAsync(&this->voiceMouseOver,7,0);
				break;
			case SDL_MOUSEBUTTONDOWN:
				if (this->react_to_click(mouseOver,screenCopy))
					return mouseOver;
				break;
		}
	}
}

bool NONS_ButtonLayer::react_to_movement(int &mouseOver,SDL_Event *event,SDL_Surface *screenCopy){
	if (mouseOver>=0 && this->buttons[mouseOver]->MouseOver(event))
		return 0;
	int tempMO=-1;
	for (ulong a=0;a<this->buttons.size() && tempMO==-1;a++)
		if (this->buttons[a] && this->buttons[a]->MouseOver(event))
			tempMO=a;
	if (tempMO<0){
		if (mouseOver>=0)
			this->buttons[mouseOver]->merge(this->screen->screen,screenCopy,0);
		mouseOver=-1;
		return 0;
	}
	if (mouseOver>=0)
		this->buttons[mouseOver]->merge(this->screen->screen,screenCopy,0);
	mouseOver=tempMO;
	this->buttons[mouseOver]->merge(this->screen->screen,screenCopy,1);
	return 1;
}

void NONS_ButtonLayer::react_to_updown(int &mouseOver,SDLKey key,SDL_Surface *screenCopy){
	if (mouseOver>=0)
		this->buttons[mouseOver]->merge(this->screen->screen,screenCopy,0);
	mouseOver+=(key==SDLK_UP)?-1:1;
	if (mouseOver<=-1)
		mouseOver=this->buttons.size()-1;
	else if ((ulong)mouseOver>=this->buttons.size())
		mouseOver=0;
	NONS_Button *button=this->buttons[mouseOver];
	button->merge(this->screen->screen,screenCopy,1);
	NONS_Rect rect=button->get_dimensions();
	rect.x=rect.x+rect.w/2.f;
	rect.y=rect.y+rect.h/2.f;
	SDL_WarpMouse((Uint16)rect.x,(Uint16)rect.y);
}

bool NONS_ButtonLayer::react_to_click(int &mouseOver,SDL_Surface *screenCopy){
	if (mouseOver<0)
		return 0;
	if (this->voiceClick.size())
		this->audio->playSoundAsync(&this->voiceClick,7,0);
	{
		NONS_MutexLocker ml(screenMutex);
		manualBlit(screenCopy,0,this->screen->screen->screens[VIRTUAL],0);
	}
	this->screen->screen->updateWholeScreen();
	SDL_FreeSurface(screenCopy);
	return 1;
}

void NONS_ButtonLayer::addImageButton(ulong index,int posx,int posy,int width,int height,int originX,int originY){
	if (this->buttons.size()<index+1)
		this->buttons.resize(index+1,0);
	else if (this->buttons[index])
		delete this->buttons[index];
	this->buttons[index]=new NONS_GraphicButton(this->loadedGraphic,posx,posy,width,height,originX,originY);
}

void NONS_ButtonLayer::addSpriteButton(ulong index,ulong sprite){
	if (this->buttons.size()<index+1)
		this->buttons.resize(index+1,0);
	else if (this->buttons[index])
		delete this->buttons[index];
	this->buttons[index]=new NONS_SpriteButton(sprite,this->screen);
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
	this->screen->BlendNoText(0);
	this->screen->copyBufferToScreenWithoutUpdating();
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
					this->react_to_movement(mouseOver,&event,screenCopy);
					break;
				case SDL_MOUSEBUTTONUP:
				case SDL_MOUSEBUTTONDOWN:
					if (!this->return_on_down && event.type==SDL_MOUSEBUTTONUP || this->return_on_down && event.type==SDL_MOUSEBUTTONDOWN){
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
						if (button==1)
							return (mouseOver<0)?-1:mouseOver;
						else
							return -button;
					}
					break;
				case SDL_KEYDOWN:
					{
						SDLKey key=event.key.keysym.sym;
						switch (key){
							case SDLK_PAUSE:
								console.enter(this->screen);
								if (!queue.emptify()){
									SDL_FreeSurface(screenCopy);
									return INT_MIN;
								}
								break;
							case SDLK_UP:
							case SDLK_DOWN:
								if (!*key_bool_map[key].second){
									this->react_to_updown(mouseOver,key,screenCopy);
									break;
								}
							case SDLK_RETURN:
								if (key==SDLK_RETURN && !*key_bool_map[key].second){
									SDL_FreeSurface(screenCopy);
									this->screen->screen->updateWholeScreen();
									return (mouseOver<0)?-1:mouseOver;
								}
							default:
								{
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
	this->buttons->makeTextButtons(
		this->strings,
		this->on,
		this->off,
		this->shadow,
		&this->voiceEntry,
		&this->voiceMO,
		&this->voiceClick,
		this->audio,
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
		if (choice!=INT_MIN && this->voiceCancel.size())
			this->audio->playSoundAsync(&this->voiceCancel,7,0);
		return 0;
	}
	return this->call(this->commands[choice]);
}

std::wstring getDefaultFontFilename();

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
		0,0,0,0,
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
		0,0,0,0,
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
			w,h);
		choice=files.getUserInput((w-files.boundingBox.w)/2,y0*2+20);
		if (choice==INT_MIN)
			ret=INT_MIN;
		else{
			if (choice==-2){
				this->slots--;
				continue;
			}
			if (choice<0 && this->voiceCancel.size())
				this->audio->playSoundAsync(&this->voiceCancel,7,0);
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
			if (choice<0 && this->voiceCancel.size())
				this->audio->playSoundAsync(&this->voiceCancel,7,0);
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
	if (FT_Init_FreeType(&this->library)){
		//throw std::string("FT_Init_FreeType() has failed!");
		exit(1);
	}
}

NONS_FreeType_Lib::~NONS_FreeType_Lib(){
	FT_Done_FreeType(this->library);
}

ulong NONS_FT_Stream_IoFunc(FT_Stream s,ulong offset,uchar *buffer,ulong count){
	NONS_DataStream *stream=(NONS_DataStream *)(s->descriptor.pointer);
	if (!count)
		return 0;
	ulong o=(ulong)stream->seek(offset,1);
	size_t a=0;
	//if (count){
		a=(size_t)count;
		if (!stream->read(buffer,a,a))
			a=0;
		o+=a;
	//}
	//s->pos=o;
	return (ulong)a;
}

void NONS_FT_Stream_CloseFunc(FT_Stream s){
	NONS_DataStream *stream=(NONS_DataStream *)(s->descriptor.pointer);
	general_archive.close(stream);
}

NONS_Font::NONS_Font(const std::wstring &filename){
	this->error=1;
	NONS_DataStream *stream=general_archive.open(filename);
	if (!stream)
		return;
	this->stream=new FT_StreamRec;
	memset(this->stream,0,sizeof(*this->stream));
	this->stream->descriptor.pointer=stream;
	this->stream->read=NONS_FT_Stream_IoFunc;
	this->stream->close=NONS_FT_Stream_CloseFunc;
	this->stream->size=(size_t)stream->get_size();
	FT_Open_Args args;
	args.flags=FT_OPEN_STREAM;
	args.stream=this->stream;
	this->error=FT_Open_Face(NONS_FreeType_Lib::instance.get_lib(),&args,0,&this->ft_font);
	if (!this->good())
		general_archive.close(stream);
	this->size=0;
}

NONS_Font::~NONS_Font(){
	if (this->good())
		FT_Done_Face(this->ft_font);
	delete this->stream;
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

FT_GlyphSlot NONS_Font::get_glyph(wchar_t codepoint,bool italic,bool bold) const{
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
		FT_Outline_Embolden(&this->ft_font->glyph->outline,FT_Pos(this->size*177/100)); //32 for every 18 of this->size
	return this->ft_font->glyph;
}

FT_GlyphSlot NONS_Font::render_glyph(wchar_t codepoint,bool italic,bool bold) const{
	return this->render_glyph(this->get_glyph(codepoint,italic,bold));
}

FT_GlyphSlot NONS_Font::render_glyph(FT_GlyphSlot slot) const{
	FT_Render_Glyph(slot,FT_RENDER_MODE_LIGHT);
	return this->ft_font->glyph;
}

struct Span{
	int x, y, w, alpha;
	Span(){}
	Span(int x,int y,int w,int coverage):x(x),y(y),w(w),alpha(coverage){}
};

typedef std::vector<Span> Spans;

void RasterCallback(int y,int count,const FT_Span *spans,void *user) {
	for (int i=0;i<count;i++){
		Span s(spans[i].x,y,spans[i].len,spans[i].coverage);
		((Spans *)user)->push_back(s);
	}
}

void RenderSpans(FT_Library library,FT_Outline *outline,Spans *spans){
	FT_Raster_Params params;
	memset(&params,0,sizeof(params));
	params.flags=FT_RASTER_FLAG_AA|FT_RASTER_FLAG_DIRECT;
	params.gray_spans=RasterCallback;
	params.user=spans;
	FT_Outline_Render(library,outline,&params);
}

uchar *render_glyph(SDL_Rect &box,FT_Glyph &glyph,int ascent,float outline_size){
	FT_Stroker stroker;
	FT_Stroker_New(NONS_FreeType_Lib::instance.get_lib(),&stroker);
	FT_Stroker_Set(stroker,FT_Fixed(outline_size)*64,FT_STROKER_LINECAP_ROUND,FT_STROKER_LINEJOIN_BEVEL,0);
	FT_Glyph_StrokeBorder(&glyph,stroker,0,1);

	Spans outlineSpans;
	RenderSpans(NONS_FreeType_Lib::instance.get_lib(),&FT_OutlineGlyph(glyph)->outline,&outlineSpans);
	FT_Stroker_Done(stroker);

	if (!outlineSpans.size())
		return 0;
	int minx=outlineSpans.front().x,
		miny=outlineSpans.front().y,
		maxx=outlineSpans.front().x+outlineSpans.front().w,
		maxy=outlineSpans.front().y;
	for (size_t a=1;a<outlineSpans.size();a++){
		minx=std::min(minx,outlineSpans[a].x);
		miny=std::min(miny,outlineSpans[a].y);
		maxx=std::max(maxx,outlineSpans[a].x+outlineSpans[a].w);
		maxy=std::max(maxy,outlineSpans[a].y);
	}
	box.x=minx;
	box.y=ascent-maxy-1;
	box.w=maxx-minx+1;
	box.h=maxy-miny+1;
	size_t bmsize=box.w*box.h;
	if (!bmsize)
		return 0;
	uchar *bitmap=new uchar[bmsize];
	memset(bitmap,0,bmsize);
	for (size_t a=0;a<outlineSpans.size();a++){
		Span &span=outlineSpans[a];
		ulong element=span.x-minx+(box.h-(span.y-miny)-1)*box.w;
		uchar *dst=bitmap+element;
		for (int b=0;b<span.w;b++)
			*dst++=(uchar)span.alpha;
	}
	return bitmap;
}

NONS_Glyph::NONS_Glyph(NONS_FontCache &fc,wchar_t codepoint,ulong size,const SDL_Color &color,bool italic,bool bold,ulong outline_size,const SDL_Color &outline_color):fc(fc){
	this->codepoint=codepoint;
	this->size=size;
	this->outline_size=outline_size;
	this->color=color;
	this->outline_color=outline_color;
	this->refCount=0;
	this->italic=italic;
	this->bold=bold;
	NONS_Font &font=fc.get_font();
	font.set_size(size);

	FT_GlyphSlot glyph_slot;
	glyph_slot=font.get_glyph(codepoint,italic,bold);
	if (outline_size){
		FT_Glyph glyph;
		FT_Get_Glyph(glyph_slot,&glyph);
		this->outline_base_bitmap=render_glyph(this->outline_bounding_box,glyph,fc.ascent,float(outline_size)/18.f*float(size));
		FT_Done_Glyph(glyph);
	}else
		this->outline_base_bitmap=0;
	
	glyph_slot=font.render_glyph(glyph_slot);
	FT_Bitmap &bitmap=glyph_slot->bitmap;
	ulong width=bitmap.width,
		height=bitmap.rows;
	uchar *dst=this->base_bitmap=new uchar[1+width*height];
	for (ulong y=0;y<(ulong)bitmap.rows;y++){
		uchar *src=bitmap.buffer+y*bitmap.pitch;
		for (ulong x=0;x<(ulong)bitmap.width;x++)
			*dst++=*src++;
	}

	this->bounding_box.x=glyph_slot->bitmap_left;
	this->bounding_box.y=Sint16(font.ascent-glyph_slot->bitmap_top);
	this->bounding_box.w=(Uint16)width;
	this->bounding_box.h=(Uint16)height;
	this->advance=FT_CEIL(glyph_slot->metrics.horiAdvance);

	if (outline_size && !this->outline_base_bitmap){
		this->outline_bounding_box=this->bounding_box;
		size_t buffer_size=this->outline_bounding_box.w*this->outline_bounding_box.h+1;
		this->outline_base_bitmap=new uchar[buffer_size];
		memset(this->outline_base_bitmap,0,buffer_size);
	}
}

NONS_Glyph::~NONS_Glyph(){
	delete[] this->base_bitmap;
	delete[] this->outline_base_bitmap;
}

inline bool operator==(const SDL_Color &a,const SDL_Color &b){
	return a.r==b.r && a.g==b.g && a.b==b.b;
}
inline bool operator!=(const SDL_Color &a,const SDL_Color &b){ return !(a==b); }

bool NONS_Glyph::needs_redraw(ulong size,bool italic,bool bold,ulong outline_size) const{
	return (this->size!=size || this->italic!=italic || this->bold!=bold || this->outline_size!=outline_size);
}

long NONS_Glyph::get_advance(){
	return long(this->advance)+this->fc.spacing;
}

void put_glyph(SDL_Surface *dst,int x,int y,uchar alpha,uchar *src,const SDL_Rect &box,const SDL_Color &color){
	x+=box.x;
	y+=box.y;
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
	uchar r0=color.r,
		g0=color.g,
		b0=color.b;
	for (ulong src_y=y0,dst_y=y;src_y<box.h && dst_y<sd.h;src_y++,dst_y++){
		uchar *dst_p=sd.pixels+dst_y*sd.pitch+x*sd.advance;
		src+=x0;
		for (ulong src_x=x0,dst_x=x;src_x<box.w && dst_x<sd.w;src_x++,dst_x++){
			uchar a0=*src,
				*r1=dst_p+sd.Roffset,
				*g1=dst_p+sd.Goffset,
				*b1=dst_p+sd.Boffset,
				*a1=dst_p+sd.Aoffset;

			do_alpha_blend(r1,g1,b1,a1,r0,g0,b0,a0,sd.alpha,1,alpha);

			src++;
			dst_p+=sd.advance;
		}
	}
	SDL_UnlockSurface(dst);
}

void NONS_Glyph::put(SDL_Surface *dst,int x,int y,uchar alpha){
	if (this->outline_base_bitmap)
		put_glyph(dst,x,y,alpha,this->outline_base_bitmap,this->outline_bounding_box,this->outline_color);
	if (this->base_bitmap)
		put_glyph(dst,x,y,alpha,this->base_bitmap,this->bounding_box,this->color);
}

void NONS_Glyph::done(){
	this->fc.done(this);
}

#ifndef _DEBUG
NONS_FontCache::NONS_FontCache(NONS_Font &f,ulong size,const SDL_Color &color,bool italic,bool bold,ulong outline_size,const SDL_Color &outline_color):font(f){
#else
NONS_FontCache::NONS_FontCache(NONS_Font &f,ulong size,const SDL_Color &color,bool italic,bool bold,ulong outline_size,const SDL_Color &outline_color,const char *file,ulong line):font(f){
	this->declared_in=file;
	this->line=line;
#endif
	this->setColor(color);
	this->setOutlineColor(outline_color);
	this->resetStyle(size,italic,bold,outline_size);
	this->spacing=0;
}

#ifndef _DEBUG
NONS_FontCache::NONS_FontCache(const NONS_FontCache &fc):font(fc.font){
#else
NONS_FontCache::NONS_FontCache(const NONS_FontCache &fc,const char *file,ulong line):font(fc.font){
	this->declared_in=file;
	this->line=line;
#endif
	this->setColor(fc.color);
	this->setOutlineColor(fc.outline_color);
	this->resetStyle(fc.size,fc.italic,fc.bold,fc.outline_size);
	this->spacing=fc.spacing;
	this->line_skip=fc.line_skip;
	this->font_line_skip=fc.font_line_skip;
	this->ascent=fc.ascent;
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
	this->font_line_skip=this->line_skip=this->font.line_skip;
	this->ascent=this->font.ascent;
}

void NONS_FontCache::resetStyle(ulong size,bool italic,bool bold,ulong outline_size){
	this->set_size(size);
	this->set_italic(italic);
	this->set_bold(bold);
	this->set_outline_size(outline_size);
}

NONS_Glyph *NONS_FontCache::getGlyph(wchar_t c){
	NONS_CommandLineOptions::replaceArray_t::iterator i=CLOptions.replaceArray.find(c);
	if (i!=CLOptions.replaceArray.end())
		c=i->second;
	if (c<32)
		return 0;
	bool must_render=(this->glyphs.find(c)==this->glyphs.end());
	NONS_Glyph *&g=this->glyphs[c];
	if (!must_render && g->needs_redraw(this->size,this->italic,this->bold,this->outline_size)){
		if (!g->refCount)
			delete g;
		else
			this->garbage.insert(g);
		must_render=1;
	}
	if (must_render)
		g=new NONS_Glyph(*this,c,this->size,this->color,this->italic,this->bold,this->outline_size,this->outline_color);
	g->setColor(this->color);
	g->setOutlineColor(this->outline_color);
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

NONS_Font *init_font(const std::wstring &filename){
	NONS_Font *font=new NONS_Font(filename);
	if (!font->good()){
		delete font;
		return 0;
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

std::wstring getConsoleFontFilename(){
	if (!settings.exists(L"console font"))
		settings.assignWString(L"console font",L"cour.ttf");
	return settings.getWString(L"console font");
}

void NONS_DebuggingConsole::init(){
	if (!this->font){
		std::wstring font=getConsoleFontFilename();
		this->font=init_font(font);
		if (!this->font){
			o_stderr <<"The font \""<<font<<"\" could not be found. The debugging console will not be available.\n";
		}else{
			SDL_Color color={0xFF,0xFF,0xFF,0xFF};
			this->cache=new NONS_FontCache(*this->font,15,color,0,0,0,color FONTCACHE_DEBUG_PARAMETERS);
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
