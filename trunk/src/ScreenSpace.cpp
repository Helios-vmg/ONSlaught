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

#include "ScreenSpace.h"
#include "IOFunctions.h"
#include "CommandLineOptions.h"

#define SCREENBUFFER_BITS 32

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
const int rmask=0xFF000000;
const int gmask=0x00FF0000;
const int bmask=0x0000FF00;
const int amask=0x000000FF;
const int rshift=24;
const int gshift=16;
const int bshift=8;
const int ashift=0;
#else
const int rmask=0x000000FF;
const int gmask=0x0000FF00;
const int bmask=0x00FF0000;
const int amask=0xFF000000;
const int rshift=0;
const int gshift=8;
const int bshift=16;
const int ashift=24;
#endif

NONS_Layer::NONS_Layer(SDL_Rect *size,unsigned rgba){
	this->data=makeSurface(size->w,size->h,32);
	SDL_FillRect(this->data,0,rgba);
	this->defaultShade=rgba;
	this->fontCache=0;
	this->visible=1;
	this->useDataAsDefaultShade=0;
	this->alpha=0xFF;
	this->clip_rect=this->data->clip_rect;
	this->clip_rect.x=size->x;
	this->clip_rect.y=size->y;
	this->position.x=0;
	this->position.y=0;
	this->position.w=0;
	this->position.h=0;
}

NONS_Layer::NONS_Layer(SDL_Surface *img,unsigned rgba){
	this->data=img;
	this->defaultShade=rgba;
	this->fontCache=0;
	this->visible=1;
	this->useDataAsDefaultShade=0;
	this->alpha=0xFF;
	this->clip_rect=this->data->clip_rect;
	this->position.x=0;
	this->position.y=0;
	this->position.w=0;
	this->position.h=0;
}

NONS_Layer::NONS_Layer(const std::wstring *string){
	this->defaultShade=0;
	this->fontCache=0;
	this->visible=1;
	this->useDataAsDefaultShade=0;
	this->alpha=0xFF;
	this->data=0;
	this->load(string);
	this->position.x=0;
	this->position.y=0;
	this->position.w=0;
	this->position.h=0;
}

NONS_Layer::~NONS_Layer(){
	this->unload();
	if (this->fontCache)
		delete this->fontCache;
}

void NONS_Layer::MakeTextLayer(NONS_FontCache &fc,const SDL_Color &foreground){
	this->fontCache=new NONS_FontCache(fc FONTCACHE_DEBUG_PARAMETERS);
	this->fontCache->setColor(foreground);
}

void NONS_Layer::Clear(){
	if (!this->useDataAsDefaultShade){
		this->load((const std::wstring *)0);
		SDL_FillRect(this->data,0,this->defaultShade);
	}
}

void NONS_Layer::setShade(uchar r,uchar g,uchar b){
	if (!this->data)
		this->data=makeSurface((ulong)this->clip_rect.w,(ulong)this->clip_rect.h,32);
	SDL_PixelFormat *format=this->data->format;
	unsigned r0=r,
		g0=g,
		b0=b,
		rgb=(0xFF<<(format->Ashift))|(r0<<(format->Rshift))|(g0<<(format->Gshift))|(b0<<(format->Bshift));
	this->defaultShade=rgb;
}

void NONS_Layer::usePicAsDefaultShade(SDL_Surface *pic){
	if (this->data)
		SDL_FreeSurface(this->data);
	this->data=pic;
	this->useDataAsDefaultShade=1;
}

#include <iostream>

bool NONS_Layer::load(const std::wstring *string){
	if (!string){
		int w=this->data->w,
			h=this->data->h;
		if (this->unload(1)){
			this->data=makeSurface(w,h,32);
			this->clip_rect=this->data->clip_rect;
		}
		return 1;
	}
	this->unload();
	if (!ImageLoader->fetchSprite(this->data,*string,&this->optimized_updates)){
		SDL_FillRect(this->data,0,this->defaultShade);
		this->clip_rect=this->data->clip_rect;
		return 0;
	}
	this->animation.parse(*string);
	this->clip_rect=this->data->clip_rect;
	/*if (this->animation.animation_length>1){
		ulong t0=SDL_GetTicks();
		SDL_Rect rect=this->getUpdateRect(0,1);
		ulong t1=SDL_GetTicks();
		std::cout <<"completed in "<<t1-t0<<" msec."<<std::endl;
	}*/
	return 1;
}

bool NONS_Layer::load(SDL_Surface *src){
	this->unload();
	this->data=makeSurface(src->w,src->h,32);
	//SDL_FillRect(this->data,0,gmask|amask);
	manualBlit(src,0,this->data,0);
	return 1;
}

bool NONS_Layer::unload(bool youCantTouchThis){
	if (!this || !this->data)
		return 1;
	if (ImageLoader->unfetchImage(this->data)){
		this->data=0;
		this->optimized_updates.clear();
		return 1;
	}else if (!youCantTouchThis){
		SDL_FreeSurface(this->data);
		this->data=0;
		this->optimized_updates.clear();
		return 1;
	}
	return 0;
}

bool NONS_Layer::advanceAnimation(ulong msec){
	long frame=this->animation.advanceAnimation(msec);
	if (frame<0)
		return 0;
	this->clip_rect.x=Sint16(frame*this->clip_rect.w);
	return 1;
}

void NONS_Layer::centerAround(int x){
	this->position.x=x-this->clip_rect.w/2;
}

