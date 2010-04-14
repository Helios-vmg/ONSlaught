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

#define USE_ACCURATE_MULTIPLICATION

#include "GFX.h"
#include "IOFunctions.h"
#include "ThreadManager.h"
#include "ImageLoader.h"
#include "Plugin/LibraryLoader.h"
#include <cmath>
#include <iostream>

#if defined _DEBUG || 1
#define BENCHMARK_EFFECTS
#endif

bool NONS_GFX::listsInitialized=0;
std::vector<filterFX_f> NONS_GFX::filters;
std::vector<transitionFX_f> NONS_GFX::transitions;

//(Parallelized surface function)
struct PSF_parameters{
	SDL_Surface *src;
	SDL_Rect *srcRect;
	SDL_Surface *dst;
	SDL_Rect *dstRect;
	uchar alpha;
	SDL_Color color;
};

ulong NONS_GFX::effectblank=0;

NONS_GFX::NONS_GFX(ulong effect,ulong duration,const std::wstring *rule){
	this->effect=effect;
	this->duration=duration;
	if (rule)
		this->rule=*rule;
	this->type=TRANSITION;
	this->stored=0;
	this->color.r=0;
	this->color.g=0;
	this->color.b=0;
	this->color.unused=0;
}

NONS_GFX::NONS_GFX(const NONS_GFX &b){
	*this=b;
}

NONS_GFX &NONS_GFX::operator=(const NONS_GFX &b){
	this->effect=b.effect;
	this->duration=b.duration;
	this->rule=b.rule;
	this->type=b.type;
	this->stored=b.stored;
	return *this;
}

void NONS_GFX::initializeLists(){
	NONS_GFX::listsInitialized=1;
	NONS_GFX::filters.push_back(effectMonochrome);
	NONS_GFX::filters.push_back(effectNegative);
	pluginLibraryFunction fp=(pluginLibraryFunction)pluginLibrary.getFunction("getFunctionPointers");
	if (!fp)
		return;
	{
		//1=Get filter effect function pointers.
		std::vector<filterFX_f> vec=*(std::vector<filterFX_f> *)fp((void *)1);
		NONS_GFX::filters.insert(NONS_GFX::filters.end(),vec.begin(),vec.end());
	}
	{
		//0=Get transition effect function pointers.
		std::vector<transitionFX_f> vec=*(std::vector<transitionFX_f> *)fp((void *)0);
		NONS_GFX::transitions.insert(NONS_GFX::transitions.end(),vec.begin(),vec.end());
	}
}

ErrorCode NONS_GFX::callEffect(ulong number,long duration,const std::wstring *rule,SDL_Surface *src,SDL_Surface *dst0,NONS_VirtualScreen *dst){
	NONS_GFX effect(number,duration,rule);
	ErrorCode ret=effect.call(src,dst0,dst);
	return ret;
}

ErrorCode NONS_GFX::callFilter(ulong number,const SDL_Color &color,const std::wstring &rule,SDL_Surface *src,SDL_Surface *dst){
	NONS_GFX effect;
	effect.effect=number;
	effect.color=color;
	effect.rule=rule;
	effect.type=POSTPROCESSING;
	ErrorCode ret=effect.call(src,dst,0);
	return ret;
}

ErrorCode NONS_GFX::call(SDL_Surface *src,SDL_Surface *dst0,NONS_VirtualScreen *dst){
	typedef void(NONS_GFX::*transitionFunction)(SDL_Surface *,SDL_Surface *,NONS_VirtualScreen *);
	static transitionFunction builtInTransitions[]={
		&NONS_GFX::effectNothing,		//0
		&NONS_GFX::effectOnlyUpdate,	//1
		&NONS_GFX::effectRshutter,		//2
		&NONS_GFX::effectLshutter,		//3
		&NONS_GFX::effectDshutter,		//4
		&NONS_GFX::effectUshutter,		//5
		&NONS_GFX::effectRcurtain,		//6
		&NONS_GFX::effectLcurtain,		//7
		&NONS_GFX::effectDcurtain,		//8
		&NONS_GFX::effectUcurtain,		//9
		&NONS_GFX::effectCrossfade,		//10
		&NONS_GFX::effectRscroll,		//11
		&NONS_GFX::effectLscroll,		//12
		&NONS_GFX::effectDscroll,		//13
		&NONS_GFX::effectUscroll,		//14
		&NONS_GFX::effectHardMask,		//15
		&NONS_GFX::effectMosaicIn,		//16
		&NONS_GFX::effectMosaicOut,		//17
		&NONS_GFX::effectSoftMask		//18
	};
	if (!NONS_GFX::listsInitialized)
		NONS_GFX::initializeLists();
	//ulong t0=SDL_GetTicks();
	SDL_Surface *ruleFile=0;
	if (this->rule.size())
		ImageLoader->fetchSprite(ruleFile,this->rule);
	if (this->type==TRANSITION){
		if (this->effect<=18){
			(this->*(builtInTransitions[this->effect]))(src,ruleFile,dst);
		}else if (this->effect-19<NONS_GFX::transitions.size()){
			NONS_GFX::transitions[this->effect-19](this->effect,this->duration,src,ruleFile,dst);
			if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
				waitNonCancellable(NONS_GFX::effectblank);
		}else{
			if (ruleFile)
				ImageLoader->unfetchImage(ruleFile);
			return NONS_NO_EFFECT;
		}
	}else{
		if (this->effect<NONS_GFX::filters.size())
			NONS_GFX::filters[this->effect](this->effect+1,this->color,src,0,dst0,0,0,src->w,src->h);
		else{
			if (ruleFile)
				ImageLoader->unfetchImage(ruleFile);
			return NONS_NO_EFFECT;
		}
	}
	if (ruleFile)
		ImageLoader->unfetchImage(ruleFile);
	//Unused:
	//ulong t1=SDL_GetTicks();
	return NONS_NO_ERROR;
}

