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

#include "ThreadManager.h"
#include "IOFunctions.h"
#include "CommandLineOptions.h"

#define NONS_PARALLELIZE

#if NONS_SYS_WINDOWS
#include <windows.h>
#elif NONS_SYS_UNIX
#include <unistd.h>
#elif NONS_SYS_PSP
#undef NONS_PARALLELIZE
#endif

DLLexport ulong cpu_count=1;
DLLexport NONS_ThreadManager threadManager;

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

void NONS_ManagedThread::init(ulong index){
	this->initialized=1;
	this->index=index;
	this->startCallEvent.init();
	this->callEndedEvent.init();
	this->thread.call(runningThread,this);
	this->function=0;
	this->parameter=0;
	this->destroy=0;
}

NONS_ManagedThread::~NONS_ManagedThread(){
	if (!this->initialized)
		return;
	//this->wait();
	this->destroy=1;
	this->startCallEvent.set();
	this->thread.join();
}

void NONS_ManagedThread::call(NONS_ThreadedFunctionPointer f,void *p){
	this->function=f;
	this->parameter=p;
	this->startCallEvent.set();
}

void NONS_ManagedThread::wait(){
	this->callEndedEvent.wait();
}

NONS_ThreadManager::NONS_ThreadManager(ulong CPUs){
	this->init(CPUs);
}

void NONS_ThreadManager::init(ulong CPUs){
	this->threads.resize(CPUs-1);
	for (ulong a=0;a<this->threads.size();a++)
		this->threads[a].init(a);
}

ulong NONS_ThreadManager::call(ulong onThread,NONS_ThreadedFunctionPointer f,void *p){
	if (onThread>=this->threads.size())
		return -1;
	this->threads[onThread].call(f,p);
	return onThread;
}

void NONS_ThreadManager::wait(ulong index){
	this->threads[index].wait();
}

void NONS_ThreadManager::waitAll(){
	for (ulong a=0;a<this->threads.size();a++)
		this->wait(a);
}

void NONS_ManagedThread::runningThread(void *p){
	NONS_ManagedThread *t=(NONS_ManagedThread *)p;
	while (1){
		t->startCallEvent.wait();
		if (t->destroy)
			break;
		t->function(t->parameter);
		t->parameter=0;
		t->function=0;
		t->callEndedEvent.set();
	}
}

void NONS_ThreadManager::setCPUcount(){
	if (!CLOptions.noThreads){
#ifdef NONS_PARALLELIZE
		//get CPU count
		{
#if NONS_SYS_WINDOWS
			SYSTEM_INFO si;
			GetSystemInfo(&si);
			cpu_count=si.dwNumberOfProcessors;
/*#elif NONS_SYS_LINUX
			FILE * fp;
			char res[128];
			fp = popen("/bin/cat /proc/cpuinfo |grep -c '^processor'","r");
			fread(res, 1, sizeof(res)-1, fp);
			fclose(fp);
			cpu_count=atoi(res);*/
#elif NONS_SYS_UNIX
			cpu_count=sysconf(_SC_NPROCESSORS_ONLN);
			if (cpu_count<1)
				cpu_count=1;
#endif
		}
		o_stdout <<"Using "<<cpu_count<<" CPU"<<(cpu_count!=1?"s":"")<<".\n";
#else
		o_stdout <<"Parallelization disabled.\n";
		cpu_count=1;
#endif
	}else{
		o_stdout <<"Parallelization disabled.\n";
		cpu_count=1;
	}
}

NONS_Thread::NONS_Thread(NONS_ThreadedFunctionPointer function,void *data){
	this->called=0;
	this->call(function,data);
}

NONS_Thread::~NONS_Thread(){
	if (!this->called)
		return;
	this->join();
#if NONS_SYS_WINDOWS
	CloseHandle(this->thread);
#endif
}

