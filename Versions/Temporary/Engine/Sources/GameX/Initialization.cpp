#include "stdafx.h"

#include "GameX_export.h"
#include "Main/Main_export.h"

#include "Misc/2Darray.h"
#include "stats_b2_m1/IconsSet.h"
#include "dbmpconsts.h"
#include "UI/commandparam.h"
#include "UI/dbuserinterface.h"
#include "B2_M1_World/ClientAckManager.h"
#include "DBConsts.h"
#include "DBGameRoot.h"
#include "InterfaceState.h"
#include "MPManager.h"

#include "Common_RTS_AI/PathFinders.h"
#include "SceneB2/Camera.h"
#include "SceneB2/Cursor.h"
#include "SceneB2/DBSceneConsts.h"
#include "Sound/SFX.h"
#include "SceneB2/Scene.h"
#include "Sound/SoundScene.h"
#include "SceneB2/StatSystem.h"
#include "Sound/MusicSystem.h"
#include "DebugTools/DebugInfoManager.h"
#include "UI/UI.h"
#include "UISpecificB2/DBUISpecificB2.h"
#include "CClientGameConsts.h"
#include "AILogic/DBAIConsts.h"
#include "Main/DBNetConsts.h"
#include "GetConsts.h"
#include "DebugTools/DebugInfoManager.h"
#include "Misc/StrProc.h"

#include <zconf.h>

namespace NMain
{
	MAIN_EXPORT bool Initialize();
}
void ReferenceCheatForReleaseLinkerUISpecificB2()
{
}

