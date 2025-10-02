#pragma once
/*
** $Id: lobject.h,v 1.82 2000/10/30 17:49:19 roberto Exp $
** Type definitions for Lua objects
** See Copyright Notice in lua.h
*/



#include "llimits.h"
#include "lua.h"

#include <assert.h>
#define LUA_INTERNALERROR(s)	assert(((void)s,0))
#define LUA_ASSERT(c,s)		ASSERT(c)

#define STK_NULL -1

#ifndef _MAPEDIT
#ifdef _DEBUG
/* to avoid warnings, and make sure value is really unused */
#define UNUSED(x)	(x=0, (void)(x))
#else
#define UNUSED(x)	((void)(x))	/* to avoid warnings */
#endif
#endif // _MAPEDIT

/* mark for closures active in the stack */
#define LUA_TMARK	6


/* tags for values visible from Lua == first user-created tag */
#define NUM_TAGS	6


/* check whether `t' is a mark */
#define is_T_MARK(t)	((t) == LUA_TMARK)


typedef union {
	int nUData; /* LUA_TUSERDATA */
	int nClosure; /* LUA_TFUNCTION */
	int nHash; /* LUA_TTABLE */
  Number n;		/* LUA_TNUMBER */
  int nCallInfo;	/* LUA_TMARK */

	struct TString *ts;	/* LUA_TSTRING */
} Value;

struct TString
{
private:
	ZDATA
	string str;
public:
	int marked;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&str); f.Add(3,&marked); return 0; }

	TString() {}
	TString( const char *_p ) : str(_p), marked(0) {}
	bool empty() const { return str.empty(); }
	int size() const { return str.size(); }
	int IsMarked() { return marked; }
	const char* c_str() const { return str.c_str(); }
	const string& GetStr() const { return str; }

	bool operator==( const TString &other ) const
	{
		return str == other.str;
	}
	friend struct TStringHash;
};

struct Hash;

struct TObject 
{
private:
	int ttype;
  Value value;
public:
	int operator&( IBinSaver &f );

	TObject() : ttype(LUA_TNIL) {}
	TObject( int type ): ttype( type ) {}

	int GetType() const { return ttype; }
	Number GetN() const { ASSERT( ttype == LUA_TNUMBER ); return value.n; }
	int GetUData() const { ASSERT( ttype == LUA_TUSERDATA ); return value.nUData; } 
	TString* GetS() const { ASSERT( ttype == LUA_TSTRING ); return value.ts; }
	int GetCL() const { ASSERT( ttype == LUA_TFUNCTION ); return value.nClosure; }
	int GetH() const { ASSERT( ttype == LUA_TTABLE ); return value.nHash; }
	int GetCI() const { ASSERT( ttype == LUA_TMARK ); return value.nCallInfo; }
	bool HasValidType() const 
	{ 
		switch( ttype ) 
		{
			case LUA_TSTRING: 
			case LUA_TMARK: 
			case LUA_TUSERDATA: 
			case LUA_TNUMBER: 
			case LUA_TNIL: 
			case LUA_TFUNCTION: 
			case LUA_TTABLE:
				return true;
		}  
		return false;
	}

	void SetN( Number n ) { ttype = LUA_TNUMBER; value.n = n; }
	void SetS( TString *str ) { ttype = LUA_TSTRING; value.ts = str; }
	void SetCL( int nClosure ) { ttype = LUA_TFUNCTION; value.nClosure = nClosure; }
	void SetH( int nHash ) { ttype = LUA_TTABLE; value.nHash = nHash; }
	void SetCI( int nCI ) { ttype = LUA_TMARK; value.nCallInfo = nCI; }
	void SetNil() { ttype = LUA_TNIL; }
	void SetUData( int nUData ) { ttype = LUA_TUSERDATA; value.nUData = nUData; }

	bool operator==( const TObject& obj ) const
	{
		ASSERT( HasValidType() );
		ASSERT( obj.HasValidType() );
		if ( ttype != obj.ttype )
			return false;
		switch ( ttype ) 
		{
		case LUA_TSTRING:
			return GetS()->GetStr() == obj.GetS()->GetStr();
		case LUA_TNUMBER:
			return GetN() == obj.GetN();
		case LUA_TNIL:
			return true;
		}
		return value.nCallInfo == obj.value.nCallInfo;
	}
};

extern const TObject luaO_nilobject;

/*
** String headers for string table
*/

/*
** most `malloc' libraries allocate memory in blocks of 8 bytes. TSPACK
** tries to make sizeof(TString) a multiple of this granularity, to reduce
** waste of space.
*/
#define TSPACK	((int)sizeof(int))



struct UserData 
{
	ZDATA
  int tag;
	int marked;
  CPtr<CObjectBase> pPtr;
	CObj<CObjectBase> pObj;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&tag); f.Add(3,&marked); f.Add(4,&pPtr); f.Add(5,&pObj); return 0; }
};

struct TStringHash
{
	int operator()( const TString &str ) const
	{
		hash<string> comparer;
		return comparer(str.str);
	}
};

/*
** Function Prototypes
*/

