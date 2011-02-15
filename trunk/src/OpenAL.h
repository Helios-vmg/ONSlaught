/*
* Copyright (c) 2008-2011, Helios (helios.vmg@gmail.com)
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

#ifndef NONS_OPENAL_H
#define NONS_OPENAL_H
#include "Common.h"
#include "Archive.h"
#include <vector>
#include <list>
#include <cstdlib>
#include <cassert>
#include <al.h>
#include <alc.h>

class audio_sink{
	ALuint source;
	static const size_t n=16;
	ALint get_state(){
		ALint state;
		alGetSourcei(this->source,AL_SOURCE_STATE,&state);
		return state;
	}
public:
	audio_sink();
	~audio_sink();
	operator bool(){ return !!this->source; }
	void push(const void *buffer,size_t length,ulong freq,ulong channels,ulong bit_depth);
	bool needs_more_data();
	void pause(){ alSourcePause(this->source); }
	void unpause(){ alSourcePlay(this->source); }
	void set_volume(float vol){ alSourcef(this->source,AL_GAIN,vol); }
};

struct audio_buffer{
	void *buffer;
	size_t length;
	ulong frequency;
	ulong channels;
	ulong bit_depth;
	audio_buffer(void *buffer,size_t length,ulong freq,ulong channels,ulong bit_depth,bool take_ownership=0);
	~audio_buffer(){
		free(this->buffer);
	}
	void push(audio_sink &sink){
		sink.push(this->buffer,this->length*this->channels,this->frequency,this->channels,this->bit_depth);
	}
	static void *allocate(ulong samples,ulong bytes_per_channel,ulong channels){
		return malloc(samples*bytes_per_channel*channels);
	}
};

class decoder{
protected:
	NONS_DataStream *stream;
	audio_buffer *buffer;
	bool good;
public:
	decoder(NONS_DataStream *stream);
	virtual ~decoder();
	virtual operator bool(){ return this->good; }
	virtual audio_buffer *get_buffer(bool &error)=0;
	virtual void loop()=0;
};

class audio_stream;
typedef std::list<audio_stream *> list_t;

class audio_stream{
	audio_sink *sink;
	decoder *decoder;
	bool good,
		playing,
		paused;
	float volume;
public:
	bool loop;
	list_t::iterator iterator;
	audio_stream(const std::wstring &filename);
	~audio_stream();
	operator bool(){ return this->good; }
	void start();
	void stop();
	void pause();
	void update();
	bool is_playing(){ return this->playing; }
	bool is_paused(){ return this->paused; }
	void set_volume(float);
};

class audio_device{
	ALCdevice *device;
	ALCcontext *context;
	list_t streams;
	bool good;
public:
	audio_device();
	~audio_device();
	operator bool(){ return this->good; }
	void update();
	void add(audio_stream &);
	void remove(list_t::iterator &);
};
#endif
