#pragma once

#include "System_export.h"

EXTERNVAR SYSTEM_EXPORT int nDGCurrentFrame;

// DG 

// any function should have version of its value to recalc only when function result
// has changed, has frame counter to prevent frequent reevaluations
class CVersioningBase: public CObjectBase
{
	OBJECT_BASIC_METHODS( CVersioningBase );
	int nFrameCalced;     // mark of last update frame, used to cut recalc time
	int nVersion;         // current version number, used to determine if setParam needed
	//
	void DoUpdate() 
	{
		ASSERT( IsValid( this ) ); 
		if ( NeedUpdate() ) 
		{
			++nVersion;
			Recalc();
		} 
		nFrameCalced = nDGCurrentFrame; 
	}
protected:
	bool IsFrameMatch() const { return nFrameCalced == nDGCurrentFrame; }
	virtual bool NeedUpdate() { return false; }
	virtual void Recalc() { ASSERT( 0 ); }
public:
	CVersioningBase() { nFrameCalced = 0; nVersion = 1; }
	void Updated() { nVersion++; } // should be called when function recalc its value or value is changed
	//! returns if node was refreshed on this frame, for information purposes, normally not needed for user
	bool WasRefreshed() const { return nFrameCalced == nDGCurrentFrame; }
	friend class CBaseScene;
	friend class CAccessForDGPtr;
	//
	virtual int operator&( IBinSaver &f ) { return 0; }
};

template <class TResult>
class CFuncBase: public CVersioningBase
{
protected:
	TResult value;
public:
	CFuncBase() {}
	CFuncBase( const TResult &_t ): value(_t) {}
	const TResult& GetValue() const { ASSERT( IsFrameMatch() ); return value; }
};

template <class TResult>
class CPtrFuncBase: public CVersioningBase
{
protected:
	CObj<TResult> pValue;
public:
	TResult* GetValue() { ASSERT( IsFrameMatch() ); if ( !IsValid( pValue ) ) Recalc(); return pValue; }
  bool HasZeroValue() const { return pValue == 0; }
};

SYSTEM_EXPORT void SetToHoldQueue( CObjectBase *p, int *pnFrame );
SYSTEM_EXPORT void ClearHoldQueue();

template <class TResult>
class CHoldedPtrFuncBase: public CPtrFuncBase<TResult>
{
	int nDeleteFrame;
protected:
	void Touch()
	{
		if ( nDGCurrentFrame > nDeleteFrame )
			SetToHoldQueue( this, &nDeleteFrame ); 
	}
	virtual bool NeedUpdate() { Touch(); return false; }
public:
	CHoldedPtrFuncBase(): nDeleteFrame(-1) {}
};

template <class TResult>
class CHoldedFuncBase: public CFuncBase<TResult>
{
	int nDeleteFrame;
protected:
	void Touch()
	{
		if ( nDGCurrentFrame > nDeleteFrame - 20 )
			SetToHoldQueue( this, &nDeleteFrame ); 
	}
	virtual bool NeedUpdate() { Touch(); return false; }
public:
	CHoldedFuncBase(): nDeleteFrame(-1) {}
};

#define DEFINE_DG_CONSTANT_NODE( ClassName, Type ) \
class ClassName: public CFuncBase< Type >          \
{                                                  \
	OBJECT_BASIC_METHODS(ClassName);                 \
public:                                            \
	ClassName() {}                                   \
	ClassName( const Type &_t ): CFuncBase< Type >(_t) { Updated(); }\
	const Type& GetValue() const { return value; }\
	void Set( const Type &_t ) { value = _t; Updated(); }\
	int operator&( IBinSaver &f ) { f.Add( 1, &value ); return 0; }\
};

// for use in CDGPtr only!
class CAccessForDGPtr
{
public:
	int GetFrameCalced( CVersioningBase *pNode ) const { return pNode->nFrameCalced; }
	void DoUpdate( CVersioningBase *pNode ) const { pNode->DoUpdate(); }
	int GetVersion( CVersioningBase *pNode ) const { return pNode->nVersion; }
};

SYSTEM_EXPORT void MarkNewDGFrame();

