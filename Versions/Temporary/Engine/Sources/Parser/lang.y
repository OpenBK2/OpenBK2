%{

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

%}

%token CLASS
%token IDENTIFIER
%token DOUBLEDOT
%token NUMBER
%token TK_TRUE
%token TK_FALSE
%token UNBOUNDED
%token STRING
%token WSTRING
%token TYPEDEF
%token ENUM
%token ATTRIBUTE
%token STRUCT
%token VIS_TYPE
%token INCLUDE
%token BASE_CLASS
%token BASE_STRUCT
%token FORWARD
%token TYPE_BOOL
%token TYPE_INT
%token TYPE_FLOAT
%token TYPE_WORD
%token TYPE_DWORD
%token TYPE_STRING
%token TYPE_HEX
%token DOUBLE_DOT
%token COMMENT
%token H_EXTERNAL
%token CPP_EXTERNAL
%token SRND_INSTANCE
%token DEFAULT_VALUE

%% /* Grammar rules and actions follow */

input:	/* empty */
|				include_section
|				file_namespace
|				include_section file_namespace
;

include_section: include									{}
|								 include_section include	{}
;

include:	INCLUDE STRING					{
																		if ( NLang::GetStep() == 0 )
																		{
																			$$.psz = $2.psz;														
																			NLang::AddInclude( $$.psz );
																		}
																	}
|					H_EXTERNAL INCLUDE STRING	{
																			if ( NLang::GetStep() == 1 )
																			{
																				$$.psz = $3.psz;
																				NLang::AddHExternal( $3.psz );
																			}
																		}
|					CPP_EXTERNAL INCLUDE STRING	{
																				if ( NLang::GetStep() == 1 )
																				{
																					$$.psz = $3.psz;
																					NLang::AddCPPExternal( $3.psz );
																				}
																			}
;

file_namespace:	type_definition									{}
|								file_namespace type_definition	{}
;

type_definition:	class								{ if ( NLang::GetStep() == 1 ) NLang::AddDef( $1.pNode ); }
|									struct							{ if ( NLang::GetStep() == 1 ) NLang::AddDef( $1.pNode ); }
|									enum								{ if ( NLang::GetStep() == 1 ) NLang::AddDef( $1.pNode ); }
|									typedef							{ if ( NLang::GetStep() == 1 ) NLang::AddDef( $1.pNode ); }
|									attribute						{ if ( NLang::GetStep() == 1 ) NLang::AddDef( $1.pNode ); }
|									newtype							{ if ( NLang::GetStep() == 1 ) NLang::AddDef( $1.pNode ); }
;

class:	FORWARD CLASS IDENTIFIER ';'									{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateForwardComplexType( $3.psz, true ); }
|				attrlist CLASS class_header type_namespace		{
																												if ( NLang::GetStep() == 1 )
																												{
																													$$.pNode = $3.pNode;
																													NLang::AddAttrToComplexTypeNode( $$.pNode, $1.pNode );
																													NLang::AddNamespaceToComplexTypeNode( $$.pNode, $4.pNode );
																													NLang::CloseNamespace();
																												}
																											}
;

struct:	FORWARD STRUCT IDENTIFIER ';'										{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateForwardComplexType( $3.psz, false ); }
|				attrlist STRUCT struct_header type_namespace		{
																													if ( NLang::GetStep() == 1 )
																													{
																														$$.pNode = $3.pNode;
																														NLang::AddAttrToComplexTypeNode( $$.pNode, $1.pNode );
																														NLang::AddNamespaceToComplexTypeNode( $$.pNode, $4.pNode );
																														NLang::CloseNamespace();
																													}
																												}
;

class_header: IDENTIFIER											{ 
																								if ( NLang::GetStep() == 1 )
																								{
																									$$.pNode = NLang::CreateComplexTypeNode( $1.psz, true );
																									NLang::OpenNewNamespace( 0 );
																								}
																							}
|							IDENTIFIER ':' parents_list			{
																								if ( NLang::GetStep() == 1 )
																								{
																									$$.pNode = NLang::CreateComplexTypeNode( $1.psz, true );
																									NLang::AddParentsOfComplexType( $$.pNode, $3.pNode );
																									NLang::OpenNewNamespace( $3.pNode );
																								}
																							}
;

