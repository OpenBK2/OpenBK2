#include "stdafx.h"
#include "InterfaceStateInternal.h"
#include "DBGameRoot.h"
#include "ScenarioTracker.h"
#include "Misc/StrProc.h"
#include "GameX/GameXClassIDs.h"
#include "3Dmotor/ScreenShot.h"
#include "UISpecificB2/DBUISpecificB2.h"
#include "UI/UIML.h"
#include "GetConsts.h"
#include "InterfaceScreenBase.h"
#include "System/Text.h"
#include "Misc/Win32Random.h"

#include "GameX_export.h"

#include "port/time.h"

#include <cstdint>

#define GET_ARRAY_SIZE( pre_name, name ) ( pre_name##name##FileRefs.size() )
#define GET_ARRAY_ELEMENT( pre_name, name, index ) ( NText::GetText( pre_name##name##FileRefs[index] ) )
#define CHECK_ARRAY_EMPTY( pre_name, name ) ( pre_name##name##FileRefs.empty() )

const wchar_t* FORBIDDEN_REPLACEMENT = L"!@#$%^&*()_+"; // общепринятое допустимое в обществе ругательство

const char* TUTORIAL_RECOMMENDED_MISSION = "TutorialRecommendedMission";

class CCustomMLHandler: public IMLHandler
{
	OBJECT_BASIC_METHODS( CCustomMLHandler );
public:
	void Exec( IML *pIML, IMLLayout *pLayout, const std::vector<std::wstring> &paramsSet );
};

REGISTER_SAVELOAD_CLASS( GAMEX, 0x300C8D43, CCampaignState )
REGISTER_SAVELOAD_CLASS( GAMEX, 0x300C8D44, CInterfaceState )
REGISTER_SAVELOAD_CLASS( GAMEX, 0x1716F400, CCustomMLHandler )

IInterfaceState* CreateInterfaceState()
{
	CInterfaceState *pState = new CInterfaceState();
	pState->Init();
	return pState;
}

struct SScreenEntryEqual
{
	std::string szName;
	
	SScreenEntryEqual( const std::string &_szName ) { szName = _szName; }
	bool operator()( const NDb::SUIScreenEntry &value ) const
	{
		return szName == value.szType;
	}
};

struct STextEntryEqual
{
	std::string szTextID;
	
	STextEntryEqual( const std::string &_szTextID ) { szTextID = _szTextID; }
	bool operator()( const CDBPtr< NDb::STextEntry > &value ) const
	{
		NI_VERIFY( value, "NULL text entry find", return false );
		return szTextID == value->szName;
	}
};

struct STagEntryEqual
{
	std::string szName;
	
	STagEntryEqual( const std::string &_szName ) { szName = _szName; }
	bool operator()( const NDb::SMLTag &value ) const
	{
		return szName == value.szName;
	}
};

struct SSoundEntryEqual
{
	std::string szTextID;
	
	SSoundEntryEqual( const std::string &_szTextID ) { szTextID = _szTextID; }
	bool operator()( const NDb::SUISoundEntry &value ) const
	{
		return szTextID == value.szType;
	}
};

struct STextureEntryEqual
{
	std::string szID;
	
	STextureEntryEqual( const std::string &_szID ) { szID = _szID; }
	bool operator()( const NDb::SUITextureEntry &value ) const
	{
		return szID == value.szTextID;
	}
};

// CCampaignState

void CCampaignState::Init( const CDBID &_dbid )
{
	dbidCampaign = _dbid;
	bIsCompleted = GetProfileVar( "Completed", 0 ) != 0;
	bIsStarted = GetProfileVar( "Started", 0 ) != 0;
}

const CDBID &CCampaignState::GetDBID() const
{
	return dbidCampaign;
}

std::string CCampaignState::GetProfileVarAbbr( const std::string &szName ) const
{
	return StrFmt( "Campaign.%s.%s", dbidCampaign.ToString().c_str(), szName.c_str() );
}