void NONS_Layer::useBaseline(int y){
	this->position.y=y-this->clip_rect.h+1;
}

std::ofstream textDumpFile;

NONS_StandardOutput::NONS_StandardOutput(NONS_Layer *fgLayer,NONS_Layer *shadowLayer,NONS_Layer *shadeLayer){
	this->foregroundLayer=fgLayer;
	this->shadowLayer=shadowLayer;
	this->shadeLayer=shadeLayer;
	this->x=0;
	this->y=0;
	this->x0=0;
	this->y0=0;
	this->w=fgLayer->data->clip_rect.w;
	this->h=fgLayer->data->clip_rect.h;
	this->display_speed=0;
	this->extraAdvance=0;
	this->visible=0;
	this->transition=new NONS_GFX(1,0,0);
	this->log.reserve(50);
	this->horizontalCenterPolicy=0;
	this->verticalCenterPolicy=0;
	this->lastStart=-1;
	this->printingStarted=0;
	this->shadowPosX=this->shadowPosY=1;
	this->indentationLevel=0;
	this->maxLogPages=-1;
}

NONS_StandardOutput::NONS_StandardOutput(NONS_FontCache &fc,SDL_Rect *size,SDL_Rect *frame,bool shadow){
	SDL_Color rgb={255,255,255,0};
	SDL_Color rgb2={0,0,0,0};
	this->foregroundLayer=new NONS_Layer(size,0x00000000);
	this->foregroundLayer->MakeTextLayer(fc,rgb);
	if (shadow){
		this->shadowLayer=new NONS_Layer(size,0x00000000);
		this->shadowLayer->MakeTextLayer(fc,rgb2);
	}else
		this->shadowLayer=0;
	this->shadeLayer=new NONS_Layer(size,(0x99<<rshift)|(0x99<<gshift)|(0x99<<bshift));
	this->x=frame->x;
	this->y=frame->y;
	this->x0=frame->x;
	this->y0=frame->y;
	this->w=frame->w;
	this->h=frame->h;
	this->display_speed=0;
	this->extraAdvance=0;
	this->visible=0;
	this->transition=new NONS_GFX(1,0,0);
	this->log.reserve(50);
	this->horizontalCenterPolicy=0;
	this->verticalCenterPolicy=0;
	this->lastStart=-1;
	this->printingStarted=0;
	this->shadowPosX=this->shadowPosY=1;
	this->indentationLevel=0;
	this->maxLogPages=-1;
}

NONS_StandardOutput::~NONS_StandardOutput(){
	this->Clear();
	delete this->foregroundLayer;
	if (this->shadowLayer)
		delete this->shadowLayer;
	delete this->shadeLayer;
	if (!this->transition->stored)
		delete this->transition;
}

#define INDENTATION_CHARACTER 0x2003

ulong NONS_StandardOutput::getIndentationSize(){
	NONS_Glyph *glyph=this->foregroundLayer->fontCache->getGlyph(INDENTATION_CHARACTER);
	long advance=glyph->get_advance();
	glyph->done();
	return this->indentationLevel*(advance+this->extraAdvance);
}

bool NONS_StandardOutput::prepareForPrinting(std::wstring str){
	long lastSpace=-1;
	int x0=this->x,y0=this->y;
	int wordL=0;
	int lineSkip=this->foregroundLayer->fontCache->line_skip;
	this->resumePrinting=0;
	bool check_at_end=1;
	ulong indentationMargin=this->x0+this->getIndentationSize();
	for (ulong a=0;a<str.size();a++){
		wchar_t character=str[a];
		NONS_Glyph *glyph=this->foregroundLayer->fontCache->getGlyph(character);
		if (character=='\n'){
			this->cachedText.push_back(character);
			if (x0+wordL>=this->w+this->x0 && lastSpace>=0){
				this->cachedText[lastSpace]='\r';
				y0+=lineSkip;
			}
			lastSpace=-1;
			x0=this->x0;
			y0+=lineSkip;
			wordL=0;
		}else if (isbreakspace(character)){
			if (x0+wordL>this->w+this->x0 && lastSpace>=0){
				this->cachedText[lastSpace]='\r';
				lastSpace=-1;
				x0=indentationMargin;
				y0+=lineSkip;
			}
			x0+=wordL;
			lastSpace=this->cachedText.size();
			wordL=glyph->get_advance()+this->extraAdvance;
			this->cachedText.push_back(character);
		}else if (character){
			wordL+=glyph->get_advance()+this->extraAdvance;
			this->cachedText.push_back(character);
		}else{
			if (x0+wordL>=this->w+this->x0 && lastSpace>=0)
				this->cachedText[lastSpace]='\r';
			check_at_end=0;
			glyph->done();
			break;
		}
		glyph->done();
	}
	if (check_at_end && x0+wordL>=this->w+this->x0 && lastSpace>=0)
		this->cachedText[lastSpace]='\r';
	this->printingStarted=1;
	this->indent_next=0;
	if (this->verticalCenterPolicy>0 && this->currentBuffer.size()>0)
		return 1;
	SDL_Rect frame={this->x0,this->y0,this->w,this->h};
	if (this->verticalCenterPolicy)
		this->y=this->setTextStart(&this->cachedText,&frame,this->verticalCenterPolicy);
	else if (!this->currentBuffer.size())
		this->y=this->y0;
	this->prebufferedText.append(L"<y=");
	this->prebufferedText.append(itoaw(this->y));
	this->prebufferedText.push_back('>');
	this->set_italic(this->get_italic());
	this->set_bold(this->get_bold());
	return 0;
}

