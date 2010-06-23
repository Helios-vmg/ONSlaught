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

#include "VirtualScreen.h"
#include "Functions.h"
#include "ThreadManager.h"
#include "CommandLineOptions.h"
#include "Plugin/LibraryLoader.h"
#include "ImageLoader.h"
#include <iostream>

#ifndef DEBUG_SCREEN_MUTEX
DLLexport NONS_Mutex screenMutex;
#else
DLLexport NONS_Mutex screenMutex(1);
#endif

//#define ONLY_NEAREST_NEIGHBOR
//#define BENCHMARK_INTERPOLATION
#define DEFAULT_SCREEN_COLOR_DEPTH 32

pipelineElement::pipelineElement(ulong effectNo,const SDL_Color &color,const std::wstring &rule,bool loadRule)
		:effectNo(effectNo),color(color),ruleStr(rule){
	if (rule.size() && loadRule)
		ImageLoader.fetchSprite(this->rule,rule);
	else
		this->rule=0;
}

pipelineElement::pipelineElement(const pipelineElement &o){
	*this=o;
}

void pipelineElement::operator=(const pipelineElement &o){
	this->effectNo=o.effectNo;
	this->color=o.color;
	this->rule=0;
	this->ruleStr=o.ruleStr;
}

pipelineElement::~pipelineElement(){
	if (this->rule)
		ImageLoader.unfetchImage(this->rule);
}

void _asyncEffectThread(void *param){
	NONS_VirtualScreen *vs=(NONS_VirtualScreen *)param;
	ulong effectNo=vs->aeffect_no;
	ulong freq=vs->aeffect_freq;
	surfaceData srcData,
		dstData;
	{
		NONS_MutexLocker ml(screenMutex);
		srcData=vs->screens[PRE_ASYNC];
		dstData=vs->screens[REAL];
	}
	ulong ms=1000/freq;
	void *userData=0;
	if (vs->initializers[effectNo])
		userData=vs->initializers[effectNo](effectNo);
	asyncEffect_f effect=vs->effects[effectNo];
	while (!vs->killAsyncEffect){
		ulong t0=SDL_GetTicks();
		{
			NONS_MutexLocker ml(screenMutex);
			SDL_LockSurface(vs->screens[PRE_ASYNC]);
			SDL_LockSurface(vs->screens[REAL]);
			dstData.pixels=(uchar *)vs->screens[REAL]->pixels;
			bool refresh=effect(effectNo+1,srcData,dstData,userData);
			SDL_UnlockSurface(vs->screens[REAL]);
			SDL_UnlockSurface(vs->screens[PRE_ASYNC]);
			if (refresh)
				SDL_UpdateRect(vs->screens[REAL],0,0,0,0);
		}
		ulong t1=SDL_GetTicks();
		t1-=t0;
		if (t1<ms)
			SDL_Delay(ms-t1);
	}
	if (vs->uninitializers[effectNo])
		vs->uninitializers[effectNo](effectNo,userData);
}

NONS_VirtualScreen::NONS_VirtualScreen(ulong w,ulong h){
	std::fill(this->screens,this->screens+REAL+1,(SDL_Surface *)0);
	this->screens[REAL]=SDL_SetVideoMode(w,h,DEFAULT_SCREEN_COLOR_DEPTH,USE_HARDWARE_SURFACES|SDL_DOUBLEBUF|((CLOptions.startFullscreen)?SDL_FULLSCREEN:0));
	if (!this->screens[REAL]){
		std::cerr <<"FATAL ERROR: Could not allocate screen!"<<std::endl
			<<"Terminating."<<std::endl;
		exit(0);
	}
	this->screens[VIRTUAL]=this->screens[REAL];
	this->x_multiplier=0x100;
	this->y_multiplier=0x100;
	this->x_divisor=this->x_multiplier;
	this->y_divisor=this->y_multiplier;
	this->inRect=this->screens[REAL]->clip_rect;
	this->outRect=this->inRect;
	this->normalInterpolation=0;
	this->fullscreen=CLOptions.startFullscreen;
	this->killAsyncEffect=0;
	this->initEffectList();
	memset(this->usingFeature,0,this->usingFeature_s*sizeof(bool));
	this->printCurrentState();
#ifdef DEBUG_SCREEN_MUTEX
	this->screens[VIRTUAL].force_mutex_check();
#endif
}

NONS_VirtualScreen::NONS_VirtualScreen(ulong iw,ulong ih,ulong ow,ulong oh){
	std::fill(this->screens,this->screens+REAL+1,(SDL_Surface *)0);
	this->screens[REAL]=SDL_SetVideoMode(ow,oh,DEFAULT_SCREEN_COLOR_DEPTH,USE_HARDWARE_SURFACES|SDL_DOUBLEBUF|((CLOptions.startFullscreen)?SDL_FULLSCREEN:0));
	if (!this->screens[REAL]){
		std::cerr <<"FATAL ERROR: Could not allocate screen!"<<std::endl
			<<"Terminating."<<std::endl;
		exit(0);
	}
	this->screens[VIRTUAL]=this->screens[REAL];
	memset(this->usingFeature,0,this->usingFeature_s*sizeof(bool));
	if (iw==ow && ih==oh){
		this->x_multiplier=0x100;
		this->y_multiplier=0x100;
		this->inRect=this->screens[REAL]->clip_rect;
		this->outRect=this->inRect;
		this->normalInterpolation=0;
	}else{
		this->usingFeature[INTERPOLATION]=1;
		this->pre_inter=VIRTUAL;
		this->post_inter=REAL;
		this->screens[VIRTUAL]=makeSurface(iw,ih,32);
		float ratio0=float(iw)/float(ih),
			ratio1=float(ow)/float(oh);
		this->normalInterpolation=&bilinearInterpolation;
		this->inRect=this->screens[VIRTUAL]->clip_rect;
		if (ABS(ratio0-ratio1)<.00001){
			this->x_multiplier=(ow<<8)/iw;
			this->y_multiplier=(oh<<8)/ih;
			this->x_divisor=(iw<<8)/ow;
			this->y_divisor=(ih<<8)/oh;
			if (x_multiplier<0x100 || y_multiplier<0x100)
				this->normalInterpolation=&bilinearInterpolation2;
			this->outRect=this->screens[REAL]->clip_rect;
		}else if (ratio0>ratio1){
			float h=float(ow)/float(ratio0);
			this->outRect.x=0;
			this->outRect.y=Sint16((oh-h)/2.0f);
			this->outRect.w=Sint16(ow);
			this->outRect.h=(Uint16)h;
			this->x_multiplier=(ow<<8)/iw;
			this->y_multiplier=ulong(h*256.0f/float(ih));
			this->x_divisor=(iw<<8)/ow;
			this->y_divisor=ulong((ih<<8)/h);
			if (x_multiplier<0x100 || h/float(ih)<1)
				this->normalInterpolation=&bilinearInterpolation2;
		}else{
			float w=float(oh)*float(ratio0);
			this->outRect.x=Sint16((ow-w)/2.0f);
			this->outRect.y=0;
			this->outRect.w=(Uint16)w;
			this->outRect.h=(Uint16)oh;
			this->x_multiplier=ulong(w*256.0f/float(iw));
			this->y_multiplier=(oh<<8)/ih;
			this->x_divisor=ulong((iw<<8)/w);
			this->y_divisor=(ih<<8)/oh;
			if (w/float(iw)<1 || y_multiplier<=0x100)
				this->normalInterpolation=&bilinearInterpolation2;
		}
		//this->normalInterpolation=&nearestNeighborInterpolation;
		this->screens[REAL]->clip_rect=this->outRect;
	}
	this->fullscreen=CLOptions.startFullscreen;
	this->killAsyncEffect=0;
	this->initEffectList();
	this->printCurrentState();
#ifdef DEBUG_SCREEN_MUTEX
	this->screens[VIRTUAL].force_mutex_check();
#endif
}

