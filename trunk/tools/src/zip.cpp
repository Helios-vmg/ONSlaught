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

#include <iostream>
#include <vector>
#include <zlib.h>
#include <bzlib.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "LZMA.h"
#include "Archive.h"

#define NONS_SYS_WINDOWS (defined _WIN32 || defined _WIN64)
#define NONS_SYS_UNIX (defined __unix__ || defined __unix)

#define COMPRESSION_NONE	0
#define COMPRESSION_DEFLATE	8
#define COMPRESSION_BZ2		12
#define COMPRESSION_LZMA	14
#define COMPRESSION_AUTOH	-1
#define COMPRESSION_AUTOM	-2
#define COMPRESSION_AUTOL	-3
#define COMPRESSION_DEFAULT	COMPRESSION_AUTOL

template <typename T> inline void zero_structure(T &s){ memset(&s,0,sizeof(s)); }

const Uint16 full16=0xFFFF;
const Uint32 full31=0x7FFFFFFF;
const Uint32 full32=0xFFFFFFFF;

ulong parseSize(const std::string &s){
	ulong multiplier=1;
	switch (s[s.size()-1]){
		case 'K':
		case 'k':
			multiplier=1<<10;
			break;
		case 'M':
		case 'm':
			multiplier=1<<20;
			break;
		case 'G':
		case 'g':
			multiplier=1<<30;
			break;
	}
	std::stringstream stream(s);
	ulong res;
	stream >>res;
	return res*multiplier;
}

struct Options{
	bool good;
	char mode;
	std::wstring outputFilename;
	ulong compressionType;
	boost::int32_t split;
	std::vector<std::pair<std::wstring,bool> > inputFilenames;
	Options(char **argv){
		static std::pair<const wchar_t *,ulong> compressions[]={
			std::make_pair(L"none",COMPRESSION_NONE),
			std::make_pair(L"deflate",COMPRESSION_DEFLATE),
			std::make_pair(L"bz2",COMPRESSION_BZ2),
			std::make_pair(L"lzma",COMPRESSION_LZMA),
			std::make_pair(L"autol",COMPRESSION_AUTOL),
			std::make_pair(L"autom",COMPRESSION_AUTOM),
			std::make_pair(L"autoh",COMPRESSION_AUTOH),
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
			"-s",
			0
		};
		this->compressionType=COMPRESSION_DEFAULT;
		this->split=full31;
		this->good=0;
		if (!++argv)
			return;
		this->mode='c';
		bool nextIsOutput=0,
			nextIsCompression=0,
			nextIsSkip=0,
			nextIsSplit=0;
		while (*argv){
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
					if (!(nextIsOutput || nextIsCompression || nextIsSplit || nextIsSkip)){
						nextIsOutput=1;
						break;
					}
				case 5: //-c
					if (!(nextIsOutput || nextIsCompression || nextIsSplit || nextIsSkip)){
						nextIsCompression=1;
						break;
					}
				case 6: //-r
					if (!(nextIsOutput || nextIsCompression || nextIsSplit || nextIsSkip)){
						nextIsSkip=1;
						break;
					}
				case 7: //-s
					if (!(nextIsOutput || nextIsCompression || nextIsSplit || nextIsSkip)){
						nextIsSplit=1;
						break;
					}
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
					}else if (nextIsSplit){
						this->split=parseSize(*argv);
						if (this->split<0)
							this->split=-this->split;
						nextIsSplit=0;
					}else{
						this->inputFilenames.push_back(std::make_pair(UniFromUTF8(*argv),nextIsSkip));
						nextIsSkip=0;
					}
			}
			argv++;
		}
		this->good=1;
	}
};

class CRC32{
	boost::uint32_t crc32;
	static boost::uint32_t CRC32lookup[];
public:
	CRC32(){
		this->Reset();
	}
	void Reset(){
		this->crc32^=~this->crc32;
	}
	void Input(const void *message_array,size_t length){
		for (const uchar *array=(const uchar *)message_array;length;length--,array++)
			this->Input(*array);
	}
	void Input(uchar message_element){
		this->crc32=(this->crc32>>8)^CRC32lookup[message_element^(this->crc32&0xFF)];
	}
	boost::uint32_t Result(){
		return ~this->crc32;
	}
};

