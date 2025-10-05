#pragma once

#include "Bind.h"


// basic interface for GameMessage observer
struct IGMObserver : public CObjectBase
{
	virtual bool Execute( const SGameMessage &msg, CObjectBase *pThis ) { return false; }
};

// member-function game message observer returning bool
template <typename TObj, typename TMsg>
class CGMMemFunBoolObserver : public IGMObserver
{
	OBJECT_NOCOPY_METHODS( CGMMemFunBoolObserver );
	bool (TObj::*pfnMemFun)( const TMsg &msg );
public:
	CGMMemFunBoolObserver() {  }
	CGMMemFunBoolObserver( bool (TObj::*_pfnMemFun)( const TMsg &_msg ) ) 
		: pfnMemFun( _pfnMemFun ) {}
	//
	bool Execute( const SGameMessage &_msg, CObjectBase *pThis ) 
	{ 
		if ( pThis )
			return ( CastToUserObject( pThis ,(TObj*)0) ->*pfnMemFun )( _msg ); 
		return false;
	}
};
template <typename TObj, typename TMsg, typename T1>
class CGMMemFunBoolObserver1 : public IGMObserver
{
	OBJECT_NOCOPY_METHODS( CGMMemFunBoolObserver1 );
	bool (TObj::*pfnMemFun)( const TMsg &msg, T1 p1 );
	T1 p1;
public:
	CGMMemFunBoolObserver1() {}
	CGMMemFunBoolObserver1( bool (TObj::*_pfnMemFun)( const TMsg &_msg, T1 _p1 ), T1 _p1 ) 
		: pfnMemFun( _pfnMemFun ), p1( _p1 ) {}
	//
	bool Execute( const SGameMessage &_msg, CObjectBase *pThis )
	{ 
		if ( pThis )
			return ( CastToUserObject( pThis ,(TObj*)0) ->*pfnMemFun )( _msg, p1 ); 
		return false;
	}
};
template <typename TObj, typename TMsg, typename T1, typename T2>
class CGMMemFunBoolObserver2 : public IGMObserver
{
	OBJECT_NOCOPY_METHODS( CGMMemFunBoolObserver2 );
	bool (TObj::*pfnMemFun)( const TMsg &_msg, T1 _p1, T2 _p2 );
	T1 p1;
	T2 p2;
public:
	CGMMemFunBoolObserver2() {}
	CGMMemFunBoolObserver2( void (TObj::*_pfnMemFun)( const TMsg &_msg, T1 _p1, T2 _p2 ), T1 _p1, T2 _p2 ) 
		: pfnMemFun( _pfnMemFun ), p1( _p1 ), p2( _p2 ) {}
	//
	bool Execute( const SGameMessage &_msg, CObjectBase *pThis )
	{ 
		if ( pThis )
			return ( CastToUserObject( pThis ,(TObj*)0) ->*pfnMemFun )( _msg, p1, p2 ); 
		return false;
	}
};

