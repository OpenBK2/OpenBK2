#pragma once
/*
** $Id: lstate.h,v 1.41 2000/10/05 13:00:17 roberto Exp $
** Global State
** See Copyright Notice in lua.h
*/


#include "lobject.h"
#include "luadebug.h"


template<class T>
class CVectorList
{
	struct SHolder
	{
		int nNextFree;
		T *pData;
		SHolder() {}
		SHolder( T* pT ) : pData(pT), nNextFree(-1) {}
		int operator&( IBinSaver &f )
		{
			bool bValid;
			if ( !f.IsReading() )
				bValid = pData;
			f.Add( 2, &nNextFree );
			f.Add( 3, &bValid );
			if ( bValid )
			{
				if ( f.IsReading() )
					pData = new T;
				f.Add( 4, pData );
			}
			else
				pData = 0;
			return 0;
		}
	};
	std::vector<SHolder> data;
	int nFirstFree;
public:
	CVectorList() : nFirstFree(-1) {}
	void clear()
	{
		for ( int i = 0; i < data.size(); ++i )
		{
			if ( data[i].pData )
				erase(i);
		}
	}
	~CVectorList() { clear(); }
	T* operator[]( int i ) { return data[i].pData; }
	int size() const { return data.size(); }
	int push( T* pT ) 
	{
		if ( nFirstFree >= 0 )
		{
			int nRet = nFirstFree;
			data[ nFirstFree ].pData = pT;
			nFirstFree = data[ nFirstFree ].nNextFree;
			return nRet;
		}
		else
		{
			data.push_back( SHolder( pT ) );
			return data.size() - 1;
		}
	}
	void erase( int i ) 
	{ 
		ASSERT( data[i].pData );
		delete data[i].pData;
		data[i].pData = 0; 
		data[i].nNextFree = nFirstFree; 
		nFirstFree = i; 
	}
	bool empty() const  
	{ 
		for ( int i = 0; i < data.size(); ++i ) 
		{
			if ( data[i].pData )
				return false; 
		}
		return true;
	} 
	int operator&( IBinSaver &f );
};


typedef int StkId;  /* index to stack elements */

/*
** marks for Reference array
*/
#define NONEXT          -1      /* to end the free list */
#define HOLD            -2
#define COLLECTED       -3
#define LOCK            -4

struct Ref {
	ZDATA
	TObject o;
	int st;  // can be LOCK, HOLD, COLLECTED, or next (for free list)
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&o); f.Add(3,&st); return 0; }
};

struct TM;  /* defined in ltm.h */


/// The current state of Lua internal virtual mashine
struct SLuaVMState
{
	ZDATA
	int nProto; // pointer to tf
	int nClosure;
	StkId base;
	int currPC;
	int nResults;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nProto); f.Add(3,&nClosure); f.Add(4,&base); f.Add(5,&currPC); f.Add(6,&nResults); return 0; }
};

class CLuaThread : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CLuaThread);

public:
	ZDATA
	StkId top;  // first free slot in the stack 
	StkId Cbase;  // base for current C function
	std::vector<TObject> stack;  // stack base
	std::vector<SLuaVMState> executedCalls;
	int thisThreadIsSleeping;
	bool bErrorInThread;
	std::string name;
	int errorHookRef;
//	CPtr<CThreadGroup> pGroup;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&top); f.Add(3,&Cbase); f.Add(4,&stack); f.Add(5,&executedCalls); f.Add(6,&thisThreadIsSleeping); f.Add(7,&bErrorInThread); f.Add(8,&name); f.Add(9,&errorHookRef); return 0; }

	lua_State *L;

	void setErrorHook();
	void clearErrorHook();

protected:
	CLuaThread();
public:
	CLuaThread( lua_State *L, const char *name );
	~CLuaThread();

	bool HasValidTop() const { return top >= 0 && top < stack.size(); }
};


//class CThreadGroup : public CObjectBase
//{
//	OBJECT_NOCOPY_METHODS(CThreadGroup);
//
//private:
//	ZDATA
//	string szName;
//	vector<CPtr<CLuaThread> > members;
//public:
//	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szName); f.Add(3,&members); return 0; }
//
//protected:
//	CThreadGroup();
//public:
//	CThreadGroup( const string &_szName ) : szName(_szName), members(1) {}
//
//	void Add( const CLuaThread *pThread )
//	{
//		members.push_back( pThread );
//	}
//	void Remove( const CLuaThread *pThread )
//	{
//		remove( members.begin(), members.end(), pThread );
//	}
////	void Clear()
////	{
////		members.clear();
////	}
//};


typedef std::list<CObj<CLuaThread> > CThreads;
typedef std::unordered_map<TString, bool, TStringHash> CLuaStrings;


struct lua_State 
{
	// execution context
	CPtr<CObjectBase> pContext;
	// lua_State instance name, used as prefix to registered C-functions
	// throughout serialization
	std::string szName;

	// thread-specific state
	CPtr<CLuaThread> pCT; // current thread
	std::vector<char> Mbuffer;  // buffer

	// global state 
	CThreads threads;
	int nGT;  // index of table for globals 
	std::vector<TM> TMtable;  // table for tag methods
	std::vector<Ref> refArray;  // locked objects
	int refFree;  // list of free positions in refArray
	unsigned long nGCticks;  // number of `bytes' currently allocated
	int nGCAvoid;
	int allowhooks;
	int nNoWait;
	
	lua_Hook callhook;
	lua_Hook linehook;
	CLuaStrings strings;
	CVectorList<Proto> protos;  
	CVectorList<Closure> closures;  
	CVectorList<Hash> tables;  
	CVectorList<UserData> userdatas;
	CVectorList<CallInfo> callInfos;

	~lua_State();
	int operator&( IBinSaver &f );
	Hash* GetHash( StkId from ) { return tables[ pCT->stack[ from ].GetH() ]; }
	Closure *GetClosure( StkId from ) { return closures[ pCT->stack[ from ].GetCL() ]; }
	UserData *GetUData( StkId from ) { return userdatas[ pCT->stack[ from ].GetUData() ]; }
	CallInfo *GetCallInfo( StkId from ) { return callInfos[ pCT->stack[ from ].GetCI() ]; }
};


inline TObject *LObj( lua_State *L, StkId st )
{
	return &L->pCT->stack[ st ];
}

inline bool iscfunction( lua_State *L, TObject *o )	
{ 
	return o->GetType() == LUA_TFUNCTION && L->closures[ o->GetCL() ]->isC; 
}

CLuaThread* lua_newThread( lua_State *L, const char *name );
void lua_setThread( lua_State *L, CLuaThread *pThread );


const std::string MakeUniqueName( lua_State *L, const char *name );



