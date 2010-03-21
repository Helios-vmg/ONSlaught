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

#ifndef NONS_MACROPARSER_H
#define NONS_MACROPARSER_H
#include "Common.h"
#include "MacroParser.tab.hpp"
#include <vector>
#include <string>
#include <set>

namespace NONS_Macro{

struct Macro;
struct Expression;
struct String;
struct SymbolTable;
struct Identifier;

struct Symbol{
	enum{
		UNDEFINED,
		MACRO,
		INTEGER,
		STRING
	} type;
	std::wstring Identifier;
	ulong declared_on_line;
	Macro *macro;
	long int_val;
	std::wstring str_val;
	Expression *int_initialization;
	String *str_initialization;
	bool has_been_checked;
	Symbol(const std::wstring &,NONS_Macro::Macro *,ulong);
	Symbol(const std::wstring &,ulong);
	Symbol(const std::wstring &,const std::wstring &,ulong);
	Symbol(const std::wstring &,long,ulong);
	Symbol(const Symbol &);
	~Symbol();
	void reset();
	void set(long);
	void set(const std::wstring &);
	long getInt();
	std::wstring getStr();
	void initializeTo(Expression *);
	void initializeTo(String *);
	ulong initialize(const SymbolTable &);
	bool checkSymbols(const SymbolTable &);
};

struct Identifier{
	std::wstring id;
	ulong referenced_on_line;
	Identifier(const std::wstring &a,ulong b):id(a),referenced_on_line(b){}
	bool checkSymbols(const SymbolTable &st,bool expectedVariable);
};

struct SymbolTable{
	std::vector<Symbol *> symbols;
	SymbolTable(){};
	SymbolTable(const SymbolTable &a):symbols(a.symbols){}
	bool declare(const std::wstring &,Macro *,ulong,bool check=1);
	bool declare(const std::wstring &,const std::wstring &,ulong,bool check=1);
	bool declare(const std::wstring &,long,ulong,bool check=1);
	bool declare(Symbol *,bool check=1);
	void addFrame(const SymbolTable &);
	Symbol *get(const std::wstring &) const;
	void resetAll();
	bool checkSymbols();
	bool checkIntersection(const SymbolTable &) const;
};

enum ErrorCode{
	MACRO_NO_ERROR=0,
	MACRO_NO_SUCH_MACRO,
	MACRO_NO_SYMBOL_FOUND,
	MACRO_NUMBER_EXPECTED
};

struct Argument{
	virtual ~Argument(){}
	virtual bool simplify()=0;
	virtual std::wstring evaluateToStr(const SymbolTable * =0,ulong * =0)=0;
	virtual long evaluateToInt(const SymbolTable * =0,ulong * =0)=0;
	virtual bool checkSymbols(const SymbolTable &)=0;
};

struct Expression:Argument{
	virtual Expression *clone()=0;
	virtual ~Expression(){}
};

Expression *simplifyExpression(Expression *);

struct ConstantExpression:Expression{
	long val;
	ConstantExpression(long a):val(a){}
	ConstantExpression(const ConstantExpression &b):val(b.val){}
	Expression *clone(){
		return this?new ConstantExpression(*this):0;
	}
	bool simplify(){
		return 1;
	}
	std::wstring evaluateToStr(const SymbolTable * =0,ulong *error=0);
	long evaluateToInt(const SymbolTable * =0,ulong *error=0);
	bool checkSymbols(const SymbolTable &){return 1;}
};

struct VariableExpression:Expression{
	Identifier id;
	Symbol *s;
	VariableExpression(const Identifier &a):id(a),s(0){}
	VariableExpression(const VariableExpression &b):id(b.id),s(b.s){}
	Expression *clone(){
		return this?new VariableExpression(*this):0;
	}
	bool simplify(){return 0;};
	std::wstring evaluateToStr(const SymbolTable * =0,ulong *error=0);
	long evaluateToInt(const SymbolTable * =0,ulong *error=0);
	bool checkSymbols(const SymbolTable &st);
};

struct FullExpression:Expression{
	yytokentype operation;
	Expression *operands[3];
	FullExpression(yytokentype,Expression * =0,Expression * =0,Expression * =0);
	FullExpression(const FullExpression &b);
	Expression *clone(){
		return this?new FullExpression(*this):0;
	}
	~FullExpression();
	bool simplify();
	std::wstring evaluateToStr(const SymbolTable * =0,ulong *error=0);
	long evaluateToInt(const SymbolTable * =0,ulong *error=0);
	bool checkSymbols(const SymbolTable &st);
};

struct String:Argument{
	virtual String *clone()=0;
	virtual ~String(){}
};

String *simplifyString(String *);

struct ConstantString:String{
	std::wstring val;
	ConstantString(const std::wstring &a):val(a){}
	ConstantString(const ConstantString &a):val(a.val){}
	String *clone(){
		return this?new ConstantString(*this):0;
	}
	bool simplify(){return 1;}
	std::wstring evaluateToStr(const SymbolTable * =0,ulong *error=0);
	long evaluateToInt(const SymbolTable * =0,ulong *error=0);
	bool checkSymbols(const SymbolTable &){return 1;}
};

struct VariableString:String{
	Identifier id;
	VariableString(const Identifier &a):id(a){}
	VariableString(const VariableString &a):id(a.id){}
	String *clone(){
		return this?new VariableString(*this):0;
	}
	bool simplify(){return 0;}
	std::wstring evaluateToStr(const SymbolTable * =0,ulong *error=0);
	long evaluateToInt(const SymbolTable * =0,ulong *error=0);
	bool checkSymbols(const SymbolTable &st);
};

struct StringConcatenation:String{
	String *operands[2];
	StringConcatenation(String *,String *);
	StringConcatenation(const StringConcatenation &a);
	String *clone(){
		return this?new StringConcatenation(*this):0;
	}
	~StringConcatenation();
	bool simplify();
	std::wstring evaluateToStr(const SymbolTable * =0,ulong *error=0);
	long evaluateToInt(const SymbolTable * =0,ulong *error=0);
	bool checkSymbols(const SymbolTable &st);
};

struct Block;
struct MacroFile;

struct Statement{
	virtual ~Statement(){}
	virtual std::wstring perform(SymbolTable,ulong * =0)=0;
	virtual bool checkSymbols(const SymbolTable &)=0;
};

struct EmptyStatement:Statement{
	EmptyStatement(){}
	std::wstring perform(SymbolTable symbol_table,ulong *error=0);
	bool checkSymbols(const SymbolTable &){return 1;}
};

struct DataBlock:Statement{
	std::wstring data;
	DataBlock(const std::wstring &a):data(a){}
	std::wstring perform(SymbolTable symbol_table,ulong *error=0);
	bool checkSymbols(const SymbolTable &){return 1;}
private:
	static std::wstring replace(const std::wstring &src,const SymbolTable &symbol_table);
};

struct AssignmentStatement:Statement{
	Identifier dst;
	Expression *src;
	AssignmentStatement(const Identifier &id,Expression *e):dst(id),src(e){}
	~AssignmentStatement(){delete this->src;}
	std::wstring perform(SymbolTable symbol_table,ulong *error=0);
	bool checkSymbols(const SymbolTable &);
};

struct StringAssignmentStatement:Statement{
	Identifier dst;
	String *src;
	StringAssignmentStatement(const Identifier &id,String *e):dst(id),src(e){}
	~StringAssignmentStatement();
	std::wstring perform(SymbolTable symbol_table,ulong *error=0);
	bool checkSymbols(const SymbolTable &);
};

struct IfStructure:Statement{
	Expression *condition;
	Block *true_block,
		*false_block;
	IfStructure(Expression *,Block *,Block * =0);
	~IfStructure();
	std::wstring perform(SymbolTable symbol_table,ulong *error=0);
	bool checkSymbols(const SymbolTable &);
};

struct WhileStructure:Statement{
	Expression *condition;
	Block *block;
	WhileStructure(Expression *,NONS_Macro::Block *);
	~WhileStructure();
	std::wstring perform(SymbolTable *symbol_table,ulong *error=0);
	std::wstring perform(SymbolTable symbol_table,ulong *error=0);
	bool checkSymbols(const SymbolTable &);
};

struct ForStructure:Statement{
	Identifier forIndex;
	Expression *start,
		*end,
		*step;
	Block *block;
	ForStructure(const Identifier &,Expression *,Expression *,Expression *,NONS_Macro::Block *);
	~ForStructure();
	std::wstring perform(SymbolTable *symbol_table,ulong *error=0);
	std::wstring perform(SymbolTable symbol_table,ulong *error=0);
	bool checkSymbols(const SymbolTable &);
};

struct MacroCall:Statement{
	Identifier id;
	std::vector<Argument *> *arguments;
	MacroCall(const Identifier &a,std::vector<Argument *> *b=0):id(a),arguments(b){}
	~MacroCall();
	std::wstring perform(SymbolTable symbol_table,ulong *error=0);
	bool checkSymbols(const SymbolTable &);
};

struct Block{
	std::vector<Statement *> *statements;
	SymbolTable *symbol_table;
	Block(std::vector<Statement *> * =0,SymbolTable * =0);
	~Block();
	std::wstring perform(SymbolTable *symbol_table,ulong *error=0,bool doNotAddFrame=0);
	std::wstring perform(SymbolTable symbol_table,ulong *error=0,bool doNotAddFrame=0);
	void addStatement(Statement *);
	//void addStatements(std::vector<Statement *> *);
	bool checkSymbols(const SymbolTable &,bool doNotAddFrame=0);
};

struct Macro{
	SymbolTable *parameters;
	Block *statements;
	Macro(Block *,SymbolTable * =0);
	~Macro();
	std::wstring perform(const std::vector<std::wstring> &parameters,SymbolTable *st,ulong *error=0);
	std::wstring perform(const std::vector<std::wstring> &parameters,SymbolTable st,ulong *error=0);
	bool checkSymbols(const SymbolTable &);
};

struct MacroFile{
	SymbolTable symbol_table;
	std::wstring call(const std::wstring &name,const std::vector<std::wstring> &parameters,ulong *error=0);
	bool checkSymbols();
};
}
#endif