// member-function game message observer
template <typename TObj, typename TMsg>
class CGMMemFunObserver : public IGMObserver
{
	OBJECT_NOCOPY_METHODS( CGMMemFunObserver );
	void (TObj::*pfnMemFun)( const TMsg &msg );
public:
	CGMMemFunObserver() {  }
	CGMMemFunObserver( void (TObj::*_pfnMemFun)( const TMsg &msg ) ) : pfnMemFun( _pfnMemFun ) {}
	//
	bool Execute( const SGameMessage &msg, CObjectBase *pThis ) 
	{ 
		if ( pThis )
		{
			( CastToUserObject( pThis ,(TObj*)0) ->*pfnMemFun )( msg ); 
			return true;
		}
		return false;
	}
};
template <typename TObj, typename TMsg, typename T1>
class CGMMemFunObserver1 : public IGMObserver
{
	OBJECT_NOCOPY_METHODS( CGMMemFunObserver1 );
	void (TObj::*pfnMemFun)( const TMsg &_msg, T1 _p1 );
	T1 p1;
public:
	CGMMemFunObserver1() {}
	CGMMemFunObserver1( void (TObj::*_pfnMemFun)( const TMsg &_msg, T1 _p1 ), T1 _p1 ) 
		: pfnMemFun( _pfnMemFun ), p1( _p1 ) {}
	//
	bool Execute( const SGameMessage &_msg, CObjectBase *pThis )
	{ 
		if ( pThis )
		{
			( CastToUserObject( pThis ,(TObj*)0) ->*pfnMemFun )( _msg, p1 ); 
			return true;
		}
		return false;
	}
};
template <typename TObj, typename TMsg, typename T1, typename T2>
class CGMMemFunObserver2 : public IGMObserver
{
	OBJECT_NOCOPY_METHODS( CGMMemFunObserver2 );
	void (TObj::*pfnMemFun)( const TMsg &_msg, T1 _p1, T2 _p2 );
	T1 p1;
	T2 p2;
public:
	CGMMemFunObserver2() {}
	CGMMemFunObserver2( void (TObj::*_pfnMemFun)( const TMsg &_msg, T1 _p1, T2 _p2 ), T1 _p1, T2 _p2 ) 
		: pfnMemFun( _pfnMemFun ), p1( _p1 ), p2( _p2 ) {}
	//
	bool Execute( const SGameMessage &_msg, CObjectBase *pThis )
	{ 
		if ( pThis )
		{
			( CastToUserObject( pThis ,(TObj*)0) ->*pfnMemFun )( _msg, p1, p2 ); 
			return true;
		}
		return false;
	}
};

// bool raw function message observers
template <typename TMsg>
class CGMFunBoolObserver : public IGMObserver
{
	OBJECT_NOCOPY_METHODS( CGMFunBoolObserver );
	bool (*pfnFun)( const TMsg &_msg );
public:
	CGMFunBoolObserver() {  }
	CGMFunBoolObserver( bool (*_pfnFun)( const TMsg &_msg ) ) 
		: pfnFun( _pfnFun ) {}
	//
	bool Execute( const SGameMessage &_msg, CObjectBase *pThis ) { return (*pfnFun)( _msg ); }
};
template <typename TMsg, typename T1>
class CGMFunBoolObserver1 : public IGMObserver
{
	OBJECT_NOCOPY_METHODS( CGMFunBoolObserver1 );
	bool (*pfnFun)( const TMsg &_msg, T1 _p1 );
	T1 p1;
public:
	CGMFunBoolObserver1() {  }
	CGMFunBoolObserver1( bool (*_pfnFun)( const TMsg &_msg, T1 _p1 ), T1 _p1 ) 
		: pfnFun( _pfnFun ), p1( _p1 ) {}
	//
	bool Execute( const SGameMessage &_msg, CObjectBase *pThis ) { return (*pfnFun)( _msg, p1 ); }
};
template <typename TMsg, typename T1, typename T2>
class CGMFunBoolObserver2 : public IGMObserver
{
	OBJECT_NOCOPY_METHODS( CGMFunBoolObserver2 );
	bool (*pfnFun)( const TMsg &_msg, T1 _p1, T2 _p2 );
	T1 p1;
	T2 p2;
public:
	CGMFunBoolObserver2() {  }
	CGMFunBoolObserver2( bool (*pfnFun)( const TMsg &_msg, T1 _p1, T2 _p2 ), T1 _p1, T2 _p2 ) 
		: pfnFun( _pfnFun ), p1( _p1 ), p2( _p2 ) {}
	//
	bool Execute( const SGameMessage &_msg, CObjectBase *pThis ) { return (*pfnFun)( _msg, p1, p2 ); }
};

