/*
* Copyright (c) 2009, 2010, Helios (helios.vmg@gmail.com)
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

#include "../../video_player.h"
#include "../common.h"
#include <SDL/SDL_gfxPrimitives.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <set>
#include <sstream>
#include <al.h>
#include <alc.h>

#include <queue>
#include <climits>

#if NONS_SYS_WINDOWS
#include <windows.h>
#elif NONS_SYS_UNIX
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#endif

typedef unsigned long ulong;

#if NONS_SYS_WINDOWS
#include <cassert>

class VirtualConsole{
	HANDLE near_end,
		far_end,
		process;
public:
	bool good;
	VirtualConsole(const std::string &name,ulong color);
	~VirtualConsole();
	void put(const char *str,size_t size=0);
	void put(const std::string &str){
		this->put(str.c_str(),str.size());
	}
};

VirtualConsole::VirtualConsole(const std::string &name,ulong color){
	this->good=0;
	SECURITY_ATTRIBUTES sa;
	sa.nLength=sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle=1;
	sa.lpSecurityDescriptor=0;
	if (!CreatePipe(&this->far_end,&this->near_end,&sa,0)){
		assert(this->near_end==INVALID_HANDLE_VALUE);
		return;
	}
	SetHandleInformation(this->near_end,HANDLE_FLAG_INHERIT,0);
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	ZeroMemory(&pi,sizeof(pi));
	ZeroMemory(&si,sizeof(si));
	si.cb=sizeof(STARTUPINFO);
	si.hStdInput=this->far_end;
	si.dwFlags|=STARTF_USESTDHANDLES;
	TCHAR program[]=TEXT("console.exe");
	TCHAR arguments[100];
#ifndef UNICODE
	sprintf(arguments,"%d",color);
#else
	swprintf(arguments,L"0 %d",color);
#endif
	if (!CreateProcess(program,arguments,0,0,1,CREATE_NEW_CONSOLE|CREATE_UNICODE_ENVIRONMENT,0,0,&si,&pi))
		return;
	this->process=pi.hProcess;
	CloseHandle(pi.hThread);
	this->good=1;

	this->put(name);
	this->put("\n",1);
}

VirtualConsole::~VirtualConsole(){
	if (this->near_end!=INVALID_HANDLE_VALUE){
		if (this->process!=INVALID_HANDLE_VALUE){
			TerminateProcess(this->process,0);
			CloseHandle(this->process);
		}
		CloseHandle(this->near_end);
		CloseHandle(this->far_end);
	}
}
	
void VirtualConsole::put(const char *str,size_t size){
	if (!this->good)
		return;
	if (!size)
		size=strlen(str);
	DWORD l;
	WriteFile(this->near_end,str,size,&l,0);
}

static VirtualConsole *vc[10]={0};
#define VC_AUDIO_DECODE 0
#define VC_VIDEO_DECODE 1
#define VC_AUDIO_RENDER 2
#define VC_VIDEO_RENDER 3
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

typedef void (*NONS_ThreadedFunctionPointer)(void *);
class NONS_Thread{
	struct threadStruct{ NONS_ThreadedFunctionPointer f; void *d; };
#if NONS_SYS_WINDOWS
	HANDLE thread;
	static DWORD __stdcall runningThread(void *);
#elif NONS_SYS_UNIX
	pthread_t thread;
	static void *runningThread(void *);
#endif
	bool called;
public:
	NONS_Thread():called(0){}
	NONS_Thread(NONS_ThreadedFunctionPointer function,void *data,bool give_highest_priority=0);
	~NONS_Thread();
	void call(NONS_ThreadedFunctionPointer function,void *data,bool give_highest_priority=0);
	void join();
};

class NONS_Mutex{
#if NONS_SYS_WINDOWS
	void *mutex;
#elif NONS_SYS_UNIX
	pthread_mutex_t mutex;
#endif
	NONS_Mutex(const NONS_Mutex &){}
	const NONS_Mutex &operator=(const NONS_Mutex &){ return *this; }
public:
	NONS_Mutex();
	~NONS_Mutex();
	void lock();
	void unlock();
	bool try_lock();
#ifdef NONS_Mutex_USE_MUTEX
	bool timed_lock(ulong ms);
#endif
};

class NONS_MutexLocker{
	NONS_Mutex &mutex;
	NONS_MutexLocker(const NONS_MutexLocker &m):mutex(m.mutex){}
	void operator=(const NONS_MutexLocker &){}
public:
	NONS_MutexLocker(NONS_Mutex &m):mutex(m){
		this->mutex.lock();
	}
	~NONS_MutexLocker(){
		this->mutex.unlock();
	}
};

template <typename T>
class TSqueue{
	std::queue<T> queue;
	NONS_Mutex mutex;
public:
	ulong max_size;
	TSqueue(){
		this->max_size=ULONG_MAX;
	}
	TSqueue(const TSqueue &b){
		NONS_MutexLocker ml(b.mutex);
		this->queue=b.queue;
	}
	const TSqueue &operator=(const TSqueue &b){
		NONS_MutexLocker ml[]={
			b.mutex,
			this->mutex
		};
		this->queue=b.queue;
	}
	void lock(){
		this->mutex.lock();
	}
	bool try_lock(){
		return this->mutex.try_lock();
	}
	void unlock(){
		this->mutex.unlock();
	}
	void push(const T &e){
		while (1){
			this->mutex.lock();
			ulong size=this->queue.size();
			if (size<this->max_size)
				break;
			this->mutex.unlock();
			SDL_Delay(10);
		}
		this->queue.push(e);
		this->mutex.unlock();
	}
	bool is_empty(){
		NONS_MutexLocker ml(this->mutex);
		return this->queue.empty();
	}
	ulong size(){
		NONS_MutexLocker ml(this->mutex);
		return this->queue.size();
	}
	T &peek(){
		NONS_MutexLocker ml(this->mutex);
		return this->queue.front();
	}
	T pop(){
		NONS_MutexLocker ml(this->mutex);
		T ret=this->queue.front();
		this->queue.pop();
		return ret;
	}
	void pop_without_copy(){
		NONS_MutexLocker ml(this->mutex);
		this->queue.pop();
	}
};

void NONS_Event::init(){
#if NONS_SYS_WINDOWS
	this->event=CreateEvent(0,0,0,0);
#elif NONS_SYS_UNIX
	sem_init(&this->sem,0,0);
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
#endif
}

void NONS_Event::set(){
#if NONS_SYS_WINDOWS
	SetEvent(this->event);
#elif NONS_SYS_UNIX
	sem_post(&this->sem);
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
#endif
}

NONS_Thread::NONS_Thread(NONS_ThreadedFunctionPointer function,void *data,bool give_highest_priority){
	this->called=0;
	this->call(function,data,give_highest_priority);
}

NONS_Thread::~NONS_Thread(){
	if (!this->called)
		return;
	this->join();
#if NONS_SYS_WINDOWS
	CloseHandle(this->thread);
#endif
}

void NONS_Thread::call(NONS_ThreadedFunctionPointer function,void *data,bool give_highest_priority){
	if (this->called)
		return;
	threadStruct *ts=new threadStruct;
	ts->f=function;
	ts->d=data;
#if NONS_SYS_WINDOWS
	this->thread=CreateThread(0,0,(LPTHREAD_START_ROUTINE)runningThread,ts,0,0);
	if (give_highest_priority)
		SetThreadPriority(this->thread,THREAD_PRIORITY_HIGHEST);
#elif NONS_SYS_UNIX
	pthread_attr_t attr,
		*pattr=0;
	if (give_highest_priority){
		pattr=&attr;
		pthread_attr_init(pattr);
		sched_param params;
		pthread_attr_getschedparam(pattr,&params);
		int policy;
		pthread_attr_getschedpolicy(pattr,&policy);
		params.sched_priority=sched_get_priority_max(policy);
		pthread_attr_setschedparam(pattr,&params);
	}
	pthread_create(&this->thread,pattr,runningThread,ts);
	if (give_highest_priority)
		pthread_attr_destroy(pattr);
#endif
	this->called=1;
}

void NONS_Thread::join(){
	if (!this->called)
		return;
#if NONS_SYS_WINDOWS
	WaitForSingleObject(this->thread,INFINITE);
#elif NONS_SYS_UNIX
	pthread_join(this->thread,0);
#endif
	this->called=0;
}

#if NONS_SYS_WINDOWS
DWORD WINAPI
#elif NONS_SYS_UNIX
void *
#endif
NONS_Thread::runningThread(void *p){
	NONS_ThreadedFunctionPointer f=((threadStruct *)p)->f;
	void *d=((threadStruct *)p)->d;
	delete (threadStruct *)p;
	f(d);
	return 0;
}

NONS_Mutex::NONS_Mutex(){
#if NONS_SYS_WINDOWS
	this->mutex=new CRITICAL_SECTION;
	InitializeCriticalSection((CRITICAL_SECTION *)this->mutex);
#elif NONS_SYS_UNIX
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&this->mutex,&attr);
	pthread_mutexattr_destroy(&attr);
#endif
}

NONS_Mutex::~NONS_Mutex(){
#if NONS_SYS_WINDOWS
	DeleteCriticalSection((CRITICAL_SECTION *)this->mutex);
	delete (CRITICAL_SECTION *)this->mutex;
#elif NONS_SYS_UNIX
	pthread_mutex_destroy(&this->mutex);
#endif
}

void NONS_Mutex::lock(){
#if NONS_SYS_WINDOWS
	EnterCriticalSection((CRITICAL_SECTION *)this->mutex);
#elif NONS_SYS_UNIX
	pthread_mutex_lock(&this->mutex);
#endif
}

void NONS_Mutex::unlock(){
#if NONS_SYS_WINDOWS
	LeaveCriticalSection((CRITICAL_SECTION *)this->mutex);
#elif NONS_SYS_UNIX
	pthread_mutex_unlock(&this->mutex);
#endif
}

template <typename T,typename T2>
std::basic_string<T> itoa(T2 n,unsigned w=0){
	std::basic_stringstream<T> stream;
	if (w){
		stream.fill('0');
		stream.width(w);
	}
	stream <<n;
	return stream.str();
}

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavcodec/imgconvert.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#define AUDIO_QUEUE_MAX_SIZE 5
#define FRAME_QUEUE_MAX_SIZE 5
#define AUDIO_QUEUE_REFILL_WAIT 25
#define FRAME_QUEUE_REFILL_WAIT 25

#ifdef AV_NOPTS_VALUE
#undef AV_NOPTS_VALUE
#endif
static const int64_t AV_NOPTS_VALUE=0x8000000000000000LL;

typedef unsigned long ulong;
typedef unsigned char uchar;

struct Packet{
	uint8_t *data;
	size_t size;
    int64_t dts,
		pts;
	Packet(AVPacket *packet){
		if (packet){
			this->size=packet->size;
			/*
			Allocate twice as many bytes as needed. Not the best way to prevent
			a buffer overflow, I know.
			*/
			this->data=(uint8_t *)av_malloc(this->size*2);
			memcpy(this->data,packet->data,this->size);
			memset(this->data+this->size,0,this->size);
			this->dts=packet->dts;
			this->pts=packet->pts;
		}else{
			this->data=0;
			this->size=0;
		}
	}
	~Packet(){
		if (this->data)
			av_free(this->data);
	}
	double compute_time(AVStream *stream,AVFrame *frame){
		double ret;
		if ((this->dts!=AV_NOPTS_VALUE))
			ret=(double)this->dts;
		else if (frame->opaque && *(int64_t *)frame->opaque!=AV_NOPTS_VALUE)
			ret=(double)*(int64_t *)frame->opaque;
		else
			ret=0;
		return ret*av_q2d(stream->time_base);
	}
	double compute_time(AVStream *stream){
		double ret;
		if ((this->dts!=AV_NOPTS_VALUE))
			ret=(double)this->dts;
		else
			ret=0;
		return ret*av_q2d(stream->time_base);
	}
