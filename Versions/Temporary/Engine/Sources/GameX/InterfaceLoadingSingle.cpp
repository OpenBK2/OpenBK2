#include "stdafx.h"
#include "InterfaceLoadingSingle.h"
#include "GameXClassIDs.h"
#include "UI/SceneClassIDs.h"
#include "SceneB2/Scene.h"
#include "InterfaceState.h"
#include "3Dmotor/GRTShare.h"
#include "SceneB2/Cursor.h"
#include "SceneB2/SceneUIVisitor.h"
#include "SaveLoadHelper.h"
#include "3Dmotor/FrameTransition.h"
#include "System/Commands.h"
#include "UISpecificB2/EffectorB2Move.h"

const float MIN_PROGRESS_DELTA = 0.001f;

static int s_nTransitionEffectToMissionDuration = 400;
static float s_fTransitionEffectToMissionLength = 0.0f; //0.1f;

static float s_fBlinkTime = 1.0f; 

CObj<IWindow> g_pLoadingSingleScreen;

void FitDescription( IWindow *pMaxPanel, IWindow *pPanel, ITextView *pDesc )
{
	if ( !pMaxPanel || !pPanel || !pDesc )
		return;

	int nTextSizeY = pDesc->GetSize().y;
	int nDescSizeY;
	pDesc->GetPlacement( 0, 0, 0, &nDescSizeY );

	if ( nTextSizeY > nDescSizeY  )
	{
		int nX, nY, nSizeX, nSizeY;
		pPanel->GetPlacement( &nX, &nY, &nSizeX, &nSizeY );
		int nMaxX, nMaxY, nMaxSizeX, nMaxSizeY;
		pMaxPanel->GetPlacement( &nMaxX, &nMaxY, &nMaxSizeX, &nMaxSizeY );

		if ( nTextSizeY > nDescSizeY + nMaxSizeY - nSizeY )
		{
			pPanel->SetPlacement( nMaxX, 0, nMaxSizeX, 0, EWPF_POS_X | EWPF_SIZE_X );
			nTextSizeY = pDesc->GetSize().y;
		}

		if ( nTextSizeY > nDescSizeY  )
		{
			int nDeltaY = nTextSizeY - nDescSizeY;
			if ( nDeltaY > nMaxSizeY - nSizeY )
				nDeltaY = nMaxSizeY - nSizeY;

			pPanel->SetPlacement( 0, nY - nDeltaY / 2, 0, nSizeY + nDeltaY, EWPF_POS_Y | EWPF_SIZE_Y );
		}
	}
}

// CInterfaceLoadingBase::CLoadingScene

class CInterfaceLoadingBase::CLoadingScene : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CLoadingScene);

	CUIVisitor *pUIVisitor;
	CObj<NGScene::I2DGameView> p2DView;
	CPtr<IWindow> pScreen;
protected:
	~CLoadingScene();
public:
	CLoadingScene();
	
	void SetScreen( IWindow *pScreen );
	
	void Draw( NGfx::CTexture *pTexture = 0 );

	int operator&( IBinSaver &saver ) { NI_ASSERT( 0, "Can't be saved/loaded" ); return 0; }
};

// CInterfaceLoadingBase::CReactions

class CInterfaceLoadingBase::CReactions : public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS(CReactions);

	ZDATA
	CPtr<IWindow> pScreen;
	bool bIsEffectFinished;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pScreen); f.Add(3,&bIsEffectFinished); return 0; }
public:
	CReactions();
	CReactions( IWindow *pScreen );
	~CReactions(); 

	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction );
	int Check( const std::string &szCheckName ) const;
	//}
	
	bool IsEffectFinished() const { return bIsEffectFinished; }
};

// CInterfaceLoadingBase::CLoadingScene

CInterfaceLoadingBase::CLoadingScene::CLoadingScene()
{
	pUIVisitor = new CUIVisitor();
	p2DView = NGScene::CreateNew2DView();
	
	const CVec2 vSize = NGfx::GetScreenRect();
	Cursor()->SetBounds( 0, 0, vSize.x, vSize.y );

	pUIVisitor->SetGView( p2DView );
	Singleton<IUIInitialization>()->Set2DGameView( p2DView );
}

CInterfaceLoadingBase::CLoadingScene::~CLoadingScene()
{
	delete pUIVisitor;
	pUIVisitor = 0;
}

void CInterfaceLoadingBase::CLoadingScene::SetScreen( IWindow *_pScreen )
{
	pScreen = _pScreen;
}