NONS_VirtualScreen::~NONS_VirtualScreen(){
	this->stopEffect();
	this->applyFilter(0,SDL_Color(),L"");
	SDL_FreeSurface(this->screens[REAL]);
	if (this->usingFeature[INTERPOLATION])
		SDL_FreeSurface(this->screens[VIRTUAL]);
}

DECLSPEC void NONS_VirtualScreen::blitToScreen(SDL_Surface *src,SDL_Rect *srcrect,SDL_Rect *dstrect){
	NONS_MutexLocker ml(screenMutex);
	if (!!src && src->format->BitsPerPixel<24)
		SDL_BlitSurface(src,srcrect,this->screens[VIRTUAL],dstrect);
	else
		manualBlit(src,srcrect,this->screens[VIRTUAL],dstrect);
}

void NONS_VirtualScreen::updateScreen(ulong x,ulong y,ulong w,ulong h,bool fast){
	NONS_MutexLocker ml(screenMutex);
	if (this->usingFeature[OVERALL_FILTER]){
		SDL_Surface *src=this->screens[VIRTUAL],
			*dst=this->screens[this->post_filter];
		SDL_LockSurface(src);
		SDL_LockSurface(dst);
		for (ulong a=0;a<this->filterPipeline.size();a++){
			if (a==1){
				SDL_UnlockSurface(src);
				src=this->screens[this->post_filter];
				SDL_LockSurface(src);
			}
			pipelineElement &el=this->filterPipeline[a];
			this->filters[el.effectNo](el.effectNo+1,el.color,src,el.rule,dst,x,y,w,h);
		}
		SDL_UnlockSurface(dst);
		SDL_UnlockSurface(src);
	}
	if (this->usingFeature[INTERPOLATION]){
		SDL_Rect s={
				(Sint16)x,
				(Sint16)y,
				(Uint16)w,
				(Uint16)h
			},d={
				(Sint16)this->convertX(x),
				(Sint16)this->convertY(y),
				(Uint16)this->convertW(w),
				(Uint16)this->convertH(h)
		};
		/*
		The following plus ones and minus ones are a patch to correct some weird
		errors. I'd really like to have the patience to track down the real
		source of the bug, but I don't.
		Basically, they move the upper-left corner one pixel up and to the left
		without moving the bottom-right corner.
		*/
		if (d.x>0){
			d.x--;
			d.w++;
		}
		if (d.y>0){
			d.y--;
			d.h++;
		}
		if (d.x+d.w>this->outRect.w+this->outRect.x)
			d.w=this->outRect.w-d.x+this->outRect.x;
		if (d.y+d.h>this->outRect.h+this->outRect.y)
			d.h=this->outRect.h-d.y+this->outRect.y;
#ifndef ONLY_NEAREST_NEIGHBOR
		if (fast)
			nearestNeighborInterpolation(this->screens[this->pre_inter],&s,this->screens[this->post_inter],&d,this->x_multiplier,this->y_multiplier);
		else
			this->normalInterpolation(this->screens[this->pre_inter],&s,this->screens[this->post_inter],&d,this->x_multiplier,this->y_multiplier);
#else
		nearestNeighborInterpolation(this->screens[this->pre_inter],&s,this->screens[this->post_inter],&d,this->x_multiplier,this->y_multiplier);
#endif
		if (!this->usingFeature[ASYNC_EFFECT])
			SDL_UpdateRect(this->screens[REAL],d.x,d.y,d.w,d.h);
	}else if (!this->usingFeature[ASYNC_EFFECT])
		SDL_UpdateRect(this->screens[REAL],x,y,w,h);
}

DECLSPEC void NONS_VirtualScreen::updateWholeScreen(bool fast){
	NONS_MutexLocker ml(screenMutex);
	this->updateWithoutLock(fast);
}

DECLSPEC void NONS_VirtualScreen::updateWithoutLock(bool fast){
	if (this->usingFeature[OVERALL_FILTER]){
		SDL_Surface *src=this->screens[VIRTUAL];
		for (ulong a=0;a<this->filterPipeline.size();a++){
			pipelineElement &el=this->filterPipeline[a];
			this->filters[el.effectNo](el.effectNo+1,el.color,src,el.rule,this->screens[this->post_filter],0,0,this->inRect.w,this->inRect.h);
			if (!a)
				src=this->screens[this->post_filter];
		}
	}
	if (this->usingFeature[INTERPOLATION]){
#ifdef BENCHMARK_INTERPOLATION
		ulong t0=SDL_GetTicks(),t1;
#endif
#ifndef ONLY_NEAREST_NEIGHBOR
		if (fast)
			nearestNeighborInterpolation(this->screens[this->pre_inter],&this->inRect,this->screens[this->post_inter],&this->outRect,this->x_multiplier,this->y_multiplier);
		else
			this->normalInterpolation(this->screens[this->pre_inter],&this->inRect,this->screens[this->post_inter],&this->outRect,this->x_multiplier,this->y_multiplier);
#else
		nearestNeighborInterpolation(this->screens[this->pre_inter],&this->inRect,this->screens[this->post_inter],&this->outRect,this->x_multiplier,this->y_multiplier);
#endif
#ifdef BENCHMARK_INTERPOLATION
		t1=SDL_GetTicks()-t0;
		std::cout <<t1<<std::endl;
#endif
		if (!this->usingFeature[ASYNC_EFFECT])
			SDL_UpdateRect(this->screens[REAL],this->outRect.x,this->outRect.y,this->outRect.w,this->outRect.h);
	}else if (!this->usingFeature[ASYNC_EFFECT])
		SDL_UpdateRect(this->screens[REAL],0,0,0,0);
}

long NONS_VirtualScreen::convertX(long x){
	return this->convertW(x)+this->outRect.x;
}

long NONS_VirtualScreen::convertY(long y){
	return this->convertH(y)+this->outRect.y;
}

long NONS_VirtualScreen::unconvertX(long x){
	return (((x-this->outRect.x)<<8)/this->x_multiplier);
}

long NONS_VirtualScreen::unconvertY(long y){
	return (((y-this->outRect.y)<<8)/this->y_multiplier);
}