private:
	Packet(const Packet &){}
	Packet &operator=(const Packet &){ return *this; }
};

struct audioBuffer{
	int16_t *buffer;
	size_t size,
		start_reading;
	bool quit,
		for_copy;
	ulong time_offset;
	audioBuffer(const int16_t *src,size_t size,bool for_copy,double t){
		this->quit=0;
		this->size=size;
		this->start_reading=0;
		//size*=sizeof(int16_t);
		this->buffer=new int16_t[size];
		memcpy(this->buffer,src,size*sizeof(int16_t));
		this->for_copy=for_copy;
		this->time_offset=ulong(t*1000.0);
	}
	audioBuffer(bool q){
		this->quit=q;
		this->buffer=0;
	}
	audioBuffer(const audioBuffer &b){
		this->buffer=b.buffer;
		this->size=b.size;
		this->start_reading=b.start_reading;
		this->quit=b.quit;
		this->for_copy=0;
		this->time_offset=b.time_offset;
	}
	~audioBuffer(){
		if (this->buffer && !this->for_copy)
			delete[] this->buffer;
	}
	//1 if this buffer is done
	bool read_into_buffer(int16_t *dst,size_t &size){
		if (size>this->size-this->start_reading)
			size=this->size-this->start_reading;
		memcpy(dst,this->buffer+this->start_reading,size*sizeof(int16_t));
		this->start_reading+=size;
		if (this->start_reading>=this->size){
			//av_free(this->buffer);
			delete[] this->buffer;
			this->buffer=0;
			return 1;
		}
		return 0;
	}
};

