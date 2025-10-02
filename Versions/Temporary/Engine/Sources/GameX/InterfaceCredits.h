#pragma once

#include "InterfaceScreenBase.h"



class CInterfaceCredits : public CInterfaceScreenBase
{
	OBJECT_NOCOPY_METHODS( CInterfaceCredits );

public:
	class CReactions : public IProgrammedReactionsAndChecks
	{
		OBJECT_NOCOPY_METHODS(CReactions);
		CPtr<IWindow> pScreen;
		CPtr<CInterfaceCredits> pInterface;
	public:
		CReactions() { }
		~CReactions()	{ }
		CReactions( IWindow *_pScreen, CInterfaceCredits *_pInterface ) 
			: pScreen( _pScreen ), pInterface( _pInterface ) { }
			bool Execute( const string &szSender, const string &szReaction );
			int Check( const string &szCheckName ) const;

			int operator&( IBinSaver &saver )
			{
				saver.Add( 1, &pScreen );
				saver.Add( 2, &pInterface );
				return 0;
			}
	};

	~CInterfaceCredits();
private:
	ZDATA_(CInterfaceScreenBase)
	CObj<CReactions> pReactions;
	CPtr<IWindow> pText;
	NTimer::STime timeLastMove;
	bool bClosed;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pReactions); f.Add(3,&pText); f.Add(4,&timeLastMove); f.Add(5,&bClosed); return 0; }


public:
	CInterfaceCredits() : CInterfaceScreenBase( "CreditsScreen", "credits_screen" ), bClosed( false ) {  }

	bool Init();
	bool StepLocal( bool bAppActive );

	bool ProcessEvent( const SGameMessage &msg );
};

class CICInterfaceCredist : public CInterfaceCommandBase<CInterfaceCredits>
{
	OBJECT_BASIC_METHODS( CICInterfaceCredist );
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};