int CCampaignState::GetProfileVar( const std::string &szName, const int nDefault ) const
{
	int nValue = NGlobal::GetVar( GetProfileVarAbbr( szName ), nDefault );
	return nValue;
}

void CCampaignState::SetProfileVar( const std::string &szName, const int nValue )
{
	NGlobal::SetVar( GetProfileVarAbbr( szName ), nValue, STORAGE_USER );
}

bool CCampaignState::IsCompleted() const
{
	const bool bKRIDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
	if ( bKRIDemo )
		return false;
		
	return bIsCompleted;
}

void CCampaignState::SetCompleted( const bool bValue )
{
	bIsCompleted = bValue;
	SetProfileVar( "Completed", bIsCompleted ? 1 : 0 );
}

bool CCampaignState::IsStarted() const
{
	return bIsStarted;
}

void CCampaignState::SetStarted( const bool bValue )
{
	bIsStarted = bValue;
	SetProfileVar( "Started", bIsStarted ? 1 : 0 );
}

// CInterfaceState

void CInterfaceState::Init()
{
	//pScreenShot = new NGScene::CScreenshotTexture;
	nDifficulty = 0;
	bSuppressEnableFocus = false;
	bTransitEffectFlag = false;
	bFirstTimeInChapter = false;
	bAutoShowCommanderScreen = false;
	nLastFreeIDForMLHandler = 0;
	bForbiddenWordsInitialized = false;
	nTutorialRecommendedMission = 0;

	Singleton<IUIInitialization>()->SetMLHandler( L"val", new CCustomMLHandler() );
}

void CInterfaceState::InitForbiddenWords()
{
	if ( bForbiddenWordsInitialized )
		return;
	bForbiddenWordsInitialized = true;

	wszForbiddenReplacement = FORBIDDEN_REPLACEMENT;
	std::wstring wszForbiddenWords;
	const NDb::SUIConstsB2 *pUIConsts = GetUIConsts();
	if ( pUIConsts )
	{
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pUIConsts->,ForbiddenWords) )
			wszForbiddenWords = GET_TEXT_PRE(pUIConsts->,ForbiddenWords);
	}
	int nPos = 0;
	while ( nPos < wszForbiddenWords.size() )
	{
		uint16_t wChar = wszForbiddenWords[nPos];
		if ( wChar == L' ' || wChar == 0x0d || wChar == 0x0a )
		{
			nPos++;
			continue;
		}
		int nFound = wszForbiddenWords.find( 0x0d, nPos );
		if ( nFound == std::wstring::npos )
			nFound = wszForbiddenWords.size();
		std::wstring wszWord = wszForbiddenWords.substr( nPos, nFound - nPos );
		nPos = nFound;
		while ( !wszWord.empty() && wszWord[wszWord.size() - 1] == L' ' )
		{
			wszWord.pop_back();
		}
		if ( !wszWord.empty() )
			forbiddenWords.push_back( wszWord );
	}
}

const NDb::SGameRoot* CInterfaceState::GetGameRoot() const
{
	const NDb::SGameRoot *pGameRoot = NGameX::GetGameRoot();
	NI_ASSERT( pGameRoot, "Game root not defined" );
	return pGameRoot;
}

const NDb::SUIConstsB2* CInterfaceState::GetUIConsts() const
{
	const NDb::SUIConstsB2 *pUIConsts = NGameX::GetUIConsts();
	NI_ASSERT( pUIConsts, "UI consts not defined" );
	return pUIConsts;
}

CDBID CInterfaceState::GetScreenEntryDBID( const std::string &szName ) const
{
//	const int nID = NGlobal::GetVar( ("screenID_" + szName).c_str(), -1 );
//	if ( nID != -1 )
//		return nID;
	const NDb::SUIScreenEntry *pEntry = GetScreenEntry( szName );
	if ( pEntry && pEntry->pScreen )
		return pEntry->pScreen->GetDBID();
	return CDBID();
}