void NONS_StandardOutput::set_italic(bool i){
	this->foregroundLayer->fontCache->set_italic(i);
	if (this->shadowLayer)
		this->shadowLayer->fontCache->set_italic(i);
	if (i)
		this->prebufferedText.append(L"<italic>");
	else
		this->prebufferedText.append(L"</italic>");
}

void NONS_StandardOutput::set_bold(bool b){
	this->foregroundLayer->fontCache->set_bold(b);
	if (this->shadowLayer)
		this->shadowLayer->fontCache->set_bold(b);
	if (b)
		this->prebufferedText.append(L"<bold>");
	else
		this->prebufferedText.append(L"</bold>");
}

void NONS_StandardOutput::set_size(ulong size){
	this->foregroundLayer->fontCache->set_size(size);
	if (this->shadowLayer)
		this->shadowLayer->fontCache->set_size(size);
}

bool NONS_StandardOutput::print(ulong start,ulong end,NONS_VirtualScreen *dst,ulong *printedChars){
	if (start>=this->cachedText.size())
		return 0;
	NONS_EventQueue queue;
	bool enterPressed=0;
	int x0,
		y0=this->y;
	SDL_Rect frame={this->x0,this->y0,this->w,this->h};
	ulong indentationMargin=this->x0+this->getIndentationSize();
	if (this->x==this->x0){
		x0=this->setLineStart(&this->cachedText,start,&frame,this->horizontalCenterPolicy);
		if (this->indent_next && !this->horizontalCenterPolicy)
			x0=indentationMargin;
		if (x0!=this->lastStart){
			this->prebufferedText.append(L"<x=");
			this->prebufferedText.append(itoaw(x0));
			this->prebufferedText.push_back('>');
			this->lastStart=x0;
		}
	}else
		x0=this->x;
	y0=this->y;
	int lineSkip=this->foregroundLayer->fontCache->line_skip;
	int fontLineSkip=this->foregroundLayer->fontCache->font_line_skip;
	ulong t0,t1;
	if (this->resumePrinting)
		start=this->resumePrintingWhere;
	for (ulong a=start;a<end && a<this->cachedText.size();a++){
		t0=SDL_GetTicks();
		wchar_t character=this->cachedText[a];
		NONS_Glyph *glyph=this->foregroundLayer->fontCache->getGlyph(character);
		NONS_Glyph *glyph2=(this->shadowLayer)?this->shadowLayer->fontCache->getGlyph(character):0;
		if (!glyph){
			if (y0+lineSkip>=this->h+this->y0){
				this->resumePrinting=1;
				this->x=x0;
				this->y=y0;
				this->resumePrinting=1;
				this->resumePrintingWhere=a+1;
				this->currentBuffer.append(this->prebufferedText);
				this->prebufferedText.clear();
				this->indent_next=(character=='\r');
				return 1;
			}
			if (a<this->cachedText.size()-1){
				x0=this->setLineStart(&this->cachedText,a+1,&frame,this->horizontalCenterPolicy);
				if (character=='\r' && !this->horizontalCenterPolicy)
					x0=indentationMargin;
				if (x0!=this->lastStart){
					this->prebufferedText.append(L"<x=");
					this->prebufferedText.append(itoaw(x0));
					this->prebufferedText.push_back('>');
					this->lastStart=x0;
				}
			}else
				x0=this->x0;
			y0+=lineSkip;
			this->prebufferedText.push_back('\n');
			continue;
		}
		int advance=glyph->get_advance()+this->extraAdvance;
		if (x0+advance>this->w+this->x0){
			if (y0+lineSkip>=this->h+this->y0){
				this->resumePrinting=1;
				this->x=x0;
				this->y=y0;
				this->resumePrinting=1;
				this->resumePrintingWhere=isbreakspace(glyph->get_codepoint())?a+1:a;
				this->currentBuffer.append(this->prebufferedText);
				this->prebufferedText.clear();
				this->indent_next=1;
				glyph->done();
				glyph2->done();
				return 1;
			}else{
				x0=this->setLineStart(&this->cachedText,a,&frame,this->horizontalCenterPolicy);
				if (!this->horizontalCenterPolicy)
					x0=indentationMargin;
				if (x0!=this->lastStart){
					this->prebufferedText.append(L"<x=");
					this->prebufferedText.append(itoaw(x0));
					this->prebufferedText.push_back('>');
					this->lastStart=x0;
				}
				y0+=lineSkip;
				this->prebufferedText.push_back('\n');
			}
		}
		switch (glyph->get_codepoint()){
			case '\\':
				this->prebufferedText.append(L"\\\\");
				break;
			case '<':
				this->prebufferedText.append(L"\\<");
				break;
			case '>':
				this->prebufferedText.append(L"\\>");
				break;
			default:
				this->prebufferedText.push_back(glyph->get_codepoint());
		}
		{
			NONS_MutexLocker ml(screenMutex);
			if (glyph2){
				glyph2->put(
					this->shadowLayer->data,
					x0+this->shadowPosX-(int)this->shadowLayer->clip_rect.x,
					y0+this->shadowPosY-(int)this->shadowLayer->clip_rect.y
				);
				glyph2->put(
					dst->screens[VIRTUAL],
					x0+this->shadowPosX,
					y0+this->shadowPosY
				);
				glyph2->done();
			}
			glyph->put(
				this->foregroundLayer->data,
				x0-(int)this->foregroundLayer->clip_rect.x,
				y0-(int)this->foregroundLayer->clip_rect.y
			);
			glyph->put(dst->screens[VIRTUAL],x0,y0);
			glyph->done();
		}
		SDL_Rect r=glyph->get_put_bounding_box((Sint16)x0,(Sint16)y0);
		if (glyph2){
			long tempX=(this->shadowPosX<=0)?0:this->shadowPosX,
				tempY=(this->shadowPosY<=0)?0:this->shadowPosY;
			r.w+=(Uint16)tempX;
			r.h+=(Uint16)tempY;
		}
		dst->updateScreen(r.x,r.y,r.w,r.h);
		if (printedChars)
			(*printedChars)++;
		x0+=advance;
		while (!CURRENTLYSKIPPING && !enterPressed && !queue.empty()){
			SDL_Event event=queue.pop();
			if (event.type==SDL_KEYDOWN && (event.key.keysym.sym==SDLK_RETURN || event.key.keysym.sym==SDLK_SPACE))
				enterPressed=1;
		}
		t1=SDL_GetTicks();
		if (!CURRENTLYSKIPPING && !enterPressed && this->display_speed>t1-t0)
			SDL_Delay(this->display_speed-(t1-t0));
	}
	this->x=x0;
	this->y=y0;
	this->resumePrinting=0;
	this->resumePrintingWhere=0;
	return 0;
}

