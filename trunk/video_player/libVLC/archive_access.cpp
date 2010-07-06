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

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_input.h>
#include <vlc_access.h>
#include <vlc_dialog.h>
#include <vlc_charset.h>
#include <cstdio>
#include <cctype>
#include <cassert>
#include <cerrno>
#include "../../video_player.h"

int Open(vlc_object_t *);
void Close(vlc_object_t *);

#define CACHING_TEXT N_("")
#define CACHING_LONGTEXT N_("")

#define MODULE_STRING "ONSlaughtAccess"
#define N_(x) (x)
vlc_module_begin ()
	set_description( N_("ONSlaught archive input") )
	set_shortname( N_("ONSlaught") )
	set_category( CAT_INPUT )
	set_subcategory( SUBCAT_INPUT_ACCESS )
	add_integer( "file-caching", DEFAULT_PTS_DELAY / 1000, NULL, CACHING_TEXT, CACHING_LONGTEXT, true )
		change_safe()
	add_obsolete_string( "file-cat" )
	set_capability( "access", 50 )
	add_shortcut( "file" )
	add_shortcut( "fd" )
	add_shortcut( "stream" )
	set_callbacks( Open, Close )
vlc_module_end ()

int Seek( access_t *, int64_t );
ssize_t Read( access_t *, uint8_t *, size_t );
int Control( access_t *, int, va_list );

typedef unsigned long ulong;

ulong hextoi(const char *s){
	ulong ret=0;
	sscanf(s,"%x",&ret);
	return ret;
}

int Open(vlc_object_t *p_this){
	access_t *p_access=(access_t*)p_this;
	var_Create(p_access,"file-caching",VLC_VAR_INTEGER|VLC_VAR_DOINHERIT);
	access_InitFields(p_access);
	ACCESS_SET_CALLBACKS(Read,0,Control,Seek);
	p_access->p_sys=(access_sys_t *)calloc(1,sizeof(file_protocol));
	file_protocol *p_sys=(file_protocol *)p_access->p_sys;
	if (!p_sys)
		return VLC_ENOMEM;
	const char *s=p_access->psz_path;
	size_t a,
		slash=(size_t)-1;
	for (a=0;s[a];a++)
		if (s[a]=='/')
			slash=a;
	file_protocol *p=(file_protocol *)hextoi(s+slash+1);
	if (slash==(size_t)-1){
		free(p_sys);
		return VLC_EGENERIC;
	}
	memcpy(p_sys,p,sizeof(*p_sys));
	return VLC_SUCCESS;
}

void Close(vlc_object_t *p_this){
	access_t *p_access=(access_t*)p_this;
	file_protocol *p_sys=(file_protocol *)p_access->p_sys;
	free(p_sys);
}


ssize_t Read(access_t *p_access,uint8_t *p_buffer,size_t i_len){
	file_protocol *p_sys=(file_protocol *)p_access->p_sys;
	int r=p_sys->read(p_sys->data,p_buffer,(int)i_len);
	if (r<0){
		p_access->info.b_eof=1;
		return 0;
	}
	p_access->info.i_pos+=r;
	p_access->info.i_size=p_sys->seek(p_sys->data,0,2);
	p_access->info.i_update|=INPUT_UPDATE_SIZE;
	return r;
}

int Seek(access_t *p_access, int64_t i_pos){
	file_protocol *p_sys=(file_protocol *)p_access->p_sys;
	p_access->info.i_pos=p_sys->seek(p_sys->data,i_pos,1);
	p_access->info.b_eof=0;
	return VLC_SUCCESS;
}

int Control( access_t *p_access, int i_query, va_list args ){
	access_sys_t *p_sys = p_access->p_sys;
	bool *pb_bool;
	int64_t *pi_64;

	switch(i_query){
		case ACCESS_CAN_SEEK:
		case ACCESS_CAN_FASTSEEK:
			pb_bool=(bool*)va_arg(args,bool *);
			*pb_bool=1;
			break;
		case ACCESS_CAN_PAUSE:
		case ACCESS_CAN_CONTROL_PACE:
			pb_bool=(bool*)va_arg(args,bool*);
			*pb_bool=1;
			break;
		case ACCESS_GET_PTS_DELAY:
			pi_64=(int64_t*)va_arg(args,int64_t *);
			*pi_64=var_GetInteger(p_access,"file-caching")*(uint64_t)1000;
		case ACCESS_SET_PAUSE_STATE:
			break;
		default:
			return VLC_EGENERIC;
	}
	return VLC_SUCCESS;
}
