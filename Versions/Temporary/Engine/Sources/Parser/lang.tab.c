
/*  A Bison parser, made from lang.y with Bison version GNU Bison version 1.24
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	CLASS	258
#define	IDENTIFIER	259
#define	DOUBLEDOT	260
#define	NUMBER	261
#define	TK_TRUE	262
#define	TK_FALSE	263
#define	UNBOUNDED	264
#define	STRING	265
#define	WSTRING	266
#define	TYPEDEF	267
#define	ENUM	268
#define	ATTRIBUTE	269
#define	STRUCT	270
#define	VIS_TYPE	271
#define	INCLUDE	272
#define	BASE_CLASS	273
#define	BASE_STRUCT	274
#define	FORWARD	275
#define	TYPE_BOOL	276
#define	TYPE_INT	277
#define	TYPE_FLOAT	278
#define	TYPE_WORD	279
#define	TYPE_DWORD	280
#define	TYPE_STRING	281
#define	TYPE_HEX	282
#define	DOUBLE_DOT	283
#define	COMMENT	284
#define	H_EXTERNAL	285
#define	CPP_EXTERNAL	286
#define	SRND_INSTANCE	287
#define	DEFAULT_VALUE	288

#line 1 "lang.y"


#define YYDEBUG 314159
#define YYERROR_VERBOSE 1

#include "stdafx.h"
#include <malloc.h>
#include <cstdarg>
#include "LangToken.h"
#include "lang.tab.h"
#include "ParseOperations.h"
#include "FileRead.h"
#include "FileNode.h"

extern int nyyLineNumber;
extern bool byySuccess;
extern bool bInTestMode;
extern bool bNoTrace;

void yyerror ( char *s, ... )  /* Called by yyparse on error */
{
	if ( !bInTestMode )
	{
		static char buff[10000];
		va_list va;
		va_start( va, s );
		vsprintf( buff, s, va );
		va_end( va );
		printf( "%s(%d) error: %s\n", NLang::GetParsingFileName(), nyyLineNumber, buff );
		if ( !bNoTrace )
			DbgTrc( "%s(%d) error: %s\n", NLang::GetParsingFileName(), nyyLineNumber, buff );
	}
	
	byySuccess = false;
}

void yyerror_no_line( char *s, ... )  /* Called on error */
{
	if ( !bInTestMode )
	{
		static char buff[10000];
		va_list va;
		va_start( va, s );
		vsprintf( buff, s, va );
		va_end( va );
		printf( "error %s\n", buff );
		if ( !bNoTrace )
			DbgTrc( "error %s\n", buff );
	}

	byySuccess = false;
}

int yylex();


#ifndef YYLTYPE
typedef
  struct yyltype
    {
      int timestamp;
      int first_line;
      int first_column;
      int last_line;
      int last_column;
      char *text;
   }
  yyltype;

#define YYLTYPE yyltype
#endif

#ifndef YYSTYPE
#define YYSTYPE int
#endif
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		172
#define	YYFLAG		-32768
#define	YYNTBASE	47

#define YYTRANSLATE(x) ((unsigned)(x) <= 288 ? yytranslate[x] : 80)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,    45,
    46,    41,     2,    40,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    35,    34,    38,
    42,    39,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
    43,     2,    44,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    36,     2,    37,     2,     2,     2,     2,     2,
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
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    28,    29,    30,    31,    32,    33
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     1,     3,     5,     8,    10,    13,    16,    20,    24,
    26,    29,    31,    33,    35,    37,    39,    41,    46,    51,
    56,    61,    63,    67,    69,    73,    75,    78,    83,    84,
    87,    90,    94,    98,   100,   102,   104,   106,   111,   118,
   123,   128,   130,   132,   136,   140,   143,   148,   150,   154,
   158,   162,   166,   170,   174,   178,   180,   184,   189,   195,
   202,   207,   208,   211,   215,   220,   222,   224,   228,   230,
   234,   238,   242,   246,   252,   259,   264,   272,   274,   277,
   281,   283,   287,   291,   295,   299,   301,   303,   305,   307,
   309,   311,   313,   315,   322,   324,   326,   327,   329,   331,
   333,   335,   337,   339
};