void NONS_GFX::effectNothing(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
}

void NONS_GFX::effectOnlyUpdate(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	if (!src0 || !dst)
		return;
	{
		NONS_MutexLocker ml(screenMutex);
		manualBlit(src0,0,dst->screens[VIRTUAL],0);
	}
	dst->updateWholeScreen();
}

void NONS_GFX::effectRshutter(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	{
		NONS_MutexLocker ml(screenMutex);
		if (!src0 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
			return;
	}
	if (CURRENTLYSKIPPING){
		dst->blitToScreen(src0,0,0);
		dst->updateWholeScreen();
		return;
	}
	long shutterW=src0->w/40;
	SDL_Rect rect={0,0,1,src0->h};
	float delay=float(this->duration)/float(shutterW);
	long idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
	for (long a=0;a<shutterW;a++){
		idealtimepos+=(long)delay;
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && a<shutterW-1){
			rect.w++;
			continue;
		}
		rect.x=Sint16(a-rect.w+1);
		{
			NONS_MutexLocker ml(screenMutex);
			for (long b=0;b<40;b++){
				SDL_BlitSurface(src0,&rect,dst->screens[VIRTUAL],&rect);
				rect.x+=Sint16(shutterW);
			}
			dst->updateWithoutLock();
		}
		rect.w=1;
		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
	}
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
}

void NONS_GFX::effectLshutter(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	{
		NONS_MutexLocker ml(screenMutex);
		if (!src0 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
			return;
	}
	if (CURRENTLYSKIPPING){
		dst->blitToScreen(src0,0,0);
		dst->updateWholeScreen();
		return;
	}
	long shutterW=src0->w/40;
	SDL_Rect rect={0,0,1,src0->h};
	float delay=float(this->duration)/float(shutterW);
	//Unused:
	long //realtimepos=0,
		idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
	for (long a=shutterW-1;a>=0;a--){
		idealtimepos+=(ulong)delay;
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && a>0){
			rect.w++;
			continue;
		}
		rect.x=(Sint16)a;
		{
			NONS_MutexLocker ml(screenMutex);
			for (long b=0;b<40;b++){
				SDL_BlitSurface(src0,&rect,dst->screens[VIRTUAL],&rect);
				rect.x+=(Sint16)shutterW;
			}
			dst->updateWithoutLock();
		}
		rect.w=1;
		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
	}
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
}

void NONS_GFX::effectDshutter(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	{
		NONS_MutexLocker ml(screenMutex);
		if (!src0 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
			return;
	}
	if (CURRENTLYSKIPPING){
		dst->blitToScreen(src0,0,0);
		dst->updateWholeScreen();
		return;
	}
	long shutterH=src0->h/30;
	SDL_Rect rect={0,0,src0->w,1};
	float delay=float(this->duration)/float(shutterH);
	//Unused:
	long //realtimepos=0,
		idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
	for (long a=0;a<shutterH;a++){
		idealtimepos+=(long)delay;
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && a<shutterH-1){
			rect.h++;
			continue;
		}
		rect.y=Sint16(a-rect.h+1);
		{
			NONS_MutexLocker ml(screenMutex);
			for (long b=0;b<30;b++){
				SDL_BlitSurface(src0,&rect,dst->screens[VIRTUAL],&rect);
				rect.y+=(Sint16)shutterH;
			}
			dst->updateWithoutLock();
		}
		rect.h=1;
		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
	}
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
}

void NONS_GFX::effectUshutter(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	{
		NONS_MutexLocker ml(screenMutex);
		if (!src0 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
			return;
	}
	if (CURRENTLYSKIPPING){
		dst->blitToScreen(src0,0,0);
		dst->updateWholeScreen();
		return;
	}
	long shutterH=src0->h/30;
	SDL_Rect rect={0,0,src0->w,1};
	float delay=float(this->duration)/float(shutterH);
	long idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
	for (long a=shutterH-1;a>=0;a--){
		idealtimepos+=(long)delay;
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && a>0){
			rect.h++;
			continue;
		}
		rect.y=(Sint16)a;
		{
			NONS_MutexLocker ml(screenMutex);
			for (long b=0;b<30;b++){
				SDL_BlitSurface(src0,&rect,dst->screens[VIRTUAL],&rect);
				rect.y+=(Sint16)shutterH;
			}
			dst->updateWithoutLock();
		}
		rect.h=1;
		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
	}
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
}