void CInterfaceLoadingBase::CLoadingScene::Draw( NGfx::CTexture *pTexture )
{
	MarkNewDGFrame();
	NGfx::Is3DActive();
	p2DView->StartNewFrame( pTexture );
	pUIVisitor->SetGView( p2DView );
	if ( pScreen )
		pScreen->Visit( pUIVisitor );
	p2DView->Flush();
	if ( !pTexture )
		NGScene::Flip();
}

// CInterfaceLoadingBase::CReactions

CInterfaceLoadingBase::CReactions::CReactions() :
	bIsEffectFinished( false )
{
}

CInterfaceLoadingBase::CReactions::CReactions( IWindow *_pScreen ) : 
	pScreen( _pScreen ),
	bIsEffectFinished( false )
{
}

CInterfaceLoadingBase::CReactions::~CReactions() 
{
}

bool CInterfaceLoadingBase::CReactions::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "effect_finished" )
	{
		bIsEffectFinished = true;
		return true;
	}

	return false;
}

int CInterfaceLoadingBase::CReactions::Check( const std::string &szCheckName ) const
{
	return 0;
}

void CInterfaceLoadingBase::ShowFirstElement()
{
	if ( pProgress )
		pProgress->ShowFirstElement( true );
}

// CInterfaceLoadingBase

CInterfaceLoadingBase::CInterfaceLoadingBase() :
	nTotalFrames( 2 ),
	nCurrFrame( 1 ),
	fCurrProgress( 0.0f ),
	bInitialized( false )
{
}

CInterfaceLoadingBase::~CInterfaceLoadingBase()
{
	pLoadingScene = 0;
	pReactions = 0;
	pScreen = 0;
}

void CInterfaceLoadingBase::InitInternal( const std::string &szScreenName )
{
	// load screen
	pScreen = MakeObjectVirtual<IScreen>( UI_SCREEN );
	pReactions = new CReactions( pScreen );

	const CDBID dbidScreen = InterfaceState()->GetScreenEntryDBID( szScreenName );

	pScreen->SetGView( Scene()->GetG2DView() );
	pScreen->Load( NDb::Get<NDb::SWindowScreen>(dbidScreen), pReactions );
	
	pLoadingScene = new CLoadingScene();
	pLoadingScene->SetScreen( pScreen );

	bInitialized = true;
}

void CInterfaceLoadingBase::MakeInterior()
{
	IWindow *pMain = GetChildChecked<IWindow>( pScreen, "Main", true );

	pProgress = GetChildChecked<IProgressBar>( pMain, "Progress", true );
//	pVideo = GetChildChecked<IPlayer>( pMain, "Video", true );

	if ( pVideo && !pVideo->IsVisible() )
		pVideo = 0;
	
	if ( pProgress )
		pProgress->SetPosition( 0.0f );
	if ( pVideo )
	{
		pVideo->Play();
		nTotalFrames = (std::max)( 2, pVideo->GetNumFrames() );
		pVideo->SetCurrentFrame( 1 );
	}
}

void CInterfaceLoadingBase::UpdateInterior()
{
	if ( !bInitialized )
		return;
		
	bool bUpdated = false;
		
	float fProgress = GetProgress();
	
	if ( pProgress && (fProgress - fCurrProgress >= MIN_PROGRESS_DELTA) )
	{
		bUpdated = true;
		fCurrProgress = fProgress;
		pProgress->SetPosition( fProgress );
	}
	
	if ( pVideo )
	{
		int nNewFrame = ceilf( fProgress * (nTotalFrames - 1) + 0.5f ) + 1;
		if ( nNewFrame > nCurrFrame )
		{
			bUpdated = true;
			pVideo->Play();
			nCurrFrame = nNewFrame;
			pVideo->SetCurrentFrame( nCurrFrame );
		}
	}

	if ( bUpdated )
		Draw();
}

void CInterfaceLoadingBase::Draw()
{
	pLoadingScene->Draw();
}

void CInterfaceLoadingBase::SetNumSteps( const int nNumSteps )
{
	CProgressHookHelper::SetNumSteps( nNumSteps );
}

void CInterfaceLoadingBase::LockRange( const int nLength )
{
	CProgressHookHelper::LockRange( nLength );
}

void CInterfaceLoadingBase::UnlockRange()
{
	CProgressHookHelper::UnlockRange();

	UpdateInterior();
}

void CInterfaceLoadingBase::Step()
{
	CProgressHookHelper::Step();

	UpdateInterior();
}

void CInterfaceLoadingBase::SetCurrentStep( int nCurrentStep )
{
	CProgressHookHelper::SetCurrentStep( nCurrentStep );
	
	UpdateInterior();
}

