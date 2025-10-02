/*
** $Id: ltm.h,v 1.18 2000/10/05 13:00:17 roberto Exp $
** Tag methods
** See Copyright Notice in lua.h
*/

#ifndef ltm_h
#define ltm_h



#include "lsaver.h"

/*
* WARNING: if you change the order of this enumeration,
* grep "ORDER TM"
*/
typedef enum {
  TM_GETTABLE = 0,
  TM_SETTABLE,
  TM_INDEX,
  TM_GETGLOBAL,
  TM_SETGLOBAL,
  TM_ADD,
  TM_SUB,
  TM_MUL,
  TM_DIV,
  TM_POW,
  TM_UNM,
  TM_LT,
  TM_CONCAT,
  TM_GC,
  TM_FUNCTION,
  TM_N		/* number of elements in the enum */
} TMS;

struct TMinfo
{
  int method[TM_N];
};

struct TM {
	ZDATA
	TMinfo info;
  list<int> collected;  /* list of garbage-collected udata indices with this tag */
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&info); f.Add(3,&collected); return 0; }
};


int luaT_tag (lua_State *L, const TObject *o);

inline int& luaT_gettm( lua_State *L, int tag, int event ) { return L->TMtable[tag].info.method[event]; }
inline int luaT_gettmindex( lua_State *L, const TObject *o, int e ) { return luaT_gettm( L, luaT_tag( L, o ), e ); }
inline Closure *luaT_gettm( lua_State *L, const TObject *o, int e )  
{ 
	int nCL = luaT_gettmindex( L, o, e );
	if ( nCL == STK_NULL )
		return NULL;
	return L->closures[ nCL ];
}

inline bool IsValidTag( lua_State *L, int nTag ) 
{
	return nTag >= NUM_TAGS && nTag < L->TMtable.size();
}

extern const char *const luaT_eventname[];


void luaT_init (lua_State *L);
void luaT_realtag (lua_State *L, int tag);
int luaT_validevent (int t, int e);  /* used by compatibility module */


#endif

