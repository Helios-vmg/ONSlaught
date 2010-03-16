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

#ifndef NONS_GFX_H
#define NONS_GFX_H

#include "Common.h"
#include "ErrorCodes.h"
#include "VirtualScreen.h"
#include <SDL/SDL.h>
#include <set>
#include <map>

enum{
	TRANSITION=0,
	POSTPROCESSING=1
};

typedef void(*transitionFX_f)(ulong,ulong,SDL_Surface *,SDL_Surface *,NONS_VirtualScreen *);
#define TRANSIC_EFFECT_F(name) void name(ulong effectNo,ulong duration,SDL_Surface *src,SDL_Surface *rule,NONS_VirtualScreen *dst)

struct NONS_GFX{
	ulong effect;
	ulong duration;
	std::wstring rule;
	long type;
	bool stored;
	SDL_Color color;
	static ulong effectblank;
	static bool listsInitialized;
	static std::vector<filterFX_f> filters;
	static std::vector<transitionFX_f> transitions;

	NONS_GFX(ulong effect=0,ulong duration=0,const std::wstring *rule=0);
	NONS_GFX(const NONS_GFX &b);
	NONS_GFX &operator=(const NONS_GFX &b);
	static ErrorCode callEffect(ulong number,long duration,const std::wstring *rule,SDL_Surface *src,SDL_Surface *dst0,NONS_VirtualScreen *dst);
	static ErrorCode callFilter(ulong number,const SDL_Color &color,const std::wstring &rule,SDL_Surface *src,SDL_Surface *dst);
	ErrorCode call(SDL_Surface *src,SDL_Surface *dst0,NONS_VirtualScreen *dst);
	void effectNothing(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectOnlyUpdate(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectRshutter(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectLshutter(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectDshutter(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectUshutter(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectRcurtain(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectLcurtain(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectDcurtain(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectUcurtain(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectCrossfade(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectRscroll(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectLscroll(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectDscroll(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectUscroll(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectHardMask(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectMosaicIn(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectMosaicOut(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);
	void effectSoftMask(SDL_Surface *src0,SDL_Surface *src1,NONS_VirtualScreen *dst);

	static void initializeLists();
};

struct NONS_GFXstore{
	std::map<ulong,NONS_GFX *> effects;
	NONS_GFX *add(ulong code,ulong effect,ulong duration,const std::wstring *rule=0);
	NONS_GFX *retrieve(ulong code);
	bool remove(ulong code);
	NONS_GFXstore();
	~NONS_GFXstore();
};
#endif