// raw function game message observer
template <typename TMsg>
class CGMFunObserver : public IGMObserver
{
	OBJECT_NOCOPY_METHODS( CGMFunObserver );
	void (*pfnFun)( const TMsg &_msg );
public:
	CGMFunObserver() {  }
	CGMFunObserver( void (*_pfnFun)( const TMsg &_msg ) ) 
		: pfnFun( _pfnFun ) {}
	//
	bool Execute( const SGameMessage &_msg, CObjectBase *pThis ) { (*pfnFun)( _msg ); return true; }
};
template <typename TMsg, typename T1>
class CGMFunObserver1 : public IGMObserver
{
	OBJECT_NOCOPY_METHODS( CGMFunObserver1 );
	void (*pfnFun)( const TMsg &_msg, T1 _p1 );
	T1 p1;
public:
	CGMFunObserver1() {  }
	CGMFunObserver1( void (*_pfnFun)( const TMsg &_msg, T1 _p1 ), T1 _p1 ) 
		: pfnFun( _pfnFun ), p1( _p1 ) {}
	//
	bool Execute( const SGameMessage &_msg, CObjectBase *pThis ) { (*pfnFun)( _msg, p1 ); return true; }
};
template <typename TMsg, typename T1, typename T2>
class CGMFunObserver2 : public IGMObserver
{
	OBJECT_NOCOPY_METHODS( CGMFunObserver2 );
	void (*pfnFun)( const TMsg &_msg, T1 _p1, T2 _p2 );
	T1 p1;
	T2 p2;
public:
	CGMFunObserver2() {  }
	CGMFunObserver2( void (*pfnFun)( const TMsg &_msg, T1 _p1, T2 _p2 ), T1 _p1, T2 _p2 ) 
		: pfnFun( _pfnFun ), p1( _p1 ), p2( _p2 ) {}
	//
	bool Execute( const SGameMessage &_msg, CObjectBase *pThis ) { (*pfnFun)( _msg, p1, p2 ); return true; }
};