void NONS_StandardOutput::endPrinting(){
	if (this->printingStarted)
		this->currentBuffer.append(this->prebufferedText);
	this->prebufferedText.clear();
	this->cachedText.clear();
	this->printingStarted=0;
}

void NONS_StandardOutput::ephemeralOut(std::wstring *str,NONS_VirtualScreen *dst,bool update,bool writeToLayers,SDL_Color *col){
	int x=this->x0,
		y=this->y0;
	int lineSkip=this->foregroundLayer->fontCache->line_skip;
	if (writeToLayers){
		this->foregroundLayer->Clear();
		if (this->shadowLayer)
			this->shadowLayer->Clear();
	}
	long lastStart=this->x0;
	NONS_FontCache cache(*this->foregroundLayer->fontCache FONTCACHE_DEBUG_PARAMETERS),
		*shadow=(this->shadowLayer)?new NONS_FontCache(*this->shadowLayer->fontCache FONTCACHE_DEBUG_PARAMETERS):0;
	cache.set_to_normal();
	if (shadow)
		shadow->set_to_normal();
	if (col)
		cache.setColor(*col);
	for (ulong a=0;a<str->size();a++){
		wchar_t character=(*str)[a];
		if (character=='<'){
			std::wstring tagname=tagName(*str,a);
			if (tagname.size()){
				if (tagname==L"x"){
					std::wstring tagvalue=tagValue(*str,a);
					if (tagvalue.size())
						lastStart=x=atoi(tagvalue);
				}else if (tagname==L"y"){
					std::wstring tagvalue=tagValue(*str,a);
					if (tagvalue.size())
						y=atoi(tagvalue);
				}else if (tagname==L"italic"){
					cache.set_italic(1);
					if (shadow)
						shadow->set_italic(1);
				}else if (tagname==L"/italic"){
					cache.set_italic(0);
					if (shadow)
						shadow->set_italic(0);
				}else if (tagname==L"bold"){
					cache.set_bold(1);
					if (shadow)
						shadow->set_bold(1);
				}else if (tagname==L"/bold"){
					cache.set_bold(0);
					if (shadow)
						shadow->set_bold(0);
				}
				a=str->find('>',a);
			}
			continue;
		}
		if (character=='\\')
			character=(*str)[++a];
		NONS_Glyph *glyph=cache.getGlyph(character);
		NONS_Glyph *glyph2=(this->shadowLayer)?shadow->getGlyph(character):0;
		if (character=='\n'){
			x=lastStart;
			y+=lineSkip;
		}else{
			if (writeToLayers){
				if (glyph2){
					glyph2->put(this->shadowLayer->data,x+1,y+1);
					if (!!dst){
						NONS_MutexLocker ml(screenMutex);
						glyph2->put(dst->screens[VIRTUAL],x+1,y+1);
					}
				}
				glyph->put(this->foregroundLayer->data,x,y);
				if (!!dst){
					NONS_MutexLocker ml(screenMutex);
					if (glyph2)
						glyph2->put(dst->screens[VIRTUAL],x+1,y+1,0);
					glyph->put(dst->screens[VIRTUAL],x,y,0);
				}
			}else if (dst){
				NONS_MutexLocker ml(screenMutex);
				if (glyph2)
					glyph2->put(dst->screens[VIRTUAL],x+1,y+1);
				glyph->put(dst->screens[VIRTUAL],x,y);
			}
			x+=glyph->get_advance();
			glyph->done();
			glyph2->done();
		}
	}
	if (shadow)
		delete shadow;
	if (update && !!dst)
		dst->updateWholeScreen();
}

int NONS_StandardOutput::setLineStart(std::wstring *arr,ulong start,SDL_Rect *frame,float center){
	while (start<arr->size() && !(*arr)[start])
		start++;
	int width=this->predictLineLength(arr,start,frame->w);
	float factor=(center<=0.5f)?center:1.0f-center;
	int pixelcenter=int(float(frame->w)*factor);
	//Magic formula. Don't mess with it.
	return int((width/2.0f>pixelcenter)?frame->x+(frame->w-width)*(center>0.5f):frame->x+frame->w*center-width/2.0f);
}

