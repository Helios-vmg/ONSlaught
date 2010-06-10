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

#define USE_ACCURATE_MULTIPLICATION
#include "Functions.h"
#include "ThreadManager.h"
#include "IOFunctions.h"
#include "enums.h"
#include <bzlib.h>

#include <cassert>

//(Parallelized surface function)
struct PSF_parameters{
	SDL_Surface *src;
	SDL_Rect *srcRect;
	SDL_Surface *dst;
	SDL_Rect *dstRect;
	manualBlitAlpha_t alpha;
	SDL_Color color;
};

bool getbit(void *arr,ulong *byteoffset,uchar *bitoffset){
	uchar *array=(uchar *)arr;
	bool res=(array[*byteoffset]>>(7-*bitoffset))&1;
	(*bitoffset)++;
	if (*bitoffset>7){
		(*byteoffset)++;
		*bitoffset=0;
	}
	return res;
}

ulong getbits(void *arr,uchar bits,ulong *byteoffset,uchar *bitoffset){
	uchar *array=(uchar *)arr;
	ulong res=0;
	if (bits>sizeof(ulong)*8)
		bits=sizeof(ulong)*8;
	for (;bits>0;bits--){
		res<<=1;
		res|=(ulong)getbit(array,byteoffset,bitoffset);
	}
	return res;
}

Uint32 secondsSince1970(){
	return (Uint32)time(0);
}

void manualBlit_threaded(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect,manualBlitAlpha_t alpha);
void manualBlit_threaded(void *parameters);

DECLSPEC void manualBlit_unthreaded(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect,manualBlitAlpha_t alpha){
	if (!src || !dst || src->format->BitsPerPixel<24 ||dst->format->BitsPerPixel<24 || !alpha)
		return;
	SDL_Rect srcRect0,dstRect0;
	if (!srcRect){
		srcRect0.x=0;
		srcRect0.y=0;
		srcRect0.w=src->w;
		srcRect0.h=src->h;
	}else
		srcRect0=*srcRect;
	if (!dstRect){
		dstRect0.x=0;
		dstRect0.y=0;
		dstRect0.w=dst->w;
		dstRect0.h=dst->h;
	}else
		dstRect0=*dstRect;

	if (srcRect0.x<0){
		if (srcRect0.w<=-srcRect0.x)
			return;
		srcRect0.w+=srcRect0.x;
		srcRect0.x=0;
	}else if (srcRect0.x>=src->w)
		return;
	if (srcRect0.y<0){
		if (srcRect0.h<=-srcRect0.y)
			return;
		srcRect0.h+=srcRect0.y;
		srcRect0.y=0;
	}else if (srcRect0.y>=src->h)
		return;
	if (srcRect0.x+srcRect0.w>src->w)
		srcRect0.w-=srcRect0.x+srcRect0.w-src->w;
	if (srcRect0.y+srcRect0.h>src->h)
		srcRect0.h-=srcRect0.y+srcRect0.h-src->h;
	if (dstRect0.x+srcRect0.w>dst->w)
		srcRect0.w-=dstRect0.x+srcRect0.w-dst->w;
	if (dstRect0.y+srcRect0.h>dst->h)
		srcRect0.h-=dstRect0.y+srcRect0.h-dst->h;

	if (dstRect0.x<0){
		srcRect0.x=-dstRect0.x;
		srcRect0.w+=dstRect0.x;
		dstRect0.x=0;
	}else if (dstRect0.x>=dst->w)
		return;
	if (dstRect0.y<0){
		srcRect0.y=-dstRect0.y;
		srcRect0.h+=dstRect0.y;
		dstRect0.y=0;
	}else if (dstRect0.y>=dst->h)
		return;

	if (srcRect0.w<=0 || srcRect0.h<=0)
		return;


	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	manualBlit_threaded(src,&srcRect0,dst,&dstRect0,alpha);
	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);
}

DECLSPEC void manualBlit(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect,manualBlitAlpha_t alpha){
	if (!src || !dst || src->format->BitsPerPixel<24 ||dst->format->BitsPerPixel<24 || !alpha)
		return;
	SDL_Rect srcRect0,dstRect0;
	if (!srcRect){
		srcRect0.x=0;
		srcRect0.y=0;
		srcRect0.w=src->w;
		srcRect0.h=src->h;
	}else
		srcRect0=*srcRect;
	if (!dstRect){
		dstRect0.x=0;
		dstRect0.y=0;
		dstRect0.w=dst->w;
		dstRect0.h=dst->h;
	}else
		dstRect0=*dstRect;

	if (srcRect0.x<0){
		if (srcRect0.w<=-srcRect0.x)
			return;
		srcRect0.w+=srcRect0.x;
		srcRect0.x=0;
	}else if (srcRect0.x>=src->w)
		return;
	if (srcRect0.y<0){
		if (srcRect0.h<=-srcRect0.y)
			return;
		srcRect0.h+=srcRect0.y;
		srcRect0.y=0;
	}else if (srcRect0.y>=src->h)
		return;
	if (srcRect0.x+srcRect0.w>src->w)
		srcRect0.w-=srcRect0.x+srcRect0.w-src->w;
	if (srcRect0.y+srcRect0.h>src->h)
		srcRect0.h-=srcRect0.y+srcRect0.h-src->h;
	if (dstRect0.x+srcRect0.w>dst->w)
		srcRect0.w-=dstRect0.x+srcRect0.w-dst->w;
	if (dstRect0.y+srcRect0.h>dst->h)
		srcRect0.h-=dstRect0.y+srcRect0.h-dst->h;

	if (dstRect0.x<0){
		srcRect0.x=-dstRect0.x;
		srcRect0.w+=dstRect0.x;
		dstRect0.x=0;
	}else if (dstRect0.x>=dst->w)
		return;
	if (dstRect0.y<0){
		srcRect0.y=-dstRect0.y;
		srcRect0.h+=dstRect0.y;
		dstRect0.y=0;
	}else if (dstRect0.y>=dst->h)
		return;

	if (srcRect0.w<=0 || srcRect0.h<=0)
		return;


	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	if (cpu_count==1 || srcRect0.w*srcRect0.h<5000){
		manualBlit_threaded(src,&srcRect0,dst,&dstRect0,alpha);
		SDL_UnlockSurface(dst);
		SDL_UnlockSurface(src);
		return;
	}
#ifndef USE_THREAD_MANAGER
	NONS_Thread *threads=new NONS_Thread[cpu_count];
#endif
	SDL_Rect *rects0=new SDL_Rect[cpu_count];
	SDL_Rect *rects1=new SDL_Rect[cpu_count];
	PSF_parameters *parameters=new PSF_parameters[cpu_count];
	ulong division=ulong(float(srcRect0.h)/float(cpu_count));
	ulong total=0;
	for (ushort a=0;a<cpu_count;a++){
		rects0[a]=srcRect0;
		rects1[a]=dstRect0;
		rects0[a].y+=Sint16(a*division);
		rects0[a].h=Sint16(division);
		rects1[a].y+=Sint16(a*division);
		total+=rects0[a].h;
		parameters[a].src=src;
		parameters[a].srcRect=rects0+a;
		parameters[a].dst=dst;
		parameters[a].dstRect=rects1+a;
		parameters[a].alpha=alpha;
	}
	rects0[cpu_count-1].h+=Uint16(srcRect0.h-total);
	//ulong t0=SDL_GetTicks();
	for (ushort a=1;a<cpu_count;a++)
#ifndef USE_THREAD_MANAGER
		threads[a].call(manualBlit_threaded,parameters+a);
#else
		threadManager.call(a-1,manualBlit_threaded,parameters+a);
#endif
	manualBlit_threaded(parameters);
#ifndef USE_THREAD_MANAGER
	for (ushort a=1;a<cpu_count;a++)
		threads[a].join();
#else
	threadManager.waitAll();
#endif
	//ulong t1=SDL_GetTicks();
	//o_stderr <<"Done in "<<t1-t0<<" ms."<<std::endl;
	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);