static ulong global_time=0,
	start_time=0,
	real_global_time=0,
	real_start_time=0;

class AudioOutput{
	ALCdevice *device;
	ALCcontext *context;
	ALuint source;
	NONS_Thread thread;
	ulong frequency;
	ALenum format;
	static const size_t n=2;
	ALuint ALbuffers[n];
	ulong current;
	int16_t *buffers[n];
	size_t buffers_size;
	TSqueue<audioBuffer> *incoming_queue;
	volatile bool stop_thread;
public:
	bool good,
		expect_buffers;
	AudioOutput(ulong channels,ulong frequency){
		this->stop_thread=0;
		this->buffers_size=frequency/10*channels;
		for (ulong a=0;a<n;a++){
			this->buffers[a]=new int16_t[this->buffers_size];
			memset(this->buffers[a],0,this->buffers_size*sizeof(int16_t));
		}
		this->device=0;
		this->context=0;
		this->good=0;
		this->device=alcOpenDevice(0);
		if (!this->device)
			return;
		this->context=alcCreateContext(this->device,0);
		if (!this->context)
			return;
		alcMakeContextCurrent(this->context);
		this->frequency=frequency;
		switch (channels){
			case 1:
				this->format=AL_FORMAT_MONO16;
				break;
			case 2:
				this->format=AL_FORMAT_STEREO16;
				break;
			case 4:
				this->format=alGetEnumValue("AL_FORMAT_QUAD16");
				break;
			case 6:
				this->format=alGetEnumValue("AL_FORMAT_51CHN16");
				break;
		}
		alGenSources(1,&this->source);
		this->good=1;
		alGenBuffers(n,this->ALbuffers);
		this->current=0;
	}
	~AudioOutput(){
		this->stop_thread=1;
		this->thread.join();
		alcMakeContextCurrent(0);
		for (ulong a=0;a<n;a++)
			delete[] this->buffers[a];
		if (this->context){
			alDeleteSources(1,&this->source);
			alDeleteBuffers(n,this->ALbuffers);
			alcDestroyContext(this->context);
		}
		if (this->device)
			alcCloseDevice(this->device);
	}
	TSqueue<audioBuffer> *startThread(){
		this->incoming_queue=new TSqueue<audioBuffer>;
		this->incoming_queue->max_size=30;
		this->thread.call(running_thread_support,this,1);
		return this->incoming_queue;
	}
	void stopThread(bool join){
		this->stop_thread=1;
		if (join)
			this->thread.join();
	}
private:
	static void running_thread_support(void *p){
		((AudioOutput *)p)->running_thread();
	}
	void running_thread(){
		if (this->expect_buffers)
			while (this->incoming_queue->is_empty());
		for (ulong a=0;a<this->n;a++){
			this->current=a;
			ulong t;
			fill(this->buffers[this->current],this->buffers_size,this->incoming_queue,t);
			alBufferData(
				this->ALbuffers[this->current],
				this->format,
				this->buffers[this->current],
				this->buffers_size*sizeof(int16_t),
				this->frequency
			);
			alSourceQueueBuffers(this->source,1,this->ALbuffers+this->current);
		}
		this->current=0;

		real_start_time=start_time=SDL_GetTicks();
		this->play();
		while (!this->stop_thread){
			SDL_Delay(10);
			ulong now=SDL_GetTicks();
			global_time=now-start_time;
			real_global_time=now-real_start_time;
			ALint buffers_finished=0,
				queued;
			alGetSourcei(this->source,AL_BUFFERS_PROCESSED,&buffers_finished);
			alGetSourcei(this->source,AL_BUFFERS_QUEUED,&queued);
			bool call_play=0;
			ulong count=0;
			if (buffers_finished==this->n){
				call_play=1;
#if NONS_SYS_WINDOWS
				vc[VC_AUDIO_RENDER]->put("Critical point!\n");
#endif
			}
			if (!call_play){
				ALint state;
				alGetSourcei(this->source,AL_SOURCE_STATE,&state);
				if (state!=AL_PLAYING){
#if NONS_SYS_WINDOWS
					vc[VC_AUDIO_RENDER]->put("The source has stopped playing!\n");
#endif
					call_play=1;
					buffers_finished=this->n;
				}
			}
			for (;buffers_finished;buffers_finished--,count++){
				ALuint temp;
				alSourceUnqueueBuffers(this->source,1,&temp);
				bool loop=1;
				ulong t;
				fill(this->buffers[this->current],this->buffers_size,this->incoming_queue,t);
				//correct time
				if (t+100*(buffers_finished)!=global_time)
					start_time=now-t+100*(buffers_finished);
				alBufferData(
					this->ALbuffers[this->current],
					this->format,
					this->buffers[this->current],
					this->buffers_size*sizeof(int16_t),
					this->frequency
				);
				alSourceQueueBuffers(this->source,1,this->ALbuffers+this->current);
				this->current=(this->current+1)%n;
			}
			if (call_play){
#if NONS_SYS_WINDOWS
				vc[VC_AUDIO_RENDER]->put("count: "+itoa<char>(count)+"\n");
#endif
				this->play();
			}
		}
		this->wait_until_stop();
		delete this->incoming_queue;
		this->incoming_queue=0;
	}
	static bool fill(int16_t *buffer,size_t size,TSqueue<audioBuffer> *queue,ulong &time){
		bool ret=1,
			time_unset=1;
		queue->lock();
		if (queue->is_empty()){
			memset(buffer,0,size*sizeof(int16_t));
			ret=0;
		}else{
			audioBuffer *buffer2=&queue->peek();
			if (buffer2->quit){
				memset(buffer,0,size*sizeof(int16_t));
			}else{
				size_t write_at=0,
					read_size;
				while (write_at<size){
					read_size=size-write_at;
					if (buffer2->read_into_buffer(buffer+write_at,read_size)){
						if (time_unset){
							time=buffer2->time_offset;
							time_unset=0;
						}
						queue->pop();
						if (queue->is_empty())
							break;
						buffer2=&queue->peek();
					}
					write_at+=read_size;
				}
				if (write_at<size){
					memset(buffer+write_at,0,(size-write_at)*sizeof(int16_t));
					ret=0;
				}
			}
		}
		queue->unlock();
		return ret;
	}
public:
	void play(){
		ALint state;
		alGetSourcei(this->source,AL_SOURCE_STATE,&state);
		if (state!=AL_PLAYING)
			alSourcePlay(this->source);
	}
	void wait_until_stop(){
		ALint state;
		do
			alGetSourcei(this->source,AL_SOURCE_STATE,&state);
		while (state==AL_PLAYING);
	}
};