void NONS_GFX::effectRcurtain(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	{
		NONS_MutexLocker ml(screenMutex);
		if (!src0 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
			return;
	}
	if (CURRENTLYSKIPPING){
		dst->blitToScreen(src0,0,0);
		dst->updateWholeScreen();
		return;
	}
	long shutterW=(long)sqrt(double(src0->w));
	SDL_Rect rect={0,0,1,src0->w};
	float delay=float(this->duration)/float(shutterW*2);
	long idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
	for (long a=0;a<shutterW*2;a++){
		idealtimepos+=(long)delay;
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && a<shutterW-1){
			rect.w++;
			continue;
		}
		rect.x=Sint16(a-rect.w+1);
		{
			NONS_MutexLocker ml(screenMutex);
			for (long b=0;b<=a;b++){
				manualBlit(src0,&rect,dst->screens[VIRTUAL],&rect);
				rect.x+=(Sint16)shutterW;
			}
			dst->updateWithoutLock();
		}
		rect.w=1;
		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
	}
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
}

void NONS_GFX::effectLcurtain(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	{
		NONS_MutexLocker ml(screenMutex);
		if (!src0 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
			return;
	}
	if (CURRENTLYSKIPPING){
		dst->blitToScreen(src0,0,0);
		dst->updateWholeScreen();
		return;
	}
	long w=src0->w,
		shutterW=(long)sqrt(double(w));
	SDL_Rect rect={0,0,1,src0->h};
	float delay=float(this->duration)/float(shutterW*2);
	long idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
	for (long a=0;a<shutterW*2;a++){
		idealtimepos+=(long)delay;
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && a<shutterW-1){
			rect.w++;
			continue;
		}
		rect.x=Sint16(w-a)/*-rect.w+1*/;
		{
			NONS_MutexLocker ml(screenMutex);
			for (long b=0;b<=a;b++){
				manualBlit(src0,&rect,dst->screens[VIRTUAL],&rect);
				rect.x-=(Sint16)shutterW;
			}
			dst->updateWithoutLock();
		}
		rect.w=1;
		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
	}
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
}

void NONS_GFX::effectDcurtain(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	{
		NONS_MutexLocker ml(screenMutex);
		if (!src0 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
			return;
	}
	if (CURRENTLYSKIPPING){
		dst->blitToScreen(src0,0,0);
		dst->updateWholeScreen();
		return;
	}
	long shutterH=(long)sqrt(double(src0->h));
	SDL_Rect rect={0,0,src0->w,1};
	float delay=float(this->duration)/float(shutterH*2);
	long idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
	for (long a=0;a<shutterH*2;a++){
		idealtimepos+=(long)delay;
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && a<shutterH-1){
			rect.h++;
			continue;
		}
		rect.y=Sint16(a-rect.h+1);
		{
			NONS_MutexLocker ml(screenMutex);
			for (long b=0;b<=a;b++){
				manualBlit(src0,&rect,dst->screens[VIRTUAL],&rect);
				rect.y+=(Sint16)shutterH;
			}
			dst->updateWithoutLock();
		}
		rect.h=1;
		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
	}
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
}

void NONS_GFX::effectUcurtain(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	{
		NONS_MutexLocker ml(screenMutex);
		if (!src0 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
			return;
	}
	if (CURRENTLYSKIPPING){
		dst->blitToScreen(src0,0,0);
		dst->updateWholeScreen();
		return;
	}
	long h=src0->h,
		shutterH=(long)sqrt(double(h));
	SDL_Rect rect={0,0,src0->w,1};
	float delay=float(this->duration)/float(shutterH*2);
	long idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
	for (long a=0;a<shutterH*2;a++){
		idealtimepos+=(long)delay;
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && a<shutterH-1){
			rect.h++;
			continue;
		}
		rect.y=Sint16(h-a)/*-rect.h+1*/;
		{
			NONS_MutexLocker ml(screenMutex);
			for (long b=0;b<=a;b++){
				manualBlit(src0,&rect,dst->screens[VIRTUAL],&rect);
				rect.y-=(Sint16)shutterH;
			}
			dst->updateWithoutLock();
		}
		rect.h=1;
		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
	}
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
}

void NONS_GFX::effectCrossfade(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	{
		NONS_MutexLocker ml(screenMutex);
		if (!src0 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
			return;
	}
	if (CURRENTLYSKIPPING){
		dst->blitToScreen(src0,0,0);
		dst->updateWholeScreen();
		return;
	}
	const long step=1;
	SDL_Surface *copyDst;
	{
		NONS_MutexLocker ml(screenMutex);
		copyDst=makeSurface(dst->screens[VIRTUAL]->w,dst->screens[VIRTUAL]->h,32);
		manualBlit(dst->screens[VIRTUAL],0,copyDst,0);
	}
	float delay=float(this->duration)/float(256/step);
	long idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
#ifdef BENCHMARK_EFFECTS
	ulong steps=0;
#endif
	for (long a=step;a<256;a+=step){
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && a<255){
			idealtimepos+=(long)delay;
			continue;
		}
		{
			NONS_MutexLocker ml(screenMutex);
			manualBlit(copyDst,0,dst->screens[VIRTUAL],0);
			manualBlit(src0,0,dst->screens[VIRTUAL],0,a);
			dst->updateWithoutLock();
		}
#ifdef BENCHMARK_EFFECTS
		steps++;
#endif
		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
		idealtimepos+=(long)delay;
	}
#ifdef BENCHMARK_EFFECTS
	std::cout <<"effectCrossfade(): "<<float(steps)/(float(this->duration)/1000.0f)<<" steps per second."<<std::endl;
#endif
	SDL_FreeSurface(copyDst);
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
}