int NONS_StandardOutput::predictLineLength(std::wstring *arr,long start,int width){
	int res=0;
	for (ulong a=start;a<arr->size() && (*arr)[a];a++){
		NONS_Glyph *glyph=this->foregroundLayer->fontCache->getGlyph((*arr)[a]);
		if (!glyph || res+glyph->get_advance()+this->extraAdvance>=width){
			glyph->done();
			break;
		}
		res+=glyph->get_advance()+this->extraAdvance;
		glyph->done();
	}
	return res;
}

int NONS_StandardOutput::predictTextHeight(std::wstring *arr){
	int lines=1;
	for (ulong a=0;a<arr->size() && (*arr)[a];a++){
		wchar_t char0=(*arr)[a];
		if (char0==10 || char0==13)
			lines++;
	}
	if ((*arr)[arr->size()-1]==13 || (*arr)[arr->size()-1]==10)
		lines--;
	return this->foregroundLayer->fontCache->line_skip*lines;
}

int NONS_StandardOutput::setTextStart(std::wstring *arr,SDL_Rect *frame,float center){
	int height=this->predictTextHeight(arr);
	float factor=(center<=0.5f)?center:1.0f-center;
	int pixelcenter=int(float(frame->h)*factor);
	return int((height/2.0f>pixelcenter)?frame->y+(frame->h-height)*(center>0.5f):frame->y+frame->h*center-height/2.0f);
}

void NONS_StandardOutput::Clear(bool eraseBuffer){
	this->foregroundLayer->Clear();
	if (this->shadowLayer)
		this->shadowLayer->Clear();
	this->x=this->x0;
	this->y=this->y0;
	if (eraseBuffer){
		if (this->printingStarted){
			this->currentBuffer.append(this->prebufferedText);
			this->prebufferedText.clear();
		}
		if (this->currentBuffer.size()>0){
			if (textDumpFile.is_open()){
				textDumpFile <<UniToUTF8(this->currentBuffer)<<std::endl;
				textDumpFile.flush();
			}
			if (this->maxLogPages){
				this->log.push_back(this->currentBuffer);
				if (this->maxLogPages>0 && this->log.size()>(ulong)this->maxLogPages)
					this->log.erase(this->log.begin());
			}
			this->currentBuffer.clear();
		}
	}
	if (this->verticalCenterPolicy>0 && this->cachedText.size()){
		SDL_Rect frame={this->x0,this->y0,this->w,this->h};
		this->y=this->setTextStart(&this->cachedText,&frame,this->verticalCenterPolicy);
		this->prebufferedText.append(L"<y=");
		this->prebufferedText.append(itoaw(this->y));
		this->prebufferedText.push_back('>');
	}
}

void NONS_StandardOutput::setPosition(int x,int y){
	this->x=this->x0+x;
	this->y=this->y0+y;
	this->currentBuffer.append(L"<x=");
	this->currentBuffer.append(itoaw(this->x));
	this->currentBuffer.append(L"><y=");
	this->currentBuffer.append(itoaw(this->y));
	this->currentBuffer.push_back('>');
}

float NONS_StandardOutput::getCenterPolicy(char which){
	which=tolower(which);
	return (which=='v')?this->verticalCenterPolicy:this->horizontalCenterPolicy;
}

void NONS_StandardOutput::setCenterPolicy(char which,float val){
	which=tolower(which);
	if (val<0)
		val=-val;
	if (val>1){
		ulong val2=(ulong)val;
		val-=val2;
	}
	if (which=='v')
		this->verticalCenterPolicy=val;
	else
		this->horizontalCenterPolicy=val;
}

void NONS_StandardOutput::setCenterPolicy(char which,long val){
	this->setCenterPolicy(which,float(val)/100);
}

bool NONS_StandardOutput::NewLine(){
	int skip=this->foregroundLayer->fontCache->line_skip;
	if (this->y+skip>=this->y0+this->h)
		return 1;
	this->y+=skip;
	this->x=this->x0;
	if (this->printingStarted)
		this->prebufferedText.append(L"\n");
	else
		this->currentBuffer.append(L"\n");
	return 0;
}

std::wstring removeTags(const std::wstring &str){
	std::wstring res;
	res.reserve(str.size());
	for (ulong a=0;a<str.size();a++){
		switch (str[a]){
			case '<':
				while (str[a]!='>')
					a++;
				break;
			case '\\':
				a++;
			default:
				res.push_back(str[a]);
				break;
		}
	}
	return res;
}