#ifndef USE_THREAD_MANAGER
	delete[] threads;
#endif
	delete[] rects0;
	delete[] rects1;
	delete[] parameters;
}

void manualBlit_threaded(void *parameters){
	PSF_parameters *p=(PSF_parameters *)parameters;
	manualBlit_threaded(p->src,p->srcRect,p->dst,p->dstRect,p->alpha);
}

uchar integer_division_lookup[0x10000];

void do_alpha_blend(uchar *r1,uchar *g1,uchar *b1,uchar *a1,long r0,long g0,long b0,long a0,bool alpha1,bool alpha0,uchar alpha){
#define do_alpha_blend_SINGLE_ALPHA_SOURCE(alpha_source)\
	ulong as=(alpha_source);							\
	*r1=(uchar)APPLY_ALPHA(r0,*r1,as);					\
	*g1=(uchar)APPLY_ALPHA(g0,*g1,as);					\
	*b1=(uchar)APPLY_ALPHA(b0,*b1,as)
#define do_alpha_blend_DOUBLE_ALPHA_SOURCE(alpha_source)			\
	ulong as=(alpha_source);										\
	ulong bottom_alpha=												\
	*a1=~(uchar)INTEGER_MULTIPLICATION(as^0xFF,*a1^0xFF);			\
	ulong composite=integer_division_lookup[as+bottom_alpha*256];	\
	if (composite){													\
		*r1=(uchar)APPLY_ALPHA(r0,*r1,composite);					\
		*g1=(uchar)APPLY_ALPHA(g0,*g1,composite);					\
		*b1=(uchar)APPLY_ALPHA(b0,*b1,composite);					\
	}
//#define APPLY_ALPHA(c0,c1,a) (INTEGER_MULTIPLICATION((a)^0xFF,(c1))+INTEGER_MULTIPLICATION((a),(c0)))
#define APPLY_ALPHA(c0,c1,a) (INTEGER_MULTIPLICATION(a,(c0)-(c1))+(c1))

	if (alpha==255){
		if (!alpha0){
			*r1=(uchar)r0;
			*g1=(uchar)g0;
			*b1=(uchar)b0;
			if (alpha1)
				*a1=0xFF;
		}else{
			if (!alpha1){
				do_alpha_blend_SINGLE_ALPHA_SOURCE(a0);
			}else{
				do_alpha_blend_DOUBLE_ALPHA_SOURCE(a0);
			}
		}
	}else{
		if (!alpha0){
			if (!alpha1){
				do_alpha_blend_SINGLE_ALPHA_SOURCE(alpha);
			}else{
				do_alpha_blend_DOUBLE_ALPHA_SOURCE(alpha);
			}
		}else{
			a0=INTEGER_MULTIPLICATION(a0,alpha);
			if (!alpha1){
				do_alpha_blend_SINGLE_ALPHA_SOURCE(a0);
			}else{
				do_alpha_blend_DOUBLE_ALPHA_SOURCE(a0);
			}
		}
	}
}

void manualBlit_threaded(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect,manualBlitAlpha_t alpha){
	SDL_Rect &srcRect0=*srcRect,
		&dstRect0=*dstRect;
	int w0=srcRect0.w, h0=srcRect0.h;
	if (srcRect0.w<=0 || srcRect0.h<=0)
		return;

	surfaceData sd[]={
		src,
		dst
	};
	sd[0].pixels+=sd[0].pitch*srcRect0.y+sd[0].advance*srcRect0.x;
	sd[1].pixels+=sd[1].pitch*dstRect0.y+sd[1].advance*dstRect0.x;

	bool negate=(alpha<0);
	if (negate)
		alpha=-alpha;
#define manualBlit_threaded_DO_ALPHA(variable_code){\
	for (int y0=0;y0<h0;y0++){						\
		uchar *pos[]={								\
			sd[0].pixels,							\
			sd[1].pixels							\
		};											\
		for (int x0=0;x0<w0;x0++){					\
			long rgba0[4];							\
			uchar *rgba1[4];						\
			for (int a=0;a<4;a++){					\
				rgba0[a]=pos[0][sd[0].offsets[a]];	\
				rgba1[a]=pos[1]+sd[1].offsets[a];	\
			}										\
													\
			{variable_code}							\
			/*avoid unnecessary jumps*/				\
			if (!(negate && rgba0[3])){				\
				pos[0]+=sd[0].advance;				\
				pos[1]+=sd[1].advance;				\
				continue;							\
			}										\
			for (int a=0;a<3;a++)					\
				*rgba1[a]=~*rgba1[a];				\
		}											\
		sd[0].pixels+=sd[0].pitch;					\
		sd[1].pixels+=sd[1].pitch;					\
	}												\
}
#define manualBlit_threaded_SINGLE_ALPHA_SOURCE(alpha_source)	\
	ulong as=(alpha_source);									\
	*rgba1[0]=(uchar)APPLY_ALPHA(rgba0[0],*rgba1[0],as);		\
	*rgba1[1]=(uchar)APPLY_ALPHA(rgba0[1],*rgba1[1],as);		\
	*rgba1[2]=(uchar)APPLY_ALPHA(rgba0[2],*rgba1[2],as)
#define manualBlit_threaded_DOUBLE_ALPHA_SOURCE(alpha_source)			\
	ulong as=(alpha_source);											\
	ulong bottom_alpha=													\
	*rgba1[3]=~(uchar)INTEGER_MULTIPLICATION(as^0xFF,*rgba1[3]^0xFF);	\
	ulong composite=integer_division_lookup[as+bottom_alpha*256];		\
	if (composite){														\
		*rgba1[0]=(uchar)APPLY_ALPHA(rgba0[0],*rgba1[0],composite);		\
		*rgba1[1]=(uchar)APPLY_ALPHA(rgba0[1],*rgba1[1],composite);		\
		*rgba1[2]=(uchar)APPLY_ALPHA(rgba0[2],*rgba1[2],composite);		\
	}

	if (alpha==255){
		if (!sd[0].alpha){
			manualBlit_threaded_DO_ALPHA(
				*rgba1[0]=(uchar)rgba0[0];
				*rgba1[1]=(uchar)rgba0[1];
				*rgba1[2]=(uchar)rgba0[2];
				if (sd[1].alpha)
					*rgba1[3]=0xFF;
			)
		}else{
			if (!sd[1].alpha){
				manualBlit_threaded_DO_ALPHA(
					manualBlit_threaded_SINGLE_ALPHA_SOURCE(rgba0[3]);
				)
			}else{
				manualBlit_threaded_DO_ALPHA(
					manualBlit_threaded_DOUBLE_ALPHA_SOURCE(rgba0[3]);
				)
			}
		}
	}else{
		if (!sd[0].alpha){
			if (!sd[1].alpha){
				manualBlit_threaded_DO_ALPHA(
					manualBlit_threaded_SINGLE_ALPHA_SOURCE(alpha);
				)
			}else{
				manualBlit_threaded_DO_ALPHA(
					manualBlit_threaded_DOUBLE_ALPHA_SOURCE(alpha);
				)
			}
		}else{
			if (!sd[1].alpha){
				manualBlit_threaded_DO_ALPHA(
					rgba0[3]=INTEGER_MULTIPLICATION(rgba0[3],alpha);
					manualBlit_threaded_SINGLE_ALPHA_SOURCE(rgba0[3]);
				)
			}else{
				manualBlit_threaded_DO_ALPHA(
					rgba0[3]=INTEGER_MULTIPLICATION(rgba0[3],alpha);
					manualBlit_threaded_DOUBLE_ALPHA_SOURCE(rgba0[3]);
				)
			}
		}
	}
}

