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

NONS_AudioDeviceManager::NONS_AudioDeviceManager():stop_thread(0){
	if (!*this)
		return;
	this->thread.call(member_bind(&NONS_AudioDeviceManager::controller_thread,this));
}

NONS_AudioDeviceManager::~NONS_AudioDeviceManager(){
	{
		NONS_MutexLocker ml(this->mutex);
		this->stop_thread=1;
	}
	this->thread.join();
}

void NONS_AudioDeviceManager::process_instruction(const instruction &i){
	bool r;
	switch (i.opcode){
		case instruction::LOAD:
			{
				chan_t::iterator it=this->channels.find(i.channel);
				if (it!=this->channels.end())
					this->remove(it);
				audio_stream *stream=new audio_stream(i.path);
				r=*stream;
				if (!r)
					delete stream;
				else
					this->dev.add(this->channels[i.channel]=stream);
			}
			break;
		case instruction::UNLOAD:
			{
				chan_t::iterator it=this->channels.find(i.channel);
				if (it==this->channels.end()){
					r=0;
					break;
				}
				r=1;
				this->remove(it);
			}
			break;
		case instruction::SETLOOP:
			{
				chan_t::iterator it=this->channels.find(i.channel);
				if (it==this->channels.end()){
					r=0;
					break;
				}
				r=1;
				it->second->loop=i.loop_times;
			}
			break;
		case instruction::PLAY:
			{
				chan_t::iterator it=this->channels.find(i.channel);
				if (it==this->channels.end()){
					r=0;
					break;
				}
				r=1;
				it->second->cleanup=i.automatic_cleanup;
				it->second->start();
			}
			break;
		case instruction::STOP:
			{
				chan_t::iterator it=this->channels.find(i.channel);
				if (it==this->channels.end()){
					r=0;
					break;
				}
				r=1;
				it->second->stop();
			}
			break;
		case instruction::STOPALL:
			r=1;
			for (chan_t::iterator it=this->channels.begin(),e=this->channels.end();it!=e;++it)
				it->second->stop();
			break;
		case instruction::PAUSE:
			{
				chan_t::iterator it=this->channels.find(i.channel);
				if (it==this->channels.end()){
					r=0;
					break;
				}
				r=1;
				it->second->pause();
			}
			break;
		case instruction::SETVOL:
			{
				chan_t::iterator it=this->channels.find(i.channel);
				if (it==this->channels.end()){
					r=0;
					break;
				}
				r=1;
				it->second->set_volume(i.volume);
			}
			break;
		case instruction::SETVOL_POSITIVE:
			{
				r=1;
				for (chan_t::iterator it=this->channels.begin(),e=this->channels.end();it!=e;++it){
					if (it->first<0)
						continue;
					it->second->set_general_volume(i.volume);
				}
			}
			break;
		case instruction::SETVOL_NEGATIVE:
			{
				r=1;
				for (chan_t::iterator it=this->channels.begin(),e=this->channels.end();it!=e;++it){
					if (it->first>=0)
						continue;
					it->second->set_general_volume(i.volume);
				}
			}
			break;
		case instruction::MUTE:
			{
				chan_t::iterator it=this->channels.find(i.channel);
				if (it==this->channels.end()){
					r=0;
					break;
				}
				r=1;
				it->second->mute();
			}
			break;
		case instruction::MUTEALL:
			r=1;
			for (chan_t::iterator it=this->channels.begin(),e=this->channels.end();it!=e;++it)
				it->second->mute();
			break;
		case instruction::NOTIFY:
			{
				chan_t::iterator it=this->channels.find(i.channel);
				if (it==this->channels.end()){
					r=0;
					break;
				}
				r=1;
				it->second->notify_on_stop(i.event_param);
			}
			break;
	}
	this->success_stack.push_back(r);
	i.event->set();
}

void NONS_AudioDeviceManager::controller_thread(){
	while (1){
		{
			NONS_MutexLocker ml(this->mutex);
			if (this->stop_thread)
				break;
			bool i_set=0;
			instruction i;
			if (this->queue.size()){
				i=this->queue.front();
				this->queue.pop_front();
				i_set=1;
			}
			if (i_set){
				this->process_instruction(i);
				continue;
			}
			this->dev.update();
		}
		SDL_Delay(50);
	}
}

void NONS_AudioDeviceManager::remove(chan_t::iterator i){
	this->dev.remove(i->second);
	this->channels.erase(i);
}

bool NONS_AudioDeviceManager::push(instruction &i){
	NONS_Event event;
	event.init();
	i.event=&event;
	{
		NONS_MutexLocker ml(this->mutex);
		this->queue.push_back(i);
	}
	event.wait();
	bool r;
	{
		NONS_MutexLocker ml(this->mutex);
		r=this->success_stack.back();
		this->success_stack.pop_back();
	}
	return r;
}