static const short yyrhs[] = {    -1,
    48,     0,    50,     0,    48,    50,     0,    49,     0,    48,
    49,     0,    17,    10,     0,    30,    17,    10,     0,    31,
    17,    10,     0,    51,     0,    50,    51,     0,    52,     0,
    53,     0,    72,     0,    71,     0,    77,     0,    75,     0,
    20,     3,     4,    34,     0,    67,     3,    54,    58,     0,
    20,    15,     4,    34,     0,    67,    15,    55,    58,     0,
     4,     0,     4,    35,    56,     0,     4,     0,     4,    35,
    56,     0,    57,     0,    16,     4,     0,    36,    59,    37,
    34,     0,     0,    59,    60,     0,    59,    62,     0,    59,
    16,    35,     0,    59,    17,    10,     0,    52,     0,    53,
     0,    72,     0,    71,     0,    32,    38,     4,    39,     0,
    32,    38,     4,    40,    76,    39,     0,    67,    76,    63,
    34,     0,    67,    61,    63,    34,     0,    65,     0,    64,
     0,    63,    40,    65,     0,    63,    40,    64,     0,    41,
    66,     0,    41,    66,    42,    33,     0,    66,     0,    66,
    42,     6,     0,    66,    42,    10,     0,    66,    42,    11,
     0,    66,    42,     7,     0,    66,    42,     8,     0,    66,
    42,     4,     0,    66,    42,    33,     0,     4,     0,     4,
    43,    44,     0,     4,    43,     6,    44,     0,     4,    43,
     6,    28,    44,     0,     4,    43,     6,    28,     6,    44,
     0,     4,    43,    28,    44,     0,     0,    67,    68,     0,
    43,    69,    44,     0,    43,    69,    34,    44,     0,    29,
     0,    70,     0,    69,    34,    70,     0,    78,     0,    78,
    42,     6,     0,    78,    42,    10,     0,    78,    42,     7,
     0,    78,    42,     8,     0,    67,    12,    76,    76,    34,
     0,    67,    12,     4,    41,    76,    34,     0,    20,    13,
     4,    34,     0,    67,    13,     4,    36,    73,    37,    34,
     0,    74,     0,    73,    40,     0,    73,    40,    74,     0,
     4,     0,     4,    42,     6,     0,     4,    42,     4,     0,
    19,    76,    34,     0,    18,    76,    34,     0,     4,     0,
    21,     0,    22,     0,    23,     0,    24,     0,    25,     0,
    26,     0,    27,     0,    14,    78,    45,    79,    46,    34,
     0,     4,     0,    16,     0,     0,    21,     0,    22,     0,
    23,     0,    24,     0,    25,     0,    26,     0,    27,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
    91,    92,    93,    94,    97,    98,   101,   108,   115,   124,
   125,   128,   129,   130,   131,   132,   133,   136,   137,   148,
   149,   160,   167,   177,   184,   194,   197,   200,   203,   204,
   211,   218,   219,   228,   229,   230,   231,   234,   235,   237,
   245,   255,   256,   257,   264,   273,   280,   290,   291,   298,
   305,   312,   319,   326,   333,   342,   343,   344,   345,   346,
   347,   350,   351,   365,   366,   367,   376,   377,   386,   387,
   388,   389,   390,   394,   395,   398,   402,   412,   413,   414,
   423,   424,   425,   428,   429,   432,   433,   434,   435,   436,
   437,   438,   439,   442,   451,   452,   455,   456,   457,   458,
   459,   460,   461,   462
};

static const char * const yytname[] = {   "$","error","$undefined.","CLASS",
"IDENTIFIER","DOUBLEDOT","NUMBER","TK_TRUE","TK_FALSE","UNBOUNDED","STRING",
"WSTRING","TYPEDEF","ENUM","ATTRIBUTE","STRUCT","VIS_TYPE","INCLUDE","BASE_CLASS",
"BASE_STRUCT","FORWARD","TYPE_BOOL","TYPE_INT","TYPE_FLOAT","TYPE_WORD","TYPE_DWORD",
"TYPE_STRING","TYPE_HEX","DOUBLE_DOT","COMMENT","H_EXTERNAL","CPP_EXTERNAL",
"SRND_INSTANCE","DEFAULT_VALUE","';'","':'","'{'","'}'","'<'","'>'","','","'*'",
"'='","'['","']'","'('","')'","input","include_section","include","file_namespace",
"type_definition","class","struct","class_header","struct_header","parents_list",
"parent_declaration","type_namespace","namespace_defs","inside_type","rnd_instance",
"field","varlist","pointer","simple_var","var","attrlist","attr_section","attributes_def",
"attribute_def","typedef","enum","enum_entries","enum_entry","newtype","new_type_id",
"attribute","attr_identifier","attr_type",""
};
#endif