const NDb::SUIScreenEntry* CInterfaceState::GetScreenEntry( const std::string &szName ) const
{
	const NDb::SGameRoot *pGameRoot = GetGameRoot();
	std::vector<NDb::SUIScreenEntry>::const_iterator it = find_if( pGameRoot->screens.begin(), pGameRoot->screens.end(),
		SScreenEntryEqual( szName ) );
	if ( it != pGameRoot->screens.end() )
	{
		const NDb::SUIScreenEntry *pEntry = &(*it);
		return pEntry;
	}
	NI_ASSERT( 0, StrFmt( "interface screen entry '%s' not found", szName.c_str() ) );
	return 0;
}

const std::wstring& CInterfaceState::GetTextEntry( const std::string &szTextID ) const
{
	const NDb::SGameRoot *pGameRoot = GetGameRoot();
	std::vector< CDBPtr< NDb::STextEntry > >::const_iterator it = find_if( pGameRoot->textEntries.begin(), pGameRoot->textEntries.end(),
		STextEntryEqual( szTextID ) );
	if ( it != pGameRoot->textEntries.end() )
	{
		return GET_TEXT_PRE( (*it)->,Text);
	}
	NI_ASSERT( 0, StrFmt( "text entry '%s' not found", szTextID.c_str() ) );
	static std::wstring empty;
	return empty;
}

const NDb::SComplexSoundDesc* CInterfaceState::GetSoundEntry( const std::string &szName ) const
{
	const NDb::SGameRoot *pGameRoot = GetGameRoot();
	std::vector<NDb::SUISoundEntry>::const_iterator it = find_if( pGameRoot->sounds.begin(), pGameRoot->sounds.end(),
		SSoundEntryEqual( szName ) );
	if ( it != pGameRoot->sounds.end() )
	{
		return it->pSound;
	}
	NI_ASSERT( 0, StrFmt( "interface sound entry '%s' not found", szName.c_str() ) );
	return 0;
}

const NDb::STexture* CInterfaceState::GetTextureEntry( const std::string &szID ) const
{
	const NDb::SGameRoot *pGameRoot = GetGameRoot();
	std::vector<NDb::SUITextureEntry>::const_iterator it = find_if( pGameRoot->textures.begin(), pGameRoot->textures.end(),
		STextureEntryEqual( szID ) );
	if ( it != pGameRoot->textures.end() )
	{
		return it->pTexture;
	}
	NI_ASSERT( 0, StrFmt( "texture entry '%s' not found", szID.c_str() ) );
	return 0;
}

const std::wstring& CInterfaceState::GetMLTag( const std::wstring &wszName ) const
{
	static std::wstring empty;

	std::string szName = NStr::ToMBCS( wszName );
	const NDb::SUIConstsB2 *pUIConsts = GetUIConsts();
	if ( !pUIConsts )
		return empty;
	std::vector<NDb::SMLTag>::const_iterator it = find_if( pUIConsts->tags.begin(), pUIConsts->tags.end(),
		STagEntryEqual( szName ) );
	if ( it != pUIConsts->tags.end() )
	{
		if ( CHECK_TEXT_NOT_EMPTY_PRE(it->,Text) )
			return GET_TEXT_PRE(it->,Text);
	}
	NI_ASSERT( 0, StrFmt( "Designers: custom ML tag '%s' not found", szName.c_str() ) );
	return empty;
}

const std::wstring& CInterfaceState::GetMPGameType( enum NDb::EMPGameType eType ) const
{
	static std::wstring wszEmpty;

	const NDb::SUIConstsB2 *pUIConsts = GetUIConsts();
	if ( !pUIConsts )
		return wszEmpty;
	for ( std::vector< NDb::SMPLocalizedGameType >::const_iterator it = pUIConsts->mPLocalizedGameTypes.begin();
		it != pUIConsts->mPLocalizedGameTypes.end(); ++it )
	{
		const NDb::SMPLocalizedGameType &gameType = *it;
		if ( gameType.eGameType == eType )
		{
			if ( CHECK_TEXT_NOT_EMPTY_PRE(gameType.,LocalizedText) )
				return GET_TEXT_PRE(gameType.,LocalizedText);
			return wszEmpty;
		}
	}
	return wszEmpty;
}

