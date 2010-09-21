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

#include "Image.h"
#include "ThreadManager.h"
#include "Plugin/LibraryLoader.h"
#include "../svg_loader.h"
#include "FileLog.h"
#include "IOFunctions.h"
#include "Archive.h"
#include <SDL/SDL_image.h>
#include <png.h>
#include <list>
#include <cmath>

NONS_Color NONS_Color::white            (0xFF,0xFF,0xFF);
NONS_Color NONS_Color::black            (0x00,0x00,0x00);
NONS_Color NONS_Color::black_transparent(0x00,0x00,0x00,0x00);
NONS_Color NONS_Color::red              (0xFF,0x00,0x00);
NONS_Color NONS_Color::green            (0x00,0xFF,0x00);
NONS_Color NONS_Color::blue             (0x00,0x00,0xFF);

NONS_AnimationInfo::TRANSPARENCY_METHODS NONS_AnimationInfo::default_trans=COPY_TRANS;

void NONS_AnimationInfo::parse(const std::wstring &image_string){
	this->string=image_string;
	this->valid=0;
	this->method=NONS_AnimationInfo::default_trans;
	this->animation_length=1;
	this->animation_time_offset=0;
	this->animation_direction=1;
	this->frame_ends.clear();
	size_t p=0;
	static const std::wstring slash_semi=L"/;";
	if (image_string[p]==':'){
		p++;
		size_t semicolon=image_string.find(';',p);
		if (semicolon==image_string.npos)
			return;
		switch (wchar_t c=NONS_tolower(image_string[p++])){
			case 'l':
			case 'r':
			case 'c':
			case 'a':
			case 'm':
				this->method=(TRANSPARENCY_METHODS)c;
				break;
			default:
				return;
		}
		size_t p2=image_string.find_first_of(slash_semi,p);
		if (this->method==SEPARATE_MASK){
			if (p2==image_string.npos)
				return;
			this->mask_filename=std::wstring(image_string.begin()+p,image_string.begin()+p2);
			tolower(this->mask_filename);
			toforwardslash(this->mask_filename);
		}
		p=p2;
		if (image_string[p]=='/'){
			std::stringstream stream;
			while (image_string[++p]!=',' && image_string[p]!=';')
				stream <<(char)image_string[p];
			if (!(stream >>this->animation_length) || image_string[p]!=',')
				return;
			stream.clear();
			if (image_string[p+1]!='<'){
				while (image_string[++p]!=',' && image_string[p]!=';')
					stream <<(char)image_string[p];
				ulong delay;
				if (!(stream >>delay))
					return;
				stream.clear();
				if (this->frame_ends.size())
					delay+=this->frame_ends.back();
				this->frame_ends.push_back(delay);
			}else{
				p++;
				size_t gt=image_string.find('>',p);
				if (gt==image_string.npos || gt>semicolon)
					return;
				while (image_string[p]!='>' && image_string[++p]!='>'){
					while (image_string[p]!=',' && image_string[p]!='>')
						stream <<(char)image_string[p++];
					ulong delay;
					if (!(stream >>delay))
						return;
					stream.clear();
					this->frame_ends.push_back(delay);
				}
				p++;
			}
			if (!this->frame_ends.size() || this->frame_ends.size()>1 && this->animation_length!=this->frame_ends.size())
				return;
			if (image_string[p]!=',')
				return;
			while (image_string[++p]!=',' && image_string[p]!=';')
				stream <<(char)image_string[p];
			ulong type;
			if (!(stream >>type))
				return;
			this->loop_type=(LOOP_TYPE)type;
			p++;
		}
	}
	if (image_string[p]==';')
		p++;
	this->filename=image_string.substr(p);
	tolower(this->filename);
	toforwardslash(this->filename);
	this->animation_time_offset=0;
	this->valid=1;
}

NONS_AnimationInfo::NONS_AnimationInfo(const std::wstring &image_string){
	this->parse(image_string);
}

NONS_AnimationInfo::NONS_AnimationInfo(const NONS_AnimationInfo &b){
	*this=b;
}

NONS_AnimationInfo &NONS_AnimationInfo::operator=(const NONS_AnimationInfo &b){
	this->method=b.method;
	this->mask_filename=b.mask_filename;
	this->animation_length=b.animation_length;
	this->frame_ends=b.frame_ends;
	this->loop_type=b.loop_type;
	this->filename=b.filename;
	this->string=b.string;
	this->animation_time_offset=b.animation_time_offset;
	this->valid=b.valid;
	return *this;
}

void NONS_AnimationInfo::resetAnimation(){
	this->animation_time_offset=0;
	this->animation_direction=1;
}

long NONS_AnimationInfo::advanceAnimation(ulong msecs){
	if (!this->frame_ends.size() || !this->animation_direction || this->loop_type==NO_CYCLE)
		return -1;
	long original=this->getCurrentAnimationFrame();
	if (this->animation_direction>0)
		this->animation_time_offset+=msecs;
	else if (msecs>this->animation_time_offset){
		this->animation_time_offset=msecs-this->animation_time_offset+this->frame_ends.front();
		this->animation_direction=1;
	}else
		this->animation_time_offset-=msecs;
	long updated=this->getCurrentAnimationFrame();
	if (updated==original)
		return -1;
	if (updated>=0)
		return updated;
	switch (this->loop_type){
		case SAWTOOTH_WAVE_CYCLE:
			this->animation_time_offset%=this->frame_ends.back();
			break;
		case SINGLE_CYCLE:
			this->animation_direction=0;
			return -1;
		case TRIANGLE_WAVE_CYCLE:
			if (this->frame_ends.size()==1)
				this->animation_time_offset=
					this->frame_ends.back()*(this->animation_length-1)-
					(this->animation_time_offset-this->frame_ends.back()*this->animation_length)
					-1;
			else
				this->animation_time_offset=
					this->frame_ends.back()-
					this->animation_time_offset-1-
					(this->frame_ends.back()-this->frame_ends[this->frame_ends.size()-2]);
			this->animation_direction=-1;
			break;
	}
	return this->getCurrentAnimationFrame();
}

long NONS_AnimationInfo::getCurrentAnimationFrame(){
	if (this->frame_ends.size()==1){
		ulong frame=this->animation_time_offset/this->frame_ends.front();
		if (frame>=this->animation_length)
			return -1;
		return frame;
	}
	for (size_t a=0;a<this->frame_ends.size();a++)
		if (this->animation_time_offset<this->frame_ends[a])
			return a;
	return -1;
}

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
	void add(std::wstring filename,const NONS_ConstSurface &surface);
	void remove(const std::wstring &filename);
	NONS_Surface get(const std::wstring &filename);
};

NONS_DiskCache::~NONS_DiskCache(){
	for (map_t::iterator i=this->cache_list.begin(),end=this->cache_list.end();i!=end;i++)
		NONS_File::delete_file(i->second);
}

void NONS_DiskCache::add(std::wstring src,const NONS_ConstSurface &s){
	toforwardslash(src);
	map_t::iterator i=this->cache_list.find(src);
	std::wstring dst;
	if (i==this->cache_list.end()){
		dst=L"__ONSlaught_cache_"+itoaw(this->state++)+L".raw";
		this->cache_list[src]=dst;
	}else
		dst=i->second;
	std::vector<uchar> buffer;
	NONS_ConstSurfaceProperties sp;
	s.get_properties(sp);
	writeDWord(sp.w,buffer);
	writeDWord(sp.h,buffer);
	buffer.resize(buffer.size()+sp.pitch*sp.h,0);
	memcpy(&buffer[8],sp.pixels,sp.pitch*sp.h);
	NONS_File::write(dst,&buffer[0],buffer.size());
}