NONS_ScreenSpace::NONS_ScreenSpace(int framesize,NONS_FontCache &fc){
	this->screen=new NONS_VirtualScreen(CLOptions.virtualWidth,CLOptions.virtualHeight,CLOptions.realWidth,CLOptions.realHeight);
	SDL_Rect size={0,0,CLOptions.virtualWidth,CLOptions.virtualHeight};
	SDL_Rect frame={framesize,framesize,CLOptions.virtualWidth-framesize*2,CLOptions.virtualHeight-framesize*2};
	this->output=new NONS_StandardOutput(fc,&size,&frame);
	this->output->visible=0;
	this->layerStack.resize(1000,0);
	this->Background=new NONS_Layer(&size,0xFF000000);
	this->leftChar=0;
	this->centerChar=0;
	this->rightChar=0;
	{
		NONS_MutexLocker ml(screenMutex);
		this->screenBuffer=makeSurface(this->screen->screens[VIRTUAL]->w,this->screen->screens[VIRTUAL]->h,SCREENBUFFER_BITS);
	}
	this->gfx_store=new NONS_GFXstore();
	this->sprite_priority=25;
	const SDL_Color *temp=&this->output->foregroundLayer->fontCache->get_color();
	this->lookback=new NONS_Lookback(this->output,temp->r,temp->g,temp->b);
	this->cursor=0;
	this->char_baseline=this->screenBuffer->h-1;
	this->blendSprites=1;

	this->characters[0]=&this->leftChar;
	this->characters[1]=&this->centerChar;
	this->characters[2]=&this->rightChar;
	this->charactersBlendOrder.push_back(0);
	this->charactersBlendOrder.push_back(1);
	this->charactersBlendOrder.push_back(2);
	this->apply_monochrome_first=0;
}

NONS_ScreenSpace::NONS_ScreenSpace(SDL_Rect *window,SDL_Rect *frame,NONS_FontCache &fc,bool shadow){
	this->screen=new NONS_VirtualScreen(CLOptions.virtualWidth,CLOptions.virtualHeight,CLOptions.realWidth,CLOptions.realHeight);
	this->output=new NONS_StandardOutput(fc,window,frame);
	this->output->visible=0;
	this->layerStack.resize(1000,0);
	SDL_Rect size={0,0,CLOptions.virtualWidth,CLOptions.virtualHeight};
	this->Background=new NONS_Layer(&size,0xFF000000);
	this->leftChar=0;
	this->centerChar=0;
	this->rightChar=0;
	this->screenBuffer=makeSurface(this->screen->screens[VIRTUAL]->w,this->screen->screens[VIRTUAL]->h,SCREENBUFFER_BITS);
	this->gfx_store=new NONS_GFXstore();
	this->sprite_priority=25;
	const SDL_Color *temp=&this->output->foregroundLayer->fontCache->get_color();
	this->lookback=new NONS_Lookback(this->output,temp->r,temp->g,temp->b);
	this->cursor=0;
	this->char_baseline=this->screenBuffer->h-1;
	this->blendSprites=1;

	this->characters[0]=&this->leftChar;
	this->characters[1]=&this->centerChar;
	this->characters[2]=&this->rightChar;
	this->charactersBlendOrder.push_back(0);
	this->charactersBlendOrder.push_back(1);
	this->charactersBlendOrder.push_back(2);
	this->apply_monochrome_first=0;
}

NONS_ScreenSpace::~NONS_ScreenSpace(){
	delete this->output;
	for (ulong a=0;a<this->layerStack.size();a++)
		if (this->layerStack[a])
			delete this->layerStack[a];
	delete this->leftChar;
	delete this->rightChar;
	delete this->centerChar;
	delete this->Background;
	delete this->screen;
	//delete this->this->cursor;
	SDL_FreeSurface(this->screenBuffer);
	delete this->gfx_store;
	delete this->lookback;
}

void NONS_ScreenSpace::BlendOptimized(std::vector<SDL_Rect> &rects){
	if (!rects.size())
		return;
////////////////////////////////////////////////////////////////////////////////
#define BLEND_OPTIM(p,function) {\
	if ((p) && (p)->data && (p)->visible){\
		SDL_Rect src={\
			refresh_area.x-Sint16((p)->position.x)+Sint16((p)->clip_rect.x),\
			refresh_area.y-Sint16((p)->position.y)+Sint16((p)->clip_rect.y),\
			Uint16(refresh_area.w>(p)->clip_rect.w?(p)->clip_rect.w:refresh_area.w),\
			Uint16(refresh_area.h>(p)->clip_rect.h?(p)->clip_rect.h:refresh_area.h)\
		};\
		if (src.x<(p)->clip_rect.x)\
			src.x=(Sint16)(p)->clip_rect.x;\
		if (src.y<(p)->clip_rect.y)\
			src.y=(Sint16)(p)->clip_rect.y;\
		SDL_Rect dst=refresh_area;\
		if (dst.x<(p)->position.x)\
			dst.x=(Sint16)(p)->position.x;\
		if (dst.y<(p)->position.y)\
			dst.y=(Sint16)(p)->position.y;\
		function((p)->data,&src,this->screenBuffer,&dst);\
	}\
}
////////////////////////////////////////////////////////////////////////////////
	ulong minx=rects[0].x,
		maxx=minx+rects[0].w,
		miny=rects[0].y,
		maxy=miny+rects[0].h;
	for (ulong a=1;a<rects.size();a++){
		ulong x0=rects[a].x,
			x1=x0+rects[a].w,
			y0=rects[a].y,
			y1=y0+rects[a].h;
		if (x0<minx)
			minx=x0;
		if (x1>maxx)
			maxx=x1;
		if (y0<miny)
			miny=y0;
		if (y1>maxy)
			maxy=y1;
	}
	SDL_Rect refresh_area={
		(Sint16)minx,
		(Sint16)miny,
		Sint16(maxx-minx),
		Sint16(maxy-miny)
	};
	if (!(refresh_area.w*refresh_area.h))
		return;
	SDL_FillRect(this->screenBuffer,&refresh_area,amask);
	BLEND_OPTIM(this->Background,manualBlit);
	for (ulong a=this->layerStack.size()-1;a>this->sprite_priority;a--){
		NONS_Layer *p=this->layerStack[a];
		BLEND_OPTIM(p,manualBlit);
	}
	for (ulong a=0;a<this->charactersBlendOrder.size();a++){
		NONS_Layer *lay=*this->characters[charactersBlendOrder[a]];
		BLEND_OPTIM(lay,manualBlit);
	}
	for (long a=this->sprite_priority;a>=0;a--){
		NONS_Layer *p=this->layerStack[a];
		BLEND_OPTIM(p,manualBlit);
	}
	for (ulong a=0;a<this->filterPipeline.size();a++){
		pipelineElement &el=this->filterPipeline[a];
		NONS_GFX::callFilter(el.effectNo,el.color,el.ruleStr,this->screenBuffer,this->screenBuffer);
	}
	if (this->output->visible){
		if (!this->output->shadeLayer->useDataAsDefaultShade){
			BLEND_OPTIM(this->output->shadeLayer,multiplyBlend);
		}else{
			BLEND_OPTIM(this->output->shadeLayer,manualBlit);
		}
		BLEND_OPTIM(this->output->shadowLayer,manualBlit);
		BLEND_OPTIM(this->output->foregroundLayer,manualBlit);
	}
	BLEND_OPTIM(this->cursor,manualBlit);
	{
		NONS_MutexLocker ml(screenMutex);
		manualBlit(this->screenBuffer,&refresh_area,this->screen->screens[VIRTUAL],&refresh_area);
	}
	this->screen->updateScreen(refresh_area.x,refresh_area.y,refresh_area.w,refresh_area.h);
}

