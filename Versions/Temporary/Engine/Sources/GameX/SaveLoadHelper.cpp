#include "stdafx.h"
#include "SaveLoadHelper.h"
#include "InterfaceState.h"
#include "3Dmotor/ScreenShot.h"
#include "Image/ImageScale.h"
#include "ScenarioTracker.h"
#include "Main/Profiles.h"
#include "System/FileUtils.h"
#include "Misc/StrProc.h"
#include "System/Commands.h"
#include "System/Text.h"
#include "GetConsts.h"
#include "DBGameRoot.h"
#include "Misc/Win32Random.h"
#include "Main/MainLoop.h"
#include "GameXClassIDs.h"
#include "Main/MODs.h"

#include <algorithm>
#include <cstdint>

#include <fmt/format.h>

namespace NSaveLoad
{

bool RECORDING_REPLAY;

const int SCREENSHOT_X_SIZE = 351;//512;
const int SCREENSHOT_Y_SIZE = 241;//384;
const int CHAPTERMAP_X_SIZE = 695;
const int CHAPTERMAP_Y_SIZE = 636;

const char* INFO_FILE_EXTENSION = ".sfo";
const char* SAVE_FILE_EXTENSION = ".sav";
const char* SAVE_NAME_PREFIX = "save_";

const char* REPLAY_EXTENSION = ".replay";
const char* REPLAY_INFO_EXTENSION = ".rfo";
const char* REPLAY_NAME_PREFIX = "replay_";

SWaitLoadData g_WaitLoadData;

std::string GetSavePath()
{
	std::string szProfileDir = NProfile::GetCurrentProfileDir();
	if ( !szProfileDir.empty() && szProfileDir[szProfileDir.size() - 1] != '\\')
		szProfileDir += '\\';
	return szProfileDir + "Saves\\";
}

std::string GetReplayPath()
{
	std::string szProfileDir = NProfile::GetCurrentProfileDir();
	if ( !szProfileDir.empty() && szProfileDir[szProfileDir.size() - 1] != '\\')
		szProfileDir += '\\';
	return szProfileDir + "Replays\\";
}

// SSaveInfo

SSaveInfo::SSaveInfo() :
	bCustomMission( false ),
	bQuickSave( false ),
	bFromChapter( false ),
	bAutoSave( false )
{
}

void SSaveInfo::Read( const std::string &szFullFileName )
{
	CFileStream stream( szFullFileName, CFileStream::WIN_READ_ONLY );
	if ( !stream.IsOk() )
	{
		wszSaveName = L"";
		return;
	}
	CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_READ );
	if ( pSaver )
	{
		pSaver->Add( 1, this );
	}
}

void SSaveInfo::Write( const std::string &szFullFileName, const std::wstring &_wszSaveName, bool _bQuickSave, bool _bAutoSave, bool _bFromChapter )
{
	if ( _bQuickSave )
		InterfaceState()->GetScreenShotTexture()->Generate( true );

	bQuickSave = _bQuickSave;
	wszSaveName = _wszSaveName;
	bAutoSave = _bAutoSave;
	bFromChapter = _bFromChapter;

	GenerateInfo();

	//Save screenshot & desc
	CFileStream stream(  szFullFileName, CFileStream::WIN_CREATE );
	CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
	if ( pSaver ) 
		pSaver->Add( 1, this );
}