//#define USE_OVERLAY
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
static bool debug_messages=0;

class CompleteVideoFrame{
	SDL_Overlay *overlay;
	SDL_Surface *old_screen;
	SDL_Rect frameRect;
	CompleteVideoFrame(const CompleteVideoFrame &){}
	const CompleteVideoFrame &operator=(const CompleteVideoFrame &){ return *this; }
public:
	uint64_t repeat;
	double pts;
	CompleteVideoFrame(volatile SDL_Surface *screen,AVStream *videoStream,AVFrame *videoFrame,double pts){
		this->overlay=0;
		ulong width,height;
		float screenRatio=float(screen->w)/float(screen->h),
			videoRatio;
		if (videoStream->sample_aspect_ratio.num)
			videoRatio=
				float(videoStream->codec->width*videoStream->sample_aspect_ratio.num)/
				float(videoStream->codec->height*videoStream->sample_aspect_ratio.den);
		else if (videoStream->codec->sample_aspect_ratio.num)
			videoRatio=
				float(videoStream->codec->width*videoStream->codec->sample_aspect_ratio.num)/
				float(videoStream->codec->height*videoStream->codec->sample_aspect_ratio.den);
		else
			videoRatio=float(videoStream->codec->width)/float(videoStream->codec->height);
		if (screenRatio<videoRatio){ //widescreen
			width=screen->w;
			height=ulong(float(screen->w)/videoRatio);
		}else if (screenRatio>videoRatio){ //"narrowscreen"
			width=ulong(float(screen->h)*videoRatio);
			height=screen->h;
		}else{
			width=screen->w;
			height=screen->h;
		}
		this->overlay=SDL_CreateYUVOverlay(width,height,SDL_YV12_OVERLAY,(SDL_Surface *)screen);
		this->frameRect.x=Sint16((screen->w-width)/2);
		this->frameRect.y=Sint16((screen->h-height)/2);
		this->frameRect.w=(Uint16)width;
		this->frameRect.h=(Uint16)height;
		SDL_LockYUVOverlay(this->overlay);
		AVPicture pict;
		pict.data[0]=this->overlay->pixels[0];
		pict.data[1]=this->overlay->pixels[2];
		pict.data[2]=this->overlay->pixels[1];
		pict.linesize[0]=this->overlay->pitches[0];
		pict.linesize[1]=this->overlay->pitches[2];
		pict.linesize[2]=this->overlay->pitches[1];
		SwsContext *sc=sws_getContext(
			videoStream->codec->width,videoStream->codec->height,videoStream->codec->pix_fmt,
			this->overlay->w,this->overlay->h,PIX_FMT_YUV420P,
			SWS_FAST_BILINEAR,
			0,0,0
		);
		sws_scale(sc,videoFrame->data,videoFrame->linesize,0,videoStream->codec->height,pict.data,pict.linesize);
		sws_freeContext(sc);
		SDL_UnlockYUVOverlay(this->overlay);
		this->old_screen=(SDL_Surface *)screen;
		this->pts=pts;
		this->repeat=videoFrame->repeat_pict;
	}
	~CompleteVideoFrame(){
		if (!!this->overlay)
			SDL_FreeYUVOverlay(this->overlay);
	}
	void blit(volatile SDL_Surface *currentScreen){
		if (this->old_screen==currentScreen){
			if (debug_messages){
				std::string s=seconds_to_time_format(this->pts);
				//s.push_back('\n');
				//s.append(seconds_to_time_format(double(real_global_time)/1000.0));
				//s.append(" "+itoa<char>(real_global_time-this->pts*1000.0,4));
				blit_font(this->overlay,s.c_str(),0,0);
			}
			SDL_DisplayYUVOverlay(this->overlay,&this->frameRect);
		}
	}
	const SDL_Rect &frame(){ return this->frameRect; }
};

