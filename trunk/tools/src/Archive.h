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

#ifndef ARCHIVE_H
#define ARCHIVE_H
#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <climits>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include "Unicode.h"

typedef boost::filesystem::wpath Path;

template <typename T>
inline size_t find_slash(const std::basic_string<T> &str,size_t off=0){
	size_t r=str.find('/',off);
	if (r==str.npos)
		r=str.find('\\',off);
	return r;
}

template <typename T> class Archive;

struct TreeNodeComp{
	bool operator()(const std::wstring &opA,const std::wstring &opB) const{
		return stdStrCmpCI(opA,opB)<0;
	}
};

template <typename T>
class TreeNode{
	std::wstring name;
	typedef std::map<std::wstring,TreeNode<T>,TreeNodeComp> container;
	typedef typename container::iterator container_iterator;
	typedef typename container::const_iterator const_container_iterator;
	container branches;
public:
	T extraData;
	Path base_path;
	bool is_dir,
		skip;
	TreeNode():is_dir(0),skip(0){}
	TreeNode(const std::wstring &s):is_dir(0),name(s),skip(0){}
	virtual ~TreeNode(){}
	void clear(){ this->branches.clear(); }
	ulong count(bool include_directories);
	TreeNode *get_branch(const std::wstring &path,bool create);
	void add(const TreeNode &with_this,const Path &with_base_path=L"");
	void write(Archive<T> *archive,const Path &src,const Path &dst);
	template <typename UserData_t,void ForeachFunction(const std::wstring &,const std::wstring &,bool,const T &,UserData_t &)>
	void foreach(const Path &external_path,const std::wstring &internal_path,UserData_t &userData);
};

template <typename T>
class Archive{
protected:
	TreeNode<T> root;
public:
	Archive():root(L""){
		this->root.is_dir=1;
		this->root.skip=1;
	}
	void add(const std::wstring &path,bool skip);
	virtual void write()=0;
	virtual void write(const Path &src,const std::wstring &dst,bool dir)=0;
};

template <typename T>
std::basic_string<T> itoa(long n,unsigned w=0){
	std::basic_stringstream<T> stream;
	if (w){
		stream.fill('0');
		stream.width(w);
	}
	stream <<n;
	return stream.str();
}

void writeByte(std::string &dst,ulong src,size_t offset=ULONG_MAX);
inline void writeByte(void *_src,ulong src,size_t &offset){
	((uchar *)_src)[offset++]=(uchar)src;
}
ulong readByte(void *src,size_t &offset);
void writeLittleEndian(size_t size,std::string &dst,ulong src,size_t offset=ULONG_MAX);
void writeLittleEndian(size_t size,void *_src,ulong src,size_t &offset);
void writeBigEndian(size_t size,std::string &dst,ulong src,size_t offset=ULONG_MAX);
void writeBigEndian(size_t size,void *_src,ulong src,size_t &offset);
ulong readLittleEndian(size_t size,void *_src,size_t &offset);
ulong readBigEndian(size_t size,void *_src,size_t &offset);

template <typename T>
TreeNode<T> *TreeNode<T>::get_branch(const std::wstring &_path,bool create){
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
			_this->branches[name]=TreeNode<T>(name);
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

template <typename T>
void TreeNode<T>::add(const TreeNode<T> &with_this,const Path &with_base_path){
	if (with_this.skip){
		Path base_path=(with_this.base_path.string().size())?with_this.base_path:with_base_path;
		if (base_path.string().size())
			base_path/=with_this.name;
		for (const_container_iterator i=with_this.branches.begin();i!=with_this.branches.end();i++)
			this->add(i->second,base_path);
	}else{
		container_iterator i=this->branches.find(with_this.name);
		if (i==this->branches.end()){
			TreeNode *tn=&(this->branches[with_this.name]=with_this);
			if (with_base_path.string().size() && !tn->base_path.string().size())
				tn->base_path=with_base_path;
		}else if (with_this.is_dir){
			Path base_path=(with_this.base_path.string().size())?with_this.base_path:with_base_path;
			if (base_path.string().size())
				base_path/=with_this.name;
			for (const_container_iterator i2=with_this.branches.begin();i2!=with_this.branches.end();i2++){
				i->second.add(i2->second,base_path);
			}
		}else{
			this->branches.erase(i);
			TreeNode *tn=&(this->branches[with_this.name]=with_this);
			if (with_base_path.string().size() && !tn->base_path.string().size())
				tn->base_path=with_base_path;
		}
	}
}

template <typename T>
void TreeNode<T>::write(Archive<T> *archive,const Path &_src,const Path &_dst){
	Path src=(this->base_path.string().size())?this->base_path:_src,
		dst=_dst;
	src/=this->name;
	if (!this->skip){
		dst/=this->name;
		std::wstring dst_str=dst.string();
		if (this->is_dir && dst_str.size() && dst_str[dst_str.size()-1]!='/')
			dst_str.push_back('/');
		archive->write(src,dst_str,this->is_dir);
	}
	for (container_iterator i=this->branches.begin();i!=this->branches.end();i++)
		i->second.write(archive,src,dst);
}

template <typename T>
template <typename UserData_t,void ForeachFunction(const std::wstring &,const std::wstring &,bool,const T &,UserData_t &)>
void TreeNode<T>::foreach(const Path &external_path,const std::wstring &internal_path,UserData_t &userData){
	Path ex_path=(this->base_path.string().size())?this->base_path:external_path;
	std::wstring in_path=internal_path;
	if (!this->skip){
		in_path.append(this->name);
		ex_path/=this->name;
		if (this->is_dir && in_path.size() && in_path[in_path.size()-1]!='/')
			in_path.push_back('/');
		ForeachFunction(ex_path.string(),in_path,this->is_dir,this->extraData,userData);
	}
	for (container_iterator i=this->branches.begin();i!=this->branches.end();i++)
		i->second.foreach<UserData_t,ForeachFunction>(ex_path,in_path,userData);
}

template <typename T>
ulong TreeNode<T>::count(bool include_directories){
	ulong res=!this->skip && (!this->is_dir || include_directories);
	for (container_iterator i=this->branches.begin();i!=this->branches.end();i++)
		res+=i->second.count(include_directories);
	return res;
}

template <typename T>
void findfiles(const std::wstring &dir_path,TreeNode<T> &node){
	node.clear();
	boost::filesystem::wdirectory_iterator end_itr;
	try{
		for (boost::filesystem::wdirectory_iterator itr(dir_path);itr!=end_itr;itr++){
			std::wstring filename=itr->leaf();
			if (!boost::filesystem::is_directory(*itr))
				node.get_branch(filename,1);
			else{
				TreeNode<T> *new_node=node.get_branch(filename,1);
				new_node->is_dir=1;
				findfiles(itr->string(),*new_node);
			}
		}
	}catch (...){}
}

template <typename T>
void Archive<T>::add(const std::wstring &path,bool skip){
	Path absolute(boost::filesystem::complete(path));
	while (absolute.leaf()==L".")
		absolute.remove_leaf();
	if (!boost::filesystem::exists(absolute))
		return;
	TreeNode<T> new_node(absolute.leaf());
	if (boost::filesystem::is_directory(absolute))
		new_node.is_dir=1;
	absolute.remove_leaf();
	new_node.base_path=absolute;
	new_node.skip=skip;
	findfiles(path,new_node);
	this->root.add(new_node);
}
#endif
