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

#include <fstream>
#include <vector>
#include <cstring>
#include <cmath>
#include "Archive.h"
//#include "DataStreams.h"
#include <zlib.h>
#include <bzlib.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/operations.hpp>

#define NONS_SYS_WINDOWS (defined _WIN32 || defined _WIN64)
#define NONS_SYS_UNIX (defined __unix__ || defined __unix)

#define COMPRESSION_NONE	0
#define COMPRESSION_SPB		1
#define COMPRESSION_LZSS	2
#define COMPRESSION_BZ2		4
#define COMPRESSION_DEFLATE	5
#define COMPRESSION_AUTOH	-1
#define COMPRESSION_AUTOL	-2
#define COMPRESSION_DEFAULT	COMPRESSION_AUTOL

template <typename T> inline void zero_structure(T &s){ memset(&s,0,sizeof(s)); }

struct Options{
	bool good;
	char mode;
	std::wstring outputFilename;
	ulong compressionType;
	std::vector<std::pair<std::wstring,bool> > inputFilenames;
	Options(char **argv){
		static std::pair<const wchar_t *,ulong> compressions[]={
			std::make_pair(L"none",COMPRESSION_NONE),
			std::make_pair(L"bz2",COMPRESSION_BZ2),
			std::make_pair(L"deflate",COMPRESSION_DEFLATE),
			std::make_pair(L"autoh",COMPRESSION_AUTOH),
			std::make_pair(L"autol",COMPRESSION_AUTOL),
			std::make_pair((const wchar_t *)0,0)
		};
		static const char *options[]={
			"-h",
			"-?",
			"--help",
			"--version",
			"-o",
			"-c",
			"-r",
			"e",
			"c",
			"l",
			0
		};
		this->compressionType=COMPRESSION_DEFAULT;
		this->good=0;
		if (!*++argv){
			this->mode='h';
			this->good=1;
			return;
		}
		bool nextIsOutput=0,
			nextIsCompression=0,
			nextIsSkip=0;
		for (;*argv;argv++){
			long option=-1;
			for (ulong a=0;options[a] && option<0;a++)
				if (!strcmp(*argv,options[a]))
					option=a;
			switch (option){
				case 0: //-h
				case 1: //-?
				case 2: //--help
					this->mode='h';
					this->good=1;
					return;
				case 3: //--version
					this->mode='v';
					this->good=1;
					return;
				case 4: //-o
					if (!(nextIsOutput || nextIsCompression || nextIsSkip)){
						nextIsOutput=1;
						break;
					}
				case 5: //-c
					if (!(nextIsOutput || nextIsCompression || nextIsSkip)){
						nextIsCompression=1;
						break;
					}
				case 6: //-r
					if (!(nextIsOutput || nextIsCompression || nextIsSkip)){
						nextIsSkip=1;
						break;
					}
				case 7: //e
				case 8: //c
				case 9: //l
					this->mode=**argv;
					break;
				default:
					if (nextIsOutput){
						this->outputFilename=UniFromUTF8(*argv);
						nextIsOutput=0;
					}else if (nextIsCompression){
						std::wstring s=UniFromUTF8(*argv);
						bool b=0;
						for (ulong a=0;compressions[a].first && !b;a++){
							if (s==compressions[a].first){
								this->compressionType=compressions[a].second;
								b=1;
							}
						}
						if (!b)
							std::cerr <<"Unrecognized compression mode."<<std::endl;
						nextIsCompression=0;
					}else{
						this->inputFilenames.push_back(std::make_pair(UniFromUTF8(*argv),nextIsSkip));
						nextIsSkip=0;
					}
			}
		}
		this->good=1;
	}
};

std::string humanReadableSize(ulong size){
	static const char *prefixes[]={"","Ki","Mi","Gi","Ti","Pi","Ei","Zi","Yi"};
	double d=size;
	int prefix=0;
	while (d>=1024){
		d/=1024;
		prefix++;
	}
	std::stringstream stream;
	stream <<floor(d*100)/100<<' '<<prefixes[prefix]<<'B';
	return stream.str();
}

bool getbit(void *arr,size_t &byteoffset,size_t &bitoffset){
	uchar *buffer=(uchar *)arr;
	bool res=(buffer[byteoffset]>>(7-bitoffset))&1;
	bitoffset++;
	if (bitoffset>7){
		byteoffset++;
		bitoffset=0;
	}
	return res;
}

ulong getbits(void *arr,uchar bits,size_t &byteoffset,size_t &bitoffset){
	uchar *buffer=(uchar *)arr;
	ulong res=0;
	if (bits>sizeof(ulong)*8)
		bits=sizeof(ulong)*8;
	for (;bits>0;bits--){
		res<<=1;
		res|=(ulong)getbit(buffer,byteoffset,bitoffset);
	}
	return res;
}

