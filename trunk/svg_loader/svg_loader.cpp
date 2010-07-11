/*
* Copyright (c) 2009, Helios (helios.vmg@gmail.com)
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

#include "../svg_loader.h"
#include <QtSvg/QtSvg>
#include <vector>

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
const int rmask=0xFF000000;
const int gmask=0x00FF0000;
const int bmask=0x0000FF00;
const int amask=0x000000FF;
#else
const int rmask=0x000000FF;
const int gmask=0x0000FF00;
const int bmask=0x00FF0000;
const int amask=0xFF000000;
#endif

struct surfaceData{
	uchar *pixels;
	uchar Roffset,
		Goffset,
		Boffset,
		Aoffset;
	ulong advance,
		pitch,
		w,h;
	bool alpha;
	surfaceData(){}
	surfaceData(const SDL_Surface *surface){
		*this=surface;
	}
	const surfaceData &operator=(const SDL_Surface *surface){
		this->pixels=(uchar *)surface->pixels;
		this->Roffset=(surface->format->Rshift)>>3;
		this->Goffset=(surface->format->Gshift)>>3;
		this->Boffset=(surface->format->Bshift)>>3;
		this->Aoffset=(surface->format->Ashift)>>3;
		this->advance=surface->format->BytesPerPixel;
		this->pitch=surface->pitch;
		this->w=surface->w;
		this->h=surface->h;
		this->alpha=(this->Aoffset!=this->Roffset && this->Aoffset!=this->Goffset && this->Aoffset!=this->Boffset);
		return *this;
	}
};

struct point{
	double x,y;
};

struct SVG{
	QSvgRenderer renderer;
	double scale_x,
		scale_y,
		rotation;
	bool use_matrix;
	double matrix[4];
	SVG(void *buffer,size_t size);
	void get_dim(double *w,double *h);
	bool scale(double x,double y);
	void best_fit(ulong x,ulong y);
	void rotate(double alpha);
	bool set_matrix(double matrix[4]);
	void transform_coordinates(double x,double y,double *dst_x,double *dst_y);
	bool add_scale(double x,double y);
	SDL_Surface *render();
	SDL_Surface *render(SDL_Surface *dst,double offset_x,double offset_y,uchar alpha);
private:
	void compute_coordinates(point bounding_box[2],point *rect,size_t size);
};

static struct SVG_Manager{
	int temp;
	QApplication a; //We need this to render fonts.
	std::vector<SVG *> array;
	SVG_Manager():temp(0),a(temp,0){
		this->array.push_back(0);
	}
	~SVG_Manager(){
		for (ulong a=1;a<this->array.size();a++)
			if (this->array[a])
				delete this->array[a];
	}
	ulong load(void *buffer,size_t size);
	bool unload(ulong index);
	bool get_dim(ulong index,double *w,double *h);
	bool scale(ulong index,double scale_x,double scale_y);
	bool best_fit(ulong index,ulong max_x,ulong max_y);
	bool rotate(ulong index,double alpha);
	bool set_matrix(ulong index,double matrix[4]);
	bool transform_coordinates(ulong index,double x,double y,double *dst_x,double *dst_y);
	bool add_scale(ulong index,double scale_x,double scale_y);
	SDL_Surface *render(ulong index);
	bool render(ulong index,SDL_Surface *dst,double offset_x,double offset_y,uchar alpha);
} manager;

SVG::SVG(void *buffer,size_t size):renderer(QByteArray((const char *)buffer,size),0){
	this->scale_x=this->scale_y=1;
	this->rotation=0;
	this->use_matrix=0;
}

void SVG::get_dim(double *w,double *h){
	*w=this->renderer.defaultSize().width();
	*h=this->renderer.defaultSize().height();
	point bounding_box[2],
		rect[4]={
			{0,0},
			{0,*h},
			{*w,0},
			{*w,*h},
	};
	this->compute_coordinates(bounding_box,rect,4);
	*w=bounding_box[1].x;
	*h=bounding_box[1].y;
}

bool SVG::scale(double x,double y){
	if (!x || !y)
		return 0;
	this->use_matrix=0;
	this->scale_x=x;
	this->scale_y=y;
	return 1;
}

void SVG::best_fit(ulong x,ulong y){
	if (!x && !y)
		return;
	this->use_matrix=0;
	double w=this->renderer.defaultSize().width(),
		h=this->renderer.defaultSize().height();
	if (x && y){
		double new_ratio=double(x)/double(y),
			old_ratio=w/h;
		if (new_ratio>old_ratio)
			this->scale_x=double(y)/h;
		else
			this->scale_x=double(x)/w;
	}else if (x)
		this->scale_x=double(x)/w;
	else
		this->scale_x=double(y)/h;
	this->scale_y=this->scale_x;
}

void SVG::rotate(double alpha){
	this->use_matrix=0;
	if (alpha>=0)
		alpha=fmod(alpha,360);
	else
		alpha=360-fmod(abs(alpha),360);
	this->rotation=alpha;
}

void invert_matrix(double m[4]){
	double temp=1.0/(m[0]*m[3]-m[1]*m[2]);
	double m2[]={
		temp*m[3],
		temp*-m[1],
		temp*-m[2],
		temp*m[0]
	};
	memcpy(m,m2,4*sizeof(double));
}

bool SVG::set_matrix(double matrix[4]){
	if (!(matrix[0]*matrix[3]-matrix[1]*matrix[2]))
		return 0;
	this->use_matrix=1;
	memcpy(this->matrix,matrix,sizeof(double)*4);
	//invert_matrix(this->matrix);
	return 1;
}

inline void transform_coord(double &x,double &y,double *matrix){
	double temp_x=x,
		temp_y=y;
	x=temp_x*matrix[0]+temp_y*matrix[1];
	y=temp_x*matrix[2]+temp_y*matrix[3];
}

void SVG::compute_coordinates(point bounding_box[2],point *rect,size_t size){
	const double pi=3.1415926535897932384626433832795,
		d2g=pi/180;
	if (!this->use_matrix){
		for (size_t a=0;a<size;a++){
			double angle,radius;
			//rect[a].x=rect[a].x;
			//rect[a].y=rect[a].y;
			if (rect[a].x)
				angle=atan(rect[a].y/rect[a].x);
			else if (rect[a].y>0)
				angle=pi/2;
			else if (rect[a].y<0)
				angle=pi*1.5;
			else
				angle=0;
			radius=sqrt(rect[a].x*rect[a].x+rect[a].y*rect[a].y);
			rect[a].x=cos(angle+this->rotation*d2g)*radius;
			rect[a].y=sin(angle+this->rotation*d2g)*radius;
		}
	}else
		for (size_t a=0;a<size;a++)
			transform_coord(rect[a].x,rect[a].y,this->matrix);
	bounding_box[0]=rect[0];
	bounding_box[1]=rect[0];
	for (int a=1;a<4;a++){
		if (rect[a].x<bounding_box[0].x)
			bounding_box[0].x=rect[a].x;
		if (rect[a].y<bounding_box[0].y)
			bounding_box[0].y=rect[a].y;

		if (rect[a].x>bounding_box[1].x)
			bounding_box[1].x=rect[a].x;
		if (rect[a].y>bounding_box[1].y)
			bounding_box[1].y=rect[a].y;
	}
	for (size_t a=0;a<size;a++){
		rect[a].x-=bounding_box[0].x;
		rect[a].y-=bounding_box[0].y;
	}
	bounding_box[1].x-=bounding_box[0].x;
	bounding_box[1].y-=bounding_box[0].y;
	bounding_box[0].x=0;
	bounding_box[0].y=0;
}

void SVG::transform_coordinates(double x,double y,double *dst_x,double *dst_y){
	double w=this->renderer.defaultSize().width(),
		h=this->renderer.defaultSize().height();
	point rect[]={
		{0,0},
		{w,0},
		{0,h},
		{w,h},
		{x,y}
	};
	point bounding_box[2];
	if (!this->use_matrix){
		{
			QSize size=this->renderer.defaultSize();
			w=size.width()*this->scale_x;
			h=size.height()*this->scale_y;
		}
		//this->get_dim(&w,&h);
		if (!this->rotation);
		else if (this->rotation==90){
			rect[4].x=h-y;
			rect[4].y=x;
		}else if (this->rotation==180){
			rect[4].x=w-x;
			rect[4].y=h-y;
		}else if (this->rotation==270){
			rect[4].x=y;
			rect[4].y=w-x;
		}else
			this->compute_coordinates(bounding_box,rect,5);
	}else
		this->compute_coordinates(bounding_box,rect,5);
	*dst_x=rect[4].x;
	*dst_y=rect[4].y;
}

bool SVG::add_scale(double x,double y){
	if (!x || !y)
		return 0;
	if (!this->use_matrix){
		this->scale_x*=x;
		this->scale_y*=y;
	}else{
		double matrix[]={x,0,0,y},
			res_matrix[4];
		res_matrix[0]=this->matrix[0]*matrix[0]+this->matrix[1]*matrix[2];
		res_matrix[1]=this->matrix[0]*matrix[1]+this->matrix[1]*matrix[3];
		res_matrix[2]=this->matrix[2]*matrix[0]+this->matrix[3]*matrix[2];
		res_matrix[3]=this->matrix[2]*matrix[1]+this->matrix[3]*matrix[3];
		memcpy(this->matrix,res_matrix,sizeof(double)*4);
	}
	return 1;
}

SDL_Surface *SVG::render(){
	return this->render(0,0,0,255);
}

SDL_Surface *SVG::render(SDL_Surface *dst,double offset_x,double offset_y,uchar alpha){
	double w=this->renderer.defaultSize().width(),
		h=this->renderer.defaultSize().height();
	point bounding_box[2]={
		{0,0},
		{w,h}
	};
	point rect[]={
		{0,0},
		{w,0},
		{0,h},
		{w,h}
	};
	QImage *img=0;
	if (!this->use_matrix){
		if (!this->rotation);
		else if (this->rotation==90){
			rect[0].x=h;
			rect[0].y=0;
			bounding_box[1].x=h;
			bounding_box[1].y=w;
		}else if (this->rotation==180){
			rect[0].x=w;
			rect[0].y=h;
		}else if (this->rotation==270){
			rect[0].x=0;
			rect[0].y=w;
			bounding_box[1].x=h;
			bounding_box[1].y=w;
		}else
			this->compute_coordinates(bounding_box,rect,4);
		img=new QImage(bounding_box[1].x*this->scale_x,bounding_box[1].y*this->scale_y,QImage::Format_ARGB32);
		img->fill(0);
		QPainter painter(img);
		painter.scale(this->scale_x,this->scale_y);
		painter.translate(rect[0].x,rect[0].y);
		painter.rotate(this->rotation);
		painter.scale(w/bounding_box[1].x/this->scale_x,h/bounding_box[1].y/this->scale_y);
		this->renderer.render(&painter);
	}else{
		this->compute_coordinates(bounding_box,rect,4);
		img=new QImage(bounding_box[1].x,bounding_box[1].y,QImage::Format_ARGB32);
		img->fill(0);
		QPainter painter(img);
		painter.translate(rect[0].x,rect[0].y);
		painter.setWorldMatrix(QMatrix(this->matrix[0],this->matrix[2],this->matrix[1],this->matrix[3],0,0),1);
		painter.scale(w/bounding_box[1].x,h/bounding_box[1].y);
		this->renderer.render(&painter);
	}
	if (!dst)
		dst=SDL_CreateRGBSurface(SDL_SRCALPHA,img->width(),img->height(),32,rmask,gmask,bmask,amask);
	
	{
		SDL_LockSurface(dst);
		surfaceData sd=dst;
		long ox=offset_x,
			oy=offset_y;
		uchar *pix=sd.pixels+oy*sd.pitch+ox*sd.advance;
		for (long y=oy,src_y=0,h=img->height();y<(long)sd.h && src_y<h;y++,src_y++){
			QRgb *scanline=(QRgb *)img->scanLine(src_y);
			uchar *pix0=pix;
			if (y>=0){
				for (long x=ox,src_x=0,w=img->width();x<(long)sd.w && src_x<w;x++,src_x++){
					if (x>=0){
						ulong
							r0=qRed(scanline[src_x]),
							g0=qGreen(scanline[src_x]),
							b0=qBlue(scanline[src_x]),
							a0=qAlpha(scanline[src_x]);
						uchar
							*r1=pix+sd.Roffset,
							*g1=pix+sd.Goffset,
							*b1=pix+sd.Boffset,
							*a1=pix+sd.Aoffset;
#define INTEGER_MULTIPLICATION(a,b) (((a)*(b))/255)
#define APPLY_ALPHA(c0,c1,a) (INTEGER_MULTIPLICATION((a)^0xFF,(c1))+INTEGER_MULTIPLICATION((a),(c0)))
						if (alpha<255)
							a0=INTEGER_MULTIPLICATION(a0,alpha);
						{
							ulong el;
							ulong previous=*a1;
							*a1=(uchar)INTEGER_MULTIPLICATION(a0^0xFF,*a1^0xFF)^0xFF;
							el=(!a0 && !previous)?0:(a0*255)/(*a1);
							*r1=(uchar)APPLY_ALPHA(r0,*r1,el);
							*g1=(uchar)APPLY_ALPHA(g0,*g1,el);
							*b1=(uchar)APPLY_ALPHA(b0,*b1,el);
						}
					}
					pix+=sd.advance;
				}
			}
			pix=pix0+sd.pitch;
		}
		SDL_UnlockSurface(dst);
	}
	delete img;
	return dst;
}

ulong SVG_Manager::load(void *buffer,size_t size){
	SVG *s=new SVG(buffer,size);
	if (!s->renderer.isValid()){
		delete s;
		return 0;
	}
	for (ulong a=1;a<this->array.size();a++){
		if (!this->array[a]){
			this->array[a]=s;
			return a;
		}
	}
	this->array.push_back(s);
	return this->array.size()-1;
}

bool SVG_Manager::unload(ulong index){
	if (index>=this->array.size() || !this->array[index])
		return 0;
	delete this->array[index];
	this->array[index]=0;
	return 1;
}

bool SVG_Manager::get_dim(ulong index,double *w,double *h){
	if (index>=this->array.size() || !this->array[index])
		return 0;
	this->array[index]->get_dim(w,h);
	return 1;
}

bool SVG_Manager::scale(ulong index,double scale_x,double scale_y){
	if (index>=this->array.size() || !this->array[index])
		return 0;
	return this->array[index]->scale(scale_x,scale_y);
}

bool SVG_Manager::best_fit(ulong index,ulong max_x,ulong max_y){
	if (index>=this->array.size() || !this->array[index])
		return 0;
	this->array[index]->best_fit(max_x,max_y);
	return 1;
}

bool SVG_Manager::rotate(ulong index,double alpha){
	if (index>=this->array.size() || !this->array[index])
		return 0;
	this->array[index]->rotate(alpha);
	return 1;
}

bool SVG_Manager::set_matrix(ulong index,double matrix[4]){
	if (index>=this->array.size() || !this->array[index])
		return 0;
	return this->array[index]->set_matrix(matrix);
}

bool SVG_Manager::transform_coordinates(ulong index,double x,double y,double *dst_x,double *dst_y){
	if (index>=this->array.size() || !this->array[index])
		return 0;
	this->array[index]->transform_coordinates(x,y,dst_x,dst_y);
	return 1;
}

bool SVG_Manager::add_scale(ulong index,double scale_x,double scale_y){
	if (index>=this->array.size() || !this->array[index])
		return 0;
	return this->array[index]->add_scale(scale_x,scale_y);
}

SDL_Surface *SVG_Manager::render(ulong index){
	if (index>=this->array.size() || !this->array[index])
		return 0;
	return this->array[index]->render();
}

bool SVG_Manager::render(ulong index,SDL_Surface *dst,double offset_x,double offset_y,uchar alpha){
	if (index>=this->array.size() || !this->array[index])
		return 0;
	return this->array[index]->render(dst,offset_x,offset_y,alpha);
}



SVG_DLLexport ulong SVG_load(void *buffer,size_t size){
	return manager.load(buffer,size);
}

SVG_DLLexport int SVG_unload(ulong index){
	return manager.unload(index);
}

SVG_DLLexport int SVG_get_dimensions(ulong index,double *w,double *h){
	return manager.get_dim(index,w,h);
}

SVG_DLLexport int SVG_set_scale(ulong index,double scale_x,double scale_y){
	return manager.scale(index,scale_x,scale_y);
}

SVG_DLLexport int SVG_best_fit(ulong index,ulong max_x,ulong max_y){
	return manager.best_fit(index,max_x,max_y);
}

SVG_DLLexport int SVG_set_rotation(ulong index,double angle){
	return manager.rotate(index,angle);
}

SVG_DLLexport int SVG_set_matrix(ulong index,double matrix[4]){
	return manager.set_matrix(index,matrix);
}

SVG_DLLexport int SVG_transform_coordinates(ulong index,double x,double y,double *dst_x,double *dst_y){
	return manager.transform_coordinates(index,x,y,dst_x,dst_y);
}

SVG_DLLexport int SVG_add_scale(ulong index,double scale_x,double scale_y){
	return manager.add_scale(index,scale_x,scale_y);
}

SVG_DLLexport SDL_Surface *SVG_render(ulong index){
	return manager.render(index);
}

SVG_DLLexport int SVG_render2(ulong index,SDL_Surface *dst,double offset_x,double offset_y,uchar alpha){
	return manager.render(index,dst,offset_x,offset_y,alpha);
}
