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

#include "IOFunctions.h"
#include "CommandLineOptions.h"
#include "MacroParser.h"
#include <cassert>

#define THROW_MACRO_ID_ERROR(line,errorMsg,id) o_stderr <<"Macro processor: Error on line "<<(line)<<": "errorMsg" '"<<id<<"'.\n"

namespace NONS_Macro{
bool Identifier::checkSymbols(const SymbolTable &st,bool expectedVariable){
	Symbol *s=st.get(this->id);
	bool r=1;
	if (!s){
		THROW_MACRO_ID_ERROR(this->referenced_on_line,"undefined Symbol",this->id);
		r=0;
	}else{
		if (expectedVariable && s->type!=Symbol::INTEGER && s->type!=Symbol::STRING){
			THROW_MACRO_ID_ERROR(this->referenced_on_line,"Symbol is not a variable",this->id);
			r=0;
		}else if (!expectedVariable && s->type!=Symbol::MACRO){
			THROW_MACRO_ID_ERROR(this->referenced_on_line,"Symbol is not a Macro",this->id);
			r=0;
		}
	}
	return r;
}

Symbol::Symbol(const std::wstring &Identifier,ulong line){
	this->type=UNDEFINED;
	this->Identifier=Identifier;
	this->declared_on_line=line;
	this->int_initialization=0;
	this->str_initialization=0;
	this->has_been_checked=0;
}

Symbol::Symbol(const std::wstring &Identifier,NONS_Macro::Macro *m,ulong line){
	this->type=MACRO;
	this->Identifier=Identifier;
	this->macro=m;
	this->declared_on_line=line;
	this->int_initialization=0;
	this->str_initialization=0;
	this->has_been_checked=0;
}

Symbol::Symbol(const std::wstring &Identifier,const std::wstring &String,ulong line){
	this->type=STRING;
	this->Identifier=Identifier;
	this->str_val=String;
	this->declared_on_line=line;
	this->int_initialization=0;
	this->str_initialization=0;
	this->has_been_checked=0;
}

Symbol::Symbol(const std::wstring &Identifier,long val,ulong line){
	this->type=INTEGER;
	this->Identifier=Identifier;
	this->int_val=val;
	this->declared_on_line=line;
	this->int_initialization=0;
	this->str_initialization=0;
	this->has_been_checked=0;
}

Symbol::Symbol(const Symbol &b){
	this->has_been_checked=b.has_been_checked;
	this->type=b.type;
	this->Identifier=b.Identifier;
	this->declared_on_line=b.declared_on_line;
	switch (this->type){
		case MACRO:
			break;
		case INTEGER:
			this->int_val=b.int_val;
			this->int_initialization=b.int_initialization?b.int_initialization->clone():0;
			break;
		case STRING:
			this->str_val=b.str_val;
			this->str_initialization=b.str_initialization?b.str_initialization->clone():0;
			break;
	}
}

Symbol::~Symbol(){
	switch (this->type){
		case UNDEFINED:
			break;
		case MACRO:
			if (this->macro)
				delete this->macro;
			break;
		case STRING:
			if (this->str_initialization)
				delete this->str_initialization;
			break;
		case INTEGER:
			if (this->int_initialization)
				delete this->int_initialization;
			break;
	}
}

void Symbol::reset(){
	switch (this->type){
		case UNDEFINED:
		case MACRO:
			break;
		case STRING:
			this->str_val.clear();
			break;
		case INTEGER:
			this->int_val=0;
	}
}

void Symbol::set(long n){
	switch (this->type){
		case UNDEFINED:
		case MACRO:
			break;
		case STRING:
			this->str_val=itoaw(n);
			break;
		case INTEGER:
			this->int_val=n;
	}
}

void Symbol::set(const std::wstring &s){
	switch (this->type){
		case UNDEFINED:
		case MACRO:
			break;
		case STRING:
			this->str_val=s;
			break;
		case INTEGER:
			this->int_val=atoi(s);
	}
}

long Symbol::getInt(){
	switch (this->type){
		case MACRO:
			return 0;
		case STRING:
			return atoi(this->str_val);
		case INTEGER:
			return this->int_val;
	}
	return 0;
}

std::wstring Symbol::getStr(){
	switch (this->type){
		case MACRO:
			return 0;
		case STRING:
			return this->str_val;
		case INTEGER:
			return itoaw(this->int_val);
	}
	return L"";
}

void Symbol::initializeTo(Expression *expr){
	this->int_initialization=expr;
}

void Symbol::initializeTo(String *String){
	this->str_initialization=String;
}

ulong Symbol::initialize(const SymbolTable &st){
	ulong error;
	switch (this->type){
		case STRING:
			if (this->str_initialization){
				std::wstring val=this->str_initialization->evaluateToStr(&st,&error);
				if (error!=MACRO_NO_ERROR)
					return error;
				this->set(val);
			}
			break;
		case INTEGER:
			if (this->int_initialization){
				long val=this->int_initialization->evaluateToInt(&st,&error);
				if (error!=MACRO_NO_ERROR)
					return error;
				this->set(val);
			}
		default:
			break;
	}
	return MACRO_NO_ERROR;
}

bool Symbol::checkSymbols(const SymbolTable &st){
	switch (this->type){
		case UNDEFINED:
			THROW_MACRO_ID_ERROR(this->declared_on_line,"Symbol of unknown type",this->Identifier);
			break;
		case MACRO:
			if (!this->has_been_checked){
				this->has_been_checked=1;
				bool ret=this->macro->checkSymbols(st);
				return ret;
			}
		default:
			break;
	}
	return 1;
}

bool SymbolTable::declare(const std::wstring &Identifier,Macro *m,ulong line,bool check){
	if (check && !!this->get(Identifier))
		return 0;
	this->symbols.push_back(new Symbol(Identifier,m,line));
	return 1;
}

bool SymbolTable::declare(const std::wstring &Identifier,const std::wstring &String,ulong line,bool check){
	if (check && !!this->get(Identifier))
		return 0;
	this->symbols.push_back(new Symbol(Identifier,String,line));
	return 1;
}

bool SymbolTable::declare(const std::wstring &Identifier,long value,ulong line,bool check){
	if (check && !!this->get(Identifier))
		return 0;
	this->symbols.push_back(new Symbol(Identifier,value,line));
	return 1;
}

bool SymbolTable::declare(Symbol *s,bool check){
	if (check && !!this->get(s->Identifier))
		return 0;
	this->symbols.push_back(s);
	return 1;
}

void SymbolTable::addFrame(const SymbolTable &st){
	for (ulong b=0;b<st.symbols.size();b++){
		if (st.symbols[b]->type==Symbol::MACRO)
			continue;
		this->symbols.push_back(new Symbol(*st.symbols[b]));
		this->symbols.back()->initialize(st);
	}
}

Symbol *SymbolTable::get(const std::wstring &name) const{
	for (ulong a=0;a<this->symbols.size();a++)
		if (this->symbols[a]->Identifier==name)
			return this->symbols[a];
	return 0;
}

void SymbolTable::resetAll(){
	for (ulong a=0;a<this->symbols.size();a++)
		this->symbols[a]->reset();
}

bool SymbolTable::checkSymbols(){
	bool res=1;
	std::set<std::wstring> s;
	for (ulong a=0;a<this->symbols.size();a++){
		if (s.find(this->symbols[a]->Identifier)!=s.end()){
			THROW_MACRO_ID_ERROR(this->symbols[a]->declared_on_line,"duplicate Symbol",this->symbols[a]->Identifier);
			res=0;
		}else
			s.insert(this->symbols[a]->Identifier);
		res&=this->symbols[a]->checkSymbols(*this);
	}
	return res;
}

bool SymbolTable::checkIntersection(const SymbolTable &st) const{
	bool r=1;
	std::set<std::wstring> set;
	for (ulong a=0;a<st.symbols.size();a++){
		if (this->get(st.symbols[a]->Identifier) || set.find(st.symbols[a]->Identifier)!=set.end()){
			THROW_MACRO_ID_ERROR(st.symbols[a]->declared_on_line,"duplicate Symbol",st.symbols[a]->Identifier);
			r=0;
		}else
			set.insert(st.symbols[a]->Identifier);
	}
	return r;
}

std::wstring ConstantExpression::evaluateToStr(const SymbolTable *st,ulong *error){
	return itoaw(this->evaluateToInt(st,error));
}

long ConstantExpression::evaluateToInt(const SymbolTable *st,ulong *error){
	if (error)
		*error=MACRO_NO_ERROR;
	return this->val;
}

std::wstring VariableExpression::evaluateToStr(const SymbolTable *st,ulong *error){
	if (error)
		*error=MACRO_NO_ERROR;
	Symbol *s=st->get(this->id.id);
	return s->getStr();
}

long VariableExpression::evaluateToInt(const SymbolTable *st,ulong *error){
	if (error)
		*error=MACRO_NO_ERROR;
	Symbol *s=st->get(this->id.id);
	return s->getInt();
}

bool VariableExpression::checkSymbols(const SymbolTable &st){
	return this->id.checkSymbols(st,1);
}

FullExpression::FullExpression(yytokentype op,Expression *A,Expression *B,Expression *C){
	this->operation=op;
	this->operands[0]=A;
	this->operands[1]=B;
	this->operands[2]=C;
}

FullExpression::FullExpression(const FullExpression &b)
		:operation(b.operation){
	memset(this->operands,0,sizeof(Expression *)*3);
	for (ulong a=0;a<3 && b.operands[a];a++)
		this->operands[a]=b.operands[a]->clone();
}

FullExpression::~FullExpression(){
	for (ulong a=0;a<3 && this->operands[a];a++)
		delete this->operands[a];
}

bool FullExpression::simplify(){
	bool r=1;
	for (ulong a=0;a<3 && this->operands[a];a++){
		bool temp=this->operands[a]->simplify();
		r&=temp;
		if (temp){
			long val=this->operands[a]->evaluateToInt();
			delete this->operands[a];
			this->operands[a]=new ConstantExpression(val);
		}
	}
	return r;
}

Expression *simplifyExpression(Expression *e){
	if (!e->simplify())
		return e;
	ConstantExpression *ret=new ConstantExpression(e->evaluateToInt());
	delete e;
	return ret;
}

std::wstring FullExpression::evaluateToStr(const SymbolTable *st,ulong *error){
	return itoaw(this->evaluateToInt(st,error));
}

#define EVALUATETOINT_EVAL(i) {\
	ulong error2;\
	ops[(i)]=this->operands[(i)]->evaluateToInt(st,&error2);\
	if (error2!=MACRO_NO_ERROR){\
		if (!!error)\
			*error=error2;\
		return 0;\
	}\
}


