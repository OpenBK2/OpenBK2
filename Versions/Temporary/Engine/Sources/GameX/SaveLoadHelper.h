#pragma once

#include "UI/CommandParam.h"
#include "UI/DBUserInterface.h"
#include "UI/UI.h"
#include "UI/BackgroundMutableTexture.h"
#include "CommandsHistory.h"
#include "port/time.h"

namespace NDb
{
	struct SCampaign;
	struct SChapter;
	struct SMapInfo;
}

namespace NSaveLoad
{
extern bool RECORDING_REPLAY;

extern const char* INFO_FILE_EXTENSION;
extern const char* SAVE_FILE_EXTENSION;
extern const char* SAVE_NAME_PREFIX;

extern const char* REPLAY_EXTENSION;
extern const char* REPLAY_INFO_EXTENSION;
extern const char* REPLAY_NAME_PREFIX;

struct SSaveInfo
{
	ZDATA
	//{ for compability
	ZSKIP
	ZSKIP
	ZSKIP
	ZSKIP
	//}
	
	bool bCustomMission;
	ZSKIP//	CDBPtr<NDb::SCampaign> pCampaignDB;
	ZSKIP//	CDBPtr<NDb::SChapter> pChapterDB;
	ZSKIP//	CDBPtr<NDb::SMapInfo> pMapInfoDB;
	CDBPtr<NDb::STexture> pFlagTexture;
	bool bQuickSave;
	bool bFromChapter; // save from chaper screen
	ZSKIP //string szSaveName;
	bool bAutoSave;
	std::wstring wszSaveName;

	ZSKIP// CDBPtr< NDb::SText > pCampaignName;
	ZSKIP// CDBPtr< NDb::SText > pChapterName;
	ZSKIP// CDBPtr< NDb::SText > pChapterDesc;
	ZSKIP// CDBPtr< NDb::SText > pMissionName;
	ZSKIP// CDBPtr< NDb::SText > pMissionDesc;
	CDBPtr<NDb::STexture> pPicture;

	CArray2D<NGfx::SPixel8888> screenShot;
	ZSKIP// CDBPtr< NDb::SText > pMissionLoadingDesc;
	//
	std::wstring wszCampaignName;
	std::wstring wszChapterName;
	std::wstring wszChapterDesc;
	std::wstring wszMissionName;
	std::wstring wszMissionDesc;
	std::wstring wszMissionLoadingDesc;
	
	NFile::CFilePath szMODPath;

	ZEND int operator&( IBinSaver &f ) { f.Add(6,&bCustomMission); f.Add(10,&pFlagTexture); f.Add(11,&bQuickSave); f.Add(12,&bFromChapter); f.Add(14,&bAutoSave); f.Add(15,&wszSaveName); f.Add(21,&pPicture); f.Add(22,&screenShot); f.Add(24,&wszCampaignName); f.Add(25,&wszChapterName); f.Add(26,&wszChapterDesc); f.Add(27,&wszMissionName); f.Add(28,&wszMissionDesc); f.Add(29,&wszMissionLoadingDesc); f.Add(30,&szMODPath); return 0; }

	SSaveInfo();
	
	void Read( const std::string &szFullFileName );
	void Write( const std::string &szFullFileName, const std::wstring &wszSaveName, bool bQuickSave, bool bAutoSave, bool bFromChapter );
	void Rename( const std::string &szFullFileName, const std::wstring &wszNewSaveName );
	const NDb::STexture* GetPlayerIcon( const NDb::SMapInfo *pMapInfo, int nPlayer ) const;
	void GenerateInfo();
	//
	const std::wstring &GetCampaignName() const { return wszCampaignName; }
	const std::wstring &GetChapterName() const { return wszChapterName; }
	const std::wstring &GetChapterDesc() const { return wszChapterDesc; }
	const std::wstring &GetMissionName() const { return wszMissionName; }
	const std::wstring &GetMissionDesc() const { return wszMissionDesc; }
	const std::wstring &GetMissionLoadingDesc() const { return wszMissionLoadingDesc; }

	//
	
	void SetLoadInfo(); // set global var
};

struct SSavegameEntry 
{
	ZDATA
	std::wstring					wszName;
	std::string					szFileName;
	std::string					szInfoFileName;
	std::string					szFileTitle;			//without path or extension
	SSystemTime			time;
	CPtr< IWindow >	pWindow;
	SSaveInfo				info;
	int nID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszName); f.Add(3,&szFileName); f.Add(4,&szInfoFileName); f.Add(5,&szFileTitle); f.Add(6,&time); f.Add(7,&pWindow); f.Add(8,&info); f.Add(9,&nID); return 0; }
};

typedef std::vector< SSavegameEntry >	CSaveList;

struct SWaitLoadData
{
	ZDATA
	ZSKIP //CDBPtr<NDb::STexture> pMinimap;
	std::wstring wszDesc;
	bool bChapter;
	CDBID dbidMinimap;
	ZEND int operator&( IBinSaver &f ) { f.Add(3,&wszDesc); f.Add(4,&bChapter); f.Add(5,&dbidMinimap); return 0; }
	
	void Set( const NDb::STexture *pMinimap, const std::wstring &wszDesc, bool bChapter );
	void Reset();
};

extern SWaitLoadData g_WaitLoadData;

std::string GetSavePath();
void GetSaveList( CSaveList *pSaves, int *pnLastID );
bool IsSaveListEmpty();
std::string GetUniqueFileName( const CSaveList &saves );
void MakeUniqueSave( const std::wstring &wszUserName, bool bQuickSave, bool bAutoSave, bool bFromChapter );

// replays

struct SReplayInfo
{
	ZDATA
		std::string szFileName;
		SSystemTime timeFile;
		SMultiplayerReplayInfo replayInfo;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szFileName); f.Add(3,&timeFile); f.Add(4,&replayInfo); return 0; }
};

typedef std::vector<SReplayInfo> CReplays;

std::string GetReplayPath();
bool SerializeReplayInfo( SMultiplayerReplayInfo *pMultiplayerReplayInfo, const std::string &szFileName, const bool bRead );
void GetReplayList( CReplays *pReplays );
bool DeleteReplay( const std::string &szFileName );

} //NSaveLoad