ulong NONS_VirtualScreen::convertW(ulong w){
	ulong r=(w<<8)*this->x_multiplier;
	if ((r&0xFFFF)>0)
		r=(r>>16)+1;
	else
		r>>=16;
	return r;
}

ulong NONS_VirtualScreen::convertH(ulong h){
	ulong r=(h<<8)*this->y_multiplier;
	if ((r&0xFFFF)>0)
		r=(r>>16)+1;
	else
		r>>=16;
	return r;
}

bool NONS_VirtualScreen::toggleFullscreen(uchar mode){
	NONS_MutexLocker ml(screenMutex);
	if (mode==2)
		this->fullscreen=!this->fullscreen;
	else
		this->fullscreen=mode&1;
	ushort w=this->screens[REAL]->w,
		h=this->screens[REAL]->h;
	SDL_Surface *tempCopy=0;
	if (!this->usingFeature[INTERPOLATION] && !this->usingFeature[ASYNC_EFFECT])
		tempCopy=copySurface(this->screens[REAL],0);
	this->screens[REAL]=SDL_SetVideoMode(w,h,DEFAULT_SCREEN_COLOR_DEPTH,USE_HARDWARE_SURFACES|SDL_DOUBLEBUF|((this->fullscreen)?SDL_FULLSCREEN:0));
	if (tempCopy){
		if (!this->usingFeature[OVERALL_FILTER])
			this->screens[VIRTUAL]=this->screens[REAL];
		manualBlit_unthreaded(tempCopy,0,this->screens[REAL],0);
		SDL_FreeSurface(tempCopy);
	}else if (this->usingFeature[INTERPOLATION])
		this->screens[REAL]->clip_rect=this->outRect;
	this->updateWithoutLock();
	return this->fullscreen;
}

SDL_Surface *NONS_VirtualScreen::toggleFullscreenFromVideo(){
	this->fullscreen=!this->fullscreen;
	ushort w=this->screens[REAL]->w,
		h=this->screens[REAL]->h;
	bool tempCopy=0;
	if (!this->usingFeature[INTERPOLATION] && !this->usingFeature[ASYNC_EFFECT])
		tempCopy=1;
	this->screens[REAL]=SDL_SetVideoMode(w,h,DEFAULT_SCREEN_COLOR_DEPTH,USE_HARDWARE_SURFACES|SDL_DOUBLEBUF|((this->fullscreen)?SDL_FULLSCREEN:0));
	if (tempCopy){
		if (!this->usingFeature[OVERALL_FILTER])
			this->screens[VIRTUAL]=this->screens[REAL];
	}else if (this->usingFeature[INTERPOLATION])
		this->screens[REAL]->clip_rect=this->outRect;
	return this->screens[REAL];
}

std::string NONS_VirtualScreen::takeScreenshot(const std::string &name){
	std::string filename=(!name.size())?getTimeString<char>(1)+'_'+itoac(SDL_GetTicks(),10)+".bmp":name;
	NONS_MutexLocker ml(screenMutex);
	SDL_SaveBMP(this->screens[REAL],filename.c_str());
	return filename;
}

void NONS_VirtualScreen::takeScreenshotFromVideo(void){
	//NONS_MutexLocker ml(screenMutex);
	SDL_SaveBMP(this->screens[REAL],(getTimeString<char>(1)+'_'+itoac(SDL_GetTicks(),10)+".bmp").c_str());
}

void NONS_VirtualScreen::initEffectList(){
	this->initializers.clear();
	this->effects.clear();
	this->uninitializers.clear();
	this->filters.clear();
	this->filters.push_back(effectMonochrome);
	this->filters.push_back(effectNegative);
	pluginLibraryFunction fp=(pluginLibraryFunction)pluginLibrary.getFunction("getFunctionPointers");
	if (!fp)
		return;
	{
		//1=Get filter effect function pointers.
		std::vector<filterFX_f> vec=*(std::vector<filterFX_f> *)fp((void *)1);
		this->filters.insert(this->filters.end(),vec.begin(),vec.end());
	}
	{
		//2=Get asynchronous effect function pointers.
		std::vector<asyncFXfunctionSet> vec=*(std::vector<asyncFXfunctionSet> *)fp((void *)2);
		for (ulong a=0;a<vec.size();a++){
			this->initializers.push_back(vec[a].initializer);
			this->effects.push_back(vec[a].effect);
			this->uninitializers.push_back(vec[a].uninitializer);
		}
	}
}

ErrorCode NONS_VirtualScreen::callEffect(ulong effectNo,ulong frequency){
	if (this->effects.size()<=effectNo)
		return NONS_NO_EFFECT;
	this->stopEffect();
	{
		NONS_MutexLocker ml(screenMutex);
		this->changeState(1,0);
	}
	this->aeffect_no=effectNo;
	this->aeffect_freq=frequency;
	this->asyncEffectThread.call(_asyncEffectThread,this);
	return NONS_NO_ERROR;
}

void NONS_VirtualScreen::stopEffect(){
	this->killAsyncEffect=1;
	this->asyncEffectThread.join();
	this->killAsyncEffect=0;
	NONS_MutexLocker ml(screenMutex);
	if (this->usingFeature[ASYNC_EFFECT])
		this->changeState(1,0);
}

ErrorCode NONS_VirtualScreen::applyFilter(ulong effectNo,const SDL_Color &color,const std::wstring &rule){
	if (!effectNo){
		if (this->usingFeature[OVERALL_FILTER]){
			this->changeState(0,1);
			this->filterPipeline.clear();
		}
		return NONS_NO_ERROR;
	}
	effectNo--;
	if (this->filters.size()<=effectNo)
		return NONS_NO_EFFECT;
	if (!this->usingFeature[OVERALL_FILTER])
		this->changeState(0,1);
	this->filterPipeline.push_back(pipelineElement(effectNo,color,rule,1));
	return NONS_NO_ERROR;
}

