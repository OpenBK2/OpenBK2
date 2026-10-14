#pragma once

#include "InterfaceScreenBase.h"

class CInterfaceCredits : public CInterfaceScreenBase
{
	OBJECT_NOCOPY_METHODS( CInterfaceCredits );

public:
	class CReactions : public IProgrammedReactionsAndChecks
	{
		OBJECT_NOCOPY_METHODS( CReactions );
		CPtr<IWindow> pScreen;
		CPtr<CInterfaceCredits> pInterface;

	public:
		CReactions() {}

		~CReactions() override {}

		CReactions( IWindow *_pScreen, CInterfaceCredits *_pInterface )
			: pScreen( _pScreen )
			, pInterface( _pInterface )
		{
		}

		bool Execute( const std::string &szSender, const std::string &szReaction ) override;
		int Check( const std::string &szCheckName ) const override;

		int operator&( IBinSaver &saver ) override
		{
			saver.Add( 1, &pScreen );
			saver.Add( 2, &pInterface );
			return 0;
		}
	};

	~CInterfaceCredits() override;

private:
	ZDATA_( CInterfaceScreenBase )
	CObj<CReactions> pReactions;
	CPtr<IWindow> pText;
	NTimer::STime timeLastMove;
	bool bClosed;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CInterfaceScreenBase *)this );
		f.Add( 2, &pReactions );
		f.Add( 3, &pText );
		f.Add( 4, &timeLastMove );
		f.Add( 5, &bClosed );
		return 0;
	}

public:
	CInterfaceCredits()
		: CInterfaceScreenBase( "CreditsScreen", "credits_screen" )
		, bClosed( false )
	{
	}

	bool Init() override;
	bool StepLocal( bool bAppActive ) override;

	bool ProcessEvent( const SGameMessage &msg ) override;
};

class CICInterfaceCredist : public CInterfaceCommandBase<CInterfaceCredits>
{
	OBJECT_BASIC_METHODS( CICInterfaceCredist );
	void PreCreate() override;
	void PostCreate( IInterface *pInterface ) override;

public:
	void Configure( const char *pszConfig ) override;
};