std::wstring CInterfaceState::GetSeasonName( enum NDb::ESeason eSeason ) const
{
	std::wstring wszEmpty;

	const NDb::SUIConstsB2 *pUIConsts = GetUIConsts();
	if ( !pUIConsts )
		return wszEmpty;

	for ( int i = 0; i < pUIConsts->seasonNames.size(); ++i )
	{
		const NDb::SUIConstsB2::SSeasonName &season = pUIConsts->seasonNames[i];
		if ( season.eSeason == eSeason )
		{
			std::wstring wszName;
			if ( CHECK_TEXT_NOT_EMPTY_PRE(season.,Name) )
				wszName = GET_TEXT_PRE(season.,Name);
			return wszName;
		}
	}

	return wszEmpty;
}

std::wstring CInterfaceState::GetMapSizeName( const NDb::SMapInfo *pMapInfo ) const
{
	std::wstring wszEmpty;

	if ( !pMapInfo )
		return wszEmpty; 
		
	std::wstring wszSize = NStr::ToUnicode( StrFmt( "%dx%d", pMapInfo->nNumPatchesX, pMapInfo->nNumPatchesY ) );
	return wszSize;
}

int CInterfaceState::GetDifficulty() const
{
	return nDifficulty;
}

void CInterfaceState::SetDifficulty( int _nDifficulty )
{
	nDifficulty = _nDifficulty;
}

ICampaignState* CInterfaceState::GetCampaign( const CDBID &dbidCampaign )
{
	CCampaignState *pCampaignState = new CCampaignState();
	pCampaignState->Init( dbidCampaign );
	campaigns[dbidCampaign] = pCampaignState;
	return pCampaignState;
}

void CInterfaceState::StartSingleMission( const CDBID &dbidCampaign, int nChapterNumber, int nMissionNumber, int nDifficulty )
{
	const NDb::SCampaign *pCampaign = NDb::Get<const NDb::SCampaign>( dbidCampaign );
	NI_VERIFY( pCampaign, StrFmt( "Start single mission: campaign \"%s\" not found", dbidCampaign.ToString().c_str() ), return );
	NI_VERIFY( nChapterNumber >= 0 && nChapterNumber < pCampaign->chapters.size(), StrFmt( "Start single mission: chapter number out of range" ), return );
	const NDb::SChapter *pChapter = pCampaign->chapters[nChapterNumber];
	NI_VERIFY( pChapter, StrFmt( "Start single mission: chapter not found" ), return );
	NI_VERIFY( nMissionNumber >= 0 && nMissionNumber < pChapter->missionPath.size(), StrFmt( "Start single mission: mission number out of range" ), return );
	const NDb::SMapInfo *pMission = pChapter->missionPath[nMissionNumber].pMap;
	NI_VERIFY( pMission, StrFmt( "Start single mission: mission not found" ), return );

	InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_SINGLE );
	
	Singleton<IScenarioTracker>()->CampaignStart( pCampaign, nDifficulty, false, false );
	for ( int i = 0; i <= nChapterNumber; ++i )
		Singleton<IScenarioTracker>()->NextChapter();
	Singleton<IScenarioTracker>()->MissionStart( pMission );
	
	NGlobal::RemoveVar( "Multiplayer.Host" );
	NGlobal::RemoveVar( "Multiplayer.Client" );

	NMainLoop::Command( ML_COMMAND_MISSION, StrFmt( "%s;normal", pMission->GetDBID().ToString().c_str() ) );
}

