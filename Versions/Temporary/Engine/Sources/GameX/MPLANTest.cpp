#include "stdafx.h"

#include "GetConsts.h"
#include "DBGameRoot.h"
#include "../Stats_B2_M1/DBMapInfo.h"
#include "../Misc/StrProc.h"
#include "MPLANTest.h"
#include "GameXClassIDs.h"
#include "../Main/MainLoop.h"
#include "../Main/MainLoopCommands.h"
#include "MultiplayerCommandManager.h"
#include "MPManager.h"
#include "../Misc/Win32Random.h"
#include "DBMPConsts.h"
#include "../System/Text.h"


#ifndef _FINALRELEASE
#define LAN_TEST_ENABLED
#endif


#ifdef LAN_TEST_ENABLED
	#include <ShellAPI.h>
#endif

//
//		Uses the following global vars:
//      LANTEST.Server = 1 (server) or 0 (client)
//			LANTEST.SessionName = "LANTEST"
//			LANTEST.TechLevel = 0
//			LANTEST.TimeLimit = 60
//			LANTEST.GameSpeed = 0
//			LANTEST.UnitExp = 0
//			LANTEST.CaptureTime = 15
//			LANTEST.Players = 2
//			LANTEST.PlayerName = "LANTEST"
//			LANTEST.ExecuteOnEnd = ""
//			LANTEST.ExecuteOnAsync = ""
//	
//			The feature is disabled in final release.
//

REGISTER_SAVELOAD_CLASS( 0x19243C00, CLANTester )

CLANTester::CLANTester() : bIsServer( NGlobal::GetVar( "LANTEST.Server", 0 ) == 1 ), bAcceptSent( false ),
	nMySlot( -1 ), bStarted( false ), nPlayersToWait( NGlobal::GetVar( "LANTEST.Players", 2 ) - 1 )
{
}

void CLANTester::Start()
{
	NGlobal::SetVar( "LANTEST", 1 );
	if ( bIsServer )
	{
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		NMainLoop::Command( ML_COMMAND_MP_GAME_ROOM, "" );
		CreateGame();
	}
	else
	{
	}
	bStarted = true;
}

void CLANTester::NewGameFound( const int nID, const string &szName )
{
	if ( bIsServer )
		return;

	if ( szName == NStr::ToMBCS( NGlobal::GetVar( "LANTEST.SessionName", L"LANTEST" ) ) )
	{
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		NMainLoop::Command( ML_COMMAND_MP_GAME_ROOM, "" );
		CPtr<SMPUIJoinGameMessage> pMessage = new SMPUIJoinGameMessage( nID );
		pMPManager->AddUIMessage( pMessage );
	}
}

void CLANTester::ClientInfoChanged( const int nSlot, const bool bReady )
{
	gameClientsReady[nSlot] = bReady;
	if ( !bAcceptSent )
	{
		if ( gameClientsReady.size() < nPlayersToWait )
			return;
		if ( !bIsServer )
		{
			for ( int i = 1; i < nMySlot; ++i ) // Потому что слот 0 - это создатель игры. Он всегда готов:)
			{
				hash_map<int,bool>::iterator it = gameClientsReady.find( i );
				if ( it == gameClientsReady.end() || !it->second )
					return;
			}
			CPtr<SMPUIUpdateSlotMessage> pMsg = new SMPUIUpdateSlotMessage;
			pMsg->nSlot = nMySlot;
			pMsg->info.nClientID = -1;
			pMsg->info.bPresent = true;
			pMsg->info.szName = NStr::ToMBCS( NGlobal::GetVar( "LANTEST.PlayerName", L"LANTEST" ) );
			pMsg->info.nTeam = nMySlot % 2;
			const NDb::SMultiplayerConsts *pMPConsts = NGameX::GetMPConsts();
			pMsg->info.nCountry = NWin32Random::Random( pMPConsts->sides.size() );
			pMsg->info.nColour = nMySlot;
			pMsg->info.bAccept = true;
			pMsg->info.bRandomCountry = false;
			pMPManager->AddUIMessage( pMsg );
			bAcceptSent = true;
		}
		else
		{
			for ( hash_map<int,bool>::iterator it = gameClientsReady.begin(); it != gameClientsReady.end(); ++it )
			{
				if ( !it->second )
					return;
			}
			pMPManager->AddUIMessage( EMUI_START_GAME );
			bAcceptSent = true;
		}
	}
	
}