void multiplyBlend_threaded(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect);
void multiplyBlend_threaded(void *parameters);

void multiplyBlend(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect){
	if (!src || !dst || src->format->BitsPerPixel<24 ||dst->format->BitsPerPixel<24)
		return;
	SDL_Rect srcRect0,dstRect0;
	if (!srcRect){
		srcRect0.x=0;
		srcRect0.y=0;
		srcRect0.w=src->w;
		srcRect0.h=src->h;
	}else
		srcRect0=*srcRect;
	if (!dstRect){
		dstRect0.x=0;
		dstRect0.y=0;
		dstRect0.w=dst->w;
		dstRect0.h=dst->h;
	}else
		dstRect0=*dstRect;

	if (srcRect0.x<0){
		if (srcRect0.w<=-srcRect0.x)
			return;
		srcRect0.w+=srcRect0.x;
		srcRect0.x=0;
	}else if (srcRect0.x>=src->w)
		return;
	if (srcRect0.y<0){
		if (srcRect0.h<=-srcRect0.y)
			return;
		srcRect0.h+=srcRect0.y;
		srcRect0.y=0;
	}else if (srcRect0.y>=src->h)
		return;
	if (srcRect0.x+srcRect0.w>src->w)
		srcRect0.w-=srcRect0.x+srcRect0.w-src->w;
	if (srcRect0.y+srcRect0.h>src->h)
		srcRect0.h-=srcRect0.y+srcRect0.h-src->h;
	if (dstRect0.x+srcRect0.w>dst->w)
		srcRect0.w-=dstRect0.x+srcRect0.w-dst->w;
	if (dstRect0.y+srcRect0.h>dst->h)
		srcRect0.h-=dstRect0.y+srcRect0.h-dst->h;

	if (dstRect0.x<0){
		srcRect0.x=-dstRect0.x;
		srcRect0.w+=dstRect0.x;
		dstRect0.x=0;
	}else if (dstRect0.x>=dst->w)
		return;
	if (dstRect0.y<0){
		dstRect0.h+=dstRect0.y;
		dstRect0.y=0;
	}else if (dstRect0.y>=dst->h)
		return;

	if (srcRect0.w<=0 || srcRect0.h<=0)
		return;

	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	if (cpu_count==1 || srcRect0.w*srcRect0.h<1000){
		multiplyBlend_threaded(src,&srcRect0,dst,&dstRect0);
		SDL_UnlockSurface(dst);
		SDL_UnlockSurface(src);
		return;
	}
#ifndef USE_THREAD_MANAGER
	NONS_Thread *threads=new NONS_Thread[cpu_count];
#endif
	SDL_Rect *rects0=new SDL_Rect[cpu_count];
	SDL_Rect *rects1=new SDL_Rect[cpu_count];
	PSF_parameters *parameters=new PSF_parameters[cpu_count];
	ulong division=ulong(float(srcRect0.h)/float(cpu_count));
	ulong total=0;
	for (ushort a=0;a<cpu_count;a++){
		rects0[a]=srcRect0;
		rects1[a]=dstRect0;
		rects0[a].y+=Sint16(a*division);
		rects0[a].h=Sint16(division);
		rects1[a].y+=Sint16(a*division);
		total+=rects0[a].h;
		parameters[a].src=src;
		parameters[a].srcRect=rects0+a;
		parameters[a].dst=dst;
		parameters[a].dstRect=rects1+a;
	}
	rects0[cpu_count-1].h+=Uint16(srcRect0.h-total);
	for (ushort a=1;a<cpu_count;a++)
#ifndef USE_THREAD_MANAGER
		threads[a].call(multiplyBlend_threaded,parameters+a);
#else
		threadManager.call(a-1,multiplyBlend_threaded,parameters+a);
#endif
	multiplyBlend_threaded(parameters);
#ifndef USE_THREAD_MANAGER
	for (ushort a=1;a<cpu_count;a++)
		threads[a].join();
#else
	threadManager.waitAll();
#endif
	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);
#ifndef USE_THREAD_MANAGER
	delete[] threads;
#endif
	delete[] rects0;
	delete[] rects1;
	delete[] parameters;
}

void multiplyBlend_threaded(void *parameters){
	PSF_parameters *p=(PSF_parameters *)parameters;
	multiplyBlend_threaded(p->src,p->srcRect,p->dst,p->dstRect);
}

void multiplyBlend_threaded(SDL_Surface *src,SDL_Rect *srcRect,SDL_Surface *dst,SDL_Rect *dstRect){
	SDL_Rect &srcRect0=*srcRect,
		&dstRect0=*dstRect;
	int w0=srcRect0.w, h0=srcRect0.h;

	uchar *pos0=(uchar *)src->pixels;
	uchar *pos1=(uchar *)dst->pixels;

	uchar Roffset0=(src->format->Rshift)>>3;
	uchar Goffset0=(src->format->Gshift)>>3;
	uchar Boffset0=(src->format->Bshift)>>3;

	uchar Roffset1=(dst->format->Rshift)>>3;
	uchar Goffset1=(dst->format->Gshift)>>3;
	uchar Boffset1=(dst->format->Bshift)>>3;

	unsigned advance0=src->format->BytesPerPixel;
	unsigned advance1=dst->format->BytesPerPixel;

	pos0+=src->pitch*srcRect0.y+srcRect0.x*advance0;
	pos1+=dst->pitch*dstRect0.y+dstRect0.x*advance1;

	for (int y0=0;y0<h0;y0++){
		uchar *pos00=pos0;
		uchar *pos10=pos1;
		for (int x0=0;x0<w0;x0++){
			uchar r0=pos0[Roffset0];
			uchar g0=pos0[Goffset0];
			uchar b0=pos0[Boffset0];

			uchar *r1=pos1+Roffset1;
			uchar *g1=pos1+Goffset1;
			uchar *b1=pos1+Boffset1;

			*r1=uchar((short(r0)*short(*r1))/255);
			*g1=uchar((short(g0)*short(*g1))/255);
			*b1=uchar((short(b0)*short(*b1))/255);

			pos0+=advance0;
			pos1+=advance1;
		}
		pos0=pos00+src->pitch;
		pos1=pos10+dst->pitch;
	}
}

void getFinalSize(SDL_Surface *src,float matrix[4],ulong &w,ulong &h){
	ulong w0=src->w,
		h0=src->h;
	float coords[][2]={
		{0,0},
		{0,(float)h0},
		{(float)w0,0},
		{(float)w0,(float)h0}
	};
	float minx,miny,maxx,maxy;
	maxx=minx=coords[0][0]*matrix[0]+coords[0][1]*matrix[1];
	maxy=miny=coords[0][0]*matrix[2]+coords[0][1]*matrix[3];
	for (int a=1;a<4;a++){
		float x=coords[a][0]*matrix[0]+coords[a][1]*matrix[1];
		float y=coords[a][0]*matrix[2]+coords[a][1]*matrix[3];
		if (x<minx)
			minx=x;
		if (x>maxx)
			maxx=x;
		if (y<miny)
			miny=y;
		if (y>maxy)
			maxy=y;
	}
	w=ulong(maxx-minx);
	h=ulong(maxy-miny);
}