void NONS_DiskCache::remove(const std::wstring &filename){
	map_t::iterator i=this->cache_list.find(filename);
	if (i==this->cache_list.end())
		return;
	NONS_File::delete_file(i->second);
	this->cache_list.erase(i);
}

NONS_Surface NONS_DiskCache::get(const std::wstring &filename){
	map_t::iterator i=this->cache_list.find(filename);
	if (i==this->cache_list.end())
		return NONS_Surface::null;
	size_t size;
	uchar *buffer=NONS_File::read(i->second,size);
	do{
		if (!buffer)
			break;
		if (size<8)
			break;
		ulong offset=0,
			width=readDWord(buffer,offset),
			height=readDWord(buffer,offset);
		if (size<8+width*height*4)
			break;
		NONS_Surface r(width,height);
		NONS_SurfaceProperties sp;
		r.get_properties(sp);
		memcpy(sp.pixels,buffer+offset,sp.pitch*sp.h);
		return r;
	}while (0);
	delete[] buffer;
	return NONS_Surface::null;
}

class NONS_SurfaceManager{
public:
	class Surface{
		uchar *data;
		ulong w,h;
		long ref_count;
		static ulong obj_count;
		static NONS_Mutex surface_mutex;
		ulong id,
			frames;
		friend class NONS_SurfaceManager;
		Surface(){ this->set_id(); }
		void set_id(){
			surface_mutex.lock();
			this->id=obj_count++;
			surface_mutex.unlock();
		}
		virtual long ref();
		virtual long unref();
	public:
		Surface(ulong w,ulong h);
		Surface(const Surface &);
		virtual ~Surface();
		template <typename T>
		void get_properties(NONS_SurfaceProperties_basic<T> &sp) const{
			sp.pixels=this->data;
			sp.w=this->w;
			sp.h=this->h;
			sp.pitch=this->w*4;
			sp.pixel_count=this->h*this->w;
			sp.byte_count=sp.pixel_count*4;
		}
#define NONS_SurfaceManager_Surface_RELATIONAL_OP(op) bool operator op(const Surface &b) const{ return this->id op b.id; }
		OVERLOAD_RELATIONAL_OPERATORS(NONS_SurfaceManager_Surface_RELATIONAL_OP)
	};
	class ScreenSurface:public Surface{
		SDL_Surface *screen;
		static NONS_Mutex screen_mutex;
		friend class NONS_SurfaceManager;
		long ref();
		long unref();
	public:
		ScreenSurface(SDL_Surface *s);
		~ScreenSurface(){ this->data=0; }
	};
	typedef std::list<Surface *> surfaces_t;
	class index_t{
		bool _good;
		Surface *p;
		surfaces_t::iterator i;
		friend class NONS_SurfaceManager;
		index_t():_good(0){}
	public:
		index_t(const surfaces_t::iterator &i):_good(1),i(i),p(0){}
		index_t(Surface *p):_good(!!p),p(p){}
		const Surface &operator*() const{ return (this->p)?*this->p:**this->i; }
		const Surface *operator->() const{ return (this->p)?this->p:*this->i; }
		Surface &operator*(){ return (this->p)?*this->p:**this->i; }
		Surface *operator->(){ return (this->p)?this->p:*this->i; }
		bool good() const{ return this->_good; }
	};

	NONS_SurfaceManager():initialized(0),screen(0),svg_library(0),filelog(0){}
	~NONS_SurfaceManager();
	void init();
	index_t allocate(ulong w,ulong h);
	index_t load(const NONS_AnimationInfo &ai);
	index_t copy(const index_t &src);
	index_t scale(const index_t &src,double scalex,double scaley);
	index_t resize(const index_t &src,long w,long h);
	index_t rotate(const index_t &src,double rotate);
	index_t transform(const index_t &src,double matrix[4]);
	void assign_screen(SDL_Surface *);
	index_t get_screen();
	SDL_Surface *get_screen(const index_t &);
	bool filelog_check(const std::wstring &string);
	void filelog_writeout();
	void filelog_commit();
	void use_fast_svg(bool);
	void set_base_scale(double x,double y);
	void ref(index_t &i){ i->ref(); }
	void unref(index_t &);
	void update(const index_t &,ulong,ulong,ulong,ulong) const;
private:
	bool initialized;
	surfaces_t surfaces;
	ScreenSurface *screen;
	NONS_LibraryLoader *svg_library;
	SVG_Functions svg_functions;
	NONS_FileLog *filelog;
	bool fast_svg;
	double base_scale[2];
	NONS_DiskCache disk_cache;
	//1 if the image was added, 0 otherwise
	bool addElementToCache(NONS_Surface *img);
	index_t load_image(const std::wstring &filename);
};

ulong NONS_SurfaceManager::Surface::obj_count=0;
NONS_Mutex NONS_SurfaceManager::Surface::surface_mutex;
NONS_Mutex NONS_SurfaceManager::ScreenSurface::screen_mutex;

const ulong unit=1<<16;

NONS_SurfaceManager::~NONS_SurfaceManager(){
	delete this->filelog;
	surfaces_t::iterator i=this->surfaces.begin(),
		e=this->surfaces.end();
	for (;i!=e;i++)
		delete *i;
	delete this->screen;
	delete this->svg_library;
}

#define LOG_FILENAME_OLD L"NScrflog.dat"
#define LOG_FILENAME_NEW L"nonsflog.dat"