static volatile bool stop_playback=0;
static volatile SDL_Surface *global_screen=0;

struct video_display_thread_params{
	TSqueue<CompleteVideoFrame *> *queue;
	AudioOutput *aoutput;
	void *user_data;
	std::vector<C_play_video_params::trigger_callback_pair> *callback_pairs;
};

void video_display_thread(void *p){
	video_display_thread_params params=*(video_display_thread_params *)p;
	delete (video_display_thread_params *)p;

	SDL_Color white={0xFF,0xFF,0xFF,0xFF},
		black={0,0,0,0xFF};
	while (!stop_playback){
		SDL_Delay(10);
		double current_time=double(global_time)/1000.0;
		if (params.queue->is_empty())
			continue;
		for (ulong a=0;a<params.callback_pairs->size();a++){
			if (*(*params.callback_pairs)[a].trigger){
				*(*params.callback_pairs)[a].trigger=0;
				global_screen=(*params.callback_pairs)[a].callback(global_screen,params.user_data);
			}
		}

		CompleteVideoFrame *frame=(CompleteVideoFrame *)params.queue->peek();

		/*
		if (debug_messages){
			std::stringstream stream;
			stream <<frame->pts<<'\t'<<current_time<<" ("<<SDL_GetTicks()<<'-'<<start_time<<'='<<global_time<<')'<<std::endl;
			threadsafe_print(stream.str());
		}
		*/

		while (frame->pts<=current_time){
			frame->blit(global_screen);
			delete params.queue->pop();
			if (params.queue->is_empty())
				break;
			frame=(CompleteVideoFrame *)params.queue->peek();
		}
	}
	while (!params.queue->is_empty()){
		CompleteVideoFrame *frame=params.queue->pop();
		if (frame)
			delete frame;
	}
}

