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

#include "AudioFormats.h"
#include <iostream>

size_t ogg_read(void *buffer,size_t size,size_t nmemb,void *s){
	NONS_DataStream *stream=(NONS_DataStream *)s;
	if (!stream->read(buffer,size,size*nmemb)){
		errno=1;
		return 0;
	}
	errno=0;
	return size;
}

int ogg_seek(void *s,ogg_int64_t offset,int whence){
	NONS_DataStream *stream=(NONS_DataStream *)s;
	stream->stdio_seek(offset,whence);
	return 0;
}

long ogg_tell(void *s){
	return (long)((NONS_DataStream *)s)->seek(0,0);
}

std::string ogg_code_to_string(int e){
	switch (e){
		case 0:
			return "no error";
		case OV_EREAD:
			return "a read from media returned an error";
		case OV_ENOTVORBIS:
			return "bitstream does not contain any Vorbis data";
		case OV_EVERSION:
			return "vorbis version mismatch";
		case OV_EBADHEADER:
			return "invalid Vorbis bitstream header";
		case OV_EFAULT:
			return "internal logic fault; indicates a bug or heap/stack corruption";
		default:
			return "unknown error";
	}
}

ogg_decoder::ogg_decoder(NONS_DataStream *stream):decoder(stream){
	if (!*this)
		return;
	this->bitstream=0;
	ov_callbacks cb;
	cb.read_func=ogg_read;
	cb.seek_func=ogg_seek;
	cb.tell_func=ogg_tell;
	cb.close_func=0;
	int error=ov_open_callbacks(stream,&this->file,0,0,cb);
	if (error<0){
		o_stderr <<ogg_code_to_string(error);
		this->good=0;
		return;
	}
}

ogg_decoder::~ogg_decoder(){
	ov_clear(&this->file);
}

audio_buffer *ogg_decoder::get_buffer(bool &error){
	static char nativeEndianness=checkNativeEndianness();
	const size_t n=1<<12;
	char *temp=(char *)audio_buffer::allocate(n/4,2,2);
	size_t size=0;
	while (size<n){
		int r=ov_read(&this->file,temp+size,n-size,nativeEndianness==NONS_BIG_ENDIAN,2,1,&this->bitstream);
		if (r<0){
			error=1;
			return 0;
		}
		error=0;
		if (!r){
			if (!size)
				return 0;
			break;
		}
		size+=r;
	}
	vorbis_info *i=ov_info(&this->file,this->bitstream);
	return new audio_buffer(temp,size/(i->channels*2),i->rate,i->channels,16,1);
}

void ogg_decoder::loop(){
	ov_pcm_seek(&this->file,0);
}

inline int16_t fix_24bit_sample(FLAC__int32 v){
	return (v+0x80)>>8;
}

inline int16_t fix_32bit_sample(FLAC__int32 v){
	FLAC__int32 v2=v+0x80;
	if (v2<v)
		v2|=~v2;
	return (v+0x80)>>8;
}

flac_decoder::flac_decoder(NONS_DataStream *stream):decoder(stream){
	if (!*this)
		return;
	this->set_md5_checking(1);
	this->good=0;
	if (this->init()!=FLAC__STREAM_DECODER_INIT_STATUS_OK)
		return;
	this->good=1;
}

flac_decoder::~flac_decoder(){
}