int CInterfaceLoadingBase::operator&( IBinSaver &saver )
{
	if ( !saver.IsChecksum() )
	{
		NI_ASSERT( 0, "Can't be saved/loaded" );
	}
	return 0;
}

void CInterfaceLoadingBase::PlayOnEnterTransitEffect()
{
	InterfaceState()->SetTransitEffectFlag( false );

	if ( !NGfx::Is3DActive() )
		return;

	NGScene::CRTPtr *pTarget = NGScene::GetFrameTransitionCapture2();
	if ( pTarget )
		pLoadingScene->Draw( pTarget->GetTexture() );

	while ( !NGScene::IsFrameTransitionComplete() )
	{
		NGScene::RenderFrameTransition();
		NGScene::Flip();

		Sleep( 10 );
	}
}

bool CInterfaceLoadingBase::IsEffectFinished() const
{
	return dynamic_cast_ptr<CReactions*>( pReactions )->IsEffectFinished();
}

// CInterfaceLoadingSingle2D

CInterfaceLoadingSingle2D::CInterfaceLoadingSingle2D()
{
	if ( NSaveLoad::g_WaitLoadData.bChapter )
		return;

	InitInternal( "LoadingSingle" );
	MakeInterior( NDb::Get<NDb::STexture>( NSaveLoad::g_WaitLoadData.dbidMinimap ), NSaveLoad::g_WaitLoadData.wszDesc, InterfaceState()->GetRandomCitation(), NSaveLoad::g_WaitLoadData.bChapter );
	if ( InterfaceState()->IsTransitEffectFlag() )
		PlayOnEnterTransitEffect();

	Draw();

	ShowOnEnterMoveEffect();
}

CInterfaceLoadingSingle2D::CInterfaceLoadingSingle2D( const NDb::STexture *pMinimap, const std::wstring &wszDesc, const std::wstring &wszCitation, bool bChapter )
{
	InitInternal( "LoadingSingle" );
	MakeInterior( pMinimap, wszDesc, wszCitation, bChapter );
	if ( InterfaceState()->IsTransitEffectFlag() )
		PlayOnEnterTransitEffect();

	Draw();

	ShowOnEnterMoveEffect();
}

CInterfaceLoadingSingle2D::~CInterfaceLoadingSingle2D()
{
	if ( IsInitialized() )
		RunFinishScreen();
}

void CInterfaceLoadingSingle2D::MakeInterior( const NDb::STexture *pTexture, const std::wstring &wszDesc, const std::wstring &wszCitation, bool bChapter )
{
	IWindow *pMain = GetChildChecked<IWindow>( GetScreen(), "Main", true );

	ITextView *pDescView = GetChildChecked<ITextView>( pMain, "Desc", true );
	IWindow *pMapPictureWnd = GetChildChecked<IWindow>( pMain, "Map", true );
	pCitation = GetChildChecked<IWindow>( GetScreen(), "BottomPanel", true );
	ITextView * pCitationText = GetChildChecked<ITextView>( pMain, "Citation", true );
	pPressKeyView = GetChildChecked<ITextView>( pMain, "PressKeyView", true );
	pInfoPanel = GetChildChecked<IWindow>( pMain, "InfoPanel", true );
	IWindow *pMaxInfoPanel = GetChildChecked<IWindow>( pMain, "MaxInfoPanel", true );

	if ( pMapPictureWnd )
	{
		pMapPictureWnd->ShowWindow( true );
		if ( pTexture )
			pMapPictureWnd->SetTexture( pTexture );
	}

	if ( pDescView )
		pDescView->SetText( pDescView->GetDBText() + wszDesc );

	FitDescription( pMaxInfoPanel, pInfoPanel, pDescView );

	if ( pCitationText)
		pCitationText->SetText( pCitationText->GetDBText() + wszCitation );

	if ( pCitation )
		pCitation->ShowWindow( false );

	if ( pInfoPanel )
		pInfoPanel->ShowWindow( false );

	if ( pPressKeyView )
		pPressKeyView->ShowWindow( false );
		
	CInterfaceLoadingBase::MakeInterior();
}

