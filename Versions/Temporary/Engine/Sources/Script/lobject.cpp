#include "stdafx.h"
/*
** $Id: lobject.c,v 1.55 2000/10/20 16:36:32 roberto Exp $
** Some generic functions over Lua objects
** See Copyright Notice in lua.h
*/

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "lobject.h"

#include "lsaver.h"

const TObject luaO_nilobject( LUA_TNIL );


const char *const luaO_typenames[] = {
  "userdata", "nil", "number", "string", "table", "function"
};



/*
** returns smaller power of 2 larger than `n' (minimum is MINPOWER2) 
*/
lint32 luaO_power2 (lint32 n) {
  lint32 p = MINPOWER2;
  while (p<=n) p<<=1;
  return p;
}


int luaO_equalObj (const TObject *t1, const TObject *t2) 
{
	return *t1 == *t2;
}


char *luaO_openspace (lua_State *L, size_t n) {
  if ( n > L->Mbuffer.size() ) 
		L->Mbuffer.resize(n);
  return &L->Mbuffer[0];
}


int luaO_str2d (const char *s, Number *result) {  /* LUA_NUMBER */
  char *endptr;
  Number res = lua_str2number(s, &endptr);
  if (endptr == s) return 0;  /* no conversion */
  while (isspace((unsigned char)*endptr)) endptr++;
  if (*endptr != '\0') return 0;  /* invalid trailing characters? */
  *result = res;
  return 1;
}


/* maximum length of a string format for `luaO_verror' */
#define MAX_VERROR	280

/* this function needs to handle only '%d' and '%.XXs' formats */
void __cdecl luaO_verror (lua_State *L, const char *fmt, ...) {
  va_list argp;
  char buff[MAX_VERROR];  /* to hold formatted message */
  va_start(argp, fmt);
  vsprintf(buff, fmt, argp);
  va_end(argp);
  lua_error(L, buff);
}


void __cdecl luaO_chunkid (char *out, const char *source, int bufflen) {
  if (!source) {
    return;
  }
  if (*source == '=') {
    strncpy(out, source+1, bufflen);  /* remove first char */
    out[bufflen-1] = '\0';  /* ensures null termination */
  }
  else {
    if (*source == '@') {
      int l;
      source++;  /* skip the `@' */
      bufflen -= sizeof("file `...%s'");
      l = strlen(source);
      if (l>bufflen) {
        source += (l-bufflen);  /* get last part of file name */
        sprintf(out, "file `...%.99s'", source);
      }
      else
        sprintf(out, "file `%.99s'", source);
    }
    else {
      int len = strcspn(source, "\n");  /* stop at first newline */
      bufflen -= sizeof("string \"%.*s...\"");
      if (len > bufflen) len = bufflen;
      if (source[len] != '\0') {  /* must truncate? */
        strcpy(out, "string \"");
        out += strlen(out);
        strncpy(out, source, len);
        strcpy(out+len, "...\"");
      }
      else
        sprintf(out, "string \"%.99s\"", source);
    }
  }
}

Proto::Proto()
{
  numparams = 0;
  is_vararg = 0;
  maxstacksize = 0;
  marked = 0;
  lineDefined = 0;
	bCreated = false;
}

// Serialize

int TObject::operator&( IBinSaver &f )
{
	f.Add( 2, &ttype );
	ASSERT( ttype != LUA_TNONE );
	switch ( ttype ) 
	{
		case LUA_TNUMBER :
		case LUA_TNIL :
			f.Add( 3, &value.n );
			break;
		case LUA_TUSERDATA :
			f.Add( 3, &value.nUData );
			break;
		case LUA_TFUNCTION :
			f.Add( 3, &value.nClosure );
			break;
		case LUA_TTABLE :
			f.Add( 3, &value.nHash );
			break;
		case LUA_TMARK :
			f.Add( 3, &value.nCallInfo );
			break;
		case LUA_TSTRING :
			lua_AddString( f, 3, &value.ts );
			break;
		default:
			ASSERT(0);
			return 0;
			// unknown object type - do nothing ( most possible stack garbage? )
	}
	return 0;
}

int LocVar::operator&( IBinSaver &f )
{
	f.Add( 2, &startpc );
	f.Add( 3, &endpc );
	lua_AddString( f, 4, &varname );
	return 0;
}

int Closure::operator&( IBinSaver &ff )
{
	ff.Add( 2, &isC );
	ff.Add( 3, &upvalues );
	if ( isC )
		lua_AddCFunc( &f.c, ff, 4 );
	else
		ff.Add( 4, &f.nProto );
	return 0;
}

int Proto::operator&( IBinSaver &f )
{
	f.Add( 2, &bCreated );
	f.Add( 3, &numbers );
	f.Add( 4, &protos );
	f.Add( 5, &code );
	f.Add( 6, &numparams );
	f.Add( 7, &is_vararg );
	f.Add( 8, &maxstacksize );
	f.Add( 9, &marked );
	f.Add( 10, &lineinfo );
	f.Add( 11, &locvars );
	f.Add( 12, &lineDefined );
#ifdef LUA_DEBUG
	f.Add( 13, &source );
#endif

	int nStrs = strings.size();
	f.Add( 14, &nStrs );
	if ( f.IsReading() )
		strings.resize( nStrs );
	for ( int i = 0; i < strings.size(); ++i )
		lua_AddString( f, 15, &strings[i], i + 1 );
	return 0;
}


