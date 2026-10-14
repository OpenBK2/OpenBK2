
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
	std::vector<SRange> ranges;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &ranges );
		return 0;
	}

protected:
	// returns a position in the whole progress
	float GetProgress() const;
	// returns progress in the given range
	int GetProgress( int nMin, int nMax ) const;

public:
	CProgressHookHelper();

	//{ IProgressHook
	void SetNumSteps( const int nNumSteps ) override;
	void LockRange( const int nLength ) override;
	void UnlockRange() override;
	void Step() override;
	void SetCurrentStep( int nCurrentStep ) override;
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
	~CInterfaceLoadingBase() override;

	void InitInternal( const std::string &szScreenName );

	void MakeInterior();
	void UpdateInterior();
	void Draw();

	bool IsInitialized() const
	{
		return bInitialized;
	}

	IScreen *GetScreen() const
	{
		return pScreen;
	}

	void PlayOnEnterTransitEffect();

	bool IsEffectFinished() const;
	void ShowFirstElement();

public:
	CInterfaceLoadingBase();

	//{ IProgressHook
	void SetNumSteps( const int nNumSteps ) override;
	void LockRange( const int nLength ) override;
	void UnlockRange() override;
	void Step() override;
	void SetCurrentStep( int nCurrentStep ) override;

	// IProgressHookB2
	void RunFinishScreen() override {}

	//}

	int operator&( IBinSaver &saver ) override;
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
	~CInterfaceLoadingSingle2D() override;

	void MakeInterior( const NDb::STexture *pTexture,
	                   const std::wstring &wszDesc,
	                   const std::wstring &wszCitation,
	                   bool bChapter );
	void ShowOnEnterMoveEffect();

public:
	CInterfaceLoadingSingle2D();
	CInterfaceLoadingSingle2D( const NDb::STexture *pTexture,
	                           const std::wstring &wszDesc,
	                           const std::wstring &wszCitation,
	                           bool bChapter );

	//{ IProgressHookB2
	void RunFinishScreen() override;
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

	ZDATA_( CInterfaceScreenBase )
	EUIState eUIState;
	CPtr<ITextView> pPressKeyView;
	std::wstring wszPressKeyFormat1;
	std::wstring wszPressKeyFormat2;
	bool bFormat1;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CInterfaceScreenBase *)this );
		f.Add( 2, &eUIState );
		f.Add( 3, &pPressKeyView );
		f.Add( 4, &wszPressKeyFormat1 );
		f.Add( 5, &wszPressKeyFormat2 );
		f.Add( 6, &bFormat1 );
		return 0;
	}

	NTimer::STime timeAbs; // don't save
private:
	bool ProcessEvent( const struct SGameMessage &msg ) override;

	void MoveOutInfoPanel();
	void StartTransitToMission();
	void StartTransitToMissionDone();
	void ShowPressKey( bool bFormat1 );

public:
	CInterfaceLoadingSingleFinished();

	bool Init() override;

	void Draw( NGScene::CRTPtr *pTarget ) override;

	bool StepLocal( bool bAppActive ) override;

	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction ) override;
	int Check( const std::string &szCheckName ) const override;

	//}

	bool IsModal() override
	{
		return false;
	}
};

class CICLoadingSingleFinished : public CInterfaceCommandBase<CInterfaceLoadingSingleFinished>
{
	OBJECT_BASIC_METHODS( CICLoadingSingleFinished );
	//
	void PreCreate() override;
	void PostCreate( IInterface *pInterface ) override;

public:
	void Configure( const char *pszConfig ) override;
};
