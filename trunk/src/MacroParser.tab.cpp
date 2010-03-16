
/* A Bison parser, made by GNU Bison 2.4.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 1

/* Substitute the variable and function names.  */
#define yyparse         macroParser_yyparse
#define yylex           macroParser_yylex
#define yyerror         macroParser_yyerror
#define yylval          macroParser_yylval
#define yychar          macroParser_yychar
#define yydebug         macroParser_yydebug
#define yynerrs         macroParser_yynerrs
#define yylloc          macroParser_yylloc

/* Copy the first part of user declarations.  */


#include "Functions.h"
#include "MacroParser.h"

template <typename T>
bool partialCompare(const std::basic_string<T> &A,size_t offset,const std::basic_string<T> &B){
	if (A.size()-offset<B.size())
		return 0;
	for (size_t a=offset,b=0;b<B.size();a++,b++)
		if (A[a]!=B[b])
			return 0;
	return 1;
}



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

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
     CODE_BLOCK = 267,
     INTEGER = 268,
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




/* Copy the second part of user declarations.  */



#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   418

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  43
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  14
/* YYNRULES -- Number of rules.  */
#define YYNRULES  58
/* YYNRULES -- Number of states.  */
#define YYNSTATES  125

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   285

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    33,     2,
      31,    32,     2,     2,    34,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    42,    40,
       2,    39,     2,    41,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    37,     2,    38,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    35,     2,    36,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     5,     7,     8,    11,    15,    22,    24,
      28,    30,    34,    41,    42,    45,    47,    51,    53,    57,
      61,    63,    66,    71,    76,    82,    90,    96,   108,   111,
     117,   119,   123,   125,   127,   129,   133,   137,   141,   145,
     147,   149,   155,   159,   163,   166,   170,   174,   178,   182,
     186,   190,   194,   198,   202,   206,   210,   213,   216
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      44,     0,    -1,    45,    -1,     7,    -1,    -1,    45,    46,
      -1,     4,    10,    48,    -1,     4,    10,    31,    47,    32,
      48,    -1,    10,    -1,    47,    34,    10,    -1,    52,    -1,
      35,    49,    36,    -1,    37,    50,    38,    35,    49,    36,
      -1,    -1,    49,    52,    -1,    51,    -1,    50,    34,    51,
      -1,    10,    -1,    10,    39,    56,    -1,    10,    39,    55,
      -1,    40,    -1,    12,    40,    -1,    10,    39,    56,    40,
      -1,    10,    39,    55,    40,    -1,     5,    31,    56,    32,
      48,    -1,     5,    31,    56,    32,    48,     6,    48,    -1,
       9,    31,    56,    32,    48,    -1,     8,    31,    10,    40,
      56,    40,    56,    40,    56,    32,    48,    -1,    10,    40,
      -1,    10,    31,    53,    32,    40,    -1,    54,    -1,    53,
      34,    54,    -1,    55,    -1,    56,    -1,    11,    -1,    10,
      33,    10,    -1,    10,    33,    55,    -1,    55,    33,    10,
      -1,    55,    33,    55,    -1,    13,    -1,    10,    -1,    56,
      41,    56,    42,    56,    -1,    56,    15,    56,    -1,    56,
      16,    56,    -1,    17,    56,    -1,    56,    19,    56,    -1,
      56,    18,    56,    -1,    56,    23,    56,    -1,    56,    22,
      56,    -1,    56,    21,    56,    -1,    56,    20,    56,    -1,
      56,    25,    56,    -1,    56,    24,    56,    -1,    56,    28,
      56,    -1,    56,    27,    56,    -1,    56,    26,    56,    -1,
      25,    56,    -1,    24,    56,    -1,    31,    56,    32,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   155,   155,   158,   164,   167,   173,   177,   183,   188,
     195,   199,   202,   207,   210,   216,   220,   226,   229,   234,
     241,   244,   248,   252,   256,   260,   264,   268,   275,   279,
     285,   289,   295,   299,   305,   309,   314,   318,   322,   327,
     330,   334,   337,   340,   343,   346,   349,   352,   355,   358,
     361,   364,   367,   370,   373,   376,   379,   382,   385
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "APOSTROPHE", "DEFINE", "IF", "ELSE",
  "ERROR", "FOR", "WHILE", "IDENTIFIER", "STRING", "CODE_BLOCK", "INTEGER",
  "TRINARY", "BOR", "BAND", "BNOT", "NOT_EQUALS", "EQUALS", "LT_EQUALS",
  "GT_EQUALS", "LOWER_THAN", "GREATER_THAN", "MINUS", "PLUS", "MOD", "DIV",
  "MUL", "NEG", "POS", "'('", "')'", "'&'", "','", "'{'", "'}'", "'['",
  "']'", "'='", "';'", "'?'", "':'", "$accept", "begin", "file",
  "macro_definition", "parameter_list", "block", "stmt_list",
  "initialization_list", "initialization", "stmt", "argument_list",
  "argument", "string", "expr", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,    40,    41,    38,    44,   123,   125,    91,    93,    61,
      59,    63,    58
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    43,    44,    44,    45,    45,    46,    46,    47,    47,
      48,    48,    48,    49,    49,    50,    50,    51,    51,    51,
      52,    52,    52,    52,    52,    52,    52,    52,    52,    52,
      53,    53,    54,    54,    55,    55,    55,    55,    55,    56,
      56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
      56,    56,    56,    56,    56,    56,    56,    56,    56
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     0,     2,     3,     6,     1,     3,
       1,     3,     6,     0,     2,     1,     3,     1,     3,     3,
       1,     2,     4,     4,     5,     7,     5,    11,     2,     5,
       1,     3,     1,     1,     1,     3,     3,     3,     3,     1,
       1,     5,     3,     3,     2,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     2,     2,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,     3,     0,     2,     1,     0,     5,     0,     0,     0,
       0,     0,     0,     0,    13,     0,    20,     6,    10,     0,
       0,     0,     0,     0,    28,    21,     8,     0,     0,    17,
       0,    15,    40,    39,     0,     0,     0,     0,     0,     0,
       0,    40,    34,     0,    30,    32,    33,     0,     0,     0,
       0,    11,    14,     0,     0,     0,    44,    57,    56,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    23,    22,     7,     9,    19,    18,    16,    13,    58,
      42,    43,    46,    45,    50,    49,    48,    47,    52,    51,
      55,    54,    53,    24,     0,     0,    26,    35,    36,    29,
      31,    37,    38,     0,     0,     0,     0,    12,    25,    41,
       0,     0,     0,     0,    27
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     2,     3,     6,    27,    17,    28,    30,    31,    18,
      43,    44,    45,    46
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -47
static const yytype_int16 yypact[] =
{
      -1,   -47,     9,    15,   -47,    23,   -47,    64,    -6,    32,
      33,    -8,    14,    55,   -47,    56,   -47,   -47,   -47,   375,
      57,   375,    -3,    -3,   -47,   -47,   -47,     5,    79,    31,
     -14,   -47,   -47,   -47,   375,   375,   375,   375,   133,    41,
     157,    38,   -47,     6,   -47,    52,   310,    -4,   181,    70,
      73,   -47,   -47,    -3,    56,    58,   345,    49,    49,   208,
     375,   375,   375,   375,   375,   375,   375,   375,   375,   375,
     375,   375,   375,    70,   375,   375,    70,    47,    54,    -3,
      50,   -47,   -47,   -47,   -47,    52,   310,   -47,   -47,   -47,
     334,   345,   356,   356,   377,   377,   377,   377,   -15,   -15,
      49,    49,    49,    86,   105,   232,   -47,    38,   -47,   -47,
     -47,    38,   -47,   104,    70,   375,   375,   -47,   -47,   310,
     259,   375,   286,    70,   -47
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -47,   -47,   -47,   -47,   -47,   -46,    10,   -47,    46,   -27,
     -47,    24,   -18,   -19
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      38,    52,    40,    83,    48,    47,     1,    41,    42,     4,
      33,    70,    71,    72,    34,    56,    57,    58,    59,     5,
      54,    35,    36,    22,    55,    19,    74,   103,    37,    80,
     106,    23,    24,     7,    86,    85,    81,    49,    78,    50,
      79,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,    25,   104,   105,   107,    42,   108,
     111,    42,   112,    20,    21,    26,    29,    39,   118,     8,
      53,    77,     9,    10,    11,     8,    12,   124,     9,    10,
      11,    75,    12,    84,     8,    80,    52,     9,    10,    11,
      74,    12,   114,    88,   109,    13,   119,   120,   113,    14,
      87,    15,   122,   110,    16,    14,     0,    15,     0,     8,
      16,     0,     9,    10,    11,    51,    12,     0,     0,    16,
      60,    61,     0,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,     0,     0,     0,     0,     0,     0,
     117,     0,     0,     0,    16,     0,    74,   115,    60,    61,
       0,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,     0,     0,     0,    73,     0,     0,     0,     0,
       0,     0,    60,    61,    74,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,     0,     0,     0,    76,
       0,     0,     0,     0,     0,     0,    60,    61,    74,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    82,    74,    60,    61,     0,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,     0,     0,     0,
      89,     0,     0,     0,     0,     0,     0,    60,    61,    74,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   116,    74,    60,    61,     0,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   121,
      74,    60,    61,     0,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,     0,     0,     0,   123,     0,
       0,     0,     0,     0,     0,    60,    61,    74,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      61,    74,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,     0,    74,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    32,    74,     0,    33,     0,
       0,     0,    34,     0,     0,     0,     0,    74,     0,    35,
      36,    68,    69,    70,    71,    72,    37,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    74
};