void SSaveInfo::GenerateInfo()
{
	//Acquire screenshot and resize
	CArray2D<NGfx::SPixel8888> tmpScreenShot;
	InterfaceState()->GetScreenShotTexture()->Get( &tmpScreenShot );
	if ( tmpScreenShot.IsEmpty() /*|| !Singleton<IScenarioTracker>()->IsMissionActive()*/ ) 
	{
		screenShot.Clear();
	}
	else
	{
		CArray2D<uint32_t> image1;

		int nScreenShotSizeX = VirtualToScreenX( SCREENSHOT_X_SIZE ); 
		int nScreenShotSizeY = VirtualToScreenY( SCREENSHOT_Y_SIZE ); 

		const int nScreenOffset = 11;
		int nScreenShotOffsetX = VirtualToScreenX( nScreenOffset );
		int nScreenShotOffsetY = VirtualToScreenY( nScreenOffset );
				
		if ( bFromChapter )
		{
			int nChapterMapSizeX = VirtualToScreenX( CHAPTERMAP_X_SIZE );
			int nChapterMapSizeY = VirtualToScreenY( CHAPTERMAP_Y_SIZE );

			nChapterMapSizeX = (std::min)( nChapterMapSizeX, tmpScreenShot.GetSizeX() - nScreenShotOffsetX );
			nChapterMapSizeY = (std::min)( nChapterMapSizeY, tmpScreenShot.GetSizeY() - nScreenShotOffsetY );

			image1.SetSizes( nChapterMapSizeX, nChapterMapSizeY );
			for ( int y = 0; y < image1.GetSizeY(); ++y )
				for ( int x = 0; x < image1.GetSizeX(); ++x )
					image1[ y ][ x ] = tmpScreenShot[ y + nScreenShotOffsetY ][ x + nScreenShotOffsetX ].dwColor;
		}
		else
		{
			image1.SetSizes( tmpScreenShot.GetSizeX(), tmpScreenShot.GetSizeY() );
			for ( int y = 0; y < tmpScreenShot.GetSizeY(); ++y )
				for ( int x = 0; x < tmpScreenShot.GetSizeX(); ++x )
					image1[ y ][ x ] = tmpScreenShot[ y ][ x ].dwColor;
		}

		CArray2D<uint32_t> image2;
		image2.SetSizes( nScreenShotSizeX, nScreenShotSizeY );
		NImage::Scale( &image2, image1, NImage::IMAGE_SCALE_METHOD_FILTER );

		screenShot.SetSizes( nScreenShotSizeX, nScreenShotSizeY );
		for ( int y = 0; y < screenShot.GetSizeY(); ++y )
			for ( int x = 0; x < screenShot.GetSizeX(); ++x )
				screenShot[ y ][ x ].dwColor = image2[ y ][ x ];
	}

	//Fill info
	bCustomMission = Singleton<IScenarioTracker>()->IsCustomMission();
	
	const NDb::SCampaign *pCampaignDB = Singleton<IScenarioTracker>()->GetCurrentCampaign();
	const NDb::SChapter *pChapterDB = Singleton<IScenarioTracker>()->GetCurrentChapter();
	const NDb::SMapInfo *pMapInfoDB = Singleton<IScenarioTracker>()->GetCurrentMission();
	if ( pCampaignDB )
		pFlagTexture = pCampaignDB->pSaveLoadFlag;

	if ( pCampaignDB )
		wszCampaignName = GET_TEXT_PRE(pCampaignDB->,LocalizedName);
	if ( pChapterDB )
	{
		wszChapterName = GET_TEXT_PRE(pChapterDB->,LocalizedNameSaveLoad);
		wszChapterDesc = GET_TEXT_PRE(pChapterDB->,LocalizedDescription);
	}
	if ( pMapInfoDB )
	{
		wszMissionName = GET_TEXT_PRE(pMapInfoDB->,LocalizedName);
		wszMissionDesc = GET_TEXT_PRE(pMapInfoDB->,LocalizedDescription);
		wszMissionLoadingDesc = GET_TEXT_PRE(pMapInfoDB->,LoadingDescription);
	}
	if ( bFromChapter )
	{
		if ( pChapterDB )
			pPicture = pChapterDB->pMapPicture;
	}
	else
	{
		if ( pMapInfoDB )
			pPicture = pMapInfoDB->pLoadingPicture;
	}
	
	if ( NMOD::DoesAnyMODAttached() )
	{
		NMOD::SMOD mod;
		NMOD::GetAttachedMOD( &mod );
		szMODPath = mod.szRelativePath;
	}
}

void SSaveInfo::Rename( const std::string &szFullFileName, const std::wstring &wszNewSaveName )
{
	{
		CFileStream stream( szFullFileName, CFileStream::WIN_READ_ONLY );
		CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_READ );
		if ( pSaver )
		{
			pSaver->Add( 1, this );
		}
	}	

	wszSaveName = wszNewSaveName;

	{
		CFileStream stream(  szFullFileName, CFileStream::WIN_CREATE );
		CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		if ( pSaver ) 
			pSaver->Add( 1, this );
	}
}

