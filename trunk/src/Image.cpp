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
#include <list>
#include <cmath>

class NONS_SurfaceManager{
public:
	class Surface{
		uchar *data;
		ulong w,h;
		long ref_count;
		static ulong obj_count;
		static NONS_Mutex mutex;
		ulong id;
		friend class NONS_SurfaceManager;
		Surface(){ this->set_id(); }
		void set_id(){
			mutex.lock();
			this->id=obj_count++;
			mutex.unlock();
		}
	public:
		Surface(ulong w,ulong h);
		virtual ~Surface();
		void get_properties(NONS_SurfaceProperties &sp) const;
#define NONS_SurfaceManager_Surface_RELATIONAL_OP(op) bool operator op(const Surface &b) const{ return this->id op b.id; }
		OVERLOAD_RELATIONAL_OPERATORS(NONS_SurfaceManager_Surface_RELATIONAL_OP)
	};
	class ScreenSurface:public Surface{
		SDL_Surface *screen;
		friend class NONS_SurfaceManager;
	public:
		ScreenSurface(SDL_Surface *s):Surface(),screen(s){
			this->w=s->w;
			this->h=s->h;
			this->data=(uchar *)s->pixels;
			this->ref_count=LONG_MAX>>1;
		}
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
		index_t(const surfaces_t::iterator &i):_good(0),i(i),p(0){}
		index_t(Surface *p):_good(0),p(p){}
		const Surface &operator*() const{ return (this->p)?*this->p:**this->i; }
		const Surface *operator->() const{ return (this->p)?this->p:*this->i; }
		bool good(){ return this->_good; }
	};

	NONS_SurfaceManager();
	~NONS_SurfaceManager();
	void init();
	index_t allocate(ulong w,ulong h);
	index_t load(const std::wstring &name);
	index_t copy(const index_t &src);
	void ref(const index_t &);
	bool deref(const index_t &);
	index_t scale(const index_t &src,double scalex,double scaley);
	index_t rotate(const index_t &src,double rotate);
	index_t transform(const index_t &src,double matrix[4]);
	void assign_screen(SDL_Surface *);
	index_t get_screen();
private:
	bool initialized;
	surfaces_t surfaces;
	ScreenSurface *screen;
	//NONS_LibraryLoader *svg_library;
	//SVG_Functions svg_functions;
	//NONS_FileLog *filelog;
	bool fast_svg;
	double base_scale[2];
	//NONS_DiskCache disk_cache;
	//1 if the image was added, 0 otherwise
	bool addElementToCache(NONS_Surface *img);
};

ulong NONS_SurfaceManager::Surface::obj_count=0;
NONS_Mutex NONS_SurfaceManager::Surface::mutex;

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

NONS_SurfaceManager sm;

struct NONS_Surface_Private{
	NONS_SurfaceManager::index_t surface;
	NONS_LongRect rect;
	NONS_Surface_Private(const NONS_SurfaceManager::index_t &s):surface(s){
		if (this->surface.good()){
			NONS_SurfaceProperties sp;
			s->get_properties(sp);
			this->rect.w=sp.w;
			this->rect.h=sp.h;
		}
	}
};

NONS_Surface::NONS_Surface(const NONS_CrippledSurface &original){
	this->data=new NONS_Surface_Private(original.data->surface);
	sm.ref(this->data->surface);
}

NONS_Surface::NONS_Surface(ulong w,ulong h){
	this->data=new NONS_Surface_Private(sm.allocate(w,h));
}

NONS_Surface::NONS_Surface(const std::wstring &name){
	this->data=new NONS_Surface_Private(sm.load(name));
}

NONS_Surface::NONS_Surface(const NONS_Surface &original,double x,double y,double rotation){
	if (x==1.0 && y==1.0 && fmod(rotation,360.0)==0.0){
		this->data=new NONS_Surface_Private(original.data->surface);
		sm.ref(this->data->surface);
	}else{
		NONS_SurfaceManager::index_t temp=sm.scale(original.data->surface,x,y);
		this->data=new NONS_Surface_Private(sm.rotate(temp,rotation));
		sm.deref(temp);
	}
}

NONS_Surface::NONS_Surface(const NONS_Surface &original,double m[4]){
	this->data=new NONS_Surface_Private(sm.transform(original.data->surface,m));
}

//------------------------------------------------------------------------------
// OVER
//------------------------------------------------------------------------------

struct over_blend_parameters{
	const NONS_SurfaceProperties *dst,
		*src;
	NONS_LongRect dst_rect,
		src_rect;
	long alpha;
};

void over_blend_threaded(const NONS_SurfaceProperties &dst,
		const NONS_LongRect &dst_rect,
		const NONS_SurfaceProperties &src,
		const NONS_LongRect &src_rect,
		long alpha);
void over_blend_threaded(void *parameters);

