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

#ifndef NONS_Surface_H
#define NONS_Surface_H
#include "Common.h"
#include "Functions.h"
#include <cmath>

#define OVERLOAD_RELATIONAL_OPERATORS(macro)\
	macro(==)\
	macro(!=)\
	macro(<)\
	macro(<=)\
	macro(>=)\
	macro(>)
#define OVERLOAD_RELATIONAL_OPERATORS2(extra,macro)\
	macro(extra,==)\
	macro(extra,!=)\
	macro(extra,<)\
	macro(extra,<=)\
	macro(extra,>=)\
	macro(extra,>)

template <typename T>
struct NONS_SurfaceProperties_basic{
	T *pixels;
	ulong w,h,
		pitch,
		byte_count,
		pixel_count,
		frames;
	uchar offsets[4];
	template <typename T2>
	operator NONS_SurfaceProperties_basic<T2>() const{
		NONS_SurfaceProperties_basic<T2> r;
		r.pixels=this->pixels;
		r.w=this->w;
		r.h=this->h;
		r.pitch=this->pitch;
		r.byte_count=this->byte_count;
		r.pixel_count=this->pixel_count;
		r.offsets[0]=this->offsets[0];
		r.offsets[1]=this->offsets[1];
		r.offsets[2]=this->offsets[2];
		r.offsets[3]=this->offsets[3];
		r.frames=this->frames;
		return r;
	}
	template <typename T2>
	bool same_format(const NONS_SurfaceProperties_basic<T2> &b){
		return
			this->offsets[0]==b.offsets[0] &&
			this->offsets[1]==b.offsets[1] &&
			this->offsets[2]==b.offsets[2] &&
			this->offsets[3]==b.offsets[3];
	}
};
typedef NONS_SurfaceProperties_basic<uchar> NONS_SurfaceProperties;
typedef NONS_SurfaceProperties_basic<const uchar> NONS_ConstSurfaceProperties;

void over_blend(
	const NONS_SurfaceProperties &dst,
	const NONS_LongRect &dst_rect,
	const NONS_ConstSurfaceProperties &src,
	const NONS_LongRect &src_rect,
	long alpha
);

class NONS_CrippledSurface;
struct NONS_Surface_Private;

struct NONS_Color{
	Uint8 rgba[4];
	NONS_Color(Uint8 r,Uint8 g,Uint8 b,Uint8 a=0xFF){
		this->rgba[0]=r;
		this->rgba[1]=g;
		this->rgba[2]=b;
		this->rgba[3]=a;
	}
	NONS_Color(Uint32 a=0){
		*this=a;
	}
	const NONS_Color &operator=(Uint32 a){
		this->rgba[0]=(a&0xFF0000)>>16;
		this->rgba[1]=(a&0xFF00)>>8;
		this->rgba[2]=a&0xFF;
		this->rgba[3]=0xFF;
		return *this;
	}
	Uint32 to_rgb(){
		Uint32 r=
			(this->rgba[0]<<16)|
			(this->rgba[1]<<8)|
			this->rgba[2];
		return r;
	}
	Uint32 to_native(uchar *format) const{
		Uint8 r[]={
			this->rgba[format[0]],
			this->rgba[format[1]],
			this->rgba[format[2]],
			this->rgba[format[3]]
		};
		return *(Uint32 *)r;
	}
	bool operator==(const NONS_Color &b) const{
		uchar f[]={0,1,2,3};
		return this->to_native(f)==b.to_native(f);
	}
	bool operator!=(const NONS_Color &b) const{
		return !(*this==b);
	}
	static NONS_Color white,
		black,
		black_transparent,
		red,
		green,
		blue;
};

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

	NONS_AnimationInfo()
		:animation_length(1),
		animation_time_offset(0),
		valid(1){}
	NONS_AnimationInfo(const std::wstring &image_string);
	NONS_AnimationInfo(const NONS_AnimationInfo &b);
	NONS_AnimationInfo &operator=(const NONS_AnimationInfo &b);
	void parse(const std::wstring &image_string);
	void resetAnimation();
	long advanceAnimation(ulong msecs);
	long getCurrentAnimationFrame() const;
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

class NONS_Matrix{
	double matrix[4];
public:
	NONS_Matrix(){
		this->matrix[0]=
		this->matrix[1]=
		this->matrix[2]=
		this->matrix[3]=0.0;
	}
	NONS_Matrix(double a,double b,double c,double d){
		this->matrix[0]=a;
		this->matrix[1]=b;
		this->matrix[2]=c;
		this->matrix[3]=d;
	}
	double determinant() const{
		return this->matrix[0]*this->matrix[3]-this->matrix[1]*this->matrix[2];
	}
	NONS_Matrix operator!() const{
		double a=1.0/this->determinant();
		return NONS_Matrix(
			a*this->matrix[3],
			a*-this->matrix[1],
			a*-this->matrix[2],
			a*this->matrix[0]
		);
	}
	NONS_Matrix operator*(const NONS_Matrix &m) const{
		return NONS_Matrix(
			this->matrix[0]*m.matrix[0]+this->matrix[1]*m.matrix[2],
			this->matrix[0]*m.matrix[1]+this->matrix[1]*m.matrix[3],
			this->matrix[2]*m.matrix[0]+this->matrix[3]*m.matrix[2],
			this->matrix[2]*m.matrix[1]+this->matrix[3]*m.matrix[3]
		);
	}
	const double &operator[](unsigned i)const{ return this->matrix[i]; }
	static NONS_Matrix rotation(double alpha){
		return NONS_Matrix(cos(alpha),-sin(alpha),sin(alpha),cos(alpha));
	}
	static NONS_Matrix scale(double x,double y){
		return NONS_Matrix(x,0,0,y);
	}
	static NONS_Matrix shear(double x,double y){
		return NONS_Matrix(1,x,y,1);
	}
};

