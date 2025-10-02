#pragma once

#include "ui/commandparam.h"
#include "ui/dbuserinterface.h"
#include "UI/UI.h"


template <typename TObj>
class CReactionObserver : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CReactionObserver )
		void (TObj::*pfnMemFun)(const string &);
public:
	CReactionObserver() {  }
	CReactionObserver( void (TObj::*_pfnMemFun)(const string &) ) 
		: pfnMemFun( _pfnMemFun ) {}
		void Execute( CObjectBase * pThis, const string &szSender ) 
		{ 
			if ( pThis )
				( CastToUserObject( pThis ,(TObj*)0) ->*pfnMemFun )( szSender ); 
		}
};

template <typename TObj>
class CCheckObserver : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CCheckObserver )
		int (TObj::*pfnMemFun)()const;
	
public:
	CCheckObserver() {  }
	CCheckObserver( int (TObj::*_pfnMemFun)()const ) 
		: pfnMemFun( _pfnMemFun ) {}
		int Execute( const CObjectBase * pThis ) const
		{ 
			if ( pThis )
				return ( CastToUserObject( const_cast<CObjectBase*>(pThis) ,(TObj*)0) ->*pfnMemFun )(); 
			return 0;
		}
};

template <typename TObj>
class CProgrammedReactionsAndChecks : public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CProgrammedReactionsAndChecks )

	typedef hash_map<string, CPtr<CReactionObserver<TObj> > > CReactions;
	CReactions reactions;
	typedef hash_map<string, CPtr<CCheckObserver<TObj> > > CChecks;
	CChecks checks;
public:
	void SetInterface( TObj *_pInterface ) 
	{
		pInterface = _pInterface;
	}
	void AddCheck( const string &szCheck, int (TObj::*_pfnMemFun)()const )
	{
		checks[szCheck] = new CCheckObserver<TObj>( _pfnMemFun );
	}
	void AddReaction( const string &szReaction, void (TObj::*_pfnMemFun)(const string &) )
	{
		reactions[szReaction] = new CReactionObserver<TObj>( _pfnMemFun );
	}
	int Check( const string &szCheckName ) const
	{
		CChecks::const_iterator pos = checks.find( szCheckName );
		if ( pos != checks.end() )
			return pos->second->Execute( this );
		return 0;
	}
	bool Execute( const string &szSender, const string &szReaction )
	{
		CReactions::iterator pos = reactions.find( szReaction );
		if ( pos != reactions.end() )
		{
			pos->second->Execute( this, szSender );
			return true;
		}
		return false;
	}
};