NGScene::CScreenshotTexture* CInterfaceState::GetScreenShotTexture()
{
	if ( !pScreenShot )
		pScreenShot = new NGScene::CScreenshotTexture;
	return pScreenShot.GetPtr();
}

void CInterfaceState::SetMissionConsoleColor( uint32_t dwColor )
{
	dwMissionChatColor = dwColor;
}

void CInterfaceState::WriteToMissionConsole( const std::wstring &wszText )
{
	WriteToPipe( PIPE_CHAT, wszText, dwMissionChatColor );
}

void CInterfaceState::WriteToMissionConsoleSelected( const std::wstring &wszText )
{
	WriteToPipe( PIPE_CHAT, wszText, 0xFFFF0000 );
}

std::wstring CInterfaceState::GetMissionConsoleMLTag() const
{
	std::wstring wszText = NStr::ToUnicode( StrFmt( "<color=0x%X>", dwMissionChatColor ) );
	return wszText;
}

void CInterfaceState::SendCommandsToCloseAllIncluding( const std::string &szInterfaceID )
{
	for ( IInterfaceBase *pI = NMainLoop::GetTopInterface(); pI != 0; pI = NMainLoop::GetPrevInterface( pI ) )
	{
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		CInterfaceScreenBase * pInterface = dynamic_cast<CInterfaceScreenBase*>( pI );
		if ( pInterface && pInterface->GetInterfaceType() == szInterfaceID )
			break;
	}
}

void CInterfaceState::SendCommandsToCloseAllIncluding( IInterfaceBase * pLastInterfaceToClose )
{
	for ( IInterfaceBase *pI = NMainLoop::GetTopInterface(); pI != 0; pI = NMainLoop::GetPrevInterface( pI ) )
	{
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		if ( pI == pLastInterfaceToClose )
			break;
	}
}

void CInterfaceState::SendCommandsToBringOnTop( IInterfaceBase * pNewTop )
{
	for ( IInterfaceBase *pI = NMainLoop::GetTopInterface(); pI != 0 && pI != pNewTop; pI = NMainLoop::GetPrevInterface( pI ) )
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
}

struct SVar
{
	ZDATA
	std::string szName;
	NGlobal::CValue value;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szName); f.Add(3,&value); return 0; }
	
	SVar() {}
	SVar( const std::string &_szName, const NGlobal::CValue &_value ) :
		szName( _szName ), value( _value ) {}
};

void CInterfaceState::OnSerialize( IBinSaver &saver )
{
	std::vector< std::pair<std::string, NGlobal::CValue> > vars;
	std::vector< SVar > vars2; // используем вспомогательный массив, поскольку vector< pair<> > сериализовать нельзя
	if ( !saver.IsReading() )
	{
		NGlobal::GetVarsByClass( &vars, STORAGE_SAVE );
		vars2.reserve( vars.size() );
		for ( std::vector< std::pair<std::string, NGlobal::CValue> >::const_iterator it = vars.begin(); it != vars.end(); ++it )
		{
			const std::pair<std::string, NGlobal::CValue> var = *it;
			vars2.push_back( SVar( var.first, var.second ) );
		}
	}

	saver.Add( 100, &vars2 );

	if ( saver.IsReading() )
	{
		NGlobal::ResetVarsToDefault( STORAGE_SAVE );
		for ( std::vector< SVar >::const_iterator it = vars2.begin(); it != vars2.end(); ++it  )
		{
			const SVar &var = *it;
			NGlobal::SetVar( var.szName, var.value, STORAGE_SAVE );
		}
	}
}

IInterfaceState::EScenarioTrackerType CInterfaceState::GetScenarioTrackerType() const
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	if ( !pST )
		return ESTT_NONE;
	if ( pST->GetGameType() != IAIScenarioTracker::EGT_SINGLE )
		return ESTT_MULTI;
	else
		return ESTT_SINGLE;
}

