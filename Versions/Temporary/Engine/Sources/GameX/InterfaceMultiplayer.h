
#pragma once

#include "InterfaceMPBase.h"

class CInterfaceMultiplayer : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceMultiplayer );

	ZDATA_( CInterfaceScreenBase )

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CInterfaceScreenBase *)this );
		return 0;
	}

private:
	void RegisterObservers();

	//{
	bool OnBack( const std::string &szSender );
	bool OnLAN( const std::string &szSender );
	bool OnNivalNet( const std::string &szSender );
	bool OnLoadReplay( const std::string &szSender );
	bool OnProfileManagerMenu();
	//}
protected:
	~CInterfaceMultiplayer() override;

public:
	CInterfaceMultiplayer();

	bool Init() override;
	void OnGetFocus( bool bFocus ) override;
	bool ProcessEvent( const SGameMessage &msg ) override;
	void AfterLoad() override;
	bool StepLocal( bool bAppActive ) override;

	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction ) override;
	int Check( const std::string &szCheckName ) const override;
	//}
};

#ifndef _SINGLE_DEMO
class CICMultiplayer : public CInterfaceCommandBase<CInterfaceMultiplayer>
{
	OBJECT_BASIC_METHODS( CICMultiplayer );
	//
	void PreCreate() override;
	void PostCreate( IInterface *pInterface ) override;

public:
	void Configure( const char *pszConfig ) override;
};
#endif // _SINGLE_DEMO
