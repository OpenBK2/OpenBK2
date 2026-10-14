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

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 3, &bIsCompleted );
		f.Add( 4, &bIsStarted );
		f.Add( 5, &dbidCampaign );
		return 0;
	}

protected:
	std::string GetProfileVarAbbr( const std::string &szName ) const;

	int GetProfileVar( const std::string &szName, const int nDefault ) const;
	void SetProfileVar( const std::string &szName, const int nValue );

public:
	void Init( const CDBID &_dbid );

	const CDBID &GetDBID() const override;

	bool IsCompleted() const override;
	void SetCompleted( const bool bValue ) override;

	bool IsStarted() const override;
	void SetStarted( const bool bValue ) override;
};

class CInterfaceState : public IInterfaceState
{
	OBJECT_BASIC_METHODS( CInterfaceState );

	typedef std::unordered_map<CDBID, CObj<ICampaignState>> CCampaignsMap;
	typedef std::unordered_map<int, std::vector<std::pair<std::wstring, std::wstring>>> CParamsForMLHandler;

	CPtr<NGScene::CScreenshotTexture> pScreenShot;
	ZDATA
	ZSKIP
	ZSKIP     //CPtr<ICampaignState> pCurrentCampaign;
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
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 5, &nDifficulty );
		f.Add( 6, &dwMissionChatColor );
		f.Add( 7, &bSuppressEnableFocus );
		f.Add( 8, &bTransitEffectFlag );
		f.Add( 9, &campaigns );
		f.Add( 10, &bFirstTimeInChapter );
		f.Add( 11, &paramsForMLHandler );
		f.Add( 12, &nLastFreeIDForMLHandler );
		f.Add( 13, &freeIDsForMLHandler );
		f.Add( 14, &mpChatMessages );
		f.Add( 15, &forbiddenWords );
		f.Add( 16, &wszForbiddenReplacement );
		f.Add( 17, &bForbiddenWordsInitialized );
		f.Add( 18, &nTutorialRecommendedMission );
		f.Add( 19, &bAutoShowCommanderScreen );
		OnSerialize( f );
		return 0;
	}

private:
	void OnSerialize( IBinSaver &saver );

	EScenarioTrackerType GetScenarioTrackerType() const;

	const std::wstring &GetMLTag( const std::wstring &wszName ) const;
	const std::vector<std::pair<std::wstring, std::wstring>> &GetParamsForMLHandler( int nID ) const;

	void InitForbiddenWords();

public:
	void Init();

	const NDb::SGameRoot *GetGameRoot() const override;
	const NDb::SUIConstsB2 *GetUIConsts() const override;

	CDBID GetScreenEntryDBID( const std::string &szName ) const override;
	const NDb::SUIScreenEntry *GetScreenEntry( const std::string &szName ) const override;
	const std::wstring &GetTextEntry( const std::string &szTextID ) const override;
	const NDb::SComplexSoundDesc *GetSoundEntry( const std::string &szName ) const override;
	const NDb::STexture *GetTextureEntry( const std::string &szID ) const override;
	const std::wstring &GetMPGameType( enum NDb::EMPGameType eType ) const override;
	std::wstring GetSeasonName( enum NDb::ESeason eSeason ) const override;
	std::wstring GetMapSizeName( const NDb::SMapInfo *pMapInfo ) const override;

	NGScene::CScreenshotTexture *GetScreenShotTexture() override;

	int GetDifficulty() const override;
	void SetDifficulty( int nDifficulty ) override;

	ICampaignState *GetCampaign( const CDBID &dbidCampaign ) override;

	void
	StartSingleMission( const CDBID &dbidCampaign, int nChapterNumber, int nMissionNumber, int nDifficulty ) override;

	void SetMissionConsoleColor( uint32_t dwColor ) override;
	void WriteToMissionConsole( const std::wstring &wszText ) override;
	void WriteToMissionConsoleSelected( const std::wstring &wszText ) override;
	std::wstring GetMissionConsoleMLTag() const override;

	bool IsSuppressEnableFocus() const override
	{
		return bSuppressEnableFocus;
	}

	void SetSuppressEnableFocus( bool bSuppress ) override
	{
		bSuppressEnableFocus = bSuppress;
	}

	void SendCommandsToBringOnTop( IInterfaceBase *pNewTop );
	void SendCommandsToCloseAllIncluding( IInterfaceBase *pLastInterfaceToClose ) override;
	void SendCommandsToCloseAllIncluding( const std::string &szInterfaceID ) override;

	void MakeScenarioTracker( EScenarioTrackerType eType ) override;
	void VerifyScenarioTracker( EScenarioTrackerType eType ) const override; // sanity check

	bool IsTransitEffectFlag() const override;
	void SetTransitEffectFlag( bool bTransit ) override;

	bool IsFirstTimeInChapter() const override
	{
		return bFirstTimeInChapter;
	}

	void SetFirstTimeInChapter( bool bFirstTime ) override
	{
		bFirstTimeInChapter = bFirstTime;
	}

	bool IsAutoShowCommanderScreen() const override
	{
		return bAutoShowCommanderScreen;
	}

	void SetAutoShowCommanderScreen( bool bAutoShow ) override
	{
		bAutoShowCommanderScreen = bAutoShow;
	}

	const NDb::STexture *GetMenuBackgroundTexture() const override;

	std::wstring GetRandomCitation() override;

	int GetAndRegisterIDForMLHandler( const std::vector<std::pair<std::wstring, std::wstring>> &params ) override;
	void UnregisterIDForMLHandler( int nID ) override;
	const std::wstring &ExpandMLTag( const std::wstring &wszTag, int nIDForHandler ) const override;

	void AddMPChatMessage( const std::wstring &wszText ) override;
	std::wstring GetMPChatMessage() override;
	void ClearMPChatMessages() override;

	std::wstring FilterMPChatText( const std::wstring &wszText ) override;

	int GetTutorialRecommendedMission() const override;
	void MarkTutorialRecommendedMission( int nMission ) override;
	void ApplyTutorialRecommendedMission() override;
};
