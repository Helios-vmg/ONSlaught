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

#ifndef NONS_AUDIO_H
#define NONS_AUDIO_H

#include <SDL/SDL.h>
#include "Common.h"
#include "ErrorCodes.h"
#include "ThreadManager.h"
#include "IOFunctions.h"
#include "OpenAL.h"
#include <map>
#include <list>
#include <string>

struct channel_listing{
	struct channel{
		std::wstring filename;
		long loop;
		int volume;
	};
	channel music;
	std::map<int,channel> sounds;
};

class NONS_AudioDeviceManager{
	audio_device dev;
	typedef std::map<int,audio_stream *> chan_t;
	chan_t channels;
	struct instruction{
		enum operation{
			LOAD,
			UNLOAD,
			SETLOOP,
			PLAY,
			STOP,
			STOPALL,
			PAUSE,
			SETVOL,
			SETVOL_POSITIVE,
			SETVOL_NEGATIVE,
			MUTE,
			MUTEALL,
			NOTIFY
		} opcode;
		int channel;
		std::wstring path;
		long loop_times;
		float volume;
		NONS_Event *event,
			*event_param;
		bool automatic_cleanup;
	};
	std::deque<instruction> queue;
	std::vector<bool> success_stack;
	NONS_Thread thread;
	NONS_Mutex mutex;
	bool stop_thread;
	void controller_thread();
	void process_instruction(const instruction &i);
	void remove(chan_t::iterator i);
	bool push(instruction &i);
public:
	NONS_AudioDeviceManager();
	~NONS_AudioDeviceManager();
	operator bool(){ return this->dev; }
	bool load(int channel,const std::wstring &path);
	bool unload(int channel);
	bool set_loop(int channel,long loop);
	bool play(int channel,bool automatic_cleanup);
	bool stop(int channel);
	bool stop_all();
	bool pause(int channel);
	bool set_volume(int channel,float vol);
	bool set_volume_music(float vol);
	bool set_volume_sfx(float vol);
	bool mute(int channel);
	bool mute_all();
	bool notify(int channel,NONS_Event *event);
	void get_channel_listing(channel_listing &cl);
	bool is_playing(int channel);
};

class NONS_Audio{
	NONS_AudioDeviceManager *manager;
	bool uninitialized,
		notmute;
	static const int initial_channel_counter=1<<20;
	int channel_counter;
	int mvol,
		svol;
public:
	static const long max_valid_channel=(long)initial_channel_counter-1;
	static const int music_channel=-1;
	std::wstring musicDir,
		musicFormat;
	NONS_Audio(const std::wstring &musicDir);
	~NONS_Audio();
	ErrorCode play_music(const std::wstring &filename,long times=-1);
	ErrorCode stop_music();
	ErrorCode pause_music();
	ErrorCode play_sound_once(const std::wstring &filename){
		return this->play_sound(filename,-1,0,1);
	}
	ErrorCode play_sound(const std::wstring &filename,int channel,long times,bool automatic_cleanup);
	ErrorCode stop_sound(int channel);
	ErrorCode stop_all_sound();
	ErrorCode load_sound_on_a_channel(const std::wstring &filename,int &channel,bool use_channel_as_input=0);
	ErrorCode unload_sound_from_channel(int channel);
	ErrorCode play(int channel,long times,bool automatic_cleanup);
	void wait_for_channel(int channel);
	int music_volume(int vol);
	int sound_volume(int vol);
	int channel_volume(int channel,int vol);
	bool toggle_mute();
	bool is_playing(int channel);
	bool is_initialized(){
		return !this->uninitialized;
	}
	void get_channel_listing(channel_listing &cl){
		if (!this->uninitialized)
			this->manager->get_channel_listing(cl);
	}
};

class NONS_ScopedAudioStream{
	int channel;
	NONS_Audio *audio;
	bool good;
public:
	NONS_ScopedAudioStream():audio(0),good(0){}
	NONS_ScopedAudioStream(NONS_Audio *audio,const std::wstring &filename){
		this->init(audio,filename);
	}
	~NONS_ScopedAudioStream(){
		if (this->good)
			this->audio->unload_sound_from_channel(this->channel);
	}
	void init(NONS_Audio *audio,const std::wstring &filename){
		this->audio=audio;
		this->good=this->audio && this->audio->load_sound_on_a_channel(filename,this->channel)==NONS_NO_ERROR;
	}
	void play(bool loop){
		if (this->good)
			this->audio->play(this->channel,loop?-1:0,0);
	}
	void stop(){
		if (this->good)
			this->audio->stop_sound(this->channel);
	}
};
#endif