long FullExpression::evaluateToInt(const SymbolTable *st,ulong *error){
	long ops[3]={0},
		res;
	EVALUATETOINT_EVAL(0);
	switch (this->operation){
		case TRINARY:
			if (ops[0]){
				EVALUATETOINT_EVAL(1);
				res=ops[1];
			}else{
				EVALUATETOINT_EVAL(2);
				res=ops[2];
			}
			break;
		case BOR:
			if (ops[0])
				res=ops[0];
			else{
				EVALUATETOINT_EVAL(1);
				res=ops[1];
			}
			break;
		case BAND:
			if (!ops[0])
				res=ops[0];
			else{
				EVALUATETOINT_EVAL(1);
				res=ops[1];
			}
			break;
		case BNOT:
			res=!ops[0];
			break;
		case NOT_EQUALS:
			EVALUATETOINT_EVAL(1);
			res=ops[0]!=ops[1];
			break;
		case EQUALS:
			EVALUATETOINT_EVAL(1);
			res=ops[0]==ops[1];
			break;
		case LT_EQUALS:
			EVALUATETOINT_EVAL(1);
			res=ops[0]<=ops[1];
			break;
		case GT_EQUALS:
			EVALUATETOINT_EVAL(1);
			res=ops[0]>=ops[1];
			break;
		case LOWER_THAN:
			EVALUATETOINT_EVAL(1);
			res=ops[0]<ops[1];
			break;
		case GREATER_THAN:
			EVALUATETOINT_EVAL(1);
			res=ops[0]>ops[1];
			break;
		case PLUS:
			EVALUATETOINT_EVAL(1);
			res=ops[0]+ops[1];
			break;
		case MINUS:
			EVALUATETOINT_EVAL(1);
			res=ops[0]-ops[1];
			break;
		case MUL:
			EVALUATETOINT_EVAL(1);
			res=ops[0]*ops[1];
			break;
		case DIV:
			EVALUATETOINT_EVAL(1);
			res=ops[0]/ops[1];
			break;
		case MOD:
			EVALUATETOINT_EVAL(1);
			res=ops[0]%ops[1];
			break;
		case POS:
			res=ops[0];
			break;
		case NEG:
			res=-ops[0];
			break;
		default: //Impossible
			assert(0);
			res=0;
			break;
	}
	if (!!error)
		*error=MACRO_NO_ERROR;
	return res;
}