boost::uint32_t CRC32::CRC32lookup[]={
	0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,0x9E6495A3,
	0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91, 
	0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7, 
	0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5, 
	0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B, 
	0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59, 
	0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F, 
	0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,0xB6662D3D, 
	0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433, 
	0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,0x91646C97,0xE6635C01, 
	0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457, 
	0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65, 
	0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB, 
	0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9, 
	0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F, 
	0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD, 
	0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683, 
	0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1, 
	0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7, 
	0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5, 
	0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B, 
	0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79, 
	0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F, 
	0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB36A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D, 
	0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x05005713, 
	0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21, 
	0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777, 
	0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45, 
	0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB, 
	0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9, 
	0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF, 
	0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D
};

class ZipArchive:public Archive<char>{
	std::vector<std::string> central_header;
	File *output_file;
	ulong current_file;
	void go_to_next();
	ulong total,
		progress;
public:
	Uint64 split;
	ulong compression;
	std::wstring outputFilename;
	ZipArchive():split(0x7FFFFFFF),compression(COMPRESSION_DEFAULT){}
	~ZipArchive(){
		delete this->output_file;
	}
	void write();
	void write(const Path &src,const std::wstring &dst,bool dir);
	void write_buffer(const char *buffer,size_t size);
	Uint64 write_header(const std::string &buffer);
	std::wstring make_filename(ulong fileno){
		std::wstring ret=this->outputFilename;
		ret.append(L".z");
		ret.append(itoa<wchar_t>(fileno+1,2));
		return ret;
	}
};

