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

#include "Plugin.h"
#include "../Functions.h"
#include <iostream>
#include <vector>

/*
	Available functions:
		void manualBlit(SDL_Surface *src, SDL_Rect *srcRect, SDL_Surface *dst, SDL_Rect *dstRect, manualBlitAlpha_t alpha=255);
			Parameters:
				src: The source surface.
				srcRect: The rectangle from src that will be blitted onto dst. If zero is passed, the
					reactangle is taken to be the entire surface.
				dst: The destination surface.
				dstRect: The top-left coordinate at which the source rectangle will be blitted.
					SDL_Rect::w and SDL_Rect::h are ignored. For the width and height, the width and
					height of srcRect are used.	If zero is passed, the coordinate is taken to be 0,0.
				alpha: An overall alpha that is applied to the source before blitting it. The source
					is not modified.
			Returns:
				Nothing.
			Description:
				Blits a surface onto another. If the source has alpha or abs(alpha)<255, alpha
				blending is applied. For alpha blending, the source is understood as being on top of
				the destination. If alpha<0, the resulting pixel is color-inverted.
				The function uses threading internally, so it shouldn't be called from #01, #02, or
				#03.

	Available classes (the members that can't be used are not listed):
		class NONS_Mutex{
		public:
			NONS_Mutex();
			~NONS_Mutex();
			void lock();
			void unlock();
		};
			Wrapper for system-specific mutexes. Supports Windows and UNIX (any UNIX-like with pthreads).

		class NONS_ThreadManager{
		public:
			ulong call(ulong onThread,NONS_ThreadedFunctionPointer f,void *p);
				Call using thread #onThread a function 'f' passing a structure 'p'.
				onThread should be >=0 and <cpu_count.
				NONS_ThreadedFunctionPointer is defined as:
					typedef void (*NONS_ThreadedFunctionPointer)(void *);
				The function is of course non-locking.
			void wait(ulong index);
				Lock the current thread until the managed thread #index returns from its function. If
				the isn't in a function already, the call does nothing.
			void waitAll();
				Calls NONS_ThreadManager::wait() for every thread.
		};
			Manages persistent threads. Supports Windows and UNIX (any UNIX-like with pthreads).

		struct NONS_VirtualScreen{
			static const size_t screens_s=4;
			SDL_Surface *screens[screens_s];
				Surfaces used by the screen under different conditions.
				screens[screens_s-1] is always the real screen.
				User functions should not write to anything other than screens[VIRTUAL].
			bool fullscreen;
				True if we're currently in fullscreen mode.

			void blitToScreen(SDL_Surface *src,SDL_Rect *srcrect,SDL_Rect *dstrect);
				Equivalent to
					manualBlit(src,srcrect,screen->screens[VIRTUAL],dstrect);
			void updateScreen(ulong x,ulong y,ulong w,ulong h,bool fast=0);
				Refreshes a rectangle of the screen.
				'fast' is used when interpolation is being used. It's better not to use it.
			void updateWholeScreen(bool fast=0);
			void updateWithoutLock(bool fast=0);
				Same as updateWholeScreen(), but doesn't lock screenMutex. Should not be used if
				screenMutex is unlocked.
		};

		class NONS_MutexLocker{
			NONS_Mutex &mutex;
		public:
			NONS_MutexLocker(NONS_Mutex &m):mutex(m){
				this->mutex.lock();
			}
			~NONS_MutexLocker(){
				this->mutex.unlock();
			}
		};
	Available data:
		ulong cpu_count;
			The number of physical cores in the system. When threading is disabled, this is always 1.
		bool ctrlIsPressed;
			Whether the user is holding down CTRL. Also becomes true if the user pressed period or
			selected a menu option to skip.
		bool forceSkip;
			Whether a function has asynchronously requested termination.
		NONS_ThreadManager threadManager;
			Allows the program to recycle the same threads, avoiding creating new ones and reducing
			the overall threading overhead.
		NONS_Mutex screenMutex;
			Used to lock accesses to the screen, to prevent race conditions. See transitionExample()
			for an example on how to use it.
*/

//Declare new functions here.
FILTER_EFFECT_F(filterExample);
TRANSIC_EFFECT_F(transitionExample);
ASYNC_EFFECT_F(asyncExample);
ASYNC_EFFECT_INIT_F(asyncExample_init);
ASYNC_EFFECT_UNINIT_F(asyncExample_uninit);

void *getTransitionEffectFunctions(){
	static std::vector<transitionFX_f> ret;
	if (!ret.size()){
		//Add new functions here.
		ret.push_back(transitionExample);
	}
	return &ret;
}

void *getFilterEffectFunctions(){
	static std::vector<filterFX_f> ret;
	if (!ret.size()){
		//Add new functions here.
		ret.push_back(filterExample);
	}
	return &ret;
}

void *getAsyncEffectFunctions(){
	static std::vector<asyncFXfunctionSet> ret;
	if (!ret.size()){
		//Add new functions here.
		ret.push_back(asyncFXfunctionSet(asyncExample_init,asyncExample,asyncExample_uninit));
	}
	return &ret;
}