template <class TFunc, typename CPtrType = CObj<TFunc> >
class CDGPtr
{
	CPtrType pNode;
	int nVersion;
public:
	CDGPtr() { nVersion = 0; }
	CDGPtr( const CDGPtr<TFunc,CPtrType> &a ): pNode( a.pNode ) { nVersion = 0; }
	CDGPtr( TFunc *_pNode ): pNode(_pNode) { nVersion = 0; }
	CDGPtr& operator=( const CDGPtr<TFunc,CPtrType> &a ) { pNode = a.pNode; nVersion = 0; return *this; }
	CDGPtr& operator=( TFunc *_pNode ) { pNode = _pNode; nVersion = 0; return *this; }
	//
	operator TFunc*() const { return pNode; }
	TFunc* operator->() const { return pNode; }
	TFunc* Get() const { return pNode; }
	TFunc* GetPtr() const { return pNode; }
	TFunc* Extract() { nVersion = 0; return pNode.Extract(); }
	bool Refresh()
	{
		ASSERT( IsValid( pNode ) );
		CAccessForDGPtr cppIsWeak;
		if ( cppIsWeak.GetFrameCalced( pNode ) != nDGCurrentFrame )
			cppIsWeak.DoUpdate( pNode );
		bool bResult = nVersion != cppIsWeak.GetVersion( pNode );
		nVersion = cppIsWeak.GetVersion( pNode );
		return bResult;
	}
	void Sync( const CDGPtr &a ) { pNode = a.pNode; nVersion = a.nVersion; }
	int operator&( IBinSaver &f ) { f.DoPtr( &pNode ); return 0; }
};
template <class T, typename TPtrType>
inline bool IsValid( const CDGPtr<T,TPtrType> &p ) { return IsValid( p.Get() ); }

enum EDGNodeChange
{
	DG_CHANGE_UNKNOWN,
	DG_CHANGE_NONE,
	DG_CHANGE_CHANGED
};
class CChangeTrackPtr
{
	CPtr<CVersioningBase> p;
	int nVersion;
	int nFrame;
public:
	CChangeTrackPtr() : nVersion(0), nFrame(0) {}
	CChangeTrackPtr( const CChangeTrackPtr &a ) : p(a.p), nVersion(0), nFrame(0) {}
	CChangeTrackPtr( CVersioningBase *_p ) : p(_p), nVersion(0), nFrame(0) {}
	CChangeTrackPtr& operator=( const CChangeTrackPtr &a ) { p = a.p; nVersion = 0; nFrame = 0; return *this; }
	CChangeTrackPtr& operator=( CVersioningBase *_p ) { p = _p; nVersion = 0; nFrame = 0; return *this; }

	EDGNodeChange GetChanges()
	{
		if ( !IsValid(p) )
			return DG_CHANGE_UNKNOWN;
		CAccessForDGPtr cppIsWeak;
		int nNodeFrame = cppIsWeak.GetFrameCalced( p );
		int nNodeVersion = cppIsWeak.GetVersion( p );
		EDGNodeChange rv;
		if ( nFrame == nNodeFrame )
			rv = DG_CHANGE_UNKNOWN;
		else
			rv = nNodeVersion == nVersion ? DG_CHANGE_NONE : DG_CHANGE_CHANGED;
		nFrame = nNodeFrame;
		nVersion = nNodeVersion;
		return rv;
	}
	int operator&( IBinSaver &f ) { f.DoPtr( &p ); return 0; }
};

template <class TSet, class TParam>
inline void UpdateSet( TSet *a, TParam *p )
{
	for ( typename TSet::iterator i = a->begin(); i != a->end(); )
	{
		if ( IsValid( *i ) && (*i)->Update( p ) )
			++i;
		else
		{
			typename TSet::iterator k = i;
			++i;
			a->erase( k );
		}
	}
}

template <class TSet, class TParam, class TParam2>
inline void UpdateSet( TSet *a, TParam *p, TParam2 *p2 )
{
	for ( typename TSet::iterator i = a->begin(); i != a->end(); )
	{
		if ( IsValid( *i ) && (*i)->Update( p, p2 ) )
			++i;
		else
		{
			typename TSet::iterator k = i;
			++i;
			a->erase( k );
		}
	}
}

template <class TSet, class TParam, class TParam2, class TParam3>
inline void UpdateSet( TSet *a, TParam *p, TParam2 *p2, TParam3 *p3 )
{
	for ( typename TSet::iterator i = a->begin(); i != a->end(); )
	{
		if ( IsValid( *i ) && (*i)->Update( p, p2, p3 ) )
			++i;
		else
		{
			typename TSet::iterator k = i;
			++i;
			a->erase( k );
		}
	}
}