static const yytype_int8 yycheck[] =
{
      19,    28,    21,    49,    23,    23,     7,    10,    11,     0,
      13,    26,    27,    28,    17,    34,    35,    36,    37,     4,
      34,    24,    25,    31,    38,    31,    41,    73,    31,    33,
      76,    39,    40,    10,    53,    53,    40,    32,    32,    34,
      34,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    40,    74,    75,    10,    11,    77,
      10,    11,    80,    31,    31,    10,    10,    10,   114,     5,
      39,    33,     8,     9,    10,     5,    12,   123,     8,     9,
      10,    40,    12,    10,     5,    33,   113,     8,     9,    10,
      41,    12,     6,    35,    40,    31,   115,   116,    88,    35,
      54,    37,   121,    79,    40,    35,    -1,    37,    -1,     5,
      40,    -1,     8,     9,    10,    36,    12,    -1,    -1,    40,
      15,    16,    -1,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    -1,    -1,    -1,    -1,    -1,    -1,
      36,    -1,    -1,    -1,    40,    -1,    41,    42,    15,    16,
      -1,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,
      -1,    -1,    15,    16,    41,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    -1,    -1,    -1,    32,
      -1,    -1,    -1,    -1,    -1,    -1,    15,    16,    41,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    40,    41,    15,    16,    -1,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    -1,    -1,    -1,
      32,    -1,    -1,    -1,    -1,    -1,    -1,    15,    16,    41,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    40,    41,    15,    16,    -1,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    40,
      41,    15,    16,    -1,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    -1,    -1,    -1,    32,    -1,
      -1,    -1,    -1,    -1,    -1,    15,    16,    41,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      16,    41,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    -1,    41,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    10,    41,    -1,    13,    -1,
      -1,    -1,    17,    -1,    -1,    -1,    -1,    41,    -1,    24,
      25,    24,    25,    26,    27,    28,    31,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     7,    44,    45,     0,     4,    46,    10,     5,     8,
       9,    10,    12,    31,    35,    37,    40,    48,    52,    31,
      31,    31,    31,    39,    40,    40,    10,    47,    49,    10,
      50,    51,    10,    13,    17,    24,    25,    31,    56,    10,
      56,    10,    11,    53,    54,    55,    56,    55,    56,    32,
      34,    36,    52,    39,    34,    38,    56,    56,    56,    56,
      15,    16,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    32,    41,    40,    32,    33,    32,    34,
      33,    40,    40,    48,    10,    55,    56,    51,    35,    32,
      56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
      56,    56,    56,    48,    56,    56,    48,    10,    55,    40,
      54,    10,    55,    49,     6,    42,    40,    36,    48,    56,
      56,    40,    56,    32,    48
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (&yylloc, stream, result, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, &yylloc, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, &yylloc, stream)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location, stream, result); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, std::wstringstream &stream, NONS_Macro::MacroFile *&result)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, stream, result)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    std::wstringstream &stream;
    NONS_Macro::MacroFile *&result;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
  YYUSE (stream);
  YYUSE (result);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, std::wstringstream &stream, NONS_Macro::MacroFile *&result)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp, stream, result)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    std::wstringstream &stream;
    NONS_Macro::MacroFile *&result;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, stream, result);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, std::wstringstream &stream, NONS_Macro::MacroFile *&result)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule, stream, result)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
    std::wstringstream &stream;
    NONS_Macro::MacroFile *&result;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       , stream, result);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule, stream, result); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, std::wstringstream &stream, NONS_Macro::MacroFile *&result)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp, stream, result)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
    std::wstringstream &stream;
    NONS_Macro::MacroFile *&result;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (stream);
  YYUSE (result);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {
      case 10: /* "IDENTIFIER" */

	{
	delete (std::wstring *)(yyvaluep->id);
};

	break;
      case 11: /* "STRING" */

	{
	delete (std::wstring *)(yyvaluep->str);
};

	break;
      case 12: /* "CODE_BLOCK" */

	{
	delete (std::wstring *)(yyvaluep->str);
};

	break;
      case 45: /* "file" */

	{
	delete (yyvaluep->macro_file);
};

	break;
      case 46: /* "macro_definition" */

	{
	delete (yyvaluep->symbol);
};

	break;
      case 47: /* "parameter_list" */

	{
	delete (yyvaluep->symbol_table);
};

	break;
      case 48: /* "block" */

	{
	delete (yyvaluep->block);
};

	break;
      case 49: /* "stmt_list" */

	{
	for (ulong a=0;a<(yyvaluep->stmt_list)->size();a++)
		delete (*(yyvaluep->stmt_list))[a];
	delete (yyvaluep->stmt_list);
};

	break;
      case 50: /* "initialization_list" */

	{
	delete (yyvaluep->symbol_table);
};

	break;
      case 51: /* "initialization" */

	{
	delete (yyvaluep->symbol);
};

	break;
      case 52: /* "stmt" */

	{
	delete (yyvaluep->stmt);
};

	break;
      case 53: /* "argument_list" */

	{
	for (ulong a=0;a<(yyvaluep->argumentVector)->size();a++)
		delete (*(yyvaluep->argumentVector))[a];
	delete (yyvaluep->argumentVector);
};

	break;
      case 54: /* "argument" */

	{
	delete (yyvaluep->argument);
};

	break;
      case 55: /* "string" */

	{
	delete (yyvaluep->string);
};

	break;
      case 56: /* "expr" */

	{
	delete (yyvaluep->expression);
};

	break;

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (std::wstringstream &stream, NONS_Macro::MacroFile *&result);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */





/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (std::wstringstream &stream, NONS_Macro::MacroFile *&result)
#else
int
yyparse (stream, result)
    std::wstringstream &stream;
    NONS_Macro::MacroFile *&result;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Location data for the lookahead symbol.  */
YYLTYPE yylloc;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.
       `yyls': related to locations.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[2];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;

#if YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 1;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);

	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
	YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

    {
		result=(yyvsp[(1) - (1)].macro_file);
	}
    break;

  case 3:

    {
		result=0;
		YYABORT;
	}
    break;

  case 4:

    {
		(yyval.macro_file)=new NONS_Macro::MacroFile;
	}
    break;

  case 5:

    {
		(yyval.macro_file)=(yyvsp[(1) - (2)].macro_file);
		(yyval.macro_file)->symbol_table.declare((yyvsp[(2) - (2)].symbol),0);
	}
    break;

  case 6:

    {
		(yyval.symbol)=new NONS_Macro::Symbol((yyvsp[(2) - (3)].id)->id,new NONS_Macro::Macro((yyvsp[(3) - (3)].block)),(yylsp[(2) - (3)]).first_line);
		delete (yyvsp[(2) - (3)].id);
	}
    break;

  case 7:

    {
		(yyval.symbol)=new NONS_Macro::Symbol((yyvsp[(2) - (6)].id)->id,new NONS_Macro::Macro((yyvsp[(6) - (6)].block),(yyvsp[(4) - (6)].symbol_table)),(yylsp[(2) - (6)]).first_line);
		delete (yyvsp[(2) - (6)].id);
	}
    break;

  case 8:

    {
		(yyval.symbol_table)=new NONS_Macro::SymbolTable;
		(yyval.symbol_table)->declare((yyvsp[(1) - (1)].id)->id,L"",(yylsp[(1) - (1)]).first_line,0);
		delete (yyvsp[(1) - (1)].id);
	}
    break;

  case 9:

    {
		(yyval.symbol_table)=(yyvsp[(1) - (3)].symbol_table);
		(yyval.symbol_table)->declare((yyvsp[(3) - (3)].id)->id,L"",(yylsp[(3) - (3)]).first_line,0);
		delete (yyvsp[(3) - (3)].id);
	}
    break;

  case 10:

    {
		(yyval.block)=new NONS_Macro::Block;
		(yyval.block)->addStatement((yyvsp[(1) - (1)].stmt));
	}
    break;

  case 11:

    {
		(yyval.block)=new NONS_Macro::Block((yyvsp[(2) - (3)].stmt_list));
	}
    break;

  case 12:

    {
		(yyval.block)=new NONS_Macro::Block((yyvsp[(5) - (6)].stmt_list),(yyvsp[(2) - (6)].symbol_table));
	}
    break;

  case 13:

    {
		(yyval.stmt_list)=new std::vector<NONS_Macro::Statement *>;
	}
    break;

  case 14:

    {
		(yyval.stmt_list)=(yyvsp[(1) - (2)].stmt_list);
		(yyval.stmt_list)->push_back((yyvsp[(2) - (2)].stmt));
	}
    break;

  case 15:

    {
		(yyval.symbol_table)=new NONS_Macro::SymbolTable;
		(yyval.symbol_table)->declare((yyvsp[(1) - (1)].symbol));
	}
    break;

  case 16:

    {
		(yyval.symbol_table)=(yyvsp[(1) - (3)].symbol_table);
		(yyval.symbol_table)->declare((yyvsp[(3) - (3)].symbol),0);
	}
    break;

  case 17:

    {
		(yyval.symbol)=new NONS_Macro::Symbol((yyvsp[(1) - (1)].id)->id,(long)0,(yylsp[(1) - (1)]).first_line);
	}
    break;

  case 18:

    {
		(yyval.symbol)=new NONS_Macro::Symbol((yyvsp[(1) - (3)].id)->id,(long)0,(yylsp[(1) - (3)]).first_line);
		(yyvsp[(3) - (3)].expression)->simplify();
		(yyval.symbol)->initializeTo((yyvsp[(3) - (3)].expression));
	}
    break;

  case 19:

    {
		(yyval.symbol)=new NONS_Macro::Symbol((yyvsp[(1) - (3)].id)->id,L"",(yylsp[(1) - (3)]).first_line);
		(yyvsp[(3) - (3)].string)->simplify();
		(yyval.symbol)->initializeTo((yyvsp[(3) - (3)].string));
	}
    break;

  case 20:

    {
		(yyval.stmt)=new NONS_Macro::EmptyStatement;
	}
    break;

  case 21:

    {
		(yyval.stmt)=new NONS_Macro::DataBlock(*(yyvsp[(1) - (2)].str));
		delete (yyvsp[(1) - (2)].str);
	}
    break;

  case 22:

    {
		(yyvsp[(3) - (4)].expression)->simplify();
		(yyval.stmt)=new NONS_Macro::AssignmentStatement(*(yyvsp[(1) - (4)].id),(yyvsp[(3) - (4)].expression));
	}
    break;

  case 23:

    {
		(yyvsp[(3) - (4)].string)->simplify();
		(yyval.stmt)=new NONS_Macro::StringAssignmentStatement(*(yyvsp[(1) - (4)].id),(yyvsp[(3) - (4)].string));
	}
    break;

  case 24:

    {
		(yyvsp[(3) - (5)].expression)->simplify();
		(yyval.stmt)=new NONS_Macro::IfStructure((yyvsp[(3) - (5)].expression),(yyvsp[(5) - (5)].block));
	}
    break;

  case 25:

    {
		(yyvsp[(3) - (7)].expression)->simplify();
		(yyval.stmt)=new NONS_Macro::IfStructure((yyvsp[(3) - (7)].expression),(yyvsp[(5) - (7)].block),(yyvsp[(7) - (7)].block));
	}
    break;

  case 26:

    {
		(yyvsp[(3) - (5)].expression)->simplify();
		(yyval.stmt)=new NONS_Macro::WhileStructure((yyvsp[(3) - (5)].expression),(yyvsp[(5) - (5)].block));
	}
    break;

  case 27:

    {
		(yyvsp[(5) - (11)].expression)->simplify();
		(yyvsp[(7) - (11)].expression)->simplify();
		(yyvsp[(9) - (11)].expression)->simplify();
		(yyval.stmt)=new NONS_Macro::ForStructure(*(yyvsp[(3) - (11)].id),(yyvsp[(5) - (11)].expression),(yyvsp[(7) - (11)].expression),(yyvsp[(9) - (11)].expression),(yyvsp[(11) - (11)].block));
		delete (yyvsp[(3) - (11)].id);
	}
    break;

  case 28:

    {
		(yyval.stmt)=new NONS_Macro::MacroCall(*(yyvsp[(1) - (2)].id));
		delete (yyvsp[(1) - (2)].id);
	}
    break;

  case 29:

    {
		(yyval.stmt)=new NONS_Macro::MacroCall(*(yyvsp[(1) - (5)].id),(yyvsp[(3) - (5)].argumentVector));
		delete (yyvsp[(1) - (5)].id);
	}
    break;

  case 30:

    {
		(yyval.argumentVector)=new std::vector<NONS_Macro::Argument *>;
		(yyval.argumentVector)->push_back((yyvsp[(1) - (1)].argument));
	}
    break;

  case 31:

    {
		(yyval.argumentVector)=(yyvsp[(1) - (3)].argumentVector);
		(yyval.argumentVector)->push_back((yyvsp[(3) - (3)].argument));
	}
    break;

  case 32:

    {
		(yyvsp[(1) - (1)].string)->simplify();
		(yyval.argument)=(yyvsp[(1) - (1)].string);
	}
    break;

  case 33:

    {
		(yyvsp[(1) - (1)].expression)->simplify();
		(yyval.argument)=(yyvsp[(1) - (1)].expression);
	}
    break;

  case 34:

    {
		(yyval.string)=new NONS_Macro::ConstantString(*(yyvsp[(1) - (1)].str));
		delete (yyvsp[(1) - (1)].str);
	}
    break;

  case 35:

    {
		(yyval.string)=new NONS_Macro::StringConcatenation(new NONS_Macro::VariableString(*(yyvsp[(1) - (3)].id)),new NONS_Macro::VariableString(*(yyvsp[(3) - (3)].id)));
		delete (yyvsp[(1) - (3)].id);
		delete (yyvsp[(3) - (3)].id);
	}
    break;

  case 36:

    {
		(yyval.string)=new NONS_Macro::StringConcatenation(new NONS_Macro::VariableString(*(yyvsp[(1) - (3)].id)),(yyvsp[(3) - (3)].string));
		delete (yyvsp[(1) - (3)].id);
	}
    break;

  case 37:

    {
		(yyval.string)=new NONS_Macro::StringConcatenation((yyvsp[(1) - (3)].string),new NONS_Macro::VariableString(*(yyvsp[(3) - (3)].id)));
		delete (yyvsp[(3) - (3)].id);
	}
    break;

  case 38:

    {
		(yyval.string)=new NONS_Macro::StringConcatenation((yyvsp[(1) - (3)].string),(yyvsp[(3) - (3)].string));
	}
    break;

  case 39:

    {
		(yyval.expression)=(yyvsp[(1) - (1)].expression);
	}
    break;

  case 40:

    {
		(yyval.expression)=new NONS_Macro::VariableExpression(*(yyvsp[(1) - (1)].id));
		delete (yyvsp[(1) - (1)].id);
	}
    break;

  case 41:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(TRINARY,(yyvsp[(1) - (5)].expression),(yyvsp[(3) - (5)].expression),(yyvsp[(5) - (5)].expression));
	}
    break;

  case 42:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(BOR,(yyvsp[(1) - (3)].expression),(yyvsp[(3) - (3)].expression));
	}
    break;

  case 43:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(BAND,(yyvsp[(1) - (3)].expression),(yyvsp[(3) - (3)].expression));
	}
    break;

  case 44:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(BNOT,(yyvsp[(2) - (2)].expression));
	}
    break;

  case 45:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(EQUALS,(yyvsp[(1) - (3)].expression),(yyvsp[(3) - (3)].expression));
	}
    break;

  case 46:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(NOT_EQUALS,(yyvsp[(1) - (3)].expression),(yyvsp[(3) - (3)].expression));
	}
    break;

  case 47:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(GREATER_THAN,(yyvsp[(1) - (3)].expression),(yyvsp[(3) - (3)].expression));
	}
    break;

  case 48:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(LOWER_THAN,(yyvsp[(1) - (3)].expression),(yyvsp[(3) - (3)].expression));
	}
    break;

  case 49:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(GT_EQUALS,(yyvsp[(1) - (3)].expression),(yyvsp[(3) - (3)].expression));
	}
    break;

  case 50:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(LT_EQUALS,(yyvsp[(1) - (3)].expression),(yyvsp[(3) - (3)].expression));
	}
    break;

  case 51:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(PLUS,(yyvsp[(1) - (3)].expression),(yyvsp[(3) - (3)].expression));
	}
    break;

  case 52:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(MINUS,(yyvsp[(1) - (3)].expression),(yyvsp[(3) - (3)].expression));
	}
    break;

  case 53:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(MUL,(yyvsp[(1) - (3)].expression),(yyvsp[(3) - (3)].expression));
	}
    break;

  case 54:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(DIV,(yyvsp[(1) - (3)].expression),(yyvsp[(3) - (3)].expression));
	}
    break;

  case 55:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(MOD,(yyvsp[(1) - (3)].expression),(yyvsp[(3) - (3)].expression));
	}
    break;

  case 56:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(MOD ,(yyvsp[(2) - (2)].expression));
	}
    break;

  case 57:

    {
		(yyval.expression)=new NONS_Macro::FullExpression(MINUS,(yyvsp[(2) - (2)].expression));
	}
    break;

  case 58:

    {
		(yyval.expression)=(yyvsp[(2) - (3)].expression);
	}
    break;



      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, stream, result, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (&yylloc, stream, result, yymsg);
	  }
	else
	  {
	    yyerror (&yylloc, stream, result, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc, stream, result);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp, stream, result);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, stream, result, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc, stream, result);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp, stream, result);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}





int macroParser_yylex(YYSTYPE *yylval,YYLTYPE *yylloc,std::wstringstream &stream){
macroParser_yylex_begin:
	wchar_t c;
	while (!stream.eof() && iswhitespace((wchar_t)stream.peek())){
		c=stream.get();
		if (c==10){
			yylloc->first_line++;
		}else if (c==13){
			c=stream.peek();
			if (c==10)
				stream.get();
			yylloc->first_line++;
		}
	}
	if (stream.eof())
		return 0;
	c=stream.peek();
	if (NONS_isdigit(c)){
		std::wstring temp;
		while (NONS_isdigit(c=stream.peek()))
			temp.push_back(stream.get());
		yylval->expression=new NONS_Macro::ConstantExpression(atoi(temp));
		return INTEGER;
	}
	if (c=='\"'){
		stream.get();
		std::wstring temp;
		while ((wchar_t)stream.peek()!=c && !stream.eof()){
			wchar_t character=stream.get();
			if (character=='\\'){
				character=stream.get();
				switch (character){
					case '\\':
					case '\"':
						temp.push_back(character);
						break;
					case 'n':
					case 'r':
						temp.push_back('\n');
						break;
					case 'x':
						{
							std::wstring temp2;
							for (ulong a=0;NONS_ishexa(stream.peek()) && a<4;a++)
								temp2.push_back(stream.get());
							if (temp2.size()<4)
								return ERROR;
							wchar_t a=0;
							for (size_t b=0;b<temp2.size();b++)
								a=(a<<4)+HEX2DEC(temp2[b]);
							temp.push_back(a?a:32);
						}
						break;
					default:
						return ERROR;
				}
			}else
				temp.push_back(character);
		}
		if ((wchar_t)stream.peek()!='\"')
			//handleErrors(NONS_UNMATCHED_QUOTES,0,"yylex",1);
			return ERROR;
		else
			stream.get();
		yylval->str=new std::wstring(temp);
		return STRING;
	}
	if (NONS_isid1char(c) || c=='^'){
		std::wstring temp;
		temp.push_back(stream.get());
		while (NONS_isidnchar(stream.peek()))
			temp.push_back(stream.get());
		if (temp==L"block" && stream.peek()=='{'){
			stream.get();
			std::wstring data,
				String=L"}block";
			while (!stream.eof()){
				c=stream.get();
				data.push_back(c);
				if (data.size()>=6 && partialCompare(data,data.size()-6,String))
					break;
			}
			data.resize(data.size()-6);
			yylval->str=new std::wstring(data);
			return CODE_BLOCK;
		}
		if (temp==L"define")
			return DEFINE;
		if (temp==L"if")
			return IF;
		if (temp==L"else")
			return ELSE;
		if (temp==L"for")
			return FOR;
		if (temp==L"while")
			return WHILE;
		yylval->id=new NONS_Macro::Identifier(temp,yylloc->first_line);
		return IDENTIFIER;
	}
	stream.get();
	switch (c){
		case '\'':
			return APOSTROPHE;
		case '+':
			return PLUS;
		case '-':
			return MINUS;
		case '*':
			return MUL;
		case '/':
			switch (stream.peek()){
				case '*':
					stream.get();
					while (!stream.eof()){
						c=stream.get();
						if (c=='*' && stream.peek()=='/'){
							stream.get();
							goto macroParser_yylex_begin;
						}
						if (c==10){
							yylloc->first_line++;
						}else if (c==13){
							c=stream.peek();
							if (c==10)
								stream.get();
							yylloc->first_line++;
						}
					}
					return 0;
				case '/':
					stream.get();
					while (!stream.eof()){
						c=stream.peek();
						if (c==10 || c==13)
							goto macroParser_yylex_begin;
						stream.get();
					}
					return 0;
				default:
					return DIV;
			}
		case '%':
			return MOD;
		case '&':
			if (stream.peek()!='&')
				return c;
			stream.get();
			return BAND;
		case '|':
			if (stream.peek()!='|')
				return ERROR;
			stream.get();
			return BOR;
		case '!':
			if (stream.peek()=='='){
				stream.get();
				return NOT_EQUALS;
			}
			return BNOT;
		case '<':
			if (stream.peek()=='='){
				stream.get();
				return LT_EQUALS;
			}
			return LOWER_THAN;
		case '>':
			if (stream.peek()=='='){
				stream.get();
				return GT_EQUALS;
			}
			return GREATER_THAN;
		case '=':
			if (stream.peek()!='=')
				return c;
			stream.get();
			return EQUALS;
		default:
			return c;
	}
}

void macroParser_yyerror(YYLTYPE *yylloc,std::wstringstream &,NONS_Macro::MacroFile *&/*,NONS_Macro::SymbolTable &*/,char const *s){
	/*if (!retrievedVar)
		handleErrors(NONS_UNDEFINED_ERROR,0,"yyparse",1,UniFromISO88591(s));*/
}