void CInterfaceState::MakeScenarioTracker( EScenarioTrackerType eType )
{
	NSingleton::UnRegisterSingleton( IScenarioTracker::tidTypeID );
	NSingleton::UnRegisterSingleton( IAIScenarioTracker::tidTypeID );

	switch ( eType )
	{
		case ESTT_NONE:
		{
			break;
		}
		
		case ESTT_SINGLE:
		{
			IScenarioTracker *pScenarioTracker = CreateScenarioTracker();
			NSingleton::RegisterSingleton( pScenarioTracker, IScenarioTracker::tidTypeID );
			NSingleton::RegisterSingleton( pScenarioTracker, IAIScenarioTracker::tidTypeID );

			break;
		}
		
		case ESTT_MULTI:
		{
			IScenarioTracker *pScenarioTracker = CreateScenarioTrackerMultiplayer();
			NSingleton::RegisterSingleton( pScenarioTracker, IScenarioTracker::tidTypeID );
			NSingleton::RegisterSingleton( pScenarioTracker, IAIScenarioTracker::tidTypeID );

			break;
		}
	}
}

void CInterfaceState::VerifyScenarioTracker( EScenarioTrackerType _eType ) const
{
	EScenarioTrackerType eType = CInterfaceState::GetScenarioTrackerType();
	NI_ASSERT( eType == _eType, "Wrong scenario tracker" );
}

bool CInterfaceState::IsTransitEffectFlag() const
{
	return bTransitEffectFlag;
}

void CInterfaceState::SetTransitEffectFlag( bool bTransit )
{
	bTransitEffectFlag = bTransit;
}

const NDb::STexture* CInterfaceState::GetMenuBackgroundTexture() const
{
	const NDb::STexture *pBackground = 0;
	
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	if ( pST )
	{
		const NDb::SCampaign *pCampaign = pST->GetCurrentCampaign();
		if ( pCampaign )
			pBackground = pCampaign->pTextureMenuBackground;
	}
	
	if ( !pBackground )
	{
		if ( const NDb::SGameRoot *pGameRoot = GetGameRoot() )
		{
			pBackground = pGameRoot->pInterfacesBackground;
		}
	}
	
	return pBackground;
}

std::wstring CInterfaceState::GetRandomCitation()
{
	std::wstring wszText;
	if ( const NDb::SGameRoot *pGameRoot = GetGameRoot() )
	{
		NWin32Random::Seed( GetCurrentTimeMilliseconds() );
		int nSize = GET_ARRAY_SIZE(pGameRoot->,citation);
		if ( nSize > 0 )
			wszText = GET_ARRAY_ELEMENT(pGameRoot->,citation,NWin32Random::Random( 0, nSize - 1 ));
	}
	return wszText;
}

int CInterfaceState::GetAndRegisterIDForMLHandler( const std::vector< std::pair<std::wstring, std::wstring> > &params )
{
	int nID;
	if ( !freeIDsForMLHandler.empty() )
	{
		nID = freeIDsForMLHandler.back();
		freeIDsForMLHandler.pop_back();
	}
	else
	{
		nID = nLastFreeIDForMLHandler;
		nLastFreeIDForMLHandler++;
	}
	paramsForMLHandler[nID] = params;
	return nID;
}

void CInterfaceState::UnregisterIDForMLHandler( int nID )
{
	if ( paramsForMLHandler.erase( nID ) != 0 )
		freeIDsForMLHandler.push_back( nID );
}