struct LocVar 
{
	TString *varname;
  int startpc;  /* first point where variable is active */
  int endpc;    /* first point where variable is dead */
	int operator&( IBinSaver &f );
};

struct Proto {
	bool bCreated;
  vector<Number> numbers;  /* Number numbers used by the function */
  vector<int> protos;  /* functions' defined inside the function, indices */
  vector<Instruction> code;
  short numparams;
  short is_vararg;
  short maxstacksize;
  short marked;
  /* debug information */
  vector<int> lineinfo;  /* map from opcodes to source lines */
  vector<LocVar> locvars;  /* information about local variables */

	vector<TString *> strings;  /* strings used by the function */
#ifdef LUA_DEBUG
	string source;
#endif
  int lineDefined;

	Proto();
	int operator&( IBinSaver &f );
};

/*
** Closures
*/
struct Closure {
  union {
    lua_CFunction c;  /* C functions */
    int nProto;  /* Lua function index in lua_State */
  } f;
  short isC;  /* 0 for Lua functions, 1 for C functions */

	struct Closure *mark;  /* marked closures (point to itself when not marked) */
  vector<TObject> upvalues;

	Closure() : mark(this), isC(0) { f.c = 0; }
	int operator&( IBinSaver &f );
};


struct ObjectHash
{
	int operator()( const TObject &key ) const
	{
		unsigned long h;
		switch ( key.GetType() ) 
		{
			case LUA_TNUMBER:
				h = (unsigned long)(long)( key.GetN() );
				break;
			case LUA_TSTRING:
				{
					hash<string> hh;
					h = hh( key.GetS()->GetStr() );
				}
				break;
			case LUA_TUSERDATA:
				h = IntPoint( key.GetUData() );
				break;
			case LUA_TTABLE:
				h = IntPoint( key.GetH() );
				break;
			case LUA_TFUNCTION:
				h = IntPoint( key.GetCL() );
				break;
			default:
				ASSERT( 0 );
				return 0;  /* invalid key */
		}
		return h;
	}
};

struct Hash 
{
	typedef hash_map<TObject, TObject, ObjectHash> CObjHash;
  struct Hash *mark;  /* marked tables (point to itself when not marked) */

	ZDATA
	CObjHash h;
	int htag;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&h); f.Add(3,&htag); return 0; }

	struct Node 
	{
		const TObject *key;
		const TObject *val;
	};
	Hash() : mark(this), htag(LUA_TTABLE) {}
	const TObject* GetObj( const TObject *key ) const
	{
		CObjHash::const_iterator it = h.find( *key );
		if ( it == h.end() )
			return &luaO_nilobject;
		return &it->second;
	}
	const TObject* GetNum( Number key ) const
	{
		TObject obj;
		obj.SetN( key );
		return GetObj( &obj );
	}
	const TObject* GetStr( TString *key ) const
	{
		TObject obj;
		obj.SetS( key );
		return GetObj( &obj );
	}
	Number GetMaxIntKey() const
	{
    Number max = 0;
		for ( CObjHash::const_iterator i = h.begin(); i != h.end(); ++i )
		{
      if ( i->first.GetType() == LUA_TNUMBER &&
					 i->second.GetType() != LUA_TNIL &&
           i->first.GetN() > max )
        max = i->first.GetN();
		}
		return max;
	}
	TObject *SetObj( const TObject *key )	{	ASSERT( key->GetType() != LUA_TNIL ); return &h[*key]; }
	TObject *SetInt( int key )
	{
		TObject obj;
		obj.SetN( key );
		return SetObj( &obj );
	}
	TObject *SetStr( TString *key )
	{
		TObject obj;
		obj.SetS( key );
		return SetObj( &obj );
	}
	void NextNode( const TObject *r, Node *pRes ) const
	{
		CObjHash::const_iterator res;
		if ( r->GetType() == LUA_TNIL )
		{
			res = h.begin();
		}
		else
		{
			res = h.find( *r );
			if ( res != h.end() )
				++res;
			else
				ASSERT(0);
		}
		if ( res == h.end() )
		{
			pRes->key = 0;
			pRes->val = 0;
		}
		else
		{
			pRes->key = &res->first;
			pRes->val = &res->second;
		}
	}
};


/* unmarked tables and closures are represented by pointing `mark' to
** themselves
*/
#define ismarked(x)	((x)->mark != (x))


/*
** informations about a call (for debugging)
*/
struct CallInfo
{
  int nFunc;  /* index of closure being called */
	int nProto;
	int lastpc;  /* last pc traced */
  int line;  /* current line */
  int refi;  /* current index in `lineinfo' */
};

extern const char *const luaO_typenames[];

inline const char *luaO_typename( const TObject * o ) { return luaO_typenames[ o->GetType() ]; }

lint32 luaO_power2 (lint32 n);
char *luaO_openspace (lua_State *L, size_t n);

int luaO_equalObj (const TObject *t1, const TObject *t2);
int luaO_str2d (const char *s, Number *result);

void __cdecl luaO_verror (lua_State *L, const char *fmt, ...);
void __cdecl luaO_chunkid (char *out, const char *source, int len);



