#include "stdafx.h"

#include "CommandsHistory.h"
#include "Transceiver.h"

#include "AILogic/AILogicCommand.h"
#include "Misc/StrProc.h"
#include "Main/Profiles.h"
#include "GameX/ScenarioTracker.h"
#include "GameX/InterfaceState.h"
#include "System/GlobalVars.h"
#include "MultiplayerCommandManager.h"
#include "Input/Bind.h"
#include "SaveLoadHelper.h"
#include "GameXClassIDs.h"
#include "InterfaceMisc.h"

bool g_nSaveToSameSpace;

START_REGISTER(CommandHistory)
REGISTER_VAR_EX( "checksum_overwrite",	NGlobal::VarBoolHandler, &g_nSaveToSameSpace, false, STORAGE_NONE );
FINISH_REGISTER

// CCommandsHistory

CCommandsHistory::CCommandsHistory() : bFinishedHistory( false ), bGameFinished( true ), bLoadedHistory( false ),
	bCanAddCommand( true ), dwLastCheckSum( 0 )
{
	pStartSeed = NRandom::CreateRandomSeedCopy();
	pStartSeed->Init();
	NRandom::SetRandomSeed( pStartSeed );
}

void CCommandsHistory::StartNewGame( const NDb::SMapInfo *_pMap )
{
	savingHistory.clear();
	if ( !bLoadedHistory )
	{
		loadedHistory.clear();
		replayInfo.pMap = _pMap;
		replayInfo.nLastCommandSegment = 0;
		replayInfo.nLastGameSegment = 0;
	}
}

const bool CCommandsHistory::SerializeHistory( const std::string &szFileName, const bool bRead )
{
	CFileStream streamMain( StrFmt( "%s%s%s", NSaveLoad::GetReplayPath().c_str(), szFileName.c_str(), NSaveLoad::REPLAY_EXTENSION ), bRead ? CFileStream::WIN_READ_ONLY : CFileStream::WIN_CREATE );
	CPtr<IBinSaver> pSaverMain = CreateBinSaver( &streamMain, bRead ? SAVER_MODE_READ : SAVER_MODE_WRITE );
	if ( !IsValid( pSaverMain ) )
		return false;

	pSaverMain->Add( 2, &replayInfo );
	if ( bRead )
		pSaverMain->Add( 3, &loadedHistory );
	else
		pSaverMain->Add( 3, &savingHistory );

  if ( !bRead )
		return NSaveLoad::SerializeReplayInfo( &replayInfo, szFileName, bRead );

	return true;
}

bool CCommandsHistory::LoadHistory( const std::string &szFileName )
{
	if ( !SerializeHistory( szFileName, true ) )
		return false;

	bLoadedHistory = true;
	bFinishedHistory = false;
	bGameFinished = false;
	savingHistory.clear();

	InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_MULTI );
	IScenarioTracker *pScenarioTracker = Singleton<IScenarioTracker>();
	pScenarioTracker->SetGameType( IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL );
	pScenarioTracker->MissionStart( replayInfo.pMap, replayInfo.nTechLevel );

	IScenarioTracker::SMultiplayerInfo scenarioInfo;
	for ( int i = 0; i < replayInfo.slots.size(); ++i )
	{
		const SMPSlot &slot = replayInfo.slots[i];
		if ( !slot.bPresent )
			continue;

		pScenarioTracker->AddPlayer( i );
		pScenarioTracker->SetPlayerSide( i, slot.nTeam );
		pScenarioTracker->SetPlayerParty( i, slot.nCountry );
		pScenarioTracker->SetPlayerColour( i, slot.nColour );

		scenarioInfo.players.push_back( IScenarioTracker::SMultiplayerInfo::SPlayer() );
		IScenarioTracker::SMultiplayerInfo::SPlayer &scenarioPlayer = scenarioInfo.players.back();
		scenarioPlayer.wszName = NStr::ToUnicode( slot.szName );
		scenarioPlayer.nTeam = slot.nTeam;
		scenarioPlayer.nIndex = i;
		scenarioPlayer.nCountry = slot.nCountry;
		scenarioPlayer.nLevel = 1;
		scenarioPlayer.wszRank = L"";
	}
	pScenarioTracker->SetMultiplayerInfo( scenarioInfo );

	NGlobal::SetVar( "AI.MultiplayerCaptureTime", replayInfo.nCaptureTime * 1000 );
	NGlobal::SetVar( "multiplayer_unit_experience", replayInfo.bUnitExperience ? 1 : 0 );
	NGlobal::SetVar( "History.Playing", 1 );

	return true;
}