namespace NGameX
{

void InitDataDependentSingletons()
{
	NSingleton::RegisterSingleton( CreateDebugSingleton(), IDebugSingleton::tidTypeID );
	NSingleton::RegisterSingleton( CreateAckManager(), IClientAckManager::tidTypeID );
}
void UnInitDataDependentSingletons()
{
	NSingleton::UnRegisterSingleton( IDebugSingleton::tidTypeID );
	NSingleton::UnRegisterSingleton( IClientAckManager::tidTypeID );
}

GAMEX_EXPORT bool Initialize()
{
	try
	{
		NGlobal::SetVar( "AI_SEGMENT_DURATION", 200 );
		NMain::Initialize();
		// scenario tracker
//		NSingleton::RegisterSingleton( CreateScenarioTracker(), IScenarioTracker::tidTypeID );
/*		IScenarioTracker * pScenarioTracker = CreateScenarioTracker();
		NSingleton::RegisterSingleton( pScenarioTracker, IScenarioTracker::tidTypeID );
		NSingleton::RegisterSingleton( pScenarioTracker, IAIScenarioTracker::tidTypeID );*/
		// sound scene
		// sound engine
		NSingleton::RegisterSingleton( CreateSoundEngine(), ISFX::tidTypeID );
		NSingleton::RegisterSingleton( CreateSoundScene(), ISoundScene::tidTypeID );
		NSingleton::RegisterSingleton( CreateMusicSystem(), IMusicSystem::tidTypeID );
		SoundScene()->Init();

		NSingleton::RegisterSingleton( CreateCursor(), ICursor::tidTypeID );
		NSingleton::RegisterSingleton( CreateCamera(), ICamera::tidTypeID );

		// constructor info
		NSingleton::RegisterSingleton( CreateConstructorInfo(), CConstructorInfo::tidTypeID );

		// scene
		NSingleton::RegisterSingleton( CreateScene(), IScene::tidTypeID );
		
		// AI DebugManager
		NSingleton::RegisterSingleton( CreateDebugInfoManager(), IDebugInfoManager::tidTypeID );

		// stats
		NSingleton::RegisterSingleton( CreateStatSystem(), IStatSystem::tidTypeID );
		// AI
		RegisterPathfinderSingleton();

		InitDataDependentSingletons();
		// interface state
		NSingleton::RegisterSingleton( CreateInterfaceState(), IInterfaceState::tidTypeID );

		//Global consts
		NSingleton::RegisterSingleton( CreateClientGameConsts(), IClientGameConsts::tidTypeID );

		//MP to UI message manager
		NSingleton::RegisterSingleton( CreateMPManager(), IMPToUIManager::tidTypeID );
	}
	catch ( ... ) 
	{
		NI_ASSERT( false, "Can't initialize specific module" );
		return false;
	}
	return true;
}

void InitConsts()
{
	const NDb::SSceneConsts *pSceneConsts = GetSceneConsts();
	const NDb::SClientGameConsts *pClientConsts = GetClientConsts();
	const NDb::SUIConstsB2 *pUIConsts = GetUIConsts();
	//
	NI_ASSERT( pUIConsts != 0 && pSceneConsts != 0 && pClientConsts != 0, "Can't find all necessary consts" );
	if ( pUIConsts != 0 && pSceneConsts != 0 && pClientConsts != 0 ) 
	{
		for ( std::vector<NDb::SCursor>::const_iterator it = pClientConsts->cursors.begin(); it != pClientConsts->cursors.end(); ++it )
			Cursor()->RegisterMode( it->eAction, it->szFileName );
		if ( pUIConsts ) 
			Singleton<IUIInitialization>()->SetUIConsts( pUIConsts );
		Singleton<IClientAckManager>()->SetClientConsts( pClientConsts );
		Singleton<IScene>()->SetSceneConsts( pSceneConsts );
	}
}

void PostStorageInitialize()
{
	InitConsts();
}

// ************************************************************************************************************************ //
// **
// ** consts retrieving
// **
// **
// **
// ************************************************************************************************************************ //

template <class TYPE>
const TYPE *GetConsts( const std::string &szVarName )
{
	std::string szValue = NStr::ToMBCS( NGlobal::GetVar( szVarName, "" ) );
	if ( szValue.empty() )
		return 0;
	NI_VERIFY( !NStr::IsDecNumber(szValue), "Deprecated way to identify resources. Use DBID instead!", return 0 );
	return NDb::Get<TYPE>( CDBID(szValue) );
}

// game root
const NDb::SGameRoot *GetGameRoot()
{
	return GetConsts<NDb::SGameRoot>( "game_root" );
}
// game consts
const NDb::SGameConsts *GetGameConsts()
{
	if ( const NDb::SGameConsts *pConsts = GetConsts<NDb::SGameConsts>("game_consts") )
		return pConsts;
	else if ( const NDb::SGameRoot *pGameRoot = GetGameRoot() )
		return pGameRoot->pConsts;
	return 0;
}
// scene consts
const NDb::SSceneConsts *GetSceneConsts()
{
	if ( const NDb::SSceneConsts *pConsts = GetConsts<NDb::SSceneConsts>("scene_consts") )
		return pConsts;
	else if ( const NDb::SGameConsts *pGameConsts = GetGameConsts() )
		return pGameConsts->pScene;
	return 0;
}
// client
const NDb::SClientGameConsts *GetClientConsts()
{
	if ( const NDb::SClientGameConsts *pConsts = GetConsts<NDb::SClientGameConsts>("client_consts") )
		return pConsts;
	else if ( const NDb::SGameConsts *pGameConsts = GetGameConsts() )
		return pGameConsts->pClient;
	return 0;
}
// UI
const NDb::SUIConstsB2 *GetUIConsts()
{
	if ( const NDb::SUIConstsB2 *pConsts = GetConsts<NDb::SUIConstsB2>("ui_consts") )
		return pConsts;
	else if ( const NDb::SGameConsts *pGameConsts = GetGameConsts() )
		return static_cast_ptr<const NDb::SUIConstsB2*>( pGameConsts->pUI );
	return 0;
}
// AI
const NDb::SAIGameConsts *GetAIConsts()
{
	if ( const NDb::SAIGameConsts *pConsts = GetConsts<NDb::SAIGameConsts>("ai_consts") )
		return pConsts;
	else if ( const NDb::SGameConsts *pGameConsts = GetGameConsts() )
		return pGameConsts->pAI;
	return 0;
}
// Net
const NDb::SNetGameConsts *GetNetConsts()
{
	if ( const NDb::SNetGameConsts *pConsts = GetConsts<NDb::SNetGameConsts>("net_consts") )
		return pConsts;
	else if ( const NDb::SGameConsts *pGameConsts = GetGameConsts() )
		return pGameConsts->pNet;
	return 0;
}
// MP
const NDb::SMultiplayerConsts *GetMPConsts()
{
	if ( const NDb::SMultiplayerConsts *pConsts = GetConsts<NDb::SMultiplayerConsts>("mp_consts") )
		return pConsts;
	else if ( const NDb::SGameConsts *pGameConsts = GetGameConsts() )
		return pGameConsts->pMultiplayer;
	return 0;
}

};