void NONS_SurfaceManager::init(){
	if (this->initialized)
		return;
	this->filelog=new NONS_FileLog(LOG_FILENAME_OLD,LOG_FILENAME_NEW);
#if !NONS_SYS_WINDOWS
	this->svg_library=new NONS_LibraryLoader("svg_loader",0);
#else
	this->svg_library=new NONS_LibraryLoader("svg_loader","svg_loader",0);
#endif
	this->svg_functions.valid=1;
#define NONS_ImageLoader_INIT_MEMBER(id)\
	if (this->svg_functions.valid && !(this->svg_functions.id=(id##_f)this->svg_library->getFunction(#id)))\
		this->svg_functions.valid=0
	NONS_ImageLoader_INIT_MEMBER(SVG_load);
	NONS_ImageLoader_INIT_MEMBER(SVG_unload);
	NONS_ImageLoader_INIT_MEMBER(SVG_get_dimensions);
	NONS_ImageLoader_INIT_MEMBER(SVG_set_scale);
	NONS_ImageLoader_INIT_MEMBER(SVG_best_fit);
	NONS_ImageLoader_INIT_MEMBER(SVG_set_rotation);
	NONS_ImageLoader_INIT_MEMBER(SVG_set_matrix);
	NONS_ImageLoader_INIT_MEMBER(SVG_transform_coordinates);
	NONS_ImageLoader_INIT_MEMBER(SVG_add_scale);
	NONS_ImageLoader_INIT_MEMBER(SVG_render);
	NONS_ImageLoader_INIT_MEMBER(SVG_render2);
	NONS_ImageLoader_INIT_MEMBER(SVG_have_linear_transformations);
	this->fast_svg=1;
	this->base_scale[0]=this->base_scale[1]=1;
	this->initialized=1;
}

NONS_SurfaceManager::index_t NONS_SurfaceManager::allocate(ulong w,ulong h){
	this->surfaces.push_front(new Surface(w,h));
	return index_t(this->surfaces.begin());
}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
const int rmask=0xFF000000;
const int gmask=0x00FF0000;
const int bmask=0x0000FF00;
const int amask=0x000000FF;
const int rshift=24;
const int gshift=16;
const int bshift=8;
const int ashift=0;
#else
const int rmask=0x000000FF;
const int gmask=0x0000FF00;
const int bmask=0x00FF0000;
const int amask=0xFF000000;
const int rshift=0;
const int gshift=8;
const int bshift=16;
const int ashift=24;
#endif

NONS_SurfaceManager::index_t NONS_SurfaceManager::load_image(const std::wstring &filename){
	NONS_DataStream *stream=general_archive.open(filename);
	index_t r;
	if (!stream)
		return r;
	SDL_RWops rw=stream->to_rwops();
	SDL_Surface *surface=IMG_Load_RW(&rw,0);
	general_archive.close(stream);
	if (!surface)
		return r;
	{
		SDL_Surface *temp=SDL_CreateRGBSurface(SDL_SWSURFACE,surface->w,surface->h,32,rmask,gmask,bmask,amask);
		SDL_BlitSurface(surface,0,temp,0);
		SDL_FreeSurface(surface);
		surface=temp;
	}
	r=this->allocate(surface->w,surface->h);
	NONS_SurfaceProperties sp;
	r->get_properties(sp);
	SDL_LockSurface(surface);
	memcpy(sp.pixels,surface->pixels,sp.byte_count);
	SDL_UnlockSurface(surface);
	SDL_FreeSurface(surface);
	return r;
}

NONS_SurfaceManager::index_t NONS_SurfaceManager::load(const NONS_AnimationInfo &ai){
	index_t primary=this->load_image(ai.getFilename());
	//TODO: complete me.
	return primary;
}

NONS_SurfaceManager::index_t NONS_SurfaceManager::copy(const index_t &src){
	NONS_ConstSurfaceProperties src_sp;
	src->get_properties(src_sp);
	index_t i=this->allocate(src_sp.w,src_sp.h);
	NONS_SurfaceProperties dst_sp;
	i->get_properties(dst_sp);
	memcpy(dst_sp.pixels,src_sp.pixels,dst_sp.pitch*dst_sp.h);
	return i;
}

void NONS_SurfaceManager::unref(NONS_SurfaceManager::index_t &i){
	if (!i->unref()){
		if (!i.p)
			this->surfaces.erase(i.i);
		else{
			i.p=0;
			delete this->screen;
			this->screen=0;
		}
	}
}

void NONS_SurfaceManager::update(const NONS_SurfaceManager::index_t &i,ulong x,ulong y,ulong w,ulong h) const{
	if (!i.good() || !i.p)
		return;
	SDL_UpdateRect(this->screen->screen,(Sint32)x,(Sint32)y,(Uint32)w,(Uint32)h);
}

NONS_SurfaceManager::index_t NONS_SurfaceManager::scale(const index_t &src,double scale_x,double scale_y){
	return this->resize(src,ulong(src->w*floor(scale_x+.5)),ulong(src->h*floor(scale_x+.5)));
}

#define GET_FRACTION(x) ((((x)&(unit-1))<<16)/unit)

//Fast.
//Scales [1;+Inf.): works
//Scales (0;1): works with decreasing precision as the scale approaches 0
void linear_interpolation1(
		uchar *dst,
		ulong w,
		ulong h,
		ulong dst_advance,
		ulong dst_pitch,
		const uchar *src,
		ulong src_advance,
		ulong src_pitch,
		ulong fractional_advance){
	ulong X=0;
	for (ulong x=0;x<w;x++){
		uchar *dst0=dst;
		const uchar *pixel[2];
		pixel[0]=src+(X>>16)*src_advance;
		pixel[1]=pixel[0]+src_advance;
		ulong weight[2];
		weight[1]=GET_FRACTION(X);
		weight[0]=unit-weight[1];
		for (ulong y=0;y<h;y++){
			dst[0]=uchar((pixel[0][0]*weight[0]+pixel[1][0]*weight[1])>>16);
			dst[1]=uchar((pixel[0][1]*weight[0]+pixel[1][1]*weight[1])>>16);
			dst[2]=uchar((pixel[0][2]*weight[0]+pixel[1][2]*weight[1])>>16);
			dst[3]=uchar((pixel[0][3]*weight[0]+pixel[1][3]*weight[1])>>16);
			pixel[0]+=src_pitch;
			pixel[1]+=src_pitch;
			dst+=dst_pitch;
		}
		X+=fractional_advance;
		dst=dst0+dst_advance;
	}
}

#define FLOOR(x) ((x)&(~(unit-1)))
#define CEIL(x) FLOOR((x)+(unit-1))

//Slow.
//Scales [1;+Inf.): doesn't work
//Scales (0;1): works
void linear_interpolation2(
		uchar *dst,
		ulong w,
		ulong h,
		ulong dst_advance,
		ulong dst_pitch,
		const uchar *src,
		ulong src_advance,
		ulong src_pitch,
		ulong fractional_advance){
	for (ulong y=0;y<h;y++){
		uchar *dst0=dst;
		ulong X0=0,
			X1=fractional_advance;
		for (ulong x=0;x<w;x++){
			const uchar *pixel=src+(X0>>16)*src_advance;
			ulong color[3]={0};
			for (ulong x0=X0;x0<X1;){
				ulong multiplier;
				if (X1-x0<unit)
					multiplier=X1-x0;
				else if (x0==X0)
					multiplier=FLOOR(X0)+unit-X0;
				else
					multiplier=unit;
				color[0]+=pixel[0]*multiplier;
				color[1]+=pixel[1]*multiplier;
				color[2]+=pixel[2]*multiplier;
				pixel+=src_advance;
				x0=FLOOR(x0)+unit;
			}
			dst[0]=uchar(color[0]/fractional_advance);
			dst[1]=uchar(color[1]/fractional_advance);
			dst[2]=uchar(color[2]/fractional_advance);
			dst+=dst_advance;
			X0=X1;
			X1+=fractional_advance;
		}
		dst=dst0+dst_pitch;
		src+=src_pitch;
	}
}

typedef void (*linear_interpolation_f)(uchar *,ulong,ulong,ulong,ulong,const uchar *,ulong,ulong,ulong);

void mirror_1D(
		uchar *dst,
		const uchar *src,
		ulong w,
		ulong h,
		int x_advance,
		int y_advance){
	for (ulong y=0;y<h;y++){
		const Uint32 *src2=(const Uint32 *)src;
		Uint32 *dst2=(Uint32 *)dst;
		src2+=(((y_advance>0)?y:(h-1-y))*w*4)+(((x_advance>0)?0:(w-1))*4);
		dst2+=y*w*4;
		for (ulong x=0;x<w;x++){
			*dst2=*src2;
			src2+=x_advance;
			dst2++;
		}
	}
}

NONS_SurfaceManager::index_t NONS_SurfaceManager::resize(const index_t &_src,long w,long h){
	if (!(w*h))
		return index_t();
	index_t src=_src;
	ulong w0=src->w,
		h0=src->h,
		minus;
	index_t dst=src;
	bool free_src=0;
	if (w<0 || h<0){
		NONS_ConstSurfaceProperties ssp;
		src->get_properties(ssp);
		NONS_SurfaceProperties dsp;
		dst=this->allocate(ssp.w,ssp.h);
		dst->get_properties(dsp);
		mirror_1D(dsp.pixels,ssp.pixels,ssp.w,ssp.h,(w>=0)?1:-1,(h>=0)?1:-1);
		if (w<0)
			w=-w;
		if (h<0)
			h=-h;
		src=dst;
		free_src=1;
	}
	dst=this->allocate(w,h0);
	linear_interpolation_f f;
	if ((ulong)w>=w0){
		f=linear_interpolation1;
		minus=1;
	}else{
		f=linear_interpolation2;
		minus=0;
	}
	f(
		(uchar *)dst->data,
		dst->w,
		dst->h,
		4,
		dst->w*4,
		(uchar *)src->data,
		4,
		src->w*4,
		((w0-minus)<<16)/(w-minus)
	);
	src=dst;
	dst=this->allocate(w,h);
	if ((ulong)h>=h0){
		f=linear_interpolation1;
		minus=1;
	}else{
		f=linear_interpolation2;
		minus=0;
	}
	f(
		(uchar *)dst->data,
		dst->h,
		dst->w,
		dst->w*4,
		4,
		(uchar *)src->data,
		src->w*4,
		4,
		((h0-minus)<<16)/(h-minus)
	);
	if (free_src)
		src->unref();
	return dst;
}

NONS_SurfaceManager::index_t NONS_SurfaceManager::rotate(const index_t &src,double angle){
	double dcos=cos(angle),
		dsin=sin(angle);

	const NONS_SurfaceManager::Surface &s=*src;
	ulong w=(ulong)ceil(abs(s.h*dsin)+abs(s.w*dcos));
	ulong h=(ulong)ceil(abs(s.h*dcos)+abs(s.w*dsin));
	long center_x=s.w<<15;
	long center_y=s.h<<15;
	long dx=w>>1;
	long dy=h>>1;
	long ncos=long(dcos*65536.f);
	long nsin=long(dsin*65536.f);
	long srotx=long(-dx*ncos+dy*nsin)+center_x;
	long sroty=long(-dx*nsin-dy*ncos)+center_y;

	index_t dst=this->allocate(w,h);
	const NONS_SurfaceManager::Surface &d=*dst;

	ulong pitch[]={
		s.w*4,
		d.w*4
	};

	uchar *dp=d.data;
	for (ulong y=0;y<h;y++){
		long rotx=srotx=srotx-nsin;
		long roty=sroty=sroty+ncos;
		for (ulong x=0;x<h;x++){
			long newx=rotx>>16;
			long newy=roty>>16;
			if (newx>=0 && newx<(long)s.w && newy>=0 && newy<(long)s.h) {
				uchar *sp=s.data+newy*pitch[0]+newx*4;
				uchar *cO=sp;
				uchar *cV=(newy<(long)s.h-1)?sp+pitch[0]:sp;
				uchar *cH=(newx<(long)s.w-1)?sp+1:sp;
				uchar *cD=(newx<(long)s.w-1)?cV+1:cV;

				long wx=rotx&0xFFFF;
				long wy=roty&0xFFFF;
				long rwx=wx^0xFFFF;
				long rwy=wy^0xFFFF;
				for (int a=0;a<4;a++){
					long m1=((cO[a]*rwx+cH[a]*wx)>>16);
					long m2=((cV[a]*rwx+cD[a]*wx)>>16);
					dp[a]=uchar((m1*rwy+m2*wy)>>16);
				}
			}			
			rotx+=ncos;
			roty+=nsin;
			dp+=4;
		}
	}
	return dst;
}

bool invert_matrix(double dst[4],double src[4]){
	double a=src[0]*src[3]-src[1]*src[2];
	if (!a)
		return 0;
	a=double(1)/a;
	dst[0]=a*src[3];
	dst[1]=a*-src[1];
	dst[2]=a*-src[2];
	dst[3]=a*src[0];
	return 1;
}

void get_corrected(ulong &w,ulong &h,double matrix[4]){
	double coords[][2]={
		{0,0},
		{0,(double)h},
		{(double)w,0},
		{(double)w,(double)h}
	};
	double minx=0,
		miny=0;
	for (int a=0;a<4;a++){
		double x=coords[a][0]*matrix[0]+coords[a][1]*matrix[1];
		double y=coords[a][0]*matrix[2]+coords[a][1]*matrix[3];
		if (x<minx || !a)
			minx=x;
		if (y<miny || !a)
			miny=y;
	}
	w=(ulong)-minx;
	h=(ulong)-miny;
}

void get_final_size(const NONS_ConstSurfaceProperties &src,double matrix[4],ulong &w,ulong &h){
	ulong w0=src.w,
		h0=src.h;
	double coords[][2]={
		{0,0},
		{0,(double)h0},
		{(double)w0,0},
		{(double)w0,(double)h0}
	};
	double minx=0,
		miny=0,
		maxx=0,
		maxy=0;
	for (int a=0;a<4;a++){
		double x=coords[a][0]*matrix[0]+coords[a][1]*matrix[1];
		double y=coords[a][0]*matrix[2]+coords[a][1]*matrix[3];
		if (x<minx || !a)
			minx=x;
		if (x>maxx || !a)
			maxx=x;
		if (y<miny || !a)
			miny=y;
		if (y>maxy || !a)
			maxy=y;
	}
	w=ulong(maxx-minx);
	h=ulong(maxy-miny);
}

NONS_SurfaceManager::index_t NONS_SurfaceManager::transform(const index_t &src,double matrix[4]){
	double inverted_matrix[4];
	if (!invert_matrix(inverted_matrix,matrix))
		return index_t();
	NONS_ConstSurfaceProperties src_sp;
	src->get_properties(src_sp);
	ulong w,h;
	get_final_size(src_sp,matrix,w,h);
	index_t dst=this->allocate(w,h);
	NONS_SurfaceProperties dst_sp;
	dst->get_properties(dst_sp);
	ulong correct_x=src_sp.w,
		correct_y=src_sp.h;
	get_corrected(correct_x,correct_y,matrix);
	long long_matrix[4];
	for (int a=0;a<4;a++)
		long_matrix[a]=long(matrix[0]*65536.0);
	for (ulong y=0;y<dst_sp.h;y++){
		Uint32 *dst_pixel=(Uint32 *)(dst_sp.pixels+y*src_sp.pitch);
		bool pixels_were_copied=0;
		long src_x0=(0-correct_x)*long_matrix[0]+(y-correct_y)*long_matrix[1],
			src_y0=(0-correct_x)*long_matrix[2]+(y-correct_y)*long_matrix[3];
		for (ulong x=0;x<dst_sp.w;x++){
			long src_x=src_x0>>16,
				src_y=src_y0>>16;
			src_x0+=long_matrix[0];
			src_y0+=long_matrix[2];
			if (!(src_x<0 || src_y<0 || (ulong)src_x>=src_sp.w || (ulong)src_y>=src_sp.h)){
				const Uint32 *src_pixel=(const Uint32 *)(src_sp.pixels+src_x*4+src_y*src_sp.pitch);
				*(Uint32 *)dst_pixel=*src_pixel;
				dst_pixel++;
				pixels_were_copied=1;
				continue;
			}
			if (!pixels_were_copied){
				dst_pixel++;
				continue;
			}
			break;
		}
	}
	return dst;
}

void NONS_SurfaceManager::assign_screen(SDL_Surface *s){
	delete this->screen;
	this->screen=new ScreenSurface(s);
}

NONS_SurfaceManager::index_t NONS_SurfaceManager::get_screen(){
	return index_t(this->screen);
}

SDL_Surface *NONS_SurfaceManager::get_screen(const NONS_SurfaceManager::index_t &i){
	return (i.p)?this->screen->screen:0;
}

bool NONS_SurfaceManager::filelog_check(const std::wstring &string){
	return this->filelog->check(string);
}

void NONS_SurfaceManager::filelog_writeout(){
	this->filelog->writeOut();
}

void NONS_SurfaceManager::filelog_commit(){
	this->filelog->commit=1;
}

void NONS_SurfaceManager::use_fast_svg(bool b){
	this->fast_svg=b;
}

void NONS_SurfaceManager::set_base_scale(double x,double y){
	this->base_scale[0]=x;
	this->base_scale[1]=y;
}

NONS_SurfaceManager sm;

struct NONS_Surface_Private{
	NONS_SurfaceManager::index_t surface;
	NONS_LongRect rect;
	NONS_AnimationInfo animation;
	NONS_Surface_Private(const NONS_SurfaceManager::index_t &s):surface(s){
		if (this->surface.good()){
			NONS_SurfaceProperties sp;
			s->get_properties(sp);
			this->rect.w=sp.w;
			this->rect.h=sp.h;
		}
	}
};

NONS_ConstSurface::NONS_ConstSurface(const NONS_ConstSurface &original):data(0){
	*this=original;
}

NONS_ConstSurface &NONS_ConstSurface::operator=(const NONS_ConstSurface &original){
	this->unbind();
	if (original){
		this->data=new NONS_Surface_Private(original.data->surface);
		sm.ref(this->data->surface);
	}
	return *this;
}

NONS_ConstSurface::~NONS_ConstSurface(){
	this->unbind();
}

void NONS_ConstSurface::unbind(){
	if (!*this)
		return;
	sm.unref(this->data->surface);
	delete this->data;
	this->data=0;
}

NONS_LongRect NONS_ConstSurface::default_box(const NONS_LongRect *b) const{
	if (b)
		return *b;
	if (*this)
		return this->data->rect;
	return NONS_LongRect();
}

NONS_Surface NONS_ConstSurface::scale(double x,double y) const{
	NONS_SurfaceManager::index_t temp=sm.scale(this->data->surface,x,y);
	if (!temp.good())
		return NONS_Surface::null;
	NONS_Surface r;
	r.data=new NONS_Surface_Private(temp);
	return r;
}

NONS_Surface NONS_ConstSurface::resize(long x,long y) const{
	NONS_SurfaceManager::index_t temp=sm.resize(this->data->surface,x,y);
	if (!temp.good())
		return NONS_Surface::null;
	NONS_Surface r;
	r.data=new NONS_Surface_Private(temp);
	return r;
}

NONS_Surface NONS_ConstSurface::rotate(double alpha) const{
	NONS_SurfaceManager::index_t temp=sm.rotate(this->data->surface,alpha);
	if (!temp.good())
		return NONS_Surface::null;
	NONS_Surface r;
	r.data=new NONS_Surface_Private(temp);
	return r;
}

NONS_Surface NONS_ConstSurface::transform(double m[4]) const{
	NONS_SurfaceManager::index_t temp=sm.transform(this->data->surface,m);
	if (!temp.good())
		return NONS_Surface::null;
	NONS_Surface r;
	r.data=new NONS_Surface_Private(temp);
	return r;
}

void write_png(png_structp png,png_bytep data,png_size_t length){
	std::vector<uchar> *v=(std::vector<uchar> *)png_get_io_ptr(png);
	size_t l=v->size();
	v->resize(l+length);
	memcpy(&(*v)[l],data,length);
}

void flush_png(png_structp){}

#include <csetjmp>

bool write_png_file(std::wstring filename,const NONS_ConstSurface &s){
	bool ret=0;
	if (!s)
		return ret;
	png_structp png=0;
	png_infop info=0;
	do{
		png_structp png=png_create_write_struct(PNG_LIBPNG_VER_STRING,0,0,0);
		if (!png)
			break;
		png_infop info=png_create_info_struct(png);
		if (!info || setjmp(png_jmpbuf(png)))
			break;
		std::vector<uchar> v;
		png_set_write_fn(png,&v,write_png,flush_png);
		if (setjmp(png_jmpbuf(png)))
			break;
		NONS_ConstSurfaceProperties sp;
		s.get_properties(sp);
		png_set_IHDR(
			png,
			info,
			sp.w,
			sp.h,
			8,
			PNG_COLOR_TYPE_RGB_ALPHA,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT
		);
		png_write_info(png, info);
		if (setjmp(png_jmpbuf(png)))
			break;

		std::vector<const uchar *> pointers(sp.h);
		for (ulong a=0;a<pointers.size();a++)
			pointers[a]=sp.pixels+sp.pitch*a;
		png_write_image(png,(png_bytepp)&pointers[0]);
		if (setjmp(png_jmpbuf(png)))
			break;
		png_write_end(png,0);

		if (filename.size()<4 || tolowerCopy(filename.substr(filename.size()-4))!=L".png")
			filename.append(L".png");
		NONS_File::write(filename,&v[0],v.size());
		ret=1;
	}while (0);
	if (png && info)
		png_destroy_write_struct(&png,&info);
	else if (png)
		png_destroy_write_struct(&png,0);
	return ret;
}

void NONS_ConstSurface::save_bitmap(const std::wstring &filename) const{
	write_png_file(filename,*this);
}

void NONS_ConstSurface::get_properties(NONS_ConstSurfaceProperties &sp) const{
	if (this->good())
		this->data->surface->get_properties(sp);
}

NONS_Surface NONS_ConstSurface::clone() const{
	NONS_Surface r=this->clone_without_pixel_copy();
	if (!r)
		return r;
	NONS_SurfaceProperties dst_sp;
	NONS_ConstSurfaceProperties src_sp;
	this->get_properties(src_sp);
	r.get_properties(dst_sp);
	memcpy(dst_sp.pixels,src_sp.pixels,dst_sp.byte_count);
	return r;
}

NONS_Surface NONS_ConstSurface::clone_without_pixel_copy() const{
	return (!*this)?NONS_Surface::null:NONS_Surface(this->data->rect.w,this->data->rect.h);
}

void NONS_ConstSurface::get_optimized_updates(optim_t &dst) const{
	//TODO
}

static NONS_AnimationInfo null;

const NONS_AnimationInfo &NONS_ConstSurface::get_animation_info() const{
	if (!*this)
		return null;
	return this->data->animation;
}

const NONS_Surface NONS_Surface::null;

NONS_Surface::NONS_Surface(const NONS_CrippledSurface &original){
	this->data=0;
	if (original){
		this->data=new NONS_Surface_Private(original.data->surface);
		if (this->data->surface.good())
			sm.ref(this->data->surface);
	}
}

void NONS_Surface::assign(ulong w,ulong h){
	delete this->data;
	this->data=new NONS_Surface_Private(sm.allocate(w,h));
}

NONS_Surface &NONS_Surface::operator=(const std::wstring &name){
	this->unbind();
	NONS_SurfaceManager::index_t i=sm.load(name);
	this->data=(!i.good())?0:new NONS_Surface_Private(i);
	return *this;
}
	
void NONS_Surface::copy_pixels(const NONS_ConstSurface &src,const NONS_LongRect *dst_rect,const NONS_LongRect *src_rect) const{
	NONS_LongRect sr,
		dr;
	if (!fix_rects(dr,sr,dst_rect,src_rect,*this,src))
		return;
	NONS_ConstSurfaceProperties ssp;
	NONS_SurfaceProperties dsp;
	src.get_properties(ssp);
	this->get_properties(dsp);
	ssp.pixels+=sr.x*4+sr.y*ssp.pitch;
	dsp.pixels+=dr.x*4+dr.y*dsp.pitch;
	for (ulong y=sr.h;y;y--){
		memcpy(dsp.pixels,ssp.pixels,sr.w*4);
		ssp.pixels+=ssp.pitch;
		dsp.pixels+=dsp.pitch;
	}
}

void NONS_Surface::get_properties(NONS_SurfaceProperties &sp) const{
	if (this->good())
		this->data->surface->get_properties(sp);
}

void NONS_Surface::fill(const NONS_Color &color) const{
	if (!*this)
		return;
	this->fill(this->data->rect,color);
}

void NONS_Surface::fill(NONS_LongRect area,const NONS_Color &color) const{
	if (!*this)
		return;
	NONS_SurfaceProperties sp;
	this->get_properties(sp);
	area=area.intersect(this->data->rect);
	area.w+=area.x;
	area.h+=area.y;
	if (!(area.w*area.h))
		return;
	Uint32 c=color.to_native();
	for (ulong y=area.y;y<(ulong)area.h;y++){
		Uint32 *pixel=(Uint32 *)sp.pixels;
		pixel+=sp.w*y+area.x;
		for (ulong x=area.x;x<(ulong)area.w;x++){
			*pixel=c;
			pixel++;
		}
	}
}

void NONS_Surface::update(ulong x,ulong y,ulong w,ulong h) const{
	if (!*this)
		return;
	sm.update(this->data->surface,x,y,w,h);
}

//------------------------------------------------------------------------------
// OVER
//------------------------------------------------------------------------------

struct over_blend_parameters{
	const NONS_SurfaceProperties *dst;
	const NONS_ConstSurfaceProperties *src;
	NONS_LongRect dst_rect,
		src_rect;
	long alpha;
};

void over_blend_threaded(NONS_SurfaceProperties dst,
		NONS_LongRect dst_rect,
		NONS_ConstSurfaceProperties src,
		NONS_LongRect src_rect,
		long alpha);
void over_blend_threaded(void *parameters);

void over_blend(
		const NONS_SurfaceProperties &dst,
		const NONS_LongRect &dst_rect,
		const NONS_ConstSurfaceProperties &src,
		const NONS_LongRect &src_rect,
		long alpha){
	if (cpu_count==1 || src_rect.w*src_rect.h<5000){
		over_blend_threaded(dst,dst_rect,src,src_rect,alpha);
		return;
	}
#ifndef USE_THREAD_MANAGER
	std::vector<NONS_Thread> threads(cpu_count);
#endif
	std::vector<over_blend_parameters> parameters(cpu_count);
	ulong division=ulong(float(src_rect.h)/float(cpu_count));
	ulong total=0;
	for (ulong a=0;a<cpu_count;a++){
		over_blend_parameters &p=parameters[a];
		p.src_rect=src_rect;
		p.src_rect.y+=a*division;
		p.src_rect.h=division;
		p.dst_rect=dst_rect;
		p.dst_rect.y+=a*division;
		total+=division;
		p.src=&src;
		p.dst=&dst;
		p.alpha=alpha;
	}
	parameters.back().src_rect.h+=src_rect.h-total;
	for (ulong a=1;a<cpu_count;a++)
#ifndef USE_THREAD_MANAGER
		threads[a].call(over_blend_threaded,&parameters[a]);
#else
		threadManager.call(a-1,over_blend_threaded,&parameters[a]);
#endif
	over_blend_threaded(&parameters[0]);
#ifndef USE_THREAD_MANAGER
	for (ulong a=1;a<cpu_count;a++)
		threads[a].join();
#else
	threadManager.waitAll();
#endif
}

void over_blend_threaded(void *parameters){
	over_blend_parameters *p=(over_blend_parameters *)parameters;
	over_blend_threaded(*p->dst,p->dst_rect,*p->src,p->src_rect,p->alpha);
}

uchar integer_division_lookup[0x10000];

void over_blend_threaded(
		NONS_SurfaceProperties dst,
		NONS_LongRect dst_rect,
		NONS_ConstSurfaceProperties src,
		NONS_LongRect src_rect,
		long alpha){
	int w=src_rect.w,
		h=src_rect.h;
	src.pixels+=src.pitch*src_rect.y+4*src_rect.x;
	dst.pixels+=dst.pitch*dst_rect.y+4*dst_rect.x;

	bool negate=(alpha<0);
	if (negate)
		alpha=-alpha;
	for (int y=0;y<h;y++){
		const uchar *pos0=src.pixels;
		uchar *pos1=dst.pixels;
		for (int x=0;x<w;x++){
			long rgba0[4];
			uchar *rgba1[4];
			for (int a=0;a<4;a++){
				rgba0[a]=pos0[a];
				rgba1[a]=pos1+a;
			}

			rgba0[3]=INTEGER_MULTIPLICATION(rgba0[3],alpha);
			ulong bottom_alpha=
				*rgba1[3]=~(uchar)INTEGER_MULTIPLICATION(rgba0[3]^0xFF,*rgba1[3]^0xFF);
			ulong composite=integer_division_lookup[rgba0[3]+(bottom_alpha<<8)];
			if (composite){
				*rgba1[0]=(uchar)APPLY_ALPHA(rgba0[0],*rgba1[0],composite);
				*rgba1[1]=(uchar)APPLY_ALPHA(rgba0[1],*rgba1[1],composite);
				*rgba1[2]=(uchar)APPLY_ALPHA(rgba0[2],*rgba1[2],composite);
			}
			
			if (!(negate && rgba0[3])){
				pos0+=4;
				pos1+=4;
				continue;
			}
			for (int a=0;a<3;a++)
				*rgba1[a]=~*rgba1[a];
			pos0+=4;
			pos1+=4;
		}
		src.pixels+=src.pitch;
		dst.pixels+=dst.pitch;
	}
}

//------------------------------------------------------------------------------

void NONS_Surface::over(const NONS_ConstSurface &src,const NONS_LongRect *dst_rect,const NONS_LongRect *src_rect,long alpha) const{
	if (!*this || !src)
		return;
	NONS_LongRect sr,
		dr;
	if (!fix_rects(dr,sr,dst_rect,src_rect,*this,src))
		return;
	NONS_ConstSurfaceProperties ssp;
	NONS_SurfaceProperties dsp;
	src.get_properties(ssp);
	this->get_properties(dsp);
	over_blend(dsp,dr,ssp,sr,alpha);
}

//------------------------------------------------------------------------------
// MULTIPLY
//------------------------------------------------------------------------------

void multiply_blend_threaded(NONS_SurfaceProperties dst,
		NONS_LongRect dst_rect,
		NONS_ConstSurfaceProperties src,
		NONS_LongRect src_rect);
void multiply_blend_threaded(void *parameters);

void multiply_blend(
		const NONS_SurfaceProperties &dst,
		const NONS_LongRect &dst_rect,
		const NONS_ConstSurfaceProperties &src,
		const NONS_LongRect &src_rect){
	if (cpu_count==1 || src_rect.w*src_rect.h<5000){
		multiply_blend_threaded(dst,dst_rect,src,src_rect);
		return;
	}
#ifndef USE_THREAD_MANAGER
	std::vector<NONS_Thread> threads(cpu_count);
#endif
	std::vector<over_blend_parameters> parameters(cpu_count);
	ulong division=ulong(float(src_rect.h)/float(cpu_count));
	ulong total=0;
	for (ulong a=0;a<cpu_count;a++){
		over_blend_parameters &p=parameters[a];
		p.src_rect=src_rect;
		p.src_rect.y+=a*division;
		p.src_rect.h=division;
		p.dst_rect=dst_rect;
		p.dst_rect.y+=a*division;
		total+=division;
		p.src=&src;
		p.dst=&dst;
	}
	parameters.back().src_rect.h+=src_rect.h-total;
	for (ulong a=1;a<cpu_count;a++)
#ifndef USE_THREAD_MANAGER
		threads[a].call(multiply_blend_threaded,&parameters[a]);
#else
		threadManager.call(a-1,multiply_blend_threaded,&parameters[a]);
#endif
	multiply_blend_threaded(&parameters[0]);
#ifndef USE_THREAD_MANAGER
	for (ulong a=1;a<cpu_count;a++)
		threads[a].join();
#else
	threadManager.waitAll();
#endif
}

void multiply_blend_threaded(void *parameters){
	over_blend_parameters *p=(over_blend_parameters *)parameters;
	multiply_blend_threaded(*p->dst,p->dst_rect,*p->src,p->src_rect);
}

void multiply_blend_threaded(
		NONS_SurfaceProperties dst,
		NONS_LongRect dst_rect,
		NONS_ConstSurfaceProperties src,
		NONS_LongRect src_rect){
	int w=src_rect.w,
		h=src_rect.h;
	src.pixels+=src.pitch*src_rect.y+4*src_rect.x;
	dst.pixels+=dst.pitch*dst_rect.y+4*dst_rect.x;

	for (int y=0;y<h;y++){
		const uchar *pos0=src.pixels;
		uchar *pos1=dst.pixels;
		for (int x=0;x<w;x++){
			long rgba0[4];
			uchar *rgba1[4];
			for (int a=0;a<4;a++){
				rgba0[a]=pos0[a];
				rgba1[a]=pos1+a;
			}
			*rgba1[0]=(uchar)INTEGER_MULTIPLICATION(rgba0[0],*rgba1[0]);
			*rgba1[1]=(uchar)INTEGER_MULTIPLICATION(rgba0[1],*rgba1[1]);
			*rgba1[2]=(uchar)INTEGER_MULTIPLICATION(rgba0[2],*rgba1[2]);
			pos0+=4;
			pos1+=4;
		}
		src.pixels+=src.pitch;
		dst.pixels+=dst.pitch;
	}
}

//------------------------------------------------------------------------------

void NONS_Surface::multiply(const NONS_ConstSurface &src,const NONS_LongRect *dst_rect,const NONS_LongRect *src_rect) const{
	if (!*this || !src)
		return;
	NONS_LongRect sr,
		dr;
	if (!fix_rects(dr,sr,dst_rect,src_rect,*this,src))
		return;
	NONS_ConstSurfaceProperties ssp;
	NONS_SurfaceProperties dsp;
	src.get_properties(ssp);
	this->get_properties(dsp);
	multiply_blend(dsp,dr,ssp,sr);
}

//------------------------------------------------------------------------------
// BILINEAR INTERPOLATION
//------------------------------------------------------------------------------

struct interpolation_parameters{
	const NONS_SurfaceProperties *dst,
		*src;
	NONS_LongRect dst_rect,
		src_rect;
	double x,y;
};

#define INTERPOLATION_SIGNATURE(x)  \
	void x(                         \
		NONS_SurfaceProperties dst, \
		NONS_LongRect dst_rect,     \
		NONS_SurfaceProperties src, \
		NONS_LongRect src_rect,     \
		double x_multiplier,        \
		double y_multiplier         \
	)
#define DECLARE_INTERPOLATION_F(a)                                          \
	INTERPOLATION_SIGNATURE(a);                                             \
	void a(void *parameters){                                               \
		interpolation_parameters *p=(interpolation_parameters *)parameters; \
		a(*p->dst,p->dst_rect,*p->src,p->src_rect,p->x,p->y);               \
	}

#define NONS_Surface_DEFINE_INTERPOLATION_F(a,b)                    \
	void NONS_Surface::a(                                           \
		const NONS_Surface &src,                                    \
		const NONS_LongRect *dst_rect,                              \
		const NONS_LongRect *src_rect,                              \
		double x,                                                   \
		double y                                                    \
	){                                                              \
		this->interpolation(b,src,dst_rect,src_rect,x,y);           \
	}

NONS_Surface_DECLARE_INTERPOLATION_F_INTERNAL(NONS_Surface::,interpolation){
	NONS_LongRect sr,
		dr;
	if (!fix_rects(dr,sr,dst_rect,src_rect,*this,src))
		return;
	NONS_SurfaceProperties ssp,
		dsp;
	src.get_properties(ssp);
	this->get_properties(dsp);
#ifndef USE_THREAD_MANAGER
	std::vector<NONS_Thread> threads(cpu_count);
#endif
	std::vector<interpolation_parameters> parameters(cpu_count);
	ulong division[]={
			ulong(float(sr.h)/float(cpu_count)),
			ulong(float(dr.h)/float(cpu_count))
		},total[]={0,0};
	for (ulong a=0;a<cpu_count;a++){
		interpolation_parameters &p=parameters[a];
		p.src_rect=sr;
		p.src_rect.y+=a*division[0];
		p.src_rect.h=division[0];
		p.dst_rect=dr;
		p.dst_rect.y+=a*division[1];
		p.dst_rect.h=division[1];
		total[0]+=division[0];
		total[1]+=division[1];
		p.src=&ssp;
		p.dst=&dsp;
	}
	parameters.back().src_rect.h+=sr.h-total[0];
	parameters.back().dst_rect.h+=dr.h-total[1];
	for (ulong a=1;a<cpu_count;a++)
#ifndef USE_THREAD_MANAGER
		threads[a].call(f,&parameters[a]);
#else
		threadManager.call(a-1,f,&parameters[a]);
#endif
	over_blend_threaded(&parameters[0]);
#ifndef USE_THREAD_MANAGER
	for (ulong a=1;a<cpu_count;a++)
		threads[a].join();
#else
	threadManager.waitAll();
#endif
}

DECLARE_INTERPOLATION_F(bilinear_interpolation_threaded)
DECLARE_INTERPOLATION_F(bilinear_interpolation2_threaded)
DECLARE_INTERPOLATION_F(NN_interpolation_threaded)
NONS_Surface_DEFINE_INTERPOLATION_F(NN_interpolation,NN_interpolation_threaded)
NONS_Surface_DEFINE_INTERPOLATION_F(bilinear_interpolation,bilinear_interpolation_threaded)
NONS_Surface_DEFINE_INTERPOLATION_F(bilinear_interpolation2,bilinear_interpolation2_threaded)

INTERPOLATION_SIGNATURE(bilinear_interpolation_threaded){
	const ulong unit=1<<16;
	ulong advance_x=ulong(65536.0/x_multiplier),
		advance_y=ulong(65536.0/y_multiplier);
	src_rect.w+=src_rect.x;
	src_rect.h+=src_rect.y;
	dst_rect.w+=dst_rect.x;
	dst_rect.h+=dst_rect.y;
	long Y=dst_rect.y*advance_y;
	uchar *dst_pix=dst.pixels+dst_rect.y*dst.pitch+dst_rect.x*4;
	for (long y=dst_rect.y;y<dst_rect.h;y++){
		long y0=Y>>16;
		const uchar *src_pix=src.pixels+src.pitch*y0;
#define GET_FRACTION(x) ((((x)&(unit-1))<<16)/unit)
		ulong fraction_y=GET_FRACTION(Y),
			ifraction_y=unit-fraction_y;
		long X=dst_rect.x*advance_x;
		uchar *dst_pix0=dst_pix;
		for (long x=dst_rect.x;x<dst_rect.w;x++){
			long x0=X>>16;
			ulong fraction_x=GET_FRACTION(X),
				ifraction_x=unit-fraction_x;

			const uchar *pixel[4];
			ulong weight[4];
			pixel[0]=src_pix+x0*4;
			pixel[1]=pixel[0]+4;
			pixel[2]=pixel[0]+src.pitch;
			pixel[3]=pixel[2]+4;
#define BILINEAR_FIXED16_MULTIPLICATION(x,y) ((((x)>>1)*((y)>>1))>>14)
			weight[0]=BILINEAR_FIXED16_MULTIPLICATION(ifraction_x,ifraction_y);
			weight[1]=BILINEAR_FIXED16_MULTIPLICATION( fraction_x,ifraction_y);
			weight[2]=BILINEAR_FIXED16_MULTIPLICATION(ifraction_x, fraction_y);
			weight[3]=BILINEAR_FIXED16_MULTIPLICATION( fraction_x, fraction_y);
#define BILINEAR_SET_PIXEL(x)\
			dst_pix[x]=uchar((pixel[0][x]*weight[0]+\
			                  pixel[1][x]*weight[1]+\
			                  pixel[2][x]*weight[2]+\
			                  pixel[3][x]*weight[3])>>16)
			BILINEAR_SET_PIXEL(0);
			BILINEAR_SET_PIXEL(1);
			BILINEAR_SET_PIXEL(2);
			BILINEAR_SET_PIXEL(3);

			X+=unit;
			dst_pix+=4;
		}
		dst_pix=dst_pix0+dst.pitch;
		Y+=unit;
	}
}

INTERPOLATION_SIGNATURE(bilinear_interpolation2_threaded){
	const ulong unit=1<<16;
	ulong advance_x=ulong(65536.0/x_multiplier),
		advance_y=ulong(65536.0/y_multiplier);
	ulong area=((advance_x>>8)*advance_y)>>8;
	src_rect.w+=src_rect.x;
	src_rect.h+=src_rect.y;
	dst_rect.w+=dst_rect.x;
	dst_rect.h+=dst_rect.y;
	ulong Y0=dst_rect.y*advance_y,
		Y1=Y0+advance_y,
		X0,X1;
	uchar *dst_pix=dst.pixels+dst_rect.y*dst.pitch+dst_rect.x*4;
	for (long y=dst_rect.y;y<dst_rect.h;y++){
		X0=dst_rect.x*advance_x;
		X1=X0+advance_x;
		const uchar *src_pix=src.pixels+src.pitch*(Y0>>16);
		uchar *dst_pix0=dst_pix;
		for (long x=dst_rect.x;x<dst_rect.w;x++){
			const uchar *pixel=src_pix+(X0>>16)*4;
			ulong color[4]={0};
			for (ulong y2=Y0;y2<Y1;){
				ulong y_multiplier;
				const uchar *pixel0=pixel;
				if (Y1-y2<unit)
					y_multiplier=Y1-y2;
				else if (y2==Y0)
					y_multiplier=FLOOR(Y0)+unit-Y0;
				else
					y_multiplier=unit;
				for (ulong x2=X0;x2<X1;){
					ulong x_multiplier;
					if (X1-x2<unit)
						x_multiplier=X1-x2;
					else if (x2==X0)
						x_multiplier=FLOOR(X0)+unit-X0;
					else
						x_multiplier=unit;
					ulong compound_multiplier=((y_multiplier>>8)*x_multiplier)>>8;
					color[0]+=ulong(pixel[0])*compound_multiplier;
					color[1]+=ulong(pixel[1])*compound_multiplier;
					color[2]+=ulong(pixel[2])*compound_multiplier;
					color[3]+=ulong(pixel[3])*compound_multiplier;
					pixel+=4;
					x2=FLOOR(x2)+unit;
				}
				pixel=pixel0+src.pitch;
				y2=FLOOR(y2)+unit;
			}
			dst_pix[0]=uchar(color[0]/area);
			dst_pix[1]=uchar(color[1]/area);
			dst_pix[2]=uchar(color[2]/area);
			dst_pix[3]=uchar(color[3]/area);
			dst_pix+=4;
			X0=X1;
			X1+=advance_x;
		}
		dst_pix=dst_pix0+dst.pitch;
		Y0=Y1;
		Y1+=advance_y;
	}
}

//------------------------------------------------------------------------------
// NEAREST NEIGHBOR INTERPOLATION
//------------------------------------------------------------------------------

INTERPOLATION_SIGNATURE(NN_interpolation_threaded){
	const ulong unit=1<<16;
	ulong advance_x=ulong(65536.0/x_multiplier),
		advance_y=ulong(65536.0/y_multiplier);
	src_rect.w+=src_rect.x;
	src_rect.h+=src_rect.y;
	dst_rect.w+=dst_rect.x;
	dst_rect.h+=dst_rect.y;
	long Y=dst_rect.y*advance_y;
	uchar *dst_pix=dst.pixels+dst_rect.y*dst.pitch+dst_rect.x*4;
	for (long y=dst_rect.y;y<dst_rect.h;y++){
		const uchar *src_pix=src.pixels+src.pitch*(Y>>16);
		long X=dst_rect.x*advance_x;
		uchar *dst_pix0=dst_pix;
		for (long x=dst_rect.x;x<dst_rect.w;x++){
			*(Uint32 *)dst_pix=*(const Uint32 *)(src_pix+(X>>16)*4);
			X+=unit;
			dst_pix+=4;
		}
		dst_pix=dst_pix0+dst.pitch;
		Y+=unit;
	}
}

//------------------------------------------------------------------------------

SDL_Surface *NONS_Surface::get_SDL_screen(){
	if (!*this)
		return 0;
	return sm.get_screen(this->data->surface);
}

NONS_Surface NONS_Surface::assign_screen(SDL_Surface *s){
	sm.assign_screen(s);
	return get_screen();
}

NONS_Surface NONS_Surface::get_screen(){
	NONS_Surface r;
	r.data=new NONS_Surface_Private(sm.get_screen());
	sm.ref(r.data->surface);
	return r;
}

void NONS_Surface::init_loader(){
	sm.init();
}

bool NONS_Surface::filelog_check(const std::wstring &string){
	return sm.filelog_check(string);
}

void NONS_Surface::filelog_writeout(){
	sm.filelog_writeout();
}

void NONS_Surface::filelog_commit(){
	sm.filelog_commit();
}

void NONS_Surface::use_fast_svg(bool b){
	sm.use_fast_svg(b);
}

void NONS_Surface::set_base_scale(double x,double y){
	sm.set_base_scale(x,y);
}

#define NONS_Surface_DEFINE_RELATIONAL_OP(type,op)       \
	bool type::operator op(const type &b) const{         \
		if (!*this || !b)                                \
			return 0;                                    \
		return *this->data->surface op *b.data->surface; \
	}
OVERLOAD_RELATIONAL_OPERATORS2(NONS_Surface,NONS_Surface_DEFINE_RELATIONAL_OP)
OVERLOAD_RELATIONAL_OPERATORS2(NONS_CrippledSurface,NONS_Surface_DEFINE_RELATIONAL_OP)

NONS_SurfaceManager::Surface::Surface(ulong w,ulong h){
	this->set_id();
	size_t n=w*h*4+1;
	this->data=new uchar[n];
	this->w=w;
	this->h=h;
	this->ref_count=1;
	memset(this->data,0,n);
}

NONS_SurfaceManager::Surface::~Surface(){
	delete[] this->data;
}

long NONS_SurfaceManager::Surface::ref(){
	NONS_MutexLocker ml(this->surface_mutex);
	return ++this->ref_count;
}

long NONS_SurfaceManager::Surface::unref(){
	NONS_MutexLocker ml(this->surface_mutex);
	return --this->ref_count;
}

NONS_SurfaceManager::ScreenSurface::ScreenSurface(SDL_Surface *s):Surface(),screen(s){
	this->w=s->w;
	this->h=s->h;
	this->data=(uchar *)s->pixels;
	this->ref_count=LONG_MAX>>1;
}

long NONS_SurfaceManager::ScreenSurface::ref(){
	this->screen_mutex.lock();
	long r=++this->ref_count;
	SDL_LockSurface(this->screen);
	return r;
}

long NONS_SurfaceManager::ScreenSurface::unref(){
	SDL_UnlockSurface(this->screen);
	long r=--this->ref_count;
	this->screen_mutex.unlock();
	return r;
}

NONS_CrippledSurface::NONS_CrippledSurface(const NONS_Surface &original){
	this->data=new NONS_Surface_Private(original.data->surface);
	this->inner=0;
}

const NONS_CrippledSurface &NONS_CrippledSurface::operator=(const NONS_CrippledSurface &original){
	this->unbind();
	if (original)
		this->data=new NONS_Surface_Private(original.data->surface);
	if (original.inner)
		this->inner=new NONS_Surface(*original.inner);
	return *this;
}

NONS_CrippledSurface::~NONS_CrippledSurface(){
	delete this->data;
	delete this->inner;
}

void NONS_CrippledSurface::get_dimensions(ulong &w,ulong &h){
	w=this->data->rect.w;
	h=this->data->rect.h;
}

void NONS_CrippledSurface::strong_bind(){
	delete this->inner;
	this->inner=new NONS_Surface(*this);
}

void NONS_CrippledSurface::unbind(){
	delete this->data;
	delete this->inner;
	this->data=0;
	this->inner=0;
}