static const short yyr1[] = {     0,
    47,    47,    47,    47,    48,    48,    49,    49,    49,    50,
    50,    51,    51,    51,    51,    51,    51,    52,    52,    53,
    53,    54,    54,    55,    55,    56,    57,    58,    59,    59,
    59,    59,    59,    60,    60,    60,    60,    61,    61,    62,
    62,    63,    63,    63,    63,    64,    64,    65,    65,    65,
    65,    65,    65,    65,    65,    66,    66,    66,    66,    66,
    66,    67,    67,    68,    68,    68,    69,    69,    70,    70,
    70,    70,    70,    71,    71,    72,    72,    73,    73,    73,
    74,    74,    74,    75,    75,    76,    76,    76,    76,    76,
    76,    76,    76,    77,    78,    78,    79,    79,    79,    79,
    79,    79,    79,    79
};

static const short yyr2[] = {     0,
     0,     1,     1,     2,     1,     2,     2,     3,     3,     1,
     2,     1,     1,     1,     1,     1,     1,     4,     4,     4,
     4,     1,     3,     1,     3,     1,     2,     4,     0,     2,
     2,     3,     3,     1,     1,     1,     1,     4,     6,     4,
     4,     1,     1,     3,     3,     2,     4,     1,     3,     3,
     3,     3,     3,     3,     3,     1,     3,     4,     5,     6,
     4,     0,     2,     3,     4,     1,     1,     3,     1,     3,
     3,     3,     3,     5,     6,     4,     7,     1,     2,     3,
     1,     3,     3,     3,     3,     1,     1,     1,     1,     1,
     1,     1,     1,     6,     1,     1,     0,     1,     1,     1,
     1,     1,     1,     1
};

static const short yydefact[] = {    62,
     0,     0,     0,     0,     0,     0,     0,    62,     5,    62,
    10,    12,    13,     0,    15,    14,    17,    16,    95,    96,
     0,     7,    86,    87,    88,    89,    90,    91,    92,    93,
     0,     0,     0,     0,     0,     0,     0,     6,    62,    11,
     0,     0,     0,     0,    66,     0,    63,    97,    85,    84,
     0,     0,     0,     8,     9,    22,     0,    86,     0,     0,
    24,     0,     0,    67,    69,    98,    99,   100,   101,   102,
   103,   104,     0,    18,    76,    20,     0,    29,    19,     0,
     0,     0,     0,    21,     0,    64,     0,     0,     0,    23,
    26,    62,     0,    74,    81,     0,    78,    25,    65,    68,
    70,    72,    73,    71,    94,    27,     0,     0,     0,    34,
    35,    30,    31,     0,    37,    36,    75,     0,     0,    79,
    32,    33,    28,     0,     0,     0,    83,    82,    77,    80,
     0,    56,     0,     0,    43,    42,    48,     0,     0,     0,
    46,    41,     0,     0,    40,    38,     0,     0,     0,    57,
     0,    45,    44,    54,    49,    52,    53,    50,    51,    55,
     0,     0,    58,    61,    47,    39,     0,    59,    60,     0,
     0,     0
};

static const short yydefgoto[] = {   170,
     8,     9,    10,    11,    12,    13,    57,    62,    90,    91,
    79,    92,   112,   125,   113,   134,   135,   136,   137,    14,
    47,    63,    64,    15,    16,    96,    97,    17,    31,    18,
    65,    73
};

static const short yypact[] = {    39,
    38,    16,    67,    67,    60,   -11,    20,    48,-32768,    97,
-32768,-32768,-32768,     2,-32768,-32768,-32768,-32768,-32768,-32768,
   -12,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
     9,    18,    42,    81,   104,    50,   108,-32768,   105,-32768,
   110,    77,   116,   117,-32768,    38,-32768,   106,-32768,-32768,
    43,    88,    92,-32768,-32768,   102,   103,    99,    67,   109,
   107,   103,   -26,-32768,   111,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,    95,-32768,-32768,-32768,   130,-32768,-32768,    67,
   113,   140,   130,-32768,     0,-32768,   128,   114,   145,-32768,
-32768,    12,   118,-32768,   112,    24,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,   115,   141,   121,-32768,
-32768,-32768,-32768,    -2,-32768,-32768,-32768,    68,   122,   140,
-32768,-32768,-32768,   119,    -1,    -1,-32768,-32768,-32768,-32768,
   154,   120,   155,    72,-32768,-32768,   123,    73,    56,     6,
   124,-32768,    -1,    76,-32768,-32768,    67,     7,   125,-32768,
   127,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
   129,     3,-32768,-32768,-32768,-32768,   126,-32768,-32768,   161,
   162,-32768
};