void invert_matrix(float m[4]){
	float temp=1/(m[0]*m[3]-m[1]*m[2]);
	float m2[]={
		temp*m[3],
		temp*-m[1],
		temp*-m[2],
		temp*m[0]
	};
	memcpy(m,m2,4*sizeof(float));
}

void getCorrected(ulong &x0,ulong &y0,float matrix[4]){
	float coords[][2]={
		{0,0},
		{0,(float)y0},
		{(float)x0,0},
		{(float)x0,(float)y0}
	};
	float minx,miny;
	minx=coords[0][0]*matrix[0]+coords[0][1]*matrix[1];
	miny=coords[0][0]*matrix[2]+coords[0][1]*matrix[3];
	for (int a=1;a<4;a++){
		float x=coords[a][0]*matrix[0]+coords[a][1]*matrix[1];
		float y=coords[a][0]*matrix[2]+coords[a][1]*matrix[3];
		if (x<minx)
			minx=x;
		if (y<miny)
			miny=y;
	}
	x0=(ulong)-minx;
	y0=(ulong)-miny;
}

struct applyTransformationMatrix_parameters{
	uchar *src,
		*dst;
	ulong x,y,
		w0,h0,
		w1,h1,
		advance0,advance1,
		pitch0,pitch1;
	long matrix[4];
	ulong correct_x,correct_y;
};

void applyTransformationMatrix_threaded(const applyTransformationMatrix_parameters &param);
void applyTransformationMatrix_threaded(void *parameters);

SDL_Surface *applyTransformationMatrix(SDL_Surface *src,float matrix[4]){
	if (!src || src->format->BitsPerPixel<24 || !(matrix[0]*matrix[3]-matrix[1]*matrix[2]))
		return 0;
	ulong w,h;
	float inv_matrix[4];
	memcpy(inv_matrix,matrix,4*sizeof(float));
	invert_matrix(inv_matrix);
	
	getFinalSize(src,matrix,w,h);
	SDL_Surface *res=makeSurface(w,h,src->format->BitsPerPixel,
		src->format->Rmask,
		src->format->Gmask,
		src->format->Bmask,
		src->format->Amask);
	//SDL_FillRect(res,0,src->format->Gmask|src->format->Amask);
	
	ulong correct_x=src->w,
		correct_y=src->h;
	getCorrected(correct_x,correct_y,matrix);

	SDL_UnlockSurface(src);
	SDL_UnlockSurface(res);

	ulong division=ulong(float(h)/float(cpu_count));
#ifndef USE_THREAD_MANAGER
	NONS_Thread *threads=new NONS_Thread[cpu_count];
#endif
	applyTransformationMatrix_parameters *parameters=new applyTransformationMatrix_parameters[cpu_count];
	ulong total=0;
	for (ulong a=0;a<cpu_count;a++){
		parameters[a].src=(uchar *)src->pixels;
		parameters[a].dst=(uchar *)res->pixels;
		parameters[a].x=0;
		parameters[a].y=a*division;
		parameters[a].w0=src->w;
		parameters[a].h0=src->h;
		parameters[a].w1=w;
		parameters[a].h1=division;
		total+=division;
		parameters[a].advance0=src->format->BytesPerPixel;
		parameters[a].advance1=res->format->BytesPerPixel;
		parameters[a].pitch0=src->pitch;
		parameters[a].pitch1=res->pitch;

		parameters[a].dst+=parameters[a].y*parameters[a].pitch1;

		for (int b=0;b<4;b++)
			parameters[a].matrix[b]=long(inv_matrix[b]*0x10000);
		parameters[a].correct_x=correct_x;
		parameters[a].correct_y=correct_y;
	}
	parameters[cpu_count-1].h1+=h-total;
	for (ulong a=1;a<cpu_count;a++)
#ifndef USE_THREAD_MANAGER
		threads[a].call(applyTransformationMatrix_threaded,parameters+a);
#else
		threadManager.call(a-1,applyTransformationMatrix_threaded,parameters+a);
#endif
	applyTransformationMatrix_threaded(parameters);
#ifndef USE_THREAD_MANAGER
	for (ushort a=1;a<cpu_count;a++)
		threads[a].join();
#else
	threadManager.waitAll();
#endif
	SDL_UnlockSurface(src);
	SDL_UnlockSurface(res);
#ifndef USE_THREAD_MANAGER
	delete[] threads;
#endif
	delete[] parameters;
	return res;
}

void applyTransformationMatrix_threaded(void *parameters){
	applyTransformationMatrix_threaded(*(applyTransformationMatrix_parameters *)parameters);
}

void applyTransformationMatrix_threaded(const applyTransformationMatrix_parameters &param){
	uchar *src=param.src,
		*dst=param.dst;
	ulong x0=param.x,
		y0=param.y,
		w0=param.w0,
		h0=param.h0,
		w1=param.w1,
		h1=param.h1+y0,
		advance0=param.advance0,
		advance1=param.advance1,
		pitch0=param.pitch0,
		pitch1=param.pitch1,
		correct_x=param.correct_x,
		correct_y=param.correct_y;
	assert(advance0==advance1);
	const long *matrix=param.matrix;
	for (ulong y=y0;y<h1;y++){
		uchar *dst0=dst;
		bool pixels_were_copied=0;
		for (ulong x=x0;x<w1;x++){
			long src_x=(x-correct_x)*matrix[0]+(y-correct_y)*matrix[1];
			long src_y=(x-correct_x)*matrix[2]+(y-correct_y)*matrix[3];
			src_x>>=16;
			src_y>>=16;
			if (src_x<0 || src_y<0 || (ulong)src_x>=w0 || (ulong)src_y>=h0){
				if (!pixels_were_copied){
					dst+=advance1;
					continue;
				}
				break;
			}
			uchar *src2=src+src_x*advance0+src_y*pitch0;
			for (ulong a=0;a<advance0;a++)
				*dst++=*src2++;
			pixels_were_copied=1;
		}
		dst=dst0+pitch1;
	}
}

char checkNativeEndianness(){
	Uint16 a=0x1234;
	if (*(uchar *)&a==0x12)
		return NONS_BIG_ENDIAN;
	else
		return NONS_LITTLE_ENDIAN;
}