bool FullExpression::checkSymbols(const SymbolTable &st){
	bool r=1;
	for (ulong a=0;a<3 && this->operands[a];a++)
		r=r & this->operands[a]->checkSymbols(st);
	return r;
}

std::wstring ConstantString::evaluateToStr(const SymbolTable *st,ulong *error){
	if (error)
		*error=MACRO_NO_ERROR;
	return this->val;
}

long ConstantString::evaluateToInt(const SymbolTable *st,ulong *error){
	if (error)
		*error=MACRO_NO_ERROR;
	return atoi(this->val);
}

std::wstring VariableString::evaluateToStr(const SymbolTable *st,ulong *error){
	if (error)
		*error=MACRO_NO_ERROR;
	Symbol *s=st->get(this->id.id);
	return s->getStr();
}

long VariableString::evaluateToInt(const SymbolTable *st,ulong *error){
	if (error)
		*error=MACRO_NO_ERROR;
	Symbol *s=st->get(this->id.id);
	return s->getInt();
}

bool VariableString::checkSymbols(const SymbolTable &st){
	return this->id.checkSymbols(st,1);
}

StringConcatenation::StringConcatenation(String *A,String *B){
	this->operands[0]=A;
	this->operands[1]=B;
}

StringConcatenation::StringConcatenation(const StringConcatenation &a){
	this->operands[0]=a.operands[0]->clone();
	this->operands[1]=a.operands[0]?a.operands[0]->clone():0;
}