const NDb::STexture* SSaveInfo::GetPlayerIcon( const NDb::SMapInfo *pMapInfo, int nPlayer ) const
{
	const NDb::STexture *pTexture = 0;
	
	if ( pMapInfo )
	{
		if ( 0 <= nPlayer && nPlayer < pMapInfo->players.size() )
		{
			if ( const NDb::SPartyDependentInfo *pPartyInfo = Singleton<IScenarioTracker>()->GetPlayerParty( nPlayer )/*pMapInfo->players[nPlayer].pPartyInfo*/ )
			{
				pTexture = pPartyInfo->pMinimapKeyObjectIcon;
			}
		}
	}
	return pTexture;
}

void SSaveInfo::SetLoadInfo()
{
	std::wstring wszDesc;
	if ( bFromChapter )
		wszDesc = wszChapterDesc;
	else
		wszDesc = wszMissionLoadingDesc;
	g_WaitLoadData.Set( pPicture, wszDesc, bFromChapter );
	
	if ( szMODPath.empty() )
	{
		if ( NMOD::DoesAnyMODAttached() )
			NMOD::DetachAllMODs();
	}
	else
	{
		if ( !NMOD::DoesMODAttached( szMODPath ) )
			NMOD::AttachMOD( szMODPath );
	}
}


void GetSaveList( CSaveList *pSaves, int *pnLastID )
{
	// Get Saved Games list
	std::string szPath = GetSavePath();
	std::list<std::string>								names;
	int													nNamePos, nExtPos;

	NFile::GetDirectoryFiles( szPath.c_str(), "*.sav", &names, false );
	pSaves->clear();
	for ( std::list<std::string>::iterator it = names.begin(); it != names.end(); ++it )			// fill in elements
	{
		SSavegameEntry sg;
		sg.szFileName = *it;
		sg.nID = *pnLastID;
		++(*pnLastID);

		// Get file title (w/o path or extension)
		nNamePos = sg.szFileName.rfind( '\\' );
		nExtPos = sg.szFileName.rfind( '.' );
		if ( (nNamePos != std::string::npos) && (nExtPos != std::string::npos) && (nExtPos > nNamePos) )
		{
			sg.szFileTitle = sg.szFileName.substr( nNamePos + 1, nExtPos - nNamePos - 1);
		} else {
			sg.szFileTitle = "???";
		}

		sg.wszName = NStr::ToUnicode( sg.szFileTitle );

		// Get screenshot & info
		sg.info.screenShot.Clear();

		// GetFileAttributesEx, FileTimeToLocalFileTime and FileTimeToSystemTime, in
		// one call. The result was never checked here, and a file that cannot be
		// stat'ed left the caller with whatever the previous iteration wrote, so
		// zeroing it is the safer reading of the same code.
		if ( !GetLocalSystemTime( &sg.time, NFile::GetLastWriteTime( sg.szFileName ) ) )
		{
			sg.time = SSystemTime();
		}

		// get extra info
		sg.szInfoFileName = szPath + sg.szFileTitle + NSaveLoad::INFO_FILE_EXTENSION;
		
		sg.info.Read( sg.szInfoFileName );
		if ( !sg.info.wszSaveName.empty() )
			sg.wszName = sg.info.wszSaveName;

		pSaves->push_back( sg );
	}
}

bool IsSaveListEmpty()
{
	// Get Saved Games list
	std::string szPath = GetSavePath();
	std::list<std::string> names;

	NFile::GetDirectoryFiles( szPath.c_str(), "*.sav", &names, false );
	
	return names.empty();
}

std::string GetUniqueFileName( const CSaveList &saves )
{
	int nIndex = 0;
	for ( int i = 0; i < saves.size(); ++i )
	{
		const SSavegameEntry &save = saves[i];
		
		int nLen = strlen( SAVE_NAME_PREFIX );
		int nFileIndex = 0;
		if ( nLen < save.szFileTitle.size() )
		{
			std::string szText = save.szFileTitle.substr( nLen );
			NStr::TrimLeft( szText, '0' );
			nFileIndex = NStr::ToInt( szText );
		}
		nIndex = (std::max)( nIndex, nFileIndex + 1 );
	}
	return fmt::format( "{}{:04d}", SAVE_NAME_PREFIX, nIndex );
}

// SWaitLoadData