void FlipSurfaceH(SDL_Surface *src,SDL_Surface *dst){
	if (!src || !dst || src->w!=dst->w || src->h!=dst->h)
		return;
	if (src==dst){
		SDL_LockSurface(src);

		ulong w=src->w,
			w0=w,
			h=src->h;
		w/=2;

		uchar *pos=(uchar *)src->pixels;
		
		ulong advance=src->format->BytesPerPixel;
		ulong pitch=src->pitch;

		Uint32 putMask=(checkNativeEndianness()==NONS_LITTLE_ENDIAN)?((~0)<<(advance*8)):((~0)>>(advance*8)),
			getMask=~putMask;
		
		for (ulong y=0;y<h;y++){
			uchar *pos0=pos,
				*pos_reverse=pos+(w0-1)*advance;
			for (ulong x=0;x<w;x++){
				Uint32 /*pixelA=*(Uint32 *)pos&getMask,*/
					pixelB=*(Uint32 *)pos_reverse&getMask;
				*(Uint32 *)pos&=putMask;
				*(Uint32 *)pos|=pixelB;
				*(Uint32 *)pos_reverse&=putMask;
				*(Uint32 *)pos_reverse|=pixelB;
				pos+=advance;
				pos_reverse-=advance;
			}
			pos=pos0+pitch;
		}

		SDL_UnlockSurface(src);
	}else{
		SDL_LockSurface(src);
		SDL_LockSurface(dst);

		surfaceData sd[]={
			src,
			dst
		};

		ulong w=sd[0].w,
			h=sd[0].h;
		sd[1].pixels+=(sd[0].w-1)*sd[1].advance;

		for (ulong y=0;y<sd[0].h;y++){
			uchar *pos[]={
				sd[0].pixels,
				sd[1].pixels
			};
			for (ulong x=0;x<sd[0].w;x++){
				pos[1][sd[1].Roffset]=pos[0][sd[0].Roffset];
				pos[1][sd[1].Goffset]=pos[0][sd[0].Goffset];
				pos[1][sd[1].Boffset]=pos[0][sd[0].Boffset];
				if (sd[1].alpha)
					pos[1][sd[1].Aoffset]=(sd[0].alpha)?pos[0][sd[0].Aoffset]:0xFF;

				pos[0]+=sd[0].advance;
				pos[1]-=sd[1].advance;
			}
			sd[0].pixels+=sd[0].pitch;
			sd[1].pixels+=sd[1].pitch;
		}

		SDL_UnlockSurface(src);
		SDL_UnlockSurface(dst);
	}
}

void FlipSurfaceV(SDL_Surface *src,SDL_Surface *dst){
	if (!src || !dst || src->w!=dst->w || src->h!=dst->h)
		return;
	if (src==dst){
		SDL_LockSurface(src);

		ulong w=src->w,
			h=src->h,
			h0=h;
		h/=2;

		uchar *pos=(uchar *)src->pixels;
		
		unsigned advance=src->format->BytesPerPixel;
		unsigned pitch=src->pitch;

		Uint32 putMask=(checkNativeEndianness()==NONS_LITTLE_ENDIAN)?((~0)<<(advance*8)):((~0)>>(advance*8)),
			getMask=~putMask;
		
		for (ulong x=0;x<w;x++){
			uchar *pos0=pos,
				*pos_reverse=pos+(h0-1)*pitch;
			for (ulong y=0;y<h;y++){
				Uint32 /*pixelA=*(Uint32 *)pos&getMask,*/
					pixelB=*(Uint32 *)pos_reverse&getMask;
				*(Uint32 *)pos&=putMask;
				*(Uint32 *)pos|=pixelB;
				*(Uint32 *)pos_reverse&=putMask;
				*(Uint32 *)pos_reverse|=pixelB;
				pos+=pitch;
				pos_reverse-=pitch;
			}
			pos=pos0+advance;
		}

		SDL_UnlockSurface(src);
	}else{
		SDL_LockSurface(src);
		SDL_LockSurface(dst);

		surfaceData sd[]={
			src,
			dst
		};

		ulong w=sd[0].w,
			h=sd[0].h;
		sd[1].pixels+=(sd[0].h-1)*sd[1].pitch;

		for (ulong y=0;y<sd[0].h;y++){
			uchar *pos[]={
				sd[0].pixels,
				sd[1].pixels
			};
			for (ulong x=0;x<sd[0].w;x++){
				pos[1][sd[1].Roffset]=pos[0][sd[0].Roffset];
				pos[1][sd[1].Goffset]=pos[0][sd[0].Goffset];
				pos[1][sd[1].Boffset]=pos[0][sd[0].Boffset];
				if (sd[1].alpha)
					pos[1][sd[1].Aoffset]=(sd[0].alpha)?pos[0][sd[0].Aoffset]:0xFF;

				pos[0]+=sd[0].advance;
				pos[1]+=sd[1].advance;
			}
			sd[0].pixels+=sd[0].pitch;
			sd[1].pixels-=sd[1].pitch;
		}

		SDL_UnlockSurface(src);
		SDL_UnlockSurface(dst);
	}
}

void FlipSurfaceHV(SDL_Surface *src,SDL_Surface *dst){
	if (!src || !dst || src->w!=dst->w || src->h!=dst->h)
		return;
	if (src==dst){
		SDL_LockSurface(src);

		ulong w=src->w,
			h=src->h,
			h0=h;
		h=h/2+h%2;

		uchar *pos=(uchar *)src->pixels;
		
		unsigned advance=src->format->BytesPerPixel;
		unsigned pitch=src->pitch;

		Uint32 putMask=(checkNativeEndianness()==NONS_LITTLE_ENDIAN)?((~0)<<(advance*8)):((~0)>>(advance*8)),
			getMask=~putMask;
		
		for (ulong y=0;y<h;y++){
			uchar *pos0=pos,
				*pos_reverse=pos+(w-1)*advance+(h-1)*pitch;
			if (y>h0/2)
				w/=2;
			for (ulong x=0;x<w;x++){
				Uint32 /*pixelA=*(Uint32 *)pos&getMask,*/
					pixelB=*(Uint32 *)pos_reverse&getMask;
				*(Uint32 *)pos&=putMask;
				*(Uint32 *)pos|=pixelB;
				*(Uint32 *)pos_reverse&=putMask;
				*(Uint32 *)pos_reverse|=pixelB;
				pos+=advance;
				pos_reverse-=advance;
			}
			pos=pos0+pitch;
		}

		SDL_UnlockSurface(src);
	}else{
		SDL_LockSurface(src);
		SDL_LockSurface(dst);

		surfaceData sd[]={
			src,
			dst
		};

		ulong w=sd[0].w,
			h=sd[0].h;
		sd[1].pixels+=(sd[0].h-1)*sd[1].pitch;

		for (ulong y=0;y<sd[0].h;y++){
			uchar *pos[]={
				sd[0].pixels,
				sd[1].pixels
			};
			for (ulong x=0;x<sd[0].w;x++){
				pos[1][sd[1].Roffset]=pos[0][sd[0].Roffset];
				pos[1][sd[1].Goffset]=pos[0][sd[0].Goffset];
				pos[1][sd[1].Boffset]=pos[0][sd[0].Boffset];
				if (sd[1].alpha)
					pos[1][sd[1].Aoffset]=(sd[0].alpha)?pos[0][sd[0].Aoffset]:0xFF;

				pos[0]+=sd[0].advance;
				pos[1]-=sd[1].advance;
			}
			sd[0].pixels+=sd[0].pitch;
			sd[1].pixels-=sd[1].pitch;
		}

		SDL_UnlockSurface(src);
		SDL_UnlockSurface(dst);
	}
}

