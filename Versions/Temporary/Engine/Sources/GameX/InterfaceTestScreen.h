#pragma once

#include "InterfaceScreenBase.h"

// Note: for test purpose only

class CInterfaceTestScreen : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_BASIC_METHODS( CInterfaceTestScreen );

	ZDATA_( CInterfaceScreenBase )
	CDBID dbid;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CInterfaceScreenBase *)this );
		f.Add( 2, &dbid );
		return 0;
	}

protected:
	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction ) override
	{
		return false;
	}

	int Check( const std::string &szCheckName ) const override
	{
		return 0;
	}

	//}
	//{
	bool StepLocal( bool bAppActive ) override;
	//}
public:
	CInterfaceTestScreen();

	bool Init() override;

	void SetScreen( const CDBID &dbid );
};

class CICTestScreen : public CInterfaceCommandBase<CInterfaceTestScreen>
{
	OBJECT_BASIC_METHODS( CICTestScreen );

	ZDATA_( CInterfaceCommandBase<CInterfaceTestScreen> )
	CDBID dbid;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CInterfaceCommandBase<CInterfaceTestScreen> *)this );
		f.Add( 2, &dbid );
		return 0;
	}

	void PreCreate() override;
	void PostCreate( IInterface *pInterface ) override;

public:
	void Configure( const char *pszConfig ) override;
};
