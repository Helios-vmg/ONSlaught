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

#ifndef NONS_THREADMANAGER_H
#define NONS_THREADMANAGER_H

#include "Common.h"
#include <vector>
typedef void (*NONS_ThreadedFunctionPointer)(void *);

#if NONS_SYS_UNIX
#include <pthread.h>
#include <semaphore.h>
#elif NONS_SYS_PSP
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#endif

#define USE_THREAD_MANAGER

class NONS_Event{
	bool initialized;
#if NONS_SYS_WINDOWS
	HANDLE event;
#elif NONS_SYS_UNIX
	sem_t sem;
#elif NONS_SYS_PSP
	SDL_sem *sem;
#endif
public:
	NONS_Event():initialized(0){}
	void init();
	~NONS_Event();
	void set();
	void reset();
	void wait();
};

class NONS_Thread{
	struct threadStruct{ NONS_ThreadedFunctionPointer f; void *d; };
#if NONS_SYS_WINDOWS
	HANDLE thread;
	static DWORD __stdcall runningThread(void *);
#elif NONS_SYS_UNIX
	pthread_t thread;
	static void *runningThread(void *);
#elif NONS_SYS_PSP
	SDL_Thread *thread;
	static int runningThread(void *);
#endif
	bool called;
public:
	NONS_Thread():called(0){}
	NONS_Thread(NONS_ThreadedFunctionPointer function,void *data);
	~NONS_Thread();
	void call(NONS_ThreadedFunctionPointer function,void *data);
	void join();
};

class NONS_ManagedThread{
	bool initialized;
	NONS_Thread thread;
	static void runningThread(void *);
	ulong index;
	volatile bool destroy;
	void *parameter;
public:
	NONS_Event startCallEvent,
		callEndedEvent;
	volatile NONS_ThreadedFunctionPointer function;
	NONS_ManagedThread():initialized(0){}
	~NONS_ManagedThread();
	void init(ulong index);
	void call(NONS_ThreadedFunctionPointer f,void *p);
	void wait();
};

class NONS_ThreadManager{
	std::vector<NONS_ManagedThread> threads;
public:
	NONS_ThreadManager(){}
	NONS_ThreadManager(ulong CPUs);
	void init(ulong CPUs);
	NONS_DECLSPEC ulong call(ulong onThread,NONS_ThreadedFunctionPointer f,void *p);
	NONS_DECLSPEC void wait(ulong index);
	NONS_DECLSPEC void waitAll();
	static void setCPUcount();
};

extern NONS_DECLSPEC NONS_ThreadManager threadManager;

class NONS_DECLSPEC NONS_Mutex{
#if NONS_SYS_WINDOWS
	//pointer to CRITICAL_SECTION
	void *mutex;
#elif NONS_SYS_UNIX
	pthread_mutex_t mutex;
#elif NONS_SYS_PSP
	SDL_mutex *mutex;
#endif
#ifdef DEBUG_SCREEN_MUTEX
	NONS_Mutex *mutex_for_self;
	void *last_locker;
#endif
public:
#ifndef DEBUG_SCREEN_MUTEX
	NONS_Mutex();
#else
	NONS_Mutex(bool track_self=0);
#endif
	~NONS_Mutex();
	void lock();
	void unlock();
#ifdef DEBUG_SCREEN_MUTEX
	bool is_locked();
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
class NONS_Atomic{
	T data;
	NONS_Mutex mutex;
public:
	NONS_Atomic(const T &d):data(d){}
	NONS_Atomic(const NONS_Atomic &o):data(o.data){}
	const NONS_Atomic &operator=(const NONS_Atomic &o){
		NONS_MutexLocker ml(this->mutex);
		this->data=o.data;
		return *this;
	}
	operator const T &() const{
		NONS_MutexLocker ml(this->mutex);
		return this->data;
	}
	const T &operator=(const T &o){
		NONS_MutexLocker ml(this->mutex);
		this->data=o;
		return this->data;
	}
	const T &operator++(){
		NONS_MutexLocker ml(this->mutex);
		return ++this->data;
	}
	const T &operator++(int){
		NONS_MutexLocker ml(this->mutex);
		return this->data++;
	}
};
#endif
