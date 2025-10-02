#pragma once

#include "../GameX/DBScenario.h"
#include "../Stats_B2_M1/DBMapInfo.h"

namespace NDb
{
	struct SGameRoot;
	struct SMapInfo;
	struct SUIConstsB2;
};
namespace NGScene
{
	class CScreenshotTexture;
};

interface ICampaignState : public CObjectBase
{
	virtual const CDBID &GetDBID() const = 0;
	
	virtual bool IsCompleted() const = 0;
	virtual void SetCompleted( const bool bValue ) = 0;
	
	virtual bool IsStarted() const = 0;
	virtual void SetStarted( const bool bValue ) = 0;
};

interface IInterfaceState : public CObjectBase
{
	enum { tidTypeID = 0x170C1B40 };
	
	enum EScenarioTrackerType
	{
		ESTT_NONE,
		ESTT_SINGLE,
		ESTT_MULTI,
	};
	
	virtual const NDb::SGameRoot* GetGameRoot() const = 0;
	virtual const NDb::SUIConstsB2* GetUIConsts() const = 0;

	virtual CDBID GetScreenEntryDBID( const string &szName ) const = 0;
	virtual const NDb::SUIScreenEntry* GetScreenEntry( const string &szName ) const = 0;
	virtual const wstring& GetTextEntry( const string &szTextID ) const = 0;
	virtual const NDb::SComplexSoundDesc* GetSoundEntry( const string &szName )	const = 0;
	virtual const NDb::STexture* GetTextureEntry( const string &szID ) const = 0;
	virtual const wstring& GetMPGameType( enum NDb::EMPGameType eType ) const = 0;
	virtual wstring GetSeasonName( enum NDb::ESeason eSeason ) const = 0;
	virtual wstring GetMapSizeName( const NDb::SMapInfo *pMapInfo ) const = 0;

	virtual NGScene::CScreenshotTexture* GetScreenShotTexture() = 0;

	virtual int GetDifficulty() const = 0;
	virtual void SetDifficulty( int nDifficulty ) = 0;
	
	virtual ICampaignState* GetCampaign( const CDBID &dbidCampaign ) = 0;

	virtual void StartSingleMission( const CDBID &dbidCampaign, int nChapterNumber, int nMissionNumber, int nDifficulty ) = 0;
	
	//{ mission console
	virtual void SetMissionConsoleColor( DWORD dwColor ) = 0;
	virtual void WriteToMissionConsole( const wstring &wszText ) = 0;
	virtual void WriteToMissionConsoleSelected( const wstring &wszText ) = 0;
	virtual wstring GetMissionConsoleMLTag() const = 0;
	//}
	
	virtual bool IsSuppressEnableFocus() const = 0;
	virtual void SetSuppressEnableFocus( bool bSuppress ) = 0;

	// closing interfaces in a bunch
	virtual void SendCommandsToCloseAllIncluding( interface IInterfaceBase * pLastInterfaceToClose ) = 0;
	virtual void SendCommandsToCloseAllIncluding( const string &szInterfaceID ) = 0;
	
	virtual void MakeScenarioTracker( EScenarioTrackerType eType ) = 0;
	virtual void VerifyScenarioTracker( EScenarioTrackerType eType ) const = 0; // sanity check
	
	virtual bool IsTransitEffectFlag() const = 0;
	virtual void SetTransitEffectFlag( bool bTransit ) = 0;

	virtual bool IsFirstTimeInChapter() const = 0;
	virtual void SetFirstTimeInChapter( bool bFirstTime ) = 0;
	
	virtual bool IsAutoShowCommanderScreen() const = 0;
	virtual void SetAutoShowCommanderScreen( bool bAutoShow ) = 0;
	
	virtual const NDb::STexture* GetMenuBackgroundTexture() const = 0;

	virtual wstring GetRandomCitation() = 0;
	
	virtual int GetAndRegisterIDForMLHandler( const vector< pair<wstring, wstring> > &params ) = 0;
	virtual void UnregisterIDForMLHandler( int nID ) = 0;
	virtual const wstring& ExpandMLTag( const wstring &wszTag, int nIDForHandler ) const = 0;
	
	// push/pop mp chat messages with tags (color etc.)
	//
	virtual void AddMPChatMessage( const wstring &wszText ) = 0;
	virtual wstring GetMPChatMessage() = 0;
	virtual void ClearMPChatMessages() = 0;

	// проверяет текст, заменяя запрещенные вхождения на спец. символы
	virtual wstring FilterMPChatText( const wstring &wszText ) = 0;
	
	virtual int GetTutorialRecommendedMission() const = 0;
	virtual void MarkTutorialRecommendedMission( int nMission ) = 0;
	virtual void ApplyTutorialRecommendedMission() = 0;
};

inline IInterfaceState* InterfaceState() { return Singleton<IInterfaceState>(); }

IInterfaceState* CreateInterfaceState();


