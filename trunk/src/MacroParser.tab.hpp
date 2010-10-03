/* A Bison parser, made by GNU Bison 2.4.2.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2006, 2009-2010 Free Software
   Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* "%code requires" blocks.  */


#include "Common.h"
#include <set>
#include <vector>
#include <string>
#undef ERROR

namespace NONS_Macro{
struct Identifier;
struct StringOperation;
struct Argument;
struct Expression;
struct String;
struct ConstantExpression;
struct VariableExpression;
struct FullExpression;
struct ConstantString;
struct VariableString;
struct StringConcatenation;
struct Statement;
struct Macro;
struct MacroFile;
struct Symbol;
struct SymbolTable;
struct Block;
}





/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     APOSTROPHE = 258,
     DEFINE = 259,
     IF = 260,
     ELSE = 261,
     ERROR = 262,
     FOR = 263,
     WHILE = 264,
     IDENTIFIER = 265,
     STRING = 266,
     INTEGER = 267,
     CONCAT_ASSIGNMENT = 268,
     TRINARY = 269,
     BOR = 270,
     BAND = 271,
     BNOT = 272,
     NOT_EQUALS = 273,
     EQUALS = 274,
     LT_EQUALS = 275,
     GT_EQUALS = 276,
     LOWER_THAN = 277,
     GREATER_THAN = 278,
     MINUS = 279,
     PLUS = 280,
     MOD = 281,
     DIV = 282,
     MUL = 283,
     NEG = 284,
     POS = 285
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{


	NONS_Macro::Identifier *id;
	std::wstring *str;
	std::vector<std::wstring> *stringVector;
	NONS_Macro::Argument *argument;
	NONS_Macro::String *string;
	NONS_Macro::Expression *expression;
	std::vector<NONS_Macro::Argument *> *argumentVector;
	NONS_Macro::Statement *stmt;
	NONS_Macro::MacroFile *macro_file;
	NONS_Macro::Symbol *symbol;
	NONS_Macro::SymbolTable *symbol_table;
	NONS_Macro::Block *block;
	std::vector<NONS_Macro::Statement *> *stmt_list;



} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif



#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



/* "%code provides" blocks.  */


	#include <sstream>
	int macroParser_yyparse(
		std::wstringstream &stream,
		NONS_Macro::MacroFile *&result
	);
	int macroParser_yylex(
		YYSTYPE *yylval,
		YYLTYPE *yylloc,
		std::wstringstream &stream
	);
	void macroParser_yyerror(
		YYLTYPE *yylloc,
		std::wstringstream &,
		NONS_Macro::MacroFile *&result,
		//NONS_Macro::SymbolTable &SymbolTable,
		char const *
	);
	extern int macroParser_yydebug;