void CInterfaceLoadingSingle2D::ShowOnEnterMoveEffect()
{
	if ( pInfoPanel )
		pInfoPanel->ShowWindow( true );

	if ( pCitation )
		pCitation->ShowWindow( true );

	CPtr<SWindowContextB2Move> pContext = new SWindowContextB2Move();
	pContext->bGoOut = false;
	pContext->fMaxMoveTime = 0.0f;
	if ( pCitation )
		GetScreen()->RunStateCommandSequience( "bottom_panel_on_exit", GetScreen(), pContext, true );
	GetScreen()->RunStateCommandSequience( "effect_info_panel_on_exit", GetScreen(), pContext, true );

	while ( !IsEffectFinished() )
	{
		GetScreen()->Segment( 10 );
		Draw();
		Sleep( 10 );
	}
	
	ShowFirstElement();
	Draw();
}

void CInterfaceLoadingSingle2D::RunFinishScreen()
{
	if ( pPressKeyView )
		pPressKeyView->ShowWindow( true );
	
	g_pLoadingSingleScreen = GetScreen();

	NMainLoop::Command( ML_COMMAND_LOADING_SINGLE_FINISHED, "" );
}

// CProgressHookHelper

CProgressHookHelper::CProgressHookHelper()
{
	ranges.push_back( SRange() );
	SRange &range = ranges.back();
	range.nStep = 0;
	range.nNumSteps = 1;
	range.nLockedRange = 0;
}

void CProgressHookHelper::SetNumSteps( const int nNumSteps )
{
	SRange &range = ranges.back();
	range.nNumSteps = (std::max)( 1, nNumSteps );
}

void CProgressHookHelper::LockRange( const int nLength )
{
	ranges.back().nLockedRange = nLength;
	
	ranges.push_back( SRange() );
	SRange &range = ranges.back();
	range.nStep = 0;
	range.nNumSteps = 1;
	range.nLockedRange = 0;
}

void CProgressHookHelper::UnlockRange()
{
	ranges.pop_back();
	SRange &range = ranges.back();
	range.nStep = (std::min)( range.nNumSteps, range.nStep + range.nLockedRange );
}

void CProgressHookHelper::Step()
{
	SRange &range = ranges.back();
	range.nStep = (std::min)( range.nNumSteps, range.nStep + 1 );
}

void CProgressHookHelper::SetCurrentStep( int nCurrentStep )
{
	SRange &range = ranges.back();
	range.nStep = (std::min)( range.nNumSteps, nCurrentStep );
}

float CProgressHookHelper::GetProgress() const
{
	float fProgress = 0.0f;
	float fSubRange = 1.0f;
	for ( int i = 0; i < ranges.size(); ++i )
	{
		const SRange &range = ranges[i];
		fProgress += fSubRange * (float)range.nStep / (float)(range.nNumSteps);
		fSubRange *= (float)range.nLockedRange / (float)(range.nNumSteps);
	}
	return fProgress;
}

int CProgressHookHelper::GetProgress( int nMin, int nMax ) const
{
	return nMin + ceilf( GetProgress() * (std::max)( 1, nMax - nMin ) + 0.5f );
}

// CInterfaceLoadingSingleFinished

CInterfaceLoadingSingleFinished::CInterfaceLoadingSingleFinished() :
	CInterfaceScreenBase( "no_interface", "no_section" ),
	eUIState( EUIS_NORMAL ),
	timeAbs( 0 ),
	bFormat1( true )
{
}

bool CInterfaceLoadingSingleFinished::Init()
{
	if ( !CInterfaceScreenBase::Init() )
		return false;

	NI_ASSERT( g_pLoadingSingleScreen, "Post loading screen not found" );
	if ( !g_pLoadingSingleScreen )
	{
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		return true;
	}
	
	pScreen = dynamic_cast_ptr<IScreen*>( g_pLoadingSingleScreen );
	g_pLoadingSingleScreen = 0;

	pScreen->SetGView( Scene()->GetG2DView() );
	pScreen->SetReactionsAndChecks( this );
	Singleton<IScene>()->AddScreen( pScreen );

	pPressKeyView = GetChildChecked<ITextView>( GetScreen(), "PressKeyView", true );
	
	if ( IScreen *pScreen = GetScreen() )
	{
		wszPressKeyFormat1 = pScreen->GetTextEntry( "T_FORMAT_1" );
		wszPressKeyFormat2 = pScreen->GetTextEntry( "T_FORMAT_2" );
	}
	
	ShowPressKey( bFormat1 );

	return true;
}

bool CInterfaceLoadingSingleFinished::StepLocal( bool bAppActive )
{
	bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );
	
	NTimer::STime timeAbsCur = Singleton<IGameTimer>()->GetAbsTime();
	if ( timeAbs == 0 )
		timeAbs = timeAbsCur;
	else
	{
		float fDelta = (float)( timeAbsCur - timeAbs ) * 0.001f;
		if ( fDelta >= s_fBlinkTime )
		{
			timeAbs = timeAbsCur;
			bFormat1 = !bFormat1;
			ShowPressKey( bFormat1 );
		}
	}
		
	
	return bResult;
}