// game message observer registrator helper class
namespace NInput
{
class CGMOReg
{
	CBind bind;
	CObj<IGMObserver> pObserver;
public:
	CGMOReg( const std::string &szEventName, IGMObserver *_pObserver ) : bind( szEventName ), pObserver(_pObserver) {}
	bool ProcessEvent( const SGameMessage &event, CObjectBase *pThis )
	{
		if ( bind.ProcessEvent( event ) )
			return pObserver->Execute( event, pThis );
		return false;
	}
};

class CGMORegContainer
{
protected:
	std::vector<NInput::CGMOReg> eventRegisters;
public:
	// void member function observer registration
	template <typename TObj, typename TMsg>
		void AddObserver( const std::string &szMsgName, void (TObj::*_pfnMemFun)( const TMsg &_msg ) )
	{
		CPtr<IGMObserver> pObserver = new CGMMemFunObserver<TObj, TMsg>( _pfnMemFun );
		AddRawObserver( szMsgName, pObserver );
	}
	template <typename TObj, typename TMsg, class T1>
		void AddObserver( const std::string &szMsgName, void (TObj::*_pfnMemFun)( const TMsg &_msg, T1 _p1 ), T1 _p1 )
	{
		CPtr<IGMObserver> pObserver = new CGMMemFunObserver1<TObj, TMsg, T1>( _pfnMemFun, _p1 );
		AddRawObserver( szMsgName, pObserver );
	}
	template <typename TObj, typename TMsg, class T1, class T2>
		void AddObserver( const std::string &szMsgName, void (TObj::*_pfnMemFun)( const TMsg &_msg, T1 _p1, T1 _p2 ), T1 _p1, T2 _p2 )
	{
		CPtr<IGMObserver> pObserver = new CGMMemFunObserver2<TObj, TMsg, T1, T2>( _pfnMemFun, _p1, _p2 );
		AddRawObserver( szMsgName, pObserver );
	}
	// bool member function observer registration
	template <typename TObj, typename TMsg>
		void AddObserver( const std::string &szMsgName, bool (TObj::*_pfnMemFun)( const TMsg &_msg ) )
	{
		CPtr<IGMObserver> pObserver = new CGMMemFunBoolObserver<TObj, TMsg>( _pfnMemFun );
		AddRawObserver( szMsgName, pObserver );
	}
	template <typename TObj, typename TMsg, class T1>
		void AddObserver( const std::string &szMsgName, bool (TObj::*_pfnMemFun)( const TMsg &_msg, T1 _p1 ), T1 _p1 )
	{
		CPtr<IGMObserver> pObserver = new CGMMemFunBoolObserver1<TObj, TMsg, T1>( _pfnMemFun, _p1 );
		AddRawObserver( szMsgName, pObserver );
	}
	template <typename TObj, typename TMsg, class T1, class T2>
		void AddObserver( const std::string &szMsgName, bool (TObj::*_pfnMemFun)( const TMsg &_msg, T1 _p1, T1 _p2 ), T1 _p1, T2 _p2 )
	{
		CPtr<IGMObserver> pObserver = new CGMMemFunBoolObserver2<TObj, TMsg, T1, T2>( _pfnMemFun, _p1, _p2 );
		AddRawObserver( szMsgName, pObserver );
	}
	// void global function observer registration
	template <typename TMsg>
		void AddObserver( const std::string &szMsgName, void (*_pfnFunc)( const TMsg &msg ) )
	{
		CPtr<IGMObserver> pObserver = new CGMFunObserver<TMsg>( _pfnFunc );
		AddRawObserver( szMsgName, pObserver );
	}
	template <typename TMsg, class T1>
		void AddObserver( const std::string &szMsgName, void (*_pfnFunc)( const TMsg &msg, T1 _p1 ), T1 _p1 )
	{
		CPtr<IGMObserver> pObserver = new CGMFunObserver1<TMsg, T1>( _pfnFunc, _p1 );
		AddRawObserver( szMsgName, pObserver );
	}
	template <typename TMsg, class T1, class T2>
		void AddObserver( const std::string &szMsgName, void (*_pfnFunc)( const TMsg &msg, T1 _p1, T2 _p2 ), T1 _p1, T2 _p2 )
	{
		CPtr<IGMObserver> pObserver = new CGMFunObserver2<TMsg, T1, T2>( _pfnFunc, _p1, _p2 );
		AddRawObserver( szMsgName, pObserver );
	}
	// bool global function observer registration
	template <typename TMsg>
		void AddObserver( const std::string &szMsgName, bool (*_pfnFunc)( const TMsg &msg ) )
	{
		CPtr<IGMObserver> pObserver = new CGMFunBoolObserver<TMsg>( _pfnFunc );
		AddRawObserver( szMsgName, pObserver );
	}
	template <typename TMsg, class T1>
		void AddObserver( const std::string &szMsgName, bool (*_pfnFunc)( const TMsg &msg, T1 _p1 ), T1 _p1 )
	{
		CPtr<IGMObserver> pObserver = new CGMFunBoolObserver1<TMsg, T1>( _pfnFunc, _p1 );
		AddRawObserver( szMsgName, pObserver );
	}
	template <typename TMsg, class T1, class T2>
		void AddObserver( const std::string &szMsgName, bool (*_pfnFunc)( const TMsg &msg, T1 _p1, T2 _p2 ), T1 _p1, T2 _p2 )
	{
		CPtr<IGMObserver> pObserver = new CGMFunBoolObserver2<TMsg, T1, T2>( _pfnFunc, _p1, _p2 );
		AddRawObserver( szMsgName, pObserver );
	}
	virtual void AddRawObserver( const std::string &szMsgName, IGMObserver *pObserver )
	{
		eventRegisters.insert( eventRegisters.begin(), NInput::CGMOReg(szMsgName, pObserver) );
	}
	bool ProcessEvent( const SGameMessage &event, CObjectBase *pThis )
	{
		for ( int k = 0; k < eventRegisters.size(); ++k )
		{
			if ( eventRegisters[k].ProcessEvent( event, pThis ) )
				return true;
		}
		return false;
	}
	bool IsEmpty() const { return eventRegisters.empty(); }
};
}

// ************************************************************************************************************************ //
// **
// ** coords packing
// **
// **
// **
// ************************************************************************************************************************ //

inline bool IsPacked2DCoords( int dwPacked )
{
	return ( dwPacked & 0xC0000000 )  == 0x40000000;
}

inline int PackCoords( const CVec2 &vPos )
{
	const int x = Float2Int( vPos.x );
	const int y = Float2Int( vPos.y );
	return ( ( x & 0x7FFF ) | ( (y & 0x7FFF) << 15 ) ) | 0x40000000;
}

inline CVec2 UnPackCoords( int _nPacked )
{
	unsigned int dwPacked = _nPacked;
	return CVec2( dwPacked & 0x7FFF, (dwPacked >> 15) & 0x7FFF );
}