SDL_Surface *horizontalShear(SDL_Surface *src,float amount){
	if (!src || src->format->BitsPerPixel<24)
		return 0;
	if (!amount)
		return copySurface(src);

	ulong w=src->w,
		h=src->h;

	//ulong resWidth=float(src->w)*(1.0f+ABS(amount));
	//ulong resWidth=float(src->w)*(1.0f+ABS(amount)*(float(src->h)/float(src->w)));
	ulong resWidth=ulong(src->w+ABS(amount)*float(src->h));
	SDL_Surface *res=makeSurface(
		resWidth,src->h,
		src->format->BitsPerPixel,
		src->format->Rmask,
		src->format->Gmask,
		src->format->Bmask,
		src->format->Amask
	);

	SDL_LockSurface(src);
	SDL_LockSurface(res);

	uchar *pos0=(uchar *)src->pixels;
	uchar *pos1=(uchar *)res->pixels;

	ulong advance0=src->format->BytesPerPixel;
	ulong advance1=res->format->BytesPerPixel;
	ulong pitch0=src->pitch;
	ulong pitch1=res->pitch;

	ulong delta=ulong(ABS(amount)*0x10000);
	ulong X=0;
	ulong x=0;

	if (amount>0){
		for (ulong y=0;y<h;y++){
			if (w+x<=resWidth)
				memcpy(pos1,pos0,advance0*w);
			else
				memcpy(pos1,pos0,advance0*(resWidth-w-x));
			X+=delta;
			while (X>=0x10000){
				pos1+=advance1;
				X-=0x10000;
				x++;
			}
			pos0+=pitch0;
			pos1+=pitch1;
		}
	}else{
		pos0+=(h-1)*pitch0;
		pos1+=(h-1)*pitch1;
		for (long y=(long)h-1;y>=0;y--){
			if (w+x<=resWidth)
				memcpy(pos1,pos0,advance0*w);
			else
				memcpy(pos1,pos0,advance0*(resWidth-w-x));
			X+=delta;
			while (X>=0x10000){
				pos1+=advance1;
				X-=0x10000;
				x++;
			}
			pos0-=pitch0;
			pos1-=pitch1;
		}
	}

	SDL_UnlockSurface(src);
	SDL_UnlockSurface(res);

	return res;
}

SDL_Surface *verticalShear(SDL_Surface *src,float amount){
	if (!src || src->format->BitsPerPixel<24)
		return 0;
	if (!amount)
		return copySurface(src);

	ulong w=src->w,
		h=src->h;

	//ulong resHeight=float(src->h)*(1.0f+ABS(amount)*(float(src->w)/float(src->h)));
	ulong resHeight=ulong(src->h+ABS(amount)*float(src->w));
	SDL_Surface *res=makeSurface(
		src->w,resHeight,
		src->format->BitsPerPixel,
		src->format->Rmask,
		src->format->Gmask,
		src->format->Bmask,
		src->format->Amask
	);

	SDL_LockSurface(src);
	SDL_LockSurface(res);

	uchar *pos0=(uchar *)src->pixels;
	uchar *pos1=(uchar *)res->pixels;

	ulong advance0=src->format->BytesPerPixel;
	ulong advance1=res->format->BytesPerPixel;
	ulong pitch0=src->pitch;
	ulong pitch1=res->pitch;

	ulong delta=ulong(ABS(amount)*0x10000);
	ulong X=0;
	ulong y=0;

	if (amount>0){
		for (ulong x=0;x<w;x++){
			uchar *pos00=pos0,
				*pos10=pos1;
			for (ulong y2=y,a=0;a<h && y2<resHeight;y2++,a++){
				for (ulong b=0;b<advance0;b++)
					pos10[b]=pos00[b];
				pos00+=pitch0;
				pos10+=pitch1;
			}
			X+=delta;
			while (X>=0x10000){
				pos1+=pitch1;
				X-=0x10000;
				y++;
			}
			pos0+=advance0;
			pos1+=advance1;
		}
	}else{
		pos0+=(w-1)*advance0;
		pos1+=(w-1)*advance1;
		for (long x=(long)w-1;x>=0;x--){
			uchar *pos00=pos0,
				*pos10=pos1;
			for (ulong y2=y,a=0;a<h && y2<resHeight;y2++,a++){
				for (ulong b=0;b<advance0;b++)
					pos10[b]=pos00[b];
				pos00+=pitch0;
				pos10+=pitch1;
			}
			X+=delta;
			while (X>=0x10000){
				pos1+=pitch1;
				X-=0x10000;
				y++;
			}
			pos0-=advance0;
			pos1-=advance1;
		}
	}

	SDL_UnlockSurface(src);
	SDL_UnlockSurface(res);

	return res;
}

Uint8 readByte(void *buffer,ulong &offset){
	return ((uchar *)buffer)[offset++];
}

Sint16 readSignedWord(char *buffer,ulong &offset){
	Sint16 r=0;
	for (char a=2;a>=0;a--){
		r<<=8;
		r|=(uchar)buffer[offset+a];
	}
	offset+=2;
	return r;
}

Uint16 readWord(void *_buffer,ulong &offset){
	uchar *buffer=(uchar *)_buffer;
	Uint16 r=0;
	for (char a=2;a>=0;a--){
		r<<=8;
		r|=buffer[offset+a];
	}
	offset+=2;
	return r;
}

Sint32 readSignedDWord(char *buffer,ulong &offset){
	Sint32 r=0;
	for (char a=3;a>=0;a--){
		r<<=8;
		r|=(uchar)buffer[offset+a];
	}
	offset+=4;
	return r;
}

Uint32 readDWord(void *_buffer,ulong &offset){
	uchar *buffer=(uchar *)_buffer;
	Uint32 r=0;
	for (char a=3;a>=0;a--){
		r<<=8;
		r|=buffer[offset+a];
	}
	offset+=4;
	return r;
}

std::string readString(char *buffer,ulong &offset){
	std::string r(buffer+offset);
	offset+=r.size()+1;
	return r;
}

void writeByte(Uint8 a,std::string &str,ulong offset){
	if (offset==ULONG_MAX)
		str.push_back(a&0xFF);
	else
		str[offset]=a&0xFF;
}

void writeWord(Uint16 a,std::string &str,ulong offset){
	ulong off=(offset==ULONG_MAX)?str.size():offset;
	for (ulong b=0;b<2;b++,off++){
		if (str.size()>off)
			str[off]=a&0xFF;
		else
			str.push_back(a&0xFF);
		a>>=8;
	}
}

void writeDWord(Uint32 a,std::string &str,ulong offset){
	ulong off=(offset==ULONG_MAX)?str.size():offset;
	for (ulong b=0;b<4;b++,off++){
		if (str.size()>off)
			str[off]=a&0xFF;
		else
			str.push_back(a&0xFF);
		a>>=8;
	}
}

void writeWordBig(Uint16 a,std::string &str,ulong offset){
	if (offset==ULONG_MAX)
		offset=str.size();
	for (ulong b=0;b<2;b++,offset++){
		if (str.size()>offset)
			str[offset]=a>>8;
		else
			str.push_back(a>>8);
		a<<=8;
	}
}

void writeDWordBig(Uint32 a,std::string &str,ulong offset){
	if (offset==ULONG_MAX)
		offset=str.size();
	for (ulong b=0;b<4;b++,offset++){
		if (str.size()>offset)
			str[offset]=a>>24;
		else
			str.push_back(a>>24);
		a<<=8;
	}
}

void writeString(const std::wstring &a,std::string &str){
	str.append(UniToUTF8(a));
	str.push_back(0);
}

char *compressBuffer_BZ2(char *src,unsigned long srcl,unsigned long *dstl){
	unsigned long l=srcl,realsize=l;
	char *dst=new char[l];
	while (BZ2_bzBuffToBuffCompress(dst,(unsigned int *)&l,src,srcl,1,0,0)==BZ_OUTBUFF_FULL){
		delete[] dst;
		l*=2;
		realsize=l;
		dst=new char[l];
	}
	if (l!=realsize){
		char *temp=new char[l];
		memcpy(temp,dst,l);
		delete[] dst;
		dst=temp;
	}
	*dstl=l;
	return dst;
}

char *decompressBuffer_BZ2(char *src,unsigned long srcl,unsigned long *dstl){
	unsigned long l=srcl,realsize=l;
	char *dst=new char[l];
	while (BZ2_bzBuffToBuffDecompress(dst,(unsigned int *)&l,src,srcl,1,0)==BZ_OUTBUFF_FULL){
		delete[] dst;
		l*=2;
		realsize=l;
		dst=new char[l];
	}
	if (l!=realsize){
		char *temp=new char[l];
		memcpy(temp,dst,l);
		delete[] dst;
		dst=temp;
	}
	*dstl=l;
	return dst;
}