void NONS_GFX::effectRscroll(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	{
		NONS_MutexLocker ml(screenMutex);
		if (!src0 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
			return;
	}
	if (CURRENTLYSKIPPING){
		dst->blitToScreen(src0,0,0);
		dst->updateWholeScreen();
		return;
	}
	SDL_Surface *srcCopy;
	{
		NONS_MutexLocker ml(screenMutex);
		srcCopy=copySurface(dst->screens[VIRTUAL]);
	}
	float delay=float(this->duration)/float(src0->w);
	long idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
	SDL_Rect srcrectA={0,0,src0->w,src0->h},
		srcrectB=srcrectA,
		dstrectA=srcrectA,
		dstrectB=srcrectA;
	for (long a=src0->w-1;a>=0;a--){
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && a>0){
			idealtimepos+=(long)delay;
			continue;
		}
		srcrectA.x=(Sint16)a;
		dstrectB.x=srcrectA.w-srcrectA.x;
		{
			NONS_MutexLocker ml(screenMutex);
			manualBlit(src0,&srcrectA,dst->screens[VIRTUAL],&dstrectA);
			manualBlit(srcCopy,&srcrectB,dst->screens[VIRTUAL],&dstrectB);
			dst->updateWithoutLock();
		}
		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
		idealtimepos+=(long)delay;
	}
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
	SDL_FreeSurface(srcCopy);
}

void NONS_GFX::effectLscroll(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	{
		NONS_MutexLocker ml(screenMutex);
		if (!src0 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
			return;
	}
	if (CURRENTLYSKIPPING){
		dst->blitToScreen(src0,0,0);
		dst->updateWholeScreen();
		return;
	}
	SDL_Surface *srcCopy;
	{
		NONS_MutexLocker ml(screenMutex);
		srcCopy=copySurface(dst->screens[VIRTUAL]);
	}
	float delay=float(this->duration)/float(src0->w);
	long idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
	SDL_Rect srcrectA={0,0,src0->w,src0->h},
		srcrectB=srcrectA,
		dstrectA=srcrectA,
		dstrectB=srcrectA;
	for (ulong a=0;a<(ulong)src0->w;a++){
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && a<(ulong)src0->w-1){
			idealtimepos+=(long)delay;
			continue;
		}
		srcrectA.x=(Sint16)a;
		dstrectB.x=srcrectA.w-srcrectA.x;
		{
			NONS_MutexLocker ml(screenMutex);
			manualBlit(srcCopy,&srcrectA,dst->screens[VIRTUAL],&dstrectA);
			manualBlit(src0,&srcrectB,dst->screens[VIRTUAL],&dstrectB);
			dst->updateWithoutLock();
		}
		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
		idealtimepos+=(long)delay;
	}
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
	SDL_FreeSurface(srcCopy);
}

void NONS_GFX::effectDscroll(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	{
		NONS_MutexLocker ml(screenMutex);
		if (!src0 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
			return;
	}
	if (CURRENTLYSKIPPING){
		dst->blitToScreen(src0,0,0);
		dst->updateWholeScreen();
		return;
	}
	SDL_Surface *srcCopy;
	{
		NONS_MutexLocker ml(screenMutex);
		srcCopy=copySurface(dst->screens[VIRTUAL]);
	}
	float delay=float(this->duration)/float(src0->h);
	long idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
	SDL_Rect srcrectA={0,0,src0->w,src0->h},
		srcrectB=srcrectA,
		dstrectA=srcrectA,
		dstrectB=srcrectA;
	for (long a=src0->h-1;a>=0;a--){
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && a>0){
			idealtimepos+=(long)delay;
			continue;
		}
		srcrectA.y=(Sint16)a;
		dstrectB.y=srcrectA.h-srcrectA.y;
		{
			NONS_MutexLocker ml(screenMutex);
			manualBlit(src0,&srcrectA,dst->screens[VIRTUAL],&dstrectA);
			manualBlit(srcCopy,&srcrectB,dst->screens[VIRTUAL],&dstrectB);
			dst->updateWithoutLock();
		}
		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
		idealtimepos+=(long)delay;
	}
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
	SDL_FreeSurface(srcCopy);
}

