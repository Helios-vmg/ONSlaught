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

#include "Audio.h"
#include "IOFunctions.h"
#include "CommandLineOptions.h"
#include "Archive.h"
#include <iostream>

#define NONS_Audio_FOREACH() for (chan_t::iterator i=this->channels.begin(),e=this->channels.end();i!=e;++i)

NONS_Audio::NONS_Audio(const std::wstring &musicDir){
	this->notmute=0;
	this->uninitialized=1;
	if (CLOptions.no_sound){
		this->dev=0;
		return;
	}
	this->dev=new audio_device;
	if (!*this->dev){
		delete this->dev;
		this->dev=0;
		return;
	}
	this->uninitialized=0;
	this->notmute=1;
	if (!musicDir.size())
		this->music_dir=L"./CD";
	else
		this->music_dir=musicDir;
	this->music_format=CLOptions.musicFormat;
	this->channel_counter=initial_channel_counter;
	this->mvol=this->svol=1.f;
	this->stop_thread=0;
	this->thread.call(member_bind(&NONS_Audio::update_thread,this),1);
}

NONS_Audio::~NONS_Audio(){
	if (this->uninitialized)
		return;
	this->stop_thread=1;
	this->thread.join();
	delete this->dev;
}

void NONS_Audio::update_thread(){
	while (!this->stop_thread){
		{
			NONS_MutexLocker ml(this->mutex);
			this->dev->update();
		}
		SDL_Delay(10);
	}
}

audio_stream *NONS_Audio::get_channel(int channel){
	NONS_MutexLocker ml(this->mutex);
	chan_t::iterator i=this->channels.find(channel);
	return (i==this->channels.end())?0:i->second;
}

extern const wchar_t *sound_formats[];

ErrorCode NONS_Audio::play_music(const std::wstring &filename,long times){
	if (this->uninitialized)
		return NONS_NO_ERROR;
	const int channel=NONS_Audio::music_channel;
	std::wstring temp;
	if (!this->music_format.size()){
		ulong a=0;
		while (1){
			if (!sound_formats[a])
				break;
			temp=this->music_dir+L"/"+filename+L"."+sound_formats[a];
			if (general_archive.exists(temp))
				break;
			a++;
		}
	}else
		temp=this->music_dir+L"/"+filename+L"."+this->music_format;
	if (!general_archive.exists(temp) && !general_archive.exists(temp=filename))
		return NONS_FILE_NOT_FOUND;

	NONS_MutexLocker ml(this->mutex);
	audio_stream *stream=this->get_channel(channel);
	if (stream)
		this->dev->remove(stream);
	stream=new audio_stream(temp);
	if (!*stream){
		delete stream;
		return NONS_UNDEFINED_ERROR;
	}
	stream->loop=times;
	stream->cleanup=0;
	stream->set_general_volume(this->mvol);
	stream->start();
	this->dev->add(stream);
	this->channels[NONS_Audio::music_channel]=stream;
	return NONS_NO_ERROR;
}

ErrorCode NONS_Audio::stop_music(){
	if (this->uninitialized)
		return NONS_NO_ERROR;
	NONS_MutexLocker ml(this->mutex);
	audio_stream *stream=this->get_channel(NONS_Audio::music_channel);
	if (!stream)
		return NONS_NO_MUSIC_LOADED;
	stream->stop();
	return NONS_NO_ERROR;
}

ErrorCode NONS_Audio::pause_music(){
	if (this->uninitialized)
		return NONS_NO_ERROR;
	audio_stream *stream=this->get_channel(NONS_Audio::music_channel);
	if (!stream)
		return NONS_NO_MUSIC_LOADED;
	stream->pause();
	return NONS_NO_ERROR;
}

ErrorCode NONS_Audio::play_sound(const std::wstring &filename,int channel,long times,bool automatic_cleanup){
	if (this->uninitialized)
		return NONS_NO_ERROR;
	if (!general_archive.exists(filename))
		return NONS_FILE_NOT_FOUND;
	ErrorCode e;
	e=this->load_sound_on_a_channel(filename,channel,channel>=0);
	if (!CHECK_FLAG(e,NONS_NO_ERROR_FLAG))
		return e;
	e=this->play(channel,times,automatic_cleanup);
	return (!CHECK_FLAG(e,NONS_NO_ERROR_FLAG))?e:NONS_NO_ERROR;
}

ErrorCode NONS_Audio::load_sound_on_a_channel(const std::wstring &filename,int &channel,bool use_channel_as_input){
	if (!use_channel_as_input)
		channel=INT_MAX;
	if (this->uninitialized)
		return NONS_NO_ERROR;
	if (!general_archive.exists(filename))
		return NONS_FILE_NOT_FOUND;
	if (!use_channel_as_input)
		channel=this->channel_counter++;
	NONS_MutexLocker ml(this->mutex);
	audio_stream *stream=this->get_channel(channel);
	if (stream)
		this->dev->remove(stream);
	stream=new audio_stream(filename);
	if (!*stream){
		delete stream;
		return NONS_UNDEFINED_ERROR;
	}
	this->dev->add(stream);
	this->channels[channel]=stream;
	return NONS_NO_ERROR;
}

