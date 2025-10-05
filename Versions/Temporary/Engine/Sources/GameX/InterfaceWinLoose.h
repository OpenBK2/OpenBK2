#pragma once

#include "InterfaceScreenBase.h"

class CInterfaceWinLoose : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceWinLoose );
private:
	enum EUIState
	{
		EUIS_NORMAL,
		EUIS_END_GAME_ROTATE_CAMERA,
		EUIS_END_GAME_FADE_SCREEN,
		EUIS_END_GAME_STATISTICS,
	};
	
	ZDATA_(CInterfaceScreenBase)
	bool bWin;
	EUIState eUIState;
	float fEndGameRestTime;
	CPtr<IWindow> pMainWnd;
	CPtr<IWindow> pWinView;
	CPtr<IWindow> pLostView;
	int nSetYawSpeedStep;
	float fSetYawSpeedTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&bWin); f.Add(3,&eUIState); f.Add(4,&fEndGameRestTime); f.Add(5,&pMainWnd); f.Add(6,&pWinView); f.Add(7,&pLostView); f.Add(8,&nSetYawSpeedStep); f.Add(9,&fSetYawSpeedTime); return 0; }
	
	NTimer::STime timeAbsLast; // don't save
private:
	void MakeInterior();
	
	bool StepLocal( bool bAppActive );
	void InterfaceStateStep( float fTime );
	
	void NextScreen();

	void EndGameRotation();
	void EndGameFade();
	void EndGameStatistics();
	
	CVec3 GetAnchor() const;
public:
	CInterfaceWinLoose();

	bool Init();
	bool ProcessEvent( const SGameMessage &msg );

	void SetMode( bool bWin );

	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction );
	int Check( const std::string &szCheckName ) const;
	//}
};

class CICWinLooseDialog : public CInterfaceCommandBase<CInterfaceWinLoose>
{
	OBJECT_BASIC_METHODS( CICWinLooseDialog );
	
	ZDATA_(CInterfaceCommandBase<CInterfaceWinLoose>)
	bool bWin;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceCommandBase<CInterfaceWinLoose>*)this); f.Add(2,&bWin); return 0; }
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};