void over_blend(
		const NONS_SurfaceProperties &dst,
		const NONS_LongRect &dst_rect,
		const NONS_SurfaceProperties &src,
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
	over_blend_threaded(*p->dst,p->src_rect,*p->src,p->dst_rect,p->alpha);
}

uchar integer_division_lookup[0x10000];

//#define _APPLY_ALPHA(c0,c1,a) (INTEGER_MULTIPLICATION((a)^0xFF,(c1))+INTEGER_MULTIPLICATION((a),(c0)))
#define _APPLY_ALPHA(c0,c1,a) ((((a)^0xFF)*(c1)+(a)*(c0))/255)
#if defined _DEBUG
#define APPLY_ALPHA _APPLY_ALPHA
#else
inline uchar APPLY_ALPHA(ulong c0,ulong c1,ulong a){
	return (uchar)_APPLY_ALPHA(c0,c1,a);
}
#endif

void over_blend_threaded(const NONS_SurfaceProperties &dst,
		const NONS_LongRect &dst_rect,
		const NONS_SurfaceProperties &src,
		const NONS_LongRect &src_rect,
		long alpha){
	int w=src_rect.w,
		h=src_rect.h;
	NONS_SurfaceProperties sp[]={
		src,
		dst
	};
	sp[0].pixels+=sp[0].pitch*src_rect.y+4*src_rect.x;
	sp[1].pixels+=sp[1].pitch*dst_rect.y+4*dst_rect.x;

	bool negate=(alpha<0);
	if (negate)
		alpha=-alpha;
	for (int y=0;y<h;y++){
		uchar *pos[]={
			sp[0].pixels,
			sp[1].pixels
		};
		for (int x=0;x<w;x++){
			long rgba0[4];
			uchar *rgba1[4];
			for (int a=0;a<4;a++){
				rgba0[a]=pos[0][a];
				rgba1[a]=pos[1]+a;
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
				pos[0]+=4;
				pos[1]+=4;
				continue;
			}
			for (int a=0;a<3;a++)
				*rgba1[a]=~*rgba1[a];
			pos[0]+=4;
			pos[1]+=4;
		}
		sp[0].pixels+=sp[0].pitch;
		sp[1].pixels+=sp[1].pitch;
	}
}

//------------------------------------------------------------------------------

NONS_Surface::~NONS_Surface(){
	sm.deref(this->data->surface);
	delete this->data;
}

void NONS_Surface::over(const NONS_Surface &src,const NONS_LongRect *dst_rect,const NONS_LongRect *src_rect,long alpha){
	NONS_LongRect sr,
		dr;
	if (!fix_rects(sr,dr,dst_rect,src_rect,*this,src))
		return;
	NONS_SurfaceProperties ssp,
		dsp;
	src.get_properties(ssp);
	this->get_properties(dsp);
	over_blend(dsp,dr,ssp,sr,alpha);
}

void NONS_Surface::copy_pixels(const NONS_Surface &src,const NONS_LongRect *dst_rect,const NONS_LongRect *src_rect){
	NONS_LongRect sr,
		dr;
	if (!fix_rects(sr,dr,dst_rect,src_rect,*this,src))
		return;
	NONS_SurfaceProperties ssp,
		dsp;
	src.get_properties(ssp);
	this->get_properties(dsp);
	ssp.pixels+=sr.x*4+sr.y*ssp.pitch;
	dsp.pixels+=dr.x*4+dr.y*dsp.pitch;
	for (ulong y=dr.h;y;y--){
		memcpy(dsp.pixels,ssp.pixels,sr.w*4);
		ssp.pixels+=ssp.pitch;
		dsp.pixels+=dsp.pitch;
	}
}

NONS_LongRect NONS_Surface::default_box(const NONS_LongRect *b) const{
	return (b)?*b:this->data->rect;
}

void NONS_Surface::scale(double x,double y){
	NONS_SurfaceManager::index_t temp=sm.scale(this->data->surface,x,y);
	if (temp.good()){
		sm.deref(this->data->surface);
		*this->data=NONS_Surface_Private(temp);
	}
}

void NONS_Surface::rotate(double alpha){
	NONS_SurfaceManager::index_t temp=sm.rotate(this->data->surface,alpha);
	if (temp.good()){
		sm.deref(this->data->surface);
		*this->data=NONS_Surface_Private(temp);
	}
}

void NONS_Surface::transform(double m[4]){
	NONS_SurfaceManager::index_t temp=sm.transform(this->data->surface,m);
	if (temp.good()){
		sm.deref(this->data->surface);
		*this->data=NONS_Surface_Private(temp);
	}
}

void NONS_Surface::get_properties(NONS_SurfaceProperties &sp) const{
	if (this->good())
		this->data->surface->get_properties(sp);
}

//------------------------------------------------------------------------------
// BILINEAR INTERPOLATION
//------------------------------------------------------------------------------

struct interpolation_parameters{
	const NONS_SurfaceProperties *dst,
		*src;
	NONS_LongRect dst_rect,
		src_rect;
};