void NONS_Thread::call(NONS_ThreadedFunctionPointer function,void *data){
	if (this->called)
		return;
	threadStruct *ts=new threadStruct;
	ts->f=function;
	ts->d=data;
#if NONS_SYS_WINDOWS
	this->thread=CreateThread(0,0,(LPTHREAD_START_ROUTINE)runningThread,ts,0,0);
#elif NONS_SYS_UNIX
	pthread_create(&this->thread,0,runningThread,ts);
#elif NONS_SYS_PSP
	this->thread=SDL_CreateThread(runningThread,ts);
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
#elif NONS_SYS_PSP
	SDL_WaitThread(this->thread,0);
#endif
	this->called=0;
}

#if NONS_SYS_WINDOWS
DWORD WINAPI 
#elif NONS_SYS_UNIX
void *
#elif NONS_SYS_PSP
int
#endif
NONS_Thread::runningThread(void *p){
	srand((unsigned int)time(0));
	NONS_ThreadedFunctionPointer f=((threadStruct *)p)->f;
	void *d=((threadStruct *)p)->d;
	delete (threadStruct *)p;
	f(d);
	return 0;
}

#ifndef DEBUG_SCREEN_MUTEX
NONS_Mutex::NONS_Mutex(){
#else
NONS_Mutex::NONS_Mutex(bool track_self){
#endif
#if NONS_SYS_WINDOWS
	this->mutex=new CRITICAL_SECTION;
	InitializeCriticalSection((CRITICAL_SECTION *)this->mutex);
#elif NONS_SYS_UNIX
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&this->mutex,&attr);
	pthread_mutexattr_destroy(&attr);
#elif NONS_SYS_PSP
	this->mutex=SDL_CreateMutex();
#endif
#ifdef DEBUG_SCREEN_MUTEX
	this->mutex_for_self=(track_self)?new NONS_Mutex:0;
	this->last_locker=0;
#endif
}

NONS_Mutex::~NONS_Mutex(){
#if NONS_SYS_WINDOWS
	DeleteCriticalSection((CRITICAL_SECTION *)this->mutex);
	delete (CRITICAL_SECTION *)this->mutex;
#elif NONS_SYS_UNIX
	pthread_mutex_destroy(&this->mutex);
#elif NONS_SYS_PSP
	SDL_DestroyMutex(this->mutex);
#endif
#ifdef DEBUG_SCREEN_MUTEX
	delete this->mutex_for_self;
#endif
}

void NONS_Mutex::lock(){
#ifdef DEBUG_SCREEN_MUTEX
	if (this->mutex_for_self)
		this->mutex_for_self->lock();
#endif
#if NONS_SYS_WINDOWS
	EnterCriticalSection((CRITICAL_SECTION *)this->mutex);
#elif NONS_SYS_UNIX
	pthread_mutex_lock(&this->mutex);
#elif NONS_SYS_PSP
	SDL_LockMutex(this->mutex);
#endif
#ifdef DEBUG_SCREEN_MUTEX
	if (this->mutex_for_self){
		this->last_locker=GetCurrentThread();
		this->mutex_for_self->unlock();
	}
#endif
}

void NONS_Mutex::unlock(){
#ifdef DEBUG_SCREEN_MUTEX
	if (this->mutex_for_self)
		this->mutex_for_self->lock();
#endif
#if NONS_SYS_WINDOWS
	LeaveCriticalSection((CRITICAL_SECTION *)this->mutex);
#elif NONS_SYS_UNIX
	pthread_mutex_unlock(&this->mutex);
#elif NONS_SYS_PSP
	SDL_UnlockMutex(this->mutex);
#endif
#ifdef DEBUG_SCREEN_MUTEX
	if (this->mutex_for_self){
		this->last_locker=0;
		this->mutex_for_self->unlock();
	}
#endif
}

#ifdef DEBUG_SCREEN_MUTEX
bool NONS_Mutex::is_locked(){
	if (!this->mutex_for_self)
		return 0;
	NONS_MutexLocker ml(*this->mutex_for_self);
	HANDLE owner=this->last_locker/*((_RTL_CRITICAL_SECTION *)this->mutex)->OwningThread*/,
		this_thread=GetCurrentThread();
	return owner==this_thread;
}
#endif