uint64_t global_pts=AV_NOPTS_VALUE;

namespace this_player{
	int get_buffer(struct AVCodecContext *c,AVFrame *pic){
		int ret=avcodec_default_get_buffer(c,pic);
		int64_t *pts=new int64_t;
		*pts=global_pts;
		pic->opaque=pts;
		return ret;
	}
	void release_buffer(struct AVCodecContext *c, AVFrame *pic) {
	  if (pic)
		  delete (int64_t *)pic->opaque;
	  avcodec_default_release_buffer(c, pic);
	}
}

struct decode_audio_params{
	AVCodecContext *audioCC;
	AVStream *audioS;
	TSqueue<Packet *> *packet_queue;
	AudioOutput *output;
};

void decode_audio(void *p){
	decode_audio_params params=*(decode_audio_params *)p;
	delete (decode_audio_params *)p;

	const size_t output_s=AVCODEC_MAX_AUDIO_FRAME_SIZE*2;
	static int16_t audioOutputBuffer[output_s];

	TSqueue<audioBuffer> *queue=params.output->startThread();

	params.audioCC->get_buffer=this_player::get_buffer;
	params.audioCC->release_buffer=this_player::release_buffer;
	while (1){
		Packet *packet=0;
#if NONS_SYS_WINDOWS
		vc[VC_AUDIO_DECODE]->put("["+itoa<char>(real_global_time,8)+"] decode_audio(): Waiting for packet.\n");
#endif
		while (params.packet_queue->is_empty() && !stop_playback){
			SDL_Delay(10);
		}
		if (stop_playback)
			break;
#if NONS_SYS_WINDOWS
		vc[VC_AUDIO_DECODE]->put("["+itoa<char>(real_global_time,8)+"] decode_audio(): Popping queue.\n");
#endif
		packet=params.packet_queue->pop();

		uint8_t *packet_data=packet->data;
		size_t packet_data_s=packet->size,
			write_at=0,
			buffer_size=0;
#if NONS_SYS_WINDOWS
		vc[VC_AUDIO_DECODE]->put("["+itoa<char>(real_global_time,8)+"] decode_audio(): Decoding packet.\n");
#endif
		while (packet_data_s){
			int bytes_decoded=output_s,
				bytes_extracted;
			bytes_extracted=avcodec_decode_audio2(params.audioCC,audioOutputBuffer+write_at,&bytes_decoded,packet_data,packet_data_s);
			if (bytes_extracted<0)
				break;
			packet_data+=bytes_extracted;
			if (packet_data_s>=(size_t)bytes_extracted)
				packet_data_s-=bytes_extracted;
			else
				packet_data_s=0;
			buffer_size+=bytes_decoded/sizeof(int16_t);
		}
#if NONS_SYS_WINDOWS
		vc[VC_AUDIO_DECODE]->put("["+itoa<char>(real_global_time,8)+"] decode_audio(): Pushing frame.\n");
#endif
		queue->push(audioBuffer(audioOutputBuffer,buffer_size,1,packet->compute_time(params.audioS)));
		delete packet;
	}
	params.output->stopThread(0);

	while (!params.packet_queue->is_empty()){
		Packet *packet=params.packet_queue->pop();
		if (packet)
			delete packet;
	}
}