StringConcatenation::~StringConcatenation(){
	delete this->operands[0];
	delete this->operands[1];
}

bool StringConcatenation::simplify(){
	bool r=1;
	for (ulong a=0;a<2 && this->operands[a];a++){
		bool temp=this->operands[a]->simplify();
		r&=temp;
		if (temp){
			std::wstring val=this->operands[a]->evaluateToStr();
			delete this->operands[a];
			this->operands[a]=new ConstantString(val);
		}
	}
	return r;
}

String *simplifyExpression(String *e){
	if (!e->simplify())
		return e;
	ConstantString *ret=new ConstantString(e->evaluateToStr());
	delete e;
	return ret;
}

std::wstring StringConcatenation::evaluateToStr(const SymbolTable *st,ulong *error){
	std::wstring res;
	ulong error2;
	for (ulong a=0;a<2 && this->operands[a];a++){
		std::wstring temp=this->operands[a]->evaluateToStr(st,&error2);
		if (error2!=MACRO_NO_ERROR){
			if (error)
				*error=error2;
			return L"";
		}
		res.append(temp);
	}
	if (error)
		*error=MACRO_NO_ERROR;
	return res;
}

long StringConcatenation::evaluateToInt(const SymbolTable *st,ulong *error){
	return atoi(this->evaluateToStr(st,error));
}

bool StringConcatenation::checkSymbols(const SymbolTable &st){
	bool r=1;
	for (ulong a=0;a<2 && this->operands[a];a++)
		r&=this->operands[a]->checkSymbols(st);
	return r;
}

std::wstring EmptyStatement::perform(SymbolTable symbol_table,ulong *error){
	if (!!error)
		*error=MACRO_NO_ERROR;
	return L"";
}

template <typename T>
bool partialCompare(const std::basic_string<T> &A,size_t offset,const std::basic_string<T> &B){
	if (A.size()-offset<B.size())
		return 0;
	for (size_t a=offset,b=0;b<B.size();a++,b++)
		if (A[a]!=B[b])
			return 0;
	return 1;
}

std::wstring DataBlock::replace(const std::wstring &src,const SymbolTable &symbol_table){
	if (!symbol_table.symbols.size())
		return src;
	const std::vector<Symbol *> &symbols=symbol_table.symbols;
	std::wstring res;
	for (ulong a=0;a<src.size();a++){
		wchar_t c=src[a];
		bool _continue=0;
		for (ulong b=0;b<symbols.size() && !_continue;b++){
			if (partialCompare(src,a,symbols[b]->Identifier)){
				if (symbols[b]->type==Symbol::STRING){
					res.append(symbols[b]->str_val);
					a+=symbols[b]->Identifier.size()-1;
					_continue=1;
				}else if (symbols[b]->type==Symbol::INTEGER){
					std::wstring temp=itoaw(symbols[b]->int_val);
					res.append(temp);
					a+=symbols[b]->Identifier.size()-1;
					_continue=1;
				}
			}
		}
		if (_continue)
			continue;
		res.push_back(c);
	}
	return res;
}