class NONS_Surface;
typedef std::map<std::pair<ulong,ulong>,NONS_LongRect> optim_t;

class NONS_DECLSPEC NONS_ConstSurface{
protected:
	NONS_Surface_Private *data;
	friend class NONS_CrippledSurface;
	friend class NONS_Surface;
public:
	//Create a null surface.
	NONS_ConstSurface():data(0){}
	//Make a shallow copy of a surface. The pointed surface's reference count
	//gets incremented.
	NONS_ConstSurface(const NONS_ConstSurface &original);
	NONS_ConstSurface &operator=(const NONS_ConstSurface &);
	virtual ~NONS_ConstSurface();
	//Returns whether the object points to something valid.
	bool good() const{ return !!this->data; }
	operator bool() const{ return this->good(); }
	//Invalidates the object. The pointed surface's reference count gets
	//decremented.
	void unbind();
	//If the pointer is !0, returns a copy of the object the pointer points to.
	//Otherwise, returns a copy of the surface's rectangle.
	NONS_LongRect default_box(const NONS_LongRect *) const;
	//Call to perform pixel-wise read operations on the surface.
	void get_properties(NONS_ConstSurfaceProperties &sp) const;
	//Returns a copy of the surface's rectangle.
	NONS_LongRect clip_rect() const{ return this->default_box(0); }
	//Returns an object that points to a deep copy of this object's surface.
	//The new surface has the same size and pixel data as the original.
	NONS_Surface clone() const;
	//Same as clone(), but doesn't copy the pixel data.
	NONS_Surface clone_without_pixel_copy() const;
	//Copies the surface while scaling it.
	NONS_Surface scale(double x,double y) const;
	//Copies the surface while resizing it.
	NONS_Surface resize(long x,long y) const;
	//Copies the surface while rotating it.
	NONS_Surface rotate(double alpha) const;
	//Copies the surface while applying a linear transformation to it.
	NONS_Surface transform(const NONS_Matrix &m,bool fast=0) const;
	void save_bitmap(const std::wstring &filename,bool fill_alpha=0) const;
	void get_optimized_updates(optim_t &dst) const;
};

class NONS_DECLSPEC NONS_Surface:public NONS_ConstSurface{
	typedef void (*interpolation_f)(NONS_SurfaceProperties,NONS_Rect,NONS_SurfaceProperties,NONS_Rect,double,double);
	NONS_Surface(int){}
public:
	static const NONS_Surface null;
	NONS_Surface(){}
	NONS_Surface(const NONS_Surface &a){
		this->data=0;
		*this=a;
	}
	NONS_Surface(const NONS_CrippledSurface &original);
	NONS_Surface(ulong w,ulong h){
		this->data=0;
		this->assign(w,h);
	}
	NONS_Surface(const std::wstring &a){
		this->data=0;
		*this=a;
	}
	void assign(ulong w,ulong h);
	NONS_Surface &operator=(const std::wstring &name);
	void over(const NONS_ConstSurface &src,const NONS_LongRect *dst_rect=0,const NONS_LongRect *src_rect=0);
	void over_with_alpha(const NONS_ConstSurface &src,const NONS_LongRect *dst_rect=0,const NONS_LongRect *src_rect=0,long alpha=255);
	void over_frame(const NONS_ConstSurface &src,ulong frame,const NONS_LongRect *dst_rect=0,const NONS_LongRect *src_rect=0);
	void over_frame_with_alpha(const NONS_ConstSurface &src,ulong frame,const NONS_LongRect *dst_rect=0,const NONS_LongRect *src_rect=0,long alpha=255);
	void multiply(const NONS_ConstSurface &src,const NONS_LongRect *dst_rect=0,const NONS_LongRect *src_rect=0);
	void multiply_frame(const NONS_ConstSurface &src,ulong frame,const NONS_LongRect *dst_rect=0,const NONS_LongRect *src_rect=0);
	void copy_pixels(const NONS_ConstSurface &src,const NONS_LongRect *dst_rect=0,const NONS_LongRect *src_rect=0);
	void copy_pixels_frame(const NONS_ConstSurface &src,ulong frame,const NONS_LongRect *dst_rect=0,const NONS_LongRect *src_rect=0);
	void get_properties(NONS_SurfaceProperties &sp) const;
	void fill(const NONS_Color &color);
	void fill(const NONS_LongRect area,const NONS_Color &color);
	void update(ulong x=0,ulong y=0,ulong w=0,ulong h=0) const;

#define NONS_Surface_DECLARE_RELATIONAL_OP(type,op) bool operator op(const type &b) const;
	OVERLOAD_RELATIONAL_OPERATORS2(NONS_Surface,NONS_Surface_DECLARE_RELATIONAL_OP)

#define NONS_Surface_DECLARE_INTERPOLATION_F(name) \
	void name(                                     \
		NONS_Surface src,                   \
		NONS_Rect dst_rect,                 \
		NONS_Rect src_rect,                 \
		double x,                                  \
		double y                                   \
	)
#define NONS_Surface_DECLARE_INTERPOLATION_F_INTERNAL(extra,name) \
	void extra name(                                              \
		interpolation_f f,                                        \
		const NONS_Surface &src,                                  \
		const NONS_Rect &dst_rect,                                \
		const NONS_Rect &src_rect,                                \
		double x,                                                 \
		double y                                                  \
	)
	NONS_Surface_DECLARE_INTERPOLATION_F_INTERNAL(,interpolation);
	NONS_Surface_DECLARE_INTERPOLATION_F(NN_interpolation);
	NONS_Surface_DECLARE_INTERPOLATION_F(bilinear_interpolation);
	NONS_Surface_DECLARE_INTERPOLATION_F(bilinear_interpolation2);
	typedef void (NONS_Surface::*public_interpolation_f)(NONS_Surface,NONS_Rect,NONS_Rect,double,double);