struct_header:	IDENTIFIER									{ 
																							if ( NLang::GetStep() == 1 )
																							{
																								$$.pNode = NLang::CreateComplexTypeNode( $1.psz, false );
																								NLang::OpenNewNamespace( 0 );
																							}
																						}
|								IDENTIFIER ':' parents_list	{
																							if ( NLang::GetStep() == 1 )
																							{
																								$$.pNode = NLang::CreateComplexTypeNode( $1.psz, false );
																								NLang::AddParentsOfComplexType( $$.pNode, $3.pNode );
																								NLang::OpenNewNamespace( $3.pNode );
																							}
																						}
;

parents_list : parent_declaration										{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateParentsList( $1.psz ); }
;

parent_declaration :	VIS_TYPE IDENTIFIER	{ if ( NLang::GetStep() == 1 ) $$.psz = $2.psz; }
;

type_namespace: '{' namespace_defs '}' ';' { if ( NLang::GetStep() == 1 ) $$.pNode = $2.pNode; }
;

namespace_defs: /*empty*/					{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::GetCurrentNamespace(); }
|			namespace_defs inside_type	{
																		if ( NLang::GetStep() == 1 )
																		{
																			$$.pNode = $1.pNode;
																			NLang::AddTypeToNamespace( $$.pNode, $2.pNode );
																		}
																	}
|						namespace_defs field	{
																		if ( NLang::GetStep() == 1 )
																		{
																			$$.pNode = $1.pNode;
																			NLang::AddVarListToNamespace( $$.pNode, $2.pNode );
																		}
																	}
|					namespace_defs VIS_TYPE ':'	{ if ( NLang::GetStep() == 1 ) $$.pNode = $1.pNode; }
|					namespace_defs INCLUDE STRING { 
																					if ( NLang::GetStep() == 1 )
																					{
																						$$.pNode = $1.pNode;
																						NLang::AddBadIncludeToNamespace( $$.pNode, $3.psz );
																					}
																				}
;

inside_type:	class								{ if ( NLang::GetStep() == 1 ) $$.pNode = $1.pNode; }
|							struct							{ if ( NLang::GetStep() == 1 ) $$.pNode = $1.pNode; }
|							enum								{ if ( NLang::GetStep() == 1 ) $$.pNode = $1.pNode; }
|							typedef							{ if ( NLang::GetStep() == 1 ) $$.pNode = $1.pNode; }
;

rnd_instance: SRND_INSTANCE '<' IDENTIFIER '>'								{ if ( NLang::GetStep() == 1 ) $$.psz = $3.psz; }
|							SRND_INSTANCE '<' IDENTIFIER ',' new_type_id '>' { if ( NLang::GetStep() == 1 ) $$.psz = $3.psz + "$" + $5.psz; }

field:	attrlist new_type_id varlist ';'	{
																						if ( NLang::GetStep() == 1 )
																						{
																							$$.pNode = $3.pNode;
																							NLang::SetAttrToVars( $$.pNode, $1.pNode );
																							NLang::SetTypeToVars( $$.pNode, $2.psz );
																						}
																					}
|				attrlist rnd_instance varlist ';'	{
																						if ( NLang::GetStep() == 1 )
																						{
																							$$.pNode = $3.pNode;
																							NLang::SetAttrToVars( $$.pNode, $1.pNode );
																							NLang::SetRndTypeToVars( $$.pNode, $2.psz );
																						}
																					}																			
;

varlist:	simple_var							{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateVarListNode( $1.pNode ); }
|					pointer									{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateVarListNode( $1.pNode ); }
|					varlist ',' simple_var	{
																		if ( NLang::GetStep() == 1 )
																		{
																			$$.pNode = $1.pNode;
																			NLang::AddVarToVarListNode( $$.pNode, $3.pNode );
																		}
																	}
|					varlist ',' pointer			{
																		if ( NLang::GetStep() == 1 )
																		{
																			$$.pNode = $1.pNode;
																			NLang::AddVarToVarListNode( $$.pNode, $3.pNode );
																		}
																	}
;

pointer: '*' var	{ 
										if ( NLang::GetStep() == 1 ) 
										{
											$$.pNode = $2.pNode;
											NLang::SetVarToPointer( $$.pNode );
										}
									}