std::wstring DataBlock::perform(SymbolTable symbol_table,ulong *error){
	if (!!error)
		*error=MACRO_NO_ERROR;
	return DataBlock::replace(this->data,symbol_table);
}

std::wstring AssignmentStatement::perform(SymbolTable st,ulong *error){
	ulong error2;
	long val=this->src->evaluateToInt(&st,&error2);
	if (error2!=MACRO_NO_ERROR){
		if (!!error)
			*error=error2;
		return L"";
	}
	st.get(this->dst.id)->set(val);
	if (!!error)
		*error=MACRO_NO_ERROR;
	return L"";
}

bool AssignmentStatement::checkSymbols(const SymbolTable &st){
	return this->dst.checkSymbols(st,1) && this->src->checkSymbols(st);
}

StringAssignmentStatement::~StringAssignmentStatement(){
	delete this->src;
}

std::wstring StringAssignmentStatement::perform(SymbolTable st,ulong *error){
	ulong error2;
	std::wstring val=this->src->evaluateToStr(&st,&error2);
	if (error2!=MACRO_NO_ERROR){
		if (!!error)
			*error=error2;
		return L"";
	}
	st.get(this->dst.id)->set(val);
	if (!!error)
		*error=MACRO_NO_ERROR;
	return L"";
}

bool StringAssignmentStatement::checkSymbols(const SymbolTable &st){
	return this->dst.checkSymbols(st,1) & this->src->checkSymbols(st);
}

IfStructure::IfStructure(Expression *a,Block *b,Block *c){
	this->condition=a;
	this->true_block=b;
	this->false_block=c;
}

IfStructure::~IfStructure(){
	delete this->condition;
	delete this->true_block;
	if (this->false_block)
		delete this->false_block;
}

std::wstring IfStructure::perform(SymbolTable st,ulong *error){
	ulong error2;
	bool condition=!!this->condition->evaluateToInt(&st,&error2);
	if (error2!=MACRO_NO_ERROR){
		if (!!error)
			*error=error2;
		return L"";
	}
	if (condition)
		return this->true_block->perform(&st,error);
	else if (!!this->false_block)
		return this->false_block->perform(&st,error);
	return L"";
}

bool IfStructure::checkSymbols(const SymbolTable &st){
	bool r=this->condition->checkSymbols(st);
	r&=this->true_block->checkSymbols(st);
	if (this->false_block)
		r&=this->false_block->checkSymbols(st);
	return r;
}

WhileStructure::WhileStructure(Expression *a,NONS_Macro::Block *b){
	this->condition=a;
	this->block=b;
}

WhileStructure::~WhileStructure(){
	delete this->condition;
	delete this->block;
}

std::wstring WhileStructure::perform(SymbolTable *st,ulong *error){
	ulong error2=0;
	std::wstring res;
	if (this->block->symbol_table)
		st->addFrame(*this->block->symbol_table);
	while (1){
		long _while=this->condition->evaluateToInt(st,&error2);
		if (error2!=MACRO_NO_ERROR){
			if (!!error)
				*error=error2;
			return L"";
		}
		if (!_while)
			break;
		res.append(this->block->perform(st,&error2,1));
		if (error2!=MACRO_NO_ERROR){
			if (!!error)
				*error=error2;
			return L"";
		}
	}
	if (!!error)
		*error=MACRO_NO_ERROR;
	return res;
}

std::wstring WhileStructure::perform(SymbolTable st,ulong *error){
	return this->perform(&st,error);
}

bool WhileStructure::checkSymbols(const SymbolTable &st){
	SymbolTable st2=st;
	bool r;
	if (this->block->symbol_table){
		r=st.checkIntersection(*this->block->symbol_table);
		st2.addFrame(*this->block->symbol_table);
	}
	r&=this->condition->checkSymbols(st2);
	r&=this->block->checkSymbols(st2,1);
	return r;
}

ForStructure::ForStructure(const Identifier &s,Expression *e1,Expression *e2,Expression *e3,NONS_Macro::Block *b):forIndex(s){
	this->start=e1;
	this->end=e2;
	this->step=e3;
	this->block=b;
}

ForStructure::~ForStructure(){
	delete this->start;
	delete this->end;
	delete this->step;
	delete this->block;
}

