
#pragma once

#include "InterfaceScreenBase.h"

extern CObj<IWindow> g_pLoadingSingleScreen;

class CProgressHookHelper : public IProgressHookB2
{
	struct SRange
	{
		int nNumSteps;
		int nStep;
		int nLockedRange;
	};
	
	ZDATA
	vector<SRange> ranges;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&ranges); return 0; }
protected:
	// returns a position in the whole progress
	float GetProgress() const;
	// returns progress in the given range
	int GetProgress( int nMin, int nMax ) const;
public:
	CProgressHookHelper();
	
	//{ IProgressHook
	void SetNumSteps( const int nNumSteps );
	void LockRange( const int nLength );
	void UnlockRange();
	void Step();
	void SetCurrentStep( int nCurrentStep );
	//}
};

// Special "2D-interface" (not a descendant of CInterfaceScreenBase)
class CInterfaceLoadingBase : public CProgressHookHelper
{
public:
	class CReactions;
	class CLoadingScene;
private:

	bool bInitialized;
	CObj<IScreen> pScreen;
	CObj<IProgrammedReactionsAndChecks> pReactions;
	CObj<IProgressBar> pProgress;
	CObj<CLoadingScene> pLoadingScene;
	CPtr<IPlayer> pVideo;
	int nTotalFrames;
	int nCurrFrame;
	float fCurrProgress;

protected:
	~CInterfaceLoadingBase();

	void InitInternal( const string &szScreenName );

	void MakeInterior();
	void UpdateInterior();
	void Draw();
	
	bool IsInitialized() const { return bInitialized; }
	IScreen* GetScreen() const { return pScreen; }

	void PlayOnEnterTransitEffect();
	
	bool IsEffectFinished() const;
	void ShowFirstElement();
public:
	CInterfaceLoadingBase();
	
	//{ IProgressHook
	void SetNumSteps( const int nNumSteps );
	void LockRange( const int nLength );
	void UnlockRange();
	void Step();
	void SetCurrentStep( int nCurrentStep );

	// IProgressHookB2
	void RunFinishScreen() {}
	//}

	int operator&( IBinSaver &saver );
};

// Special "2D-interface" (not a descendant of CInterfaceScreenBase)
class CInterfaceLoadingSingle2D : public CInterfaceLoadingBase
{
	OBJECT_NOCOPY_METHODS( CInterfaceLoadingSingle2D );
private:
	CPtr<ITextView> pPressKeyView;
	CPtr<IWindow> pInfoPanel;
	CPtr<IWindow> pCitation;
protected:
	~CInterfaceLoadingSingle2D();

	void MakeInterior( const NDb::STexture *pTexture, const wstring &wszDesc, const wstring &wszCitation, bool bChapter );
	void ShowOnEnterMoveEffect();
public:
	CInterfaceLoadingSingle2D();
	CInterfaceLoadingSingle2D( const NDb::STexture *pTexture, const wstring &wszDesc, const wstring &wszCitation, bool bChapter );
	
	//{ IProgressHookB2
	void RunFinishScreen();
	//}
};

class CInterfaceLoadingSingleFinished : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceLoadingSingleFinished );
	
	enum EUIState
	{
		EUIS_NORMAL,
		EUIS_MOVE_OUT_INFO_PANEL,
		EUIS_START_TRANSIT_TO_MISSION,
		EUIS_START_TRANSIT_TO_MISSION_DONE,
	};

	ZDATA_(CInterfaceScreenBase)
	EUIState eUIState;
	CPtr<ITextView> pPressKeyView;
	wstring wszPressKeyFormat1;
	wstring wszPressKeyFormat2;
	bool bFormat1;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&eUIState); f.Add(3,&pPressKeyView); f.Add(4,&wszPressKeyFormat1); f.Add(5,&wszPressKeyFormat2); f.Add(6,&bFormat1); return 0; }
	
	NTimer::STime timeAbs; // don't save
private:
	bool ProcessEvent( const struct SGameMessage &msg );
	
	void MoveOutInfoPanel();
	void StartTransitToMission();
	void StartTransitToMissionDone();
	void ShowPressKey( bool bFormat1 );
public:
	CInterfaceLoadingSingleFinished();

	bool Init();
	
	void Draw( NGScene::CRTPtr *pTarget );

	bool StepLocal( bool bAppActive );

	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;
	//}
	
	bool IsModal() { return false; }
};

class CICLoadingSingleFinished : public CInterfaceCommandBase<CInterfaceLoadingSingleFinished>
{
	OBJECT_BASIC_METHODS( CICLoadingSingleFinished );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};