FLAC__StreamDecoderWriteStatus flac_decoder::write_callback(const FLAC__Frame *frame,const FLAC__int32 * const *buffer){
	void *push_me;
	bool stereo=frame->header.channels!=1;
	ulong channels=stereo?2:1,
		bits;
	switch (frame->header.bits_per_sample){
		case 8:
			bits=8;
			{
				push_me=audio_buffer::allocate(frame->header.blocksize,bits/8,channels);
				int8_t *temp_buffer=(int8_t *)push_me;
				for (size_t i=0;i<frame->header.blocksize;i++){
					temp_buffer[i*channels]=buffer[0][i];
					if (stereo)
						temp_buffer[i*channels+1]=buffer[1][i];
				}
			}
			break;
		case 16:
			bits=16;
			{
				push_me=audio_buffer::allocate(frame->header.blocksize,bits/8,channels);
				int16_t *temp_buffer=(int16_t *)push_me;
				for (size_t i=0;i<frame->header.blocksize;i++){
					temp_buffer[i*channels]=buffer[0][i];
					if (stereo)
						temp_buffer[i*channels+1]=buffer[1][i];
				}
			}
			break;
		case 24:
			bits=16;
			{
				push_me=audio_buffer::allocate(frame->header.blocksize,bits/8,channels);
				int16_t *temp_buffer=(int16_t *)push_me;
				for (size_t i=0;i<frame->header.blocksize;i++){
					temp_buffer[i*channels]=fix_24bit_sample(buffer[0][i]);
					if (stereo)
						temp_buffer[i*channels+1]=fix_24bit_sample(buffer[1][i]);
				}
			}
			break;
		case 32:
			bits=16;
			{
				push_me=audio_buffer::allocate(frame->header.blocksize,bits/8,channels);
				int16_t *temp_buffer=(int16_t *)push_me;
				for (size_t i=0;i<frame->header.blocksize;i++){
					temp_buffer[i*channels]=fix_32bit_sample(buffer[0][i]);
					if (stereo)
						temp_buffer[i*channels+1]=fix_32bit_sample(buffer[1][i]);
				}
			}
			break;
	}
	assert(!this->buffer);
	this->buffer=new audio_buffer(push_me,frame->header.blocksize,frame->header.sample_rate,channels,bits,1);
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__StreamDecoderReadStatus flac_decoder::read_callback(FLAC__byte *buffer,size_t *bytes){
	return
		(!this->stream || !this->stream->read(buffer,*bytes,*bytes))
		?FLAC__STREAM_DECODER_READ_STATUS_ABORT
		:FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus flac_decoder::seek_callback(FLAC__uint64 absolute_byte_offset){
	if (!this->stream)
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	this->stream->seek(absolute_byte_offset,1);
	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

bool flac_decoder::eof_callback(){
	return (!this->stream)?1:this->stream->get_offset()>=this->stream->get_size();
}

FLAC__StreamDecoderTellStatus flac_decoder::tell_callback(FLAC__uint64 *absolute_byte_offset){
	if (!this->stream)
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	*absolute_byte_offset=this->stream->get_offset();
	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus flac_decoder::length_callback(FLAC__uint64 *stream_length){
	if (!this->stream)
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
	*stream_length=this->stream->get_size();
	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

audio_buffer *flac_decoder::get_buffer(bool &error){
	bool ok=1;
	while (!this->buffer && (ok=this->process_single()) && this->get_state()!=FLAC__STREAM_DECODER_END_OF_STREAM);
	error=!ok;
	audio_buffer *ret=this->buffer;
	this->buffer=0;
	return ret;
}

void flac_decoder::loop(){
	this->seek_absolute(0);
}

NONS_Mutex mp3_decoder::mutex;
bool mp3_decoder::mpg123_initialized=0;

struct fd_tracker{
	NONS_Mutex mutex;
	int counter;
	typedef std::map<int,NONS_DataStream *> map_t;
	map_t map;
	fd_tracker():counter(0){}
	int add(NONS_DataStream *);
	NONS_DataStream *get(int);
} tracker;

int fd_tracker::add(NONS_DataStream *s){
	NONS_MutexLocker ml(this->mutex);
	this->map[this->counter]=s;
	return this->counter++;
}

NONS_DataStream *fd_tracker::get(int fd){
	NONS_MutexLocker ml(this->mutex);
	return this->map[fd];
}

ssize_t mp3_read(int fd,void *dst,size_t n){
	NONS_DataStream *stream=tracker.get(fd);
	stream->read(dst,n,n);
	return n;
}

off_t mp3_seek(int fd,off_t offset,int whence){
	NONS_DataStream *stream=tracker.get(fd);
	return (off_t)stream->stdio_seek(offset,whence);
}

#define HANDLE_MPG123_ERRORS(call,r) {                 \
	error=call;                                        \
	if (error!=MPG123_OK){                             \
		o_stderr <<mpg123_plain_strerror(error)<<"\n"; \
		return r;                                      \
	}                                                  \
}

mp3_decoder::mp3_decoder(NONS_DataStream *stream):decoder(stream){
	if (!*this)
		return;
	this->good=0;
	int error;
	{
		NONS_MutexLocker ml(mp3_decoder::mutex);
		if (!mp3_decoder::mpg123_initialized){
			HANDLE_MPG123_ERRORS(mpg123_init(),);
			mp3_decoder::mpg123_initialized=1;
		}
	}
	this->handle=mpg123_new(0,&error);
	if (!this->handle){
		o_stderr <<mpg123_plain_strerror(error)<<"\n";
		return;
	}
	HANDLE_MPG123_ERRORS(mpg123_replace_reader(this->handle,mp3_read,mp3_seek),);
	HANDLE_MPG123_ERRORS(mpg123_open_fd(this->handle,tracker.add(stream)),);
	HANDLE_MPG123_ERRORS(mpg123_format_none(this->handle),);
}

mp3_decoder::~mp3_decoder(){
	mpg123_close(this->handle);
	mpg123_delete(this->handle);
}

audio_buffer *mp3_decoder::get_buffer(bool &there_was_an_error){
	there_was_an_error=1;
	int error;
	HANDLE_MPG123_ERRORS(mpg123_format(this->handle,44100,MPG123_STEREO,MPG123_ENC_SIGNED_16),0);
	uchar *buffer;
	off_t offset;
	size_t size;
	error=mpg123_decode_frame(this->handle,&offset,&buffer,&size);
	if (error==MPG123_DONE){
		there_was_an_error=0;
		return 0;
	}
	if (error!=MPG123_OK && error!=MPG123_NEW_FORMAT){
		o_stderr <<mpg123_plain_strerror(error)<<"\n";
		return 0;
	}
	ulong channels=2;
	there_was_an_error=0;
	return new audio_buffer(buffer,size/(channels*2),44100,channels,16);
}

void mp3_decoder::loop(){
	mpg123_seek(this->handle,0,SEEK_SET);
}

#if 0
bool mod_decoder::mikmod_initialized=0;
NONS_Mutex mod_decoder::mutex;

mod_decoder::mod_decoder(NONS_DataStream *stream):decoder(stream){
	if (!*this)
		return;
	this->good=0;
	{
		NONS_MutexLocker ml(mod_decoder::mutex);
		if (!mod_decoder::mpg123_initialized){
			MikMod_Init(
			MDRIVER d;
			d.
			MikMod_RegisterDriver
			MikMod_RegisterDriver
			mp3_decoder::mpg123_initialized=1;
		}
	}
}

mod_decoder::~mod_decoder(){
}

audio_buffer *mod_decoder::get_buffer(bool &error){
}

void mod_decoder::loop(){
}
#endif