bool NONS_AudioDeviceManager::load(int channel,const std::wstring &path){
	instruction i;
	i.opcode=instruction::LOAD;
	i.channel=channel;
	i.path=path;
	return this->push(i);
}

bool NONS_AudioDeviceManager::unload(int channel){
	instruction i;
	i.opcode=instruction::UNLOAD;
	i.channel=channel;
	return this->push(i);
}

bool NONS_AudioDeviceManager::set_loop(int channel,long loop){
	instruction i;
	i.opcode=instruction::SETLOOP;
	i.channel=channel;
	i.loop_times=loop;
	return this->push(i);
}

bool NONS_AudioDeviceManager::play(int channel,bool automatic_cleanup){
	instruction i;
	i.opcode=instruction::PLAY;
	i.channel=channel;
	i.automatic_cleanup=automatic_cleanup;
	return this->push(i);
}

bool NONS_AudioDeviceManager::stop(int channel){
	instruction i;
	i.opcode=instruction::STOP;
	i.channel=channel;
	return this->push(i);
}

bool NONS_AudioDeviceManager::stop_all(){
	instruction i;
	i.opcode=instruction::STOPALL;
	return this->push(i);
}

bool NONS_AudioDeviceManager::pause(int channel){
	instruction i;
	i.opcode=instruction::PAUSE;
	i.channel=channel;
	return this->push(i);
}

bool NONS_AudioDeviceManager::set_volume(int channel,float vol){
	instruction i;
	i.opcode=instruction::SETVOL;
	i.channel=channel;
	i.volume=vol;
	return this->push(i);
}

bool NONS_AudioDeviceManager::set_volume_music(float vol){
	instruction i;
	i.opcode=instruction::SETVOL_NEGATIVE;
	i.volume=vol;
	return this->push(i);
}

bool NONS_AudioDeviceManager::set_volume_sfx(float vol){
	instruction i;
	i.opcode=instruction::SETVOL_POSITIVE;
	i.volume=vol;
	return this->push(i);
}

bool NONS_AudioDeviceManager::mute(int channel){
	instruction i;
	i.opcode=instruction::MUTE;
	i.channel=channel;
	return this->push(i);
}

bool NONS_AudioDeviceManager::mute_all(){
	instruction i;
	i.opcode=instruction::MUTEALL;
	return this->push(i);
}