void NONS_VirtualScreen::changeState(bool switchAsyncState,bool switchFilterState){
	ulong originalState=(ulong)this->usingFeature[OVERALL_FILTER]|((ulong)this->usingFeature[ASYNC_EFFECT]<<1),
		newState=(ulong)(this->usingFeature[OVERALL_FILTER]^switchFilterState)|((ulong)(this->usingFeature[ASYNC_EFFECT]^switchAsyncState)<<1),
		transition=(originalState<<2)|(newState);
	bool interpolation=this->usingFeature[INTERPOLATION];
	switch (transition){
		case 0:  // ignore changing from one state to itself
		case 5:
		case 10:
		case 15:
		case 3:	 // ignore changing more than one state at a time
		case 6:
		case 9:
		case 12:
			break;
		case 1: //turn filter on, effect remains off
			if (interpolation){
				this->post_filter=1;
				this->pre_inter=1;
				this->screens[this->pre_inter]=this->screens[VIRTUAL];
			}else
				this->post_filter=3;
			this->screens[VIRTUAL]=copySurface(this->screens[VIRTUAL]);
			break;
		case 2: //turn effect on, filter remains off
			this->screens[PRE_ASYNC]=copySurface(this->screens[REAL]);
			if (interpolation)
				this->post_inter=2;
			else
				this->screens[VIRTUAL]=this->screens[PRE_ASYNC];
			break;
		case 4: //turn filter off, effect remains off
			if (interpolation){
				SDL_FreeSurface(this->screens[this->pre_inter]);
				this->pre_inter=0;
			}else{
				SDL_FreeSurface(this->screens[VIRTUAL]);
				this->screens[VIRTUAL]=this->screens[REAL];
			}
			break;
		case 7: //turn effect on, filter remains on
			if (interpolation)
				this->post_inter=2;
			else
				this->post_filter=2;
			this->screens[PRE_ASYNC]=copySurface(this->screens[REAL]);
			break;
		case 8: //turn effect off, filter remains off
			SDL_FreeSurface(this->screens[PRE_ASYNC]);
			if (interpolation)
				this->post_inter=3;
			else
				this->screens[VIRTUAL]=this->screens[REAL];
			break;
		case 11: //turn filter on, effect remains on
			if (interpolation){
				this->post_filter=1;
				this->pre_inter=1;
				this->screens[this->pre_inter]=copySurface(this->screens[VIRTUAL]);
			}else{
				this->post_filter=2;
				this->screens[VIRTUAL]=copySurface(this->screens[VIRTUAL]);
			}
			break;
		case 13: //turn effect off, filter remains on
			SDL_FreeSurface(this->screens[PRE_ASYNC]);
			if (interpolation)
				this->post_inter=3;
			else
				this->post_filter=3;
			break;
		case 14: //turn filter off, effect remains on
			if (interpolation){
				SDL_FreeSurface(this->screens[this->pre_inter]);
				this->pre_inter=0;
			}else{
				SDL_FreeSurface(this->screens[VIRTUAL]);
				this->screens[VIRTUAL]=this->screens[PRE_ASYNC];
			}
			break;
	}
	this->usingFeature[OVERALL_FILTER]^=switchFilterState;
	this->usingFeature[ASYNC_EFFECT]^=switchAsyncState;
	this->printCurrentState();
}

void NONS_VirtualScreen::printCurrentState(){
	std::cout <<"Processing pipeline: ";
	for (ulong a=0;a<3;a++){
		std::cout <<a;
		ulong pointsTo=a;
		for (ulong b=a+1;b<4 && pointsTo==a;b++)
			if (this->screens[a]==this->screens[b])
				pointsTo=b;
		if (pointsTo!=a){
			std::cout <<"->";
			a=pointsTo-1;
			continue;
		}
		std::cout <<"*-(";
		if (a<1 && this->usingFeature[OVERALL_FILTER]){
			a=this->post_filter-1;
			std::cout <<"filter";
		}else if (a<2 && this->usingFeature[INTERPOLATION]){
			a=this->post_inter-1;
			std::cout <<"interpolation";
		}else if (a<3 && this->usingFeature[ASYNC_EFFECT]){
			a=REAL-1;
			std::cout <<"async effect";
		}
		std::cout <<")->";
	}
	std::cout <<3<<std::endl;
}

struct filterParameters{
	ulong effectNo;
	SDL_Color color;
	SDL_Surface *src,
		*dst;
	SDL_Rect *rect;
};

#define RED_MONOCHROME(x) ((x)*3/10)
#define GREEN_MONOCHROME(x) ((x)*59/100)
#define BLUE_MONOCHROME(x) ((x)*11/100)

void effectMonochrome_threaded(SDL_Surface *src,SDL_Surface *dst,SDL_Rect *rect,SDL_Color &color){
	surfaceData srcData=src,
		dstData=dst;
	long x=rect->x,
		y=rect->y,
		w=rect->w,
		h=rect->h;
	uchar *pos0=srcData.pixels+x*srcData.advance+y*srcData.pitch,
		*pos1=dstData.pixels+x*dstData.advance+y*dstData.pitch;
	for (long y0=0;y0<h;y0++){
		uchar *pos00=pos0,
			*pos10=pos1;
		for (long x0=0;x0<w;x0++){
			ulong r0=RED_MONOCHROME(ulong(pos0[srcData.Roffset])),
				g0=GREEN_MONOCHROME(ulong(pos0[srcData.Goffset])),
				b0=BLUE_MONOCHROME(ulong(pos0[srcData.Boffset])),
				r1,g1,b1;
			r1=r0*ulong(color.r)+
				g0*ulong(color.r)+
				b0*ulong(color.r);
			r1/=255;
			g1=r0*ulong(color.g)+
				g0*ulong(color.g)+
				b0*ulong(color.g);
			g1/=255;
			b1=r0*ulong(color.b)+
				b0*ulong(color.b)+
				g0*ulong(color.b);
			b1/=255;
			pos1[dstData.Roffset]=(uchar)r1;
			pos1[dstData.Goffset]=(uchar)g1;
			pos1[dstData.Boffset]=(uchar)b1;
			pos0+=srcData.advance;
			pos1+=dstData.advance;
		}
		pos0=pos00+srcData.pitch;
		pos1=pos10+dstData.pitch;
	}
}

void effectMonochrome_threaded(void *parameters){
	filterParameters *p=(filterParameters *)parameters;
	effectMonochrome_threaded(p->src,p->dst,p->rect,p->color);
}

FILTER_EFFECT_F(effectMonochrome){
	if (!dst || dst->format->BitsPerPixel<24)
		return;
	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	SDL_Rect dstRect={(Sint16)x,(Sint16)y,(Uint16)w,(Uint16)h};
	if (cpu_count==1){
		effectMonochrome_threaded(src,dst,&dstRect,color);
		SDL_UnlockSurface(src);
		SDL_UnlockSurface(dst);
		return;
	}
#ifndef USE_THREAD_MANAGER
	NONS_Thread *threads=new NONS_Thread[cpu_count];
#endif
	SDL_Rect *rects=new SDL_Rect[cpu_count];
	filterParameters *parameters=new filterParameters[cpu_count];
	ulong division=ulong(float(dstRect.h)/float(cpu_count));
	ulong total=0;
	for (ushort a=0;a<cpu_count;a++){
		rects[a]=dstRect;
		rects[a].y+=Sint16(a*division);
		rects[a].h=Sint16(division);
		total+=rects[a].h;
		parameters[a].src=src;
		parameters[a].dst=dst;
		parameters[a].rect=rects+a;
		parameters[a].color=color;
	}
	rects[cpu_count-1].h+=Uint16(dstRect.h-total);
	for (ushort a=1;a<cpu_count;a++)
#ifndef USE_THREAD_MANAGER
		threads[a].call(effectMonochrome_threaded,parameters+a);
#else
		threadManager.call(a-1,effectMonochrome_threaded,parameters+a);
#endif
	effectMonochrome_threaded(parameters);
#ifndef USE_THREAD_MANAGER
	for (ushort a=1;a<cpu_count;a++)
		threads[a].join();
#else
	threadManager.waitAll();
#endif
	SDL_UnlockSurface(src);
	SDL_UnlockSurface(dst);
#ifndef USE_THREAD_MANAGER
	delete[] threads;
#endif
	delete[] rects;
	delete[] parameters;
}