extern "C" PLUGIN_DLLexport void *getFunctionPointers(void *param){
	switch ((ulong)param){
		case 0:
			return getTransitionEffectFunctions();
		case 1:
			return getFilterEffectFunctions();
		case 2:
			return getAsyncEffectFunctions();
	}
	return 0;
}

//Define new functions here.

/*
Asynchronous effect initialization function (#01):

	Signature:
		void *asyncExample_init(ulong effectNo)
	Parameters:
		effectNo: The effect number.
	Returns:
		A generic pointer that can be used by the matching effect function and will be freed by the
		matching uninitializer function.
	Description: 
		Can be used to initialize state used by #03. If it's not necessary, it can be left undefined
		and pass a zero to asyncFXfunctionSet::asyncFXfunctionSet()'s first parameter. The engine
		will not call it in that case.
*/
ASYNC_EFFECT_INIT_F(asyncExample_init){
	return new ulong(0);
}

/*
Asynchronous effect uninitialization function (#02):

	Signature:
		void *asyncExample_uninit(ulong effectNo, void *userData)
	Parameters:
		effectNo: The effect number.
		userData: The generic pointer that was returned by #01.
	Returns:
		Nothing.
	Description:
		Can be used to free state allocated by #01. If it's not necessary, it can be left undefined
		and pass a zero to asyncFXfunctionSet::asyncFXfunctionSet()'s third parameter. The engine
		will not call it in that case.
*/
ASYNC_EFFECT_UNINIT_F(asyncExample_uninit){
	delete (ulong *)userData;
}

/*
Asynchronous effect function (#03):

	Signature:
		bool asyncExample(ulong effectNo, surfaceData srcData, surfaceData dstData, void *userData)
	Parameters:
		effectNo: The effect number.
		srcData: Information about the source surface:
			uchar *pixels;		Pointer to the start of the pixel information
			uchar Roffset,		Offset of the red channel from the start of a pixel, in bytes
				Goffset,		Offset of the greed channel
				Boffset,		Offset of the blue channel
				Aoffset;		Offset of the alpha channel
								Note: The destination surface is guaranteed to ignore the alpha channel
			ulong advance,		The size of each pixel in bytes
				pitch,			The size of each row in bytes
				w,				The width of the surface in pixels
				h;				The height of the surface
			bool alpha;			Does the surface has an alpha channel?
		dstData: Information about the destination surface.
		userData: The generic pointer that was returned by #01.
	Returns:
		A bool indicating whether the screen should be refreshed afterwards.
	Description:
		The function is called n times per second (n being the second parameter for asynceffect in
		the script) to apply an effect. That means that, on average, the function shouldn't take
		longer than 1000/n milliseconds to finish, so performance is a priority. To give a general
		idea, this example function takes ~11 ms on a Core 2 Duo @ 1.68 GHz.
		This function should not use threading, since it's already running on its own thread.
		Adding more threading wouldn't improve the performance, specially in single core systems.
		Unless the engine was compiled with threading disabled, manualBlit() uses threads, so it
		shouldn't be called from here, either.
*/
ASYNC_EFFECT_F(asyncExample){
	ulong *n=(ulong *)userData;
	uchar *pos_src=srcData.pixels;
	uchar *pos_dst=dstData.pixels;
	for (ulong y=0;y<dstData.h;y++){
		uchar *pos_src0=pos_src;
		uchar *pos_dst0=pos_dst;
		if (y!=*n){
			for (ulong x=0;x<dstData.w;x++){
				ulong v=rand()&0xFF;
				//c = c*7/8 + v*1/8 (It's an alpha blend with the noise layer at 12.5% alpha.)
				pos_dst[dstData.Roffset]=uchar((ulong(pos_src[srcData.Roffset])*7+v)>>3);
				pos_dst[dstData.Goffset]=uchar((ulong(pos_src[srcData.Goffset])*7+v)>>3);
				pos_dst[dstData.Boffset]=uchar((ulong(pos_src[srcData.Boffset])*7+v)>>3);
				pos_src+=srcData.advance;
				pos_dst+=dstData.advance;
			}
		}else
			memset(pos_dst,0xFF,dstData.pitch);
		pos_src=pos_src0+srcData.pitch;
		pos_dst=pos_dst0+dstData.pitch;
	}
	*n=(*n+2)%dstData.h;
	return 1;
}

