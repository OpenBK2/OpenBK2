#pragma once

#include "InterfaceState.h"

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
	string GetProfileVarAbbr( const string &szName ) const;
	
	int GetProfileVar( const string &szName, const int nDefault ) const;
	void SetProfileVar( const string &szName, const int nValue );
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
	
	typedef hash_map<CDBID, CObj<ICampaignState> > CCampaignsMap;
	typedef hash_map< int, vector< pair<wstring, wstring> > > CParamsForMLHandler;
	
	CPtr<NGScene::CScreenshotTexture> pScreenShot;
	ZDATA
	ZSKIP
	ZSKIP //CPtr<ICampaignState> pCurrentCampaign;
	ZSKIP //CPtr<NGScene::CScreenshotTexture> pScreenShot;
	int nDifficulty;
	DWORD dwMissionChatColor;
	bool bSuppressEnableFocus;
	bool bTransitEffectFlag;
	CCampaignsMap campaigns;
	bool bFirstTimeInChapter;
	CParamsForMLHandler paramsForMLHandler;
	int nLastFreeIDForMLHandler;
	vector<int> freeIDsForMLHandler;
	list<wstring> mpChatMessages;
	vector<wstring> forbiddenWords;
	wstring wszForbiddenReplacement; // допустимое в обществе ругательство
	bool bForbiddenWordsInitialized;
	int nTutorialRecommendedMission;
	bool bAutoShowCommanderScreen;
	
	ZONSERIALIZE
	ZEND int operator&( IBinSaver &f ) { f.Add(5,&nDifficulty); f.Add(6,&dwMissionChatColor); f.Add(7,&bSuppressEnableFocus); f.Add(8,&bTransitEffectFlag); f.Add(9,&campaigns); f.Add(10,&bFirstTimeInChapter); f.Add(11,&paramsForMLHandler); f.Add(12,&nLastFreeIDForMLHandler); f.Add(13,&freeIDsForMLHandler); f.Add(14,&mpChatMessages); f.Add(15,&forbiddenWords); f.Add(16,&wszForbiddenReplacement); f.Add(17,&bForbiddenWordsInitialized); f.Add(18,&nTutorialRecommendedMission); f.Add(19,&bAutoShowCommanderScreen); OnSerialize( f ); return 0; }

private:
	void OnSerialize( IBinSaver &saver );
	
	EScenarioTrackerType GetScenarioTrackerType() const;

	const wstring& GetMLTag( const wstring &wszName ) const;
	const vector< pair<wstring, wstring> >& GetParamsForMLHandler( int nID ) const;
	
	void InitForbiddenWords();
public:
	void Init();

	const NDb::SGameRoot* GetGameRoot() const;
	const NDb::SUIConstsB2* GetUIConsts() const;

	CDBID GetScreenEntryDBID( const string &szName ) const;
	const NDb::SUIScreenEntry* GetScreenEntry( const string &szName ) const;
	const wstring& GetTextEntry( const string &szTextID ) const;
	const NDb::SComplexSoundDesc* GetSoundEntry( const string &szName ) const;
	const NDb::STexture* GetTextureEntry( const string &szID ) const;
	const wstring& GetMPGameType( enum NDb::EMPGameType eType ) const;
	wstring GetSeasonName( enum NDb::ESeason eSeason ) const;
	wstring GetMapSizeName( const NDb::SMapInfo *pMapInfo ) const;

	NGScene::CScreenshotTexture* GetScreenShotTexture();

	int GetDifficulty() const;
	void SetDifficulty( int nDifficulty );

	ICampaignState* GetCampaign( const CDBID &dbidCampaign );

	void StartSingleMission( const CDBID &dbidCampaign, int nChapterNumber, int nMissionNumber, int nDifficulty );

	void SetMissionConsoleColor( DWORD dwColor );
	void WriteToMissionConsole( const wstring &wszText );
	void WriteToMissionConsoleSelected( const wstring &wszText );
	wstring GetMissionConsoleMLTag() const;

	bool IsSuppressEnableFocus() const { return bSuppressEnableFocus; }
	void SetSuppressEnableFocus( bool bSuppress ) { bSuppressEnableFocus = bSuppress; }

	void SendCommandsToBringOnTop( IInterfaceBase * pNewTop );
	void SendCommandsToCloseAllIncluding( IInterfaceBase * pLastInterfaceToClose );
	void SendCommandsToCloseAllIncluding( const string &szInterfaceID );

	void MakeScenarioTracker( EScenarioTrackerType eType );
	void VerifyScenarioTracker( EScenarioTrackerType eType ) const; // sanity check

	bool IsTransitEffectFlag() const;
	void SetTransitEffectFlag( bool bTransit );
	
	bool IsFirstTimeInChapter() const { return bFirstTimeInChapter; }
	void SetFirstTimeInChapter( bool bFirstTime ) { bFirstTimeInChapter = bFirstTime; }

	bool IsAutoShowCommanderScreen() const { return bAutoShowCommanderScreen; }
	void SetAutoShowCommanderScreen( bool bAutoShow ) { bAutoShowCommanderScreen = bAutoShow; }

	const NDb::STexture* GetMenuBackgroundTexture() const;
	
	wstring GetRandomCitation();

	int GetAndRegisterIDForMLHandler( const vector< pair<wstring, wstring> > &params );
	void UnregisterIDForMLHandler( int nID );
	const wstring& ExpandMLTag( const wstring &wszTag, int nIDForHandler ) const;

	void AddMPChatMessage( const wstring &wszText );
	wstring GetMPChatMessage();
	void ClearMPChatMessages();

	wstring FilterMPChatText( const wstring &wszText );

	int GetTutorialRecommendedMission() const;
	void MarkTutorialRecommendedMission( int nMission );
	void ApplyTutorialRecommendedMission();
};


