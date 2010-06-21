/*
* Copyright (c) 2010, Helios (helios.vmg@gmail.com)
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

#include <cstdio>
#include <stdint.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <SDL/SDL.h>
#include <SDL/SDL_mutex.h>
#include <vlc/vlc.h>
#include "../../video_player.h"
#include "../common.h"

typedef unsigned char uchar;
typedef unsigned long ulong;

#if NONS_SYS_WINDOWS
#include <windows.h>
#elif NONS_SYS_UNIX
#include <unistd.h>
#elif NONS_SYS_PSP
#undef NONS_PARALLELIZE
#endif

class NONS_Event{
	bool initialized;
#if NONS_SYS_WINDOWS
	HANDLE event;
#elif NONS_SYS_UNIX
	sem_t sem;
#endif
public:
	NONS_Event():initialized(0){}
	void init();
	~NONS_Event();
	void set();
	void reset();
	void wait();
};

void NONS_Event::init(){
#if NONS_SYS_WINDOWS
	this->event=CreateEvent(0,0,0,0);
#elif NONS_SYS_UNIX
	sem_init(&this->sem,0,0);
#elif NONS_SYS_PSP
	this->sem=SDL_CreateSemaphore(0);
#endif
	this->initialized=1;
}

NONS_Event::~NONS_Event(){
	if (!this->initialized)
		return;
#if NONS_SYS_WINDOWS
	CloseHandle(this->event);
#elif NONS_SYS_UNIX
	sem_destroy(&this->sem);
#elif NONS_SYS_PSP
	SDL_DestroySemaphore(this->sem);
#endif
}

void NONS_Event::set(){
#if NONS_SYS_WINDOWS
	SetEvent(this->event);
#elif NONS_SYS_UNIX
	sem_post(&this->sem);
#elif NONS_SYS_PSP
	SDL_SemPost(this->sem);
#endif
}

void NONS_Event::reset(){
#if NONS_SYS_WINDOWS
	ResetEvent(this->event);
#endif
}

void NONS_Event::wait(){
#if NONS_SYS_WINDOWS
	WaitForSingleObject(this->event,INFINITE);
#elif NONS_SYS_UNIX
	sem_wait(&this->sem);
#elif NONS_SYS_PSP
	SDL_SemWait(this->sem);
#endif
}

class ctx{
protected:
	SDL_mutex *mutex;
	NONS_Event event;
public:
	bool debug;
	ulong t0;
	ctx(){
		this->mutex=SDL_CreateMutex();
		this->event.init();
	}
	virtual ~ctx(){
		SDL_DestroyMutex(this->mutex);
	}
	virtual void *lock()=0;
	virtual void unlock()=0;
	virtual void display(SDL_Surface *screen,SDL_Rect *rect)=0;
	virtual ulong get_width()=0;
	virtual ulong get_height()=0;
	virtual ulong get_pitch()=0;
	virtual const char *get_chroma()=0;
#ifdef VLC09X
	static void *_lock(ctx *a){
		return a->lock();
	}
#else
	static void _lock(ctx *a,void **pp_ret){
		*pp_ret=a->lock();
	}
#endif
	static void _unlock(ctx *a){
		a->unlock();
	}
};

class ctx_surface:public ctx{
	SDL_Surface *surf;
public:
	ctx_surface(ulong w,ulong h):ctx(){
		this->surf=SDL_CreateRGBSurface(SDL_HWSURFACE,w,h,32,0xFF0000,0xFF00,0xFF,0xFF000000);
		this->mutex=SDL_CreateMutex();
	}
	~ctx_surface(){
		SDL_FreeSurface(this->surf);
	}
	void *lock(){
		SDL_LockMutex(this->mutex);
		SDL_LockSurface(this->surf);
		return this->surf->pixels;
	}
	void unlock(){
		this->event.set();
		SDL_UnlockSurface(this->surf);
		SDL_UnlockMutex(this->mutex);
	}
	void display(SDL_Surface *screen,SDL_Rect *rect){
		this->event.wait();
		SDL_LockMutex(this->mutex);
		SDL_BlitSurface(this->surf,0,screen,rect);
		SDL_UnlockMutex(this->mutex);
		if (this->debug)
			blit_font(screen,seconds_to_time_format(double(SDL_GetTicks()-this->t0)/1000.0),rect->x,rect->y);
		SDL_UpdateRect(screen,0,0,0,0);
	}
	ulong get_width(){
		return this->surf->w;
	}
	ulong get_height(){
		return this->surf->h;
	}
	ulong get_pitch(){
		return this->surf->pitch;
	}
	const char *get_chroma(){
		return "RV32";
	}
};

#if 0
class ctx_overlay:public ctx{
	SDL_Overlay *surf;
public:
	ctx_overlay(SDL_Surface *screen,ulong w,ulong h):ctx(){
		this->surf=SDL_CreateYUVOverlay(w,h,SDL_UYVY_OVERLAY,screen);
	}
	~ctx_overlay(){
		SDL_FreeYUVOverlay(this->surf);
	}
	void *lock(){
		SDL_LockMutex(this->mutex);
		SDL_LockYUVOverlay(this->surf);
		return this->surf->pixels;
	}
	void unlock(){
		this->event.set();
		SDL_UnlockYUVOverlay(this->surf);
		SDL_UnlockMutex(this->mutex);
	}
	void display(SDL_Surface *screen,SDL_Rect *rect){
		this->event.wait();
		SDL_LockMutex(this->mutex);
		SDL_DisplayYUVOverlay(this->surf,rect);
		SDL_UnlockMutex(this->mutex);
		SDL_UpdateRect(screen,0,0,0,0);
	}
	ulong get_width(){
		return this->surf->w;
	}
	ulong get_height(){
		return this->surf->h;
	}
	ulong get_pitch(){
		ulong ret=0;
		for (ulong a=0;a<this->surf->planes;a++)
			ret+=this->surf->pitches[a];
		return ret;
	}
	const char *get_chroma(){
		return "UYVY";
	}
};
#endif

bool catch_f(std::string &str,libvlc_exception_t *ex){
	if (libvlc_exception_raised(ex)){
		str=libvlc_exception_get_message(ex);
		libvlc_exception_clear(ex);
		return 0;
	}
	libvlc_exception_clear(ex);
	return 1;
}
#define CATCH if (!catch_f(exception_string,&ex)) return 0

ulong options=SDL_ANYFORMAT|SDL_HWSURFACE|SDL_DOUBLEBUF;

int libvlc_video_get_size(libvlc_media_player_t *,unsigned,unsigned *,unsigned *);

bool get_true_dimensions(ulong &w,ulong &h,const char *filename,std::string &exception_string){
	char clock[64],
		cunlock[64],
		cdata[64],
		width[32],
		height[32],
		pitch[32];

	ctx_surface ctx(50,50);
	std::vector<const char *> vlc_argv;
	vlc_argv.push_back(".");
	vlc_argv.push_back("-q");
	vlc_argv.push_back("--plugin-path");
	vlc_argv.push_back("./modules");
	vlc_argv.push_back("--ignore-config");
	vlc_argv.push_back("--noaudio");
	vlc_argv.push_back("--vout");
	vlc_argv.push_back("vmem");
	vlc_argv.push_back("--vmem-width" );
	vlc_argv.push_back(width);
	vlc_argv.push_back("--vmem-height");
	vlc_argv.push_back(height);
	vlc_argv.push_back("--vmem-pitch" );
	vlc_argv.push_back(pitch);
	vlc_argv.push_back("--vmem-chroma");
	vlc_argv.push_back(ctx.get_chroma());
	vlc_argv.push_back("--vmem-lock"  );
	vlc_argv.push_back(clock);
	vlc_argv.push_back("--vmem-unlock");
	vlc_argv.push_back(cunlock);
	vlc_argv.push_back("--vmem-data"  );
	vlc_argv.push_back(cdata);
	vlc_argv.push_back("--no-osd");
	vlc_argv.push_back(filename);

	sprintf(clock, "%lld", (long long int)(intptr_t)::ctx::_lock);
	sprintf(cunlock, "%lld", (long long int)(intptr_t)::ctx::_unlock);
	sprintf(cdata, "%lld", (long long int)(intptr_t)&ctx);
	sprintf(width, "%i", ctx.get_width());
	sprintf(height, "%i", ctx.get_height());
	sprintf(pitch, "%i", ctx.get_pitch());

	libvlc_exception_t ex;
	libvlc_exception_init(&ex);
	libvlc_instance_t *libvlc=libvlc_new(vlc_argv.size(),&vlc_argv[0],&ex);
	CATCH;
	libvlc_media_t *m=libvlc_media_new(libvlc,vlc_argv.back(),&ex);
	CATCH;
	libvlc_media_player_t *mp=libvlc_media_player_new_from_media(m,&ex);
	CATCH;
	libvlc_media_release(m);


	libvlc_media_player_play(mp, &ex);
	CATCH;

	w=h=0;
	while (!w){
		w=libvlc_video_get_width(mp,&ex);
		catch_f(exception_string,&ex);
	}
	while (!h){
		h=libvlc_video_get_height(mp,&ex);
		catch_f(exception_string,&ex);
	}
	libvlc_media_player_stop(mp, &ex);
	CATCH;
	libvlc_media_player_release(mp);
	libvlc_release(libvlc);
	return 1;
}

SDL_Rect fix_rect(SDL_Surface *screen,ulong w,ulong h){
	SDL_Rect r;
	float screenRatio=float(screen->w)/float(screen->h),
		videoRatio=float(w)/float(h);
	ulong w2,
		h2;
	if (screenRatio<videoRatio){ //widescreen
		w2=screen->w;
		h2=ulong(float(screen->w)/videoRatio);
	}else if (screenRatio>videoRatio){ //"narrowscreen"
		w2=ulong(float(screen->h)*videoRatio);
		h2=screen->h;
	}else{
		w2=screen->w;
		h2=screen->h;
	}
	r.x=Sint16((screen->w-w2)/2);
	r.y=Sint16((screen->h-h2)/2);
	r.w=(Uint16)w2;
	r.h=(Uint16)h2;
	return r;
}

#include "../C_play_video.cpp"

play_video_SIGNATURE{
	char clock[64],
		cunlock[64],
		cdata[64],
		width[32],
		height[32],
		pitch[32];

	ulong w,h;
	if (!get_true_dimensions(w,h,input,exception_string)){
		exception_string="Could not get video resolution: "+exception_string;
		return 0;
	}
	SDL_Rect rect=fix_rect(screen,w,h);

	ctx_surface ctx(rect.w,rect.h);

	std::vector<const char *> vlc_argv;
	vlc_argv.push_back(".");
	vlc_argv.push_back("-q");
	vlc_argv.push_back("--plugin-path");
	vlc_argv.push_back("./modules");
	vlc_argv.push_back("--ignore-config");
	//vlc_argv.push_back("--noaudio");
	vlc_argv.push_back("--vout");
	vlc_argv.push_back("vmem");
	vlc_argv.push_back("--vmem-width" );
	vlc_argv.push_back(width);
	vlc_argv.push_back("--vmem-height");
	vlc_argv.push_back(height);
	vlc_argv.push_back("--vmem-pitch" );
	vlc_argv.push_back(pitch);
	vlc_argv.push_back("--vmem-chroma");
	vlc_argv.push_back(ctx.get_chroma());
	vlc_argv.push_back("--vmem-lock"  );
	vlc_argv.push_back(clock);
	vlc_argv.push_back("--vmem-unlock");
	vlc_argv.push_back(cunlock);
	vlc_argv.push_back("--vmem-data"  );
	vlc_argv.push_back(cdata);
	vlc_argv.push_back("--no-osd");
	vlc_argv.push_back(input);

	sprintf(clock, "%lld", (long long int)(intptr_t)::ctx::_lock);
	sprintf(cunlock, "%lld", (long long int)(intptr_t)::ctx::_unlock);
	sprintf(cdata, "%lld", (long long int)(intptr_t)&ctx);
	sprintf(width, "%i", ctx.get_width());
	sprintf(height, "%i", ctx.get_height());
	sprintf(pitch, "%i", ctx.get_pitch());

	libvlc_exception_t ex;
	libvlc_exception_init(&ex);
	//reset stream before trying to open again
	fp.seek(fp.data,0,1);
	libvlc_instance_t *libvlc=libvlc_new(vlc_argv.size(),&vlc_argv[0],&ex);
	CATCH;
	libvlc_media_t *m=libvlc_media_new(libvlc,vlc_argv.back(),&ex);
	CATCH;
	libvlc_media_player_t *mp=libvlc_media_player_new_from_media(m,&ex);
	CATCH;
	libvlc_media_release(m);


	libvlc_media_player_play(mp, &ex);
	CATCH;
	ctx.t0=SDL_GetTicks();
	ctx.debug=print_debug;

	for (ulong i=0;!*stop;i++){
		bool a=libvlc_media_player_get_state(mp,&ex)==libvlc_Ended;
		CATCH;
		if (a)
			break;
		for (ulong a=0;a<callback_pairs.size();a++){
			if (*callback_pairs[a].trigger){
				*callback_pairs[a].trigger=0;
				screen=callback_pairs[a].callback(screen,user_data);
			}
		}

		ctx.display(screen,&rect);
	}

	libvlc_media_player_stop(mp, &ex);
	CATCH;
	libvlc_media_player_release(mp);
	libvlc_release(libvlc);
	return 1;
}

PLAYER_TYPE_FUNCTION_SIGNATURE{
	return "libVLC";
}

PLAYER_VERSION_FUNCTION_SIGNATURE{
	return C_PLAY_VIDEO_PARAMS_VERSION;
}