	void make_critical(ulong max_copies);

	//SDL-related:
	SDL_Surface *get_SDL_screen() const;
	static NONS_Surface assign_screen(SDL_Surface *,bool get_screen=1);
	static NONS_Surface get_screen();
	static void init_loader();
	static bool filelog_check(const std::wstring &string);
	static void filelog_writeout();
	static void filelog_commit();
	static void use_fast_svg(bool);
	static void set_base_scale(double x,double y);
};

class NONS_CrippledSurface{
	NONS_Surface_Private *data;
	friend class NONS_Surface;
	NONS_Surface *inner;
public:
	NONS_CrippledSurface():data(0),inner(0){}
	NONS_CrippledSurface(const NONS_Surface &original);
	NONS_CrippledSurface(const NONS_CrippledSurface &original):data(0),inner(0){ *this=original; }
	const NONS_CrippledSurface &operator=(const NONS_CrippledSurface &);
	~NONS_CrippledSurface();
	void get_dimensions(ulong &w,ulong &h);
	bool good() const{ return !!this->data; }
	operator bool() const{ return this->good(); }
	template <typename T>
	NONS_BasicRect<T> get_dimensions(){
		ulong a,b;
		this->get_dimensions(a,b);
		return NONS_BasicRect<T>(0,0,(T)a,(T)b);
	}
	NONS_Surface get_surface() const{ return (this->inner)?*this->inner:NONS_Surface(*this); }
	void strong_bind();
	void unbind();
	OVERLOAD_RELATIONAL_OPERATORS2(NONS_CrippledSurface,NONS_Surface_DECLARE_RELATIONAL_OP)

	static void copy_surface(NONS_CrippledSurface &dst,const NONS_ConstSurface &src){
		NONS_Surface s=src.clone();
		dst=s;
		dst.strong_bind();
	}
};

template<typename T>
bool fix_rects(
		NONS_BasicRect<T> &dst1,
		NONS_BasicRect<T> &src1,
		const NONS_BasicRect<T> *dst0,
		const NONS_BasicRect<T> *src0,
		const NONS_ConstSurface &dst,
		const NONS_ConstSurface &src){
	NONS_BasicRect<T> dst_rect=NONS_BasicRect<T>(dst.default_box(0)),
		src_rect=NONS_BasicRect<T>(src.default_box(0));
	dst1=NONS_BasicRect<T>(dst.default_box(dst0));
	src1=NONS_BasicRect<T>(src.default_box(src0));
	src1=src1.intersect(src_rect);
	if (src1.w<=0 || src1.h<=0)
		return 0;
	dst1.w=src1.w;
	dst1.h=src1.h;
	dst1=dst1.intersect(dst_rect);
	if (dst1.w<=0 || dst1.h<=0)
		return 0;
	src1.w=dst1.w;
	src1.h=dst1.h;
	return 1;
}

//#define _APPLY_ALPHA(c0,c1,a) (INTEGER_MULTIPLICATION((a)^0xFF,(c1))+INTEGER_MULTIPLICATION((a),(c0)))
#define _APPLY_ALPHA(c0,c1,a) ((((a)^0xFF)*(c1)+(a)*(c0))/255)
#if !defined _DEBUG
#define APPLY_ALPHA _APPLY_ALPHA
#else
inline uchar APPLY_ALPHA(ulong c0,ulong c1,ulong a){
	return (uchar)_APPLY_ALPHA(c0,c1,a);
}
#endif
#endif