std::wstring readline(std::wstring::const_iterator start,std::wstring::const_iterator end,std::wstring::const_iterator *out){
	std::wstring::const_iterator end2=start;
	for (;end2!=end && *end2!=13 && *end2!=10;end2++);
	if (!!out){
		*out=end2;
		for (;*out!=end && (**out==13 || **out==10);(*out)++);
	}
	return std::wstring(start,end2);
}

ErrorCode inPlaceDecryption(char *buffer,ulong length,ulong mode){
	switch (mode){
		case ENCRYPTION::NONE:
		default:
			return NONS_NO_ERROR;
		case ENCRYPTION::XOR84:
			for (ulong a=0;a<length;a++)
				buffer[a]^=0x84;
			return NONS_NO_ERROR;
		case ENCRYPTION::VARIABLE_XOR:
			{
				uchar magic_numbers[5]={0x79,0x57,0x0d,0x80,0x04};
				ulong index=0;
				for (ulong a=0;a<length;a++){
					((uchar *)buffer)[a]^=magic_numbers[index];
					index=(index+1)%5;
				}
				return NONS_NO_ERROR;
			}
		case ENCRYPTION::TRANSFORM_THEN_XOR84:
			{
				o_stderr <<"TRANSFORM_THEN_XOR84 (aka mode 4) encryption not implemented for a very good\n"
					"reason. Which I, of course, don\'t need to explain to you. Good day.";
				return NONS_NOT_IMPLEMENTED;
			}
	}
	return NONS_NO_ERROR;
}

#if NONS_SYS_WINDOWS
#include <windows.h>
extern HWND mainWindow;

BOOL CALLBACK findMainWindow_callback(HWND handle,LPARAM lparam){
	const std::wstring *caption=(const std::wstring *)lparam;
	std::wstring buffer=*caption;
	GetWindowText(handle,&buffer[0],buffer.size());
	if (buffer!=*caption)
		return 1;
	mainWindow=handle;
	return 0;
}

void findMainWindow(const wchar_t *caption){
	std::wstring string=caption;
	EnumWindows(findMainWindow_callback,(LPARAM)&string);
}
#endif

extern wchar_t SJIS2Unicode[0x10000],
	Unicode2SJIS[0x10000];

#define SINGLE_CHARACTER_F(name) inline bool name(wchar_t &dst,const uchar *src,size_t size,ulong &bytes_used)

SINGLE_CHARACTER_F(ConvertSingleUTF8Character){
	bytes_used=0;
	uchar byte=src[bytes_used++];
	wchar_t c=0;
	if (!(byte&0x80))
		c=byte;
	else if ((byte&0xC0)==0x80){
		bytes_used=1;
		return 0;
	}else if ((byte&0xE0)==0xC0){
		if (size<2){
			bytes_used=size;
			return 0;
		}
		c=byte&0x1F;
		c<<=6;
		c|=src[bytes_used++]&0x3F;
	}else if ((byte&0xF0)==0xE0){
		if (size<3){
			bytes_used=size;
			return 0;
		}
		c=byte&0x0F;
		c<<=6;
		c|=src[bytes_used++]&0x3F;
		c<<=6;
		c|=src[bytes_used++]&0x3F;
	}else if ((byte&0xF8)==0xF0){
		if (size<4){
			bytes_used=size;
			return 0;
		}
#if WCHAR_MAX==0xFFFF
		c='?';
#else
		c=byte&0x07;
		c<<=6;
		c|=src[bytes_used++]&0x3F;
		c<<=6;
		c|=src[bytes_used++]&0x3F;
		c<<=6;
		c|=src[bytes_used++]&0x3F;
#endif
	}
	dst=c;
	return 1;
}

#define IS_SJIS_WIDE(x) ((x)>=0x81 && (x)<=0x9F || (x)>=0xE0 && (x)<=0xEF)

SINGLE_CHARACTER_F(ConvertSingleSJISCharacter){
	bytes_used=0;
	uchar c0=src[bytes_used++];
	wchar_t c1;
	if (IS_SJIS_WIDE(c0)){
		if (size<2){
			bytes_used=size;
			return 0;
		}
		c1=(c0<<8)|src[bytes_used++];
	}else
		c1=c0;
	if (SJIS2Unicode[c1]=='?' && c1!='?'){
		o_stderr <<"ENCODING ERROR: Character SJIS+"<<itohexc(c1,4)
			<<" is unsupported by this Shift JIS->Unicode implementation. Replacing with '?'.\n";
	}
	dst=SJIS2Unicode[c1];
	return 1;
}

void ConvertSingleCharacterToUTF8(char dst[4],wchar_t src,ulong &bytes_used){
	bytes_used=0;
	if (src<0x80)
		dst[bytes_used++]=(char)src;
	else if (src<0x800){
		dst[bytes_used++]=(src>>6)|0xC0;
		dst[bytes_used++]=src&0x3F|0x80;
#if WCHAR_MAX==0xFFFF
	}else{
#else
	}else if (src<0x10000){
#endif
		dst[bytes_used++]=(src>>12)|0xE0;
		dst[bytes_used++]=((src>>6)&0x3F)|0x80;
		dst[bytes_used++]=src&0x3F|0x80;
#if WCHAR_MAX==0xFFFF
	}
#else
	}else{
		dst[bytes_used++]=(src>>18)|0xF0;
		dst[bytes_used++]=((src>>12)&0x3F)|0x80;
		dst[bytes_used++]=((src>>6)&0x3F)|0x80;
		dst[bytes_used++]=src&0x3F|0x80;
	}
#endif
}

void ISO_WC(wchar_t *dst,const uchar *src,ulong srcl){
	for (ulong a=0;a<srcl;a++,src++,dst++)
		*dst=*src;
}

void UTF8_WC(wchar_t *dst,const uchar *src,ulong srcl){
	ulong advance;
	for (ulong a=0;a<srcl;a+=advance)
		if (!ConvertSingleUTF8Character(*dst++,src+a,srcl-a,advance))
			break;
}

ulong SJIS_WC(wchar_t *dst,const uchar *src,ulong srcl){
	ulong ret=0;
	ulong advance;
	for (ulong a=0;a<srcl;a+=advance,ret++)
		if (!ConvertSingleSJISCharacter(*dst++,src+a,srcl-a,advance))
			break;
	return ret;
}

void WC_88591(uchar *dst,const wchar_t *src,ulong srcl){
	for (ulong a=0;a<srcl;a++,src++,dst++)
		*dst=(*src>0xFF)?'?':*src;
}

ulong getUTF8size(const wchar_t *buffer,ulong size){
	ulong res=0;
	for (ulong a=0;a<size;a++){
		if (buffer[a]<0x80)
			res++;
		else if (buffer[a]<0x800)
			res+=2;
#if WCHAR_MAX==0xFFFF
		else
#else
		else if (buffer[a]<0x10000)
#endif
			res+=3;
#if WCHAR_MAX!=0xFFFF
		else
			res+=4;
#endif
	}
	return res;
}