void NONS_GFX::effectUscroll(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	{
		NONS_MutexLocker ml(screenMutex);
		if (!src0 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
			return;
	}
	if (CURRENTLYSKIPPING){
		dst->blitToScreen(src0,0,0);
		dst->updateWholeScreen();
		return;
	}
	SDL_Surface *srcCopy;
	{
		NONS_MutexLocker ml(screenMutex);
		srcCopy=copySurface(dst->screens[VIRTUAL]);
	}
	float delay=float(this->duration)/float(src0->h);
	long idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
	SDL_Rect srcrectA={0,0,src0->w,src0->h},
		srcrectB=srcrectA,
		dstrectA=srcrectA,
		dstrectB=srcrectA;
	for (ulong a=0;a<(ulong)src0->h;a++){
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && a<(ulong)src0->h-1){
			idealtimepos+=(long)delay;
			continue;
		}
		srcrectA.y=(Sint16)a;
		dstrectB.y=srcrectA.h-srcrectA.y;
		{
			NONS_MutexLocker ml(screenMutex);
			manualBlit(srcCopy,&srcrectA,dst->screens[VIRTUAL],&dstrectA);
			manualBlit(src0,&srcrectB,dst->screens[VIRTUAL],&dstrectB);
			dst->updateWithoutLock();
		}
		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
		idealtimepos+=(long)delay;
	}
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
	SDL_FreeSurface(srcCopy);
}

struct EHM_parameters{
	uchar *pos0,
		*pos1,
		*pos2;
	uchar Roffset0,
		Goffset0,
		Boffset0,
		Boffset1,
		Roffset2,
		Goffset2,
		Boffset2;
	ulong advance0,
		advance1,
		advance2,
		pitch0,
		pitch1,
		pitch2;
	int w,h;
	long a;
};

void effectHardMask_threaded(void *parameters){
	EHM_parameters *p=(EHM_parameters *)parameters;
	uchar *pos0=p->pos0,
		*pos1=p->pos1,
		*pos2=p->pos2;
	for (int y0=0;y0<p->h;y0++){
		for (int x0=0;x0<p->w;x0++){
			uchar r0=pos0[p->Roffset0];
			uchar g0=pos0[p->Goffset0];
			uchar b0=pos0[p->Boffset0];

			uchar b1=pos1[p->Boffset1];
			uchar *b12=pos1+p->Boffset1;

			uchar *r2=pos2+p->Roffset2;
			uchar *g2=pos2+p->Goffset2;
			uchar *b2=pos2+p->Boffset2;

			if (b1<=p->a){
				*r2=r0;
				*g2=g0;
				*b2=b0;
				*b12=0xFF;
			}

			pos0+=p->advance0;
			pos1+=p->advance1;
			pos2+=p->advance2;
		}
	}
}

void NONS_GFX::effectHardMask(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
#ifndef USE_THREAD_MANAGER
	NONS_Thread *threads;
#endif
	EHM_parameters *parameters;
	SDL_Surface *copyMask;
	{
		NONS_MutexLocker ml(screenMutex);
		if (!src0 || !src1 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
			return;
		if (CURRENTLYSKIPPING){
			dst->blitToScreen(src0,0,0);
			dst->updateWithoutLock();
			return;
		}
		copyMask=makeSurface(dst->screens[VIRTUAL]->w,dst->screens[VIRTUAL]->h,32);
		SDL_Rect srcrect={0,0,src1->w,src1->h},
			dstrect=srcrect;
		//copy the rule as a tile
		for (dstrect.y=0;dstrect.y<dst->screens[VIRTUAL]->h;dstrect.y+=srcrect.h)
			for (dstrect.x=0;dstrect.x<dst->screens[VIRTUAL]->w;dstrect.x+=srcrect.w)
				manualBlit(src1,&srcrect,copyMask,&dstrect);
		src1=copyMask;
		//prepare for threading
#ifndef USE_THREAD_MANAGER
		threads=new NONS_Thread[cpu_count];
#endif
		parameters=new EHM_parameters[cpu_count];
		ulong division=ulong(float(dst->screens[VIRTUAL]->h)/float(cpu_count));
		ulong total=0;
		parameters[0].pos0=(uchar *)src0->pixels;
		parameters[0].pos1=(uchar *)src1->pixels;
		parameters[0].pos2=(uchar *)dst->screens[VIRTUAL]->pixels;
		parameters[0].Roffset0=(src0->format->Rshift)>>3;
		parameters[0].Goffset0=(src0->format->Gshift)>>3;
		parameters[0].Boffset0=(src0->format->Bshift)>>3;
		parameters[0].Boffset1=(src1->format->Bshift)>>3;
		parameters[0].Roffset2=(dst->screens[VIRTUAL]->format->Rshift)>>3;
		parameters[0].Goffset2=(dst->screens[VIRTUAL]->format->Gshift)>>3;
		parameters[0].Boffset2=(dst->screens[VIRTUAL]->format->Bshift)>>3;
		parameters[0].advance0=src0->format->BytesPerPixel;
		parameters[0].advance1=src1->format->BytesPerPixel;
		parameters[0].advance2=dst->screens[VIRTUAL]->format->BytesPerPixel;
		parameters[0].pitch0=src0->pitch;
		parameters[0].pitch1=src1->pitch;
		parameters[0].pitch2=dst->screens[VIRTUAL]->pitch;
		parameters[0].w=dst->screens[VIRTUAL]->w;
		parameters[0].h=dst->screens[VIRTUAL]->h;
		for (ushort a=0;a<cpu_count;a++){
			parameters[a]=parameters[0];
			parameters[a].pos0+=parameters[a].pitch0*Sint16(a*division);
			parameters[a].pos1+=parameters[a].pitch1*Sint16(a*division);
			parameters[a].pos2+=parameters[a].pitch2*Sint16(a*division);
			parameters[a].h=Sint16(division);
			total+=parameters[a].h;
		}
		parameters[cpu_count].h+=dst->screens[VIRTUAL]->h-total;
	}

	float delay=float(this->duration)/256.0f;
	long idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
	for (long a=0;a<256;a++){
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && a<255){
			idealtimepos+=(long)delay;
			continue;
		}

		{
			NONS_MutexLocker ml(screenMutex);
			SDL_LockSurface(src0);
			SDL_LockSurface(src1);
			SDL_LockSurface(dst->screens[VIRTUAL]);

			for (ushort b=0;b<cpu_count;b++)
				parameters[b].a=a;
			for (ushort b=1;b<cpu_count;b++)
#ifndef USE_THREAD_MANAGER
				threads[b].call(effectHardMask_threaded,parameters+b);
#else
				threadManager.call(b-1,effectHardMask_threaded,parameters+b);
#endif
			effectHardMask_threaded(parameters);
#ifndef USE_THREAD_MANAGER
			for (ushort b=1;b<cpu_count;b++)
				threads[b].join();
#else
			threadManager.waitAll();
#endif

			SDL_UnlockSurface(dst->screens[VIRTUAL]);
			SDL_UnlockSurface(src1);
			SDL_UnlockSurface(src0);
			dst->updateWithoutLock();
		}

		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
		idealtimepos+=(long)delay;
	}
#ifndef USE_THREAD_MANAGER
	delete[] threads;
#endif
	delete[] parameters;
	SDL_FreeSurface(copyMask);
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
}