struct decode_video_params{
	AVCodecContext *videoCC;
	AVStream *videoS;
	TSqueue<Packet *> *packet_queue;
	AudioOutput *aoutput;
	void *user_data;
	std::vector<C_play_video_params::trigger_callback_pair> *callback_pairs;
};

struct cmp_pCompleteVideoFrame{
	bool operator()(CompleteVideoFrame * const &A,CompleteVideoFrame * const &B){
		return A->pts>B->pts;
	}
};

void decode_video(void *p){
	decode_video_params params=*(decode_video_params *)p;
	delete (decode_video_params *)p;

	AVFrame *videoFrame=avcodec_alloc_frame();
	std::set<CompleteVideoFrame *,cmp_pCompleteVideoFrame> preQueue;
	TSqueue<CompleteVideoFrame *> frameQueue;
	frameQueue.max_size=FRAME_QUEUE_MAX_SIZE;
	NONS_Thread display;
	{
		video_display_thread_params *temp=new video_display_thread_params;
		temp->queue=&frameQueue;
		temp->aoutput=params.aoutput;
		temp->user_data=params.user_data;
		temp->callback_pairs=params.callback_pairs;
		display.call(video_display_thread,temp);
	}

	params.videoCC->get_buffer=this_player::get_buffer;
	params.videoCC->release_buffer=this_player::release_buffer;
	double max_pts=-9999;
	bool msg=0;
	while (1){
		Packet *packet;
		if (stop_playback)
			break;
		if (params.packet_queue->is_empty()){
			if (!msg){
#if NONS_SYS_WINDOWS
				vc[VC_VIDEO_DECODE]->put("["+itoa<char>(real_global_time,8)+"] decode_video(): Queue is empty. Nothing to do.\n");
#endif
				msg=1;
			}
			SDL_Delay(10);
			continue;
		}
		msg=0;
#if NONS_SYS_WINDOWS
		vc[VC_VIDEO_DECODE]->put("["+itoa<char>(real_global_time,8)+"] decode_video(): Popping queue.\n");
#endif
		packet=params.packet_queue->pop();
		int frameFinished;
		global_pts=packet->pts;
#if NONS_SYS_WINDOWS
		vc[VC_VIDEO_DECODE]->put("["+itoa<char>(real_global_time,8)+"] decode_video(): Decoding packet.\n");
#endif
		avcodec_decode_video(params.videoCC,videoFrame,&frameFinished,packet->data,packet->size);
		double pts=packet->compute_time(params.videoS,videoFrame);
		if (frameFinished){
			CompleteVideoFrame *new_frame=new CompleteVideoFrame(global_screen,params.videoS,videoFrame,pts);
			if (new_frame->pts>max_pts){
				max_pts=new_frame->pts;
				while (!preQueue.empty()){
					std::set<CompleteVideoFrame *,cmp_pCompleteVideoFrame>::iterator first=preQueue.begin();
#if NONS_SYS_WINDOWS
					vc[VC_AUDIO_DECODE]->put("["+itoa<char>(real_global_time,8)+"] decode_audio(): Pushing frame.\n");
#endif
					frameQueue.push(*first);
					preQueue.erase(first);
				}
			}
			preQueue.insert(new_frame);
		}
		delete packet;
	}
	av_free(videoFrame);
	display.join();
	while (!frameQueue.is_empty()){
		CompleteVideoFrame *frame=frameQueue.pop();
		if (frame)
			delete frame;
	}
	while (!preQueue.empty()){
		std::set<CompleteVideoFrame *,cmp_pCompleteVideoFrame>::iterator first=preQueue.begin();
		delete *first;
		preQueue.erase(first);
	}
	while (!params.packet_queue->is_empty()){
		Packet *packet=params.packet_queue->pop();
		if (packet)
			delete packet;
	}
}

#include "../C_play_video.cpp"