/*
Filter effect function (#04):

	Signature:
		void filterExample(
			ulong effectNo,
			SDL_Color color,
			SDL_Surface *src,
			SDL_Surface *rule,
			SDL_Surface *dst,
			ulong x,
			ulong y,
			ulong w,
			ulong h
		)
	Parameters:
		effectNo: The effect number.
		color: A color value. If this isn't used, it can be ignored. The color is obtained from the
			second parameter provided for effects (e.g. the fourth parameter for the command effect).
		src: The source surface.
		rule: A "rule" surface that can be used for some effect. E.g. the rule could be an image that
		      will be alpha-blended into the image.
		      The rule is obtained from the third parameter provided for effects (e.g. the fourth
		      parameter for the command effect). If the rule cannot be found as a file, an invalid
		      pointer (zero) will be passed as this parameter. So if this parameter is needed, that
		      should be checked.
		dst: The destination surface.
		x: The coordinate of the left side of the rectangle that needs to be filtered.
		y: The coordinate of the top side of the rectangle that needs to be filtered.
		w: The width of the rectangle that needs to be filtered.
		h: The height of the rectangle that needs to be filtered.
	Returns:
		Nothing.
	Description:
		The function is used to apply a filter to a surface. For example, the engine internally uses
		this very interface to implement the function behind the command nega. In fact, this example
		function performs the exact same effect.
		Depending on what commands were used in the script, src and dst may actually always point to
		the same surface. However, this shouldn't be relied on.
		The function is always calle from the main thread, so it's allowed to use threading
		functions to speed up its execution. However, the function is, to my knowledge, never
		called from performance-critical sections.
*/
FILTER_EFFECT_F(filterExample){
	//Obtain information about the surfaces.
	surfaceData srcData=src,
		dstData=dst;
	//Advance pointers to the top-left corner of the rectangle being filtered.
	srcData.pixels+=x*srcData.advance+y*srcData.pitch;
	dstData.pixels+=x*dstData.advance+y*dstData.pitch;
	for (y=0;y<h;y++){
		uchar *pos00=srcData.pixels;
		uchar *pos10=dstData.pixels;
		for (ulong x=0;x<w;x++){
			//This is a faster version of c=255-c.
			dstData.pixels[dstData.Roffset]=~srcData.pixels[srcData.Roffset];
			dstData.pixels[dstData.Goffset]=~srcData.pixels[srcData.Goffset];
			dstData.pixels[dstData.Boffset]=~srcData.pixels[srcData.Boffset];
			//Move pointers to the next pixel.
			srcData.pixels+=srcData.advance;
			dstData.pixels+=dstData.advance;
		}
		//Move pointers to the next row, relative from the start of the current row.
		srcData.pixels=pos00+srcData.pitch;
		dstData.pixels=pos10+dstData.pitch;
	}
}

/*
Transition effect function (#05):

	Signature:
		void transitionExample(
			ulong effectNo,
			ulong duration,
			SDL_Surface *src,
			SDL_Surface *rule,
			NONS_VirtualScreen *dst
		)
	Parameters:
		effectNo: The effect number.
		duration: The function shouldn't take longer to complete than this many milliseconds.
		src: The source surface.
		rule: The "rule" surface. See #04 for details.
		dst: The destination virtual screen.
	Returns:
		Nothing.
	Description:
		This is the only effect that doesn't write directly to an SDL_Surface. This is because
		a transition effect is expected to update the screen several times before completing.
		Transition effects are also allowed to use threading functions, which may be important
		for more complex and expensive effects.
*/
TRANSIC_EFFECT_F(transitionExample){
	//If the user is pressing CTRL or exit was requested...
	if (CURRENTLYSKIPPING){
		//...blit the source to the screen in one step...
		dst->blitToScreen(src,0,0);
		//...and update the screen.
		dst->updateWholeScreen();
		return;
	}
	SDL_Rect rect={0,0,1,src->h};
	long w=src->w;
	//Each for step should last this long.
	float delay=float(duration)/float(w);
	long idealtimepos=0,
		lastT=9999,
		start=SDL_GetTicks();
	for (long a=0;a<w;a++){
		idealtimepos+=(long)delay;
		long t0=SDL_GetTicks();
		//If we're behind schedule or the user is pressing CTRL or exit was requested, and we're not
		//at the last step (because the screen should be written to at least once)...
		if ((t0-start-idealtimepos>lastT || CURRENTLYSKIPPING) && a<w-1){
			//...skip this step.
			rect.w++;
			continue;
		}
		{
			//Lock the screen.
			//Because as many as three different threads could be trying to access the screen at any
			//given time, all attempts to use any element of dst->screens should be locked. If this
			//Isn't done, the program could crash, say, when the user tries to take a screenshot or
			//go to fullscreen.
			NONS_MutexLocker ml(screenMutex);
			//dst->screens[VIRTUAL] is the only place that should be written to. Writing to any other
			//element of dst->screens could crash the engine.
			manualBlit(src,&rect,dst->screens[VIRTUAL],&rect);
			//Thanks to resource acquisition is initialization (RAII), we don't need to explictly unlock
			//the mutex. Merely leaving this block will destruct 'ml', unlocking the mutex.
		}
		dst->updateScreen(rect.x,rect.y,rect.w,rect.h);
		rect.x+=rect.w;
		rect.w=1;
		long t1=SDL_GetTicks();
		lastT=t1-t0;
		//If running this step took less time than necessary...
		if (lastT<delay)
			//...wait the rest of the delay.
			SDL_Delay(Uint32(delay-lastT));
	}
}
