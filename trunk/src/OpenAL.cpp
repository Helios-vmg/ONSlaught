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

#include "OpenAL.h"
#include "AudioFormats.h"

decoder::decoder(NONS_DataStream *stream):stream(stream),buffer(0),good(stream){}

decoder::~decoder(){
	general_archive.close(this->stream);
	free(this->buffer);
}

audio_sink::audio_sink(){
	alGenSources(1,&this->source);
	if (alGetError()!=AL_NO_ERROR)
		this->source=0;
}

audio_sink::~audio_sink(){
	if (*this)
		alDeleteSources(1,&this->source);
}

ALenum make_format(ulong channels,ulong bit_depth){
	static const ALenum array[]={
		AL_FORMAT_MONO8,
		AL_FORMAT_MONO16,
		AL_FORMAT_STEREO8,
		AL_FORMAT_STEREO16
	};
	return array[((channels==2)<<1)|(bit_depth==16)];
}

bool audio_sink::needs_more_data(){
	if (!*this)
		return 0;
	ALint finished,
		queued,
		state;
	alGetSourcei(this->source,AL_BUFFERS_PROCESSED,&finished);
	alGetSourcei(this->source,AL_BUFFERS_QUEUED,&queued);
	state=this->get_state();
	return !(!finished && queued>=n && (state==AL_PLAYING || state==AL_PAUSED));
}

void audio_sink::push(const void *buffer,size_t length,ulong freq,ulong channels,ulong bit_depth){
	if (!*this)
		return;
	ALint queued,
		finished,
		state;
	alGetSourcei(this->source,AL_BUFFERS_PROCESSED,&finished);
	alGetSourcei(this->source,AL_BUFFERS_QUEUED,&queued);
	state=this->get_state();
	assert(state!=AL_PAUSED);
	bool call_play=(finished==queued || state!=AL_PLAYING);
	std::vector<ALuint> temp(queued);
	ALuint new_buffer;
	if (finished){
		alSourceUnqueueBuffers(this->source,finished,&temp[0]);
		alDeleteBuffers(finished-1,&temp[0]);
		new_buffer=temp[finished-1];
	}else
		alGenBuffers(1,&new_buffer);
	alBufferData(
		new_buffer,
		make_format(channels,bit_depth),
		buffer,
		length*((bit_depth==8)?1:2),
		freq
	);
	alSourceQueueBuffers(this->source,1,&new_buffer);
	if (call_play)
		alSourcePlay(this->source);
}

audio_buffer::audio_buffer(void *buffer,size_t length,ulong freq,ulong channels,ulong bit_depth,bool take_ownership){
	size_t n=length*(bit_depth/8)*channels;
	if (!take_ownership){
		this->buffer=malloc(n);
		memcpy(this->buffer,buffer,n);
	}else
		this->buffer=buffer;
	this->length=length;
	this->frequency=freq;
	this->channels=channels;
	this->bit_depth=bit_depth;
}

audio_stream::audio_stream(const std::wstring &filename){
	std::wstring copy=filename;
	tolower(copy);
	if (ends_with(copy,(std::wstring)L".ogg"))
		this->decoder=new ogg_decoder(general_archive.open(filename));
	else if (ends_with(copy,(std::wstring)L".flac"))
		this->decoder=new flac_decoder(general_archive.open(filename));
	/*else if (ends_with(copy,L".mp3"))
		this->decoder=new mp3_decoder(general_archive.open(filename));*/
	else
		this->decoder=0;
	this->sink=0;
	this->loop=0;
	this->playing=0;
	this->paused=0;
	this->good=this->decoder;
	this->volume=1.0;
}

audio_stream::~audio_stream(){
	this->stop();
	delete this->decoder;
}

void audio_stream::start(){
	this->sink=new audio_sink;
	if (!*this->sink){
		delete this->sink;
		this->sink=0;
	}else{
		this->playing=1;
		this->paused=0;
		this->sink->set_volume(this->volume);
	}
}

void audio_stream::stop(){
	delete this->sink;
	this->sink=0;
	this->playing=0;
	this->paused=0;
}

void audio_stream::pause(){
	if (this->paused){
		this->sink->unpause();
		this->paused=0;
	}else if (this->playing){
		this->sink->pause();
		this->paused=1;
	}
}

void audio_stream::update(){
	if (!this->sink || !this->playing || this->paused || !*this)
		return;
	while (this->sink->needs_more_data()){
		bool error;
		audio_buffer *buffer;
		buffer=this->decoder->get_buffer(error);
		bool stop=0;
		if (!buffer && !error){
			if (this->loop){
				this->decoder->loop();
				buffer=this->decoder->get_buffer(error);
				if (!buffer && !error)
					stop=1;
			}else
				stop=1;
		}
		if (stop){
			this->playing=0;
			return;
		}
		if (error){
			delete buffer;
			delete this->sink;
			this->sink=0;
			this->good=0;
			return;
		}
		buffer->push(*this->sink);
		delete buffer;
	}
}

void audio_stream::set_volume(float vol){
	if (vol<0)
		vol=0;
	else if (vol>1.f)
		vol=1.f;
	this->volume=vol;
	if (this->sink)
		this->sink->set_volume(vol);
}

audio_device::audio_device(){
	this->device=0;
	this->context=0;
	this->good=0;
	this->device=alcOpenDevice(0);
	if (!this->device)
		return;
	this->context=alcCreateContext(this->device,0);
	if (!this->context)
		return;
	this->good=1;
	alcMakeContextCurrent(this->context);
}

audio_device::~audio_device(){
	while (this->streams.size()){
		delete this->streams.front();
		this->streams.pop_front();
	}
	alcMakeContextCurrent(0);
	if (this->context)
		alcDestroyContext(this->context);
	if (this->device)
		alcCloseDevice(this->device);
}

void audio_device::update(){
	for (list_t::iterator i=this->streams.begin(),e=this->streams.end();i!=e;i++)
		(*i)->update();
}

void audio_device::add(audio_stream &stream){
	this->streams.push_front(&stream);
	stream.iterator=this->streams.begin();
}