void effectNegative_threaded(SDL_Surface *src,SDL_Surface *dst,SDL_Rect *rect){
	surfaceData srcData=src,
		dstData=dst;
	long x=rect->x,
		y=rect->y,
		w=rect->w,
		h=rect->h;
	uchar *pos0=srcData.pixels+x*srcData.advance+y*srcData.pitch,
		*pos1=dstData.pixels+x*dstData.advance+y*dstData.pitch;
	for (long y0=0;y0<h;y0++){
		uchar *pos00=pos0,
			*pos10=pos1;
		for (long x0=0;x0<w;x0++){
			pos1[dstData.Roffset]=~pos0[srcData.Roffset];
			pos1[dstData.Goffset]=~pos0[srcData.Goffset];
			pos1[dstData.Boffset]=~pos0[srcData.Boffset];
			pos0+=srcData.advance;
			pos1+=dstData.advance;
		}
		pos0=pos00+srcData.pitch;
		pos1=pos10+dstData.pitch;
	}
}

void effectNegative_threaded(void *parameters){
	filterParameters *p=(filterParameters *)parameters;
	effectNegative_threaded(p->src,p->dst,p->rect);
}

FILTER_EFFECT_F(effectNegative){
	if (!dst || dst->format->BitsPerPixel<24)
		return;
	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	SDL_Rect dstRect={(Sint32)x,(Sint32)y,(Uint32)w,(Uint32)h};
	if (cpu_count==1){
		effectNegative_threaded(src,dst,&dstRect);
		SDL_UnlockSurface(src);
		SDL_UnlockSurface(dst);
		return;
	}
#ifndef USE_THREAD_MANAGER
	NONS_Thread *threads=new NONS_Thread[cpu_count];
#endif
	SDL_Rect *rects=new SDL_Rect[cpu_count];
	filterParameters *parameters=new filterParameters[cpu_count];
	ulong division=ulong(float(dstRect.h)/float(cpu_count));
	ulong total=0;
	for (ushort a=0;a<cpu_count;a++){
		rects[a]=dstRect;
		rects[a].y+=Sint16(a*division);
		rects[a].h=Sint16(division);
		total+=rects[a].h;
		parameters[a].src=src;
		parameters[a].dst=dst;
		parameters[a].rect=rects+a;
	}
	rects[cpu_count-1].h+=Uint16(dstRect.h-total);
	for (ushort a=1;a<cpu_count;a++)
#ifndef USE_THREAD_MANAGER
		threads[a].call(effectNegative_threaded,parameters+a);
#else
		threadManager.call(a-1,effectNegative_threaded,parameters+a);
#endif
	effectNegative_threaded(parameters);
#ifndef USE_THREAD_MANAGER
	for (ushort a=1;a<cpu_count;a++)
		threads[a].join();
#else
	threadManager.waitAll();
#endif
	SDL_UnlockSurface(src);
	SDL_UnlockSurface(dst);
#ifndef USE_THREAD_MANAGER
	delete[] threads;
#endif
	delete[] rects;
	delete[] parameters;
}

//(Interpolation Function)
struct IF_parameters{
	SDL_Surface *src;
	SDL_Surface *dst;
	SDL_Rect *srcRect;
	SDL_Rect *dstRect;
	ulong x_factor;
	ulong y_factor;
};

void nearestNeighborInterpolation_threaded(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect,ulong x_factor,ulong y_factor);
void nearestNeighborInterpolation_threaded(void *parameters);

void nearestNeighborInterpolation(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect,ulong x_factor,ulong y_factor){
	if (!src || !dst)
		return;
	SDL_Rect srcRect0,dstRect0;
	//assign SDL_Rects
	if (!srcRect){
		srcRect0.x=0;
		srcRect0.y=0;
		srcRect0.w=src->w;
		srcRect0.h=src->h;
	}else
		srcRect0=*srcRect;
	if (!dstRect){
		dstRect0.x=0;
		dstRect0.y=0;
		dstRect0.w=dst->w;
		dstRect0.h=dst->h;
	}else
		dstRect0=*dstRect;
	//correct SDL_Rects
	if (srcRect0.x+srcRect0.w>src->w)
		srcRect0.w-=srcRect0.x+srcRect0.w-src->w;
	if (srcRect0.y+srcRect0.h>src->h)
		srcRect0.h-=srcRect0.y+srcRect0.h-src->h;
	if (dstRect0.x+dstRect0.w>dst->w)
		srcRect0.w-=dstRect0.x+srcRect0.w-dst->w;
	if (dstRect0.y+dstRect0.h>dst->h)
		srcRect0.h-=dstRect0.y+srcRect0.h-dst->h;
	//check SDL_Rects
	if (srcRect0.x<0 || srcRect0.x>=src->w || srcRect0.y<0 || srcRect0.y>=src->h || srcRect0.w<=0 || srcRect0.h<=0 ||
		dstRect0.x<0 || dstRect0.x>=dst->w || dstRect0.y<0 || dstRect0.y>=dst->h || dstRect0.w<=0 || dstRect0.h<=0)
		return;
	if (srcRect0.w==dstRect0.w && srcRect0.h==dstRect0.h){
		manualBlit(src,&srcRect0,dst,&dstRect0);
		return;
	}
	if (src->format->BitsPerPixel<24 || dst->format->BitsPerPixel<24)
		return;
	SDL_FillRect(dst,&dstRect0,dst->format->Amask|dst->format->Gmask);

#ifndef USE_THREAD_MANAGER
	NONS_Thread *threads=new NONS_Thread[cpu_count];
#endif
	SDL_Rect *rects0=new SDL_Rect[cpu_count];
	SDL_Rect *rects1=new SDL_Rect[cpu_count];
	IF_parameters *parameters=new IF_parameters[cpu_count];
	ulong division0=ulong(float(srcRect0.h)/float(cpu_count));
	ulong division1=ulong(float(division0)*(float(y_factor)/256));
	ulong total0=0,
		total1=0;
	for (ushort a=0;a<cpu_count;a++){
		rects0[a]=srcRect0;
		rects1[a]=dstRect0;
		rects0[a].y+=Sint16(a*division0);
		//rects0[a].h=Sint16(division0);
		rects1[a].y+=Sint16(a*division1);
		rects1[a].h=Sint16(division1);
		
		total0+=rects0[a].h;
		total1+=rects1[a].h;
		parameters[a].src=src;
		parameters[a].srcRect=rects0+a;
		parameters[a].dst=dst;
		parameters[a].dstRect=rects1+a;
		parameters[a].x_factor=x_factor;
		parameters[a].y_factor=y_factor;
	}
	//rects0[cpu_count-1].h+=srcRect0.h-total0;
	rects1[cpu_count-1].h+=Uint16(dstRect0.h-total1);

	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	for (ushort a=1;a<cpu_count;a++)
#ifndef USE_THREAD_MANAGER
		threads[a].call(nearestNeighborInterpolation_threaded,parameters+a);
#else
		threadManager.call(a-1,nearestNeighborInterpolation_threaded,parameters+a);
#endif
	nearestNeighborInterpolation_threaded(parameters);
#ifndef USE_THREAD_MANAGER
	for (ushort a=0;a<cpu_count;a++)
		threads[a].join();
#else
	threadManager.waitAll();
#endif
	SDL_UnlockSurface(src);
	SDL_UnlockSurface(dst);
#ifndef USE_THREAD_MANAGER
	delete[] threads;
#endif
	delete[] rects0;
	delete[] rects1;
	delete[] parameters;
}