struct ESM_parameters{
	uchar *pos0,
		*pos1,
		*pos2;
	uchar Roffset0,
		Goffset0,
		Boffset0,
		Boffset1,
		Roffset2,
		Goffset2,
		Boffset2;
	ulong advance0,
		advance1,
		advance2,
		pitch0,
		pitch1,
		pitch2;
	int w,h;
	long a;
};

void effectSoftMask_threaded(void *parameters){
	EHM_parameters *p=(EHM_parameters *)parameters;
	uchar *pos0=p->pos0,
		*pos1=p->pos1,
		*pos2=p->pos2;
	for (int y0=0;y0<p->h;y0++){
		for (int x0=0;x0<p->w;x0++){
			short r0=pos0[p->Roffset0];
			short g0=pos0[p->Goffset0];
			short b0=pos0[p->Boffset0];

			uchar b1=pos1[p->Boffset1];
			uchar *b12=pos1+p->Boffset1;

			uchar *r2=pos2+p->Roffset2;
			uchar *g2=pos2+p->Goffset2;
			uchar *b2=pos2+p->Boffset2;

			if (long(b1)<=p->a){
				short alpha=short(p->a-b1);
#define APPLY_ALPHA(c0,c1,a) (INTEGER_MULTIPLICATION((a)^0xFF,(c1))+INTEGER_MULTIPLICATION((a),(c0)))
				/*short deltar=r0-*r2;
				short deltag=g0-*g2;
				short deltab=b0-*b2;*/
				if (alpha<0)
					alpha=0;
				if (alpha>255)
					alpha=255;
				*r2=APPLY_ALPHA(r0,*r2,alpha);
				*g2=APPLY_ALPHA(g0,*g2,alpha);
				*b2=APPLY_ALPHA(b0,*b2,alpha);
				/*
				(*r2)+=INTEGER_MULTIPLICATION(deltar,alpha);
				(*g2)+=INTEGER_MULTIPLICATION(deltag,alpha);
				(*b2)+=INTEGER_MULTIPLICATION(deltab,alpha);
				*/
				if (long(b1)<p->a-255)
					*b12=0;
			}

			pos0+=p->advance0;
			pos1+=p->advance1;
			pos2+=p->advance2;
		}
	}
}

