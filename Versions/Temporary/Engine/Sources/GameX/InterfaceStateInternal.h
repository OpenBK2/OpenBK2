#pragma once

#include "InterfaceState.h"

#include <cstdint>

namespace NDb
{
	struct SGameRoot;
};

class CCampaignState : public ICampaignState
{
	OBJECT_BASIC_METHODS( CCampaignState );
	
	ZDATA
		ZSKIP
		bool bIsCompleted;
		bool bIsStarted;
		CDBID dbidCampaign;
	ZEND int operator&( IBinSaver &f ) { f.Add(3,&bIsCompleted); f.Add(4,&bIsStarted); f.Add(5,&dbidCampaign); return 0; }
protected:
	std::string GetProfileVarAbbr( const std::string &szName ) const;
	
	int GetProfileVar( const std::string &szName, const int nDefault ) const;
	void SetProfileVar( const std::string &szName, const int nValue );
public:
	void Init( const CDBID &_dbid );
	
	const CDBID &GetDBID() const;

	bool IsCompleted() const;
	void SetCompleted( const bool bValue );
	
	bool IsStarted() const;
	void SetStarted( const bool bValue );
};

class CInterfaceState : public IInterfaceState
{
	OBJECT_BASIC_METHODS( CInterfaceState );
	
	typedef std::unordered_map<CDBID, CObj<ICampaignState> > CCampaignsMap;
	typedef std::unordered_map< int, std::vector< std::pair<std::wstring, std::wstring> > > CParamsForMLHandler;
	
	CPtr<NGScene::CScreenshotTexture> pScreenShot;
	ZDATA
	ZSKIP
	ZSKIP //CPtr<ICampaignState> pCurrentCampaign;
	ZSKIP //CPtr<NGScene::CScreenshotTexture> pScreenShot;
	int nDifficulty;
	uint32_t dwMissionChatColor;
	bool bSuppressEnableFocus;
	bool bTransitEffectFlag;
	CCampaignsMap campaigns;
	bool bFirstTimeInChapter;
	CParamsForMLHandler paramsForMLHandler;
	int nLastFreeIDForMLHandler;
	std::vector<int> freeIDsForMLHandler;
	std::list<std::wstring> mpChatMessages;
	std::vector<std::wstring> forbiddenWords;
	std::wstring wszForbiddenReplacement; // допустимое в обществе ругательство
	bool bForbiddenWordsInitialized;
	int nTutorialRecommendedMission;
	bool bAutoShowCommanderScreen;
	
	ZONSERIALIZE
	ZEND int operator&( IBinSaver &f ) { f.Add(5,&nDifficulty); f.Add(6,&dwMissionChatColor); f.Add(7,&bSuppressEnableFocus); f.Add(8,&bTransitEffectFlag); f.Add(9,&campaigns); f.Add(10,&bFirstTimeInChapter); f.Add(11,&paramsForMLHandler); f.Add(12,&nLastFreeIDForMLHandler); f.Add(13,&freeIDsForMLHandler); f.Add(14,&mpChatMessages); f.Add(15,&forbiddenWords); f.Add(16,&wszForbiddenReplacement); f.Add(17,&bForbiddenWordsInitialized); f.Add(18,&nTutorialRecommendedMission); f.Add(19,&bAutoShowCommanderScreen); OnSerialize( f ); return 0; }

private:
	void OnSerialize( IBinSaver &saver );
	
	EScenarioTrackerType GetScenarioTrackerType() const;

	const std::wstring& GetMLTag( const std::wstring &wszName ) const;
	const std::vector< std::pair<std::wstring, std::wstring> >& GetParamsForMLHandler( int nID ) const;
	
	void InitForbiddenWords();
public:
	void Init();

	const NDb::SGameRoot* GetGameRoot() const;
	const NDb::SUIConstsB2* GetUIConsts() const;

	CDBID GetScreenEntryDBID( const std::string &szName ) const;
	const NDb::SUIScreenEntry* GetScreenEntry( const std::string &szName ) const;
	const std::wstring& GetTextEntry( const std::string &szTextID ) const;
	const NDb::SComplexSoundDesc* GetSoundEntry( const std::string &szName ) const;
	const NDb::STexture* GetTextureEntry( const std::string &szID ) const;
	const std::wstring& GetMPGameType( enum NDb::EMPGameType eType ) const;
	std::wstring GetSeasonName( enum NDb::ESeason eSeason ) const;
	std::wstring GetMapSizeName( const NDb::SMapInfo *pMapInfo ) const;

	NGScene::CScreenshotTexture* GetScreenShotTexture();

	int GetDifficulty() const;
	void SetDifficulty( int nDifficulty );

	ICampaignState* GetCampaign( const CDBID &dbidCampaign );

	void StartSingleMission( const CDBID &dbidCampaign, int nChapterNumber, int nMissionNumber, int nDifficulty );

	void SetMissionConsoleColor( uint32_t dwColor );
	void WriteToMissionConsole( const std::wstring &wszText );
	void WriteToMissionConsoleSelected( const std::wstring &wszText );
	std::wstring GetMissionConsoleMLTag() const;

	bool IsSuppressEnableFocus() const { return bSuppressEnableFocus; }
	void SetSuppressEnableFocus( bool bSuppress ) { bSuppressEnableFocus = bSuppress; }

	void SendCommandsToBringOnTop( IInterfaceBase * pNewTop );
	void SendCommandsToCloseAllIncluding( IInterfaceBase * pLastInterfaceToClose );
	void SendCommandsToCloseAllIncluding( const std::string &szInterfaceID );

	void MakeScenarioTracker( EScenarioTrackerType eType );
	void VerifyScenarioTracker( EScenarioTrackerType eType ) const; // sanity check

	bool IsTransitEffectFlag() const;
	void SetTransitEffectFlag( bool bTransit );
	
	bool IsFirstTimeInChapter() const { return bFirstTimeInChapter; }
	void SetFirstTimeInChapter( bool bFirstTime ) { bFirstTimeInChapter = bFirstTime; }

	bool IsAutoShowCommanderScreen() const { return bAutoShowCommanderScreen; }
	void SetAutoShowCommanderScreen( bool bAutoShow ) { bAutoShowCommanderScreen = bAutoShow; }

	const NDb::STexture* GetMenuBackgroundTexture() const;
	
	std::wstring GetRandomCitation();

	int GetAndRegisterIDForMLHandler( const std::vector< std::pair<std::wstring, std::wstring> > &params );
	void UnregisterIDForMLHandler( int nID );
	const std::wstring& ExpandMLTag( const std::wstring &wszTag, int nIDForHandler ) const;

	void AddMPChatMessage( const std::wstring &wszText );
	std::wstring GetMPChatMessage();
	void ClearMPChatMessages();

	std::wstring FilterMPChatText( const std::wstring &wszText );

	int GetTutorialRecommendedMission() const;
	void MarkTutorialRecommendedMission( int nMission );
	void ApplyTutorialRecommendedMission();
};