void nearestNeighborInterpolation_threaded(void *parameters){
	IF_parameters *p=(IF_parameters *)parameters;
	nearestNeighborInterpolation_threaded(p->src,p->srcRect,p->dst,p->dstRect,p->x_factor,p->y_factor);
}

void nearestNeighborInterpolation_threaded(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect,ulong x_factor,ulong y_factor){
	SDL_Rect &srcRect0=*srcRect,
		&dstRect0=*dstRect;
	uchar *pixels0=(uchar *)src->pixels;
	uchar *pixels1=(uchar *)dst->pixels;
	uchar Roffset0=(src->format->Rshift)>>3;
	uchar Goffset0=(src->format->Gshift)>>3;
	uchar Boffset0=(src->format->Bshift)>>3;
	uchar Roffset1=(dst->format->Rshift)>>3;
	uchar Goffset1=(dst->format->Gshift)>>3;
	uchar Boffset1=(dst->format->Bshift)>>3;
	uchar advance0=src->format->BytesPerPixel;
	uchar advance1=dst->format->BytesPerPixel;
	Uint16 pitch0=src->pitch;
	Uint16 pitch1=dst->pitch;
	uchar *pos0=pixels0+srcRect0.x*advance0+srcRect0.y*pitch0;
	uchar *pos1=pixels1+dstRect0.x*advance1+dstRect0.y*pitch1;
	long errorX=0,errorY=0;
	ulong derrorX=!x_factor?(ulong(dstRect0.w)<<8)/srcRect0.w:x_factor;
	ulong derrorY=!y_factor?(ulong(dstRect0.h)<<8)/srcRect0.h:y_factor;
	/*SDL_Rect s=srcRect0,
		d=dstRect0;*/
	srcRect0.w+=srcRect0.x;
	dstRect0.w+=dstRect0.x;
	srcRect0.h+=srcRect0.y;
	dstRect0.h+=dstRect0.y;
	for (unsigned y0=srcRect0.y,y1=dstRect0.y;y0<srcRect0.h && y1<dstRect0.h;){
		uchar *pos00=pos0;
		errorY+=derrorY;
		//while (errorY>=0x80 && y1<dstRect0.h){
		while (errorY>0 && y1<dstRect0.h){
			uchar *pos10=pos1;
			errorX=0;
			for (unsigned x0=srcRect0.x,x1=dstRect0.x;x0<srcRect0.w && x1<dstRect0.w;){
				errorX+=derrorX;
				//while (errorX>=0x80 && x1<dstRect0.w){
				while (errorX>0 && x1<dstRect0.w){
					pos1[Roffset1]=pos0[Roffset0];
					pos1[Goffset1]=pos0[Goffset0];
					pos1[Boffset1]=pos0[Boffset0];
					errorX-=0x100;
					pos1+=advance1;
					x1++;
				}
				pos0+=advance0;
				x0++;
			}
			errorY-=0x100;
			pos1=pos10+pitch1;
			y1++;
			pos0=pos00;
		}
		pos0+=pitch0;
		y0++;
	}
}

void bilinearInterpolation_threaded(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect,ulong x_factor,ulong y_factor);
void bilinearInterpolation_threaded(void *parameters);

//This algorithm works well for scales [.5;inf.), but has VERY slight precision
//problems at scales <1.
void bilinearInterpolation(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect,ulong x_factor,ulong y_factor){
	if (!src || !dst)
		return;
	SDL_Rect srcRect0,dstRect0;
	//assign SDL_Rects
	if (!srcRect){
		srcRect0.x=0;
		srcRect0.y=0;
		srcRect0.w=src->w;
		srcRect0.h=src->h;
	}else
		srcRect0=*srcRect;
	if (!dstRect){
		dstRect0.x=0;
		dstRect0.y=0;
		dstRect0.w=dst->w;
		dstRect0.h=dst->h;
	}else
		dstRect0=*dstRect;
	//correct SDL_Rects
	if (srcRect0.x+srcRect0.w>src->w)
		srcRect0.w-=srcRect0.x+srcRect0.w-src->w;
	if (srcRect0.y+srcRect0.h>src->h)
		srcRect0.h-=srcRect0.y+srcRect0.h-src->h;
	if (dstRect0.x+dstRect0.w>dst->w)
		srcRect0.w-=dstRect0.x+srcRect0.w-dst->w;
	if (dstRect0.y+dstRect0.h>dst->h)
		srcRect0.h-=dstRect0.y+srcRect0.h-dst->h;
	//check SDL_Rects
	if (srcRect0.x<0 || srcRect0.x>=src->w || srcRect0.y<0 || srcRect0.y>=src->h || srcRect0.w<=0 || srcRect0.h<=0 ||
		dstRect0.x<0 || dstRect0.x>=dst->w || dstRect0.y<0 || dstRect0.y>=dst->h || dstRect0.w<=0 || dstRect0.h<=0)
		return;
	if (srcRect0.w==dstRect0.w && srcRect0.h==dstRect0.h){
		manualBlit(src,&srcRect0,dst,&dstRect0);
		return;
	}
	if (src->format->BitsPerPixel<24 || dst->format->BitsPerPixel<24)
		return;
	SDL_FillRect(dst,&dstRect0,dst->format->Amask|dst->format->Gmask);

#ifndef USE_THREAD_MANAGER
	NONS_Thread *threads=new NONS_Thread[cpu_count];
#endif
	SDL_Rect *rects0=new SDL_Rect[cpu_count];
	SDL_Rect *rects1=new SDL_Rect[cpu_count];
	IF_parameters *parameters=new IF_parameters[cpu_count];
	ulong division0=ulong(float(srcRect0.h)/float(cpu_count));
	//ulong division1=float(division0)*(float(y_factor)/256);
	ulong division1=ulong(float(dstRect0.h)/float(cpu_count));
	ulong total0=0,
		total1=0;
	for (ushort a=0;a<cpu_count;a++){
		rects0[a]=srcRect0;
		rects1[a]=dstRect0;
		rects0[a].y+=Sint16(total0);
		rects0[a].h=Sint16(division0);
		rects1[a].y+=Sint16(total1);
		rects1[a].h=Sint16(division1);
		
		total0+=rects0[a].h;
		total1+=rects1[a].h;
		parameters[a].src=src;
		parameters[a].srcRect=rects0+a;
		parameters[a].dst=dst;
		parameters[a].dstRect=rects1+a;
		parameters[a].x_factor=x_factor;
		parameters[a].y_factor=y_factor;
	}
	rects0[cpu_count-1].h+=Uint16(srcRect0.h-total0);
	rects1[cpu_count-1].h+=Uint16(dstRect0.h-total1);

	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	for (ushort a=1;a<cpu_count;a++)
#ifndef USE_THREAD_MANAGER
		threads[a].call(bilinearInterpolation_threaded,parameters+a);
#else
		threadManager.call(a-1,bilinearInterpolation_threaded,parameters+a);
#endif
	bilinearInterpolation_threaded(parameters);
#ifndef USE_THREAD_MANAGER
	for (ushort a=0;a<cpu_count;a++)
		threads[a].join();
#else
	threadManager.waitAll();
#endif
#ifndef USE_THREAD_MANAGER
	delete[] threads;
#endif
	delete[] rects0;
	delete[] rects1;
	delete[] parameters;
	SDL_UnlockSurface(src);
	SDL_UnlockSurface(dst);
}

