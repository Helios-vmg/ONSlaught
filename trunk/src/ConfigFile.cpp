/*
* Copyright (c) 2008, 2009, Helios (helios.vmg@gmail.com)
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

#include "ConfigFile.h"
#include "IOFunctions.h"

template <typename T>
T DEC2HEX(T x){
	return x<10?'0'+x:'A'+x-10;
}

void getMembers(const std::wstring &src,std::wstring &var,std::wstring &val){
	size_t equals=src.find('=');
	if (equals==src.npos)
		return;
	var=src.substr(0,equals);
	val=src.substr(equals+1);
	trim_string(var);
	trim_string(val);
	if (!var.size() || !val.size()){
		var.clear();
		val.clear();
	}
}

//0=str, 1=dec, 2=hex, 3=bin
char getDataType(const std::wstring &string){
	if (string[0]=='\"')
		return 0;
	if (string[0]=='0' && string[1]=='x')
		return 2;
	if (string[string.size()-1]=='b')
		return 3;
	return 1;
}

ConfigFile::ConfigFile(const std::wstring &filename,ENCODINGS encoding){
	this->init(filename,encoding);
}

void ConfigFile::init(const std::wstring &filename,ENCODINGS encoding){
	this->entries.clear();
	ulong l;
	char *buffer=(char *)NONS_File::read(filename,l);
	if (!buffer)
		return;
	std::wstring decoded;
	switch (encoding){
		case ISO_8859_1_ENCODING:
			decoded=UniFromISO88591(std::string(buffer,l));
			break;
		case UCS2_ENCODING:
			decoded=UniFromUCS2(std::string(buffer,l),UNDEFINED_ENDIANNESS);
			break;
		case UTF8_ENCODING:
			decoded=UniFromUTF8(std::string(buffer,l));
			break;
		case SJIS_ENCODING:
			decoded=UniFromSJIS(std::string(buffer,l));
	}
	delete[] buffer;
	if (decoded[decoded.size()-1]!=10 && decoded[decoded.size()-1]!=13)
		decoded.push_back(10);
	for (ulong a=0,size=decoded.size();a<size;a++){
		while (a<size && (decoded[a]==13 || decoded[a]==10))
			a++;
		ulong second=a;
		while (second<size && decoded[second]!=13 && decoded[second]!=10)
			second++;
		std::wstring line=decoded.substr(a,second-a);
		a=second;
		std::wstring var,
			val;
		getMembers(line,var,val);
		if (!var.size() || !val.size())
			continue;
		tolower(var);
		this->entries[var]=getParameterList(val,1);
	}
}

std::wstring ConfigFile::getWString(const std::wstring &index,ulong subindex){
	config_map_t::iterator i=this->entries.find(index);
	if (i==this->entries.end())
		return L"";
	std::wstring &str=i->second[subindex];
	return str.substr(1,str.size()-2);
}

long ConfigFile::getInt(const std::wstring &index,ulong subindex){
	config_map_t::iterator i=this->entries.find(index);
	if (i==this->entries.end())
		return 0;
	std::wstring &str=i->second[subindex];
	long ret=0;
	std::wstringstream stream;
	switch (getDataType(str)){
		case 1:
			stream <<str;
			stream >>ret;
			break;
		case 2:
			for (std::wstring::iterator i=str.begin()+2;i!=str.end();i++){
				ret<<=4;
				ret|=HEX2DEC(*i);
			}
			break;
		case 3:
			for (std::wstring::iterator i=str.begin()+2;*i!='b';i++){
				ret<<=1;
				ret|=*i-'0';
			}
			break;
		default:
			break;
	}
	return ret;
}

void ConfigFile::assignWString(const std::wstring &var,const std::wstring &val,ulong subindex){
	config_map_t::iterator i=this->entries.find(var);
	std::wstring str=UniFromISO88591("\"");
	str+=val;
	str.push_back('\"');
	if (i!=this->entries.end()){
		if (subindex>=i->second.size())
			i->second.push_back(str);
		else
			i->second[subindex]=str;
	}else{
		this->entries[var]=std::vector<std::wstring>();
		this->entries[var].push_back(str);
	}
}

void ConfigFile::assignInt(const std::wstring &var,long val,ulong subindex){
	config_map_t::iterator i=this->entries.find(var);
	std::wstringstream stream;
	stream <<val;
	if (i!=this->entries.end()){
		if (subindex>=i->second.size())
			i->second.push_back(stream.str());
		else
			i->second[subindex]=stream.str();
	}else{
		this->entries[var]=std::vector<std::wstring>();
		this->entries[var].push_back(stream.str());
	}
}

void ConfigFile::writeOut(const std::wstring &filename,ENCODINGS encoding){
	std::string temp=this->writeOut(encoding);
	NONS_File::write(filename,&temp[0],temp.size());
}

std::string ConfigFile::writeOut(ENCODINGS encoding){
	std::wstring buffer;
	for(config_map_t::iterator i=this->entries.begin(),end=this->entries.end();i!=end;i++){
		buffer.append(i->first);
		buffer.push_back('=');
		for (ulong a=0;;){
			buffer.append(i->second[a++]);
			if (a<i->second.size())
				buffer.push_back(' ');
			else{
				buffer.push_back(13);
				buffer.push_back(10);
				break;
			}
		}
	}
	switch (encoding){
		case ISO_8859_1_ENCODING:
			return UniToISO88591(buffer);
			break;
		case UCS2_ENCODING:
			return UniToUCS2(buffer);
			break;
		case UTF8_ENCODING:
			return UniToUTF8(buffer);
			break;
		case SJIS_ENCODING:
			return UniToSJIS(buffer);
	}
	return "";
}

bool ConfigFile::exists(const std::wstring &var){
	return this->entries.find(var)!=this->entries.end();
}
