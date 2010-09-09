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
		pixel_count;
	NONS_SurfaceProperties_basic<T>(){}
	template <typename T2>
	operator NONS_SurfaceProperties_basic<T2>() const{
		NONS_SurfaceProperties_basic<T2> r;
		r.pixels=this->pixels;
		r.w=this->w;
		r.h=this->h;
		r.pitch=this->pitch;
		r.byte_count=this->byte_count;
		r.pixel_count=this->pixel_count;
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
		for (int a=0;a<4;a++){
			this->rgba[a]=a&0xFF;
			a>>=8;
		}
		return *this;
	}
	Uint32 to_u32(){
		Uint32 r=0;
		for (int a=0;a<4;a++){
			r<<=8;
			r|=this->rgba[3-a];
		}
		return r;
	}
};

class NONS_Surface;
typedef std::map<std::pair<ulong,ulong>,SDL_Rect> optim_t;

class NONS_ConstSurface{
protected:
	NONS_Surface_Private *data;
	friend class NONS_CrippledSurface;
public:
	//Create a null surface.
	NONS_ConstSurface():data(0){}
	//Make a shallow copy of a surface. The pointed surface's reference count
	//gets incremented.
	NONS_ConstSurface(const NONS_ConstSurface &original);
	NONS_Surface &operator=(const NONS_ConstSurface &);
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
	//Copies the surface while rotating it.
	NONS_Surface rotate(double alpha) const;
	//Copies the surface while applying a linear transformation to it.
	NONS_Surface transform(double transformation_matrix[4]) const;
	void get_optimized_updates(optim_t &dst) const;
};

class NONS_Surface:public NONS_ConstSurface{
	typedef void(*interpolation_f)(void*);
public:
	NONS_Surface(){}
	NONS_Surface(const NONS_Surface &a){
		this->data=0;
		*this=a;
	}
	NONS_Surface(const NONS_CrippledSurface &);
	NONS_Surface(ulong w,ulong h);
	NONS_Surface(const std::wstring &a){
		this->data=0;
		*this=a;
	}
	void assign(ulong w,ulong h);
	NONS_Surface &operator=(const NONS_Surface &);
	NONS_Surface &operator=(const std::wstring &name);
	void over(const NONS_ConstSurface &src,const NONS_LongRect *dst_rect=0,const NONS_LongRect *src_rect=0,long alpha=255) const;
	void multiply(const NONS_ConstSurface &src,const NONS_LongRect *dst_rect=0,const NONS_LongRect *src_rect=0) const;
	void copy_pixels(const NONS_ConstSurface &src,const NONS_LongRect *dst_rect=0,const NONS_LongRect *src_rect=0) const;
	void get_properties(NONS_SurfaceProperties &sp) const;
	void fill(const NONS_Color &color) const;
	void fill(const NONS_LongRect &area,const NONS_Color &color) const;
	void update(ulong x=0,ulong y=0,ulong w=0,ulong h=0) const;

#define NONS_Surface_DECLARE_RELATIONAL_OP(type,op) bool operator op(const type &b) const;
	OVERLOAD_RELATIONAL_OPERATORS2(NONS_Surface,NONS_Surface_DECLARE_RELATIONAL_OP)

#define NONS_Surface_DECLARE_INTERPOLATION_F(x) \
	void x(                                     \
		const NONS_Surface &src,                \
		const NONS_LongRect *dst_rect,          \
		const NONS_LongRect *src_rect,          \
		double x,                               \
		double y                                \
	)
#define NONS_Surface_DECLARE_INTERPOLATION_F_INTERNAL(extra,x) \
	void extra x(                                              \
		interpolation_f f,                                     \
		const NONS_Surface &src,                               \
		const NONS_LongRect *dst_rect,                         \
		const NONS_LongRect *src_rect,                         \
		double x,                                              \
		double y                                               \
	) const
	NONS_Surface_DECLARE_INTERPOLATION_F_INTERNAL(,interpolation);
	NONS_Surface_DECLARE_INTERPOLATION_F(NN_interpolation);
	NONS_Surface_DECLARE_INTERPOLATION_F(bilinear_interpolation);
	NONS_Surface_DECLARE_INTERPOLATION_F(bilinear_interpolation2);
	typedef void (NONS_Surface::*public_interpolation_f)(const NONS_Surface &,const NONS_LongRect *,const NONS_LongRect *,double,double);

	//SDL-related:
	SDL_Surface *get_SDL_screen();
	static NONS_Surface assign_screen(SDL_Surface *);
	static NONS_Surface get_screen();
	static bool filelog_check(const std::wstring &string);
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

	static void copy_surface(NONS_CrippledSurface &dst,const NONS_Surface &src){
		NONS_Surface s=src;
		s=s.clone();
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
	dst1.x=src1.x;
	dst1.y=src1.y;
	dst1=dst1.intersect(dst_rect);
	if (dst1.w<=0 || dst1.h<=0)
		return 0;
	src1.w=dst1.w;
	src1.h=dst1.h;
	return 1;
}

//#define _APPLY_ALPHA(c0,c1,a) (INTEGER_MULTIPLICATION((a)^0xFF,(c1))+INTEGER_MULTIPLICATION((a),(c0)))
#define _APPLY_ALPHA(c0,c1,a) ((((a)^0xFF)*(c1)+(a)*(c0))/255)
#if defined _DEBUG
#define APPLY_ALPHA _APPLY_ALPHA
#else
inline uchar APPLY_ALPHA(ulong c0,ulong c1,ulong a){
	return (uchar)_APPLY_ALPHA(c0,c1,a);
}
#endif
#endif