void SWaitLoadData::Set( const NDb::STexture *_pMinimap, const std::wstring &_wszDesc, bool _bChapter )
{
	dbidMinimap = _pMinimap ? _pMinimap->GetDBID() : "";
	wszDesc = _wszDesc;
	bChapter = _bChapter;
}

void SWaitLoadData::Reset()
{
	dbidMinimap = "";
}

void MakeUniqueSave( const std::wstring &wszUserName, bool bQuickSave, bool bAutoSave, bool bFromChapter )
{
	CSaveList saves;
	int nLastID = 0;
	GetSaveList( &saves, &nLastID );

	std::string szName = GetUniqueFileName( saves );
	for ( CSaveList::iterator it = saves.begin(); it != saves.end(); ++it )
	{
		if ( it->wszName == wszUserName )
		{
			szName = it->szFileTitle;
			remove( it->szFileName.c_str() );
			remove( it->szInfoFileName.c_str() );
			saves.erase( it );
			break;
		}
	}

	NI_VERIFY( NFile::IsValidDirName( szName ), fmt::format("Wrong file name \"{}\"", szName), return );

	NSaveLoad::SSaveInfo info;
	info.Write( GetSavePath() + szName + INFO_FILE_EXTENSION, 
		          wszUserName, bQuickSave, bAutoSave, bFromChapter );
	NMainLoop::Command( ML_COMMAND_SAVE_GAME, szName.c_str() );
}

// replay serialization helpers

bool SerializeReplayInfo( SMultiplayerReplayInfo *pMultiplayerReplayInfo, const std::string &szFileName, const bool bRead )
{
	CFileStream file( fmt::format( "{}{}{}", GetReplayPath(), szFileName, REPLAY_INFO_EXTENSION ), bRead ? CFileStream::WIN_READ_ONLY : CFileStream::WIN_CREATE );
	CPtr<IBinSaver> pSaver = CreateBinSaver( &file, bRead ? SAVER_MODE_READ : SAVER_MODE_WRITE );
	if ( !IsValid( pSaver ) )
		return false;
	pSaver->Add( 2, pMultiplayerReplayInfo );
	return true;
}

void GetReplayList( CReplays *pReplays )
{
	if ( !pReplays )
		return;

	const std::string szPath = GetReplayPath();

	std::list<std::string> names;
	const auto mask = fmt::format( "*{}", REPLAY_INFO_EXTENSION );
	NFile::GetDirectoryFiles( szPath.c_str(), mask.c_str(), &names, false );
	pReplays->clear();
	for ( std::list<std::string>::iterator it = names.begin(); it != names.end(); ++it )
	{
		SReplayInfo replay;
		const std::string szFullFileName = *it;

		const int nNamePos = szFullFileName.rfind( '\\' );
		const int nExtPos = szFullFileName.rfind( '.' );
		if ( (nNamePos != std::string::npos) && (nExtPos != std::string::npos) && (nExtPos > nNamePos) )
		{
			replay.szFileName = szFullFileName.substr( nNamePos + 1, nExtPos - nNamePos - 1);
			if ( NFile::DoesFileExist( szPath + replay.szFileName + REPLAY_EXTENSION ) )
			{
				// GetFileAttributesEx returning FALSE skipped the replay entirely, and
				// a zero last write time means the same thing here: the file could not
				// be stat'ed.
				const std::time_t timeWrite = NFile::GetLastWriteTime( szFullFileName );
				if ( timeWrite != 0 && GetLocalSystemTime( &replay.timeFile, timeWrite ) )
				{
					if ( SerializeReplayInfo( &replay.replayInfo, replay.szFileName, true ) )
						pReplays->push_back( replay );
				}
			}
		}
	}
}

bool DeleteReplay( const std::string &szFileName )
{
	const std::string szReplayFile = GetReplayPath() + szFileName + REPLAY_EXTENSION;
	const std::string szReplayInfoFile = GetReplayPath() + szFileName + REPLAY_INFO_EXTENSION;
	const bool bResult1 = NFile::RemoveFile( szReplayFile );
	const bool bResult2 = NFile::RemoveFile( szReplayInfoFile );
	return bResult1 && bResult2;
}

} //NSaveLoad

START_REGISTER(SaveLoad)

REGISTER_VAR_EX( "recording_replay", NGlobal::VarBoolHandler, &NSaveLoad::RECORDING_REPLAY, false, STORAGE_USER );

FINISH_REGISTER