std::wstring ForStructure::perform(SymbolTable *st,ulong *error){
	ulong error2=0;
	std::wstring res;
	st->declare(this->forIndex.id,(long)0,this->forIndex.referenced_on_line,0);
	if (this->block->symbol_table)
		st->addFrame(*this->block->symbol_table);
	long start=this->start->evaluateToInt(st,&error2);
	if (error2!=MACRO_NO_ERROR){
		if (!!error)
			*error=error2;
		return L"";
	}
	Symbol *s=st->get(this->forIndex.id);
	s->set(start);
	while (1){
		long end=this->end->evaluateToInt(st,&error2);
		if (error2!=MACRO_NO_ERROR){
			if (!!error)
				*error=error2;
			return L"";
		}
		if (s->getInt()>end)
			break;
		res.append(this->block->perform(st,&error2,1));
		if (error2!=MACRO_NO_ERROR){
			if (!!error)
				*error=error2;
			return L"";
		}
		long step=this->step->evaluateToInt(st,&error2);
		if (error2!=MACRO_NO_ERROR){
			if (!!error)
				*error=error2;
			return L"";
		}
		s->set(s->getInt()+step);
	}
	if (!!error)
		*error=MACRO_NO_ERROR;
	return res;
}

std::wstring ForStructure::perform(SymbolTable st,ulong *error){
	return this->perform(&st,error);
}

bool ForStructure::checkSymbols(const SymbolTable &st){
	SymbolTable st2=st;
	bool r=st2.declare(this->forIndex.id,(long)0,this->forIndex.referenced_on_line,0);
	if (this->block->symbol_table){
		r&=st.checkIntersection(*this->block->symbol_table);
		st2.addFrame(*this->block->symbol_table);
	}
	r&=this->forIndex.checkSymbols(st2,1);
	r&=this->start->checkSymbols(st2);
	r&=this->end->checkSymbols(st2);
	r&=this->step->checkSymbols(st2);
	r&=this->block->checkSymbols(st2,1);
	return r;
}

MacroCall::~MacroCall(){
	if (this->arguments){
		for (ulong a=0;a<this->arguments->size();a++)
			delete (*this->arguments)[a];
		delete this->arguments;
	}
}

std::wstring MacroCall::perform(SymbolTable st,ulong *error){
	std::vector<std::wstring> parameters;
	if (!!this->arguments){
		parameters.resize(this->arguments->size());
		ulong error2;
		for (ulong a=0;a<parameters.size();a++){
			parameters[a]=(*this->arguments)[a]->evaluateToStr(&st,&error2);
			if (error2!=MACRO_NO_ERROR){
				if (!!error)
					*error=error2;
				return L"";
			}
		}
	}
	return st.get(this->id.id)->macro->perform(parameters,&st,error);
}

bool MacroCall::checkSymbols(const SymbolTable &st){
	bool r=this->id.checkSymbols(st,0);
	for (ulong a=0;a<this->arguments->size();a++)
		r&=(*this->arguments)[a]->checkSymbols(st);
	return r;
}

Block::Block(std::vector<Statement *> *a,SymbolTable *b){
	this->statements=a;
	this->symbol_table=b;
}

Block::~Block(){
	for (ulong a=0;a<this->statements->size();a++)
		delete (*this->statements)[a];
	delete this->statements;
}

std::wstring Block::perform(SymbolTable *symbol_table,ulong *error,bool doNotAddFrame){
	if (!doNotAddFrame && this->symbol_table)
		symbol_table->addFrame(*this->symbol_table);
	std::wstring res;
	ulong error2=0;
	for (ulong a=0;a<this->statements->size();a++){
		res.append((*this->statements)[a]->perform(*symbol_table,&error2));
		if (error2){
			if (!!error)
				*error=error2;
			return L"";
		}
	}
	if (!!error)
		*error=MACRO_NO_ERROR;
	return res;
}

std::wstring Block::perform(SymbolTable symbol_table,ulong *error,bool doNotAddFrame){
	return this->perform(&symbol_table,error,doNotAddFrame);
}

void Block::addStatement(Statement *s){
	if (!this->statements)
		this->statements=new std::vector<Statement *>;
	this->statements->push_back(s);
}

bool Block::checkSymbols(const SymbolTable &st,bool doNotAddFrame){
	SymbolTable st2=st;
	bool r=1;
	if (!doNotAddFrame && this->symbol_table && (r=st.checkIntersection(*this->symbol_table)))
		st2.addFrame(*this->symbol_table);
	for (ulong a=0;a<this->statements->size();a++)
		r&=(*this->statements)[a]->checkSymbols(st2);
	return r;
}