static const short yypgoto[] = {-32768,
-32768,   156,   159,    -3,    79,    80,-32768,-32768,    90,-32768,
   131,-32768,-32768,-32768,-32768,    49,    31,    33,    44,    86,
-32768,-32768,    94,    89,    91,-32768,    62,-32768,    -4,-32768,
   179,-32768
};


#define	YYLAST		193


static const short yytable[] = {    32,
    41,    23,   132,    19,    41,    36,    40,    85,   167,    42,
    43,   148,    44,    42,    43,    20,    44,    86,    24,    25,
    26,    27,    28,    29,    30,    22,    45,   107,   108,   124,
    45,     5,    48,   149,   162,    40,    37,    59,    -1,   133,
    46,    19,    49,    99,    46,    51,   168,    -2,   109,   150,
   163,    50,     1,    20,    81,     2,     3,     4,     5,    54,
   119,     1,    33,   120,     2,     3,     4,     5,     6,     7,
    23,   127,    34,   128,    35,    93,    74,     6,     7,   154,
    58,   155,   156,   157,    52,   158,   159,    24,    25,    26,
    27,    28,    29,    30,   146,   147,    -3,    24,    25,    26,
    27,    28,    29,    30,    -4,   142,   145,    53,   160,   126,
     1,   143,   143,    56,     3,     4,     5,    55,     1,    60,
    61,    75,     3,     4,     5,    76,    66,    67,    68,    69,
    70,    71,    72,   101,   102,   103,    77,   104,    78,    80,
    88,    83,   161,    95,    82,    89,    94,   105,   106,   121,
   122,   117,    87,   118,   123,   129,   131,   139,   132,   165,
   171,   172,   140,    38,   144,   151,    39,   166,   164,   169,
   110,   111,    98,   152,   138,   153,   141,   114,   100,    21,
   115,   130,   116,     0,     0,     0,     0,     0,     0,     0,
     0,     0,    84
};