play_video_SIGNATURE{
	stop_playback=0;
	debug_messages=!!print_debug;
	global_screen=screen;
	av_register_all();
	AVFormatContext *avfc;
	SDL_FillRect(screen,0,0);
	SDL_UpdateRect(screen,0,0,0,0);
#if NONS_SYS_WINDOWS
	std::auto_ptr<VirtualConsole> auto_vc[]={
		std::auto_ptr<VirtualConsole>(vc[0]=new VirtualConsole("audio_decode",7)),
		std::auto_ptr<VirtualConsole>(vc[1]=new VirtualConsole("video_decode",7)),
		std::auto_ptr<VirtualConsole>(vc[2]=new VirtualConsole("audio_render",7))
	};
#endif

	if (av_open_input_file(&avfc,input,0,0,0)!=0){
		exception_string="File not found.";
		return 0;
	}
	if (av_find_stream_info(avfc)<0){
		av_close_input_file(avfc);
		exception_string="Stream info not found.";
		return 0;
	}
	if (debug_messages)
		dump_format(avfc,0,input,0);

	AVCodecContext *videoCC,
		*audioCC=0;
	long videoStream=-1,
		audioStream=-1;
	for (ulong a=0;a<avfc->nb_streams && (videoStream<0 || audioStream<0);a++){
		if (avfc->streams[a]->codec->codec_type==CODEC_TYPE_VIDEO)
			videoStream=a;
		else if (avfc->streams[a]->codec->codec_type==CODEC_TYPE_AUDIO)
			audioStream=a;
	}
	bool useAudio=(audioStream!=-1);
	if (videoStream==-1){
		av_close_input_file(avfc);
		if (videoStream==-1)
			exception_string="No video stream.";
		if (audioStream==-1){
			if (exception_string.size())
				exception_string.push_back(' ');
			exception_string.append("No audio stream.");
		}
		return 0;
	}
	videoCC=avfc->streams[videoStream]->codec;
	if (useAudio)
		audioCC=avfc->streams[audioStream]->codec;

	AVCodec *videoCodec=avcodec_find_decoder(videoCC->codec_id),
		*audioCodec=0;
	if (useAudio)
		audioCodec=avcodec_find_decoder(audioCC->codec_id);
	if (!videoCodec || useAudio && !audioCodec){
		av_close_input_file(avfc);
		if (!videoCodec)
			exception_string="Unsupported video codec.";
		if (!audioCodec){
			if (exception_string.size())
				exception_string.push_back(' ');
			exception_string.append("Unsupported audio codec.");
		}
		return 0;
	}
	{
		int video_codec=avcodec_open(videoCC,videoCodec),
			audio_codec=0;
		if (useAudio)
			audio_codec=avcodec_open(audioCC,audioCodec);
		if (video_codec<0 || useAudio && audio_codec<0){
			int ret=0;
			if (video_codec<0)
				exception_string="Open video codec failed.";
			else
				avcodec_close(videoCC);
			if (useAudio){
				if (audio_codec<0){
					if (exception_string.size())
						exception_string.push_back(' ');
					exception_string.append("Open audio codec failed.");
				}else
					avcodec_close(audioCC);
			}
			av_close_input_file(avfc);
			return 0;
		}
	}
	ulong channels,sample_rate;
	if (useAudio){
		channels=audioCC->channels;
		sample_rate=audioCC->sample_rate;
	}else{
		channels=1;
		sample_rate=11025;
	}
	{
		AudioOutput output(channels,sample_rate);
		if (!output.good){
			avcodec_close(videoCC);
			if (useAudio)
				avcodec_close(audioCC);
			av_close_input_file(avfc);
			exception_string="Open audio output failed.";
			return 0;
		}
		output.expect_buffers=useAudio;

		AVPacket packet;
		NONS_Thread audio_decoder;
		TSqueue<Packet *> audio_packets;
		{
			decode_audio_params *params=new decode_audio_params;
			params->audioCC=audioCC;
			params->audioS=avfc->streams[audioStream];
			params->packet_queue=&audio_packets;
			params->output=&output;
			audio_decoder.call(decode_audio,params,1);
		}
		NONS_Thread video_decoder;
		TSqueue<Packet *> video_packets;
		{
			decode_video_params *params=new decode_video_params;
			params->videoCC=videoCC;
			params->videoS=avfc->streams[videoStream];
			params->packet_queue=&video_packets;
			params->aoutput=&output;
			params->user_data=user_data;
			params->callback_pairs=&callback_pairs;
			video_decoder.call(decode_video,params);
		}
		video_packets.max_size=25;
		audio_packets.max_size=200;
		for (ulong a=0;av_read_frame(avfc,&packet)>=0;a++){
			if (*stop){
				av_free_packet(&packet);
				break;
			}
			if (packet.stream_index==videoStream){
				video_packets.push(new Packet(&packet));
			}else if (packet.stream_index==audioStream){
				audio_packets.push(new Packet(&packet));
			}
			av_free_packet(&packet);
		}
		stop_playback=1;
		audio_decoder.join();
		video_decoder.join();
		output.wait_until_stop();
		avcodec_close(videoCC);
		if (useAudio)
			avcodec_close(audioCC);
		av_close_input_file(avfc);
	}
	return 1;
}

PLAYER_TYPE_FUNCTION_SIGNATURE{
	return "FFmpeg";
}

PLAYER_VERSION_FUNCTION_SIGNATURE{
	return C_PLAY_VIDEO_PARAMS_VERSION;
}