const std::wstring& CInterfaceState::ExpandMLTag( const std::wstring &wszTag, int nIDForHandler ) const
{
	if ( !wszTag.empty() && wszTag[0] == L'%' )
	{
		if ( nIDForHandler >= 0 )
		{
			std::wstring wszDynamicTag = wszTag.substr( 1 );
			
			if ( !wszDynamicTag.empty() )
			{
				const std::vector< std::pair<std::wstring, std::wstring> > &params = GetParamsForMLHandler( nIDForHandler );

				// find tag by name
				for ( int i = 0; i < params.size(); ++i )
				{
					const std::pair<std::wstring, std::wstring> &param = params[i];
					if ( param.first == wszDynamicTag )
						return param.second;
				}

				// find tag by number
				int nIndex = NStr::ToInt( NStr::ToMBCS( wszDynamicTag ) ) - 1;
				if ( nIndex >= 0 && nIndex < params.size() )
					return params[nIndex].second;
			}
		}
#ifdef _FINALRELEASE
		static std::wstring wszEmpty;
		return wszEmpty;
#else
		// can't use NI_ASSERT for dynamic tags
		static std::wstring wszUndefined;
		wszUndefined = wszTag;
		return wszUndefined;
#endif //_FINALRELEASE
	}
	
	const std::wstring &wszText = GetMLTag( wszTag );
	return wszText;
}

const std::vector< std::pair<std::wstring, std::wstring> >& CInterfaceState::GetParamsForMLHandler( int nID ) const
{
	static std::vector< std::pair<std::wstring, std::wstring> > emptyParams;

	CParamsForMLHandler::const_iterator it = paramsForMLHandler.find( nID );
	if ( it != paramsForMLHandler.end() )
	{
		const std::vector< std::pair<std::wstring, std::wstring> > &params = it->second;
		return params;
	}
	return emptyParams;
}

void CInterfaceState::AddMPChatMessage( const std::wstring &wszText )
{
	mpChatMessages.push_back( wszText );
}

std::wstring CInterfaceState::GetMPChatMessage()
{
	std::wstring wszText;
	if ( !mpChatMessages.empty() )
	{
		wszText = mpChatMessages.front();
		mpChatMessages.pop_front();
	}
	return wszText;
}

void CInterfaceState::ClearMPChatMessages()
{
	mpChatMessages.clear();
}

std::wstring CInterfaceState::FilterMPChatText( const std::wstring &_wszText )
{
	InitForbiddenWords();

	std::wstring wszText = _wszText;
	std::string szText = NStr::ToMBCS( wszText );
	NStr::ToLower( &szText );
	const std::wstring wszTextLow = NStr::ToUnicode( szText );
	int nPos = 0;
	while ( nPos < wszText.size() )
	{
		int nTailSize = wszText.size() - nPos;
		bool bReplaced = false;
		for ( int i = 0; i < forbiddenWords.size(); ++i )
		{
			const std::wstring &wszForbiddenWord = forbiddenWords[i];
			if ( nTailSize >= wszForbiddenWord.size() )
			{
				if ( memcmp( &wszForbiddenWord[0], &wszTextLow[nPos], wszForbiddenWord.size() * sizeof( wchar_t ) ) == 0 )
				{
					bReplaced = true;
					int nForbiddenCount = wszForbiddenWord.size();
					while ( nForbiddenCount > 0 )
					{
						int nCount = (std::min<int>)( nForbiddenCount, wszForbiddenReplacement.size() );
						memcpy( &wszText[nPos], &wszForbiddenReplacement[0], nCount * sizeof( wchar_t ) );
						nPos += nCount;
						nForbiddenCount -= nCount;
					}
					break;
				}
			}
		}
		if ( !bReplaced )
			nPos++;
	}
	return wszText;
}

int CInterfaceState::GetTutorialRecommendedMission() const
{
	int nRecommendedMission = NGlobal::GetVar( TUTORIAL_RECOMMENDED_MISSION, 0 );
	return nRecommendedMission;
}

void CInterfaceState::MarkTutorialRecommendedMission( int nMission )
{
	nTutorialRecommendedMission = nMission;
}

void CInterfaceState::ApplyTutorialRecommendedMission()
{
	NGlobal::SetVar( TUTORIAL_RECOMMENDED_MISSION, nTutorialRecommendedMission, STORAGE_USER );
}

// CCustomMLHandler

