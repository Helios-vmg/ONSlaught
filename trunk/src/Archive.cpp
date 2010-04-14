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

#include "Archive.h"
#include "CommandLineOptions.h"
#include "puff.h"
#include "LZMA.h"
#include <bzlib.h>
#include <iostream>
#include <cassert>

class CRC32{
	Uint32 crc32;
	static Uint32 CRC32lookup[];
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
	Uint32 Result(){
		return ~this->crc32;
	}
};

Uint32 CRC32::CRC32lookup[]={
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

template <typename T>
inline size_t find_slash(const std::basic_string<T> &str,size_t off=0){
	size_t r=str.find('/',off);
	if (r==str.npos)
		r=str.find('\\',off);
	return r;
}

TreeNode::~TreeNode(){
	if (this->freeExtraData)
		this->freeExtraData(this->extraData);
}

TreeNode *TreeNode::get_branch(const std::wstring &_path,bool create){
	TreeNode *_this=this;
	std::wstring path=_path;
	while (1){
		size_t slash=find_slash(path);
		std::wstring name=path.substr(0,slash);
		if (!name.size())
			return _this;
		bool is_dir=(slash!=path.npos);
		container_iterator i=_this->branches.find(name);
		if (i==_this->branches.end()){
			if (!create)
				return 0;
			_this->branches[name]=TreeNode(name);
			i=_this->branches.find(name);
			i->second.is_dir=is_dir;
		}
		if (slash!=path.npos)
			name=path.substr(slash+1);
		else
			name.clear();
		if (!name.size())
			return &(i->second);
		path=name;
		_this=&(i->second);
	}
	return 0;
}

bool Archive::exists(const std::wstring &path){
	TreeNode *node=this->root.get_branch(path,0);
	return !!node;
}

bool Archive::test(){
	return this->test_rec(&this->root);
}

bool Archive::test_rec(TreeNode *node){
	bool ret=1;
	if (!node->is_dir && !node->skip){
		ulong l;
		uchar *buffer=this->get_file_buffer(L"",node,l);
		if (!buffer)
			return 0;
		delete[] buffer;
	}
	for (TreeNode::container_iterator i=node->branches.begin();i!=node->branches.end() && ret;i++)
		ret=ret&this->test_rec(&(i->second));
	return ret;
}

uchar *Archive::get_file_buffer(const std::wstring &path,ulong &size){
	if (!this->good)
		return 0;
	TreeNode *node=this->root.get_branch(path,0);
	if (!node)
		return 0;
	return this->get_file_buffer(path,node,size);
}

template <typename T>
ulong readBigEndian(size_t size,void *_src,T &offset){
	ulong res=0;
	uchar *src=(uchar *)_src;
	src+=offset;
	for (int a=size;a;a--,src++)
		res=(res<<8)|*src;
	offset+=size;
	return res;
}

void writeLittleEndian(size_t size,void *_src,ulong src,size_t &offset){
	for (int a=size;a;a--,offset++){
		((uchar *)_src)[offset]=(uchar)src;
		src>>=8;
	}
}

NSAarchive::NSAarchive(const std::wstring &path,bool nsa)
		:Archive(),
		file(path,1),
		path(path),
		nsa(nsa){
	this->root.freeExtraData=NSAarchive::freeExtraData;
	if (!this->file)
		return;
	ulong l;
	char *buffer=(char *)this->file.read(6,l,0);
	if (l<6){
		delete[] buffer;
		return;
	}
	ulong offset=0;
	ulong n=readBigEndian(2,buffer,offset),
		file_data_start=readBigEndian(4,buffer,offset);
	delete[] buffer;
	l=file_data_start-offset;
	buffer=(char *)this->file.read(l,l,offset);
	if (l<file_data_start-offset){
		delete[] buffer;
		return;
	}
	offset=0;
	for (ulong a=0;a<n;a++){
		TreeNode *new_node;
		{
			std::string temp=buffer+offset;
			offset+=temp.size()+1;
			new_node=this->root.get_branch(UniFromSJIS(temp),1);
		}
		NSAdata extraData;
		if (nsa)
			extraData.compression=(NSAdata::compression_type)readByte(buffer,offset);
		else
			extraData.compression=NSAdata::COMPRESSION_NONE;
		extraData.offset=file_data_start+readBigEndian(4,buffer,offset);
		extraData.compressed=readBigEndian(4,buffer,offset);
		if (nsa)
			extraData.uncompressed=readBigEndian(4,buffer,offset);
		else
			extraData.uncompressed=extraData.compressed;
		new_node->extraData=new NSAdata(extraData);
		new_node->freeExtraData=NSAarchive::freeExtraData;
	}
	delete[] buffer;
	this->good=1;
}

void NSAarchive::freeExtraData(void *p){
	if (p)
		delete &derefED(p);
}

inline void writeByte(void *_src,ulong src,size_t &offset){
	((uchar *)_src)[offset++]=(uchar)src;
}

char *decode_SPB(char *buffer,ulong compressedSize,ulong decompressedSize){
	ulong ioffset=0;
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
	uchar ibitoffset=0;
	char *buf=res+54;
	ulong surface=width*height,
		decompressionbufferlen=surface+4;
	char *decompressionbuffer=new char[decompressionbufferlen];
	ooffset=54;
	for (int a=0;a<3;a++){
		ulong count=0;
		uchar x=(uchar)getbits(buffer,8,&ioffset,&ibitoffset);
		decompressionbuffer[count++]=x;
		while (count<surface){
			uchar n=(uchar)getbits(buffer,3,&ioffset,&ibitoffset);
			if (!n){
				for (int b=4;b;b--)
					decompressionbuffer[count++]=x;
				continue;
			}
			uchar m;
			if (n==7)
				m=getbit(buffer,&ioffset,&ibitoffset)+1;
			else
				m=n+2;
			for (int b=4;b;b--){
				if (m==8)
					x=(uchar)getbits(buffer,8,&ioffset,&ibitoffset);
				else{
					ulong k=(ulong)getbits(buffer,m,&ioffset,&ibitoffset);
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
	ulong byteoffset=0;
	uchar bitoffset=0;
	for (ulong len=0;len<decompressedSize;){
		if (getbit(buffer,&byteoffset,&bitoffset)){
			uchar a=(uchar)getbits(buffer,8,&byteoffset,&bitoffset);
			res[len++]=a;
			decompression_buffer[decompresssion_buffer_offset++]=a;
			decompresssion_buffer_offset&=0xFF;
		}else{
			uchar a=(uchar)getbits(buffer,8,&byteoffset,&bitoffset);
			uchar b=(uchar)getbits(buffer,4,&byteoffset,&bitoffset);
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

uchar *NSAarchive::get_file_buffer(const std::wstring &path,TreeNode *node,ulong &size){
	ulong l;
	uchar *buffer=this->file.read(derefED(node->extraData).compressed,l,derefED(node->extraData).offset);
	if (l<derefED(node->extraData).compressed){
		delete[] buffer;
		return 0;
	}
	uchar *res=0;
	size=derefED(node->extraData).uncompressed;
	if ((derefED(node->extraData).compression)==NSAdata::COMPRESSION_NONE)
		res=buffer;
	else{
		switch (derefED(node->extraData).compression){
			case NSAdata::COMPRESSION_SPB:
				res=(uchar *)decode_SPB((char *)buffer,l,size);
				break;
			case NSAdata::COMPRESSION_LZSS:
				res=(uchar *)decode_LZSS((char *)buffer,l,size);
				break;
			case NSAdata::COMPRESSION_BZ2:
				res=(uchar *)decompressBuffer_BZ2((char *)buffer+4,l,size);
				break;
		}
		delete[] buffer;
	}
	return res;
}

struct endOfCDR{
	bool good;
	ulong disk_number,
		CD_start_disk,
		CD_entries_n_, //(total number of entries in the central directory on this disk)
		CD_entries_n, //(total number of entries in the central directory)
		CD_size,
		CD_start;
	endOfCDR():good(0){}
	endOfCDR(NONS_File &file,ulong offset);
	bool init(NONS_File &file,ulong offset);
};

endOfCDR::endOfCDR(NONS_File &file,ulong offset){
	this->init(file,offset);
}

bool endOfCDR::init(NONS_File &file,ulong offset){
	this->good=0;
	ulong l;
	uchar *buffer=file.read(22,l,offset);
	if (l<22){
		delete[] buffer;
		return 0;
	}
	if (ZIParchive::getSignatureType(buffer)!=ZIParchive::EOCDR)
		return 0;
	ulong offset2=4;
	this->disk_number=readWord(buffer,offset2);
	this->CD_start_disk=readWord(buffer,offset2);
	this->CD_entries_n_=readWord(buffer,offset2);
	this->CD_entries_n=readWord(buffer,offset2);
	this->CD_size=readDWord(buffer,offset2);
	this->CD_start=readDWord(buffer,offset2);
	ulong comment_l=readWord(buffer,offset2);
	assert(offset2==22);
	delete[] buffer;
	delete[] file.read(comment_l,l,offset+offset2);
	if (l<comment_l)
		return 0;
	this->good=1;
	return 1;
}

struct centralHeader{
	std::string filename;
	bool good;
	ulong bit_flag,
		compression_method;
	Uint32 crc32;
	ulong compressed_size,
		uncompressed_size,
		disk_number_start,
		local_header_off;
	centralHeader(NONS_File &file,ulong &offset,bool &enough_room);
};

centralHeader::centralHeader(NONS_File &file,ulong &offset,bool &enough_room){
	this->good=0;
	ulong l;
	enough_room=0;
	uchar *buffer=file.read(46,l,offset);
	if (l<46){
		delete[] buffer;
		return;
	}
	if (ZIParchive::getSignatureType(buffer)==ZIParchive::CENTRAL_HEADER){
		ulong offset2=4;
		readWord(buffer,offset2);
		readWord(buffer,offset2);
		this->bit_flag=readWord(buffer,offset2);
		this->compression_method=readWord(buffer,offset2);
		readWord(buffer,offset2);
		readWord(buffer,offset2);
		this->crc32=readDWord(buffer,offset2);
		this->compressed_size=readDWord(buffer,offset2);
		this->uncompressed_size=readDWord(buffer,offset2);
		ulong filename_l=readWord(buffer,offset2);
		ulong extra_field_l=readWord(buffer,offset2),
			file_comment_l=readWord(buffer,offset2);
		this->disk_number_start=readWord(buffer,offset2);
		readWord(buffer,offset2);
		readDWord(buffer,offset2);
		this->local_header_off=readDWord(buffer,offset2);

		assert(offset2==46);
		delete[] buffer;
		buffer=file.read(filename_l,l,offset+offset2);
		if (l<filename_l){
			delete[] buffer;
			return;
		}
		this->filename.assign((char *)buffer,filename_l);
		delete[] buffer;
		extra_field_l+=file_comment_l;
		delete[] file.read(extra_field_l,l,offset+offset2+filename_l);
		if (l<extra_field_l)
			return;
		offset+=offset2+filename_l+extra_field_l;
		enough_room=1;
		this->good=1;
	}
}

struct localHeader{
	std::string filename;
	bool good;
	ulong bit_flag,
		compression_method;
	Uint32 crc32;
	ulong compressed_size,
		uncompressed_size,
		data_offset;
	localHeader():good(0){}
	localHeader(NONS_File &file,ulong offset);
};

localHeader::localHeader(NONS_File &file,ulong offset){
	this->good=0;
	ulong l;
	uchar *buffer=file.read(30,l,offset);
	if (l<30){
		delete[] buffer;
		return;
	}
	if (ZIParchive::getSignatureType(buffer)==ZIParchive::LOCAL_HEADER){
		ulong offset2=4;
		readWord(buffer,offset2);
		this->bit_flag=readWord(buffer,offset2);
		this->compression_method=readWord(buffer,offset2);
		readWord(buffer,offset2);
		readWord(buffer,offset2);
		this->crc32=readDWord(buffer,offset2);
		this->compressed_size=readDWord(buffer,offset2);
		this->uncompressed_size=readDWord(buffer,offset2);
		ulong filename_l=readWord(buffer,offset2),
			extra_field_l=readWord(buffer,offset2);

		assert(offset2==30);
		buffer=file.read(filename_l,l,offset+offset2);
		if (l<filename_l){
			delete[] buffer;
			return;
		}
		this->filename.assign((char *)buffer,filename_l);
		delete[] buffer;
		delete[] file.read(extra_field_l,l,offset+offset2+filename_l);
		if (l<extra_field_l)
			return;
		this->data_offset=offset+offset2+filename_l+l;
		this->good=1;
	}
}

#define ZIP_FLAG_UTF8 0x800

ZIParchive::ZIParchive(const std::wstring &path)
		:Archive(){
	this->root.freeExtraData=ZIParchive::freeExtraData;
	endOfCDR eocdr;
	NONS_File file;
	file.open(path,1);
	ulong filesize,offset;
	if (!file || (filesize=file.filesize())<22)
		return;
	offset=filesize-22;
	eocdr.init(file,offset);
	while (!eocdr.good && offset)
		eocdr.init(file,--offset);
	if (!offset)
		return;
	std::wstring name=path;
	size_t dot=name.rfind('.');
	if (dot!=name.npos){
		this->disks.clear();
		name=name.substr(0,dot+1);
		name.push_back('z');
		for (ulong a=0;a<eocdr.disk_number;a++)
			this->disks.push_back(name+itoaw(a+1,2));
	}
	this->disks.push_back(path);
	file.close();
	file.open(this->disks[eocdr.CD_start_disk],1);
	if (!file)
		return;
	filesize=file.filesize();
	offset=eocdr.CD_start;
	if (filesize<=offset)
		return;
	bool fail=0;
	for (ulong entry=0,fileno=eocdr.CD_start_disk;entry<eocdr.CD_entries_n && fileno<this->disks.size() && !fail;entry++){
		bool enough;
		centralHeader temp(file,offset,enough);
		if (!temp.good){
			if (!enough && fileno+1<eocdr.disk_number){
				file.close();
				file.open(this->disks[++fileno],1);
				entry--;
			}else
				fail=1;
		}else{
			if (temp.filename[temp.filename.size()-1]!='/'){
				TreeNode *new_node;
				localHeader *lh;
				if (temp.disk_number_start!=fileno){
					NONS_File file2(this->disks[temp.disk_number_start],1);
					lh=new localHeader(file2,temp.local_header_off);
				}else
					lh=new localHeader(file,temp.local_header_off);
				if (lh->good){
					std::wstring filename;
					if (CHECK_FLAG(temp.bit_flag,ZIP_FLAG_UTF8))
						filename=UniFromUTF8(temp.filename);
					else
						filename=UniFromISO88591(temp.filename);
					new_node=this->root.get_branch(filename,1);
					ZIPdata extraData;
					extraData.bit_flag=lh->bit_flag;
					extraData.compression=(ZIPdata::compression_type)lh->compression_method;
					extraData.crc32=lh->crc32;
					extraData.compressed=lh->compressed_size;
					extraData.uncompressed=lh->uncompressed_size;
					extraData.disk=temp.disk_number_start;
					extraData.data_offset=lh->data_offset;
					new_node->extraData=new ZIPdata(extraData);
					new_node->freeExtraData=ZIParchive::freeExtraData;
				}
				delete lh;
			}
		}
	}
	this->good=!fail;
}

bool UncompressDEFLATE(void *dst,size_t dst_l,void *src,size_t src_l){
	ulong dst_l2=dst_l,
		src_l2=src_l;
	//int res=uncompress((Bytef *)dst,&dst_l2,(Bytef *)src,src_l2);
	int res=puff((uchar *)dst,&dst_l2,(uchar *)src,&src_l2);
	return !res;
}

bool UncompressBZ2(void *dst,size_t dst_l,void *src,size_t src_l){
	unsigned int dst_l2=dst_l,
		src_l2=src_l;
	int res=BZ2_bzBuffToBuffDecompress((char *)dst,&dst_l2,(char *)src,src_l2,1,0);
	return res==BZ_OK;
}

bool UncompressLZMA(void *dst,size_t dst_l,void *src,size_t src_l){
	uchar *data=((uchar *)src)+9,
		*header=((uchar *)src)+4;
	src_l-=9;
	return LzmaUncompress(
		(uchar *)dst,
		(SizeT *)&dst_l,
		data,
		(SizeT *)&dst_l,
		header,5)==SZ_OK;
}


uchar *ZIParchive::get_file_buffer(const std::wstring &path,TreeNode *node,ulong &size){
	ulong fileno=derefED(node->extraData).disk;
	NONS_File file(this->disks[fileno],1);
	ulong l,
		offset=derefED(node->extraData).data_offset,
		desired=derefED(node->extraData).compressed,
		write_at=0;
	uchar *compressed=new uchar[derefED(node->extraData).compressed];
	while (1){
		uchar *buffer=file.read(desired,l,offset);
		if (!buffer){
			delete[] compressed;
			return 0;
		}
		memcpy(compressed+write_at,buffer,l);
		delete[] buffer;
		if (l==desired)
			break;
		else{
			write_at+=l;
			desired-=l;
			offset=0;
			file.close();
			file.open(this->disks[++fileno],1);
		}
	}
	uchar *uncompressed=0;
	ulong compressed_l=derefED(node->extraData).compressed;
	size=derefED(node->extraData).uncompressed;
	if (derefED(node->extraData).compression==ZIPdata::COMPRESSION_NONE)
		uncompressed=compressed;
	else{
		bool good;
		switch (derefED(node->extraData).compression){
			case ZIPdata::COMPRESSION_DEFLATE:
				uncompressed=new uchar[size];
				good=UncompressDEFLATE(uncompressed,size,compressed,compressed_l);
				break;
			case ZIPdata::COMPRESSION_BZ2:
				uncompressed=new uchar[size];
				good=UncompressBZ2(uncompressed,size,compressed,compressed_l);
				break;
			case ZIPdata::COMPRESSION_LZMA:
				uncompressed=new uchar[size];
				good=UncompressLZMA(uncompressed,size,compressed,compressed_l);
				break;
			default:
				o_stderr <<"Unsupported compression ("<<(ulong)derefED(node->extraData).compression<<") used for file \""<<path<<"\".\n";
				good=0;
		}
		if (!good){
			delete[] compressed;
			if (uncompressed)
				delete[] uncompressed;
			return 0;
		}
		delete[] compressed;
	}
	CRC32 crc;
	crc.Input(uncompressed,size);
	if (crc.Result()!=derefED(node->extraData).crc32){
		delete[] uncompressed;
		uncompressed=0;
	}
	return uncompressed;
}

void ZIParchive::freeExtraData(void *p){
	if (p)
		delete[] &derefED(p);
}

ZIParchive::SignatureType ZIParchive::getSignatureType(void *buffer){
	ulong offset=0;
	switch (readDWord((char *)buffer,offset)){
		case ZIParchive::local_signature:
			return ZIParchive::LOCAL_HEADER;
		case ZIParchive::central_signature:
			return ZIParchive::CENTRAL_HEADER;
		case ZIParchive::EOCDR_signature:
			return ZIParchive::EOCDR;
	}
	return ZIParchive::NOT_A_SIGNATURE;
}

NONS_GeneralArchive::NONS_GeneralArchive(){
	std::wstring path;
	if (CLOptions.archiveDirectory.size())
		path=CLOptions.archiveDirectory+L"/";
	else
		path=L"./";
	const wchar_t *base=L"arc",
		*formats[]={
			L".sar",
			L".nsa",
			L".zip",
			0
		};
	for (ulong a=ULONG_MAX;;a++){
		std::wstring full_name;
		ulong format;
		if (a!=ULONG_MAX){
			std::wstring name=base;
			if (a)
				name.append(itoaw(a));
			bool found=0;
			//!a: if a==0, ".sar" should not be tested
			for (ulong b=!a;formats[b] && !found;b++){
				full_name=path;
				full_name.append(name);
				full_name.append(formats[b]);
				if (!fileExists(full_name) && !fileExists(toupperCopy(full_name)))
					continue;
				found=1;
				format=b;
			}
			if (!found)
				break;
		}else{
			full_name=path+L"arc.sar";
			if (!fileExists(full_name) && !fileExists(toupperCopy(full_name)))
				continue;
			format=0;
		}
		Archive *arc;
		switch (format){
			case 0:
				arc=(Archive *)new NSAarchive(full_name,0);
				break;
			case 1:
				arc=(Archive *)new NSAarchive(full_name,1);
				break;
			case 2:
				arc=(Archive *)new ZIParchive(full_name);
				break;
		}
		if (arc->good){
			/*std::cout <<"Testing "<<full_name<<"..."<<std::endl;
			ulong t0=SDL_GetTicks();
			bool test=arc->test();
			ulong t1=SDL_GetTicks();
			std::cout <<"Archive "<<full_name<<" is "<<((test)?"":"in")<<"valid."<<std::endl;
			std::cout <<"Tested in "<<t1-t0<<" ms."<<std::endl;
			if (test)*/
				this->archives.push_back(arc);
		}else{
			delete arc;
			std::cout <<"Archive "<<full_name<<" is invalid."<<std::endl;
		}
	}
}

NONS_GeneralArchive::~NONS_GeneralArchive(){
	for (ulong a=0;a<this->archives.size();a++)
		if (this->archives[a])
			delete this->archives[a];
}

uchar *NONS_GeneralArchive::getFileBuffer(const std::wstring &filepath,ulong &buffersize){
	uchar *res=getFileBufferWithoutFS(filepath,buffersize);
	return (res)?res:NONS_File::read(filepath,buffersize);
}

uchar *NONS_GeneralArchive::getFileBufferWithoutFS(const std::wstring &filepath,ulong &buffersize){
	uchar *res=0;
	for (long a=this->archives.size()-1;a>=0;a--)
		if (res=this->archives[a]->get_file_buffer(filepath,buffersize))
			return res;
	return res;
}

bool NONS_GeneralArchive::exists(const std::wstring &filepath){
	for (long a=this->archives.size()-1;a>=0;a--)
		if (this->archives[a]->exists(filepath))
			return 1;
	return fileExists(filepath);
}