void CInterfaceLoadingSingleFinished::ShowPressKey( bool bFormat1 )
{
	std::wstring wszFormat;
	if ( bFormat1 )
		wszFormat = wszPressKeyFormat1;
	else
		wszFormat = wszPressKeyFormat2;
	if ( pPressKeyView )
		pPressKeyView->SetText( wszFormat + pPressKeyView->GetDBText() );
}

bool CInterfaceLoadingSingleFinished::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "effect_finished" )
	{
		StartTransitToMission();
		return true;
	}

	return false;
}

int CInterfaceLoadingSingleFinished::Check( const std::string &szCheckName ) const
{
	return 0;
}

bool CInterfaceLoadingSingleFinished::ProcessEvent( const struct SGameMessage &msg )
{
	if ( eUIState != EUIS_NORMAL )
		return false;

	if ( msg.mMessage.cType == NInput::CT_KEY )
	{
		MoveOutInfoPanel();
		return true;
	}

	if ( msg.mMessage.cType != NInput::CT_UNKNOWN )
		return true;

	return CInterfaceScreenBase::ProcessEvent( msg );
}

void CInterfaceLoadingSingleFinished::StartTransitToMission()
{
	eUIState = EUIS_START_TRANSIT_TO_MISSION;

}

void CInterfaceLoadingSingleFinished::StartTransitToMissionDone()
{
	eUIState = EUIS_START_TRANSIT_TO_MISSION_DONE;

	InterfaceState()->SetTransitEffectFlag( true );

	NGScene::SFrameTransitionInfo ftInfo;
	ftInfo.bRandomDir = false;
	ftInfo.vTransitionDir.Set( 1.0f, 0.0f );
	ftInfo.fTransitionLength = s_fTransitionEffectToMissionLength;
	ftInfo.nEffectDuration = s_nTransitionEffectToMissionDuration;
	ftInfo.fQuadsGroup1MinZ = 1.0f;
	ftInfo.fQuadsGroup1MaxZ = 1.1f;
	ftInfo.fQuadsGroup2MinZ = 1.0f;
	ftInfo.fQuadsGroup2MaxZ = 0.9f;
	CInterfaceScreenBase::Draw( NGScene::GetFrameTransitionCapture1( ftInfo ) );

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
}

void CInterfaceLoadingSingleFinished::MoveOutInfoPanel()
{
	GetScreen()->Enable( false );

	CPtr<SWindowContextB2Move> pContext = new SWindowContextB2Move();
	pContext->bGoOut = true;
	pContext->fMaxMoveTime = 0.0f;
	GetScreen()->RunStateCommandSequience( "bottom_panel_on_exit", GetScreen(), pContext, true );
	GetScreen()->RunStateCommandSequience( "effect_info_panel_on_exit", GetScreen(), pContext, true );
}

void CInterfaceLoadingSingleFinished::Draw( NGScene::CRTPtr *pTexture )
{
	if ( eUIState == EUIS_START_TRANSIT_TO_MISSION )
	{
		StartTransitToMissionDone();
	}
	else if ( eUIState == EUIS_START_TRANSIT_TO_MISSION_DONE )
	{
		// do nothing
	}
	else
	{
		CInterfaceScreenBase::Draw( pTexture );
	}
}

// CICLoadingSingleFinished

void CICLoadingSingleFinished::PreCreate()
{
}

void CICLoadingSingleFinished::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICLoadingSingleFinished::Configure( const char *pszConfig )
{
}

START_REGISTER(PWLSingleCommands)

REGISTER_VAR_EX( "transition_effect_to_mission_duration", NGlobal::VarIntHandler, &s_nTransitionEffectToMissionDuration, 300, STORAGE_NONE );
REGISTER_VAR_EX( "pwl_blink_time_sec", NGlobal::VarFloatHandler, &s_fBlinkTime, 1.0f, STORAGE_NONE );

FINISH_REGISTER

// just for using by MakeObjectVirtual( int nTypeID )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_LOADING_SINGLE_2D, CInterfaceLoadingSingle2D );

REGISTER_SAVELOAD_CLASS( ML_COMMAND_LOADING_SINGLE_FINISHED, CICLoadingSingleFinished )
REGISTER_SAVELOAD_CLASS( 0x171933C0, CInterfaceLoadingSingleFinished )

BASIC_REGISTER_CLASS( CInterfaceLoadingBase::CLoadingScene )