void CCustomMLHandler::Exec( IML *pIML, IMLLayout *pLayout, const std::vector<std::wstring> &paramsSet )
{
	std::vector<std::wstring>::const_iterator it = paramsSet.begin();
	if ( it == paramsSet.end() )
		return;
	++it;
	std::wstring wszName;
	for ( ; it != paramsSet.end(); ++it )
	{
		wszName += *it;
	}
	const std::wstring &wszText = InterfaceState()->ExpandMLTag( wszName, pIML->GetIDForHandler() );
	if ( !wszText.empty() )
		pIML->GetStream()->InsertString( wszText );
}


static int GetDifficultyFromName( const std::string &szName )
{
	if ( NStr::IsDecNumber(szName) )
		return NStr::ToInt( szName );
	else if ( szName == "easy" )
		return 0;
	else if ( szName == "normal" )
		return 1;
	else if ( szName == "hard" )
		return 2;
	else if ( szName == "veryeasy" )
		return 4;
	else
		return 0;	// default = easy
}

int GetMissionIndexFromName( const CDBID &dbidCampaign, const int nChapterIndex, const std::string &_szMissionName )
{
	std::string szMissionName = _szMissionName;
	NStr::TrimBoth( szMissionName, '\"' );
	//
	if ( NStr::IsDecNumber(szMissionName) )
		return NStr::ToInt( szMissionName );
	//
	const NDb::SCampaign *pCampaign = NDb::Get<const NDb::SCampaign>( dbidCampaign );
	NI_VERIFY( pCampaign, StrFmt( "Campaign \"%s\" not found", dbidCampaign.ToString().c_str() ), return 0 );
	NI_VERIFY( nChapterIndex >= 0 && nChapterIndex < pCampaign->chapters.size(), StrFmt( "Chapter (%d) out of range (%d)", nChapterIndex, pCampaign->chapters.size() ), return 0 );
	const NDb::SChapter *pChapter = pCampaign->chapters[nChapterIndex];
	NI_VERIFY( pChapter, StrFmt( "Chapter %d empty", nChapterIndex ), return 0 );
	//
	const CDBID dbidMission = szMissionName;
	for ( int i = 0; i < pChapter->missionPath.size(); ++i )
	{
		const NDb::CResource *pMapRes = pChapter->missionPath[i].pMap.GetBarePtrNoLoad();
		if ( pMapRes != 0 && pMapRes->GetDBID() == dbidMission )
			return i;
	}
	return 0;
}

CDBID GetCampaignDBIDFromName( const std::string &_szName )
{
	std::string szName = _szName;
	NStr::TrimBoth( szName, '\"' );
	NI_VERIFY( !NStr::IsDecNumber(szName), "Deprecated way to identify resource! Use DBID instead!", return CDBID() );
	return CDBID( szName );
}

void StartSingleMission( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	NI_VERIFY( paramsSet.size() >= 3, "Not enough params for 'mission' commmand (mission <campaign_id> <chapter number 0..> <mission number 0..> [<difficulty level 0..>])", return );

	const CDBID dbidCampaign = GetCampaignDBIDFromName( NStr::ToMBCS(paramsSet[0]) );
	if ( dbidCampaign.IsEmpty() )
		return;
		
	const int nChapterIndex = NStr::ToInt( NStr::ToMBCS(paramsSet[1]) );
	const int nMissionIndex = GetMissionIndexFromName( dbidCampaign, nChapterIndex, NStr::ToMBCS(paramsSet[2]) );
	const int nDifficulty = (paramsSet.size() >= 4) ? GetDifficultyFromName( NStr::ToMBCS(paramsSet[3]) ) : 0;
	
	InterfaceState()->StartSingleMission( dbidCampaign, nChapterIndex, nMissionIndex, nDifficulty );
}

START_REGISTER(InterfaceState)
REGISTER_CMD( "mission", StartSingleMission );
FINISH_REGISTER


