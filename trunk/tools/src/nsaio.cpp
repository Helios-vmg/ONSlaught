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
#include <bzlib.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#define NONS_SYS_WINDOWS (defined _WIN32 || defined _WIN64)
#define NONS_SYS_UNIX (defined __unix__ || defined __unix)

typedef boost::filesystem::ifstream InputFile;
typedef boost::filesystem::ofstream OutputFile;

#define COMPRESSION_NONE	0
#define COMPRESSION_SPB		1
#define COMPRESSION_LZSS	2
#define COMPRESSION_BZ2		4
#define COMPRESSION_AUTO	-1
#define COMPRESSION_AUTO_MT	(COMPRESSION_AUTO-1)
#define COMPRESSION_DEFAULT	COMPRESSION_AUTO_MT

struct Options{
	bool good;
	char mode;
	std::wstring outputFilename;
	ulong compressionType;
	std::vector<std::pair<std::wstring,bool> > inputFilenames;
	Options(char **argv){
		static std::pair<std::wstring,ulong> compressions[]={
			std::make_pair(L"none",COMPRESSION_NONE),
			std::make_pair(L"lzss",COMPRESSION_LZSS),
			std::make_pair(L"bz2",COMPRESSION_BZ2),
			std::make_pair(L"auto",COMPRESSION_AUTO),
			std::make_pair(L"automt",COMPRESSION_AUTO_MT)
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
						for (ulong a=0,b=4;b;a++,b--){
							if (s==compressions[a].first){
								this->compressionType=compressions[a].second;
								break;
							}
							if (b==1){
								std::cerr <<"Unrecognized compression mode."<<std::endl;
								return;
							}
						}
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

uchar *readfile(const std::wstring &name,ulong &len){
	InputFile file(name,std::ios::binary|std::ios::ate);
	if (!file)
		return 0;
	ulong pos=file.tellg();
	len=pos;
	file.seekg(0,std::ios::beg);
	uchar *buffer=new uchar[pos];
	file.read((char *)buffer,pos);
	file.close();
	return buffer;
}

uchar *readfile(InputFile &file,ulong &len,ulong offset){
	if (!file)
		return 0;
	ulong originalPosition=file.tellg();
	file.seekg(0,std::ios::end);
	ulong size=file.tellg();
	file.seekg(offset,std::ios::beg);
	size=size-offset>=len?len:size-offset;
	len=size;
	uchar *buffer=new uchar[size];
	file.read((char *)buffer,size);
	file.seekg(originalPosition,std::ios::beg);
	return buffer;
}

char writefile(const std::wstring &name,char *buffer,ulong size){
	boost::filesystem::ofstream file(name,std::ios::binary);
	if (!file)
		return 1;
	file.write(buffer,size);
	file.close();
	return 0;
}

struct sarExtra{
	ulong offset,
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
	InputFile file;
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
	this->file.open(this->file_source,std::ios::binary);
	if (!this->file)
		return;
	ulong l=6;
	char *buffer=(char *)readfile(this->file,l,0);
	if (l<6){
		delete[] buffer;
		return;
	}
	size_t offset=0;
	ulong n=readBigEndian(2,buffer,offset),
		file_data_start=readBigEndian(4,buffer,offset);
	delete[] buffer;
	l=file_data_start-offset;
	buffer=(char *)readfile(this->file,l,offset);
	if (l<file_data_start-offset){
		delete[] buffer;
		return;
	}
	offset=0;
	for (ulong a=0;a<n;a++){
		TreeNode<sarExtra> *new_node;
		{
			std::string path=buffer+offset;
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

//Default:
const ulong NN=8,
	MAXS=4;

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
				//It is. Increment starts and try again
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

struct sarArchiveWrite_param{
	OutputFile &file;
	std::string buffer;
	ulong step,
		total,
		progress;
	std::vector<ulong> compression_offsets;
	ulong compression_used;
	sarArchiveWrite_param(OutputFile &f):file(f),step(0),compression_used(COMPRESSION_DEFAULT){}
};

void compression_helper(char *(*f)(void *,size_t,size_t &),char **r,void *a,size_t b,size_t *c){
	*r=f(a,b,*c);
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
		std::cout <<(params.progress++*100/params.total)<<"%) "<<UniToUTF8(in_path)<<"..."<<std::endl;
		ulong raw_l;
		char *raw=(char *)readfile(ex_path,raw_l);
		ulong compression=(raw_l)?params.compression_used:COMPRESSION_NONE;
		size_t compressed_l;
		char *compressed;
		switch (compression){
			case COMPRESSION_NONE:
				compressed=raw;
				compressed_l=raw_l;
				break;
			case COMPRESSION_LZSS:
				compressed=(char *)encode_LZSS(raw,raw_l,compressed_l);
				delete[] raw;
				break;
			case COMPRESSION_BZ2:
				compressed=compressBuffer_BZ2(raw,raw_l,compressed_l);
				delete[] raw;
				raw=new char[compressed_l+4];
				{
					std::string temp;
					writeBigEndian(4,temp,raw_l);
					memcpy(raw,&temp[0],4);
				}
				memcpy(raw+4,compressed,compressed_l);
				delete[] compressed;
				compressed=raw;
				compressed_l+=4;
				break;
			case COMPRESSION_AUTO:
			case COMPRESSION_AUTO_MT:
				{
					size_t lzss=raw_l,
						bz2=raw_l;
					char *bz2_buffer,
						*lzss_buffer;
					if (compression==COMPRESSION_AUTO_MT){
						boost::thread thread(boost::bind(compression_helper,compressBuffer_BZ2,&bz2_buffer,raw,raw_l,&bz2));
						lzss_buffer=encode_LZSS(raw,raw_l,lzss);
						thread.join();
						{
							char *temp=new char[bz2+4];
							size_t temp2=0;
							writeBigEndian(4,temp,raw_l,temp2);
							memcpy(temp+4,bz2_buffer,bz2);
							delete[] bz2_buffer;
							bz2_buffer=temp;
							bz2+=4;
						}
					}else{
						bz2_buffer=compressBuffer_BZ2(raw,raw_l,bz2);
						lzss_buffer=encode_LZSS(raw,raw_l,lzss);
					}
					if (raw_l<=lzss && raw_l<=bz2){
						compressed=raw;
						compressed_l=raw_l;
						delete[] lzss_buffer;
						delete[] bz2_buffer;
						compression=COMPRESSION_NONE;
					}else{
						delete[] raw;
						if (lzss<raw_l && lzss<=bz2){
							compressed=lzss_buffer;
							compressed_l=lzss;
							delete[] bz2_buffer;
							compression=COMPRESSION_LZSS;
						}else{
							compressed=bz2_buffer;
							compressed_l=bz2;
							delete[] lzss_buffer;
							compression=COMPRESSION_BZ2;
						}
					}
				}
		}
		ulong offset=params.compression_offsets[params.step++-1];
		params.buffer[offset]=(char)compression;
		writeBigEndian(4,params.buffer,size_t(params.file.tellp())-params.buffer.size(),offset+1);
		writeBigEndian(4,params.buffer,compressed_l,offset+5);
		writeBigEndian(4,params.buffer,raw_l,offset+9);
		params.file.write(compressed,compressed_l);
		delete[] compressed;
	}
}

void sarArchive::write(const std::wstring &outputFilename,ulong compression){
	OutputFile file(outputFilename.size()?outputFilename:L"output.nsa",std::ios::binary);
	sarArchiveWrite_param params(file);
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
	file.seekp(0);
	file.write(&params.buffer[0],params.buffer.size());
	std::cout <<"Done."<<std::endl;
}

void listSARfunction(const std::wstring &ex_path,const std::wstring &in_path,bool is_dir,const sarExtra &extraData,void *&){
	static const char *methods[]={
		"No",
		"SPB",
		"LZSS",
		0,
		"BZ2",
	};
	if (!is_dir)
		std::cout <<UniToUTF8(in_path)<<std::endl
			<<"\tCompressed:   "<<humanReadableSize(extraData.compressed_length)<<std::endl
			<<"\tUncompressed: "<<humanReadableSize(extraData.uncompressed_length)<<std::endl
			<<'\t'<<methods[extraData.compression]<<" compression."<<std::endl;
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

char *decompressBuffer_BZ2(char *src,unsigned long srcl,unsigned long &dstl){
	unsigned long l=dstl,realsize=l;
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
	dstl=l;
	return dst;
}

static uchar *decompress(uchar *src,ulong srcl,ulong dstl,ulong method){
	uchar *ret;
	switch (method){
		case COMPRESSION_NONE:
			ret=src;
			break;
		case COMPRESSION_SPB:
			ret=(uchar *)decode_SPB((char *)src,srcl,dstl);
			delete[] src;
			break;
		case COMPRESSION_LZSS:
			ret=(uchar *)decode_LZSS((char *)src,srcl,dstl);
			delete[] src;
			break;
		case COMPRESSION_BZ2:
			ret=(uchar *)decompressBuffer_BZ2((char *)src+4,srcl,dstl);
			delete[] src;
			break;
	}
	return ret;
}

struct extractSARfunction_param{
	Path working_dir;
	InputFile &file;
};

void extractSARfunction(const std::wstring &ex_path,const std::wstring &in_path,bool is_dir,const sarExtra &extraData,extractSARfunction_param &params){
	Path working_dir=params.working_dir;
	working_dir/=in_path;
	std::cout <<UniToUTF8(in_path)<<std::endl;
	if (is_dir)
		boost::filesystem::create_directory(working_dir);
	else{
		ulong l=extraData.compressed_length;
		uchar *buffer=readfile(params.file,l,extraData.offset);
		if (extraData.compression==COMPRESSION_SPB)
			std::cout <<"\tSPB"<<std::endl;
		buffer=decompress(buffer,extraData.compressed_length,extraData.uncompressed_length,extraData.compression);
		OutputFile file(working_dir,std::ios::binary);
		file.write((char *)buffer,extraData.uncompressed_length);
		delete[] buffer;
	}
}

void sarArchive::extract(const std::wstring &outputFilename){
	if (!this->good || !this->constructed_for_extracting)
		return;
	extractSARfunction_param params={
		boost::filesystem::initial_path<Path>(),
		this->file
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
	std::cout <<"nsaio v0.99\n\n"
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
			if (options.inputFilenames.size()>1){
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