void bilinearInterpolation_threaded(void *parameters){
	IF_parameters *p=(IF_parameters *)parameters;
	bilinearInterpolation_threaded(p->src,p->srcRect,p->dst,p->dstRect,p->x_factor,p->y_factor);
}

void bilinearInterpolation_threaded(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect,ulong x_factor,ulong y_factor){
	SDL_Rect srcRect0=*srcRect,
		dstRect0=*dstRect;
	uchar *pixels0=(uchar *)src->pixels;
	uchar *pixels1=(uchar *)dst->pixels;
	uchar Roffset0=(src->format->Rshift)>>3;
	uchar Goffset0=(src->format->Gshift)>>3;
	uchar Boffset0=(src->format->Bshift)>>3;
	uchar Roffset1=(dst->format->Rshift)>>3;
	uchar Goffset1=(dst->format->Gshift)>>3;
	uchar Boffset1=(dst->format->Bshift)>>3;
	uchar advance0=src->format->BytesPerPixel;
	uchar advance1=dst->format->BytesPerPixel;
	Uint16 pitch0=src->pitch;
	Uint16 pitch1=dst->pitch;
	uchar *pos0=pixels0;
	uchar *pos1=pixels1;
	long X=0,
		Y=0;
	ulong W0=src->clip_rect.w,
		H0=src->clip_rect.h,
		W1=dst->clip_rect.w,
		H1=dst->clip_rect.h;
	ulong advanceX=((W0-1)<<16)/W1,
		advanceY=((H0-1)<<16)/H1;
	srcRect0.w+=srcRect0.x;
	dstRect0.w+=dstRect0.x;
	srcRect0.h+=srcRect0.y;
	dstRect0.h+=dstRect0.y;
	for (long y1=dstRect0.y;y1<dstRect0.h;y1++){
		bool skipY=0;
		Y=(advanceY>>2)+(y1-dst->clip_rect.y)*advanceY;
		long y0=Y>>16;
		if ((ulong)y0+1>=H0)
			break;
		uchar *pos00=pos0+pitch0*y0;
		ulong fractionY=Y&0xFFFF,
			ifractionY=0xFFFF-fractionY;
		for (long x1=dstRect0.x;x1<dstRect0.w;x1++){
			bool skipX=0;
			pos1=pixels1+pitch1*y1+advance1*x1;
			X=(advanceX>>2)+(x1-dst->clip_rect.x)*advanceX;
			long x0=X>>16;
			if ((ulong)x0+1>=W0)
				break;
			ulong fractionX=X&0xFFFF,
				ifractionX=0xFFFF-fractionX;
			uchar *pixel0=pos00+advance0*x0,
				*pixel1=pixel0+advance0,
				*pixel2=pixel0+pitch0,
				*pixel3=pixel1+pitch0;
			ulong weight0=((ifractionX>>8)*ifractionY)>>8,
				weight1=((fractionX>>8)*ifractionY)>>8,
				weight2=((ifractionX>>8)*fractionY)>>8,
				weight3=((fractionX>>8)*fractionY)>>8;
			ulong r0,g0,b0;
			uchar *r1=pos1+Roffset1;
			uchar *g1=pos1+Goffset1;
			uchar *b1=pos1+Boffset1;
			if (weight0){
				r0=pixel0[Roffset0]*weight0;
				g0=pixel0[Goffset0]*weight0;
				b0=pixel0[Boffset0]*weight0;
			}else{
				r0=0;
				g0=0;
				b0=0;
			}
			if (weight1){
				if (!skipX){
					r0+=pixel1[Roffset0]*weight1;
					g0+=pixel1[Goffset0]*weight1;
					b0+=pixel1[Boffset0]*weight1;
				}else{
					r0+=pixel0[Roffset0]*weight1;
					g0+=pixel0[Goffset0]*weight1;
					b0+=pixel0[Boffset0]*weight1;
				}
			}
			if (weight2){
				if (!skipY){
					r0+=pixel2[Roffset0]*weight2;
					g0+=pixel2[Goffset0]*weight2;
					b0+=pixel2[Boffset0]*weight2;
				}else{
					r0+=pixel0[Roffset0]*weight2;
					g0+=pixel0[Goffset0]*weight2;
					b0+=pixel0[Boffset0]*weight2;
				}
			}
			if (weight3){
				if (!skipX && !skipY){
					r0+=pixel3[Roffset0]*weight3;
					g0+=pixel3[Goffset0]*weight3;
					b0+=pixel3[Boffset0]*weight3;
				}else{
					r0+=pixel0[Roffset0]*weight3;
					g0+=pixel0[Goffset0]*weight3;
					b0+=pixel0[Boffset0]*weight3;
				}
			}
			*r1=uchar(r0/0xFFFF);
			*g1=uchar(g0/0xFFFF);
			*b1=uchar(b0/0xFFFF);
		}
	}
}

#define FLOOR(x) ((x)&0xFFFF0000)
#define CEIL(x) (((x)&0xFFFF)?(FLOOR(x)+0x10000):FLOOR(x))

//This algorithm works well for scales [0;1). It's ~50% more expensive than
//bilinearInterpolation().
void bilinearInterpolation2_threaded(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect,ulong x_factor,ulong y_factor);
void bilinearInterpolation2_threaded(void *parameters);