void WC_UTF8(uchar *dst,const wchar_t *src,ulong srcl){
	for (ulong a=0;a<srcl;a++){
		wchar_t character=*src++;
		if (character<0x80)
			*dst++=(uchar)character;
		else if (character<0x800){
			*dst++=(character>>6)|0xC0;
			*dst++=character&0x3F|0x80;
#if WCHAR_MAX==0xFFFF
		}else{
#else
		}else if (character<0x10000){
#endif
			*dst++=(character>>12)|0xE0;
			*dst++=((character>>6)&0x3F)|0x80;
			*dst++=character&0x3F|0x80;
#if WCHAR_MAX==0xFFFF
		}
#else
		}else{
			*dst++=(character>>18)|0xF0;
			*dst++=((character>>12)&0x3F)|0x80;
			*dst++=((character>>6)&0x3F)|0x80;
			*dst++=character&0x3F|0x80;
		}
#endif
	}
}

ulong WC_SJIS(uchar *dst,const wchar_t *src,ulong srcl){
	ulong ret=0;
	for (ulong a=0;a<srcl;a++){
		wchar_t srcc=*src++,
			character=Unicode2SJIS[srcc];
		if (character=='?' && srcc!='?'){
			o_stderr <<"ENCODING ERROR: Character U+"<<itohexc(srcc,4)
				<<" is unsupported by this Unicode->Shift JIS implementation. Replacing with '?'.\n";
		}
		if (character<0x100)
			dst[ret++]=(uchar)character;
		else{
			dst[ret++]=character>>8;
			dst[ret++]=character&0xFF;
		}
	}
	return ret;
}

std::wstring UniFromISO88591(const std::string &str){
	std::wstring res;
	res.resize(str.size());
	ISO_WC(&res[0],(const uchar *)&str[0],str.size());
	return res;
}

std::wstring UniFromUTF8(const std::string &str){
	ulong start=0;
	if (str.size()>=3 && (uchar)str[0]==BOM8A && (uchar)str[1]==BOM8B && (uchar)str[2]==BOM8C)
		start+=3;
	const uchar *str2=(const uchar *)&str[0]+start;
	ulong size=0;
	for (ulong a=start,end=str.size();a<end;a++,str2++)
		if (*str2<128 || (*str2&192)==192)
			size++;
	std::wstring res;
	res.resize(size);
	str2=(const uchar *)&str[0]+start;
	UTF8_WC(&res[0],str2,str.size()-start);
	return res;
}

std::wstring UniFromSJIS(const std::string &str){
	std::wstring res;
	res.resize(str.size());
	res.resize(SJIS_WC(&res[0],(const uchar *)&str[0],str.size()));
	return res;
}

std::string UniToISO88591(const std::wstring &str){
	std::string res;
	res.resize(str.size());
	WC_88591((uchar *)&res[0],&str[0],str.size());
	return res;
}

std::string UniToUTF8(const std::wstring &str,bool addBOM){
	std::string res;
	res.resize(getUTF8size(&str[0],str.size())+int(addBOM)*3);
	if (addBOM){
		res.push_back(BOM8A);
		res.push_back(BOM8B);
		res.push_back(BOM8C);
	}
	WC_UTF8((uchar *)&res[int(addBOM)*3],&str[0],str.size());
	return res;
}

std::string UniToSJIS(const std::wstring &str){
	std::string res;
	res.resize(str.size()*sizeof(wchar_t));
	res.resize(WC_SJIS((uchar *)&res[0],&str[0],str.size()));
	return res;
}

bool ConvertSingleCharacter(wchar_t &dst,void *buffer,size_t size,ulong &bytes_used,ENCODING::ENCODING encoding){
	switch (encoding){
		case ENCODING::UTF8:
			return ConvertSingleUTF8Character(dst,(uchar *)buffer,size,bytes_used);
		case ENCODING::SJIS:
			return ConvertSingleSJISCharacter(dst,(uchar *)buffer,size,bytes_used);
		case ENCODING::ISO_8859_1:
			bytes_used=1;
			dst=*(uchar *)buffer;
			return 1;
		default:
			bytes_used=1;
			return 0;
	}
}

ulong find_first_not_of_in_utf8(const std::string &str,const wchar_t *find,ulong start_at){
	while (start_at<str.size()){
		wchar_t c;
		ulong unused;
		if (
				ConvertSingleUTF8Character(c,(const uchar *)&str[start_at],str.size()-start_at,unused) &&
				!multicomparison(c,find))
			return start_at;
		start_at++;
	}
	return ULONG_MAX;
}

ulong find_last_not_of_in_utf8(const std::string &str,const wchar_t *find,ulong start_at){
	if (start_at==ULONG_MAX)
		start_at=str.size()-1;
	while (1){
		wchar_t c;
		ulong unused;
		if (
				ConvertSingleUTF8Character(c,(const uchar *)&str[start_at],str.size()-start_at,unused) &&
				!multicomparison(c,find))
			return start_at;
		if (!start_at)
			break;
		start_at--;
	}
	return ULONG_MAX;
}

bool iswhitespace(wchar_t character){
	const wchar_t *whitespace=WCS_WHITESPACE;
	for (const wchar_t *a=whitespace;*a;a++)
		if (character==*a)
			return 1;
	return 0;
}

bool iswhitespace(char character){
	return (uchar(character)<0x80)?iswhitespaceASCIIe(character):0;
}

bool iswhitespaceASCIIe(char character){
	const char *whitespace=STR_WHITESPACE;
	for (const char *a=whitespace;*a;a++)
		if (character==*a)
			return 1;
	return 0;
}

bool isbreakspace(wchar_t character){
	switch (character){
		case 0x0020:
		case 0x1680:
		case 0x180E:
		case 0x2002:
		case 0x2003:
		case 0x2004:
		case 0x2005:
		case 0x2006:
		case 0x2008:
		case 0x2009:
		case 0x200A:
		case 0x200B:
		case 0x200C:
		case 0x200D:
		case 0x205F:
		case 0x3000:
			return 1;
	}
	return 0;
}

bool isbreakspace(char character){
	return (character>0)?iswhitespaceASCIIe(character):0;
}

bool isbreakspaceASCIIe(char character){
	return character==0x20;
}

bool isValidUTF8(const char *buffer,ulong size){
	const uchar *unsigned_buffer=(const uchar *)buffer;
	for (ulong a=0;a<size;a++){
		ulong char_len;
		if (!(*unsigned_buffer&128))
			char_len=1;
		else if ((*unsigned_buffer&224)==192)
			char_len=2;
		else if ((*unsigned_buffer&240)==224)
			char_len=3;
		else if ((*unsigned_buffer&248)==240)
			char_len=4;
		else
			return 0;
		unsigned_buffer++;
		if (char_len<2)
			continue;
		a++;
		for (ulong b=1;b<char_len;b++,a++,unsigned_buffer++)
			if (*unsigned_buffer<0x80 || (*unsigned_buffer&0xC0)!=0x80)
				return 0;
	}
	return 1;
}

bool isValidSJIS(const char *buffer,ulong size){
	const uchar *unsigned_buffer=(const uchar *)buffer;
	for (ulong a=0;a<size;a++,unsigned_buffer++){
		if (!IS_SJIS_WIDE(*unsigned_buffer)){
			//Don't bother trying to understand what's going on here. It took
			//*me* around ten minutes. It works, and that's all you need to
			//know.
			if (*unsigned_buffer>=0x80 && *unsigned_buffer<=0xA0 || *unsigned_buffer>=0xF0)
				return 0;
			continue;
		}
		a++;
		unsigned_buffer++;
		if (*unsigned_buffer<0x40 || *unsigned_buffer>0xFC || *unsigned_buffer==0x7F)
			return 0;
	}
	return 1;
}

void NONS_tolower(wchar_t *param){
	for (;*param;param++)
		*param=NONS_tolower(*param);
}

void NONS_tolower(char *param){
	for (;*param;param++)
		*param=NONS_tolower(*param);
}