|				 '*' var '=' DEFAULT_VALUE {
																			if ( NLang::GetStep() == 1 ) 
																			{
																				$$.pNode = $2.pNode;
																				NLang::SetVarToPointer( $$.pNode );
																				NLang::SetComplexDefaultValueToVarNode( $$.pNode, $4.psz );
																			}
																		}
;

simple_var:	var								{ if ( NLang::GetStep() == 1 ) $$.pNode = $1.pNode; }
|						var '=' NUMBER		{
																if ( NLang::GetStep() == 1 )
																{
																	$$.pNode = $1.pNode;
																	NLang::SetDefValueToVarNode( $$.pNode, $3.psz, false );
																}
															}
|						var	'=' STRING		{
																if ( NLang::GetStep() == 1 )
																{
																	$$.pNode = $1.pNode;
																	NLang::SetDefValueToVarNode( $$.pNode, $3.psz, true );
																}
															}
|						var '=' WSTRING		{
																if ( NLang::GetStep() == 1 )
																{
																	$$.pNode = $1.pNode;
																	NLang::SetDefWStrValueToVarNode( $$.pNode, $3.psz );
																}
															}
|						var '=' TK_TRUE		{
																if ( NLang::GetStep() == 1 )
																{
																	$$.pNode = $1.pNode;
																	NLang::SetDefValueToVarNode( $$.pNode, "true", false );
																}
															}
|						var	'=' TK_FALSE	{
																if ( NLang::GetStep() == 1 )
																{
																	$$.pNode = $1.pNode;
																	NLang::SetDefValueToVarNode( $$.pNode, "false", false );
																}
															}
|						var '='	IDENTIFIER {
																if ( NLang::GetStep() == 1 )
																{
																	$$.pNode = $1.pNode;
																	NLang::SetEnumValueToVarNode( $$.pNode, $3.psz );
																}
															 }
|						var '=' DEFAULT_VALUE {
																		if ( NLang::GetStep() == 1 )
																		{
																			$$.pNode = $1.pNode;
																			NLang::SetComplexDefaultValueToVarNode( $$.pNode, $3.psz );
																		}
																	}
;

var:	IDENTIFIER																	{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateVar( $1.psz, "", "" ) };
|			IDENTIFIER '['']'														{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateVar( $1.psz, "0", "unbounded" ) };
|			IDENTIFIER '[' NUMBER ']'										{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateVar( $1.psz, "0", $3.psz ); }
|			IDENTIFIER '[' NUMBER DOUBLE_DOT ']'				{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateVar( $1.psz, $3.psz, "unbounded" ); }
|			IDENTIFIER '[' NUMBER DOUBLE_DOT NUMBER ']'	{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateVar( $1.psz, $3.psz, $5.psz ); }
|			IDENTIFIER '[' DOUBLE_DOT ']'								{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateVar( $1.psz, "0", "unbounded" ) }
;

attrlist: /* empty */						{ $$.pNode = 0; }
|					attrlist attr_section	{ 
																	if ( NLang::GetStep() == 1 )
																	{
																		if ( $1.pNode == 0 )
																			$$.pNode = $2.pNode;
																		else
																		{
																			$$.pNode = $1.pNode;
																			NLang::MergeAttrList( $$.pNode, $2.pNode );
																		}
																	}
																}
;

attr_section: '[' attributes_def ']'			{ if ( NLang::GetStep() == 1 ) $$.pNode = $2.pNode; }
|							'[' attributes_def ';' ']'	{ if ( NLang::GetStep() == 1 ) $$.pNode = $2.pNode; }
|							COMMENT											{
																						if ( NLang::GetStep() == 1 )
																						{
																							$$.pNode = NLang::CreateAttrDef( "comments", $1.psz, true );
																							$$.pNode = CreateAttrListNode( $$.pNode );
																						}
																					}
;

attributes_def:	attribute_def											{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateAttrListNode( $1.pNode ); }
|								attributes_def ';' attribute_def	{ 
																										if ( NLang::GetStep() == 1 )
																										{
																											$$.pNode = $1.pNode;
																											NLang::AddAttrEntry( $$.pNode, $3.pNode );
																										}
																									}
;