bool CCommandsHistory::SaveReplay( const std::string &szFileName, const SB2GameSpecificData &gameDesc, const std::vector<SMPSlot> &slots, const int nWinningSide )
{
	replayInfo.bUnitExperience = gameDesc.bUnitExp;
	replayInfo.nCaptureTime = gameDesc.nCaptureTime;;
	replayInfo.nTechLevel = gameDesc.nTechLevel;
	replayInfo.bUnitExperience = gameDesc.bUnitExp;
	replayInfo.nWinningSide = nWinningSide;
	replayInfo.slots = slots;

	if ( !SerializeHistory( szFileName, false ) )
		return false;

//	WriteToPipe( PIPE_CHAT, StrFmt( "Replay \"%s\" succesfully saved", szFileName ), 0xFF00FF00, true );
	return true;
}

bool CCommandsHistory::AddChecksumLog( const int nGameTime, const unsigned long ulChecksum, const int nEntry )
{
	if ( !bLoadedHistory && nEntry == 0 )
		dwLastCheckSum = ulChecksum;

	return true;
}

void CCommandsHistory::AddCommand( const int nSegment, IAILogicCommandB2 *pCmd )
{
	if ( bCanAddCommand && pCmd->NeedToBeStored() )
		savingHistory[nSegment].push_back( pCmd );

	if ( nSegment > replayInfo.nLastCommandSegment )
		replayInfo.nLastCommandSegment = nSegment;
}

void CCommandsHistory::ExecuteSegmentCommands( const int nSegment, ITransceiver *pTranceiver )
{
	if ( !bLoadedHistory )
	{
		if ( nSegment > replayInfo.nLastGameSegment )
			replayInfo.nLastGameSegment = nSegment;
	}
	else
	{
		if ( !bFinishedHistory )
		{
			bCanAddCommand = false;
			if ( loadedHistory.find( nSegment ) != loadedHistory.end() )
			{
				for ( std::list< CPtr<IAILogicCommandB2> >::iterator iter = loadedHistory[nSegment].begin(); iter != loadedHistory[nSegment].end(); ++iter )
					pTranceiver->SendCommand( *iter );
			}
			bCanAddCommand = true;
			if ( nSegment >= replayInfo.nLastCommandSegment )
			{
//				WriteToPipe( PIPE_CHAT, "Replay finished: No more commands", 0xFF00FF00, true );
				bFinishedHistory = true;
			}
		}
		if ( !bGameFinished )
		{
			if ( nSegment >= replayInfo.nLastGameSegment )
			{
//				WriteToPipe( PIPE_CHAT, "Replay finished: Last game segment executed", 0xFF00FF00, true );
				Singleton<IGameTimer>()->Pause( true, PAUSE_TYPE_USER_PAUSE );
				NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
					CICMessageBox::MakeConfigString( "MessageBoxWindowOk", 
					InterfaceState()->GetTextEntry( "T_REPLAY_FINISHED" ) ).c_str() );

				bGameFinished = true;
			}
		}
	}
}

ICommandsHistory *CreateCommandsHistory( const SReplayInfo &replay )
{
	CCommandsHistory *pRes = new CCommandsHistory();
	if ( replay.bDoReplay )
	{
		if ( !pRes->LoadHistory( replay.szReplayName ) )
		{
			NI_ASSERT( false, StrFmt( "Cannot load replay from file \"%s\"", replay.szReplayName ) );
			NGlobal::SetVar( "History.Playing", 0 );
		}
	}
	else
		NGlobal::SetVar( "History.Playing", 0 );

	return pRes;
}

REGISTER_SAVELOAD_CLASS( 0x1007AB80, CCommandsHistory );