Macro::Macro(Block *c,SymbolTable *b){
	this->statements=c;
	this->parameters=b;
	if (!this->parameters)
		this->parameters=new SymbolTable;
	this->parameters->declare(L"^calls",(long)0,0,0);
}

Macro::~Macro(){
	delete this->parameters;
	delete this->statements;
}

std::wstring Macro::perform(const std::vector<std::wstring> &parameters,SymbolTable *st,ulong *error){
	Symbol *calls=this->parameters->get(L"^calls");
	calls->int_val++;
	ulong first=st->symbols.size();
	if (this->parameters)
		st->addFrame(*this->parameters);
	for (ulong a=first;a<st->symbols.size() && a-first<parameters.size();a++)
		st->symbols[a]->set(parameters[a-first]);
	return this->statements->perform(st,error);
}

std::wstring Macro::perform(const std::vector<std::wstring> &parameters,SymbolTable st,ulong *error){
	return this->perform(parameters,&st,error);
}

bool Macro::checkSymbols(const SymbolTable &st){
	SymbolTable st2=st;
	bool r=1;
	if (this->parameters && (r=st.checkIntersection(*this->parameters)))
		st2.addFrame(*this->parameters);
	return r & this->statements->checkSymbols(st2);
}

std::wstring MacroFile::call(const std::wstring &name,const std::vector<std::wstring> &parameters,ulong *error){
	Symbol *s=this->symbol_table.get(name);
	if (!s || s->type!=Symbol::MACRO){
		if (!!error)
			*error=MACRO_NO_SUCH_MACRO;
		return L"";
	}
	return s->macro->perform(parameters,this->symbol_table,error);
}

bool MacroFile::checkSymbols(){
	return this->symbol_table.checkSymbols();
}
}

//0: no changes; !0: something changed
bool preprocess(std::wstring &dst,const std::wstring &script,NONS_Macro::MacroFile &macros,bool output);

bool preprocess(std::wstring &dst,const std::wstring &script){
	ulong l;
	char *temp;
	bool ret;
	if (temp=(char *)NONS_File::read(L"macros.txt",l)){
		std::wstringstream stream(UniFromUTF8(std::string(temp,l)));
		delete[] temp;
		NONS_Macro::MacroFile *MacroFile;
		bool pointerIsValid=!macroParser_yyparse(stream,MacroFile);
		if (pointerIsValid && MacroFile->checkSymbols() && preprocess(dst,script,*MacroFile,1)){
			if (CLOptions.outputPreprocessedFile){
				std::string str2=UniToUTF8(dst);
				NONS_File::write(CLOptions.preprocessedFile,&str2[0],str2.size());
			}
			ret=1;
		}else{
			if (CLOptions.outputPreprocessedFile){
				std::string str2=UniToUTF8(script);
				NONS_File::write(CLOptions.preprocessedFile,&str2[0],str2.size());
			}
			ret=0;
		}
		if (pointerIsValid)
			delete MacroFile;
	}else{
		if (CLOptions.outputPreprocessedFile){
			std::string str2=UniToUTF8(script);
			NONS_File::write(CLOptions.preprocessedFile,&str2[0],str2.size());
		}
		ret=0;
	}
	if (CLOptions.preprocessAndQuit){
		if (!ret)
			o_stderr <<"Preprocessing failed in some way.\n";
		exit(0);
	}
	return ret;
}

bool readIdentifier(std::wstring &dst,const std::wstring &src,ulong offset){
	if (!NONS_isid1char(src[offset]))
		return 1;
	ulong len=1;
	while (NONS_isidnchar(src[offset+len]))
		len++;
	if (offset+len>=src.size())
		return 0;
	dst=src.substr(offset,len);
	return 1;
}

ulong countLinesUntilOffset(const std::wstring &str,ulong offset){
	ulong res=1;
	for (ulong a=0;a<offset && a<str.size();a++){
		if (str[a]==10)
			res++;
		else if (str[a]==13){
			res++;
			if (a+1<str.size() && str[a+1]==10)
				a++;
		}
	}
	return res;
}

template <typename T>
bool partialCompare(const std::basic_string<T> &A,size_t offset,const std::basic_string<T> &B){
	if (A.size()-offset<B.size())
		return 0;
	for (size_t a=offset,b=0;b<B.size();a++,b++)
		if (A[a]!=B[b])
			return 0;
	return 1;
}

