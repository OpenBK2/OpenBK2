
#pragma once

#include "InterfaceMPBase.h"

class CInterfaceMultiplayer : public CInterfaceScreenBase, 
	public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceMultiplayer );

	ZDATA_(CInterfaceScreenBase)
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); return 0; }
private:
	void RegisterObservers();

	//{
	bool OnBack( const string &szSender );
	bool OnLAN( const string &szSender );
	bool OnNivalNet( const string &szSender );
	bool OnLoadReplay( const string &szSender );
	bool OnProfileManagerMenu();
	//}
protected:
	~CInterfaceMultiplayer();
public:
	CInterfaceMultiplayer();

	bool Init();
	void OnGetFocus( bool bFocus );
	bool ProcessEvent( const SGameMessage &msg );
	void AfterLoad();
	bool StepLocal( bool bAppActive );

	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;
	//}
};

#ifndef _SINGLE_DEMO
class CICMultiplayer : public CInterfaceCommandBase<CInterfaceMultiplayer>
{
	OBJECT_BASIC_METHODS( CICMultiplayer );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};
#endif // _SINGLE_DEMO


