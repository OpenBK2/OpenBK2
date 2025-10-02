#pragma once

#include "InterfaceScreenBase.h"

// Note: for test purpose only

class CInterfaceTestScreen : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_BASIC_METHODS( CInterfaceTestScreen );
	
	ZDATA_( CInterfaceScreenBase )
	CDBID dbid;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CInterfaceScreenBase *)this); f.Add(2,&dbid); return 0; }
protected:
	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction ) { return false; }
	int Check( const string &szCheckName ) const { return 0; }
	//}
	//{
	bool StepLocal( bool bAppActive );
	//}
public:
	CInterfaceTestScreen();

	bool Init();
	
	void SetScreen( const CDBID &dbid );
};

class CICTestScreen : public CInterfaceCommandBase<CInterfaceTestScreen>
{
	OBJECT_BASIC_METHODS( CICTestScreen );
	
	ZDATA_(CInterfaceCommandBase<CInterfaceTestScreen>)
	CDBID dbid;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceCommandBase<CInterfaceTestScreen>*)this); f.Add(2,&dbid); return 0; }

	void PreCreate( );
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};

