#include "StdAfx.h"
#include "PlayMovieInterface.h"
#include "Misc/StrProc.h"
#include "GameXClassIDs.h"
#include "SceneB2/Cursor.h"
#include "UI/SceneClassIDs.h"
#include "SceneB2/Scene.h"
#include "Main/MainLoopCommands.h"
#include "InterfaceState.h"
#include "Sound/MusicSystem.h"
#include "SceneB2/FullScreenFader.h"
#include "System/VFSOperations.h"
#include "System/XmlSaver.h"

namespace
{

struct SProlog
{
	float fFadeIn;
	
	SProlog() : fFadeIn( 0.0f ) {}

	int operator&( IXmlSaver &saver )
	{
		saver.Add( "FadeIn", &fFadeIn );
		return 0;
	}
};

} //namespace unnamed

// ************************************************************************************************************************ //
// **
// ** play movie interface command
// **
// **
// **
// ************************************************************************************************************************ //

int CICPlayMovie::operator&( IBinSaver &saver )
{
	saver.Add( 1, &szSequenceName );
	saver.Add( 2, &szNextCommand );
	return 0;
}

void CICPlayMovie::Configure( const char *pszConfig )
{
	if ( !pszConfig ) 
		return;
	// split string
	vector<string> szStrings;
	NStr::SplitStringWithMultipleBrackets( pszConfig, szStrings, ';' );
	// movie sequence name
	if ( szStrings.size() >= 1 ) 
		szSequenceName = szStrings[0];
	// next interface type ID
	if ( szStrings.size() >= 2 ) 
	{
		szNextCommand = szStrings[1];
		NStr::TrimBoth( szNextCommand, "\t\n\r \"" );
	}
}

void CICPlayMovie::PostCreate( IInterface *pInterface ) 
{ 
	string szMovie = szSequenceName;
	if ( szSequenceName.size() >= 4 && szSequenceName.compare( szSequenceName.size() - 4, 4, ".xml" ) != 0 )
		szMovie += ".xml";
	pInterface->LoadMovieSequence( szMovie );
	pInterface->SetNextInterface( szNextCommand );
	NMainLoop::PushInterface( pInterface ); 
}

// ************************************************************************************************************************ //
// **
// ** play movie interface
// **
// **
// **
// ************************************************************************************************************************ //

CPlayMovieInterface::CPlayMovieInterface()
: CInterfaceScreenBase( "InterMission", "play_movies" ),
	bProlog( false ),
	bFadeIn( false ),
	bFadeOut( false ),
	fFadeIn( 0.0f )
{
	AddObserver( "movie_skip_sequence", &CPlayMovieInterface::MsgSkipSequence );
	AddObserver( "movie_skip_movie", &CPlayMovieInterface::MsgSkipMovie );
}

CPlayMovieInterface::~CPlayMovieInterface()
{
}

void CPlayMovieInterface::LoadMovieSequence( const string &_szFileName )
{
	szFileName = _szFileName;
	
	if ( NGlobal::GetVar("novideo", 0) != 0 )
	{
		return;
	}
	
	if ( !pScreen )
	{
		pScreen = MakeObjectVirtual<IScreen>( UI_SCREEN );
		AddUIScreen( pScreen, "InterMission", 0 );

		pPlayerControl = GetChildChecked<IPlayer>( pScreen, "VideoPlayer", true );
		pBackground = GetChildChecked<IWindow>( pPlayerControl, "Background", true );
		
		SProlog prolog;
		{
			CFileStream stream( NVFS::GetMainVFS(), szFileName );
			if ( stream.IsOk() )
			{
				CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
				pSaver->Add( "Prolog", &prolog );
			}
		}
		fFadeIn = prolog.fFadeIn;
		if ( fFadeIn > 0.0f )
		{
			bProlog = true;
			bFadeIn = false;
			bFadeOut = false;
			if ( pBackground )
				pBackground->ShowWindow( false );
		}
		else
		{
			if ( pPlayerControl )
			{
				pPlayerControl->SetSequence( szFileName );
				pPlayerControl->Play();
			}
		}
	}
	
	const bool bDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
	if ( bDemo )
	{
		if ( !pPlayerControl || !pPlayerControl->IsPlaying() )
			NGlobal::SetVar( "DEMO_MODE_CONTINUE_MOVIE", 0 );
	}
}