attribute_def:	attr_identifier								{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateAttrDef( $1.psz, "", false ); }
|								attr_identifier '=' NUMBER		{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateAttrDef( $1.psz, $3.psz, false ); }
|								attr_identifier '=' STRING		{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateAttrDef( $1.psz, $3.psz, true ); }
|								attr_identifier '=' TK_TRUE		{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateAttrDef( $1.psz, "true", false ); }
|								attr_identifier '=' TK_FALSE	{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateAttrDef( $1.psz, "false", false ); }
;


typedef:	attrlist TYPEDEF new_type_id new_type_id ';' { if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateTypeDefNode( $1.pNode, $3.psz, $4.psz, false ); }
|					attrlist TYPEDEF IDENTIFIER '*' new_type_id ';' { if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateTypeDefNode( $1.pNode, $3.psz, $5.psz, true ); }
;

enum: FORWARD ENUM IDENTIFIER ';'								{
																									if ( NLang::GetStep() == 1 )
																										$$.pNode = NLang::CreateForwardEnumNode( $3.psz );
																								}
|			attrlist ENUM IDENTIFIER '{' enum_entries '}' ';'	{ 
																													if ( NLang::GetStep() == 1 )
																													{
																														$$.pNode = $5.pNode; 
																														NLang::SetNameToEnumNode( $$.pNode, $3.psz );
																														NLang::AddAttrToEnumNode( $$.pNode, $1.pNode );
																													}
																												}
;

enum_entries:	enum_entry									{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateEnumNode( $1.pNode ); }
|							enum_entries ','						{	if ( NLang::GetStep() == 1 ) $$.pNode = $1.pNode; }
|							enum_entries ',' enum_entry	{
																						if ( NLang::GetStep() == 1 )
																						{
																							$$.pNode = $1.pNode;
																							NLang::AddEnumEntry( $$.pNode, $3.pNode );
																						}
																					}
;

enum_entry:	IDENTIFIER								{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateEnumEntryNode( $1.psz, "", false ); }
|						IDENTIFIER '=' NUMBER			{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateEnumEntryNode( $1.psz, $3.psz, true ); }
|						IDENTIFIER '=' IDENTIFIER	{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateEnumEntryNode( $1.psz, $3.psz, false ); }
;

newtype:	BASE_STRUCT new_type_id ';'	{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateBaseTypeNode( $2.psz, false ); }
|					BASE_CLASS new_type_id ';'	{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateBaseTypeNode( $2.psz, true ); }
;

new_type_id:	IDENTIFIER	{ $$.psz = $1.psz; }
|							TYPE_BOOL		{ $$.psz = "bool"; }
|							TYPE_INT		{ $$.psz = "int";	}
|							TYPE_FLOAT	{ $$.psz = "float"; }
|							TYPE_WORD		{ $$.psz = "WORD"; }
|							TYPE_DWORD	{ $$.psz = "DWORD"; }
|							TYPE_STRING	{ $$.psz = "string"; }
|							TYPE_HEX		{ $$.psz = "hexbinary"; }
;

attribute: ATTRIBUTE attr_identifier '(' attr_type ')' ';' {
																															if ( NLang::GetStep() == 1 )
																															{
																																$$.pNode = $4.pNode;
																																NLang::SetNameToAttrDef( $$.pNode, $2.psz );
																															}
																														}
;

attr_identifier:	IDENTIFIER	{ if ( NLang::GetStep() == 1 ) $$.psz = $1.psz; }
|									VIS_TYPE		{ if ( NLang::GetStep() == 1 ) $$.psz = $1.psz; }
;
																											
attr_type: /* empty */	{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateAttributeDefNode( NLang::EST_NOTYPE ); }
| TYPE_BOOL							{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateAttributeDefNode( NLang::EST_BOOL ); }
| TYPE_INT							{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateAttributeDefNode( NLang::EST_INT ); }
| TYPE_FLOAT						{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateAttributeDefNode( NLang::EST_FLOAT ); }
| TYPE_WORD							{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateAttributeDefNode( NLang::EST_WORD ); }
| TYPE_DWORD						{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateAttributeDefNode( NLang::EST_DWORD ); }
| TYPE_STRING						{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateAttributeDefNode( NLang::EST_STRING ); }
| TYPE_HEX							{ if ( NLang::GetStep() == 1 ) $$.pNode = NLang::CreateAttributeDefNode( NLang::EST_HEXBINARY ); }
;
%%
