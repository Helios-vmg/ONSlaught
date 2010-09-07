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

struct NONS_SurfaceProperties{
	uchar *pixels;
	ulong w,h,
		pitch,
		byte_count,
		pixel_count;
};

void over_blend(
	const NONS_SurfaceProperties &dst,
	const NONS_LongRect &dst_rect,
	const NONS_SurfaceProperties &src,
	const NONS_LongRect &src_rect,
	long alpha
);

class NONS_CrippledSurface;
struct NONS_Surface_Private;

class NONS_Surface{
	NONS_Surface_Private *data;
	friend class NONS_CrippledSurface;
public:
	NONS_Surface():data(0){}
	NONS_Surface(const NONS_CrippledSurface &);
	NONS_Surface(ulong w,ulong h);
	NONS_Surface(const std::wstring &name);
	NONS_Surface(const NONS_Surface &,double scalex=1.0,double scaley=1.0,double rotation=0.0);
	NONS_Surface(const NONS_Surface &,double transformation_matrix[4]);
	~NONS_Surface();
	void assign(ulong x,ulong y);
	const NONS_Surface &operator=(const NONS_Surface &);
	const NONS_Surface &operator=(const std::wstring &name);
	void unbind();
	void over(const NONS_Surface &src,const NONS_LongRect *dst_rect=0,const NONS_LongRect *src_rect=0,long alpha=255);
	void copy_pixels(const NONS_Surface &src,const NONS_LongRect *dst_rect=0,const NONS_LongRect *src_rect=0);
	NONS_LongRect default_box(const NONS_LongRect *) const;
	void scale(double x,double y);
	void rotate(double alpha);
	void transform(double transformation_matrix[4]);
	void get_properties(NONS_SurfaceProperties &sp) const;
	NONS_Surface clone() const;
	void update();
	SDL_Surface *get_SDL_screen();
	bool good() const{ return !!this->data; }
#define NONS_Surface_DECLARE_RELATIONAL_OP(type,op) bool operator op(const type &b) const;
	OVERLOAD_RELATIONAL_OPERATORS2(NONS_Surface,NONS_Surface_DECLARE_RELATIONAL_OP)

#define NONS_Surface_DECLARE_INTERPOLATION_F(x) \
	void x(                                     \
		const NONS_Surface &src,                \
		const NONS_LongRect *dst_rect=0,        \
		const NONS_LongRect *src_rect=0         \
	)

#define NONS_Surface_DECLARE_INTERPOLATION_F_INTERNAL(extra,x) \
	void extra x(                                              \
		interpolation_f f,                                     \
		const NONS_Surface &src,                               \
		const NONS_LongRect *dst_rect,                         \
		const NONS_LongRect *src_rect                          \
	)
	typedef void(*interpolation_f)(void*);
	NONS_Surface_DECLARE_INTERPOLATION_F_INTERNAL(,bilinear_interpolation);
	NONS_Surface_DECLARE_INTERPOLATION_F(bilinear_interpolation);
	NONS_Surface_DECLARE_INTERPOLATION_F(bilinear_interpolation2);
	static NONS_Surface assign_screen(SDL_Surface *);
	static NONS_Surface get_screen();
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
		return NONS_BasicRect<T>(0,0,a,b);
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
		const NONS_Surface &dst,
		const NONS_Surface &src){
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
#endif