void CPlayMovieInterface::SetNextInterface( const string &_szNextCommand )
{
	szNextCommand = _szNextCommand;
}

bool CPlayMovieInterface::Init()
{
	CInterfaceScreenBase::Init();
	Cursor()->Show( false );
	Singleton<IMusicSystem>()->Init( 0, 0 );
	return true;
}

void CPlayMovieInterface::MsgSkipSequence( const SGameMessage &msg )
{
	const bool bDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
	if ( bDemo )
		NGlobal::SetVar( "DEMO_MODE_CONTINUE_MOVIE", 0 );

	if ( bProlog )
		return;
	if ( pPlayerControl )
		pPlayerControl->SkipSequence();
}
void CPlayMovieInterface::MsgSkipMovie( const SGameMessage &msg )
{
	const bool bDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
	if ( bDemo )
		NGlobal::SetVar( "DEMO_MODE_CONTINUE_MOVIE", 0 );

	if ( bProlog )
		return;
	if ( pPlayerControl )
		pPlayerControl->SkipMovie();
}

bool CPlayMovieInterface::StepLocal( bool bAppActive )
{
	const bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );
	if ( !bAppActive ) 
		return false;

	if ( bProlog )
	{
		if ( bFadeOut )
		{
			IFullScreenFader *pFader = Scene()->GetScreenFader();
			if ( pFader && pFader->IsInProgress() )
				return bResult;

			if ( pPlayerControl )
			{
				pPlayerControl->SetSequence( szFileName );
				pPlayerControl->Play();
			}

			bProlog = false;
		}
		else if ( bFadeIn )
		{
			IFullScreenFader *pFader = Scene()->GetScreenFader();
			if ( pFader && pFader->IsInProgress() )
				return bResult;

			if ( pFader )	
				pFader->Start( 0.0f, SCREEN_FADER_CLEAR, SCREEN_FADER_CLEAR, false );
			if ( pBackground )
				pBackground->ShowWindow( true );
			bFadeIn = false;
			bFadeOut = true;
		}
		else
		{
			IFullScreenFader *pFader = Scene()->GetScreenFader();
			if ( pFader )
				pFader->Start( fFadeIn, SCREEN_FADER_CLEAR, SCREEN_FADER_BLACK, true );
			bFadeIn = true;
		}
		return bResult;
	}
	
	if ( !pPlayerControl || !pPlayerControl->IsPlaying() )
		StartNextInterface();								// exit this screen...
	return bResult;
}

void CPlayMovieInterface::StartNextInterface()
{
	if ( !szNextCommand.empty() )
	{
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		wstring wszNextCommand = NStr::ToUnicode( szNextCommand );
		NGlobal::ProcessCommand( wszNextCommand );
	}
	else
		NMainLoop::Command( CreateICExitGame() );
}

void CPlayMovieInterface::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );

	if ( !bFocus )
		PauseIntermission( true );
}

void PlayMovieSequence( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.empty() ) 
	{
		csSystem << "usage: movie_sequence <file name>" << endl;
		return;
	}
	string szSeqName = NStr::ToMBCS( paramsSet[0] );
	if ( paramsSet.size() > 1 ) 
		szSeqName += ";" + NStr::ToMBCS( paramsSet[1] );
	NMainLoop::Command( ML_COMMAND_PLAY_MOVIE, szSeqName.c_str() );
}

START_REGISTER(PlayMovieCommands)
REGISTER_CMD( "movie_sequence", PlayMovieSequence );
FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( 0x1005C440, CPlayMovieInterface );
REGISTER_SAVELOAD_CLASS( ML_COMMAND_PLAY_MOVIE, CICPlayMovie );