static const short yycheck[] = {     4,
     3,     4,     4,     4,     3,    17,    10,    34,     6,    12,
    13,     6,    15,    12,    13,    16,    15,    44,    21,    22,
    23,    24,    25,    26,    27,    10,    29,    16,    17,    32,
    29,    20,    45,    28,    28,    39,    17,    42,     0,    41,
    43,     4,    34,    44,    43,     4,    44,     0,    37,    44,
    44,    34,    14,    16,    59,    17,    18,    19,    20,    10,
    37,    14,     3,    40,    17,    18,    19,    20,    30,    31,
     4,     4,    13,     6,    15,    80,    34,    30,    31,     4,
     4,     6,     7,     8,     4,    10,    11,    21,    22,    23,
    24,    25,    26,    27,    39,    40,     0,    21,    22,    23,
    24,    25,    26,    27,     0,    34,    34,     4,    33,   114,
    14,    40,    40,     4,    18,    19,    20,    10,    14,     4,
     4,    34,    18,    19,    20,    34,    21,    22,    23,    24,
    25,    26,    27,     6,     7,     8,    35,    10,    36,    41,
    46,    35,   147,     4,    36,    16,    34,    34,     4,    35,
    10,    34,    42,    42,    34,    34,    38,     4,     4,    33,
     0,     0,    43,     8,    42,    42,     8,    39,    44,    44,
    92,    92,    83,   143,   126,   143,   133,    92,    85,     1,
    92,   120,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    62
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(FROM,TO,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (from, to, count)
     char *from;
     char *to;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *from, char *to, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 192 "bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#else
#define YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#endif

int
yyparse(YYPARSE_PARAM)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 5:
#line 97 "lang.y"
{;
    break;}
case 6:
#line 98 "lang.y"
{;
    break;}
case 7:
#line 101 "lang.y"
{
																		if ( NLang::GetStep() == 0 )
																		{
																			yyval.psz = yyvsp[0].psz;														
																			NLang::AddInclude( yyval.psz );
																		}
																	;
    break;}
case 8:
#line 108 "lang.y"
{
																			if ( NLang::GetStep() == 1 )
																			{
																				yyval.psz = yyvsp[0].psz;
																				NLang::AddHExternal( yyvsp[0].psz );
																			}
																		;
    break;}
case 9:
#line 115 "lang.y"
{
																				if ( NLang::GetStep() == 1 )
																				{
																					yyval.psz = yyvsp[0].psz;
																					NLang::AddCPPExternal( yyvsp[0].psz );
																				}
																			;
    break;}
case 10:
#line 124 "lang.y"
{;
    break;}
case 11:
#line 125 "lang.y"
{;
    break;}
case 12:
#line 128 "lang.y"
{ if ( NLang::GetStep() == 1 ) NLang::AddDef( yyvsp[0].pNode ); ;
    break;}
case 13:
#line 129 "lang.y"
{ if ( NLang::GetStep() == 1 ) NLang::AddDef( yyvsp[0].pNode ); ;
    break;}
case 14:
#line 130 "lang.y"
{ if ( NLang::GetStep() == 1 ) NLang::AddDef( yyvsp[0].pNode ); ;
    break;}
case 15:
#line 131 "lang.y"
{ if ( NLang::GetStep() == 1 ) NLang::AddDef( yyvsp[0].pNode ); ;
    break;}
case 16:
#line 132 "lang.y"
{ if ( NLang::GetStep() == 1 ) NLang::AddDef( yyvsp[0].pNode ); ;
    break;}
case 17:
#line 133 "lang.y"
{ if ( NLang::GetStep() == 1 ) NLang::AddDef( yyvsp[0].pNode ); ;
    break;}
case 18:
#line 136 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateForwardComplexType( yyvsp[-1].psz, true ); ;
    break;}
case 19:
#line 137 "lang.y"
{
																												if ( NLang::GetStep() == 1 )
																												{
																													yyval.pNode = yyvsp[-1].pNode;
																													NLang::AddAttrToComplexTypeNode( yyval.pNode, yyvsp[-3].pNode );
																													NLang::AddNamespaceToComplexTypeNode( yyval.pNode, yyvsp[0].pNode );
																													NLang::CloseNamespace();
																												}
																											;
    break;}
case 20:
#line 148 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateForwardComplexType( yyvsp[-1].psz, false ); ;
    break;}
case 21:
#line 149 "lang.y"
{
																													if ( NLang::GetStep() == 1 )
																													{
																														yyval.pNode = yyvsp[-1].pNode;
																														NLang::AddAttrToComplexTypeNode( yyval.pNode, yyvsp[-3].pNode );
																														NLang::AddNamespaceToComplexTypeNode( yyval.pNode, yyvsp[0].pNode );
																														NLang::CloseNamespace();
																													}
																												;
    break;}
case 22:
#line 160 "lang.y"
{ 
																								if ( NLang::GetStep() == 1 )
																								{
																									yyval.pNode = NLang::CreateComplexTypeNode( yyvsp[0].psz, true );
																									NLang::OpenNewNamespace( 0 );
																								}
																							;
    break;}
case 23:
#line 167 "lang.y"
{
																								if ( NLang::GetStep() == 1 )
																								{
																									yyval.pNode = NLang::CreateComplexTypeNode( yyvsp[-2].psz, true );
																									NLang::AddParentsOfComplexType( yyval.pNode, yyvsp[0].pNode );
																									NLang::OpenNewNamespace( yyvsp[0].pNode );
																								}
																							;
    break;}
case 24:
#line 177 "lang.y"
{ 
																							if ( NLang::GetStep() == 1 )
																							{
																								yyval.pNode = NLang::CreateComplexTypeNode( yyvsp[0].psz, false );
																								NLang::OpenNewNamespace( 0 );
																							}
																						;
    break;}
case 25:
#line 184 "lang.y"
{
																							if ( NLang::GetStep() == 1 )
																							{
																								yyval.pNode = NLang::CreateComplexTypeNode( yyvsp[-2].psz, false );
																								NLang::AddParentsOfComplexType( yyval.pNode, yyvsp[0].pNode );
																								NLang::OpenNewNamespace( yyvsp[0].pNode );
																							}
																						;
    break;}
case 26:
#line 194 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateParentsList( yyvsp[0].psz ); ;
    break;}
case 27:
#line 197 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.psz = yyvsp[0].psz; ;
    break;}
