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


#include <string>
#include <sstream>
#include "Functions.h"
#include "MacroParser.h"
#undef ERROR




/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     PUSH = 258,
     PUSH_LINE = 259,
     CALL = 260,
     PARAMS = 261,
     END_KEY = 262,
     COMMA_KEY = 263,
     POP = 264,
     ERROR = 265,
     TEXT = 266,
     IDENTIFIER = 267,
     STRING = 268
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{


	std::wstring *str;
	NONS_Macro::file *file;
	std::vector<NONS_Macro::file_element *> *fe_list;
	NONS_Macro::file_element *file_element;
	NONS_Macro::text *text;
	NONS_Macro::push *push;
	NONS_Macro::call *call;
	std::vector<std::wstring> *params;



} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif




/* "%code provides" blocks.  */


	int macroParser_yyparse(
		cheap_input_stream &stream,
		ParserState::ParserState &state,
		std::deque<wchar_t> &token_queue,
		NONS_Macro::file *&file
	);
	int macroParser_yylex(
		YYSTYPE *yylval,
		cheap_input_stream &stream,
		ParserState::ParserState &state,
		std::deque<wchar_t> &token_queue
	);
	inline void macroParser_yyerror(
		cheap_input_stream &,
		ParserState::ParserState &,
		std::deque<wchar_t> &,
		NONS_Macro::file *file,
		char const *
	){}
	extern int macroParser_yydebug;