void bilinearInterpolation2(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect,ulong x_factor,ulong y_factor){
	if (!src || !dst)
		return;
	SDL_Rect srcRect0,dstRect0;
	//assign SDL_Rects
	if (!srcRect){
		srcRect0.x=0;
		srcRect0.y=0;
		srcRect0.w=src->w;
		srcRect0.h=src->h;
	}else
		srcRect0=*srcRect;
	if (!dstRect){
		dstRect0.x=0;
		dstRect0.y=0;
		dstRect0.w=dst->w;
		dstRect0.h=dst->h;
	}else
		dstRect0=*dstRect;
	//correct SDL_Rects
	if (srcRect0.x+srcRect0.w>src->w)
		srcRect0.w-=srcRect0.x+srcRect0.w-src->w;
	if (srcRect0.y+srcRect0.h>src->h)
		srcRect0.h-=srcRect0.y+srcRect0.h-src->h;
	if (dstRect0.x+dstRect0.w>dst->w)
		srcRect0.w-=dstRect0.x+srcRect0.w-dst->w;
	if (dstRect0.y+dstRect0.h>dst->h)
		srcRect0.h-=dstRect0.y+srcRect0.h-dst->h;
	//check SDL_Rects
	if (srcRect0.x<0 || srcRect0.x>=src->w || srcRect0.y<0 || srcRect0.y>=src->h || srcRect0.w<=0 || srcRect0.h<=0 ||
		dstRect0.x<0 || dstRect0.x>=dst->w || dstRect0.y<0 || dstRect0.y>=dst->h || dstRect0.w<=0 || dstRect0.h<=0)
		return;
	if (srcRect0.w==dstRect0.w && srcRect0.h==dstRect0.h){
		manualBlit(src,&srcRect0,dst,&dstRect0);
		return;
	}
	if (src->format->BitsPerPixel<24 || dst->format->BitsPerPixel<24)
		return;
	SDL_FillRect(dst,&dstRect0,dst->format->Amask|dst->format->Gmask);

#ifndef USE_THREAD_MANAGER
	NONS_Thread *threads=new NONS_Thread[cpu_count];
#endif
	SDL_Rect *rects0=new SDL_Rect[cpu_count];
	SDL_Rect *rects1=new SDL_Rect[cpu_count];
	IF_parameters *parameters=new IF_parameters[cpu_count];
	ulong division0=ulong(float(srcRect0.h)/float(cpu_count));
	ulong division1=ulong(float(dstRect0.h)/float(cpu_count));
	ulong total1=0;
	for (ushort a=0;a<cpu_count;a++){
		rects0[a]=srcRect0;
		rects1[a]=dstRect0;
		rects0[a].y+=Sint16(a*division0);
		rects1[a].y+=Sint16(a*division1);
		rects1[a].h=Sint16(division1);
		
		total1+=rects1[a].h;
		parameters[a].src=src;
		parameters[a].srcRect=rects0+a;
		parameters[a].dst=dst;
		parameters[a].dstRect=rects1+a;
		parameters[a].x_factor=x_factor;
		parameters[a].y_factor=y_factor;
	}
	rects1[cpu_count-1].h+=Uint16(dstRect0.h-total1);

	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	for (ushort a=1;a<cpu_count;a++)
#ifndef USE_THREAD_MANAGER
		threads[a].call(bilinearInterpolation2_threaded,parameters+a);
#else
		threadManager.call(a-1,bilinearInterpolation2_threaded,parameters+a);
#endif
	bilinearInterpolation2_threaded(parameters);
#ifndef USE_THREAD_MANAGER
	for (ushort a=0;a<cpu_count;a++)
		threads[a].join();
#else
	threadManager.waitAll();
#endif
	SDL_UnlockSurface(src);
	SDL_UnlockSurface(dst);
#ifndef USE_THREAD_MANAGER
	delete[] threads;
#endif
	delete[] rects0;
	delete[] rects1;
	delete[] parameters;
}

void bilinearInterpolation2_threaded(void *parameters){
	IF_parameters *p=(IF_parameters *)parameters;
	bilinearInterpolation2_threaded(p->src,p->srcRect,p->dst,p->dstRect,p->x_factor,p->y_factor);
}

void bilinearInterpolation2_threaded(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect,ulong x_factor,ulong y_factor){
	SDL_Rect srcRect0=*srcRect,
		dstRect0=*dstRect;
	uchar *pixels0=(uchar *)src->pixels;
	uchar *pixels1=(uchar *)dst->pixels;
	uchar Roffset0=(src->format->Rshift)>>3;
	uchar Goffset0=(src->format->Gshift)>>3;
	uchar Boffset0=(src->format->Bshift)>>3;
	uchar Roffset1=(dst->format->Rshift)>>3;
	uchar Goffset1=(dst->format->Gshift)>>3;
	uchar Boffset1=(dst->format->Bshift)>>3;
	uchar advance0=src->format->BytesPerPixel;
	uchar advance1=dst->format->BytesPerPixel;
	Uint16 pitch0=src->pitch;
	Uint16 pitch1=dst->pitch;
	uchar *pos0=pixels0;
	uchar *pos1=pixels1;
	Uint32 sizeX=(src->clip_rect.w<<16)/dst->clip_rect.w,
		sizeY=(src->clip_rect.h<<16)/dst->clip_rect.h;
	srcRect0.w+=srcRect0.x;
	dstRect0.w+=dstRect0.x;
	srcRect0.h+=srcRect0.y;
	dstRect0.h+=dstRect0.y;
	Uint32 Y0=(dstRect0.y-dst->clip_rect.y)*sizeY,
		Y1=Y0+sizeY,
		X0,X1;
	Uint32 area=((sizeX>>8)*sizeY)>>8;
	uchar *pixel1=pos1+pitch1*dstRect0.y+advance1*dstRect0.x;
	for (long y1=dstRect0.y;y1<dstRect0.h;y1++){
		X0=(dstRect0.x-dst->clip_rect.x)*sizeX,
		X1=X0+sizeX;
		uchar *row=pos0+pitch0*(Y0>>16);
		uchar *pixel10=pixel1;
		for (long x1=dstRect0.x;x1<dstRect0.w;x1++){
			uchar *pixel0=row+advance0*(X0>>16);
			unsigned r=0,g=0,b=0;
			for (Uint32 y2=Y0;y2<Y1;){
				Uint32 ymultiplier;
				uchar *pixel00=pixel0;
				if (Y1-y2<0x10000)
					ymultiplier=Y1-y2;
				else if (y2==Y0)
					ymultiplier=FLOOR(Y0)+0x10000-Y0;
				else
					ymultiplier=0x10000;
				for (Uint32 x2=X0;x2<X1;){
					Uint32 xmultiplier;
					if (X1-x2<0x10000)
						xmultiplier=X1-x2;
					else if (x2==X0)
						xmultiplier=FLOOR(X0)+0x10000-X0;
					else
						xmultiplier=0x10000;
					Uint32 compound_mutiplier=((ymultiplier>>8)*xmultiplier)>>8;
					r+=pixel0[Roffset0]*compound_mutiplier;
					g+=pixel0[Goffset0]*compound_mutiplier;
					b+=pixel0[Boffset0]*compound_mutiplier;
					pixel0+=advance0;
					x2=FLOOR(x2)+0x10000;
				}
				pixel0=pixel00+pitch0;
				y2=FLOOR(y2)+0x10000;
			}
			pixel1[Roffset1]=r/area;
			pixel1[Goffset1]=g/area;
			pixel1[Boffset1]=b/area;
			pixel1+=advance1;
			X0=X1;
			X1+=sizeX;
		}
		pixel1=pixel10+pitch1;
		Y0=Y1;
		Y1+=sizeY;
	}
}