case 28:
#line 200 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = yyvsp[-2].pNode; ;
    break;}
case 29:
#line 203 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::GetCurrentNamespace(); ;
    break;}
case 30:
#line 204 "lang.y"
{
																		if ( NLang::GetStep() == 1 )
																		{
																			yyval.pNode = yyvsp[-1].pNode;
																			NLang::AddTypeToNamespace( yyval.pNode, yyvsp[0].pNode );
																		}
																	;
    break;}
case 31:
#line 211 "lang.y"
{
																		if ( NLang::GetStep() == 1 )
																		{
																			yyval.pNode = yyvsp[-1].pNode;
																			NLang::AddVarListToNamespace( yyval.pNode, yyvsp[0].pNode );
																		}
																	;
    break;}
case 32:
#line 218 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = yyvsp[-2].pNode; ;
    break;}
case 33:
#line 219 "lang.y"
{ 
																					if ( NLang::GetStep() == 1 )
																					{
																						yyval.pNode = yyvsp[-2].pNode;
																						NLang::AddBadIncludeToNamespace( yyval.pNode, yyvsp[0].psz );
																					}
																				;
    break;}
case 34:
#line 228 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = yyvsp[0].pNode; ;
    break;}
case 35:
#line 229 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = yyvsp[0].pNode; ;
    break;}
case 36:
#line 230 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = yyvsp[0].pNode; ;
    break;}
case 37:
#line 231 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = yyvsp[0].pNode; ;
    break;}
case 38:
#line 234 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.psz = yyvsp[-1].psz; ;
    break;}
case 39:
#line 235 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.psz = yyvsp[-3].psz + "$" + yyvsp[-1].psz; ;
    break;}
case 40:
#line 237 "lang.y"
{
																						if ( NLang::GetStep() == 1 )
																						{
																							yyval.pNode = yyvsp[-1].pNode;
																							NLang::SetAttrToVars( yyval.pNode, yyvsp[-3].pNode );
																							NLang::SetTypeToVars( yyval.pNode, yyvsp[-2].psz );
																						}
																					;
    break;}
case 41:
#line 245 "lang.y"
{
																						if ( NLang::GetStep() == 1 )
																						{
																							yyval.pNode = yyvsp[-1].pNode;
																							NLang::SetAttrToVars( yyval.pNode, yyvsp[-3].pNode );
																							NLang::SetRndTypeToVars( yyval.pNode, yyvsp[-2].psz );
																						}
																					;
    break;}
case 42:
#line 255 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateVarListNode( yyvsp[0].pNode ); ;
    break;}
case 43:
#line 256 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateVarListNode( yyvsp[0].pNode ); ;
    break;}
case 44:
#line 257 "lang.y"
{
																		if ( NLang::GetStep() == 1 )
																		{
																			yyval.pNode = yyvsp[-2].pNode;
																			NLang::AddVarToVarListNode( yyval.pNode, yyvsp[0].pNode );
																		}
																	;
    break;}
case 45:
#line 264 "lang.y"
{
																		if ( NLang::GetStep() == 1 )
																		{
																			yyval.pNode = yyvsp[-2].pNode;
																			NLang::AddVarToVarListNode( yyval.pNode, yyvsp[0].pNode );
																		}
																	;
    break;}
case 46:
#line 273 "lang.y"
{ 
										if ( NLang::GetStep() == 1 ) 
										{
											yyval.pNode = yyvsp[0].pNode;
											NLang::SetVarToPointer( yyval.pNode );
										}
									;
    break;}
case 47:
#line 280 "lang.y"
{
																			if ( NLang::GetStep() == 1 ) 
																			{
																				yyval.pNode = yyvsp[-2].pNode;
																				NLang::SetVarToPointer( yyval.pNode );
																				NLang::SetComplexDefaultValueToVarNode( yyval.pNode, yyvsp[0].psz );
																			}
																		;
    break;}
case 48:
#line 290 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = yyvsp[0].pNode; ;
    break;}
case 49:
#line 291 "lang.y"
{
																if ( NLang::GetStep() == 1 )
																{
																	yyval.pNode = yyvsp[-2].pNode;
																	NLang::SetDefValueToVarNode( yyval.pNode, yyvsp[0].psz, false );
																}
															;
    break;}