ErrorCode NONS_Audio::unload_sound_from_channel(int channel){
	if (this->uninitialized)
		return NONS_NO_ERROR;
	NONS_MutexLocker ml(this->mutex);
	audio_stream *stream=this->get_channel(channel);
	if (!stream)
		return NONS_NO_SOUND_EFFECT_LOADED;
	this->dev->remove(stream);
	return NONS_NO_ERROR;
}

ErrorCode NONS_Audio::play(int channel,long times,bool automatic_cleanup){
	if (this->uninitialized)
		return NONS_NO_ERROR;
	NONS_MutexLocker ml(this->mutex);
	audio_stream *stream=this->get_channel(channel);
	if (!stream)
		return NONS_NO_SOUND_EFFECT_LOADED;
	stream->loop=times;
	stream->cleanup=automatic_cleanup;
	stream->set_general_volume(this->svol);
	stream->start();
	return NONS_NO_ERROR;
}

void NONS_Audio::wait_for_channel(int channel){
	NONS_Event event;
	event.init();
	{
		NONS_MutexLocker ml(this->mutex);
		audio_stream *stream=this->get_channel(channel);
		stream->notify_on_stop(&event);
	}
	event.wait();
}

ErrorCode NONS_Audio::stop_sound(int channel){
	if (this->uninitialized)
		return NONS_NO_ERROR;
	NONS_MutexLocker ml(this->mutex);
	audio_stream *stream=this->get_channel(channel);
	if (!stream)
		return NONS_NO_SOUND_EFFECT_LOADED;
	stream->stop();
	return NONS_NO_ERROR;
}

ErrorCode NONS_Audio::stop_all_sound(){
	if (this->uninitialized)
		return NONS_NO_ERROR;
	NONS_MutexLocker ml(this->mutex);
	NONS_Audio_FOREACH()
		i->second->stop();
	return NONS_NO_ERROR;
}

int NONS_Audio::set_volume(float &p,int vol){
	if (this->uninitialized)
		return 0;
	if (vol<0){
		NONS_MutexLocker ml(this->mutex);
		return int(p*100.f);
	}
	saturate_value(vol,0,100);
	NONS_MutexLocker ml(this->mutex);
	p=vol/100.f;
	return vol;
}

int NONS_Audio::channel_volume(int channel,int vol){
	if (this->uninitialized)
		return 0;
	if (vol<0){
		NONS_MutexLocker ml(this->mutex);
		audio_stream *stream=this->get_channel(channel);
		if (!stream)
			return 0;
		return int(stream->get_volume()*100.f);
	}
	saturate_value(vol,0,100);
	NONS_MutexLocker ml(this->mutex);
	audio_stream *stream=this->get_channel(channel);
	if (!stream)
		return 0;
	stream->set_volume(vol/100.f);
	return vol;
}

bool NONS_Audio::toggle_mute(){
	if (this->uninitialized)
		return 0;
	NONS_MutexLocker ml(this->mutex);
	this->notmute=!this->notmute;
	NONS_Audio_FOREACH()
		i->second->mute(!this->notmute);
	return this->notmute;
}

bool NONS_Audio::is_playing(int channel){
	if (this->uninitialized)
		return 0;
	NONS_MutexLocker ml(this->mutex);
	audio_stream *stream=this->get_channel(channel);
	if (!stream)
		return 0;
	return stream->is_sink_playing();
}

void read_channel(channel_listing::channel &c,const audio_stream &stream){
	if (!stream.is_playing()){
		c.filename.clear();
		c.loop=0;
		c.volume=100;
	}else{
		c.filename=stream.filename;
		c.loop=(stream.loop>0)?-1:0;
		c.volume=int(stream.get_volume()*100.f);
	}
}

void NONS_Audio::get_channel_listing(channel_listing &cl){
	if (this->uninitialized)
		return;
	NONS_MutexLocker ml(this->mutex);
	NONS_Audio_FOREACH(){
		if (i->first==-1)
			read_channel(cl.music,*i->second);
		else{
			channel_listing::channel c;
			read_channel(c,*i->second);
			cl.sounds[i->first]=c;
		}
	}
}

asynchronous_audio_stream *NONS_Audio::new_video_stream(){
	if (this->uninitialized)
		return 0;
	NONS_MutexLocker ml(this->mutex);
	asynchronous_audio_stream *stream=new asynchronous_audio_stream();
	this->dev->add(stream);
	return stream;
}
