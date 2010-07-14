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

typedef unsigned long ulong;
typedef unsigned char uchar;

#include <iostream>
#include <fstream>
#include "Unicode.h"

uchar *readfile(const std::wstring &name,ulong &len){
	std::ifstream file(UniToUTF8(name).c_str(),std::ios::binary|std::ios::ate);
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

char writefile(const std::wstring &name,char *buffer,ulong size){
	std::ofstream file(UniToUTF8(name).c_str(),std::ios::binary);
	if (!file)
		return 1;
	file.write(buffer,size);
	file.close();
	return 0;
}

namespace ENCODINGS{
enum{
	AUTO_ENCODING=0,
	UCS2_ENCODING=1,
	UCS2L_ENCODING=2,
	UCS2B_ENCODING=3,
	UTF8_ENCODING=4,
	ISO_8859_1_ENCODING=5,
	SJIS_ENCODING=6
};
}

const char *encodings[][2]={
	{"auto","Attempt to automatically determine the encoding"},
	{"ucs2","UCS-2 (auto endianness)"},
	{"ucs2l","UCS-2 (little endian)"},
	{"ucs2b","UCS-2 (big endian)"},
	{"utf8","UTF-8"},
	{"iso","ISO 8859-1"},
	{"sjis","Shift JIS"},
	{0,0},
};

void usage();

void version(){
	std::cout <<"recoder v0.95 - A simple encoding and code page converter.\n\n"
		"Copyright (c) 2008-2010, Helios (helios.vmg@gmail.com)\n"
		"All rights reserved."<<std::endl;
	exit(0);
}

void initialize_conversion_tables();

int main(int argc,char **argv){
	if (argc<2)
		usage();
	if (!strcmp(argv[1],"--version"))
		version();
	if (argc<5 ||!strcmp(argv[1],"-h") || !strcmp(argv[1],"-?") || !strcmp(argv[1],"--help"))
		usage();
	initialize_conversion_tables();
	std::string ienc=argv[1],
		oenc=argv[3];
	std::wstring ifile=UniFromUTF8(std::string(argv[2])),
		ofile=UniFromUTF8(std::string(argv[4]));
	long inputEncoding=-1,outputEncoding=-1;
	for (ulong a=0;encodings[a][0] && inputEncoding==-1;a++)
		if (ienc==encodings[a][0])
			inputEncoding=a;
	if (inputEncoding==-1){
		std::cout <<"Could not make sense of argument. Input encoding defaults to auto."<<std::endl;
		inputEncoding=ENCODINGS::AUTO_ENCODING;
	}
	ulong l;
	char *buffer=(char *)readfile(ifile,l);
	if (!buffer){
		std::cout <<"File not found."<<std::endl;
		return 0;
	}
	std::string middleBuffer(buffer,l);
	delete[] buffer;
	std::wstring WmiddleBuffer;
	if (isValidUTF8(&middleBuffer[0],middleBuffer.size())){
		std::cout <<"The script seems to be a valid UTF-8 stream. Using it as such.\n";
		inputEncoding=ENCODINGS::UTF8_ENCODING;
	}else if (isValidSJIS(&middleBuffer[0],middleBuffer.size())){
		std::cout <<"The script seems to be a valid Shift JIS stream. Using it as such.\n";
		inputEncoding=ENCODINGS::SJIS_ENCODING;
	}else{
		std::cout <<"The script seems to be a valid ISO-8859-1 stream. Using it as such.\n";
		inputEncoding=ENCODINGS::ISO_8859_1_ENCODING;
	}
	if (inputEncoding!=ENCODINGS::AUTO_ENCODING){
		if (middleBuffer.size()>=3 &&
				(uchar)middleBuffer[0]==BOM8A &&
				(uchar)middleBuffer[1]==BOM8B &&
				(uchar)middleBuffer[2]==BOM8C &&
				inputEncoding!=ENCODINGS::UTF8_ENCODING)
			std::cout <<"WARNING: The file appears to be a UTF-8."<<std::endl;
		else if (middleBuffer.size()>=2 &&
				(uchar)middleBuffer[0]==BOM16BA &&
				(uchar)middleBuffer[1]==BOM16BB &&
				inputEncoding==ENCODINGS::UCS2L_ENCODING)
			std::cout <<"WARNING: The file appears to be a big endian UCS-2."<<std::endl;
		else if (middleBuffer.size()>=2 &&
				(uchar)middleBuffer[0]==BOM16LA &&
				(uchar)middleBuffer[1]==BOM16LB &&
				inputEncoding==ENCODINGS::UCS2B_ENCODING)
			std::cout <<"WARNING: The file appears to be a little endian UCS-2."<<std::endl;
	}
	switch (inputEncoding){
		case ENCODINGS::UCS2_ENCODING:
			WmiddleBuffer=UniFromUCS2(middleBuffer);
			break;
		case ENCODINGS::UCS2L_ENCODING:
			WmiddleBuffer=UniFromUCS2(middleBuffer,NONS_LITTLE_ENDIAN);
			break;
		case ENCODINGS::UCS2B_ENCODING:
			WmiddleBuffer=UniFromUCS2(middleBuffer,NONS_BIG_ENDIAN);
			break;
		case ENCODINGS::UTF8_ENCODING:
			WmiddleBuffer=UniFromUTF8(middleBuffer);
			break;
		case ENCODINGS::ISO_8859_1_ENCODING:
			WmiddleBuffer=UniFromISO88591(middleBuffer);
			break;
		case ENCODINGS::SJIS_ENCODING:
			WmiddleBuffer=UniFromSJIS(middleBuffer);
			break;
	}
	middleBuffer.clear();
	for (ulong a=0;encodings[a][0] && outputEncoding==-1;a++)
		if (oenc==encodings[a][0])
			outputEncoding=a;
	if (outputEncoding==-1){
		std::cout <<"Could not make sense of argument. Output encoding defaults to auto."<<std::endl;
		outputEncoding=ENCODINGS::AUTO_ENCODING;
	}
switchOutputEncoding:
	switch (outputEncoding){
		case ENCODINGS::AUTO_ENCODING:
			{
				bool canbeISO=1;
				for (ulong a=0;a<l && canbeISO;a++)
					if (WmiddleBuffer[a]>0xFF)
						canbeISO=0;
				long UTF8size=getUTF8size(&WmiddleBuffer[0],WmiddleBuffer.size());
				long ucs2size=l*2+2;
				if (canbeISO && float(UTF8size)/float(l)>1.25){
					outputEncoding=ENCODINGS::ISO_8859_1_ENCODING;
					goto switchOutputEncoding;
				}else if (UTF8size<ucs2size){
					outputEncoding=ENCODINGS::UTF8_ENCODING;
					goto switchOutputEncoding;
				}
				outputEncoding=ENCODINGS::UCS2_ENCODING;
				goto switchOutputEncoding;
			}
		case ENCODINGS::UCS2_ENCODING:
			middleBuffer=UniToUCS2(WmiddleBuffer);
			break;
		case ENCODINGS::UCS2L_ENCODING:
			middleBuffer=UniToUCS2(WmiddleBuffer,NONS_LITTLE_ENDIAN);
			break;
		case ENCODINGS::UCS2B_ENCODING:
			middleBuffer=UniToUCS2(WmiddleBuffer,NONS_BIG_ENDIAN);
			break;
		case ENCODINGS::UTF8_ENCODING:
			middleBuffer=UniToUTF8(WmiddleBuffer);
			break;
		case ENCODINGS::ISO_8859_1_ENCODING:
			middleBuffer=UniToISO88591(WmiddleBuffer);
			break;
		case ENCODINGS::SJIS_ENCODING:
			middleBuffer=UniToSJIS(WmiddleBuffer);
			break;
	}
	WmiddleBuffer.clear();
	if (writefile(ofile,&middleBuffer[0],middleBuffer.size()))
		std::cout <<"Writing to file failed."<<std::endl;
	return 0;
}

void usage(){
	std::cout <<"Usage: recoder <input encoding> <input file> <output encoding> <output file>\n"
		"\n"
		"Available encodings:\n"<<std::endl;
	for (short a=0;encodings[a][0];a++)
		std::cout <<encodings[a][0]<<"\t"<<encodings[a][1]<<std::endl;
	exit(0);
}
