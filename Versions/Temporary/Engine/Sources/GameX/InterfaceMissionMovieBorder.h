
#pragma once

#include "InterfaceScreenBase.h"

class CInterfaceMissionMovieBorder : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceMissionMovieBorder );

private:
	void RegisterObservers();

protected:
	~CInterfaceMissionMovieBorder() override;

public:
	CInterfaceMissionMovieBorder();

	bool Init() override;
	void OnGetFocus( bool bFocus ) override;
	bool StepLocal( bool bAppActive ) override;
	bool ProcessEvent( const SGameMessage &msg ) override;
	void AfterLoad() override;

	bool IsModal() override
	{
		return false;
	}

	void MsgQuickSave( const SGameMessage &msg );
	void MsgQuickLoad( const SGameMessage &msg );

	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction ) override;
	int Check( const std::string &szCheckName ) const override;
	//}
};

class CICMissionMovieBorder : public CInterfaceCommandBase<CInterfaceMissionMovieBorder>
{
	OBJECT_BASIC_METHODS( CICMissionMovieBorder );

	ZDATA_( CInterfaceCommandBase<CInterfaceMissionMovieBorder> )

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CInterfaceCommandBase<CInterfaceMissionMovieBorder> *)this );
		return 0;
	}

	//
	void PreCreate() override;
	void PostCreate( IInterface *pInterface ) override;

public:
	CICMissionMovieBorder();

	void Configure( const char *pszConfig ) override;
};