bool preprocess(std::wstring &dst,const std::wstring &script,NONS_Macro::MacroFile &macros,bool output){
	ulong first=script.find(L";#call");
	if (first==script.npos)
		return 0;
	std::deque<ulong> calls;
	while (first!=script.npos && first+7<script.size()){
		wchar_t c=script[first+6];
		if (iswhitespace(c))
			calls.push_back(first+6);
		first=script.find(L";#call",first+6);
	}
	if (!calls.size())
		return 0;
	ulong callsCompleted=0;
	dst.append(script.substr(0,calls.front()-6));
	static const std::wstring open_block=L"block{",
		close_block=L"}block";
	while (calls.size()){
		first=script.find_first_not_of(WCS_WHITESPACE,calls.front());
		ulong lineNo=countLinesUntilOffset(script,calls.front());
		std::wstring identifier;
		if (!readIdentifier(identifier,script,first)){
			o_stderr <<"Warning: Line "<<lineNo<<": Possible macro call truncated by the end of file: \'"<<identifier<<"\'.\n";
			return !!callsCompleted;
		}
		first+=identifier.size();
		first=script.find_first_not_of(WCS_WHITESPACE,first);
		if (!macros.symbol_table.get(identifier)){
			o_stderr <<"Warning: Line "<<lineNo
				<<": Possible macro call doesn't call a defined macro: \'"<<identifier<<"\'.\n";
			while (calls.size() && calls.front()<=first)
				calls.pop_front();
			goto preprocess_000;
		}
		if (script[first]!='('){
			o_stderr <<"Warning: Line "<<(ulong)lineNo<<": Possible macro call invalid. Expected \'(\' but found \'"<<script[first]<<"\'.\n";
			goto preprocess_000;
		}
		{
			std::vector<std::wstring> arguments;
			bool found_lparen=0;
			first++;
			while (!found_lparen){
				std::wstring argument;
				if (partialCompare(script,first,open_block)){
					do{
						first+=6;
						ulong len=script.find(close_block,first);
						if (len==script.npos){
							o_stderr <<"Warning: Line "<<lineNo<<": Possible macro call invalid. Expected \"}block\" but found end of file.\n";
							goto preprocess_000;
						}
						len-=first;
						argument.append(script.substr(first,len));
						first+=len+6;
						first=script.find_first_not_of(WCS_WHITESPACE,first);
					}while (first!=script.npos && partialCompare(script,first,open_block));
					if (first==script.npos){
						o_stderr <<"Warning: Line "<<lineNo<<": Possible macro call invalid. Expected \',\' or \')\' but found end of file.\n";
						goto preprocess_000;
					}
					if (script[first]!=',' && script[first]!=')'){
						o_stderr <<"Warning: Line "<<lineNo<<": Possible macro call invalid. Expected \',\' or \')\' but found \'"<<script[first]<<"\'.\n";
						goto preprocess_000;
					}
					if (script[first++]==')')
						found_lparen=1;
				}else{
					ulong second=first;
					bool found_comma=0;
					for (;second<script.size() && !found_comma;second++){
						switch (script[second]){
							case '\\':
								if (second+1>=script.size()){
									o_stderr <<"Warning: Line "<<lineNo<<": Possible macro call invalid. Incomplete escape sequence.\n";
									goto preprocess_000;
								}
								argument.push_back(script[++second]);
								break;
							case ')':
								found_lparen=1;
							case ',':
								found_comma=1;
								break;
							default:
								argument.push_back(script[second]);
						}
					}
					first=second;
				}
				arguments.push_back(argument);
			}
			std::wstring macroResult=macros.call(identifier,arguments);
			while (1){
				std::wstring temp;
				if (!preprocess(temp,macroResult,macros,0))
					break;
				macroResult=temp;
			}
			callsCompleted++;
			if (output && CLOptions.outputPreprocessedFile)
				o_stderr <<"Call to macro \""<<identifier<<"\" on line "<<lineNo<<" was written to the result on line "<<countLinesUntilOffset(dst,dst.size())<<".\n";
			dst.append(macroResult);
		}
preprocess_000:
		while (calls.size() && calls.front()<first)
			calls.pop_front();
		if (calls.size())
			dst.append(script.substr(first,calls.front()-6-first));
		else
			dst.append(script.substr(first));
	}
	return !!callsCompleted;
}