void CLANTester::ClientRemoved( const int nClientID )
{
	gameClientsReady.erase( nClientID );
}

void CLANTester::CreateGame()
{
	const NDb::SGameRoot *pGameRoot = NGameX::GetGameRoot();
	NI_VERIFY( pGameRoot, "No GameRoot found!", return )
	NI_VERIFY( pGameRoot->pTestMap, "TestMap is not set", return )
	CPtr<SMPUICreateGameMessage> pCreateMsg = new SMPUICreateGameMessage();
	pCreateMsg->info.szSessionName = NStr::ToMBCS( NGlobal::GetVar( "LANTEST.SessionName", "LANTEST" ) );
	pCreateMsg->info.szMapName = NStr::ToMBCS( GET_TEXT_PRE( pGameRoot->pTestMap->, MapName ) );
	pCreateMsg->specificInfo.pMPMap = pGameRoot->pTestMap;
	pCreateMsg->info.nPlayersMax = NGlobal::GetVar( "LANTEST.nPlayers", 2 );
	pCreateMsg->specificInfo.nTimeLimit = NGlobal::GetVar( "LANTEST.TimeLimit", 60 );
	pCreateMsg->specificInfo.nGameSpeed = NGlobal::GetVar( "LANTEST.GameSpeed", 0 );
	pCreateMsg->specificInfo.nTechLevel = NGlobal::GetVar( "LANTEST.TechLevel", 0 );
	pCreateMsg->specificInfo.bUnitExp = ( NGlobal::GetVar( "LANTEST.UnitExp", 0 ) == 1 );
	pCreateMsg->specificInfo.nCaptureTime = NGlobal::GetVar( "LANTEST.CaptureTime", 15 );
	pCreateMsg->specificInfo.bRandomPlacement = false;
	pMPManager->AddUIMessage( pCreateMsg );
}

void CLANTester::RunShellCommand( const wstring &wszCommand  )
{
#ifdef LAN_TEST_ENABLED
	if ( wszCommand != L"" )
	{
		vector<TCHAR> winCommand( wszCommand.begin(), wszCommand.end() );
		winCommand.push_back( '\0' );
		ShellExecute( 0, "open", &( winCommand[0] ), "", "", SW_SHOWNORMAL );
	}	
#endif
}

void CLANTester::EndGame()
{
	wstring wszCommand = NGlobal::GetVar( "LANTEST.ExecuteOnEnd", "" );
	RunShellCommand( wszCommand );
	NMainLoop::Command( CreateICExitGame() );
}

void CLANTester::AsyncDetected()
{
	wstring wszCommand = NGlobal::GetVar( "LANTEST.ExecuteOnAsync", "" );
	RunShellCommand( wszCommand );
	NMainLoop::Command( CreateICExitGame() );
}

#ifdef LAN_TEST_ENABLED

void StartLanTest( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	CDynamicCast<CMPManager> pMPManager = Singleton<IMPToUIManager>();
	if ( pMPManager )
	{
		CPtr<CLANTester> pLANTester = new CLANTester();
		pLANTester->SetMPManager( pMPManager );
		pMPManager->AddUIMessage( EMUI_LAN_NET );
		pMPManager->SetLanTester( pLANTester );
		NMainLoop::Command( ML_COMMAND_MP_CUSTOM_GAME_MENU, "" );
	}
}

START_REGISTER(AutomaticLanTest)
REGISTER_CMD( "lan_test", StartLanTest );
FINISH_REGISTER

#endif