case 50:
#line 298 "lang.y"
{
																if ( NLang::GetStep() == 1 )
																{
																	yyval.pNode = yyvsp[-2].pNode;
																	NLang::SetDefValueToVarNode( yyval.pNode, yyvsp[0].psz, true );
																}
															;
    break;}
case 51:
#line 305 "lang.y"
{
																if ( NLang::GetStep() == 1 )
																{
																	yyval.pNode = yyvsp[-2].pNode;
																	NLang::SetDefWStrValueToVarNode( yyval.pNode, yyvsp[0].psz );
																}
															;
    break;}
case 52:
#line 312 "lang.y"
{
																if ( NLang::GetStep() == 1 )
																{
																	yyval.pNode = yyvsp[-2].pNode;
																	NLang::SetDefValueToVarNode( yyval.pNode, "true", false );
																}
															;
    break;}
case 53:
#line 319 "lang.y"
{
																if ( NLang::GetStep() == 1 )
																{
																	yyval.pNode = yyvsp[-2].pNode;
																	NLang::SetDefValueToVarNode( yyval.pNode, "false", false );
																}
															;
    break;}
case 54:
#line 326 "lang.y"
{
																if ( NLang::GetStep() == 1 )
																{
																	yyval.pNode = yyvsp[-2].pNode;
																	NLang::SetEnumValueToVarNode( yyval.pNode, yyvsp[0].psz );
																}
															 ;
    break;}
case 55:
#line 333 "lang.y"
{
																		if ( NLang::GetStep() == 1 )
																		{
																			yyval.pNode = yyvsp[-2].pNode;
																			NLang::SetComplexDefaultValueToVarNode( yyval.pNode, yyvsp[0].psz );
																		}
																	;
    break;}
case 56:
#line 342 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateVar( yyvsp[0].psz, "", "" ) ;
    break;}
case 57:
#line 343 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateVar( yyvsp[-2].psz, "0", "unbounded" ) ;
    break;}
case 58:
#line 344 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateVar( yyvsp[-3].psz, "0", yyvsp[-1].psz ); ;
    break;}
case 59:
#line 345 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateVar( yyvsp[-4].psz, yyvsp[-2].psz, "unbounded" ); ;
    break;}
case 60:
#line 346 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateVar( yyvsp[-5].psz, yyvsp[-3].psz, yyvsp[-1].psz ); ;
    break;}
case 61:
#line 347 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateVar( yyvsp[-3].psz, "0", "unbounded" ) ;
    break;}
case 62:
#line 350 "lang.y"
{ yyval.pNode = 0; ;
    break;}
case 63:
#line 351 "lang.y"
{ 
																	if ( NLang::GetStep() == 1 )
																	{
																		if ( yyvsp[-1].pNode == 0 )
																			yyval.pNode = yyvsp[0].pNode;
																		else
																		{
																			yyval.pNode = yyvsp[-1].pNode;
																			NLang::MergeAttrList( yyval.pNode, yyvsp[0].pNode );
																		}
																	}
																;
    break;}
case 64:
#line 365 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = yyvsp[-1].pNode; ;
    break;}
case 65:
#line 366 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = yyvsp[-2].pNode; ;
    break;}
case 66:
#line 367 "lang.y"
{
																						if ( NLang::GetStep() == 1 )
																						{
																							yyval.pNode = NLang::CreateAttrDef( "comments", yyvsp[0].psz, true );
																							yyval.pNode = CreateAttrListNode( yyval.pNode );
																						}
																					;
    break;}
case 67:
#line 376 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateAttrListNode( yyvsp[0].pNode ); ;
    break;}
case 68:
#line 377 "lang.y"
{ 
																										if ( NLang::GetStep() == 1 )
																										{
																											yyval.pNode = yyvsp[-2].pNode;
																											NLang::AddAttrEntry( yyval.pNode, yyvsp[0].pNode );
																										}
																									;
    break;}
case 69:
#line 386 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateAttrDef( yyvsp[0].psz, "", false ); ;
    break;}
case 70:
#line 387 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateAttrDef( yyvsp[-2].psz, yyvsp[0].psz, false ); ;
    break;}
case 71:
#line 388 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateAttrDef( yyvsp[-2].psz, yyvsp[0].psz, true ); ;
    break;}
case 72:
#line 389 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateAttrDef( yyvsp[-2].psz, "true", false ); ;
    break;}
case 73:
#line 390 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateAttrDef( yyvsp[-2].psz, "false", false ); ;
    break;}