#define INTERPOLATION_SIGNATURE(x)\
	void x(NONS_SurfaceProperties dst,NONS_LongRect dst_rect,NONS_SurfaceProperties src,NONS_LongRect src_rect)
#define DECLARE_INTERPOLATION_F(x)\
	INTERPOLATION_SIGNATURE(x);\
	void x(void *parameters){\
		interpolation_parameters *p=(interpolation_parameters *)parameters;\
		x(*p->dst,p->dst_rect,*p->src,p->src_rect);\
	}

#define NONS_Surface_DEFINE_INTERPOLATION_F(x,y)                    \
	void NONS_Surface::x(                                           \
		const NONS_Surface &src,                                    \
		const NONS_LongRect *dst_rect,                              \
		const NONS_LongRect *src_rect                               \
	){                                                              \
		this->x(y,src,dst_rect,src_rect); \
	}

NONS_Surface_DECLARE_INTERPOLATION_F_INTERNAL(NONS_Surface::,bilinear_interpolation){
	NONS_LongRect sr,
		dr;
	if (!fix_rects(sr,dr,dst_rect,src_rect,*this,src))
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
NONS_Surface_DEFINE_INTERPOLATION_F(bilinear_interpolation,bilinear_interpolation_threaded)

INTERPOLATION_SIGNATURE(bilinear_interpolation_threaded){
	const ulong unit=1<<16;
	ulong advance_x=(src.w<<16)/dst.w,
		advance_y=(src.h<<16)/dst.h;
	src_rect.w+=src_rect.x;
	src_rect.h+=src_rect.y;
	dst_rect.w+=dst_rect.x;
	dst_rect.h+=dst_rect.y;
	long X=(advance_x>>2)+dst_rect.x*advance_x,
		Y=(advance_y>>2)+dst_rect.y*advance_y;
	uchar *dst_pix=dst.pixels+dst_rect.y*dst.pitch+dst_rect.x*4;
	for (long y=dst_rect.y;y<dst_rect.h;y++){
		long y0=Y>>16;
		if ((ulong)y0+1>=src.h)
			break;
		const uchar *src_pix=src.pixels+src.pitch*y0;
#define GET_FRACTION(x) ((((x)&(unit-1))<<16)/unit)
		ulong fraction_y=GET_FRACTION(Y),
			ifraction_y=unit-fraction_y;
		uchar *dst_pix0=dst_pix;
		for (long x=dst_rect.x;x<dst_rect.w;x++){
			long x0=X>>16;
			if ((ulong)x0+1>=src.w)
				break;
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
			ulong color[4]={0};
			for (int a=0;a<4;a++){
				if (!weight[a])
					continue;
				for (int b=0;b<4;b++)
					color[b]+=pixel[a][b]*weight[a];
				if (weight[a]==unit)
					break;
			}
			for (int a=0;a<4;a++)
				dst_pix[a]=uchar(color[a]>>16);
			X+=unit;
			dst_pix+=4;
		}
		dst_pix=dst_pix0+dst.pitch;
		Y+=unit;
	}
}

//------------------------------------------------------------------------------

NONS_Surface NONS_Surface::assign_screen(SDL_Surface *s){
	sm.assign_screen(s);
	return get_screen();
}

NONS_Surface NONS_Surface::get_screen(){
	NONS_Surface r;
	*r.data=NONS_Surface_Private(sm.get_screen());
	return r;
}

#define NONS_Surface_DEFINE_RELATIONAL_OP(type,op) \
	bool type::operator op(const type &b) const{ return *this->data->surface op *b.data->surface; }
OVERLOAD_RELATIONAL_OPERATORS2(NONS_Surface,NONS_Surface_DEFINE_RELATIONAL_OP)
OVERLOAD_RELATIONAL_OPERATORS2(NONS_CrippledSurface,NONS_Surface_DEFINE_RELATIONAL_OP)

NONS_SurfaceManager::Surface::Surface(ulong w,ulong h){
	this->set_id();
	size_t n=w*h*4+1;
	this->data=new uchar[n];
	this->w=w;
	this->h=h;
	this->ref_count=0;
	memset(this->data,0,n);
}

NONS_SurfaceManager::Surface::~Surface(){
	delete[] this->data;
}

void NONS_SurfaceManager::Surface::get_properties(NONS_SurfaceProperties &sp) const{
	sp.pixels=this->data;
	sp.w=this->w;
	sp.h=this->h;
	sp.pitch=this->w*4;
	sp.pixel_count=this->h*this->w;
	sp.byte_count=sp.pixel_count*4;
}

NONS_CrippledSurface::NONS_CrippledSurface(const NONS_Surface &original){
	this->data=new NONS_Surface_Private(original.data->surface);
	this->inner=0;
}

const NONS_CrippledSurface &NONS_CrippledSurface::operator=(const NONS_CrippledSurface &original){
	delete this->data;
	delete this->inner;
	this->data=new NONS_Surface_Private(original.data->surface);
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