namespace compression{
	struct base_in_decompression{
		in_func f;
		typedef SRes (*lzma_f)(ISeqInStream *,void *,size_t *);
		lzma_f f2;
		uchar *in_buffer;
		size_t remaining;
		Uint64 processed,
			total_size;
		CRC32 crc;
		base_in_decompression():in_buffer(0),processed(0),f(0),f2(0),total_size(0){}
		virtual ~base_in_decompression(){}
		virtual bool eof()=0;
		void process_crc(){
			this->crc.Input(this->in_buffer,this->remaining);
		}
	};
	struct base_out_decompression{
		static const ulong bits=15,
			size=1<<bits;
		out_func f;
		typedef size_t (*lzma_f)(ISeqOutStream *,const void *,size_t);
		lzma_f f2;
		std::vector<uchar> out;
		Uint64 processed;
		base_out_decompression():out(size),processed(0),f(0),f2(0){}
		virtual ~base_out_decompression(){}
	};
	struct decompress_from_file:public base_in_decompression{
		File *file;
		Uint64 offset;
		std::vector<uchar> in;
		decompress_from_file():base_in_decompression(),offset(0){
			this->f=in_func;
			this->f2=in_func2;
		}
		virtual bool eof(){
			return this->offset==this->file->filesize();
		}
		static unsigned in_func(void *p,unsigned char **buffer){
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
			_this->process_crc();
			_this->offset+=l;
			_this->processed+=l;
			return l;
		}
		static SRes in_func2(ISeqInStream *p,void *buf,size_t *size){
			decompress_from_file *_this=(decompress_from_file *)(p->user_data);
			size_t l=*size;
			_this->file->read(buf,l,l,_this->offset);
			*size=_this->remaining=l;
			_this->in_buffer=(uchar *)buf;
			_this->process_crc();
			_this->offset+=l;
			_this->processed+=l;
			return 0;
		}
	};
	struct decompress_to_file:public base_out_decompression{
		ZipArchive *file;
		decompress_to_file():base_out_decompression(){
			this->f=out_func;
			this->f2=out_func2;
		}
		static int out_func(void *p,unsigned char *buffer,unsigned size){
			decompress_to_file *_this=(decompress_to_file *)p;
			_this->file->write_buffer((const char *)buffer,size);
			_this->processed+=size;
			return 0;
		}
		static size_t out_func2(ISeqOutStream *p,const void *buf,size_t size){
			decompress_to_file *_this=(decompress_to_file *)(p->user_data);
			_this->file->write_buffer((const char *)buf,size);
			_this->processed+=size;
			return size;
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


	static void *SzAlloc(void *p, size_t size) { return malloc(size); }
	static void SzFree(void *p, void *address) { free(address); }
	bool CompressLZMA(base_out_decompression *dst,base_in_decompression *src){
		CFileSeqInStream inStream;
		CFileOutStream outStream;
		inStream.s.Read=src->f2;
		inStream.s.user_data=src;
		outStream.s.Write=dst->f2;
		outStream.s.user_data=dst;
		CLzmaEncHandle enc;
		CLzmaEncProps props;

		ISzAlloc alloc={SzAlloc,SzFree};

		enc=LzmaEnc_Create(&alloc);
		if (!enc)
			return 0;
		LzmaEncProps_Init(&props);
		props.dictSize=1<<23;
		props.fb=273;
		props.lc=8;
		props.level=9;
		props.lp=4;
		props.numThreads=1;
		props.pb=4;
		props.writeEndMark=1;
		SRes res=LzmaEnc_SetProps(enc, &props);
		if (res)
			return 0;
		Byte header[LZMA_PROPS_SIZE+4]={0,0,5,0};
		size_t headerSize=LZMA_PROPS_SIZE;
		UInt64 fileSize;

		res=LzmaEnc_WriteProperties(enc,header+4,&headerSize);
		fileSize=src->total_size;
		dst->f2(&outStream.s,header,headerSize+4);
		res=LzmaEnc_Encode(enc,&outStream.s,&inStream.s,0,&alloc,&alloc);
		LzmaEnc_Destroy(enc,&alloc,&alloc);
		return 1;
	}
	typedef bool (*compression_f)(base_out_decompression *,base_in_decompression *);
}

void ZipArchive::write(){
	this->current_file=0;
	if (!this->outputFilename.size())
		this->outputFilename=L"output";
	this->output_file=new File(this->outputFilename+L".z01",0);
	this->central_header.clear();
	this->total=this->root.count(1);
	this->progress=0;
	this->root.write(this,L"",L"");
	Uint64 central_size=0,
		central_start_disk,
		central_start_offset,
		entries_on_this_disk=0,
		current_disk;
	for (ulong a=0;a<this->central_header.size();a++){
		Uint64 temp=this->write_header(this->central_header[a]);
		if (!a){
			central_start_disk=this->current_file;
			central_start_offset=temp;
		}
		if (!temp)
			entries_on_this_disk=1;
		else
			entries_on_this_disk++;
		central_size+=this->central_header[a].size();
	}
	std::string buffer;
	//write end of central directory ZIP64
	size_t current_disk_offset,
		size_of_eocdr64_offset;
	writeLittleEndian(4,buffer,0x06064b50);
	size_of_eocdr64_offset=buffer.size();
	writeLittleEndian(8,buffer,0);
	writeLittleEndian(2,buffer,0);
	writeLittleEndian(2,buffer,0);
	current_disk_offset=buffer.size();
	writeLittleEndian(4,buffer,0);
	writeLittleEndian(4,buffer,central_start_disk);
	writeLittleEndian(8,buffer,entries_on_this_disk);
	writeLittleEndian(8,buffer,this->central_header.size());
	writeLittleEndian(8,buffer,central_size);
	writeLittleEndian(8,buffer,central_start_offset);
	writeLittleEndian(8,buffer,buffer.size()-(size_of_eocdr64_offset+8),size_of_eocdr64_offset);
	current_disk=this->current_file+((this->split-this->output_file->filesize()<buffer.size())?1:0);
	writeLittleEndian(4,buffer,current_disk,current_disk_offset);
	Uint64 eocdr64_offset=this->write_header(buffer);
	buffer.clear();
	//write end of central directory ZIP64 locator
	writeLittleEndian(4,buffer,0x07064b50);
	writeLittleEndian(4,buffer,this->current_file);
	writeLittleEndian(8,buffer,eocdr64_offset);
	current_disk_offset=buffer.size();
	writeLittleEndian(4,buffer,0);
	//write end of central directory
	writeLittleEndian(4,buffer,0x06054b50);
	writeLittleEndian(2,buffer,full16);
	writeLittleEndian(2,buffer,full16);
	writeLittleEndian(2,buffer,full16);
	writeLittleEndian(2,buffer,full16);
	writeLittleEndian(4,buffer,full32);
	writeLittleEndian(4,buffer,full32);
	std::string comment="Archive created by zip (ONSlaught implementation) v1.1.";
	writeLittleEndian(2,buffer,comment.size());
	buffer.append(comment);
	current_disk=this->current_file+((this->split-this->output_file->filesize()<22)?1:0);
	writeLittleEndian(4,buffer,current_disk+1,current_disk_offset);
	this->write_header(buffer);
	this->output_file->close();

	{
		std::wstring temp=this->outputFilename+L".zip";
		if (File::file_exists(temp))
			File::delete_file(temp);
		boost::filesystem::rename(this->make_filename(this->current_file),temp);
	}
	std::cout <<"Done."<<std::endl;
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
		"gif","jpeg","jpg","tga","tif","tiff","svgz",
		"ogg","mp3","it","xm","s3m","mod","aiff","flac","669","med","voc","mka",
		"mkv","avi","mpeg","mpg","mp4","flv",
		0
	};
	for (const char **p=compressed_types;*p;p++)
		if (extension==*p)
			return COMPRESSION_NONE;
	switch (compression){
		case COMPRESSION_AUTOH:
			return COMPRESSION_LZMA;
		case COMPRESSION_AUTOM:
			return COMPRESSION_BZ2;
		case COMPRESSION_AUTOL:
			return COMPRESSION_DEFLATE;
	}
	return COMPRESSION_NONE;
}

#define ZIP_FLAG_UTF8 0x800

void write_local_header(std::string &dst,ulong compression,Uint32 crc,Uint64 compressed,Uint64 uncompressed,const std::wstring &path){
	writeLittleEndian(4,dst,0x04034B50);
	writeLittleEndian(2,dst,10);
	writeLittleEndian(2,dst,ZIP_FLAG_UTF8);
	writeLittleEndian(2,dst,compression);
	writeLittleEndian(2,dst,0);
	writeLittleEndian(2,dst,0);
	writeLittleEndian(4,dst,crc);
	std::string zip64;
	writeLittleEndian(2,zip64,1);
	size_t write_zip64_size=zip64.size();
	writeLittleEndian(2,zip64,0);
	if (uncompressed<=full31){
		writeLittleEndian(4,dst,compressed);
		writeLittleEndian(4,dst,uncompressed);
	}else{
		writeLittleEndian(4,dst,full32);
		writeLittleEndian(8,zip64,compressed);
		writeLittleEndian(4,dst,full32);
		writeLittleEndian(8,zip64,uncompressed);
	}
	if (zip64.size()==4)
		zip64.clear();
	else
		writeLittleEndian(2,zip64,zip64.size()-4,write_zip64_size);
	std::string utf8=UniToUTF8(path);
	writeLittleEndian(2,dst,utf8.size()&0xFFFF);
	writeLittleEndian(2,dst,zip64.size());
	dst.append(utf8.substr(0,0xFFFF));
	dst.append(zip64);
}

void write_central_header(
		std::string &dst,
		ulong compression,
		Uint32 crc,
		Uint64 compressed,
		Uint64 uncompressed,
		const std::wstring &path,
		ulong fileno,
		Uint64 offset){
	writeLittleEndian(4,dst,0x02014b50);
	writeLittleEndian(2,dst,10);
	writeLittleEndian(2,dst,10);
	writeLittleEndian(2,dst,ZIP_FLAG_UTF8);
	writeLittleEndian(2,dst,compression);
	writeLittleEndian(2,dst,0);
	writeLittleEndian(2,dst,0);
	writeLittleEndian(4,dst,crc);
	std::vector<Uint64> zip64list;
	bool written[4]={0};
#define WRITE_ZIP64(size,constant,step,src)\
	if ((src)<(constant))\
		writeLittleEndian((size),dst,(src));\
	else{\
		writeLittleEndian((size),dst,(constant));\
		zip64list.push_back(src);\
		written[step]=1;\
	}
	WRITE_ZIP64(4,full32,0,compressed);
	WRITE_ZIP64(4,full32,1,uncompressed);
	std::string utf8=UniToUTF8(path);
	writeLittleEndian(2,dst,utf8.size()&0xFFFF);
	size_t extra_field_offset=dst.size();
	writeLittleEndian(2,dst,0);
	writeLittleEndian(2,dst,0);
	WRITE_ZIP64(2,full16,2,fileno);
	writeLittleEndian(2,dst,0);
	writeLittleEndian(4,dst,0);
	WRITE_ZIP64(4,full32,3,offset);
	dst.append(utf8.substr(0,0xFFFF));
	if (zip64list.size()){
		std::string zip64;
		writeLittleEndian(2,zip64,1);
		writeLittleEndian(2,zip64,zip64list.size()*8-(written[2]?4:0));
		if (written[2] && written[3])
			std::swap(zip64list.back(),zip64list[zip64list.size()-2]);
		for (size_t a=0;a<zip64list.size()-1;a++)
			writeLittleEndian(8,zip64,zip64list[a]);
		writeLittleEndian(written[2]?4:8,zip64,zip64list.back());

		writeLittleEndian(2,dst,zip64.size(),extra_field_offset);
		dst.append(zip64);
	}
}

void ZipArchive::write(const Path &src,const std::wstring &dst,bool dir){
	if (!File::file_exists(src.string()))
		return;
	(std::cout <<'(').width(2);
	std::cout <<(this->progress++*100)/this->total<<"%) "<<UniToUTF8(dst)<<"..."<<std::endl;
	std::string buffer;
	Uint64 uncompressed_l=0;
	CRC32 crc;
	Uint64 overwrite_header_offset=0,
		local_header_start_offset=0;
	ulong overwrite_header_file=0,
		local_header_start_file=0;

	compression::decompress_from_file dff;
	compression::decompress_to_file dtf;
	ulong compression;
	if (dir){
		compression=COMPRESSION_NONE;
		write_local_header(buffer,0,0,0,0,dst);
		local_header_start_file=this->current_file;
		local_header_start_offset=this->output_file->filesize();
		overwrite_header_offset=this->write_header(buffer);
		overwrite_header_file=this->current_file;
	}else{
		File file(src.string(),1);
		uncompressed_l=file.filesize();
		compression=figure_out_compression(UniToUTF8(src.string()),file.filesize(),this->compression);
		write_local_header(buffer,compression,0,0,uncompressed_l,dst);
		local_header_start_file=this->current_file;
		local_header_start_offset=this->output_file->filesize();
		overwrite_header_offset=this->write_header(buffer);
		overwrite_header_file=this->current_file;
		
		dff.file=&file;
		dff.total_size=file.filesize();
		dtf.file=this;
		compression::compression_f f;

		switch (compression){
			case COMPRESSION_NONE:
				f=compression::simple_copy;
				break;
			case COMPRESSION_DEFLATE:
				f=compression::CompressDEFLATE;
				break;
			case COMPRESSION_BZ2:
				f=compression::CompressBZ2;
				{
					std::string temp;
					writeBigEndian(4,temp,(ulong)file.filesize());
					file.write(&temp[0],temp.size());
				}
				break;
			case COMPRESSION_LZMA:
				f=compression::CompressLZMA;
		}
		f(&dtf,&dff);
		file.close();
	}
	//write local header to buffer
	buffer.clear();
	write_local_header(buffer,compression,dff.crc.Result(),dtf.processed,uncompressed_l,dst);
	if (overwrite_header_file==this->current_file)
		this->output_file->write_at_offset(&buffer[0],buffer.size(),overwrite_header_offset);
	else{
		File file(this->make_filename(overwrite_header_file),0,0);
		file.write_at_offset(&buffer[0],buffer.size(),overwrite_header_offset);
	}
	buffer.clear();
	write_central_header(
		buffer,
		compression,
		dff.crc.Result(),
		dtf.processed,
		uncompressed_l,
		dst,
		local_header_start_file,
		local_header_start_offset
	);
	this->central_header.push_back(buffer);
}

void ZipArchive::go_to_next(){
	this->output_file->close();
	this->output_file->open(this->make_filename(++this->current_file),0);
}

void ZipArchive::write_buffer(const char *buffer,size_t size){
	while (this->split-this->output_file->filesize()<size){
		size_t diff=size_t(this->split-this->output_file->filesize());
		this->output_file->write(buffer,diff);
		buffer+=diff;
		size-=diff;
		this->go_to_next();
	}
	this->output_file->write(buffer,size);
}

Uint64 ZipArchive::write_header(const std::string &buffer){
	Uint64 diff=this->split-this->output_file->filesize();
	Uint64 ret;
	if (diff<buffer.size()){
		char *temp=new char[(size_t)diff];
		memset(temp,0,(size_t)diff);
		this->output_file->write(temp,(size_t)diff);
		delete[] temp;
		this->go_to_next();
		ret=0;
	}else
		ret=this->output_file->filesize();
	this->write_buffer(&buffer[0],buffer.size());
	return ret;
}

void version(){
	std::cout <<"zip v1.1\n"
		"Copyright (c) 2009, Helios (helios.vmg@gmail.com)\n"
		"All rights reserved.\n\n";
}

void usage(){
	version();
	std::cout <<
		"zip <options> <input files>\n"
		"\n"
		"OPTIONS:\n"
		"    -o <filename>\n"
		"        Output filename.\n"
		"    -c <compression>\n"
		"        Set compression.\n"
		"    -s <file size>\n"
		"        Split archive into pieces no bigger than this.\n"
		"\n"
		"See the documentation for details.\n";
}

void initialize_conversion_tables();

int main(int,char **argv){
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
		case 'c':
			{
				boost::posix_time::ptime t0=boost::posix_time::microsec_clock::local_time();
				{
					ZipArchive archive;
					for (ulong a=0;a<options.inputFilenames.size();a++)
						archive.add(options.inputFilenames[a].first,options.inputFilenames[a].second);
					archive.split=options.split;
					archive.outputFilename=options.outputFilename;
					archive.compression=options.compressionType;
					archive.write();
				}
				boost::posix_time::ptime t1=boost::posix_time::microsec_clock::local_time();
				std::cout <<"Elapsed time: "<<double((t1-t0).total_milliseconds())/1000.0<<'s'<<std::endl;
			}
			break;
	}
	return 0;
}
