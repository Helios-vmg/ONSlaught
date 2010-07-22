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

#ifndef NONS_IMAGELOADER_H
#define NONS_IMAGELOADER_H

#include "Common.h"
#include "Archive.h"
#include "FileLog.h"
#include "../svg_loader.h"
#include "Plugin/LibraryLoader.h"
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <vector>

#undef LoadImage

struct NONS_AnimationInfo{	
	enum TRANSPARENCY_METHODS{
		LEFT_UP='l',
		RIGHT_UP='r',
		COPY_TRANS='c',
		PARALLEL_MASK='a',
		SEPARATE_MASK='m'
	} method;
	ulong animation_length;
	/*
	If size==1, the first element contains how long each frame will stay on.
	Otherwise, element i contains how long will frame i stay on, plus the
	value of element i-1 if i>0.
	For example, for the string "<10,20,30>", the resulting contents will be
	{10,30,60}
	*/
	std::vector<ulong> frame_ends;
	enum LOOP_TYPE{
		SAWTOOTH_WAVE_CYCLE=0,
		SINGLE_CYCLE,
		TRIANGLE_WAVE_CYCLE,
		NO_CYCLE
	} loop_type;
	ulong animation_time_offset;
	int animation_direction;
	bool valid;
	static TRANSPARENCY_METHODS default_trans;

	NONS_AnimationInfo(){}
	NONS_AnimationInfo(const std::wstring &image_string);
	NONS_AnimationInfo(const NONS_AnimationInfo &b);
	NONS_AnimationInfo &operator=(const NONS_AnimationInfo &b);
	~NONS_AnimationInfo();
	void parse(const std::wstring &image_string);
	void resetAnimation();
	long advanceAnimation(ulong msecs);
	long getCurrentAnimationFrame();
	const std::wstring &getFilename() const{
		return this->filename;
	}
	const std::wstring &getString() const{
		return this->string;
	}
	const std::wstring &getMaskFilename() const{
		return this->mask_filename;
	}
private:
	std::wstring filename;
	std::wstring string;
	std::wstring mask_filename;
};

struct SVG_Functions{
#define SVG_Functions_DECLARE_MEMBER(id) id##_f id
	SVG_Functions_DECLARE_MEMBER(SVG_load);
	SVG_Functions_DECLARE_MEMBER(SVG_unload);
	SVG_Functions_DECLARE_MEMBER(SVG_get_dimensions);
	SVG_Functions_DECLARE_MEMBER(SVG_set_scale);
	SVG_Functions_DECLARE_MEMBER(SVG_best_fit);
	SVG_Functions_DECLARE_MEMBER(SVG_set_rotation);
	SVG_Functions_DECLARE_MEMBER(SVG_set_matrix);
	SVG_Functions_DECLARE_MEMBER(SVG_transform_coordinates);
	SVG_Functions_DECLARE_MEMBER(SVG_add_scale);
	SVG_Functions_DECLARE_MEMBER(SVG_render);
	SVG_Functions_DECLARE_MEMBER(SVG_render2);
	SVG_Functions_DECLARE_MEMBER(SVG_have_linear_transformations);
	bool valid;
};

class NONS_DiskCache{
	typedef std::map<std::wstring,std::wstring,stdStringCmpCI<wchar_t> > map_t;
	map_t cache_list;
	ulong state;
public:
	NONS_DiskCache():state(0){}
	~NONS_DiskCache();
	void add(const std::wstring &filename,SDL_Surface *surface);
	void remove(const std::wstring &filename);
	SDL_Surface *get(const std::wstring &filename);
};

typedef std::map<std::pair<ulong,ulong>,SDL_Rect> optim_t;

struct NONS_Image{
	SDL_Surface *image;
	NONS_AnimationInfo animation;
	ulong refCount;
	optim_t optimized_updates;
	/*
	If the image was rendered from an SVG, this holds the virtual pointer
	returned by SVG_load().
	Otherwise, this is set to zero.
	*/
	ulong svg_source;
	static SVG_Functions *svg_functions;

	NONS_Image();
	NONS_Image(SDL_Surface *image);
	NONS_Image(const NONS_AnimationInfo *anim,const NONS_Image *primary,const NONS_Image *secondary,double base_scale[2],optim_t *rects);
	~NONS_Image();
	SDL_Surface *LoadImage(const std::wstring &string,NONS_DataStream *stream,NONS_DiskCache *dcache,double base_scale[2]);
private:
	SDL_Rect getUpdateRect(ulong from,ulong to);
};

struct NONS_ImageLoader{
	std::vector<NONS_Image *> imageCache;
	NONS_LibraryLoader *svg_library;
	SVG_Functions svg_functions;
	NONS_FileLog *filelog;
	bool fast_svg;
	double base_scale[2];
	NONS_DiskCache disk_cache;

	NONS_ImageLoader();
	~NONS_ImageLoader();
	void init();
	ulong getCacheSize();
	bool fetchSprite(SDL_Surface *&dst,const std::wstring &string,optim_t *rects=0);
	bool unfetchImage(SDL_Surface *which);
	NONS_Image *elementFromSurface(SDL_Surface *srf);
	void printCurrent();
private:
	//1 if the image was added, 0 otherwise
	bool addElementToCache(NONS_Image *img);
	bool initialized;
};

extern NONS_ImageLoader ImageLoader;
#endif