void NONS_GFX::effectSoftMask(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	if (!src0 || !src1 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
		return;
	if (CURRENTLYSKIPPING){
		dst->blitToScreen(src0,0,0);
		dst->updateWholeScreen();
		return;
	}
	SDL_Surface *copyDst;
	SDL_Surface *copyMask;
	{
		NONS_MutexLocker ml(screenMutex);
		copyDst=makeSurface(dst->screens[VIRTUAL]->w,dst->screens[VIRTUAL]->h,32);
		manualBlit(dst->screens[VIRTUAL],0,copyDst,0);
		copyMask=makeSurface(dst->screens[VIRTUAL]->w,dst->screens[VIRTUAL]->h,32);
		SDL_Rect srcrect={0,0,src1->w,src1->h},
			dstrect=srcrect;
		for (dstrect.y=0;dstrect.y<dst->screens[VIRTUAL]->h;dstrect.y+=srcrect.h)
			for (dstrect.x=0;dstrect.x<dst->screens[VIRTUAL]->w;dstrect.x+=srcrect.w)
				manualBlit(src1,&srcrect,copyMask,&dstrect);
	}
	src1=copyMask;
	//prepare for threading
#ifndef USE_THREAD_MANAGER
	NONS_Thread *threads=new NONS_Thread[cpu_count];
#endif
	ESM_parameters *parameters=new ESM_parameters[cpu_count];
	ulong division=ulong(float(dst->screens[VIRTUAL]->h)/float(cpu_count));
	ulong total=0;
	parameters[0].pos0=(uchar *)src0->pixels;
	parameters[0].pos1=(uchar *)src1->pixels;
	parameters[0].pos2=(uchar *)dst->screens[VIRTUAL]->pixels;
	parameters[0].Roffset0=(src0->format->Rshift)>>3;
	parameters[0].Goffset0=(src0->format->Gshift)>>3;
	parameters[0].Boffset0=(src0->format->Bshift)>>3;
	parameters[0].Boffset1=(src1->format->Bshift)>>3;
	parameters[0].Roffset2=(dst->screens[VIRTUAL]->format->Rshift)>>3;
	parameters[0].Goffset2=(dst->screens[VIRTUAL]->format->Gshift)>>3;
	parameters[0].Boffset2=(dst->screens[VIRTUAL]->format->Bshift)>>3;
	parameters[0].advance0=src0->format->BytesPerPixel;
	parameters[0].advance1=src1->format->BytesPerPixel;
	parameters[0].advance2=dst->screens[VIRTUAL]->format->BytesPerPixel;
	parameters[0].pitch0=src0->pitch;
	parameters[0].pitch1=src1->pitch;
	parameters[0].pitch2=dst->screens[VIRTUAL]->pitch;
	parameters[0].w=dst->screens[VIRTUAL]->w;
	parameters[0].h=Sint16(division);
	total=parameters[0].h;
	for (ushort a=1;a<cpu_count;a++){
		parameters[a]=parameters[0];
		parameters[a].pos0+=parameters[a].pitch0*Sint16(a*division);
		parameters[a].pos1+=parameters[a].pitch1*Sint16(a*division);
		parameters[a].pos2+=parameters[a].pitch2*Sint16(a*division);
		total+=parameters[a].h;
	}
	parameters[cpu_count].h+=dst->screens[VIRTUAL]->h-total;

	float delay=float(this->duration)/512.0f;
	long idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
#ifdef BENCHMARK_EFFECTS
	ulong steps=0;
#endif
	for (long a=0;a<512;a++){
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && a<511){
			idealtimepos+=(long)delay;
			continue;
		}

		{
			NONS_MutexLocker ml(screenMutex);

			parameters[0].pos2=(uchar *)dst->screens[VIRTUAL]->pixels;
			for (ushort b=1;b<cpu_count;b++)
				parameters[b].pos2=parameters[0].pos2+parameters[b].pitch2*Sint16(b*division);

			manualBlit(copyDst,0,dst->screens[VIRTUAL],0);
			//long w0=dst->screens[VIRTUAL]->w,
			//	h0=dst->screens[VIRTUAL]->h;
			SDL_LockSurface(src0);
			SDL_LockSurface(src1);
			SDL_LockSurface(dst->screens[VIRTUAL]);

			for (ushort b=0;b<cpu_count;b++)
				parameters[b].a=a;
			for (ushort b=1;b<cpu_count;b++)
#ifndef USE_THREAD_MANAGER
				threads[b].call(effectSoftMask_threaded,parameters+b);
#else
				threadManager.call(b-1,effectSoftMask_threaded,parameters+b);
#endif
			effectSoftMask_threaded(parameters);
#ifndef USE_THREAD_MANAGER
			for (ushort b=1;b<cpu_count;b++)
				threads[b].join();
#else
			threadManager.waitAll();
#endif

			SDL_UnlockSurface(dst->screens[VIRTUAL]);
			SDL_UnlockSurface(src1);
			SDL_UnlockSurface(src0);
			dst->updateWithoutLock();
		}
#ifdef BENCHMARK_EFFECTS
		steps++;
#endif

		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
		idealtimepos+=(long)delay;
	}
#ifdef BENCHMARK_EFFECTS
	std::cout <<"effectSoftMask(): "<<float(steps)/(float(this->duration)/1000.0f)<<" steps per second."<<std::endl;
#endif
#ifndef USE_THREAD_MANAGER
	delete[] threads;
#endif
	delete[] parameters;
	SDL_FreeSurface(copyMask);
	SDL_FreeSurface(copyDst);
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
}

void NONS_GFX::effectMosaicIn(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	{
		NONS_MutexLocker ml(screenMutex);
		if (!src0 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
			return;
	}
	if (CURRENTLYSKIPPING){
		dst->blitToScreen(src0,0,0);
		dst->updateWholeScreen();
		return;
	}
	float delay=float(this->duration)/10.0f;
	long idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
	for (long a=9;a>=0;a--){
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && lastT>5 && a>0){
			idealtimepos+=(long)delay;
			continue;
		}
		{
			NONS_MutexLocker ml(screenMutex);
			if (a==0){
				manualBlit(src0,0,dst->screens[VIRTUAL],0);
			}else{
				SDL_LockSurface(src0);
				long advance=src0->format->BytesPerPixel;
				long pixelSize=1;
				for (short b=0;b<a;b++) pixelSize*=2;
				for (long y=0;y<dst->screens[VIRTUAL]->h;y+=pixelSize){
					long tps=y+pixelSize-1>=dst->screens[VIRTUAL]->h?dst->screens[VIRTUAL]->h-y-1:pixelSize;
					for (long x=0;x<dst->screens[VIRTUAL]->w;x+=pixelSize){
						SDL_Rect rect={
							(Sint16)x,
							(Sint16)y,
							(Sint16)pixelSize,
							(Sint16)pixelSize
						};
						uchar *pos0=(uchar *)src0->pixels;
						pos0+=src0->pitch*(y+tps-1)+advance*x;
						uchar Roffset0=(src0->format->Rshift)>>3;
						uchar Goffset0=(src0->format->Gshift)>>3;
						uchar Boffset0=(src0->format->Bshift)>>3;
						unsigned r=pos0[Roffset0],
							g=pos0[Goffset0],
							b=pos0[Boffset0];
						r<<=dst->screens[VIRTUAL]->format->Rshift;
						g<<=dst->screens[VIRTUAL]->format->Gshift;
						b<<=dst->screens[VIRTUAL]->format->Bshift;
						r|=g|b;
						SDL_FillRect(dst->screens[VIRTUAL],&rect,r);
					}
				}
				SDL_UnlockSurface(src0);
			}
			dst->updateWithoutLock();
		}
		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
		idealtimepos+=(long)delay;
	}
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
}