case 74:
#line 394 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateTypeDefNode( yyvsp[-4].pNode, yyvsp[-2].psz, yyvsp[-1].psz, false ); ;
    break;}
case 75:
#line 395 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateTypeDefNode( yyvsp[-5].pNode, yyvsp[-3].psz, yyvsp[-1].psz, true ); ;
    break;}
case 76:
#line 398 "lang.y"
{
																									if ( NLang::GetStep() == 1 )
																										yyval.pNode = NLang::CreateForwardEnumNode( yyvsp[-1].psz );
																								;
    break;}
case 77:
#line 402 "lang.y"
{ 
																													if ( NLang::GetStep() == 1 )
																													{
																														yyval.pNode = yyvsp[-2].pNode; 
																														NLang::SetNameToEnumNode( yyval.pNode, yyvsp[-4].psz );
																														NLang::AddAttrToEnumNode( yyval.pNode, yyvsp[-6].pNode );
																													}
																												;
    break;}
case 78:
#line 412 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateEnumNode( yyvsp[0].pNode ); ;
    break;}
case 79:
#line 413 "lang.y"
{	if ( NLang::GetStep() == 1 ) yyval.pNode = yyvsp[-1].pNode; ;
    break;}
case 80:
#line 414 "lang.y"
{
																						if ( NLang::GetStep() == 1 )
																						{
																							yyval.pNode = yyvsp[-2].pNode;
																							NLang::AddEnumEntry( yyval.pNode, yyvsp[0].pNode );
																						}
																					;
    break;}
case 81:
#line 423 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateEnumEntryNode( yyvsp[0].psz, "", false ); ;
    break;}
case 82:
#line 424 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateEnumEntryNode( yyvsp[-2].psz, yyvsp[0].psz, true ); ;
    break;}
case 83:
#line 425 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateEnumEntryNode( yyvsp[-2].psz, yyvsp[0].psz, false ); ;
    break;}
case 84:
#line 428 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateBaseTypeNode( yyvsp[-1].psz, false ); ;
    break;}
case 85:
#line 429 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateBaseTypeNode( yyvsp[-1].psz, true ); ;
    break;}
case 86:
#line 432 "lang.y"
{ yyval.psz = yyvsp[0].psz; ;
    break;}
case 87:
#line 433 "lang.y"
{ yyval.psz = "bool"; ;
    break;}
case 88:
#line 434 "lang.y"
{ yyval.psz = "int";	;
    break;}
case 89:
#line 435 "lang.y"
{ yyval.psz = "float"; ;
    break;}
case 90:
#line 436 "lang.y"
{ yyval.psz = "WORD"; ;
    break;}
case 91:
#line 437 "lang.y"
{ yyval.psz = "DWORD"; ;
    break;}
case 92:
#line 438 "lang.y"
{ yyval.psz = "string"; ;
    break;}
case 93:
#line 439 "lang.y"
{ yyval.psz = "hexbinary"; ;
    break;}
case 94:
#line 442 "lang.y"
{
																															if ( NLang::GetStep() == 1 )
																															{
																																yyval.pNode = yyvsp[-2].pNode;
																																NLang::SetNameToAttrDef( yyval.pNode, yyvsp[-4].psz );
																															}
																														;
    break;}
case 95:
#line 451 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.psz = yyvsp[0].psz; ;
    break;}
case 96:
#line 452 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.psz = yyvsp[0].psz; ;
    break;}
case 97:
#line 455 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateAttributeDefNode( NLang::EST_NOTYPE ); ;
    break;}
case 98:
#line 456 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateAttributeDefNode( NLang::EST_BOOL ); ;
    break;}
case 99:
#line 457 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateAttributeDefNode( NLang::EST_INT ); ;
    break;}
case 100:
#line 458 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateAttributeDefNode( NLang::EST_FLOAT ); ;
    break;}
case 101:
#line 459 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateAttributeDefNode( NLang::EST_WORD ); ;
    break;}
case 102:
#line 460 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateAttributeDefNode( NLang::EST_DWORD ); ;
    break;}
case 103:
#line 461 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateAttributeDefNode( NLang::EST_STRING ); ;
    break;}
case 104:
#line 462 "lang.y"
{ if ( NLang::GetStep() == 1 ) yyval.pNode = NLang::CreateAttributeDefNode( NLang::EST_HEXBINARY ); ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 487 "bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 464 "lang.y"