ErrorCode NONS_ScreenSpace::BlendAll(ulong effect){
	this->BlendNoCursor(0);
	if (this->cursor && this->cursor->data)
		manualBlit(
			this->cursor->data,
			&this->cursor->clip_rect.to_SDL_Rect(),
			this->screenBuffer,
			&this->cursor->position.to_SDL_Rect()
		);
	if (effect){
		NONS_GFX *e=this->gfx_store->retrieve(effect);
		if (!e)
			return NONS_UNDEFINED_EFFECT;
		return e->call(this->screenBuffer,0,this->screen);
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScreenSpace::BlendAll(ulong effect,long timing,const std::wstring *rule){
	this->BlendAll(0);
	return NONS_GFX::callEffect(effect,timing,rule,this->screenBuffer,0,this->screen);
}

ErrorCode NONS_ScreenSpace::BlendNoCursor(ulong effect){
	this->BlendNoText(0);
	if (this->output->visible){
		if (!this->output->shadeLayer->useDataAsDefaultShade)
			multiplyBlend(
				this->output->shadeLayer->data,
				0,
				this->screenBuffer,
				&this->output->shadeLayer->clip_rect.to_SDL_Rect()
			);
		else
			manualBlit(
				this->output->shadeLayer->data,
				0,
				this->screenBuffer,
				&this->output->shadeLayer->clip_rect.to_SDL_Rect()
			);
		if (this->output->shadowLayer)
			manualBlit(
				this->output->shadowLayer->data,
				0,
				this->screenBuffer,
				&this->output->shadowLayer->clip_rect.to_SDL_Rect(),
				this->output->shadowLayer->alpha
			);
		manualBlit(
			this->output->foregroundLayer->data,
			0,
			this->screenBuffer,
			&this->output->foregroundLayer->clip_rect.to_SDL_Rect(),
			this->output->foregroundLayer->alpha
		);
	}
	if (effect){
		NONS_GFX *e=this->gfx_store->retrieve(effect);
		if (!e)
			return NONS_UNDEFINED_EFFECT;
		return e->call(this->screenBuffer,0,this->screen);
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScreenSpace::BlendNoCursor(ulong effect,long timing,const std::wstring *rule){
	this->BlendNoCursor(0);
	return NONS_GFX::callEffect(effect,timing,rule,this->screenBuffer,0,this->screen);
}

ErrorCode NONS_ScreenSpace::BlendNoText(ulong effect){
	this->BlendOnlyBG(0);
	if (this->blendSprites){
		for (ulong a=this->layerStack.size()-1;a>this->sprite_priority;a--)
			if (this->layerStack[a] && this->layerStack[a]->visible && this->layerStack[a]->data)
				manualBlit(
					this->layerStack[a]->data,
					&this->layerStack[a]->clip_rect.to_SDL_Rect(),
					this->screenBuffer,
					&this->layerStack[a]->position.to_SDL_Rect(),
					this->layerStack[a]->alpha
				);
	}
	for (ulong a=0;a<this->charactersBlendOrder.size();a++){
		NONS_Layer *lay=*this->characters[charactersBlendOrder[a]];
		if (lay && lay->data)
			manualBlit(
				lay->data,
				&lay->clip_rect.to_SDL_Rect(),
				this->screenBuffer,
				&lay->position.to_SDL_Rect(),
				lay->alpha
			);
	}
	if (this->blendSprites){
		for (long a=this->sprite_priority;a>=0;a--)
			if (this->layerStack[a] && this->layerStack[a]->visible && this->layerStack[a]->data)
				manualBlit(
					this->layerStack[a]->data,
					&this->layerStack[a]->clip_rect.to_SDL_Rect(),
					this->screenBuffer,
					&this->layerStack[a]->position.to_SDL_Rect(),
					this->layerStack[a]->alpha
				);
	}
	for (ulong a=0;a<this->filterPipeline.size();a++){
		pipelineElement &el=this->filterPipeline[a];
		NONS_GFX::callFilter(el.effectNo,el.color,el.ruleStr,this->screenBuffer,this->screenBuffer);
	}
	if (effect){
		NONS_GFX *e=this->gfx_store->retrieve(effect);
		if (!e)
			return NONS_UNDEFINED_EFFECT;
		return e->call(this->screenBuffer,0,this->screen);
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScreenSpace::BlendNoText(ulong effect,long timing,const std::wstring *rule){
	this->BlendNoText(0);
	return NONS_GFX::callEffect(effect,timing,rule,this->screenBuffer,0,this->screen);
}

ErrorCode NONS_ScreenSpace::BlendOnlyBG(ulong effect){
	SDL_FillRect(this->screenBuffer,0,amask);
	if (!!this->Background && !!this->Background->data)
		manualBlit(
			this->Background->data,
			&this->Background->clip_rect.to_SDL_Rect(),
			this->screenBuffer,
			&this->Background->position.to_SDL_Rect()
		);
	if (effect){
		NONS_GFX *e=this->gfx_store->retrieve(effect);
		if (!e)
			return NONS_UNDEFINED_EFFECT;
		return e->call(this->screenBuffer,0,this->screen);
	}
	return NONS_NO_ERROR;
}

ErrorCode NONS_ScreenSpace::BlendOnlyBG(ulong effect,long timing,const std::wstring *rule){
	this->BlendOnlyBG(0);
	return NONS_GFX::callEffect(effect,timing,rule,this->screenBuffer,0,this->screen);
}

void NONS_ScreenSpace::clearText(){
	this->output->Clear();
	this->BlendNoCursor(1);
	//SDL_UpdateRect(this->screen,0,0,0,0);
}

void NONS_ScreenSpace::hideText(){
	if (!this->output->visible)
		return;
	this->output->visible=0;
	this->BlendNoCursor(0);
	this->output->transition->call(this->screenBuffer,0,this->screen);
}

void NONS_ScreenSpace::showText(){
	if (this->output->visible)
		return;
	this->output->visible=1;
	this->BlendNoCursor(0);
	this->output->transition->call(this->screenBuffer,0,this->screen);
}

void NONS_ScreenSpace::resetParameters(SDL_Rect *window,SDL_Rect *frame,NONS_FontCache &fc,bool shadow){
	NONS_GFX *temp;
	bool a=this->output->transition->stored;
	if (a)
		temp=this->output->transition;
	else
		temp=new NONS_GFX(*(this->output->transition));
	delete this->output;
	this->output=new NONS_StandardOutput(fc,window,frame,shadow);
	delete this->output->transition;
	this->output->transition=temp;
	this->lookback->reset(this->output);
}

ErrorCode NONS_ScreenSpace::loadSprite(ulong n,const std::wstring &string,long x,long y,uchar alpha,bool visibility){
	if (!string[0])
		return NONS_EMPTY_STRING;
	if (n>this->layerStack.size())
		return NONS_INVALID_RUNTIME_PARAMETER_VALUE;
	if (!this->layerStack[n])
		this->layerStack[n]=new NONS_Layer(&string);
	else
		this->layerStack[n]->load(&string);
	if (!this->layerStack[n]->data)
		return NONS_UNDEFINED_ERROR;
	this->layerStack[n]->position.x=(Sint16)x;
	this->layerStack[n]->position.y=(Sint16)y;
	this->layerStack[n]->visible=visibility;
	this->layerStack[n]->alpha=alpha;
	return NONS_NO_ERROR;
}

void NONS_ScreenSpace::clear(){
#define CHECK_AND_DELETE(p) (p)->unload();  //if (!!(p)){ /*delete (p); (p)=0;*/}
	CHECK_AND_DELETE(this->Background);
	CHECK_AND_DELETE(this->leftChar);
	CHECK_AND_DELETE(this->rightChar);
	CHECK_AND_DELETE(this->centerChar);
	for (ulong a=0;a<this->layerStack.size();a++){
		if (this->layerStack[a]){
			CHECK_AND_DELETE(this->layerStack[a]);
		}
	}
	this->clearText();
	this->BlendNoCursor(1);
}

bool NONS_ScreenSpace::advanceAnimations(ulong msecs,std::vector<SDL_Rect> &rects){
	rects.clear();
	bool requireRefresh=0;
	std::vector<NONS_Layer *> arr;
	arr.reserve(5+this->layerStack.size());
	arr.push_back(this->Background);
	arr.push_back(this->leftChar);
	arr.push_back(this->rightChar);
	arr.push_back(this->centerChar);
	for (ulong a=0;a<this->layerStack.size();a++)
		arr.push_back(this->layerStack[a]);
	arr.push_back(this->cursor);
	for (ulong a=0;a<arr.size();a++){
		NONS_Layer *p=arr[a];
		if (p && p->data){
			ulong first=p->animation.getCurrentAnimationFrame();
			bool b=p->advanceAnimation(msecs);
			requireRefresh|=b;
			if (b){
				ulong second=p->animation.getCurrentAnimationFrame();
				SDL_Rect push=p->optimized_updates[std::pair<ulong,ulong>(first,second)];
				push.x+=(Sint16)p->position.x;
				push.y+=(Sint16)p->position.y;
				rects.push_back(push);
			}
		}
	}
	return requireRefresh;
}