void NONS_GFX::effectMosaicOut(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst){
	long advance;
	uchar Roffset0,
		Goffset0,
		Boffset0;
	{
		NONS_MutexLocker ml(screenMutex);
		if (!src0 || !dst || src0->w!=dst->screens[VIRTUAL]->w || src0->h!=dst->screens[VIRTUAL]->h)
			return;
		if (CURRENTLYSKIPPING){
			dst->blitToScreen(src0,0,0);
			dst->updateWithoutLock();
			return;
		}
		advance=dst->screens[VIRTUAL]->format->BytesPerPixel;
		Roffset0=(dst->screens[VIRTUAL]->format->Rshift)>>3;
		Goffset0=(dst->screens[VIRTUAL]->format->Gshift)>>3;
		Boffset0=(dst->screens[VIRTUAL]->format->Bshift)>>3;
	}
	float delay=float(this->duration)/10.0f;
	long idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
	for (long a=0;a<10;a++){
		long t0=SDL_GetTicks();
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && lastT>5 && a<9){
			idealtimepos+=(long)delay;
			continue;
		}
		{
			NONS_MutexLocker ml(screenMutex);
			SDL_LockSurface(src0);
			long pixelSize=1;
			for (short b=0;b<a;b++) pixelSize*=2;
			for (long y=0;y<dst->screens[VIRTUAL]->h;y+=pixelSize){
				//long tps;
				long tps=y+pixelSize-1>=dst->screens[VIRTUAL]->h?dst->screens[VIRTUAL]->h-y-1:pixelSize;
				for (long x=0;x<dst->screens[VIRTUAL]->w;x+=pixelSize){
					SDL_Rect rect={
						(Sint16)x,
						(Sint16)y,
						(Sint16)pixelSize,
						(Sint16)pixelSize
					};
					uchar *pos0=(uchar *)dst->screens[VIRTUAL]->pixels;
					pos0+=dst->screens[VIRTUAL]->pitch*(y+tps-1)+advance*x;
					unsigned r=pos0[Roffset0],
						g=pos0[Goffset0],
						b=pos0[Boffset0];
					r<<=dst->screens[VIRTUAL]->format->Rshift;
					g<<=dst->screens[VIRTUAL]->format->Gshift;
					b<<=dst->screens[VIRTUAL]->format->Bshift;
					r|=g|b;
					SDL_FillRect(dst->screens[VIRTUAL],&rect,r);
				}
			}
			SDL_UnlockSurface(src0);
			dst->updateWithoutLock();
		}
		long t1=SDL_GetTicks();
		lastT=t1-t0;
		if (lastT<delay)
			SDL_Delay(Uint32(delay-lastT));
		idealtimepos+=(long)delay;
	}
	manualBlit(src0,0,dst->screens[VIRTUAL],0);
	if (!CURRENTLYSKIPPING && NONS_GFX::effectblank)
		waitNonCancellable(NONS_GFX::effectblank);
}

NONS_GFXstore::NONS_GFXstore(){
	this->effects[0]=new NONS_GFX();
	this->effects[0]->stored=1;
	this->effects[1]=new NONS_GFX(1);
	this->effects[1]->stored=1;
}

NONS_GFXstore::~NONS_GFXstore(){
	for (std::map<ulong,NONS_GFX *>::iterator i=this->effects.begin();i!=this->effects.begin();i++)
		delete i->second;
}

NONS_GFX *NONS_GFXstore::retrieve(ulong code){
	std::map<ulong,NONS_GFX *>::iterator i=this->effects.find(code);
	if (i==this->effects.end())
		return 0;
	return i->second;
}

bool NONS_GFXstore::remove(ulong code){
	std::map<ulong,NONS_GFX *>::iterator i=this->effects.find(code);
	if (i==this->effects.end())
		return 0;
	delete i->second;
	this->effects.erase(i);
	return 1;
}

NONS_GFX *NONS_GFXstore::add(ulong code,ulong effect,ulong duration,const std::wstring *rule){
	if (this->retrieve(code))
		return 0;
	NONS_GFX *res=new NONS_GFX(effect,duration,rule);
	this->effects[code]=res;
	return res;
}