char *compressBuffer_BZ2(void *src,size_t srcl,size_t &dstl){
	unsigned int l=srcl,
		realsize=l;
	char *dst=new char[l];
	while (BZ2_bzBuffToBuffCompress(dst,&l,(char *)src,srcl,9,0,30)==BZ_OUTBUFF_FULL){
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
	dstl=l;
	return dst;
}

struct sarExtra{
	size_t offset,
		compression,
		compressed_length,
		uncompressed_length;
	sarExtra()
		:offset(0),
		compression(0),
		compressed_length(0),
		uncompressed_length(0){}
};

class sarArchive:public Archive<sarExtra>{
	File file;
	Path file_source;
	bool constructed_for_extracting;
	ulong total,
		progress;
public:
	bool good;
	sarArchive():constructed_for_extracting(0),good(1){}
	sarArchive(const std::wstring &filename,bool nsa);
	~sarArchive(){}
	void write(const std::wstring &outputFilename,ulong compression);
	void write(){}
	void write(const Path &src,const std::wstring &dst,bool dir){}
	void list();
	void extract(const std::wstring &outputFilename);
};

sarArchive::sarArchive(const std::wstring &filename,bool nsa){
	this->good=0;
	this->constructed_for_extracting=1;
	this->file_source=boost::filesystem::complete(filename);
	this->file.open(this->file_source.string(),1);
	if (!this->file)
		return;
	size_t l=6;
	uchar *buffer=this->file.read(l,l,0);
	if (!buffer)
		return;
	if (l<6){
		delete[] buffer;
		return;
	}
	size_t offset=0;
	ulong n=readBigEndian(2,buffer,offset),
		file_data_start=readBigEndian(4,buffer,offset);
	delete[] buffer;
	l=file_data_start-offset;
	buffer=this->file.read(l,l,0);
	if (l<file_data_start-offset){
		delete[] buffer;
		return;
	}
	offset=0;
	for (ulong a=0;a<n;a++){
		TreeNode<sarExtra> *new_node;
		{
			std::string path=(char *)(buffer+offset);
			offset+=path.size()+1;
			new_node=this->root.get_branch(UniFromSJIS(path),1);
		}
		if (nsa)
			new_node->extraData.compression=readByte(buffer,offset);
		else
			new_node->extraData.compression=COMPRESSION_NONE;
		new_node->extraData.offset=file_data_start+readBigEndian(4,buffer,offset);
		new_node->extraData.compressed_length=readBigEndian(4,buffer,offset);
		if (nsa)
			new_node->extraData.uncompressed_length=readBigEndian(4,buffer,offset);
		else
			new_node->extraData.uncompressed_length=new_node->extraData.compressed_length;
	}
	delete[] buffer;
	this->good=1;
}

//LZSS compression is discontinued. This implementation is only kept for
//future reference.
static const ulong NN=8,
	MAXS=4;
#if 0
void writeBits(uchar *buffer,ulong *byteOffset,ulong *bitOffset,ulong data,ushort s){
	ulong byteo=*byteOffset,
		bito=*bitOffset;
	data<<=(sizeof(data)*8-s)-bito;
	buffer+=byteo;
	for (ushort a=0;a<sizeof(data) && data;a++){
		*(buffer++)|=data>>(sizeof(data)-1)*8;
		data<<=8;
	}
	bito+=s;
	*bitOffset=bito%8;
	byteo+=bito/8;
	*byteOffset=byteo;
}

ulong compare(void *current,void *test,ulong max){
	uchar *A=(uchar *)current,
		*B=(uchar *)test;
	for (ulong a=0;a<max;a++)
		if (A[a]!=B[a] || B+a>=A)
			return a;
	return max;
}

char *encode_LZSS(void *src,size_t srcl,size_t &dstl){
	//NN is the size in bits of the offset component of a keyword.
	ulong window_bits=NN,
		//We determine the size of the window by doing 2^NN
		window_size=1<<window_bits,
		//MAXS is the size in bits of the size component of a keyword.
		max_string_bits=MAXS,
		//Right now, threshold contains the size of a keyword plus the size of a flag
		threshold=1+window_bits+max_string_bits,
		//Again, determine the size of something by raising 2 to some power.
		max_string_len=(1<<max_string_bits);
	uchar *buffer=(uchar *)src;
	//Transform threshold into the minimum sequence size in bytes that will be
	//considered a match. Considering a sequence smaller than this a match will
	//inflate the data.
	threshold=(threshold>>3)+!!(threshold&7);
	//Because the smallest sequence we can store is not zero, we can change the
	//meaning of size zero in a keyword. Now, a size of zero found in a keyword
	//translates to an actual size of threshold.
	//I think I made an off-by-one error here. I'm not sure.
	max_string_len+=threshold-1;
	//No matter how much entropy the input contains, it will not be inflated by
	//more than srcl/8+(srcl%8!=0)
	ulong res_size=srcl+(srcl>>3)+!!(srcl&7);
	uchar *res=new uchar[res_size];
	memset(res,0,res_size);
	//Offsets in bytes and bits. bit<=7 should always be true.
	ulong byte=0,
		bit=0;
	ulong offset=window_size-max_string_len;
	
	//I'll explain these lines later on.
	std::vector<ulong> tree[256];
	ulong starts[256];
	for (ulong a=0;a<srcl;a++)
		tree[buffer[a]].push_back(a);
	memset(starts,0,256*sizeof(ulong));
	
	//We will be advancing a manually, so a++ is unnecessary.
	for (ulong a=0;a<srcl;){
		long found=-1,max=-1;
		ulong found_size=0;
		//What we do in this loop is look for matches in the past input. We do
		//this by using tree[256], an array of vectors that contain offsets
		//where byte values appear. starts[256] stores offsets within each
		//vector. Below this offset, the offsets stored by the vector are
		//already useless. Using starts is *much* faster than using
		//std::vector::erase().
		for (ulong b=starts[buffer[a]],size=tree[buffer[a]].size();b<size;b++){
			//element is the offset of a possible match.
			ulong &element=tree[buffer[a]][b];
			//Is element outside of the sliding window?
			if (a>=window_size && element<a-window_size){
				//It is. Increment starts and try again.
				starts[buffer[a]]++;
				continue;
			}
			//Is element in the future?
			if (element>=a)
				//It is. We can't store offsets to sequences we haven't seen
				//yet, so that means there are no matches.
				break;
			//compare() compares two buffers and returns how many bytes from the
			//start they have in common
			found_size=compare(buffer+a,buffer+element,max_string_len);
			//Do the buffers have in common so few bytes that it would produce
			//inflation?
			if (found_size<threshold)
				//Yes. Try next match.
				continue;
			//We're trying to find the biggest match possible.
			if (max<0 || found_size>(ulong)max){
				found=element;
				max=found_size;
				if (max==max_string_len)
					//If we're already at the biggest match allowed by the
					//algorithm, stop looking for matches.
					break;
			}
		}
		found_size=max;
		if (found<0){
			//We couldn't find a match, so write flag 1 and the byte.
			writeBits(res,&byte,&bit,1,1);
			writeBits(res,&byte,&bit,buffer[a],8);
			a++;
		}else{
			//We found a match, write flag 0...
			writeBits(res,&byte,&bit,0,1);
			ulong pos=(found+offset)%window_size;
			//...the offset within the sliding window...
			writeBits(res,&byte,&bit,pos,(ushort)window_bits);
			//...and the size of the match. Here we're taking advantage of
			//knowing the size of the smallest possible match.
			writeBits(res,&byte,&bit,found_size-threshold,(ushort)max_string_bits);
			a+=found_size;
		}
	}
	//We had to write at least one more bit, so the output size is incremented.
	//This is the reason for the !!(srcl&7) above.
	byte+=!!bit;
	dstl=byte;
	return (char *)res;
}
#endif

struct sarArchiveWrite_param{
	File *file;
	std::string buffer;
	ulong step,
		total,
		progress;
	std::vector<ulong> compression_offsets;
	ulong compression_used;
	sarArchiveWrite_param(File *f):file(f),step(0),compression_used(COMPRESSION_DEFAULT){}
};

void compression_helper(char *(*f)(void *,size_t,size_t &),char **r,void *a,size_t b,size_t *c){
	*r=f(a,b,*c);
}

ulong figure_out_compression(std::string extension,Uint64 filesize,long compression){
	if (compression>=COMPRESSION_NONE || !filesize)
		return compression;
	size_t dot=extension.rfind('.');
	if (dot!=extension.npos)
		extension=extension.substr(dot+1);
	else
		extension.clear();
	tolower(extension);
	//files with these extensions will not be compressed
	static const char *compressed_types[]={
		"gif","jpeg","jpg","tga","tif","tiff","svgz","png",
		"ogg","mp3","it","xm","s3m","aiff","flac","669","med","voc","mka",
		"mkv","avi","mpeg","mpg","mp4","flv",
		0
	};
	for (const char **p=compressed_types;*p;p++)
		if (extension==*p)
			return COMPRESSION_NONE;
	return (compression==COMPRESSION_AUTOH)?COMPRESSION_BZ2:COMPRESSION_DEFLATE;
}

namespace compression{
	struct base_in_decompression{
		in_func f;
		uchar *in_buffer;
		size_t remaining;
		Uint64 processed;
		base_in_decompression():in_buffer(0),processed(0),f(0){}
		virtual ~base_in_decompression(){}
		virtual bool eof()=0;
	};
	struct base_out_decompression{
		static const ulong bits=15,
			size=1<<bits;
		out_func f;
		std::vector<uchar> out;
		Uint64 processed;
		base_out_decompression():out(size),processed(0),f(0){}
		virtual ~base_out_decompression(){}
	};
	struct decompress_from_file:public base_in_decompression{
		File *file;
		Uint64 offset;
		std::vector<uchar> in;
		decompress_from_file():base_in_decompression(),offset(0){
			this->f=in_f;
		}
		virtual bool eof(){
			return this->offset==this->file->filesize();
		}
		static unsigned in_f(void *p,unsigned char **buffer){
			decompress_from_file *_this=(decompress_from_file *)p;
			size_t l=1<<12;
			_this->in.resize(l);
			_this->file->read(&_this->in[0],l,l,_this->offset);
			_this->in.resize(l);
			if (!l){
				_this->in_buffer=0;
				if (buffer)
					*buffer=0;
				_this->remaining=0;
				return 0;
			}
			_this->in_buffer=&_this->in[0];
			if (buffer)
				*buffer=_this->in_buffer;
			_this->remaining=l;
			_this->offset+=l;
			_this->processed+=l;
			return l;
		}
	};
	struct decompress_to_file:public base_out_decompression{
		File *file;
		decompress_to_file():base_out_decompression(){
			this->f=out_f;
		}
		static int out_f(void *p,unsigned char *buffer,unsigned size){
			((decompress_to_file *)p)->file->write(buffer,size);
			((decompress_to_file *)p)->processed+=size;
			return 0;
		}
	};
	bool simple_copy(base_out_decompression *dst,base_in_decompression *src){
		in_func in=src->f;
		out_func out=dst->f;
		while (1){
			unsigned a=in(src,0);
			if (!a)
				break;
			out(dst,src->in_buffer,a);
		}
		return 1;
	}
	bool DecompressBZ2(base_out_decompression *dst,base_in_decompression *src){
		bz_stream stream;
		zero_structure(stream);

		stream.next_in=0;
		stream.avail_in=0;
		stream.next_out=(char *)&dst->out[0];
		stream.avail_out=dst->size;
		if (BZ2_bzDecompressInit(&stream,0,0)!=BZ_OK)
			return 0;
		int res;
		in_func in=src->f;
		out_func out=dst->f;
		while ((res=BZ2_bzDecompress(&stream))==BZ_OK){
			if (!stream.avail_out){
				out(dst,&dst->out[0],dst->size);
				stream.next_out=(char *)&dst->out[0];
				stream.avail_out=dst->size;
			}
			if (!stream.avail_in && !(stream.avail_in=in(src,(uchar **)&stream.next_in))){
				while (1){
					BZ2_bzDecompress(&stream);
					if (stream.avail_out<dst->size){
						out(dst,&dst->out[0],dst->size);
						stream.next_out=(char *)&dst->out[0];
						stream.avail_out=dst->size;
					}else
						break;
				}
				break;
			}
		}
		bool ret=1;
		if (res!=BZ_STREAM_END)
			ret=0;
		else if (stream.avail_out<dst->size)
			out(dst,&dst->out[0],dst->size-stream.avail_out);
		BZ2_bzDecompress(&stream);
		return ret;
	}
	bool CompressBZ2(base_out_decompression *dst,base_in_decompression *src){
		bz_stream stream;
		zero_structure(stream);

		in_func in=src->f;
		out_func out=dst->f;
		stream.avail_in=in(src,(uchar **)&stream.next_in);
		stream.next_out=(char *)&dst->out[0];
		stream.avail_out=dst->size;
		if (BZ2_bzCompressInit(&stream,4,0,0)!=BZ_OK)
			return 0;
		int res;
		int action=BZ_RUN;
		while (1){
			res=BZ2_bzCompress(&stream,action);
			if (res!=BZ_OK && res!=BZ_RUN_OK && res!=BZ_FINISH_OK)
				break;
			if (!stream.avail_out){
				out(dst,&dst->out[0],dst->size);
				stream.next_out=(char *)&dst->out[0];
				stream.avail_out=dst->size;
			}
			if (!stream.avail_in)
				if (!(stream.avail_in=in(src,(uchar **)&stream.next_in)))
					action=BZ_FINISH;
		}
		bool ret=1;
		if (res!=BZ_STREAM_END)
			ret=0;
		else if (stream.avail_out<dst->size)
			out(dst,&dst->out[0],dst->size-stream.avail_out);
		BZ2_bzCompressEnd(&stream);
		return ret;
	}
	bool DecompressDEFLATE(base_out_decompression *dst,base_in_decompression *src){
		z_stream stream;
		zero_structure(stream);

		if (inflateBackInit(&stream,dst->bits,&dst->out[0])!=Z_OK)
			return 0;
		int res=inflateBack(
			&stream,
			src->f,
			src,
			dst->f,
			dst
		);
		inflateBackEnd(&stream);
		return res==Z_STREAM_END;
	}
	bool CompressDEFLATE(base_out_decompression *dst,base_in_decompression *src){
		z_stream stream;
		zero_structure(stream);

		if (deflateInit2(&stream,9,Z_DEFLATED,-15,9,Z_DEFAULT_STRATEGY)!=Z_OK)
			return 0;
		in_func in=src->f;
		out_func out=dst->f;
		stream.avail_in=in(src,(uchar **)&stream.next_in);
		stream.next_out=(Bytef *)&dst->out[0];
		stream.avail_out=dst->size;
		int res,
			flush=Z_NO_FLUSH;
		while ((res=deflate(&stream,flush))==Z_OK){
			if (!stream.avail_out){
				out(dst,&dst->out[0],dst->size);
				stream.next_out=(Bytef *)&dst->out[0];
				stream.avail_out=dst->size;
			}
			if (!stream.avail_in)
				stream.avail_in=in(src,(uchar **)&stream.next_in);
			if (src->eof())
				flush=Z_FINISH;
		}
		bool ret=1;
		if (res<0)
			ret=0;
		else{
			if (stream.avail_out<dst->size){
				out(dst,&dst->out[0],dst->size-stream.avail_out);
				stream.next_out=(Bytef *)&dst->out[0];
				stream.avail_out=dst->size;
			}
		}
		deflateEnd(&stream);
		return ret;
	}
	typedef bool (*compression_f)(base_out_decompression *,base_in_decompression *);
}

void sarArchiveWrite(const std::wstring &ex_path,const std::wstring &in_path,bool is_dir,const sarExtra &extraData,sarArchiveWrite_param &params){
	if (is_dir)
		return;
	if (!params.step){
		std::wstring path_temp=in_path;
		tolower(path_temp);
		tobackslash(path_temp);
		std::string sjis=UniToSJIS(path_temp);
		params.buffer.append(sjis);
		writeByte(params.buffer,0); //terminating zero
		params.compression_offsets.push_back(params.buffer.size());
		writeByte(params.buffer,0);
		writeBigEndian(4,params.buffer,0);
		writeBigEndian(4,params.buffer,0);
		writeBigEndian(4,params.buffer,0);
	}else{
		(std::cout <<'(').width(2);
		std::cout <<(params.progress++*100/params.total)<<"%) "<<UniToUTF8(in_path)<<"...\n";
		File file(ex_path,1);
		if (file.filesize()>0xFFFFFFFF)
			std::cerr <<UniToUTF8(in_path)<<" is too large and cannot be contained by the archive format. Skipping.\n";
		else{
			size_t raw_l=(size_t)file.filesize();
			ulong compression=figure_out_compression(UniToUTF8(ex_path),file.filesize(),params.compression_used);
			
			compression::decompress_from_file dff;
			compression::decompress_to_file dtf;
			dff.file=&file;
			dtf.file=params.file;
			compression::compression_f f;

			size_t size_before_compression=(size_t)params.file->filesize();

			switch (compression){
				case COMPRESSION_NONE:
					f=compression::simple_copy;
					break;
				case COMPRESSION_BZ2:
					f=compression::CompressBZ2;
					{
						std::string temp;
						writeBigEndian(4,temp,(ulong)file.filesize());
						file.write(&temp[0],temp.size());
					}
					break;
				case COMPRESSION_DEFLATE:
					f=compression::CompressDEFLATE;
					break;
			}
			f(&dtf,&dff);

			file.close();

			ulong offset=params.compression_offsets[params.step++-1];
			params.buffer[offset]=(char)compression;
			offset+=1;
			writeBigEndian(4,params.buffer,size_before_compression-params.buffer.size(),offset);
			offset+=4;
			writeBigEndian(4,params.buffer,(size_t)dtf.processed,offset);
			offset+=4;
			writeBigEndian(4,params.buffer,(size_t)dff.processed,offset);
		}
	}
}

void sarArchive::write(const std::wstring &outputFilename,ulong compression){
	File file(outputFilename,0);
	sarArchiveWrite_param params(&file);
	params.total=this->root.count(0);
	params.progress=0;
	params.compression_used=compression;
	writeBigEndian(2,params.buffer,params.total);
	writeBigEndian(4,params.buffer,0);
	this->root.foreach<sarArchiveWrite_param,sarArchiveWrite>(L"",L"",params);
	writeBigEndian(4,params.buffer,params.buffer.size(),2);
	file.write(&params.buffer[0],params.buffer.size());
	params.step++;
	this->root.foreach<sarArchiveWrite_param,sarArchiveWrite>(L"",L"",params);
	file.write(&params.buffer[0],params.buffer.size(),0);
	std::cout <<"Done."<<std::endl;
}

void listSARfunction(const std::wstring &ex_path,const std::wstring &in_path,bool is_dir,const sarExtra &extraData,void *&){
	static const char *methods[]={
		"No",
		"SPB",
		"LZSS",
		0,
		"BZ2",
		"DEFLATE",
	};
	if (!is_dir)
		std::cout <<UniToUTF8(in_path)<<"\n"
			"\tCompressed:   "<<humanReadableSize(extraData.compressed_length)<<"\n"
			"\tUncompressed: "<<humanReadableSize(extraData.uncompressed_length)
				<<"\tCompression ratio:"
				<<double(extraData.compressed_length)/double(extraData.uncompressed_length)*100.0<<"%\n"
			"\t"<<methods[extraData.compression]<<" compression."<<std::endl;
}

void sumSizes(const std::wstring &ex_path,const std::wstring &in_path,bool is_dir,const sarExtra &extraData,ulong *&sizes){
	if (is_dir)
		return;
	sizes[0]+=extraData.compressed_length;
	sizes[1]+=extraData.uncompressed_length;
}

void sarArchive::list(){
	if (!this->good || !this->constructed_for_extracting)
		return;
	void *p;
	this->root.foreach<void *,listSARfunction>(L"",L"",p);
	ulong sizes[2]={0};
	{
		ulong *cast=sizes;
		this->root.foreach<ulong *,sumSizes>(L"",L"",cast);
	}
	std::cout
		<<"\nFile count: "<<this->root.count(0)
		<<"\nTotal size of the archive (compressed):   "<<humanReadableSize(sizes[0])
		<<"\n                          (uncompressed): "<<humanReadableSize(sizes[1])
		<<"\nCompression ratio: "<<double(sizes[0])/double(sizes[1])<<std::endl;
}

char *decode_SPB(char *buffer,ulong compressedSize,ulong decompressedSize){
	size_t ioffset=0;
	ulong width=readBigEndian(2,buffer,ioffset),
		height=readBigEndian(2,buffer,ioffset);
	ulong width_pad=(4-width*3%4)%4,
		original_length=(width*3+width_pad)*height+54;
	char *res=new char[original_length];
	memset(res,0,original_length);
	size_t ooffset=0;
	writeByte(res,'B',ooffset);
	writeByte(res,'M',ooffset);
	writeLittleEndian(4,res,original_length,ooffset);
	ooffset=10;
	writeByte(res,54,ooffset);
	ooffset=14;
	writeByte(res,40,ooffset);
	ooffset=18;
	writeLittleEndian(4,res,width,ooffset);
	writeLittleEndian(4,res,height,ooffset);
	writeLittleEndian(2,res,1,ooffset);
	writeByte(res,24,ooffset);
	ooffset=34;
	writeLittleEndian(4,res,original_length-54,ooffset);
	size_t ibitoffset=0;
	char *buf=res+54;
	ulong surface=width*height,
		decompressionbufferlen=surface+4;
	char *decompressionbuffer=new char[decompressionbufferlen];
	ooffset=54;
	for (int a=0;a<3;a++){
		ulong count=0;
		uchar x=(uchar)getbits(buffer,8,ioffset,ibitoffset);
		decompressionbuffer[count++]=x;
		while (count<surface){
			uchar n=(uchar)getbits(buffer,3,ioffset,ibitoffset);
			if (!n){
				for (int b=4;b;b--)
					decompressionbuffer[count++]=x;
				continue;
			}
			uchar m;
			if (n==7)
				m=getbit(buffer,ioffset,ibitoffset)+1;
			else
				m=n+2;
			for (int b=4;b;b--){
				if (m==8)
					x=(uchar)getbits(buffer,8,ioffset,ibitoffset);
				else{
					ulong k=(ulong)getbits(buffer,m,ioffset,ibitoffset);
					if (k&1)
						x+=uchar((k>>1)+1);
					else
						x-=uchar(k>>1);
				}
				decompressionbuffer[count++]=x;
			}
		}
		char *pbuf=buf+(width*3+width_pad)*(height-1)+a;
		char *psbuf=decompressionbuffer;
		for (ulong b=0;b<height;b++){
			if (b&1){
				for (ulong c=0;c<width;c++,pbuf-=3)
					*pbuf=*psbuf++;
				pbuf-=width*3+width_pad-3;
			}else{
				for (ulong c=0;c<width;c++,pbuf+=3)
					*pbuf=*psbuf++;
				pbuf-=width*3+width_pad+3;
			}
		}
		long b=0;
		for (long y0=height-1;y0>=0;y0--){
			if (y0&1){
				for (ulong x0=0;x0<width;x0++)
					buf[a+x0*3+y0*(width*3+width_pad)]=decompressionbuffer[b++];
			}else{
				for (long x0=(long)width-1;x0>=0;x0--)
					buf[a+x0*3+y0*(width*3+width_pad)]=decompressionbuffer[b++];
			}
		}
	}
	delete[] decompressionbuffer;
	return res;
}

char *decode_LZSS(char *buffer,ulong compressedSize,ulong decompressedSize){
	uchar decompression_buffer[256*2];
	ulong decompresssion_buffer_offset=239;
	memset(decompression_buffer,0,239);
	uchar *res=new uchar[decompressedSize];
	size_t byteoffset=0;
	size_t bitoffset=0;
	for (ulong len=0;len<decompressedSize;){
		if (getbit(buffer,byteoffset,bitoffset)){
			uchar a=(uchar)getbits(buffer,8,byteoffset,bitoffset);
			res[len++]=a;
			decompression_buffer[decompresssion_buffer_offset++]=a;
			decompresssion_buffer_offset&=0xFF;
		}else{
			uchar a=(uchar)getbits(buffer,8,byteoffset,bitoffset);
			uchar b=(uchar)getbits(buffer,4,byteoffset,bitoffset);
			for (long c=0;c<=b+1 && len<decompressedSize;c++){
				uchar d=decompression_buffer[(a+c)&0xFF];
				res[len++]=d;
				decompression_buffer[decompresssion_buffer_offset++]=d;
				decompresssion_buffer_offset&=0xFF;
			}
		}
	}
	return (char *)res;
}

char *decode_LZSS2(char *buffer,ulong compressedSize,ulong decompressedSize){
	ulong window_bits=NN,
		window_size=1<<window_bits,
		max_string_bits=MAXS,
		threshold=1+window_bits+max_string_bits,
		max_string_len=(1<<max_string_bits);
	threshold=(threshold>>3)+!!(threshold&7);
	max_string_len+=threshold-1;
	ulong offset=window_size-max_string_len;

	uchar *decompression_buffer=new uchar[window_size];
	ulong decompression_buffer_offset=offset;
	memset(decompression_buffer,0,window_size);
	uchar *res=new uchar[decompressedSize];
	size_t byteoffset=0;
	size_t bitoffset=0;
	ulong len;
	for (len=0;len<decompressedSize;){
		if (getbit(buffer,byteoffset,bitoffset)){
			uchar a=(uchar)getbits(buffer,8,byteoffset,bitoffset);
			res[len++]=a;
			decompression_buffer[decompression_buffer_offset++]=a;
			decompression_buffer_offset%=window_size;
		}else{
			ulong a=(uchar)getbits(buffer,(uchar)window_bits,byteoffset,bitoffset);
			ulong b=(uchar)getbits(buffer,(uchar)max_string_bits,byteoffset,bitoffset)+threshold;
			for (ulong c=0;c<b;c++){
				uchar d=decompression_buffer[(a+c)%window_size];
				res[len++]=d;
				decompression_buffer[decompression_buffer_offset++]=d;
				decompression_buffer_offset%=window_size;
			}
		}

	}
	delete[] decompression_buffer;
	return (char *)res;
}

static uchar *decompress(uchar *src,ulong srcl,ulong dstl,ulong method){
	uchar *ret;
	switch (method){
		case COMPRESSION_SPB:
			ret=(uchar *)decode_SPB((char *)src,srcl,dstl);
			delete[] src;
			break;
		case COMPRESSION_LZSS:
			ret=(uchar *)decode_LZSS((char *)src,srcl,dstl);
			delete[] src;
			break;
	}
	return ret;
}

struct extractSARfunction_param{
	Path working_dir;
	File *file;
};

void extractSARfunction(const std::wstring &ex_path,const std::wstring &in_path,bool is_dir,const sarExtra &extraData,extractSARfunction_param &params){
	Path working_dir=params.working_dir;
	working_dir/=in_path;
	std::cout <<UniToUTF8(in_path)<<std::endl;
	if (is_dir)
		boost::filesystem::create_directory(working_dir);
	else{
		if (extraData.compression==COMPRESSION_SPB || extraData.compression==COMPRESSION_LZSS){
			size_t l=extraData.compressed_length;
			uchar *buffer=params.file->read(l,l,extraData.offset);
			if (extraData.compression==COMPRESSION_SPB)
				std::cout <<"\tSPB\n";
			buffer=decompress(buffer,extraData.compressed_length,extraData.uncompressed_length,extraData.compression);
			File::write(working_dir.string(),buffer,extraData.uncompressed_length);
			delete[] buffer;
		}else{
			File file(working_dir.string(),0);
			compression::decompress_from_file dff;
			compression::decompress_to_file dtf;
			dff.file=params.file;
			dff.offset=extraData.offset;
			dtf.file=&file;
			compression::compression_f f;
			switch (extraData.compression){
				case COMPRESSION_NONE:
					f=compression::simple_copy;
					break;
				case COMPRESSION_BZ2:
					f=compression::DecompressBZ2;
					break;
				case COMPRESSION_DEFLATE:
					f=compression::DecompressDEFLATE;
					break;
			}
			if (!f(&dtf,&dff))
				std::cout <<"\tFAILED!\n";
		}
	}
}

void sarArchive::extract(const std::wstring &outputFilename){
	if (!this->good || !this->constructed_for_extracting)
		return;
	extractSARfunction_param params={
		boost::filesystem::initial_path<Path>(),
		&this->file
	};
	if (!outputFilename.size()){
		std::wstring temp=this->file_source.leaf();
		for (ulong a=0;a<temp.size();a++)
			if (temp[a]=='.')
				temp[a]='_';
		tolower(temp);
		params.working_dir/=temp;
	}else
		params.working_dir/=outputFilename;
	boost::filesystem::create_directory(params.working_dir);
	this->root.foreach<extractSARfunction_param,extractSARfunction>(L"",L"",params);
}

void version(){
	std::cout <<"nsaio v1.1\n\n"
		"Copyright (c) 2008-2010, Helios (helios.vmg@gmail.com)\n"
		"All rights reserved.\n\n";
}

void usage(){
	version();
	std::cout <<
		"nsaio <mode> <options> <input files>\n"
		"\n"
		"MODES:\n"
		"    e - Extract\n"
		"    c - Create\n"
		"    l - List\n"
		"\n"
		"OPTIONS:\n"
		"    -o <filename>\n"
		"        Output filename.\n"
		"    -c <compression>\n"
		"        Set compression.\n"
		"    -r <directory>\n"
		"        Add subdirectories instead of the directory passed as argument.\n"
		"\n"
		"See the documentation for details.\n";
}

bool isNSA(const std::wstring &filename){
	size_t dot=filename.rfind('.');
	if (dot==filename.npos)
		return 0;
	std::wstring extension=filename.substr(dot+1);
	tolower(extension);
	if (extension==L"sar")
		return 0;
	return 1;
}

void initialize_conversion_tables();

int main(int argc,char **argv){
	initialize_conversion_tables();
	Options options(argv);
	if (!options.good)
		return 1;
	switch (options.mode){
		case 'h':
			usage();
			break;
		case 'v':
			version();
			break;
		case 'e':
			if (options.inputFilenames.size()<1){
				std::cerr <<"No input files."<<std::endl;
				return 1;
			}
			if (options.inputFilenames.size()>1){
				std::cerr <<"Too many input files."<<std::endl;
				return 1;
			}
			{
				boost::posix_time::ptime t0=boost::posix_time::microsec_clock::local_time();
				{
					std::wstring &filename=options.inputFilenames[0].first;
					sarArchive archive(filename,isNSA(filename));
					if (!archive.good){
						std::cerr <<"\""<<UniToUTF8(filename)<<"\" is not a valid archive."<<std::endl;
						return 1;
					}
					archive.extract(options.outputFilename);
				}
				boost::posix_time::ptime t1=boost::posix_time::microsec_clock::local_time();
				std::cout <<"Elapsed time: "<<double((t1-t0).total_milliseconds())/1000.0<<'s'<<std::endl;
			}
			break;
		case 'c':
			{
				boost::posix_time::ptime t0=boost::posix_time::microsec_clock::local_time();
				{
					sarArchive archive;
					for (ulong a=0;a<options.inputFilenames.size();a++)
						archive.add(options.inputFilenames[a].first,options.inputFilenames[a].second);
					archive.write(options.outputFilename+L".nsa",options.compressionType);
				}
				boost::posix_time::ptime t1=boost::posix_time::microsec_clock::local_time();
				std::cout <<"Elapsed time: "<<double((t1-t0).total_milliseconds())/1000.0<<'s'<<std::endl;
			}
			break;
		case 'l':
			for (ulong a=0;a<options.inputFilenames.size();a++){
				std::wstring &filename=options.inputFilenames[a].first;
				sarArchive archive(filename,isNSA(filename));
				if (!archive.good){
					std::cerr <<"\""<<UniToUTF8(filename)<<"\" is not a valid archive."<<std::endl;
					continue;
				}
				archive.list();
			}
			break;
	}
	return 0;
}