bool NONS_AudioDeviceManager::notify(int channel,NONS_Event *event){
	instruction i;
	i.opcode=instruction::NOTIFY;
	i.channel=channel;
	i.event_param=event;
	return this->push(i);
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

void NONS_AudioDeviceManager::get_channel_listing(channel_listing &cl){
	NONS_MutexLocker ml(this->mutex);
	for (chan_t::iterator i=this->channels.begin(),e=this->channels.end();i!=e;++i){
		if (i->first==-1)
			read_channel(cl.music,*i->second);
		else{
			channel_listing::channel c;
			read_channel(c,*i->second);
			cl.sounds[i->first]=c;
		}
	}
}

bool NONS_AudioDeviceManager::is_playing(int channel){
	NONS_MutexLocker ml(this->mutex);
	chan_t::iterator i=this->channels.find(channel);
	if (i==this->channels.end())
		return 0;
	return i->second->is_sink_playing();
}

NONS_Audio::NONS_Audio(const std::wstring &musicDir){
	if (CLOptions.no_sound){
		this->uninitialized=1;
		this->manager=0;
		this->notmute=0;
		return;
	}
	this->manager=new NONS_AudioDeviceManager;
	if (!musicDir.size())
		this->musicDir=L"./CD";
	else
		this->musicDir=musicDir;
	this->musicFormat=CLOptions.musicFormat;
	this->notmute=1;
	this->uninitialized=0;
	this->channel_counter=initial_channel_counter;
	this->mvol=this->svol=100;
}

NONS_Audio::~NONS_Audio(){
	if (this->uninitialized)
		return;
	delete this->manager;
}

extern const wchar_t *sound_formats[];

ErrorCode NONS_Audio::playMusic(const std::wstring &filename,long times){
	if (this->uninitialized)
		return NONS_NO_ERROR;
	const int channel=NONS_Audio::music_channel;
	std::wstring temp;
	if (!this->musicFormat.size()){
		ulong a=0;
		while (1){
			if (!sound_formats[a])
				break;
			temp=this->musicDir+L"/"+filename+L"."+sound_formats[a];
			if (general_archive.exists(temp))
				break;
			a++;
		}
	}else
		temp=this->musicDir+L"/"+filename+L"."+this->musicFormat;
	if (!general_archive.exists(temp) && !general_archive.exists(temp=filename))
		return NONS_FILE_NOT_FOUND;
	if (!this->manager->load(channel,temp) || !this->manager->set_loop(channel,times) || !this->manager->play(channel,0))
		return NONS_UNDEFINED_ERROR;
	return NONS_NO_ERROR;
}

ErrorCode NONS_Audio::stopMusic(){
	if (this->uninitialized)
		return NONS_NO_ERROR;
	return (this->manager->stop(NONS_Audio::music_channel))?NONS_NO_ERROR:NONS_NO_MUSIC_LOADED;
}

ErrorCode NONS_Audio::pauseMusic(){
	if (this->uninitialized)
		return NONS_NO_ERROR;
	return (this->manager->pause(NONS_Audio::music_channel))?NONS_NO_ERROR:NONS_NO_MUSIC_LOADED;
}

ErrorCode NONS_Audio::playSound(const std::wstring &filename,int channel,long times,bool automatic_cleanup){
	if (this->uninitialized)
		return NONS_NO_ERROR;
	if (!general_archive.exists(filename))
		return NONS_FILE_NOT_FOUND;
	ErrorCode e;
	e=this->loadSoundOnAChannel(filename,channel,channel>=0);
	if (!CHECK_FLAG(e,NONS_NO_ERROR_FLAG))
		return e;
	e=this->play(channel,times,automatic_cleanup);
	return (!CHECK_FLAG(e,NONS_NO_ERROR_FLAG))?e:NONS_NO_ERROR;
}

ErrorCode NONS_Audio::loadSoundOnAChannel(const std::wstring &filename,int &channel,bool use_channel_as_input){
	if (!use_channel_as_input)
		channel=INT_MAX;
	if (this->uninitialized)
		return NONS_NO_ERROR;
	if (!general_archive.exists(filename))
		return NONS_FILE_NOT_FOUND;
	if (!use_channel_as_input)
		channel=this->channel_counter++;
	if (!this->manager->load(channel,filename))
		return NONS_UNDEFINED_ERROR;
	return NONS_NO_ERROR;
}

ErrorCode NONS_Audio::unloadSoundFromChannel(int channel){
	if (this->uninitialized)
		return NONS_NO_ERROR;
	if (!this->manager->unload(channel))
		return NONS_UNDEFINED_ERROR;
	return NONS_NO_ERROR;
}

ErrorCode NONS_Audio::play(int channel,long times,bool automatic_cleanup){
	if (this->uninitialized)
		return NONS_NO_ERROR;
	if (!this->manager->set_loop(channel,times) || !this->manager->play(channel,automatic_cleanup))
		return NONS_UNDEFINED_ERROR;
	return NONS_NO_ERROR;
}

void NONS_Audio::waitForChannel(int channel){
	NONS_Event event;
	event.init();
	this->manager->notify(channel,&event);
	event.wait();
}

ErrorCode NONS_Audio::stopSound(int channel){
	if (this->uninitialized)
		return NONS_NO_ERROR;
	return (this->manager->stop(channel))?NONS_NO_ERROR:NONS_NO_SOUND_EFFECT_LOADED;
}

ErrorCode NONS_Audio::stopAllSound(){
	if (this->uninitialized)
		return NONS_NO_ERROR;
	return this->manager->stop_all();
	return NONS_NO_ERROR;
}

int NONS_Audio::musicVolume(int vol){
	if (this->uninitialized)
		return 0;
	if (vol<0)
		return this->mvol;
	saturate_value(vol,0,100);
	this->mvol=vol;
	return (!this->manager->set_volume_music(float(vol)/100.f))?0:vol;
}

int NONS_Audio::soundVolume(int vol){
	if (this->uninitialized)
		return 0;
	if (vol<0)
		return this->svol;
	saturate_value(vol,0,100);
	this->svol=vol;
	return (!this->manager->set_volume_sfx(float(vol)/100.f))?0:vol;
}

int NONS_Audio::channelVolume(int channel,int vol){
	if (this->uninitialized)
		return 0;
	saturate_value(vol,0,100);
	return (!this->manager->set_volume(channel,float(vol)/100.f))?0:vol;
}

bool NONS_Audio::toggleMute(){
	if (this->uninitialized)
		return 0;
	this->notmute=!this->notmute;
	this->manager->mute_all();
	return this->notmute;
}

bool NONS_Audio::is_playing(int channel){
	bool r=this->manager->is_playing(channel);
	return r;
}
