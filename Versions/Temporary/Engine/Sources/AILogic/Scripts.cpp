#include "stdafx.h"

#include "Scripts.h"

#include "NewUpdater.h"
#include "Building.h"
#include "Entrenchment.h"
#include "UnitsIterators2.h"
#include "UnitsIterators.h"
#include "Cheats.h"
#include "UnitCreation.h"
#include "Formation.h"
#include "Soldier.h"
#include "GroupLogic.h"
#include "Statistics.h"
#include "General.h"
#include "UnitStates.h"
#include "UnitGuns.h"
#include "Weather.h"
#include "StaticObjectsIters.h"
#include "Input/Bind.h"
#include "Script/Script.h"
#include "Script/ScriptWrapper.h"
#include "GlobeScriptHandler.h"
#include "Sound/MusicSystem.h"
#include "Sound/DBMusicSystem.h"

#include "SingleReinforcement.h"
#include "ScenarioTracker.h"
#include "AILogicInternal.h"
#include "Misc/StrProc.h"
#include "PlayerReinforcement.h"
#include "Technics.h"
#include "ExecutorContainer.h"
#include "ExecutorAttackGroup.h"
#include "ExecutorAttackGroupEvent.h"
#include "ExecutorSimpleEvent.h"
#include "UI/UI.h"
#include "System/Commands.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "KeyBuildingBonusSystem.h"
#include "DBAIConsts.h"
#include "Stats_B2_M1/RPGStatsAutomagic.h"
#include "3Dmotor/DBScene.h"
#include "Stats_B2_M1/ReinfUpdates.h"
#include "CombatEstimator.h"
#include "FeedbackSystem.h"
#include "CommandRegistratorForScript.h"
#include "System/VFSOperations.h"
#include "Artillery.h"
#include "GlobalWarFog.h"
#include "Aviation.h"

extern CFeedBackSystem theFeedBackSystem;
extern CCombatEstimator theCombatEstimator;
extern CKeyBuildingBonusSystem theBonusSystem;
extern CPlayerReinforcementArray theReinfArray;
extern CWeather theWeather;
extern CSupremeBeing theSupremeBeing;
extern CGroupLogic theGroupLogic;
extern CUnitCreation theUnitCreation;
extern CUnits units;
extern CEventUpdater updater;
extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern SCheats theCheats;
extern CStatistics theStatistics;
extern CStaticObjects theStatObjs;
extern CExecutorContainer theExecutorContainer;
extern CCommandRegistratorForScript theCommandTrackerForScript;
extern CGlobalWarFog theWarFog;

CScripts* pScripts;
const int CScripts::TIME_TO_CHECK_SUSPENDED_REINF = 200;
CScriptGroups CScripts::groups;
CDBPtr<NDb::SMapInfo> CScripts::pMapInfo;
CAttackGroup CScripts::attackGroups;
static bool bScriptTimeCalculate;
std::list<CObj<CAIUnit> > CScripts::rememberedUnits;
START_REGISTER(Scripts)
REGISTER_VAR_EX( "script_time_calculate",	NGlobal::VarBoolHandler, &bScriptTimeCalculate, false, STORAGE_NONE );
FINISH_REGISTER


#define CHECK_ERROR( bCond, message, script )																												\
if ( !(bCond) )																																												\
{																																																			\
	pScripts->OutScriptError( ( std::string("Script error. ") + std::string(message) ).c_str() );				\
	return ( script.GetPushCount() );																																									\
}


static const std::string Convert2Param( Script::Object &object )
{
	if ( object.IsNumber() )
		return object.GetString();
	else
		return std::string("\"") + object.GetString() + "\"";
}

class CUnitsInScriptAreaEnumerator : public IScriptAreaEnumerator
{
	Script *pScript;
	const NDb::SScriptArea *pArea;
	int nUnits;
	std::unordered_map<int, bool> squads;

public:
	CUnitsInScriptAreaEnumerator( Script *_pScript, const NDb::SScriptArea *_pArea ) 
		: pScript( _pScript ), pArea( _pArea ), nUnits( 0 ) {  }
		bool AddUnit( CAIUnit *pUnit )
		{
			if ( pUnit->IsInfantry() && pUnit->GetFormation() )
			{
				const int nSquadID = pUnit->GetFormation()->GetUniqueId();
				if ( squads.find( nSquadID ) == squads.end() )
				{
					pScript->PushNumber( nSquadID );
					squads[nSquadID] = true;
					++nUnits;
				}
			}
			else
			{
				pScript->PushNumber( pUnit->GetUniqueId() );
				++nUnits;
			}
			return false;
		}
		void Done() { }
		const NDb::SScriptArea &GetArea() const
		{
			return *pArea;
		}
		const int GetNUnits() const { return nUnits; }
};

class CIsSomeUnitInScriptAreaEnumerator : public IScriptAreaEnumerator
{
	Script *pScript;
	const NDb::SScriptArea *pArea;
	bool bPresent;

public:
	CIsSomeUnitInScriptAreaEnumerator( Script *_pScript, const NDb::SScriptArea *_pArea ) 
		: pScript( _pScript ), pArea( _pArea ), bPresent( false ) {  }
		bool AddUnit( CAIUnit *pUnit )
		{
			bPresent = true;
			return true;
		}
		void Done() 
		{
			pScript->PushNumber( bPresent );
		}
		const NDb::SScriptArea &GetArea() const
		{
			return *pArea;
		}
};

int CScripts::GlobeCommand( struct lua_State *pState )
{
	if ( IGlobeScriptHandler *pGlobeScriptHandler = pScripts->GetGlobeScriptHandler() )
	{
		Script script( pState );
		CHECK_ERROR( script.GetObject( 1 ).IsString(), "GlobeCommand: the 1st parameter is not a string", script );

		std::string szFuncCall = "";
		szFuncCall = std::string(script.GetObject( 1 ).GetString()) + "(";

		if ( script.GetTop() >= 2 )
			szFuncCall += Convert2Param( script.GetObject( 2 ) );

		for ( int i = 3; i <= script.GetTop(); ++i )
			szFuncCall += "," + Convert2Param( script.GetObject( i ) );
		szFuncCall += ");";

		return pGlobeScriptHandler->CallGlobeScriptFunction( szFuncCall );
	}
	
	return 0;
}

SRegFunction CScripts::pRegList[] =
{
	{ "SetDynamicObjectiveScriptGroup", CScripts::SetDynamicObjectiveScriptGroup },
	{ "GetDifficultyLevel",					CScripts::GetDifficultyLevel },
	{ "RegisterCommandObserver",		CScripts::RegisterCommandObserver },
	{ "RemoveAllUnitsTmp",					CScripts::RemoveAllUnitsTmp },
	{ "ReturnAllUnits",							CScripts::ReturnAllUnits },

	{ "EnablePlayerReinforcement",	CScripts::EnablePlayerReinforcement },
	{ "GiveReinforcementCalls",			CScripts::GiveReinforcementCalls },
	{ "EnablePlayerSuperWeapon",		CScripts::EnablePlayerSuperWeapon },
	{ "IsImmobilized",							CScripts::IsImmobilized },
	{ "PlayerCanSee",								CScripts::PlayerCanSee },
	{ "UnitCanSee",									CScripts::UnitCanSee },
	{ "SetAmmo",										CScripts::SetAmmo },
	{ "UnitPlayAnimation",					CScripts::UnitPlayAnimation },
	{ "PlayEffect",									CScripts::PlayEffect },
	{ "GetAmmo",										CScripts::GetAmmo },
								
	{ "AttackGroupCreate",						CScripts::AttackGroupCreate },
	{ "AttackGroupAddUnit",						CScripts::AttackGroupAddUnit },
	{ "AttackGroupStartAttack",				CScripts::AttackGroupStartAttack },
	{ "AttackGroupDelete",						CScripts::AttackGroupDelete },
		
	{ "IsSomeUnitInArea",								CScripts::IsSomeUnitInArea },
	{ "IsSomeUnitInParty",							CScripts::IsSomeUnitInParty },
	{ "IsSomePlayerUnit",								CScripts::IsSomePlayerUnit },
	{ "IsUnitNearScriptObject",					CScripts::IsUnitNearScriptObject },
	{ "WaitForGroupInArea",							CScripts::WaitForGroupInArea },
	{ "IsSomeBodyAlive",								CScripts::IsSomeBodyAlive },
		
	// 1
//	{ "PlayVoice",											CScripts::PlayVoice },

	{ "GetPassangers",									CScripts::GetPassangers },
	{ "GetUnitListInArea",							CScripts::GetUnitListInArea },
	{ "GetUnitListOfPlayer",						CScripts::GetUnitListOfPlayer },
	{ "IsUnitInArea",										CScripts::IsUnitInArea },
	{ "LandReinforcement",							CScripts::LandReinforcement },
	{ "IsReinforcementAvailable",				CScripts::IsReinforcementAvailable },
	{ "LandReinforcementFromMap",				CScripts::LandReinforcementFromMap },

	{ "UnitCmd",												CScripts::GiveUnitCommand },
	{ "UnitQCmd",												CScripts::GiveUnitQCommand	},
	{ "GetObjectList",									CScripts::GetObjectList },
	{ "GetFreeArtillery",								CScripts::GetFreeArtillery },

	{ "CallReinforcement",							CScripts::CallReinforcement },
	{ "GetReinforcementCallsLeft",			CScripts::GetReinforcementCallsLeft },
	{ "GiveXPToReinforcement",					CScripts::GiveXPToReinforcement },
	{ "GiveXPToPlayer",									CScripts::GiveXPToPlayer },

	// 3
	{ "GetObjectHPs",										CScripts::GetObjectHPs },
	{ "ChangePlayer",										CScripts::ChangePlayer },

	{ "GiveCommand",										CScripts::GiveCommand },
	{ "Cmd",														CScripts::GiveCommand	},
	{ "GiveQCommand",										CScripts::GiveQCommand },
	{ "QCmd",														CScripts::GiveQCommand },
	{ "UnitRemove",											CScripts::UnitRemove },
	{ "DamageObject",										CScripts::DamageObject },

	{ "SetCatchArtFlag",					CScripts::SetCatchArtFlag },

	// 5 Script Movie support
	//{ "StartSequence",									CScripts::StartSequence },
	//{ "EndSequence",										CScripts::EndSequence },

	// Camera functions
	//{ "CameraMove",											CScripts::CameraMove },
	{ "IsAlive",												CScripts::IsAlive },
	//{ "StopSound",											CScripts::StopSound },
	//{ "StartSound",											CScripts::StartSound },

	{ "GetUnitState",										CScripts::GetUnitState },
	{ "GetSquadStates",									CScripts::GetSquadStates },
	{ "GetUnitRPGStats",								CScripts::GetUnitRPGStats },

	{ "GameMesage",											CScripts::GameMesage },
	//{ "AddChatMessage",									CScripts::AddChatMessage },

	// old and correct
	{ "GetNUnitsInCircle",							CScripts::GetNUnitsInCircle },
	{ "LandReinforcementOLD",						CScripts::LandReinforcementOLD },
	{	"Win",														CScripts::Win },
	{ "Loose",													CScripts::Loose },
	{ "Draw",														CScripts::Draw },
	{	"GetNUnitsInScriptGroup",					CScripts::GetNUnitsInScriptGroup },
	{	"ShowActiveScripts",							CScripts::ShowActiveScripts },
	{	"GetNUnitsInArea",								CScripts::GetNUnitsInArea },
	{ "GetNScriptUnitsInArea",					CScripts::GetNScriptUnitsInArea },
	{ "ChangeWarFog",										CScripts::ChangeWarFog },
	{ "God",														CScripts::God },
	{ "SetIGlobalVar",									CScripts::SetIGlobalVar },
	{ "SetFGlobalVar",									CScripts::SetFGlobalVar },
	{ "SetSGlobalVar",									CScripts::SetSGlobalVar },
	{ "GetIGlobalVar",									CScripts::GetIGlobalVar },
	{ "GetFGlobalVar",									CScripts::GetFGlobalVar },
	{ "GetSGlobalVar",									CScripts::GetSGlobalVar },

	{ "GetNUnitsInParty",								CScripts::GetNUnitsInParty },
	{ "GetNUnitsInPartyUF",							CScripts::GetNUnitsInPartyUF },
	{ "GetNUnitsInPlayerUF",						CScripts::GetNUnitsInPlayerUF },
	{ "ChangeFormation",								CScripts::ChangeFormation },

	{ "Trace",													CScripts::Trace },
	{ "DisplayTrace",										CScripts::DisplayTrace },
	{ "ObjectiveChanged",								CScripts::ObjectiveChanged },
	{ "GetNAmmo",												CScripts::GetNAmmo },

	{ "GetPartyOfUnits",								CScripts::GetPartyOfUnits },
	{ "CallAssert",											CScripts::CallAssert },
	{ "GetSquadInfo",										CScripts::GetSquadInfo },
	{ "IsFollowing",										CScripts::IsFollowing },
	{ "GetFrontDir",										CScripts::GetFrontDir },
	{ "GetActiveShellType",							CScripts::GetActiveShellType },
	{ "AskClient",											CScripts::AskClient },
	{ "RandomFloat",										CScripts::RandomFloat },
	{ "RandomInt",											CScripts::RandomInt },
	{ "ChangeSelection",								CScripts::ChangeSelection },
	{ "ReturnScriptIDs",								CScripts::ReturnScriptIDs },
	{ "GetPlayersMask",									CScripts::GetPlayersMask },
	{ "ObjectGetCoord",									CScripts::ObjectGetCoord },
	{ "GetScriptAreaParams",						CScripts::GetScriptAreaParams },
	{ "IsPlayerPresent",								CScripts::IsPlayerPresent },
	{ "SwitchWeather",									CScripts::SwitchWeather },
	{ "SwitchWeatherAutomatic",					CScripts::SwitchWeatherAutomatic },

	{ "GetNUnitsInSide",								CScripts::GetNUnitsInSide },
	{ "AddIronMan",											CScripts::AddIronMan },
	{ "SetDifficultyLevel",							CScripts::SetDifficultyLevel },
	{ "SetCheatDifficultyLevel",				CScripts::SetCheatDifficultyLevel },
	{ "ViewZone",												CScripts::ViewZone },
	{ "IsStandGround",									CScripts::IsStandGround },
	{ "IsEntrenched",										CScripts::IsEntrenched },

	{ "GetNAPFencesInScriptArea",				CScripts::GetNAPFencesInScriptArea },
	{ "GetNAntitankInScriptArea",				CScripts::GetNAntitankInScriptArea },
	{ "GetNFencesInScriptArea",					CScripts::GetNFencesInScriptArea },
	{ "GetNTrenchesInScriptArea",				CScripts::GetNTrenchesInScriptArea },
	{ "GetNMinesInScriptArea",					CScripts::GetNMinesInScriptArea },
	{ "Password",												CScripts::Password },
	{ "SetGameSpeed",										CScripts::SetGameSpeed },
	{ "GetNUnitsOfType",								CScripts::GetNUnitsOfType },
	{	"GetMapSize",											CScripts::GetMapSize },

	{	"CheckMissionBonus",							CScripts::CheckMissionBonus },
	{ "GetGameTime",										CScripts::GetGameTime },

	{ "StartSequenceWOMovieBorder",			CScripts::StartSequenceWOMovieBorder },
	{ "EndSequenceWOMovieBorder",				CScripts::EndSequenceWOMovieBorder },

	{ "ShowMovieBorder",								CScripts::ShowMovieBorder },
	{ "HideMovieBorder",								CScripts::HideMovieBorder },

	{ "SwitchUnitLightFX",							CScripts::SwitchUnitLightFX },
	{ "SwitchSquadLightFX",							CScripts::SwitchSquadLightFX },

	{ "ObjectPlayAttachedEffect",				CScripts::ObjectPlayAttachedEffect },
	{ "ObjectStopAttachedEffect",				CScripts::ObjectStopAttachedEffect },
	{ "ObjectPlayAnimation",						CScripts::ObjectPlayAnimation },

	{ "SCRunTime",											CScripts::SCRunTime },
	{ "SCRunSpeed",											CScripts::SCRunSpeed },
	{ "SCReset",												CScripts::SCReset },

	{ "SCStartMovie",										CScripts::SCStartMovie },
	{ "SCStopMovie",										CScripts::SCStopMovie },

	{ "glb",														CScripts::GlobeCommand },

	// debug
	{ "SetLeader",											CScripts::SetLeader },

	// for testers!
	{ "DamageAllUnits",									CScripts::DamageAllUnits },
	{ "AddAmmoToAllUnits",							CScripts::AddAmmoToAllUnits },

	{ "BlinkActionButton",							CScripts::BlinkActionButton },

	{ 0, 0 } // End
};

void CScripts::RegisterScriptForSaveLoad()
{
	NScript::AddScriptFunctionsToSaveLoad( pRegList );
	NScript::RegisterCommonFunctionsToSaveLoad();
}

CScripts::~CScripts()
{
	//for ( hash_map<int, SScriptInfo>::iterator iter = activeScripts.begin(); iter != activeScripts.end(); ++iter )
		//pScript->Unref( iter->first );
}

int CScripts::GetScriptID( CUpdatableObj *pObj ) const
{
	if ( groupUnits.find( pObj->GetUniqueId() ) != groupUnits.end() )
		return groupUnits.find( pObj->GetUniqueId() )->second;

	return -1;
}

void CScripts::AddObjToScriptGroup( CUpdatableObj *pObj, const int nGroup )
{
	if ( nGroup != -1 )
	{
		groups[nGroup].push_back( pObj );
		groupUnits[pObj->GetUniqueId()] = nGroup;
		pObj->SetScriptID( nGroup );
	}
}

void CScripts::AddUnitToReinforcGroup( const SMapObjectInfo &mapObject, const int nGroup, const SHPObjectRPGStats *pStats/*, IScenarioUnit *pScenarioUnit */)
{
	NI_ASSERT( nGroup != -1, "Wrong number of reinforcement group" );
	reinforcs[nGroup].push_back( SReinforcementObject( mapObject, pStats/*, pScenarioUnit */) );
}

void CScripts::Init( const SMapInfo *_pMapInfo )
{
	pMapInfo = _pMapInfo;
	pScripts = this;
	pConsole = Singleton<IConsoleBuffer>();

	groups.clear();
	attackGroups.clear();
	groupUnits.clear();
	reinforcs.clear();
	reservePositions.clear();
	lastTimeToCheckSuspendedReinforcs = 0;
	reinforcsIter = suspendedReinforcs.begin();

	pMapInfo = _pMapInfo;
	for ( std::vector< SBattlePosition >::const_iterator iter = pMapInfo->reservePositionsList.begin(); iter != pMapInfo->reservePositionsList.end(); ++iter )
	{
		reservePositions[iter->nArtilleryLinkID] = iter->nTruckLinkID;
		reservePositions[iter->nTruckLinkID] = iter->nArtilleryLinkID;
	}
	bShowErrors = NGlobal::GetVar( "ShowScriptErrors", 0 ) != 0;
}

void CScripts::Clear()
{
	pMapInfo = 0;
	pScripts = 0;
	pConsole = 0;

	groups.clear();
	attackGroups.clear();
	groupUnits.clear();
	reinforcs.clear();
	reservePositions.clear();
	lastTimeToCheckSuspendedReinforcs = 0;
	reinforcsIter = suspendedReinforcs.begin();
}

void CScripts::InitAreas( const NDb::SScriptArea scriptAreas[], const int nLen )
{
	for ( int i = 0; i < nLen; ++i )
		areas[scriptAreas[i].szName] = scriptAreas[i];
}

void CScripts::RunScriptFromFile( const std::string &szScriptFileName )
{
	if ( szScriptFileName.empty() )
		return;
	std::string szScriptText;
	CFileStream stream( NVFS::GetMainVFS(), szScriptFileName );
	if ( stream.IsOk() )
	{
		const int nSize = stream.GetSize();
		if ( nSize > 0 )
		{
			szScriptText.resize( stream.GetSize() );
			stream.Read( const_cast<char*>(szScriptText.data()), nSize );
		}
	}
	if ( !szScriptText.empty() )
		pScript->RunScript( szScriptText.c_str() );
}

void CScripts::Load( const std::string &szScriptFileName, const NDb::SAIGameConsts *pConsts )
{
	pScript = CreateScriptWrapper();
	pScript->Init();
	pScript->AddRegFunctions( pRegList );

	for ( int i = 0; i < pConsts->commonScriptFileRefs.size(); ++i )
		RunScriptFromFile( pConsts->commonScriptFileRefs[i].szScriptFileRef );
	// to launch all default functions and constants
	if ( pScript )
		pScript->Segment();
	//
	RunScriptFromFile( szScriptFileName );
}

bool CScripts::ParseCommand( SAIUnitCmd *pCmd, Script &script, bool bIDsAreScriptIDs ) 
{
	SAIUnitCmd &command = *pCmd;
	//
	command.nCmdType = EActionCommand( int( script.GetObject( 1 ) ) );
	bool bValid = true;
	switch ( command.nCmdType )
	{
	case ACTION_COMMAND_ENTRENCH_SELF:
		CHECK_ERROR( script.GetObject( 3 ).IsNumber(), "Give ACTION_COMMAND_ENTRENCH_SELF command : the third parameter is not ESpecialAbilityParam", script );
		pCmd->fNumber = script.GetObject( 3 );

		break;
	case ACTION_COMMAND_MOVE_TO:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_MOVE_TO command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_MOVE_TO command : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );

		break;
	case ACTION_COMMAND_ATTACK_UNIT:
		CHECK_ERROR( script.GetObject( 3 ).IsNumber(), "Give ACTION_COMMAND_ATTACK_UNIT command : the third parameter is not a script id", script );

		{
			const int targetId = script.GetObject( 3 );
			if ( bIDsAreScriptIDs )
			{
				pScripts->DelInvalidBegin( targetId );
				if ( !pScripts->groups[targetId].empty() )
					command.nObjectID = pScripts->groups[targetId].back()->GetUniqueId();
				else
					bValid = false;
			}
			else
			{
				command.nObjectID = targetId;
				bValid = command.nObjectID != 0;;
			}
		}

		break;
	case ACTION_COMMAND_ATTACK_OBJECT:
		{
			const int targetId = script.GetObject( 3 );

			if ( bIDsAreScriptIDs )
				command.nObjectID = pScripts->groups[targetId].back()->GetUniqueId();
			else
				command.nObjectID = targetId;
			bValid = command.nObjectID != 0;
		}

		break;
	case ACTION_COMMAND_SWARM_TO:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_SWARM_TO command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_SWARM_TO command : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );

		break;
	case ACTION_COMMAND_LOAD:
		CHECK_ERROR( script.GetObject( 3 ).IsNumber(), "Give ACTION_COMMAND_LOAD command : the third parameter is not a script id", script );

		{
			const int targetId = script.GetObject( 3 );
			if ( bIDsAreScriptIDs )
			{
				pScripts->DelInvalidBegin( targetId );
				if ( !pScripts->groups[targetId].empty() )
					command.nObjectID = pScripts->groups[targetId].back()->GetUniqueId();
			}
			else
				command.nObjectID = targetId;
			bValid = command.nObjectID != 0;
		}

		break;
	case ACTION_COMMAND_UNLOAD:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_UNLOAD command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_UNLOAD command : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );

		break;
	case ACTION_COMMAND_ENTER:
		CHECK_ERROR( script.GetObject( 3 ).IsNumber(), "Give ACTION_COMMAND_ENTER command : the third parameter is not a script id", script );

		{
			const int targetId = script.GetObject( 3 );
			if ( bIDsAreScriptIDs )
			{
				pScripts->DelInvalidBegin( targetId );
				if ( !pScripts->groups[targetId].empty() )
					command.nObjectID = pScripts->groups[targetId].back()->GetUniqueId();
			}
			else
				command.nObjectID = targetId;

			if ( command.nObjectID )
			{
				if ( dynamic_cast<CBuilding*>( GetObjectByCmd( command ) ) )
					command.fNumber = 0;
				else if ( dynamic_cast<CEntrenchmentPart*>( GetObjectByCmd( command ) ) )
					command.fNumber = 2;
				else
					CHECK_ERROR( false, "Give ACTION_COMMAND_ENTER command : the object to enter to is not buliding or entrenchment", script );
			}
			else
				bValid = false;
		}

		break;
	case ACTION_COMMAND_LEAVE:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_LEAVE command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_LEAVE пїЅommand : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );

		break;
	case ACTION_COMMAND_ROTATE_TO:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_ROTATE_TO command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_ROTATE_TO command : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );

		break;
	case ACTION_COMMAND_ROTATE_TO_DIR:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_ROTATE_TO_DIR command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_ROTATE_TO_DIR command : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );

		break;
	case ACTION_COMMAND_STOP:

		break;
	case ACTION_COMMAND_DIE:

		break;
	case ACTION_COMMAND_TAKE_ARTILLERY:
		CHECK_ERROR( script.GetObject( 3 ).IsNumber(), "Give ACTION_COMMAND_TAKE_ARTILLERY command : the third parameter is not a script id", script );
		{
			const int targetId = script.GetObject( 3 );
			if ( bIDsAreScriptIDs )
			{
				pScripts->DelInvalidBegin( targetId );

				if ( !pScripts->groups[targetId].empty() )
					command.nObjectID = pScripts->groups[targetId].back()->GetUniqueId();
			}
			else
				command.nObjectID = targetId;
			bValid = command.nObjectID != 0;
		}

		break;
	case ACTION_COMMAND_LOAD_NOW:
		CHECK_ERROR( script.GetObject( 3 ).IsNumber(), "Give ACTION_COMMAND_LOAD_NOW command : the third parameter is not a script id", script );
		{
			const int targetId = script.GetObject( 3 );
			if ( bIDsAreScriptIDs )
			{
				pScripts->DelInvalidBegin( targetId );
				if ( !pScripts->groups[targetId].empty() )
					command.nObjectID = pScripts->groups[targetId].back()->GetUniqueId();
			}
			else
				command.nObjectID = targetId;
			bValid = command.nObjectID != 0;
		}

		break;
	case ACTION_COMMAND_UNLOAD_NOW:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_UNLOAD_NOW command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_UNLOAD_NOW command : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );

		break;
	case ACTION_COMMAND_IDLE_BUILDING:
		CHECK_ERROR( script.GetObject( 3 ).IsNumber(), "ACTION_COMMAND_IDLE_BUILDING command : the third parameter is not a script id", script );

		{
			const int targetId = script.GetObject( 3 );
			if ( bIDsAreScriptIDs )
			{
				pScripts->DelInvalidBegin( targetId );
				if ( !pScripts->groups[targetId].empty() )
					command.nObjectID = pScripts->groups[targetId].back()->GetUniqueId();
			}
			else
				command.nObjectID = targetId;
			bValid = command.nObjectID != 0;

		}

		break;
	case ACTION_COMMAND_IDLE_TRENCH:
		CHECK_ERROR( script.GetObject( 3 ).IsNumber(), "Give ACTION_COMMAND_IDLE_TRENCH command : the third parameter is not a script id", script );

		{
			const int targetId = script.GetObject( 3 );
			if ( bIDsAreScriptIDs )
			{
				pScripts->DelInvalidBegin( targetId );
				if ( !pScripts->groups[targetId].empty() )
					command.nObjectID = pScripts->groups[targetId].back()->GetUniqueId();
			}
			else
				command.nObjectID = targetId;
			bValid = command.nObjectID != 0;
		}

		break;
	case ACTION_COMMAND_LEAVE_NOW:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_LEAVE_NOW command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_LEAVE_NOW command : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );

		break;
	case ACTION_COMMAND_DISAPPEAR:

		break;
	case ACTION_COMMAND_PLACEMINE:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_PLACEMINE command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_PLACEMINE command : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );

		break;
	case ACTION_COMMAND_AMBUSH:

		break;
	case ACTION_COMMAND_ART_BOMBARDMENT:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_ART_BOMBARDMENT command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_ART_BOMBARDMENT command : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );

		break;
	case ACTION_COMMAND_RANGE_AREA:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_RANGE_AREA command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_RANGE_AREA command : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );

		break;
	case ACTION_COMMAND_INSTALL:

		break;
	case ACTION_COMMAND_UNINSTALL:

		break;
	case ACTION_COMMAND_USE_SPYGLASS:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_USE_SPYGLASS command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_USE_SPYGLASS command : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );

		break;
	case ACTION_COMMAND_DISBAND_FORMATION:

		break;
	case ACTION_COMMAND_FORM_FORMATION:

		break;
	case ACTION_COMMAND_FOLLOW:
		CHECK_ERROR( script.GetObject( 3 ).IsNumber(), "Give ACTION_COMMAND_FOLLOW command : the third parameter is not a script id", script );

		{
			const int targetId = script.GetObject( 3 );
			if ( bIDsAreScriptIDs )
			{
				pScripts->DelInvalidBegin( targetId );
				if ( !pScripts->groups[targetId].empty() )
					command.nObjectID = pScripts->groups[targetId].back()->GetUniqueId();
			}
			else
				command.nObjectID = targetId;
			bValid = command.nObjectID != 0;
		}

		break;
	case ACTION_COMMAND_STAND_GROUND:

		break;
	case ACTION_COMMAND_DEPLOY_ARTILLERY:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_DEPLOY_ARTILLERY command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_DEPLOY_ARTILLERY command : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );
		command.fNumber = 0;

		break;
	case ACTION_COMMAND_CATCH_ARTILLERY:
		CHECK_ERROR( script.GetObject( 3 ).IsNumber(), "Give ACTION_COMMAND_CATCH_ARTILLERY command : the third parameter is not a script id", script );
		{
			const int targetId = script.GetObject( 3 );
			if ( bIDsAreScriptIDs )
			{
				pScripts->DelInvalidBegin( targetId );
				if ( !pScripts->groups[targetId].empty() )
					command.nObjectID = pScripts->groups[targetId].back()->GetUniqueId();
			}
			else
				command.nObjectID = targetId;
			command.fNumber = true; // use formation part
			bValid = command.nObjectID != 0;
		}

		break;
	case ACTION_COMMAND_REPAIR:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_REPAIR command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_REPAIR command : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );
		break;
	case ACTION_COMMAND_PATROL:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_PATROL command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_PATROL command : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );
		break;
	case ACTION_COMMAND_RESUPPLY:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_RESUPPLY command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_RESUPPLY command : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );
		break;
	case ACTION_COMMAND_CLEARMINE:
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "Give ACTION_COMMAND_CLEARMINE command : the 4 parameter is not an X coordinate", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "Give ACTION_COMMAND_CLEARMINE command : the 5 parameter is not an Y coordinate", script );

		command.vPos.x = script.GetObject( 4 );
		command.vPos.y = script.GetObject( 5 );
		break;
	case ACTION_COMMAND_WAIT:
		CHECK_ERROR( script.GetObject( 3 ).IsNumber(), "Give ACTION_COMMAND_WAIT command : the third parameter is not wait duration in seconds", script );

		pCmd->fNumber = script.GetObject( 3 );
		break;
	default:
		CHECK_ERROR( false, StrFmt( "Unknown command %d", command.nCmdType ), script );
	}
	return bValid;
}

bool CScripts::CanUnitLand( const SMapObjectInfo &mapObject, const CVec2 &vShift )
{
	const SUnitBaseRPGStats* pStats = dynamic_cast_ptr<const SUnitBaseRPGStats*>( mapObject.pObject ); 
	NI_ASSERT( pStats != 0, StrFmt( "Object \"%s\" (of %s) isn't a unit", NDb::GetResName(mapObject.pObject), typeid(*mapObject.pObject).name() ) );

	const CVec2 vCenter( mapObject.vPos.x + vShift.x, mapObject.vPos.y + vShift.y );
	if ( GetTerrain()->CanUnitGo( pStats->nBoundTileRadius, GetAIMap()->GetTile( vCenter), (EAIClasses)pStats->nAIPassabilityClass ) != FREE_NONE )
	{
		const float length = pStats->vAABBHalfSize.y * SConsts::BOUND_RECT_FACTOR;
		const float width = pStats->vAABBHalfSize.x * SConsts::BOUND_RECT_FACTOR;

		const CVec2 realDirVec( GetVectorByDirection( mapObject.nDir ) );
		const CVec2 dirPerp( realDirVec.y, -realDirVec.x );
		const CVec2 vShift( realDirVec * pStats->vAABBCenter.y + dirPerp * pStats->vAABBCenter.x );

		SRect unitRect;
		unitRect.InitRect( vCenter + vShift, realDirVec, length, width );

		for ( CUnitsIter<0,1> iter( 0, ANY_PARTY, vCenter, 2 * length ); !iter.IsFinished(); iter.Iterate() )
		{
			CAIUnit *pUnit = *iter;
			if ( ( !pStats->IsInfantry() || !pUnit->GetStats()->IsInfantry() ) && unitRect.IsIntersected( pUnit->GetUnitRect() ) )
				return false;
		}

		return true;
	}
	else
		return false;

}

bool CScripts::CanFormationLand( const SMapObjectInfo &mapObject, const CVec2 &vShift )
{
	const SSquadRPGStats* pStats = dynamic_cast_ptr<const SSquadRPGStats*>(mapObject.pObject);
	NI_ASSERT( pStats != 0, StrFmt( "Object \"%s\" (of %s) isn't a formation", NDb::GetResName(mapObject.pObject), typeid(*mapObject.pObject).name()) );

	const CVec2 vCenter( mapObject.vPos.x + vShift.x, mapObject.vPos.y + vShift.y );

	std::list<CVec2> centers;
	theUnitCreation.GetCentersOfAllFormationUnits( pStats, vCenter, mapObject.nDir, mapObject.nFrameIndex, -1, &centers );

	SMapObjectInfo soldierMapObject;
	soldierMapObject.pObject = pStats->members[0];
	for ( std::list<CVec2>::iterator iter = centers.begin(); iter != centers.end(); ++iter )
	{
		soldierMapObject.vPos = CVec3(*iter, 0 );
		if ( !CanUnitLand( soldierMapObject ) )
			return false;
	}

	return true;
}

bool CScripts::CanLandWithShift( const SMapObjectInfo &mapObject, CVec2 *pvShift )
{
	const SUnitBaseRPGStats *pStats = dynamic_cast_ptr<const SUnitBaseRPGStats*>(mapObject.pObject);
	if ( pStats != 0 )
	{
		*pvShift = VNULL2;
		if ( CanUnitLand( mapObject, *pvShift ) )
			return true;

		if ( !pStats->IsTrain() )
		{
			for ( int x = -4; x <= 4; ++x )
			{
				for ( int y = -4; y <= 4; ++y )
				{
					pvShift->x = x * 2 * SConsts::TILE_SIZE;
					pvShift->y = y * 2 * SConsts::TILE_SIZE;

					if ( CanUnitLand( mapObject, *pvShift ) )
						return true;
				}
			}
		}
	}
	else if ( dynamic_cast<const SSquadRPGStats*>(pStats) != 0 )
	{
		*pvShift = VNULL2;
		if ( CanFormationLand( mapObject, *pvShift ) )
			return true;

		for ( int x = -4; x <= 4; ++x )
		{
			for ( int y = -4; y <= 4; ++y )
			{
				pvShift->x = x * 2 * SConsts::TILE_SIZE;
				pvShift->y = y * 2 * SConsts::TILE_SIZE;

				if ( CanFormationLand( mapObject, *pvShift ) )
					return true;
			}
		}
	}
	else
		return true;

	return false;
}

bool IsUnit( const SMapObjectInfo &mapObject )
{
	return ( dynamic_cast_ptr<const SUnitBaseRPGStats*>(mapObject.pObject) != 0 );
}

void CScripts::LandSuspendedReiforcements()
{
	if ( curTime >= lastTimeToCheckSuspendedReinforcs + TIME_TO_CHECK_SUSPENDED_REINF )
	{
		lastTimeToCheckSuspendedReinforcs = curTime;
		if ( reinforcsIter == suspendedReinforcs.end() )
			reinforcsIter = suspendedReinforcs.begin();

		int cnt = 0;
		while ( reinforcsIter != suspendedReinforcs.end() && cnt < 1 )
		{
			if ( reinforcsIter->mapObject.link.nLinkWith == 0 )
			{
				++cnt;
				CVec2 vShift( VNULL2 );
				if ( CanLandWithShift( reinforcsIter->mapObject, &vShift ) )
				{
					std::unordered_set<int> candidates;
					const int nLink = reinforcsIter->mapObject.link.nLinkID;
					candidates.insert( nLink );
					CReinfList candObjects;

					bool bAdded = false;
					bool bCanLand = true;
					do
					{
						bAdded = false;
						
						CReinfList::iterator candreinforcsIter = suspendedReinforcs.begin();
						while ( candreinforcsIter != suspendedReinforcs.end() )
						{
							const int nLinkID = candreinforcsIter->mapObject.link.nLinkID;
							const int nLinkWith = candreinforcsIter->mapObject.link.nLinkWith;
							bool bReserve = reservePositions.find( nLinkID ) != reservePositions.end();

							if ( candidates.find( nLinkID ) == candidates.end() &&
									 ( nLinkWith != 0 && candidates.find( nLinkWith ) != candidates.end() ||
										 bReserve && candidates.find( reservePositions[nLinkID] ) != candidates.end() )
									)
							{
								if ( false )
//								if ( !CanLand( candreinforcsIter->mapObject, pIDB ) )
								{
									bCanLand = false;
									//
									suspendedReinforcs.splice( suspendedReinforcs.begin(), candObjects );
									break;
								}
								else
								{
									candidates.insert( nLinkID );
									CReinfList::iterator oldIter = candreinforcsIter++;
									//
									candObjects.splice( candObjects.begin(), suspendedReinforcs, oldIter );
									bAdded = true;
								}
							}
							else
								++candreinforcsIter;
						}
					} while ( bAdded && bCanLand );

					if ( bCanLand )
					{
						candObjects.push_back( *reinforcsIter );
						LandReinforcementWithoutLandCheck( &candObjects, vShift );
						reinforcsIter = suspendedReinforcs.erase( reinforcsIter );
					}
					else
						++reinforcsIter;
				}
				else
					++reinforcsIter;
			}
			else
				++reinforcsIter;
		}
	}
}

void CScripts::Segment()
{
	// retrieve all script calls
	{
		bool bOldShowScriptErrors = bShowErrors;
		bShowErrors = true;
		std::string szCommand;
		while ( ReadFromPipe( PIPE_SCRIPT_CMDS, &szCommand, 0 ) )
		{
			CallScriptFunction( szCommand.c_str() );
		}
		bShowErrors = bOldShowScriptErrors;
	}

	NHPTimer::STime time;
	NHPTimer::GetTime( &time );
	if ( pScript )
		pScript->Segment();
	const double tPassed = NHPTimer::GetTimePassed( &time );

	if ( bScriptTimeCalculate )
	{
		static std::vector<double> times( 20 );
		static int nCurrentPos = -1;
		static bool bCollected = false;

		++nCurrentPos;
		if ( nCurrentPos >= 20 )
		{
			nCurrentPos %= 20;
			bCollected = true;
		}
		times[nCurrentPos] = tPassed;

		if ( bCollected )
		{
			double tFullTime = 0;
			for ( int i = 0; i < 20; ++i )
				tFullTime += times[i];

			IDebugSingleton *pDebug = Singleton<IDebugSingleton>();
			if ( pDebug )
			{
				IStatsSystemWindow *pStatsSystemWindow = pDebug->GetStatsWindow();
				if ( pStatsSystemWindow )
					pStatsSystemWindow->UpdateEntry( L"ScriptTime", NStr::ToUnicode(StrFmt( "%.2f%%", float( 100.0f * tFullTime ) )), 0xff00ff00 );
			}
		}
	}
	LandSuspendedReiforcements();
}


void CScripts::CallScriptFunction( const char *pszCommand )
{
	pScript->CallScriptFunction( pszCommand, true );
}

void CScripts::DelInvalidBegin( const int targetId )
{
	if ( groups.find( targetId ) != groups.end() )
	{
		while ( !groups[targetId].empty() )
		{
			CUpdatableObj *pObj = groups[targetId].back();
			if ( pObj == 0 || !pObj->IsRefValid() || dynamic_cast<CStaticObject*>(pObj) == 0 && !pObj->IsAlive() )
				groups[targetId].pop_back();
			else
				break;
		}

		int nDeleted;
		for( std::unordered_map< int, int>::iterator it = groupUnits.begin(); it != groupUnits.end(); ++it )
		{
			const int nUniqueId = it->first;
			CLinkObject *pObj = GetObjectByUniqueIdSafe<CLinkObject>( nUniqueId );
			if ( !pObj || !pObj->IsRefValid() || dynamic_cast<CStaticObject*>(pObj) == 0 && !pObj->IsAlive() )
				nDeleted = nUniqueId;
			else
				break;
		}

		groupUnits.erase( nDeleted );
	}
}

void CScripts::DelInvalidUnits( const int scriptId )
{
	if ( groups.find( scriptId ) != groups.end() )
	{
		std::list<CPtr<CUpdatableObj> >::iterator iter = groups[scriptId].begin();
		while ( iter != groups[scriptId].end() )
		{
			CUpdatableObj *pObj = *iter;
			if ( pObj == 0 || !pObj->IsRefValid() || dynamic_cast<CStaticObject*>(pObj) == 0 && !pObj->IsAlive() )
				iter = groups[scriptId].erase( iter );
			else
				++iter;
		}

		std::list<int> deleted;
		for( std::unordered_map< int, int>::iterator it = groupUnits.begin(); it != groupUnits.end(); ++it )
		{
			const int nUniqueId = it->first;
			CLinkObject *pObj = GetObjectByUniqueIdSafe<CLinkObject>( nUniqueId );
			if ( !pObj || !pObj->IsRefValid() || dynamic_cast<CStaticObject*>(pObj) == 0 && !pObj->IsAlive() )
				deleted.push_back( nUniqueId );
		}

		while( !deleted.empty() )
		{
			groupUnits.erase( deleted.front() );
			deleted.pop_front();
		}
	}
}

int CScripts::GetCheckObjectsInScriptArea( const SScriptArea &area, const struct ICheckObjects &check )
{
	float fR = area.eType == EAT_CIRCLE ? area.fR : (std::max)( fabs(area.vAABBHalfSize.x), fabs(area.vAABBHalfSize.y) );
	int nResult = 0;
	for ( CStObjCircleIter<false> iter( area.vCenter, fR ); !iter.IsFinished(); iter.Iterate() )
	{
		CExistingObject *pObj = *iter;
		if ( pObj->IsRefValid() && pObj->IsAlive() && check.IsGoodObj( pObj ) )
			++nResult;
	}

	return nResult;
}

void CScripts::OutScriptError( const char *pszString )
{
	if ( bShowErrors )
	{
		CONSOLE_BUFFER_LOG2( CONSOLE_STREAM_CONSOLE, pszString, 0xffff0000, true );
	}
}

int CScripts::GetGameTime( struct lua_State *state )
{
	Script script(state);
	script.PushNumber( curTime / 1000 );
	return 1;
}

int CScripts::AddIronMan( struct lua_State *state )
{
	Script script(state);
	const int nScriptID = script.GetObject( 1 );
	theSupremeBeing.AddIronman( nScriptID );
	return 0;
}

int CScripts::GetNUnitsInCircle( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject(1).IsNumber(), "GetNUnitsInCircle : the first parameter is not a number", script );
	CHECK_ERROR( script.GetObject(2).IsNumber(), "GetNUnitsInCircle : the second parameter is not a number", script );
	CHECK_ERROR( script.GetObject(3).IsNumber(), "GetNUnitsInCircle : the third parameter is not a number", script );
	CHECK_ERROR( script.GetObject(4).IsNumber(), "GetNUnitsInCircle : the fourth parameter is not a number", script );

	const float fR = script.GetObject( -1 );
	const CVec2 center( script.GetObject( -3 ), script.GetObject( -2 ) );
	const int nPlayer = script.GetObject( -4 );

	int cnt = 0;

	for ( CUnitsIter<0,2> iter( 0, ANY_PARTY, center, fR ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( pUnit->IsRefValid() && pUnit->IsAlive() &&
				 pUnit->GetPlayer() == nPlayer && !pUnit->GetStats()->IsAviation() )
		{
			if ( fabs2( (*iter)->GetCenterPlain() - center ) <= fR * fR )
				++cnt;
		}
	}

	script.PushNumber( cnt );
	
	return 1;
}
/*

int CScripts::StopSound( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "StopSound: the 1st parameter is not a sound ID", 1 );
	Singleton<ISoundScene>()->RemoveSound( script.GetObject( 1 ).GetInteger() );
	return 0;
}

int CScripts::StartSound( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsString(), "StartSound: the 1st parameter is not a sound sound name in map", 1 );
	CHECK_ERROR( script.GetObject(2).IsNumber(), "StartSound: the 2nd parameter is not a x", 1 );
	CHECK_ERROR( script.GetObject(3).IsNumber(), "StartSound: the 3rd parameter is not a y", 1 );
	CHECK_ERROR( script.GetObject(4).IsNumber(), "StartSound: the 4th parameter is not a z", 1 );
	CHECK_ERROR( script.GetObject(5).IsNumber(), "StartSound: the 5th parameter is not a ESoundMixType", 1 );
	CHECK_ERROR( script.GetObject(6).IsNumber(), "StartSound: the 6th parameter is not a ESoundAddMode", 1 );
	
	
	const string szSoundID = script.GetObject( 1 ).GetString();
	int nSound = -1;
	for ( int i = 0; i < pMapInfo->scriptSounds.size(); ++i )
	{
		if ( pMapInfo->scriptSounds[i].szSoundName == szSoundID )
		{
			nSound = i;
			break;
		}
	}
	CHECK_ERROR( nSound != -1, StrFmt( "StartSound cannot find sound \"%s\"", szSoundID.c_str() ) );

	const CVec3 vPos( script.GetObject( 2 ).GetNumber(), script.GetObject( 3 ).GetNumber(), script.GetObject( 4 ).GetNumber() );
	const ESoundMixType eMixMode( ESoundMixType( script.GetObject( 5 ).GetInteger() ) );
	const ESoundAddMode eAddMode( ESoundAddMode( script.GetObject( 6 ).GetInteger() ) );
	
	const int nID = Singleton<ISoundScene>()->AddSound( pMapInfo->scriptSounds[nSound], vPos, eMixMode, eAddMode, 0 );
	script.PushNumber( nID );
	return 1;
}*/

int CScripts::GetNUnitsInScriptGroup( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject(1).IsNumber(), "GetNUnitsInScriptGroup: the first parameter is not a number", script );
	
	const int nScriptID = script.GetObject( 1 );

	int nNumber = 0;	
	if ( pScripts->groups.find( nScriptID ) != pScripts->groups.end() )
	{
		pScripts->DelInvalidUnits( nScriptID );

		if ( script.GetObject( 2 ).IsNumber() )
		{
			const int nPlayer	= script.GetObject( 2 );
			for ( std::list<CPtr<CUpdatableObj> >::iterator it = pScripts->groups[nScriptID].begin();
					it != pScripts->groups[nScriptID].end(); ++it )
			{
				if ( (*it)->GetPlayer() == nPlayer )
					++nNumber;
			}
		}
		else
		{
			for ( std::list<CPtr<CUpdatableObj> >::iterator it = pScripts->groups[nScriptID].begin();
						it != pScripts->groups[nScriptID].end(); ++it )
			{
				if ( (*it)->IsAlive() )
					++nNumber;
			}
		}
	}

	script.PushNumber( nNumber );
	
	return 1;
}

void CScripts::SetNewLinksToReinforcement( CReinfList *pReinf, std::unordered_map<int, int> *pOld2NewLinks )
{
	// set new links (not intersected with existing)
	std::list<int> freeLinks;
	CLinkObject::GetFreeLinks( &freeLinks, pReinf->size() );
	for ( CReinfList::iterator iter = pReinf->begin(); iter != pReinf->end(); ++iter )
	{
		(*pOld2NewLinks)[iter->mapObject.link.nLinkID] = freeLinks.front();
		iter->mapObject.link.nLinkID = freeLinks.front();
		freeLinks.pop_front();
	}

	for ( CReinfList::iterator iter = pReinf->begin(); iter != pReinf->end(); ++iter )
	{
		const int nLinkWith = iter->mapObject.link.nLinkWith;
		if ( pOld2NewLinks->find( nLinkWith ) != pOld2NewLinks->end() )
			iter->mapObject.link.nLinkWith = (*pOld2NewLinks)[nLinkWith];
	}
}

void CScripts::LandReinforcementWithoutLandCheck( CReinfList *pReinf, const CVec2 &vShift )
{
	std::unordered_map<int, int> old2NewLinks;
	SetNewLinksToReinforcement( pReinf, &old2NewLinks );

	std::list<CCommonUnit*> pUnits;

	// land reinforcement

	LinkInfo linksInfo;
	CReinfList transports;
	//
	for ( CReinfList::iterator iter = pReinf->begin(); iter != pReinf->end(); ++iter )
	{
		const SUnitBaseRPGStats *pStats = dynamic_cast_ptr<const SUnitBaseRPGStats*>( iter->mapObject.pObject );
		if ( !pStats || !pStats->IsTransport() )
		{
			iter->mapObject.vPos += CVec3( vShift, 0.0f );
			const int nUniqueID = ++SLinkObjDataAutoMagic::pLinkObjData->nCurUniqueID;
			CObjectBase *pUnit = dynamic_cast<CAILogic*>(Singleton<IAILogic>())->AddObject( nUniqueID, iter->mapObject, &linksInfo, false, iter->pStats );
			
			if ( pUnit )
			{
				if ( CCommonUnit *pUnit1 = dynamic_cast<CCommonUnit*>(pUnit) )
				{
					pUnits.push_back( pUnit1 );
				}
			}
		}
		else
			transports.push_back( *iter );
	}

	//
	for ( CReinfList::iterator iter = transports.begin(); iter != transports.end(); ++iter )
	{
		iter->mapObject.vPos += CVec3( vShift, 0.0f );
		const int nUniqueID = ++SLinkObjDataAutoMagic::pLinkObjData->nCurUniqueID;
		CObjectBase *pUnit = 
			dynamic_cast<CAILogic*>(Singleton<IAILogic>())->AddObject( nUniqueID, iter->mapObject, &linksInfo, false, iter->pStats );
		
		if ( pUnit )
		{
			if ( CCommonUnit *pUnit1 = dynamic_cast<CCommonUnit*>(pUnit) )
				pUnits.push_back( pUnit1 );
		}
	}


	dynamic_cast<CAILogic*>(Singleton<IAILogic>())->InitLinks( linksInfo );
	dynamic_cast<CAILogic*>(Singleton<IAILogic>())->InitReservePositions( old2NewLinks );
	dynamic_cast<CAILogic*>(Singleton<IAILogic>())->InitStartCommands( linksInfo, old2NewLinks );

	theSupremeBeing.GiveNewUnitsToGenerals( pUnits );
}

void CScripts::SendShowReinoforcementPlacementFeedback( std::list<CVec2> *pCenters )
{
	CVec2 vMostLeftCenter = pCenters->front();

	for ( std::list<CVec2>::iterator iter = pCenters->begin(); iter != pCenters->end(); ++iter )
	{
		if ( vMostLeftCenter.x > iter->x )
			vMostLeftCenter = *iter;
	}

	int nUnits = 0;
	CVec2 vGroupCenter( VNULL2 );

	std::list<CVec2>::iterator iter = pCenters->begin();
	while ( iter != pCenters->end() )
	{
		if ( fabs2( vMostLeftCenter - (*iter) ) < sqr( SConsts::REINFORCEMENT_GROUP_DISTANCE ) )
		{
			vGroupCenter += *iter;
			++nUnits;

			iter = pCenters->erase( iter );
		}
		else
			++iter;
	}

	vGroupCenter /= nUnits;
	
	updater.AddUpdate( EFB_REINFORCEMENT_CENTER_LOCAL_PLAYER, MAKELONG( vGroupCenter.x, vGroupCenter.y ) );
}

int CScripts::LandReinforcementOLD( struct lua_State *state )
{
	Script script( state );
	
	CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "LandReinforcementOLD: the first parameter is not a number", script );

	const int nRein = script.GetObject( 1 );
	CHECK_ERROR( pScripts->reinforcs.find( nRein ) != pScripts->reinforcs.end(), StrFmt( "Wrong number of reinforcement, %d", nRein ), script );

	WORD playersOfReinforcement = 0;
	bool bSendAck = false;
	std::list<CVec2> centers;
	for ( CReinfList::iterator iter = pScripts->reinforcs[nRein].begin(); iter != pScripts->reinforcs[nRein].end(); ++iter )
	{
		const int nPlayer = iter->mapObject.nPlayer;
		if ( theDipl.IsPlayerExist( nPlayer ) )
		{
			pScripts->suspendedReinforcs.push_front( *iter );

			//
			if ( nPlayer == theDipl.GetMyNumber() )
				bSendAck = true;

			if ( nPlayer == theDipl.GetMyNumber() )
				centers.push_back( CVec2( iter->mapObject.vPos.x, iter->mapObject.vPos.y ) );

			playersOfReinforcement = playersOfReinforcement | ( 1 << nPlayer );
		}
	}

	if ( bSendAck )
	{
		updater.AddUpdate( EFB_REINFORCEMENT_ARRIVED );
		while ( !centers.empty() )
			pScripts->SendShowReinoforcementPlacementFeedback( &centers );
	}

	for ( int i = 0; i < theDipl.GetNPlayers(); ++i )
	{
		if ( playersOfReinforcement & 1 )
		{
			NI_ASSERT( theDipl.IsPlayerExist( i ), "Wrong number of player" );
			theStatistics.ReinforcementUsed( i );
		}

		playersOfReinforcement >>= 1;
	}
	
	//
	if ( pScripts->lastTimeToCheckSuspendedReinforcs != curTime )
	{
		pScripts->lastTimeToCheckSuspendedReinforcs = 0;
		pScripts->reinforcsIter = pScripts->suspendedReinforcs.begin();
		pScripts->LandSuspendedReiforcements();
	}

	return 0;
}

int CScripts::Draw( struct lua_State *state )
{
	Script script( state );
	NInput::PostEvent( "local_draw", 0, 0 );

	return 0;
}

int CScripts::Win( struct lua_State *state )
{
	Script script( state );
	
	CHECK_ERROR( script.GetObject( 1 ).IsNumber(), "Win: the first parameter is not a number", script );
	const int nParty = script.GetObject( 1 );

	if ( nParty == theDipl.GetMyParty() )
	{
		if ( GetScenarioTracker()->GetGameType() == IAIScenarioTracker::EGT_SINGLE )
			NInput::PostEvent( "local_win", 0, 0 );
		else
			NInput::PostEvent( "multiplayer_win", 0, 0 );
			//Singleton<IMPToUIManager>()->AddUIMessage( new SMPUISimpleMessage( EMUI_CRAP_END_GAME ) );
		//updater.AddUpdate( EFB_WIN );
	}
	else
	{
		if ( GetScenarioTracker()->GetGameType() == IAIScenarioTracker::EGT_SINGLE )
			NInput::PostEvent( "local_loose", 0, 0 );
		else
			NInput::PostEvent( "multiplayer_loose", 0, 0 );
			//Singleton<IMPToUIManager>()->AddUIMessage( new SMPUISimpleMessage( EMUI_CRAP_END_GAME ) );
		//updater.AddUpdate( EFB_LOOSE );
	}

	return 0;
}

int CScripts::Loose( struct lua_State *state )
{
	if ( GetScenarioTracker()->GetGameType() == IAIScenarioTracker::EGT_SINGLE )
		NInput::PostEvent( "local_loose", 0, 0 );
	//else
			//Singleton<IMPToUIManager>()->AddUIMessage( new SMPUISimpleMessage( EMUI_CRAP_END_GAME ) );

	return 0;
}

int CScripts::ProcessCommand( struct lua_State *state, const bool bPlaceInQueue, const bool b2NdParamIsScriptID )
{
	Script script( state );

	CHECK_ERROR( script.GetObject(1).IsNumber(), "GiveCommand : the first parameter is not a command", script );
	CHECK_ERROR( script.GetObject(2).IsNumber(), "GiveCommand : the second parameter is not a script id", script );

	// Cmd should work only in Single and LANTest
	CHECK_ERROR( Singleton<IAIScenarioTracker>()->GetGameType() == IAIScenarioTracker::EGT_SINGLE 
		|| NGlobal::GetVar( "LANTEST", 0 ) != 0, "Cmd: works only in single-player", script );

	const float fDispersion = script.GetObject( 3 ).IsNumber() ? script.GetObject( 3 ) : 0.0f;

	int nGroup = -1;
	std::list< std::pair<CCommonUnit*, int> > oldUnitsGroups;

	SAIUnitCmd command;
	const bool bValid = ParseCommand( &command, script, b2NdParamIsScriptID );

	if ( bValid )
	{
		if ( b2NdParamIsScriptID )
		{
			const int scriptId = script.GetObject( 2 );
			//
			if ( scriptId != -1 && pScripts->groups.find( scriptId ) != pScripts->groups.end() )
			{
				pScripts->DelInvalidUnits( scriptId );
				// group registration
				std::vector<int> objIDs( pScripts->groups[scriptId].size() );
				int nLen = 0;
				for ( std::list<CPtr<CUpdatableObj> >::iterator iter = pScripts->groups[scriptId].begin(); iter != pScripts->groups[scriptId].end(); ++iter )
				{
					CHECK_ERROR( dynamic_cast_ptr<CCommonUnit*>(*iter) != 0, "Can't give command to non-unit", script );
					CCommonUnit *pUnit = dynamic_cast_ptr<CCommonUnit*>(*iter);
					objIDs[nLen++] = pUnit->GetUniqueId();
					oldUnitsGroups.push_back( std::pair<CCommonUnit*, int>( pUnit, pUnit->GetNGroup() ) );
				}
				
				if ( fDispersion != 0.0f ) // apply dispersion to command
				{
					std::vector<int> id(1);
					SAIUnitCmd tmpCmd( command );
					for ( int i = 0; i < objIDs.size(); ++i )
					{
						id[0] = objIDs[i];
						tmpCmd.vPos = command.vPos + NRandom::Random( fDispersion ) * GetVectorByDirection( NRandom::Random( 0, 65535 ) );
						const int nGroup = dynamic_cast<CAILogic*>(Singleton<IAILogic>())->GenerateGroupNumber();
						Singleton<IAILogic>()->RegisterGroup( id, nGroup );
						Singleton<IAILogic>()->GroupCommand( &tmpCmd, nGroup, bPlaceInQueue );
						Singleton<IAILogic>()->UnregisterGroup( nGroup );
						Singleton<IAILogic>()->SetNeedNewGroupNumber();
					}
					return 0;	
				}
				else
				{
					nGroup = dynamic_cast<CAILogic*>(Singleton<IAILogic>())->GenerateGroupNumber();
					Singleton<IAILogic>()->RegisterGroup( objIDs, nGroup );
				}
			}
		}
		else
		{
			const int nUniqueID = script.GetObject( 2 );
			// command to 1 unit
			CCommonUnit *pUnit = checked_cast<CCommonUnit*>( CLinkObject::GetObjectByUniqueId(nUniqueID) );
			if ( pUnit )
			{
				std::vector<int> units( 1 );
				units[0] = pUnit->GetUniqueId();
				oldUnitsGroups.push_back( std::pair<CCommonUnit*, int>( pUnit, pUnit->GetNGroup() ) );
				nGroup = Singleton<IAILogic>()->GenerateGroupNumber();
				Singleton<IAILogic>()->RegisterGroup( units, nGroup );
				if ( fDispersion != 0.0f )
				{
					SAIUnitCmd tmpCmd( command );
					tmpCmd.vPos = command.vPos + NRandom::Random( fDispersion ) * GetVectorByDirection( NRandom::Random( 0, 65535 ) );
					Singleton<IAILogic>()->GroupCommand( &tmpCmd, nGroup, bPlaceInQueue );
					Singleton<IAILogic>()->UnregisterGroup( nGroup );
					Singleton<IAILogic>()->SetNeedNewGroupNumber();
					return 0;
				}
			}
		}
	}

	if ( bValid )
	{
		if ( nGroup != -1 )
		{
			Singleton<IAILogic>()->GroupCommand( &command, nGroup, bPlaceInQueue );
			Singleton<IAILogic>()->UnregisterGroup( nGroup );
			Singleton<IAILogic>()->SetNeedNewGroupNumber();
		}
	}

	for ( std::list< std::pair<CCommonUnit*, int> >::iterator iter = oldUnitsGroups.begin(); iter != oldUnitsGroups.end(); ++iter )
		theGroupLogic.AddUnitToGroup( iter->first, iter->second );

	return 0;
}

int CScripts::GiveCommand( struct lua_State *state )
{
	return ProcessCommand( state, false, true );
}

int CScripts::GiveQCommand( struct lua_State *state )
{
	return ProcessCommand( state, true, true );
}

int CScripts::ShowActiveScripts( struct lua_State *state )
{
//	for ( hash_map<string, int>::iterator iter = pScripts->name2pScript->begin(); iter != pScripts->name2pScript->end(); ++iter )
	//	pScripts->pConsole->WriteASCII( CONSOLE_STREAM_CONSOLE, iter->first.c_str(), 0xff00ff00 );

	return 0;
}

class CUnitsEnumerator : public IEnumerator
{
	Script *pScript;
	int nResult;
	std::unordered_set<int> squads;
public:
	CUnitsEnumerator( Script *_pScript ) : pScript( _pScript ), nResult( 0 ) {}
	bool AddUnit( CAIUnit *pUnit )
	{
		if ( pUnit->IsInfantry() && pUnit->GetFormation() )
		{
			const int nSquadID = pUnit->GetFormation()->GetUniqueId();
			if ( squads.find( nSquadID ) == squads.end() )
			{
				++nResult;
				squads.emplace(nSquadID);
			}
		}
		else
			++nResult;
		return false;
	}

	void Done()
	{
		pScript->PushNumber( nResult );
	}
};

class CUnitsInScriptAreaCounter : public IScriptAreaEnumerator
{
	CUnitsEnumerator enumerator;
	const NDb::SScriptArea &area;
public:
	CUnitsInScriptAreaCounter( Script *_pScript, const NDb::SScriptArea &_area ) : enumerator( _pScript ), area( _area ) {}
	const NDb::SScriptArea & GetArea() const { return area; }
	bool AddUnit( CAIUnit *pUnit ) { return enumerator.AddUnit( pUnit ); }
	void Done() { enumerator.Done(); }
};

bool CScripts::IsUnitInCirlceArea( CAIUnit *pUnit, const CVec2 &vCenter, const int fR, const int nPlayer )
{
	return pUnit->IsRefValid() && pUnit->IsAlive() &&
		pUnit->GetPlayer() == nPlayer && //!pUnit->GetStats()->IsAviation() && 
		pUnit->GetUnitRect().IsIntersectCircle( vCenter, fR );
}

bool CScripts::IsUnitInRectArea( CAIUnit *pUnit, const SRect &rect, const int nPlayer )
{
	return pUnit->IsRefValid() && pUnit->IsAlive() &&
		( nPlayer == -1 || pUnit->GetPlayer() == nPlayer ) && //!pUnit->GetStats()->IsAviation() && 
		pUnit->GetUnitRect().IsIntersected( rect );
}

bool IsUnitQualified( bool bCountPlanes, CAIUnit *pUnit )
{
	return bCountPlanes || 
		( !pUnit->GetStats()->IsAviation() && // don't count planes
			pUnit->GetState()->GetName() != EUSN_PARTROOP ) && // paratroopers during landing
			(!pUnit->IsInfantry() || !checked_cast<CSoldier*>(pUnit)->IsInTransport() ||
			!checked_cast<CSoldier*>(pUnit)->GetTransportUnit()->GetStats()->IsAviation() );// paratroopers in planes
}

void CScripts::EnumUnitsInCircle( IEnumerator *pEnumerator, const int nPlayer, bool bCountPlanes, const CVec2 &vCenter, const float fRadius )
{
	for ( CUnitsIter<0,2> iter( 0, ANY_PARTY, vCenter, fRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( IsValidObj( pUnit ) && pUnit->GetPlayer() == nPlayer && IsUnitInCirlceArea( pUnit, vCenter, fRadius, nPlayer ) && IsUnitQualified( false, pUnit ) )
		{
			if ( pEnumerator->AddUnit( pUnit ) )
			{
				pEnumerator->Done();
				return;
			}
		}
	}
	if ( bCountPlanes )
	{
		for ( CPlanesIter iter; !iter.IsFinished(); iter.Iterate() )
		{
			CAviation *pUnit = *iter;
			if ( IsValidObj( pUnit ) && pUnit->GetPlayer() == nPlayer && IsUnitInCirlceArea( pUnit, vCenter, fRadius, nPlayer ) )
			{
				if ( pEnumerator->AddUnit( pUnit ) )
				{
					pEnumerator->Done();
					return;
				}
			}
		}
	}
	pEnumerator->Done();
}

void CScripts::EnumUnitsInRect( IEnumerator *pEnumerator, const int nPlayer, bool bCountPlanes, const SRect &rect )
{
	for ( CUnitsIter<0,2> iter( 0, ANY_PARTY, rect.center, 1.5f * (std::max)( rect.width, rect.lengthAhead + rect.lengthBack ) ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( IsValidObj(pUnit) && pUnit->GetPlayer() == nPlayer && IsUnitInRectArea( pUnit, rect, nPlayer ) && IsUnitQualified( false, pUnit ) )
		{
			if ( pEnumerator->AddUnit( pUnit ) )
			{
				pEnumerator->Done();
				return;
			}
		}
	}
	if ( bCountPlanes )
	{
		for ( CPlanesIter iter; !iter.IsFinished(); iter.Iterate() )
		{
			CAviation *pUnit = *iter;
			if ( IsValidObj(pUnit) && pUnit->GetPlayer() == nPlayer && IsUnitInRectArea( pUnit, rect, nPlayer ) )
			{
				if ( pEnumerator->AddUnit( pUnit ) )
				{
					pEnumerator->Done();
					return;
				}
			}
		}
	}
	pEnumerator->Done();
}

void CScripts::EnumUnitsInScriptArea( IScriptAreaEnumerator *pEnumerator, const int nPlayer, bool bCountPlanes )
{
	const NDb::SScriptArea &area = pEnumerator->GetArea();
	if ( area.eType == EAT_CIRCLE ) 
		EnumUnitsInCircle( pEnumerator, nPlayer, bCountPlanes, area.vCenter, area.fR );
	else
	{
		SRect areaRect;
		areaRect.InitRect( area.vCenter, CVec2( 1, 0 ), fabs(area.vAABBHalfSize.x), fabs(area.vAABBHalfSize.y) );
		EnumUnitsInRect( pEnumerator, nPlayer, bCountPlanes, areaRect );
	}
}

int CScripts::IsSomeUnitInArea( struct lua_State *state )
{
	Script script( state );


	CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "IsSomeUnitInArea: the first parameter is not a number", script );

	const int nPlayer = script.GetObject( 1 );

	if ( script.GetObject( 2 ).IsNumber() )
	{
		CHECK_ERROR( script.GetObject(2).IsNumber(), "IsSomeUnitInArea : the second parameter is not a X", script );
		CHECK_ERROR( script.GetObject(3).IsNumber(), "IsSomeUnitInArea : the third parameter is not a Y", script );
		CHECK_ERROR( script.GetObject(4).IsNumber(), "IsSomeUnitInArea : the fourth parameter is not a R", script );
		const bool bCountPlanes = script.GetObject( 5 ).IsNumber() ? script.GetObject( 5 ) : true;
		const float fR = script.GetObject( 4 );
		const CVec2 center( script.GetObject( 2 ), script.GetObject( 3 ) );
		bool bFound = false;
		for ( CUnitsIter<0,2> iter( 0, ANY_PARTY, center, fR ); !iter.IsFinished(); iter.Iterate() )
		{
			CAIUnit * pUnit = *iter;
			if ( pUnit->IsRefValid() && pUnit->GetPlayer() == nPlayer && pUnit->IsAlive() && pUnit->GetPlayer() == nPlayer )
			{
				bFound = true;
				break;
			}
		}
		script.PushNumber( bFound );
	}
	else
	{
		const std::string szName = script.GetObject( 2 );
		const bool bCountPlanes = script.GetObject( 3 ).IsNumber() ? script.GetObject( 3 ) : true;
		CHECK_ERROR( pScripts->areas.find( szName ) != pScripts->areas.end(), StrFmt( "GetNUnitsInArea: wrong script area name (%s)", szName.c_str() ), script );
		const NDb::SScriptArea &area = pScripts->areas[szName];
		CIsSomeUnitInScriptAreaEnumerator cnt( &script, &area );

		EnumUnitsInScriptArea( &cnt, nPlayer, bCountPlanes );
	}
	return 1;
}

int CScripts::GetNUnitsInArea( struct lua_State *state )
{
	Script script( state );
	
	CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "GetNUnitsInArea: the first parameter is not a number", script );
	const int nPlayer = script.GetObject( 1 );


	if ( script.GetObject( 2 ).IsNumberOnly() )
	{
		const int nPlayer = script.GetObject( 1 );
		CHECK_ERROR( script.GetObject(2).IsNumber(), "GetNUnitsInArea : the second parameter is not a X", script );
		CHECK_ERROR( script.GetObject(3).IsNumber(), "GetNUnitsInArea : the third parameter is not a Y", script );
		CHECK_ERROR( script.GetObject(4).IsNumber(), "GetNUnitsInArea : the fourth parameter is not a R", script );

		const float fRadius = script.GetObject( 4 );
		const CVec2 vCenter( script.GetObject( 2 ), script.GetObject( 3 ) );
		const bool bCountPlanes = script.GetObject( 5 ).IsNumber() ? script.GetObject( 5 ) : true;

		CUnitsEnumerator cnt( &script );
		EnumUnitsInCircle( &cnt, nPlayer, bCountPlanes, vCenter, fRadius );
	}
	else
	{
		CHECK_ERROR( script.GetObject( 2 ).IsString( ), "GetNUnitsInArea: the second parameter is not a string", script );
		const std::string szName = script.GetObject( 2 );
		CHECK_ERROR( pScripts->areas.find( szName ) != pScripts->areas.end(), StrFmt( "GetNUnitsInArea: wrong script area name (%s)", szName.c_str() ), script );

		const bool bCountPlanes = script.GetObject( 3 ).IsNumber() ? script.GetObject( 3 ) : true;

		const NDb::SScriptArea &area = pScripts->areas[szName];
		CUnitsInScriptAreaCounter cnt( &script, area );
		EnumUnitsInScriptArea( &cnt, nPlayer, bCountPlanes );
	}
	return 1;
}

int CScripts::GetNScriptUnitsInArea( struct lua_State *state )
{
	Script script( state );
	CHECK_ERROR( script.GetObject( 1 ).IsNumber(), "GetNScriptUnitsInArea: the first parameter is not a number", script );
	

	float fR = 0;
	CVec2 vCenter( VNULL2 );
	const int nScriptGroup = script.GetObject( 1 );
	std::string szName;
	bool bCountPlanes;
	bool bCircle = false;

	CHECK_ERROR( nScriptGroup >= 0, StrFmt( "GetNScriptUnitsInArea: wrong number of script group (%d)", nScriptGroup ), script );

	if ( script.GetObject( 2 ).IsNumberOnly() )
	{
		CHECK_ERROR( script.GetObject( 2 ).IsNumber(), "GetNScriptUnitsInArea: the 2 parameter is not a X", script );
		CHECK_ERROR( script.GetObject( 3 ).IsNumber(), "GetNScriptUnitsInArea: the 3 parameter is not a Y", script );
		CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "GetNScriptUnitsInArea: the 4 parameter is not a R", script );
		bCountPlanes = script.GetObject( 5 ).IsNumber() ? script.GetObject( 5 ) : true;
		fR = script.GetObject( 4 );
		vCenter = CVec2( script.GetObject( 2 ), script.GetObject( 3 ) );
		bCircle = true;
	}
	else
	{
		CHECK_ERROR( script.GetObject( 2 ).IsString(), "GetNScriptUnitsInArea: the 2 parameter is not a string", script );
		szName = script.GetObject( 2 ).GetString();
		CHECK_ERROR( pScripts->areas.find( szName ) != pScripts->areas.end(), StrFmt( "GetNScriptUnitsInArea: wrong script area name (%s)", szName.c_str() ), script );
		bCountPlanes = script.GetObject( 3 ).IsNumber() ? script.GetObject( 3 ) : true;
		const NDb::SScriptArea &area = pScripts->areas[szName];
		if ( area.eType == EAT_CIRCLE )
		{
			fR = area.fR;
			vCenter = area.vCenter;
			bCircle = true;
		}
	}

	pScripts->DelInvalidUnits( nScriptGroup );


	if ( pScripts->groups.find( nScriptGroup ) == pScripts->groups.end() || pScripts->groups[nScriptGroup].empty() )
		script.PushNumber( 0 );
	else
	{
		int nResult = 0;
		if ( bCircle )
		{
			// CRAP{ ??? static objects ??? dynamic_cast
			for ( std::list< CPtr<CUpdatableObj> >::iterator iter = pScripts->groups[nScriptGroup].begin(); iter != pScripts->groups[nScriptGroup].end(); ++iter )
			{
				if ( CFormation *pFormation = dynamic_cast_ptr<CFormation*>( *iter ) )
				{
					for ( int i = 0; i < pFormation->Size(); ++i )
					{
						CSoldier * pUnit = (*pFormation)[i];
						if ( IsUnitQualified( bCountPlanes, pUnit ) && 
								pUnit->GetUnitRect().IsIntersectCircle( vCenter, fR ) )
						{
							++nResult;
						}
					}
				}
				else if ( CAIUnit *pUnit = dynamic_cast_ptr<CAIUnit*>( *iter ) )
				{
					if ( IsUnitQualified( bCountPlanes, pUnit ) && 
								pUnit->GetUnitRect().IsIntersectCircle( vCenter, fR ) )
						++nResult;
				}
			}
			// }CRAP
		}
		else
		{
			const NDb::SScriptArea &area = pScripts->areas[szName];
			SRect areaRect;
			areaRect.InitRect( area.vCenter, CVec2( 1, 0 ), fabs(area.vAABBHalfSize.x), fabs(area.vAABBHalfSize.y) );
			// CRAP{ ??? static objects ??? dynamic_cast
			for ( std::list<CPtr<CUpdatableObj> >::iterator iter = pScripts->groups[nScriptGroup].begin(); iter != pScripts->groups[nScriptGroup].end(); ++iter )
			{
				if ( CFormation *pFormation = dynamic_cast_ptr<CFormation*>( *iter ) )
				{
					for ( int i = 0; i < pFormation->Size(); ++i )
					{
						CSoldier * pUnit = (*pFormation)[i];
						if ( IsUnitQualified( bCountPlanes, pUnit ) && IsUnitInRectArea( pUnit, areaRect, -1 ) )
						{
							++nResult;
						}
					}
				}
				else if ( CAIUnit *pUnit = dynamic_cast_ptr<CAIUnit*>( *iter ) )
				{
					if ( IsUnitQualified( bCountPlanes, pUnit ) && IsUnitInRectArea( pUnit, areaRect, -1 ) )
						++nResult;
				}
			}
			// }CRAP
		}

		script.PushNumber( nResult );	
	}
		
	return 1;
}

int CScripts::ChangeWarFog( struct lua_State *state )
{
	Script script( state );
	
	CHECK_ERROR( script.GetObject( 1 ).IsNumber(   ), "ChangeWarFog: the first parameter is not a number", script );

	const int nParty = script.GetObject( 1 );
	CHECK_ERROR( nParty < 3, StrFmt( "ChangeWarFog: wrong number of party (%d)", nParty ), script );

	theCheats.SetNPartyForWarFog( nParty, false );

	return 0;
}

int CScripts::ChangePlayer( struct lua_State *state )
{
	Script script( state );
	
	CHECK_ERROR( script.GetObject( 1 ).IsNumber(   ), "ChangePlayer: the first parameter is not a number", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(   ), "ChangePlayer: the second parameter is not a number", script );

	const int nUniqueID = script.GetObject( 1 );
	const int nPlayer = script.GetObject( 2 );

	if ( CLinkObject::IsLinkObjectExists( nUniqueID ) )
	{
		CLinkObject *pObj = CLinkObject::GetObjectByUniqueId( nUniqueID );
		{
			CDynamicCast<CCommonUnit> pUnit( pObj );
			if ( pUnit )
			{
				pUnit->ChangePlayer( nPlayer );
				if ( nPlayer == theDipl.GetMyNumber() )
				{
					theFeedBackSystem.AddFeedbackAndForget( nUniqueID, pUnit->GetCenterPlain(), EFB_UNITS_GIVEN, -1 );
				}
			}
		}
		{
			CDynamicCast<CBuildingSimple> pBuilding( pObj );
			if ( pBuilding )
			{
				NI_ASSERT( pBuilding->GetNDefenders() == 0, StrFmt( "cannot change player for building %i, soldiers are inside", nUniqueID ) );
				if ( pBuilding->GetNDefenders() == 0 )
					pBuilding->ChangePlayer( nPlayer );
			}
		}
	}

	return 0;
}

int CScripts::GetPassangers( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(   ), "GetPassangers: the first parameter is not a number", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(   ), "GetPassangers: the second parameter is not a number", script );

	const int nUniqueID = script.GetObject( 1 );
	const int nPlayer = script.GetObject( 2 );

	std::unordered_map<int,bool> ids;

	if ( CLinkObject::IsLinkObjectExists( nUniqueID ) )
	{
		CLinkObject *pObj = CLinkObject::GetObjectByUniqueId( nUniqueID );
		{
			CDynamicCast<CMilitaryCar> pUnit( pObj );
			if ( pUnit )
			{
				for ( int i = 0; i < pUnit->GetNPassengers(); ++i )
				{
					if ( pUnit->GetPlayer() == nPlayer && pUnit->GetPassenger( i ) && pUnit->GetPassenger( i )->GetFormation() )
						ids[pUnit->GetPassenger( i )->GetFormation()->GetUniqueID()] = true;
				}
				for ( int i = 0; i < pUnit->GetNBoarded(); ++i )
					if ( pUnit->GetPlayer() == nPlayer && pUnit->GetBoarded( i ) )
						ids[pUnit->GetBoarded( i )->GetUniqueID()] = true;
			}
		}
		{
			CDynamicCast<CBuildingSimple> pBuilding( pObj );
			if ( pBuilding )
			{
				for ( int i = 0; i < pBuilding->GetNDefenders(); ++i )
				{
					if ( pBuilding->GetUnit( i ) && pBuilding->GetUnit( i )->GetPlayer() == nPlayer && pBuilding->GetUnit( i )->GetFormation() )
						ids[pBuilding->GetUnit( i )->GetFormation()->GetUniqueID()] = true;
				}
			}
		}
	}

	for ( std::unordered_map<int,bool>::const_iterator it = ids.begin(); it != ids.end(); ++it )
		script.PushNumber( it->first );	
	return ids.size();
}

int CScripts::God( struct lua_State *state )
{
	Script script( state );
	
	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "God: the first parameter is not a number", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(  ), "God: the second parameter is not a number", script );

	const int nPlayer = script.GetObject( 1 );
	const int nMode = script.GetObject( 2 );

	CHECK_ERROR( nPlayer >= 0 && nPlayer < theDipl.GetNPlayers(), StrFmt( "God: wrong nubmer of party (%d), total number of parties (%d)", nPlayer, theDipl.GetNPlayers() ), script );
	CHECK_ERROR( nMode >= 0 && nMode <= 5, StrFmt( "God: wrong nubmer of mode (%d), total number of modes (%d)", nMode, 5 ), script );

	// nMode = 0 - ??? god mode ???
	// nMode = 1 - ???
	// nMode = 2 - ???
	// nMode = 3 - /??
	// nMode = 4 - ???
	// nMode = 5 - ???

	switch ( nMode )
	{
		case 0: theCheats.SetImmortals( nPlayer, 0 ); theCheats.SetFirstShoot( nPlayer, 0 ); break;
		case 1: theCheats.SetImmortals( nPlayer, 1 ); break;
		case 2: theCheats.SetImmortals( nPlayer, 1 ); theCheats.SetFirstShoot( nPlayer, 1 ); break;
		case 3: theCheats.SetFirstShoot( nPlayer, 1 ); break;
		case 4: theCheats.SetImmortals( nPlayer, 0 ); break;
		case 5: theCheats.SetFirstShoot( nPlayer, 0 ); break;
	}
	
	return 0;
}

int CScripts::SetIGlobalVar( struct lua_State *state )
{
	Script script( state );
	
	CHECK_ERROR( script.GetObject( 1 ).IsString( ), "SetIGlobalVar: the first parameter is not a string", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(  ), "SetIGlobalVar: the second parameter is not a number", script );

	NGlobal::SetVar( script.GetObject( 1 ).GetString(), int(script.GetObject( 2 )), STORAGE_SAVE );

	return 0;
}

int CScripts::SetFGlobalVar( struct lua_State *state )
{
	Script script( state );
	
	CHECK_ERROR( script.GetObject( 1 ).IsString( ), "SetFGlobalVar: the first parameter is not a string", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(   ), "SetFGlobalVar: the second parameter is not a number", script );

	NGlobal::SetVar( script.GetObject( 1 ).GetString(), float(script.GetObject( 2 ).GetNumber()), STORAGE_SAVE );

	return 0;
}

int CScripts::SetSGlobalVar( struct lua_State *state )
{
	Script script( state );
	
	CHECK_ERROR( script.GetObject( 1 ).IsString(), "SetSGlobalVar: the first parameter is not a string", script );
	CHECK_ERROR( script.GetObject( 2 ).IsString( ), "SetSGlobalVar: the second parameter is not a number", script );

	NGlobal::SetVar( script.GetObject( 1 ).GetString(), (const char *)(script.GetObject( 2 )), STORAGE_SAVE );

	return 0;
}

int CScripts::GetIGlobalVar( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsString( ), "GetIGlobalVar: the first parameter is not a string", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(   ), "GetIGlobalVar: the second parameter is not a number", script );

	script.PushNumber( NGlobal::GetVar( script.GetObject(1).GetString(), int(script.GetObject(2)) ) );

	return 1;
}

int CScripts::GetFGlobalVar( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsString(  ), "GetFGlobalVar: the first parameter is not a string", script );
	CHECK_ERROR( script.GetObject( 2 ).IsString(  ), "GetFGlobalVar: the second parameter is not a number", script );

	script.PushNumber( NGlobal::GetVar( script.GetObject(1).GetString(), float(script.GetObject(2)) ) );

	return 1;
}

int CScripts::GetSGlobalVar( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsString(   ), "GetSGlobalVar: the first parameter is not a string", script );
	CHECK_ERROR( script.GetObject( 2 ).IsString(  ), "GetSGlobalVar: the second parameter is not a string", script );

	script.PushString( NStr::ToMBCS( NGlobal::GetVar( script.GetObject(1).GetString(), (const char*)(script.GetObject(2)) ) ).c_str() );

	return 1;
}

int CScripts::GetObjectHPs( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "GetObjectHPs: the first parameter is not a number", script );
	const int nUniquieID = script.GetObject( 1 );
	if ( CLinkObject::IsLinkObjectExists( nUniquieID ) )
	{
		CLinkObject *pObj = CLinkObject::GetObjectByUniqueId( nUniquieID );
		CStaticObject * pStatObj = dynamic_cast<CStaticObject*>( pObj );
		CAIUnit *pUnit = dynamic_cast<CAIUnit*>( pObj );
		if ( pStatObj )
			script.PushNumber( pStatObj->GetHitPoints() );
		else if ( pUnit )
			script.PushNumber( pUnit->GetHitPoints() );
		else if ( CDynamicCast<CFormation> pFormation = pObj )
		{
			float fHP = 0;
			for ( int i = 0; i < pFormation->Size(); ++i )
				fHP += (*pFormation)[i]->GetHitPoints();
			script.PushNumber( fHP );
		}
		else
			script.PushNumber( 0 );
	}
	else
		script.PushNumber( 0 );

	return 1;
}

int CScripts::IsSomePlayerUnit( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "IsSomePlayerUnit: the first parameter is not a number", script );
	const int nPlayer = script.GetObject( 1 );
	CHECK_ERROR( nPlayer < pMapInfo->players.size(), StrFmt( "IsSomePlayerUnit: wrong number of party (%d)", nPlayer ), script );

	bool bPresent = false;
	for ( CGlobalIter iter( theDipl.GetNParty( nPlayer ), EDI_FRIEND ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( pUnit->IsRefValid() && pUnit->IsAlive() && pUnit->GetPlayer() == nPlayer )
		{
			bPresent = true;
			break;
		}
	}
	script.PushNumber( bPresent );
	return 1;
}


int CScripts::IsSomeUnitInParty( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "GetNUnitsInParty: the first parameter is not a number", script );
	const int nParty = script.GetObject( 1 );
	CHECK_ERROR( nParty < 3, StrFmt( "GetNUnitsInParty: wrong number of party (%d)", nParty ), script );

	bool bPresent = false;
	for ( CGlobalIter iter( nParty, EDI_FRIEND ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( pUnit->IsRefValid() && pUnit->IsAlive() )
		{
			bPresent = true;
			break;
		}
	}

	script.PushNumber( bPresent );

	return 1;
}
inline CVec2 ToCVec2( const CVec3 &v3 )
{
	return CVec2( v3.x, v3.y );
}

int CScripts::IsUnitNearScriptObject( struct lua_State *state )
{
	Script script( state );
	script.PushNumber( 1 );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "IsUnitNearScriptObject: the 1st parameter is not a player number", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(  ), "IsUnitNearScriptObject: the 2nd parameter is not a script ID", script );
	CHECK_ERROR( script.GetObject( 3 ).IsNumber(  ), "IsUnitNearScriptObject: the 2nd parameter is not a radius", script );

	const int nPlayer = script.GetObject( 1 );
	const int nScriptGroup = script.GetObject( 2 );
	const int nRadius = script.GetObject( 3 );
	
	CScriptGroups::const_iterator groupPos =  groups.find( nScriptGroup );
	int nUnits = 0;
	if ( groupPos != groups.end() )
	{
		SAINotifyPlacement placement;

		const CScriptGroup &group = groupPos->second;
		for ( CScriptGroup::const_iterator it = group.begin(); it != group.end(); ++it )
		{
			CVec2 vCenter;
			CDynamicCast<CStaticObject> pStatObj( *it );
			CDynamicCast<CCommonUnit> pUnit( *it );
			if ( pStatObj )
				vCenter = CVec2( pStatObj->GetCenter().x, pStatObj->GetCenter().y ) ;
			if ( pUnit )
				vCenter = pUnit->GetCenterPlain();
			for ( CUnitsIter<0,2> iter( 0, ANY_PARTY, vCenter, nRadius ); !iter.IsFinished(); iter.Iterate() )
			{
				if ( IsValidObj( *iter ) && (*iter)->GetPlayer() == nPlayer )
				{
					script.PushNumber( 1 );
					return 1;
				}
			}
		}
	}

	script.PushNumber( 0 );
	return 1;
}

int CScripts::GetNUnitsInParty( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "GetNUnitsInParty: the first parameter is not a number", script );
	const int nParty = script.GetObject( 1 );
	CHECK_ERROR( nParty < 3, StrFmt( "GetNUnitsInParty: wrong number of party (%d)", nParty ), script );

	int cnt = 0;
	for ( CGlobalIter iter( nParty, EDI_FRIEND ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( pUnit->IsRefValid() && pUnit->IsAlive() )
		{
			IUnitState *pState = pUnit->GetStats()->IsInfantry() ? pUnit->GetFormation()->GetState() : pUnit->GetState();
			if ( !pState || pState->GetName() != EUSN_GUN_CREW_STATE )
				++cnt;
		}
	}

	script.PushNumber( cnt );

	return 1;
}

int CScripts::GetNUnitsInPartyUF( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "GetNUnitsInParty: the first parameter is not a number", script );
	const int nParty = script.GetObject( 1 );
	CHECK_ERROR( nParty < 3, StrFmt( "GetNUnitsInParty: wrong number of party (%d)", nParty ), script );

	int cnt = 0;
	for ( CGlobalIter iter( nParty, EDI_FRIEND ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( pUnit->IsRefValid() && pUnit->IsAlive() && ( !pUnit->GetStats()->IsInfantry() || pUnit == (*pUnit->GetFormation())[0] ) )
		{
			IUnitState *pState = pUnit->GetStats()->IsInfantry() ? pUnit->GetFormation()->GetState() : pUnit->GetState();
			if ( !pState || pState->GetName() != EUSN_GUN_CREW_STATE )
				++cnt;
		}
	}

	script.PushNumber( cnt );

	return 1;
}

int CScripts::GetNUnitsInPlayerUF( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(   ), "GetNUnitsInParty: the first parameter is not a number", script );
	const int nPlayer = script.GetObject( 1 );

	int cnt = 0;
	for ( CGlobalIter iter( theDipl.GetNParty( nPlayer ), EDI_FRIEND ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( pUnit->IsRefValid() && pUnit->IsAlive() && pUnit->GetPlayer() == nPlayer &&
				 ( !pUnit->GetStats()->IsInfantry() || pUnit == (*pUnit->GetFormation())[0] ) )
		{
			IUnitState *pState = pUnit->GetStats()->IsInfantry() ? pUnit->GetFormation()->GetState() : pUnit->GetState();
			if ( !pState || pState->GetName() != EUSN_GUN_CREW_STATE )
				++cnt;
		}
	}

	script.PushNumber( cnt );

	return 1;
}

void GetTraceFormatResult( Script *pScript, std::string *pResult )
{
	Script &script = *pScript;

	const char *pStr = script.GetObject( 1 );
	
	switch ( pScript->GetTop() )
	{
		case 1:	*pResult = pStr;
			break;
		case 2: *pResult = StrFmt( pStr, float(script.GetObject( 2 )) );
			break;
		case 3: *pResult = StrFmt( pStr, float(script.GetObject( 2 )), float(script.GetObject( 3 )) );
			break;
		case 4: *pResult = StrFmt( pStr, float(script.GetObject( 2 )), float(script.GetObject( 3 )), float(script.GetObject( 4 )) );
			break;
		case 5: *pResult = StrFmt( pStr, float(script.GetObject( 2 )), float(script.GetObject( 3 )), float(script.GetObject( 4 )), 
																	float(script.GetObject( 5 )) );
			break;
		case 6: *pResult = StrFmt( pStr, float(script.GetObject( 2 )), float(script.GetObject( 3 )), float(script.GetObject( 4 )), 
																	float(script.GetObject( 5 )), float(script.GetObject( 6 )) );
			break;
		case 7: *pResult = StrFmt( pStr, float(script.GetObject( 2 )), float(script.GetObject( 3 )), float(script.GetObject( 4 )), 
																	float(script.GetObject( 5 )), float(script.GetObject( 6 )), float(script.GetObject( 7 )) );
			break;
		case 8: *pResult = StrFmt( pStr, float(script.GetObject( 2 )), float(script.GetObject( 3 )), float(script.GetObject( 4 )), 
																	float(script.GetObject( 5 )), float(script.GetObject( 6 )), float(script.GetObject( 7 )), float(script.GetObject( 8 )) );
			break;
		case 9: *pResult = StrFmt( pStr, float(script.GetObject( 2 )), float(script.GetObject( 3 )), float(script.GetObject( 4 )), 
																	float(script.GetObject( 5 )), float(script.GetObject( 6 )), float(script.GetObject( 7 )), float(script.GetObject( 8 )), float(script.GetObject( 9 )) );
			break;
		case 10: *pResult = StrFmt( pStr, float(script.GetObject( 2 )), float(script.GetObject( 3 )), float(script.GetObject( 4 )), 
																	float(script.GetObject( 5 )), float(script.GetObject( 6 )), float(script.GetObject( 7 )), float(script.GetObject( 8 )), float(script.GetObject( 9 )), float(script.GetObject( 10 )) );
			break;
		default:
			break;
	}
}

int CScripts::Trace( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsString(   ), "Trace: the first parameter is not a number", script );
	for ( int i = 2; i <= script.GetTop(); ++i )
	{
		// it asserts on Trace("%i",0). removed.
		NI_ASSERT( script.GetObject( i ).IsNil() ? script.GetObject( i ) != 0 : true, "NILL value passed, check sintaxis" );
		CHECK_ERROR( script.GetObject( i ).IsNumber(  ), StrFmt( "Trace: the %d parameter is not a number", i ), script );
	}
	std::string result;
	GetTraceFormatResult( &script, &result );

	pScripts->pConsole->WriteASCII( CONSOLE_STREAM_CONSOLE, result.c_str(), 0xff00ff00 );
	DebugTrace( "%s", result.c_str() );

	return 0;
}

int CScripts::DisplayTrace( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsString(  ), "DisplayTrace: the first parameter is not a number", script );
	for ( int i = 2; i <= script.GetTop(); ++i )
		CHECK_ERROR( script.GetObject( i ).IsNumber( ), StrFmt( "DisplayTrace: the %d parameter is not a number", i ), script );

	std::string result;
	GetTraceFormatResult( &script, &result );

	const std::string szSeasonName = NStr::ToMBCS( NGlobal::GetVar( "World.Season", "" ) );
	const DWORD dwColor = NGlobal::GetVar( ("Scene.Colors." + szSeasonName + ".Text.Chat.Color").c_str(), int(0xffffffff) );
	CONSOLE_BUFFER_LOG2( PIPE_CHAT, result.c_str(), dwColor, false );

	return 0;
}

int CScripts::ChangeFormation( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "ChangeFormationOrder: first parameter isn't a number", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber( ), "ChangeFormationOrder: second parameter isn't a number", script );

	const int nScriptID = script.GetObject( 1 );
	const int nGeometry = script.GetObject( 2 );

	CHECK_ERROR( pScripts->groups.find( nScriptID ) != pScripts->groups.end(), StrFmt( "ChangeFormationOrder: wrong script id (%d)", nScriptID ), script );
	pScripts->DelInvalidUnits( nScriptID );

	for ( std::list<CPtr<CUpdatableObj> >::iterator iter = pScripts->groups[nScriptID].begin(); iter != pScripts->groups[nScriptID].end(); ++iter )
	{
		if ( CFormation *pFormation = dynamic_cast_ptr<CFormation*>(*iter) )
		{
			if ( nGeometry < pFormation->GetGeometriesCount() )
				pFormation->ChangeGeometry( nGeometry );
		}
	}

	return 0;
}

int CScripts::ObjectiveChanged( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "ObjectiveChanged: first parameter isn't a number", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(  ), "ObjectiveChanged: second parameter isn't a number", script );

	const int nObj = script.GetObject( 1 );
	const int nValue = script.GetObject( 2 );
	
	CHECK_ERROR( nObj >= 0 && nObj < 255, StrFmt( "ObjectiveChanged: wrong number of objective (%d)", nObj ), script );
	CHECK_ERROR( nValue >= 0 && nValue < 255, StrFmt( "ObjectiveChanged: wrong value of objective (%d)", nValue ), script );

	updater.AddUpdate( EFB_OBJECTIVE_CHANGED, ( nObj << 8 ) | nValue, 0 );

	return 0;
}

int CScripts::GetNAmmo( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "GetNAmmo: first paramter isn't a nubmer", script );

	const int nScriptID = script.GetObject( 1 );
	CHECK_ERROR( pScripts->groups.find( nScriptID ) != pScripts->groups.end(), StrFmt( "GetNAmmo: wrong script id (%d)", nScriptID ), script );
	pScripts->DelInvalidBegin( nScriptID );

	if ( pScripts->groups[nScriptID].empty() )
	{
		script.PushNumber( 0 );
		script.PushNumber( 0 );
	}
	else
	{
		CUpdatableObj *pObj = *(pScripts->groups[nScriptID].begin());
		int nMainAmmo = 0;
		int nSecondaryAmmo = 0;

		if ( CDynamicCast<CAIUnit> pUnit = pObj )
		{
			for ( int i = 0; i < pUnit->GetNCommonGuns(); ++i )
			{
				if ( pUnit->GetCommonGunStats( i ).bIsPrimary )
					nMainAmmo += pUnit->GetNAmmo( i );
				else
					nSecondaryAmmo += pUnit->GetNAmmo( i );
			}
		}
		else if ( CDynamicCast<CFormation> pFormation = pObj )
		{
			for ( int i = 0; i < pFormation->Size(); ++i )
			{
				CAIUnit *pUnit = (*pFormation)[i];
				for ( int i = 0; i < pUnit->GetNCommonGuns(); ++i )
				{
					if ( pUnit->GetCommonGunStats( i ).bIsPrimary )
						nMainAmmo += pUnit->GetNAmmo( i );
					else
						nSecondaryAmmo += pUnit->GetNAmmo( i );
				}
			}
		}
		script.PushNumber( nMainAmmo );
		script.PushNumber( nSecondaryAmmo );
	}

	return 2;
}

int CScripts::GetPartyOfUnits( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "GetPartyOfUnits: first parameter isn't a number", script );

	const int nScriptID = script.GetObject( 1 );
	CHECK_ERROR( pScripts->groups.find( nScriptID ) != pScripts->groups.end(), StrFmt( "GetPartyOfUnits: wrong script id (%d)", nScriptID ), script );

	CUpdatableObj *pObj = *(pScripts->groups[nScriptID].begin());
	CHECK_ERROR( dynamic_cast<CCommonUnit*>(pObj) != 0, StrFmt( "GetPartyOfUnits: first object in script group (%d) is not a unit", nScriptID ), script );

	CCommonUnit *pUnit = dynamic_cast<CCommonUnit*>(pObj);
	script.PushNumber( pUnit->GetParty() );

	return 1;
}

int CScripts::SetCatchArtFlag( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "SetArtilleryCatchFlag: first parameter isn't a number", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber( ), "SetArtilleryCatchFlag: second parameter isn't a number", script );

	const int nScriptID = script.GetObject( 1 );
	const DWORD dwCatchArtFlag = script.GetObject( 2 );

	if ( pScripts->groups.find( nScriptID ) != pScripts->groups.end() )
	{
		pScripts->DelInvalidUnits( nScriptID );
		CScriptGroups::iterator pos = pScripts->groups.find( nScriptID );
		for ( std::list<CPtr<CUpdatableObj> >::iterator it = pos->second.begin(); it != pos->second.end(); ++it )
		{
			if ( CDynamicCast<CFormation> pFormation = *it )
				pFormation->SetCatchArtFlag( dwCatchArtFlag );
		}
	}

	return 0;
}

int CScripts::DamageObject( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "DamageObject: first parameter isn't a number", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber( ), "DamageObject: second parameter isn't a number", script );

	const int nUniqueID = script.GetObject( 1 );
	const float fDamage = script.GetObject( 2 );

	CLinkObject *pUpdatableObj = CLinkObject::GetObjectByUniqueId( nUniqueID );
	if ( CStaticObject *pObj = dynamic_cast<CStaticObject*>(pUpdatableObj) )
	{
		if ( fDamage == 0.0f )
			pObj->TakeDamage( pObj->GetHitPoints() * 2, true, theDipl.GetNeutralPlayer(), 0 );
		else if ( fDamage < 0.0f )
			pObj->SetHitPoints( pObj->GetHitPoints() - fDamage );
		else
			pObj->TakeDamage( fDamage, true, theDipl.GetNeutralPlayer(), 0 );
	}
	else if ( CAIUnit *pUnit = dynamic_cast<CAIUnit*>(pUpdatableObj) )
	{
		if ( fDamage == 0.0f )
			pUnit->TakeDamage( pUnit->GetHitPoints() * 2, 0, theDipl.GetNeutralPlayer(), 0 );
		else if ( fDamage < 0.0f )
			pUnit->IncreaseHitPoints( -fDamage );
		else
			pUnit->TakeDamage( fDamage, 0, theDipl.GetNeutralPlayer(), 0 );
	}
	else if ( CFormation *pFormation = dynamic_cast<CFormation*>(pUpdatableObj) )
	{
		std::list<CSoldier*> soldiers;
		for ( int i = 0; i < pFormation->Size(); ++i )
			soldiers.push_back( (*pFormation)[i] );
		for ( std::list<CSoldier*>::iterator iter = soldiers.begin(); iter != soldiers.end(); ++iter )
		{
			CSoldier *pSoldier = *iter;

			if ( fDamage == 0.0f )
				pSoldier->TakeDamage( pSoldier->GetHitPoints() * 2, 0, theDipl.GetNeutralPlayer(), 0 );
			else if ( fDamage < 0.0f )
				pSoldier->IncreaseHitPoints( -fDamage );
			else
				pSoldier->TakeDamage( fDamage, 0, theDipl.GetNeutralPlayer(), 0 );
		}
	}

	return 0;
}

int CScripts::CallAssert( struct lua_State *pState )
{
	NI_ASSERT( false, "You are welcome!" );

	return 0;
}

int CScripts::GetUnitRPGStats( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "GetUnitState: first parameter isn't a number", script );
	const int nUniqueID = script.GetObject( 1 );

	if ( CLinkObject::IsLinkObjectExists( nUniqueID ) )
	{
		CLinkObject *pObj = CLinkObject::GetObjectByUniqueId( nUniqueID );

		// ???: 
		// 1) bool IsUnit (true for Unit, false for Formation)
		// 2) Id ??? (MechUnitRPGStats, SquadRPGStats), 
		// 3) ??? (UnitType, SquadType - ???), 
		// 4) Price (??? Price ???), 
		// 5) MaxHP (???), 
		// 6) Weight (???), 
		// 7) TowingForce (???)

		CCommonUnit * pUnit = dynamic_cast<CCommonUnit*>( pObj );
		if ( pUnit )
		{
			if ( pUnit->IsFormation() )
			{
				CFormation *pFormation = checked_cast<CFormation*>( pUnit );
				const SSquadRPGStats *pStats = pFormation->GetStats();
				script.PushNumber( 0 ); // 1
				script.PushNumber( -1 ); //2
				script.PushNumber( pStats->eSquadType ); // 3
				float fPrice = 0;
				for ( int i = 0; i < pFormation->Size(); ++i )
					fPrice += (*pFormation)[i]->GetPrice();
				script.PushNumber( fPrice ); // 4
				script.PushNumber( 0 ); // 5
				script.PushNumber( 0 ); // 6
				script.PushNumber( 0 ); // 7
			}
			else
			{
				CAIUnit *pSingleUnit = checked_cast<CAIUnit*>( pUnit );
				if ( !pSingleUnit->IsInfantry() )
				{
					const SMechUnitRPGStats *pStats = checked_cast<const SMechUnitRPGStats*>( pSingleUnit->GetStats() );
					script.PushNumber( 1 ); //
					script.PushNumber( -1 ); //2
					script.PushNumber( pStats->eUnitType ); // 3
					script.PushNumber( pStats->fPrice ); // 4
					script.PushNumber( pStats->fMaxHP ); // 5
					script.PushNumber( pStats->fWeight ); // 6
					script.PushNumber( pStats->fTowingForce ); // 7
				}
			}
		}
	}
	else
		script.PushNumber( -1 );

	return script.GetPushCount();
}

int CScripts::GetSquadStates( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "GetSquadStates: first parameter isn't a number", script );
	const int nUniqueID= script.GetObject( 1 );

	
	
	if ( !CLinkObject::IsLinkObjectExists( nUniqueID ) )
		script.PushNumber( -1 );
	else
	{
		CDynamicCast<CFormation> pFormation = CLinkObject::GetObjectByUniqueId( nUniqueID );
		CHECK_ERROR( pFormation != 0, "GetSquadStates: scriptID object isn't a unit", script );

		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			CSoldier *pSoldier = (*pFormation)[i];
			if ( IsValid( pSoldier ) )
			{
				IUnitState *pState = pSoldier->GetState();
				if ( IsValid( pState ) )
					script.PushNumber( pState->GetName() );
				else
					script.PushNumber( -1 );
			}
			else
				script.PushNumber( -1 );
		}

	}

	return script.GetPushCount();
}

int CScripts::GetUnitState( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "GetUnitState: first parameter isn't a number", script );
	const int nUniqueID= script.GetObject( 1 );

	if ( !CLinkObject::IsLinkObjectExists( nUniqueID ) )
		script.PushNumber( -1 );
	else
	{
		CLinkObject *pObj = CLinkObject::GetObjectByUniqueId( nUniqueID );

		CHECK_ERROR( dynamic_cast<CQueueUnit*>(pObj) != 0, "GetUnitState: scriptID object isn't a unit", script );
		CQueueUnit *pUnit = dynamic_cast<CQueueUnit*>(pObj);

		IUnitState *pState = pUnit->GetState();

		if ( pState == 0 || !pState->IsRefValid() )
			script.PushNumber( -1 );
		else
			script.PushNumber( pState->GetName() );
	}

	return 1;
}

int CScripts::GetSquadInfo( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "GetSquadInfo: first parameters isn't a number", script );
	const int nScriptID = script.GetObject( 1 );

	pScripts->DelInvalidUnits( nScriptID );
	
	if ( pScripts->groups.find( nScriptID ) == pScripts->groups.end() || pScripts->groups[nScriptID].empty() )
		script.PushNumber( -3 );
	else
	{
		CUpdatableObj *pObj = pScripts->groups[nScriptID].front();
		CFormation *pFormation = dynamic_cast<CFormation*>( pObj );
		if ( pFormation == 0 )
			script.PushNumber( -2 );
		else if ( pFormation->IsDisabled() || pFormation->Size() == 1 && (*pFormation)[0]->GetMemFormation() != 0 )
			script.PushNumber( -1 );
		else
			script.PushNumber( pFormation->GetCurrentGeometry() );
	}

	return 1;
}

int CScripts::IsFollowing( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(   ), "IsFollowing: first parameters isn't a number", script );
	const int nScriptID = script.GetObject( 1 );

	pScripts->DelInvalidUnits( nScriptID );

	if ( pScripts->groups.find( nScriptID ) == pScripts->groups.end() || pScripts->groups[nScriptID].empty() )
		script.PushNumber( -1 );
	else
	{
		CUpdatableObj *pObj = pScripts->groups[nScriptID].front();
		CCommonUnit *pUnit = dynamic_cast<CCommonUnit*>(pObj);
		if ( pUnit == 0 )
			script.PushNumber( -1 );
		else if ( pUnit->IsInFollowState() )
			script.PushNumber( 1 );
		else
			script.PushNumber( 0 );
	}

	return 1;
}

int CScripts::GetFrontDir( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "IsFollowing: first parameters isn't a number", script );
	const int nUniqueID = script.GetObject( 1 );

	if ( !CLinkObject::IsLinkObjectExists( nUniqueID ) )
		script.PushNumber( 0 );
	else
	{
		CLinkObject *pObj = CLinkObject::GetObjectByUniqueId( nUniqueID );
		
		CBasePathUnit *pUnit = dynamic_cast<CBasePathUnit*>( pObj );
		if ( pUnit == 0 )
			script.PushNumber( 0 );
		else
			script.PushNumber( pUnit->GetFrontDirection() );
	}

	return 1;
}

int CScripts::GetActiveShellType( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(   ), "GetActiveShellType: first parameter isn't a number", script );
	const int nScriptID = script.GetObject( 1 );

	pScripts->DelInvalidUnits( nScriptID );
	CHECK_ERROR( pScripts->groups.find( nScriptID ) != pScripts->groups.end(), StrFmt( "GetActiveShellType: object with scriptID (%d) doesn't exist", nScriptID ), script );
	CHECK_ERROR( !pScripts->groups[nScriptID].empty(), StrFmt( "GetActiveShellType: object with scriptID (%d) doesn't exist", nScriptID ), script );

	CUpdatableObj *pObj = pScripts->groups[nScriptID].front();
	CAIUnit *pUnit = dynamic_cast<CAIUnit*>( pObj );

	CHECK_ERROR( pUnit != 0, StrFmt( "GetActiveShellType: object with scriptID (%d) isn't a unit", nScriptID ), script );

	script.PushNumber( pUnit->GetGuns()->GetActiveShellType() );

	return 1;
}

int CScripts::AskClient( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsString(  ), "AskClient: first parameter isn't a string", script );
	std::string szRequest = script.GetObject( 1 );

	Singleton<IConsoleBuffer>()->WriteASCII( PIPE_CONSOLE_CMDS, szRequest.c_str() );

	return 0;
}

int CScripts::RandomFloat( struct lua_State *pState )
{
	Script script( pState );

	script.PushNumber( NRandom::Random( 0.0f, 1.0f ) );

	return 1;
}

int CScripts::RandomInt( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "RandomInt: the first parameter isn't a number", script );
	const int n = script.GetObject( 1 );
	CHECK_ERROR( n >= 1, StrFmt( "RandomInt: upper parameter (%d) is too small", n ), script );

	if ( n == 1 )
		script.PushNumber( 0 );
	else
		script.PushNumber( NRandom::Random( 0, n - 1 ) );

	return 1;
}

int CScripts::ChangeSelection( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "ChangeSelection: first parameter isn't a number", script );
	const int nScriptID = script.GetObject( 1 );

	CHECK_ERROR( script.GetObject( 2 ).IsNumber(   ), "ChangeSelection: second parameters isn't a number", script );
	const int nParam = script.GetObject( 2 );

	pScripts->DelInvalidUnits( nScriptID );

	if ( pScripts->groups.find( nScriptID ) != pScripts->groups.end() )
	{
		for ( std::list<CPtr<CUpdatableObj> >::iterator iter = pScripts->groups[nScriptID].begin(); iter != pScripts->groups[nScriptID].end(); ++iter )
			updater.AddUpdate( 0, ACTION_NOTIFY_CHANGE_SELECTION, *iter, nParam );
	}

	return 0;
}

int CScripts::ReturnScriptIDs( struct lua_State *pState )
{
	Script script( pState );

	const int nReturns = script.GetTop();
	std::unordered_set<int> selectedUnits;
	for ( int i = 1; i <= nReturns; ++i )
	{
		NI_ASSERT( script.GetObject( i ).IsNumber(  ), "ReturnScriptIDs: %d parameter isn't a number" );
		
		const int nPtr = script.GetObject( i );
		CObjectBase *pObj = reinterpret_cast<CObjectBase*>( nPtr );

		NI_ASSERT( dynamic_cast<CUpdatableObj*>(pObj) != 0, "Unknown object passed" );
		CUpdatableObj *pUpdatableObject = dynamic_cast<CUpdatableObj*>(pObj);

		const int nScriptID = pScripts->GetScriptID( pUpdatableObject );
		if ( nScriptID != -1 )
			selectedUnits.insert( nScriptID );
	}

	while ( !selectedUnits.empty() )
	{
		const int nScriptID = *selectedUnits.begin();
		selectedUnits.erase( nScriptID );

		pScripts->CallScriptFunction( StrFmt( "GetSelectedUnitsFeedBack( %d )", nScriptID ) );
	}
						
	return 0;
}

int CScripts::GetPlayersMask( struct lua_State *pState )
{
	Script script( pState );

	DWORD wMask = 0;
	for ( int i = 0; i < theDipl.GetNPlayers(); ++i )
	{
		if ( theDipl.IsPlayerExist( i ) )
			wMask |= ( 1UL << i );
	}

	script.PushNumber( wMask );

	return 1;
}

int CScripts::ObjectGetCoord( struct lua_State *pState )
{
	Script script( pState );

	const int nUniqueID = script.GetObject( 1 );
	if ( !nUniqueID )
	{
		script.PushNumber( -1 );
		script.PushNumber( -1 );
		return 2;
	}

	CVec2 vCenter( -1.0f, -1.0f );

	NI_ASSERT( CLinkObject::IsLinkObjectExists( nUniqueID ), StrFmt( "Link Object does not exists, UniqueID = %i", nUniqueID ) );
	CUpdatableObj *pObj = CLinkObject::GetObjectByUniqueIdSafe( nUniqueID );

	if ( pObj )
	{
		if ( CStaticObject *pStaticObj = dynamic_cast<CStaticObject*>( pObj ) )
			vCenter = CVec2(pStaticObj->GetCenter().x,pStaticObj->GetCenter().y);
		else if ( CBasePathUnit *pPathUnit = dynamic_cast<CBasePathUnit*>( pObj ) )
			vCenter = pPathUnit->GetCenterPlain();
	}

	script.PushNumber( vCenter.x );
	script.PushNumber( vCenter.y );

	return 2;
}

int CScripts::GetScriptAreaParams( struct lua_State *pState )
{
	Script script( pState );

	const std::string szName = script.GetObject( 1 );

	CHECK_ERROR( pScripts->areas.find( szName ) != pScripts->areas.end(), StrFmt( "GetScriptAreaParams: wrong script area name (%s)", szName.c_str() ), script );
	const NDb::SScriptArea &area = pScripts->areas[szName];

	script.PushNumber( area.vCenter.x );
	script.PushNumber( area.vCenter.y );

	if ( area.eType == EAT_CIRCLE )
	{
		script.PushNumber( area.fR );
		script.PushNumber( area.fR );
	}
	else
	{
		script.PushNumber( fabs(area.vAABBHalfSize.x) );
		script.PushNumber( fabs(area.vAABBHalfSize.y) );
	}

	return 4;
}

int CScripts::IsPlayerPresent( struct lua_State *pState )
{
	Script script( pState );

	const int nPlayer = script.GetObject( 1 );
	script.PushNumber( (int)theDipl.IsPlayerExist( nPlayer ) );

	return 1;
}

int CScripts::SwitchWeather( struct lua_State *pState )
{
	Script script( pState );
	const bool bOn = ( script.GetObject( 1 ) == 1 );
	theWeather.Switch( bOn );
	return 1;
}

int CScripts::SwitchWeatherAutomatic( struct lua_State *pState )
{
	Script script( pState );
	const bool bOn = ( script.GetObject( 1 ) == 1 );
	theWeather.SwitchAutomatic( bOn );
	return 1;
}

int CScripts::GetNUnitsInSide( struct lua_State *pState )
{
	Script script( pState );

	const int nParty = script.GetObject( 1 );
	CHECK_ERROR( nParty < 3, StrFmt( "GetNUnitsInSide: wrong number of side (%d)", nParty ), script );

	script.PushNumber( units.Size( nParty ) );

	return 1;
}

int CScripts::SetDifficultyLevel( struct lua_State *state )
{
	Script script( state );

	const int nLevel = script.GetObject( 1 );
	CHECK_ERROR( nLevel >= 0 && nLevel < 3, StrFmt( "SetDifficultyLevel: lever (%d) not in range [0..2]", nLevel ), script );
	
	dynamic_cast<CAILogic*>(Singleton<IAILogic>())->SetDifficultyLevel( nLevel );

	return 0;
}

int CScripts::SetCheatDifficultyLevel( struct lua_State *state )
{
	Script script( state );

	const int nLevel = script.GetObject( 1 );
	CHECK_ERROR( nLevel >= 0 && nLevel < 3, StrFmt( "SetCheatDifficultyLevel: lever (%d) not in range [0..2]", nLevel ), script );
	
	dynamic_cast<CAILogic*>(Singleton<IAILogic>())->SetCheatDifficultyLevel( nLevel );

	return 0;
}

int CScripts::UnitRemove( struct lua_State *pState )
{
	Script script( pState );

	const int nUniqueID = script.GetObject( 1 );

	if ( CLinkObject::IsLinkObjectExists( nUniqueID ) )
	{
		if ( CCommonUnit *pUnit = dynamic_cast<CCommonUnit*>( CLinkObject::GetObjectByUniqueId( nUniqueID )) )
		{
			if ( pUnit->IsAlive() )
			pUnit->Disappear();
		}
	}
	return 0;
}

int CScripts::ViewZone( struct lua_State *pState )
{
	Script script( pState );
	if ( !theDipl.IsNetGame() )
	{
		CHECK_ERROR( script.GetObject( 1 ).IsString(  ), "ViewZone: the first parameter is not a string", script );
		CHECK_ERROR( script.GetObject( 2 ).IsNumber(  ), "ViewZone: the second parameter is not a number", script );

		const std::string szName = script.GetObject( 1 );
		const int nShow = script.GetObject( 2 );

		CHECK_ERROR( pScripts->areas.find( szName ) != pScripts->areas.end(), StrFmt( "ViewZone: wrong script area name (%s)", szName.c_str() ), script );

		const NDb::SScriptArea &area = pScripts->areas[szName];
		CHECK_ERROR( area.eType == EAT_CIRCLE, StrFmt( "ViewZone: wrong type of area %s", szName.c_str() ), script );

		theWarFog.ToggleOpenForScriptAreaTiles( area, nShow == 1 );
	}
	else
	{
		CHECK_ERROR( false, "ViewZone: can't perform in multiplayer game", script );
	}

	return 0;
}

int CScripts::IsStandGround( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(   ), "IsStandGround: first parameter isn't a number", script );
	const int nScriptID = script.GetObject( 1 );

	pScripts->DelInvalidUnits( nScriptID );

	int nAnswer = 0;
	if ( pScripts->groups.find( nScriptID ) != pScripts->groups.end() && !pScripts->groups[nScriptID].empty() )
	{
		if ( CCommonUnit *pUnit = dynamic_cast_ptr<CCommonUnit*>( pScripts->groups[nScriptID].front() ) )
			nAnswer = pUnit->GetBehaviourMoving() == SBehaviour::EMHoldPos;
	}
	script.PushNumber( nAnswer );

	return 1;
}

int CScripts::IsEntrenched( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "IsEntrenched: first parameter isn't a number", script );
	const int nScriptID = script.GetObject( 1 );

	pScripts->DelInvalidUnits( nScriptID );
	
	int nAnswer = 0;
	if ( pScripts->groups.find( nScriptID ) != pScripts->groups.end() && !pScripts->groups[nScriptID].empty() )
	{
		if ( CAIUnit *pUnit = dynamic_cast_ptr<CAIUnit*>( pScripts->groups[nScriptID].front() ) )
			nAnswer = pUnit->IsInTankPit();
	}
	script.PushNumber( nAnswer );

	return 1;
}

int CScripts::GetNMinesInScriptArea( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsString(  ), "GetNMinesInScriptArea: the second parameter is not a string", script );
	const std::string szName = script.GetObject( 1 );
	CHECK_ERROR( pScripts->areas.find( szName ) != pScripts->areas.end(), StrFmt( "GetNMinesInScriptArea: wrong script area name (%s)", szName.c_str() ), script );

	const NDb::SScriptArea &area = pScripts->areas[szName];

	class CMinesCheck : public ICheckObjects
	{
		public:
			virtual bool IsGoodObj( CExistingObject *pObj ) const { return pObj->GetObjectType() == ESOT_MINE; }
	};

	CMinesCheck minesCheck;
	script.PushNumber( pScripts->GetCheckObjectsInScriptArea( area, minesCheck ) );

	return 1;
}

int CScripts::GetNTrenchesInScriptArea( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsString(  ), "GetNTrenchesInScriptArea: the second parameter is not a string", script );
	const std::string szName = script.GetObject( 1 );
	CHECK_ERROR( pScripts->areas.find( szName ) != pScripts->areas.end(), StrFmt( "GetNTrenchesInScriptArea: wrong script area name (%s)", szName.c_str() ), script );

	const NDb::SScriptArea &area = pScripts->areas[szName];

	class CTrenchesCheck : public ICheckObjects
	{
		public:
			virtual bool IsGoodObj( CExistingObject *pObj ) const { return pObj->GetObjectType() == ESOT_ENTR_PART; }
	};

	CTrenchesCheck trenchesCheck;
	script.PushNumber( pScripts->GetCheckObjectsInScriptArea( area, trenchesCheck ) );

	return 1;
}

int CScripts::GetNFencesInScriptArea( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsString( ), "GetNFencesInScriptArea: the second parameter is not a string", script );
	const std::string szName = script.GetObject( 1 );
	CHECK_ERROR( pScripts->areas.find( szName ) != pScripts->areas.end(), StrFmt( "GetNFencesInScriptArea: wrong script area name (%s)", szName.c_str() ), script );

	const NDb::SScriptArea &area = pScripts->areas[szName];
	
	class CFencesCheck : public ICheckObjects
	{
		public:
			virtual bool IsGoodObj( CExistingObject *pObj ) const { return pObj->GetObjectType() == ESOT_FENCE; }
	};

	CFencesCheck fencesCheck;
	script.PushNumber( pScripts->GetCheckObjectsInScriptArea( area, fencesCheck ) );

	return 1;
}

int CScripts::GetNAntitankInScriptArea( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsString(  ), "GetNAntitankInScriptArea: the second parameter is not a string", script );
	const std::string szName = script.GetObject( 1 );
	CHECK_ERROR( pScripts->areas.find( szName ) != pScripts->areas.end(), StrFmt( "GetNAntitankInScriptArea: wrong script area name (%s)", szName.c_str() ), script );

	const NDb::SScriptArea &area = pScripts->areas[szName];

	class CAntiTankCheck : public ICheckObjects
	{
		public:
			virtual bool IsGoodObj( CExistingObject *pObj ) const { return theUnitCreation.IsAntiTank( pObj->GetStats() ); }
	};
	
	CAntiTankCheck antitankCheck;
	script.PushNumber( pScripts->GetCheckObjectsInScriptArea( area, antitankCheck ) );

	return 1;
}

int CScripts::GetNAPFencesInScriptArea( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsString(   ), "GetNAPFencesInScriptArea: the second parameter is not a string", script );
	const std::string szName = script.GetObject( 1 );
	CHECK_ERROR( pScripts->areas.find( szName ) != pScripts->areas.end(), StrFmt( "GetNAPFencesInScriptArea: wrong script area name (%s)", szName.c_str() ), script );

	const NDb::SScriptArea &area = pScripts->areas[szName];

	class CAntiTankCheck : public ICheckObjects
	{
		public:
			virtual bool IsGoodObj( CExistingObject *pObj ) const { return theUnitCreation.IsAPFence( pObj->GetStats() ); }
	};
	
	CAntiTankCheck antitankCheck;
	script.PushNumber( pScripts->GetCheckObjectsInScriptArea( area, antitankCheck ) );

	return 1;
}

int CScripts::Password( struct lua_State *pState )
{
	Script script( pState );

	if (script.GetObject( 1 ).IsString(  ) )
		theCheats.CheckPassword( std::string(script.GetObject( 1 ).GetString()) );

	return 0;
}

int CScripts::SetGameSpeed( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "SetGameSpeed: first parameters isn't a number", script );
	const int nSpeed = script.GetObject( 1 );

	CAITimer::SetSpeed( nSpeed );

	return 0;
}

int CScripts::GetNUnitsOfType( struct lua_State *pState )
{
	
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsString(  ), "GetNUnitsOfType: first parameters isn't a string", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(  ), "GetNUnitsOfType: second parameters isn't a number", script );

	const std::string szType = script.GetObject( 1 );
	const int nParty = script.GetObject( 2 );

	CHECK_ERROR( nParty >= 0 && nParty < 3, StrFmt( "GetNUnitsOfType: wrong number of party (%d)", nParty ), script );

	CPtr<IRPGStatsAutomagic> pAutoMagic = MakeObject<IRPGStatsAutomagic>( IRPGStatsAutomagic::tidTypeID );
	const int nType = pAutoMagic->ToInt( szType.c_str() );

	CHECK_ERROR( nType != -1, StrFmt( "GetNUnitsOfType: type %s not found", szType.c_str() ), script );
	script.PushNumber( units.GetNUnitsOfType( nParty, nType ) );

	return 1;
}

int CScripts::GetMapSize( struct lua_State *pState )
{
	Script script( pState );

	script.PushNumber( GetAIMap()->GetSizeX() * SConsts::TILE_SIZE );
	script.PushNumber( GetAIMap()->GetSizeY() * SConsts::TILE_SIZE );

	return 2;
}

int CScripts::CheckMissionBonus( struct lua_State *pState )
{
	Script script( pState );
	const std::string szBonus = script.GetObject( 1 );
	
	bool bRes = 0; // pTracker->GetBonusStatus( szBonus );
	
	script.PushBool( bRes );
	
	return 1;
}

int CScripts::GetUnitListOfPlayer( struct lua_State *state )
{
	Script script( state );
	CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "GetUnitListOfPlayer: the first parameter is not a number", script );
	const int nPlayer = script.GetObject( 1 );			 

	CUnitsInScriptAreaEnumerator enumerator( &script, 0 );
	for ( CGlobalIter iter( theDipl.GetNParty( nPlayer ), EDI_FRIEND ); !iter.IsFinished(); iter.Iterate() )
		if ( (*iter)->GetPlayer() == nPlayer )
			enumerator.AddUnit( *iter );

	return enumerator.GetNUnits();
}

int CScripts::GetUnitListInArea( struct lua_State *state )
{
	Script script( state );
	CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "GetUnitListInArea: the first parameter is not a number", script );
	const int nPlayer = script.GetObject( 1 );

	if ( script.GetObject( 2 ).IsNumberOnly() )
	{
		const int nPlayer = script.GetObject( 1 );
		CHECK_ERROR( script.GetObject(2).IsNumber(), "GetNUnitsInArea : the second parameter is not a X", script );
		CHECK_ERROR( script.GetObject(3).IsNumber(), "GetNUnitsInArea : the third parameter is not a Y", script );
		CHECK_ERROR( script.GetObject(4).IsNumber(), "GetNUnitsInArea : the fourth parameter is not a R", script );

		const float fR = script.GetObject( 4 );
		const CVec2 center( script.GetObject( 2 ), script.GetObject( 3 ) );
		const bool bCountPlanes = script.GetObject( 5 ).IsNumber() ? script.GetObject( 5 ) : true;
		int cnt = 0;
		for ( CUnitsIter<0,2> iter( 0, ANY_PARTY, center, fR ); !iter.IsFinished(); iter.Iterate() )
		{
			CAIUnit *pUnit = *iter;
			if ( pUnit->IsRefValid() && pUnit->IsAlive() && pUnit->GetPlayer() == nPlayer && IsUnitQualified( bCountPlanes, pUnit ) )
				if ( fabs2( (*iter)->GetCenterPlain() - center ) <= fR * fR )
				{
					script.PushNumber( pUnit->GetUniqueId() );
					++cnt;
				}
		}
		return cnt;
	}
	else
	{
		CHECK_ERROR( script.GetObject( 2 ).IsString( ), "GetUnitListInArea: the second parameter is not a string", script );

		const std::string szName = script.GetObject( 2 );
		const bool bCountPlanes = script.GetObject( 3 ).IsNumber() ? script.GetObject( 3 ) : true;

		CHECK_ERROR( pScripts->areas.find( szName ) != pScripts->areas.end(), StrFmt( "GetNUnitsInArea: wrong script area name (%s)", szName.c_str() ), script );

		const NDb::SScriptArea &area = pScripts->areas[szName];
		CUnitsInScriptAreaEnumerator cnt( &script, &area );
		EnumUnitsInScriptArea( &cnt, nPlayer, bCountPlanes );

		Script script( state );
		return cnt.GetNUnits();
	}
}

int CScripts::IsUnitInArea( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "GetUnitListInArea: the first parameter is not a number", script );
	const int nPlayer = script.GetObject( 1 );
	std::string szName;
	float fR ( 0 );
	CVec2 vCenter( VNULL2 );
	bool bCircle = false;
	int nLinkID;

	if ( !script.GetObject( 2 ).IsNumberOnly( ) )
	{
		CHECK_ERROR( script.GetObject( 2 ).IsString( ), "GetUnitListInArea: the second parameter is not a string", script );
		CHECK_ERROR( script.GetObject( 3 ).IsNumber( ), "GetUnitListInArea: the 3rd parameter is not UniqueId", script );
		szName = script.GetObject( 2 ).GetString();
		nLinkID = script.GetObject( 3 );
		CHECK_ERROR( pScripts->areas.find( szName ) != pScripts->areas.end(), StrFmt( "GetNUnitsInArea: wrong script area name (%s)", szName.c_str() ), script );
		const NDb::SScriptArea &area = pScripts->areas[szName];
		if ( area.eType == EAT_CIRCLE )
		{
			vCenter = area.vCenter;
			fR = area.fR;
			bCircle = true;
		}
	}
	else
	{
		CHECK_ERROR( script.GetObject( 2 ).IsNumber( ), "GetUnitListInArea: the 3 parameter is not X", script );
		CHECK_ERROR( script.GetObject( 3 ).IsNumber( ), "GetUnitListInArea: the 4 parameter is not Y", script );
		CHECK_ERROR( script.GetObject( 4 ).IsNumber( ), "GetUnitListInArea: the 5 parameter is not R", script );
		CHECK_ERROR( script.GetObject( 5 ).IsNumber( ), "GetUnitListInArea: the 6 parameter is not UniqueId", script );
		nLinkID = script.GetObject( 5 );
		bCircle = true;
		fR = script.GetObject( 4 );
		vCenter = CVec2 ( script.GetObject( 2 ), script.GetObject( 3 ) );
	}

	if ( CDynamicCast<CFormation> pFormation = CLinkObject::GetObjectByUniqueId( nLinkID ) )
	{
		if ( bCircle )
		{
			for ( int i = 0; i < pFormation->Size(); ++i )
			{
				if ( IsUnitInCirlceArea( (*pFormation)[i], vCenter, fR, nPlayer ) )
				{
					script.PushBool( true );
					return 1;
				}
			}
		}
		else
		{
			CHECK_ERROR( pScripts->areas.find( szName ) != pScripts->areas.end(), StrFmt( "GetNUnitsInArea: wrong script area name (%s)", szName.c_str() ), script );
			const NDb::SScriptArea &area = pScripts->areas[szName];
			SRect areaRect;
			areaRect.InitRect( area.vCenter, CVec2( 1, 0 ), fabs(area.vAABBHalfSize.x), fabs(area.vAABBHalfSize.y) );
			for ( int i = 0; i < pFormation->Size(); ++i )
			{
				if ( IsUnitInRectArea( (*pFormation)[i], areaRect, nPlayer ) )
				{
					script.PushBool( true );
					return 1;
				}
			}
		}
		script.PushBool( false );
		return 1;
	}
	else
	{
		CDynamicCast<CAIUnit> pUnit = CLinkObject::GetObjectByUniqueId( nLinkID );
		if ( !pUnit )
		{
			script.PushBool( false );
			return 1;
		}

		if ( bCircle ) 
			script.PushBool( IsUnitInCirlceArea( pUnit, vCenter, fR, nPlayer ) );
		else
		{
			CHECK_ERROR( pScripts->areas.find( szName ) != pScripts->areas.end(), StrFmt( "GetNUnitsInArea: wrong script area name (%s)", szName.c_str() ), script );
			const NDb::SScriptArea &area = pScripts->areas[szName];
			SRect areaRect;
			areaRect.InitRect( area.vCenter, CVec2( 1, 0 ), fabs(area.vAABBHalfSize.x), fabs(area.vAABBHalfSize.y) );
			script.PushBool( IsUnitInRectArea( pUnit, areaRect, nPlayer ) );
		}
		return 1;
	}
}

int CScripts::GetObjectList( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "GetObjectList: the first parameter is not a ScriptGroupNumber", script );
	const int nScriptGroup = script.GetObject( 1 );
	CScriptGroups::const_iterator groupPos =  groups.find( nScriptGroup );
	int nUnits = 0;
	if ( groupPos != groups.end() )
	{
		const CScriptGroup &group = groupPos->second;
		for ( CScriptGroup::const_iterator it = group.begin(); it != group.end(); ++it )
		{
			script.PushNumber( (*it)->GetUniqueId() );
			++nUnits;
		}
	}

	return nUnits;
}

struct SArtillerySort
{
	CVec2 vCenter;
	SArtillerySort() : vCenter( VNULL2 ) {}
	SArtillerySort( const CVec2 &_vCenter ) : vCenter( _vCenter ) {}

	bool operator()( const CArtillery *pArt1, const CArtillery *pArt2 ) const
	{
		return fabs2( pArt1->GetCenterPlain() - vCenter ) > fabs2( pArt2->GetCenterPlain() - vCenter );
	}
};

int CScripts::GetFreeArtillery( struct lua_State *state )
{
	Script script( state );

	CVec2 vCenter( VNULL2 );
	float fRadius = 0.0f;
	if ( script.GetObject( 1 ).IsString() )
	{
		const std::string szName = script.GetObject( 1 );
		CHECK_ERROR( pScripts->areas.find( szName ) != pScripts->areas.end(), StrFmt( "GetFreeArtillery: wrong script area name (%s)", szName.c_str() ), script );
		const NDb::SScriptArea &area = pScripts->areas[szName];
		CHECK_ERROR( area.eType == NDb::EAT_CIRCLE, StrFmt( "GetFreeArtillery: wrong script area (%s) must be EAT_CIRCLE", szName.c_str() ), script );
		vCenter = area.vCenter;
		fRadius = area.fR;
	}
	else if ( script.GetObject( 1 ).IsNumber() && script.GetObject( 2 ).IsNumber() && script.GetObject( 3 ).IsNumber() )
	{
		vCenter.x = script.GetObject( 1 );
		vCenter.y = script.GetObject( 2 );
		fRadius = script.GetObject( 3 );
	}
	else
		CHECK_ERROR( false, "GetFreeArtillery: wrong params: GetFreeArtillery( ScriptArea | [ x, y, radius ] )", script );

	std::vector<CPtr<CArtillery > > artilleries;
	for ( CUnitsIter<0,2> iter( 0, ANY_PARTY, vCenter, fRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		CArtillery *pArtillery = dynamic_cast<CArtillery*>( *iter );
		if ( pArtillery )
		{
			if ( !pArtillery->HasServeCrew() )
				artilleries.push_back( pArtillery );
		}
	}
	sort( artilleries.begin(), artilleries.end(), SArtillerySort( vCenter ) );

	for ( std::vector<CPtr<CArtillery > >::const_iterator it = artilleries.begin(); it != artilleries.end(); ++it )
	{
		script.PushNumber( (*it)->GetUniqueID() );
	}
	return artilleries.size();
}

int CScripts::LandReinforcement( struct lua_State *state )
{
	Script script( state );
	NI_ASSERT( false, "MAP DESIGN: don't use LandReinforcement function" )
	/*CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "LandReinforcement: the first parameter is not a player number", 0 );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber( ), "LandReinforcement: the second parameter is not a nReinfID", 0 );
	CHECK_ERROR( script.GetObject( 3 ).IsNumber( ), "LandReinforcement: the 3rd parameter is not a nPosition", 0 );

	const int nPlayer = script.GetObject( 1 );
	const int nReinfID = script.GetObject( 2 );
	const int nPosition = script.GetObject( 3 );
	const int nScriptID = script.GetObject( 4 ).IsNumber() ? script.GetObject( 4 ) : -1;

	CHECK_ERROR( theDipl.IsPlayerExist( nPlayer ), "Wrong player index when calling LandReinforcement()", 0 );

	CDBPtr<NDb::SReinforcement> pReinf = NDb::Get<NDb::SReinforcement>( nReinfID );
	CHECK_ERROR( pReinf != 0, StrFmt( "Reinforcement not found %d", nReinfID ), 0 );
	const NDb::SReinforcementPosition *pPos = theReinfArray[nPlayer].GetPosition( nPosition );
	CHECK_ERROR( pPos != 0, StrFmt( "Position not found %d", nPosition ), 0 );
	const NDb::SDeployTemplate *pTemplate = CPlayerReinforcement::GetDeployTemplate( *pPos, pReinf->eType );
	
	if ( pReinf != 0 && pTemplate != 0 && pPos != 0 )
	{
		CVec2 vCallPos = pPos->vPosition;

		if ( pReinf->eType == NDb::RT_BOMBERS || 
			pReinf->eType == NDb::RT_FIGHTERS || 
			pReinf->eType == NDb::RT_GROUND_ATTACK_PLANES || 
			pReinf->eType == NDb::RT_RECON )
		{
			vCallPos = pPos->vAviationPosition;
		}

		NReinforcement::PlaceSingleLandReinforcement( nPlayer, pReinf, pTemplate, vCallPos, pPos->nDirection, nScriptID );
	}
*/
	return 0;
}

int CScripts::LandReinforcementFromMap( struct lua_State *state )
{
	Script script( state );
	CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "LandReinforcement: the first parameter is not a player number", script );
	CHECK_ERROR( script.GetObject( 3 ).IsNumber( ), "LandReinforcement: the 3rd parameter is not a nPosition", script );

	const int nPlayer = script.GetObject( 1 );
	const int nPosition = script.GetObject( 3 );
	const int nScriptID = script.GetObject( 4 ).IsNumber() ? script.GetObject( 4 ) : -1;
	
	CHECK_ERROR( nPlayer < pMapInfo->players.size(), "SCRIPT: Wrong player index when calling LandReinforcement()", script );
	CDBPtr<NDb::SReinforcement> pReinf;

	if ( script.GetObject( 2 ).IsNumberOnly() )
	{
		const int nReinfIndex = script.GetObject( 2 );
		CHECK_ERROR( nReinfIndex < pMapInfo->players[nPlayer].scriptReinforcements.size(), "SCRIPT: wrong reinforcement index", script );
		pReinf = pMapInfo->players[nPlayer].scriptReinforcements[nReinfIndex];
	}
	else
	{
		CHECK_ERROR( script.GetObject( 2 ).IsString(), "LandReinforcementFromMap: the second parameter is not a ReinfName", script );
		const std::string szName = script.GetObject( 2 ).GetString();
		for ( int i = 0; i < pMapInfo->players[nPlayer].scriptReinforcementsTextID.size(); ++i )
		{
			if ( szName == pMapInfo->players[nPlayer].scriptReinforcementsTextID[i].szName )
				pReinf = pMapInfo->players[nPlayer].scriptReinforcementsTextID[i].pReinforcement;
		}
		CHECK_ERROR( pReinf != 0, StrFmt( "MAP DESIGN: no reinforcement with name \"%s\" player = %i", szName.c_str(), nPlayer ), script );
	}
	const NDb::SReinforcementPosition *pPos = theReinfArray[nPlayer].GetPosition( nPosition );
	CHECK_ERROR( pPos != 0, StrFmt( "Position not found %d", nPosition ), script );
	const NDb::SDeployTemplate *pTemplate = CPlayerReinforcement::GetDeployTemplate( *pPos, pReinf->eType );

	if ( pReinf != 0 && pTemplate != 0 && pPos != 0 )
	{
		CVec2 vCallPos = pPos->vPosition;

		if ( pReinf->eType == NDb::RT_BOMBERS || 
			pReinf->eType == NDb::RT_FIGHTERS || 
			pReinf->eType == NDb::RT_GROUND_ATTACK_PLANES || 
			pReinf->eType == NDb::RT_RECON )
		{
			vCallPos = pPos->vAviationPosition;
		}

		NReinforcement::PlaceSingleLandReinforcement( nPlayer, pReinf, pReinf->eType, pTemplate, vCallPos, pPos->nDirection, nScriptID, 0, false );
	}

	return 0;
}

int CScripts::GetReinforcementCallsLeft( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "GetReinforcementCallsLeft: the first parameter is not a player number", script );

	const int nPlayer = script.GetObject( 1 );
	if ( theReinfArray.size() <= nPlayer )
	{
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_CONSOLE, StrFmt( "GetReinforcementCallsLeft: player %i doesn't exists", nPlayer ) );
		script.PushNumber( 0 );
	}
	else
		script.PushNumber( theReinfArray[nPlayer].GetReinforcementCallsLeft() );

	return 1;
}

int CScripts::IsReinforcementAvailable( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "IsReinforcementAvailable: the first parameter is not a player number", script );
	const int nPlayer = script.GetObject( 1 );
	if ( theReinfArray.size() <= nPlayer )
	{
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_CONSOLE, StrFmt( "CallReinforcement: player %i doesn't exists", nPlayer ) );
		script.PushNumber( 0 );
	}
	else
	{
		script.PushNumber( theReinfArray[nPlayer].GetReinforcementCallsLeft() != 0 && theReinfArray[nPlayer].HasReinforcement( _RT_NONE ) && theReinfArray[nPlayer].GetRandomPointID() != -1 );
	}

	return 1;
}

int CScripts::CallReinforcement( struct lua_State *state )
{
	Script script( state );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber( ), "CallReinforcement: the first parameter is not a player number", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber( ), "CallReinforcement: the second parameter is not a nReinfType", script );
	CHECK_ERROR( script.GetObject( 3 ).IsNumber( ), "CallReinforcement: the 3rd parameter is not a nPosition", script );

	const int nPlayer = script.GetObject( 1 );
	const int nReinfType = script.GetObject( 2 );
	const int nPosition = script.GetObject( 3 );
	const int nScriptID = script.GetObject( 4 ).IsNumber() ? script.GetObject( 4 ) : -1;

	if ( theReinfArray.size() <= nPlayer )
	{
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_CONSOLE, StrFmt( "CallReinforcement: player %i doesn't exists", nPlayer ) );
	}
	else if ( script.GetObject( 5 ).IsNumber() && script.GetObject( 6 ).IsNumber() )
	{
		theReinfArray[nPlayer].CallReinforcement( (NDb::EReinforcementType)nReinfType, CVec2( script.GetObject( 5 ), script.GetObject( 6 )), nScriptID );
	}
	else
	{
		theReinfArray[nPlayer].CallReinforcement( (NDb::EReinforcementType)nReinfType, nPosition, nScriptID, 0 );
	}

	return 0;
}

int CScripts::GiveUnitCommand( struct lua_State *state )
{
	ProcessCommand( state, false, false );
	return 1;
}

int CScripts::GiveUnitQCommand( struct lua_State *state )
{
	ProcessCommand( state, true, false );
	return 1;
}
/*

int CScripts::StartSequence( struct lua_State *pState )
{
	NInput::PostEvent( "begin_script_movie_sequence", 0, 0 );
	return 1;
}

int CScripts::EndSequence( struct lua_State *pState )
{
	NInput::PostEvent( "end_script_movie_sequence", 0, 0 );
	return 1;
}
*/

int CScripts::StartSequenceWOMovieBorder( struct lua_State *pState )
{
	NInput::PostEvent( "begin_script_movie_sequence", 1, 0 );
	return 1;
}

int CScripts::EndSequenceWOMovieBorder( struct lua_State *pState )
{
	//NInput::PostEvent( "end_script_movie_sequence", 1, 0 );
	return 1;
}

int CScripts::ShowMovieBorder( struct lua_State *pState )
{
	NInput::PostEvent( "begin_script_movie_sequence", 2, 0 );
	return 1;
}

int CScripts::HideMovieBorder( struct lua_State *pState )
{
//	NInput::PostEvent( "end_script_movie_sequence", 2, 0 );
	return 1;
}
/*

int CScripts::CameraMove( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(), "CameraMove: the first parameter is not a number", 1 );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(), "CameraMove: the second parameter is not a number", 1 );

	const int nCameraPos = script.GetObject( 1 );
	const int nTimeToMove = script.GetObject( 2 );

	NInput::PostEvent( "move_camera", nCameraPos, nTimeToMove );
	//IGameEvent *pEvent = Singleton<IInput>()->GetEvent( "move_camera" );
	//pEvent->RaiseEvent( nCameraPos, nTimeToMove );

	return 1;
}*/

int CScripts::GameMesage( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsString(), "GameMesage: the first parameter is not a number", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(), "GameMesage: the second parameter is not a number", script );
	CHECK_ERROR( script.GetObject( 3 ).IsNumber(), "GameMesage: the 3rd parameter is not a number", script );
	const std::string szEventName = script.GetObject( 1 ).GetString();
	
	NInput::PostEvent( szEventName, float(script.GetObject( 2 ).GetNumber()), float(script.GetObject( 3 ).GetNumber()) );
	//IGameEvent * pEvent = Singleton<IInput>()->GetEvent( szEventName );
	//if ( pEvent )
	//	pEvent->RaiseEvent( float(script.GetObject( 2 ).GetNumber()), float(script.GetObject( 3 ).GetNumber()) );
	return 1;
}

/*int CScripts::AddChatMessage( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsString(), "AddChatMessage: the 1st parameter is not message name", 1 );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(), "AddChatMessage: the 2nd parameter is not a number", 1 );
	
	const string nTextID = script.GetObject( 1 ).GetNumber();

	const int nColor = script.GetObject( 2 ).GetNumber();
	const NDb::SText *pText = NDb::Get<NDb::SText>( nTextID );
	if ( pText )
		WriteToPipe( PIPE_CHAT, pText->wszText, nColor );

	return 1;	
}*/

int CScripts::IsImmobilized( struct lua_State * pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(), "IsImmobilized: the 1st parameter is not a UniqueID", script );
	CDynamicCast<CTank> pUnit = CLinkObject::GetObjectByUniqueIdSafe( script.GetObject( 1 ) );
	script.PushNumber( pUnit != 0 ? pUnit->IsTrackDamaged() : true );
	return 1;
}

int CScripts::IsSomeBodyAlive( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(), "IsSomeBodyAlive: the 1st parameter is not a Player", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(), "IsSomeBodyAlive: the 2nd parameter is not a ScriptGroup", script );

	const int nPlayer = script.GetObject( 1 );
	const int nScriptGroup = script.GetObject( 2 );

	CScriptGroups::const_iterator groupPos =  groups.find( nScriptGroup );
	int nUnits = 0;
	if ( groupPos != groups.end() )
	{
		const CScriptGroup &group = groupPos->second;
		for ( CScriptGroup::const_iterator it = group.begin(); it != group.end(); ++it )
		{
			CDynamicCast<CBuilding> pBuilding( *it );
			if ( pBuilding )
			{
				if ( pBuilding->IsAlive() && pBuilding->GetPlayer() == nPlayer  )
				{
					if ( theBonusSystem.IsKeyBuilding( pBuilding->GetUniqueId() ) )
						script.PushNumber( true );
					else if ( pBuilding->IsUnitsInside() )
						script.PushNumber( true );
					return 1;
				}
				else
				{
					script.PushNumber( false );
					return 1;
				}
			}
			CDynamicCast<CStaticObject> pStaticObject( *it );
			if ( pStaticObject && pStaticObject->IsAlive() )
			{
				script.PushNumber( true );
				return 1;
			}
			CDynamicCast<CCommonUnit> pUnit( *it );
			if ( pUnit && pUnit->IsAlive() && pUnit->GetPlayer() == nPlayer )
			{
				script.PushNumber( true );
				return 1;
			}
		}
	}

	script.PushNumber( false );
	return 1;
}

int CScripts::IsAlive( struct lua_State *pState )
{
	Script script( pState );

	if ( script.GetObject( 1 ).IsNumber() )
	{
		const int nUniqueID = script.GetObject( 1 );

		if ( CLinkObject::IsLinkObjectExists( nUniqueID ) )
			script.PushNumber( CLinkObject::GetObjectByUniqueId( nUniqueID )->IsAlive() );
		else
			script.PushNumber( 0 );
	}
	else
		script.PushNumber( 0 );

	return 1;
}

int CScripts::SwitchUnitLightFX( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "SwitchUnitLightFX: first parameter isn't a UniqueID", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(  ), "SwitchUnitLightFX: second parameter isn't a number", script );
	const int nUniqueID = script.GetObject( 1 );
	const int nNewState = script.GetObject( 2 );

	CLinkObject *pObject = CLinkObject::GetObjectByUniqueId( nUniqueID );
	if ( pObject && pObject->IsAlive() )
	{
		CExistingObject *pEObj = dynamic_cast<CExistingObject*>( pObject );

		if ( !pEObj || pEObj && !pEObj->IsTrampled() )
			updater.AddUpdate( 0, ACTION_NOTIFY_HEADLIGHT, pObject, nNewState );
	}

	return 0;
}

int CScripts::SwitchSquadLightFX( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "SwitchSquadLightFX: first parameter isn't a Script ID", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(  ), "SwitchSquadLightFX: second parameter isn't a number", script );
	const int nUniqueID = script.GetObject( 1 );
	const int nNewState = script.GetObject( 2 );

	const int nScriptID = script.GetObject( 1 );

	CScriptGroups::iterator itg = pScripts->groups.find( nScriptID );
	CHECK_ERROR( itg != pScripts->groups.end(), StrFmt( "SwitchSquadLightFX: wrong script id (%d)", nScriptID ), script );
	CScriptGroup *pGroup = &(itg->second);

	for( CScriptGroup::iterator it = pGroup->begin(); it != pGroup->end(); ++it )
	{
		CUpdatableObj *pObj = *it;
		if ( !pObj || pObj && !pObj->IsAlive() )
			continue;
		CExistingObject *pEObj = dynamic_cast<CExistingObject*>( pObj );

		if ( !pEObj || pEObj && !pEObj->IsTrampled() )
			updater.AddUpdate( 0, ACTION_NOTIFY_HEADLIGHT, pObj, nNewState );
	}

	return 0;
}

void CScripts::SendUpdateToObjects( int nScriptID, EActionNotify eUpdateType, int nParam )
{
	if ( pScripts->groups.find( nScriptID ) == pScripts->groups.end() )
		return;

	pScripts->DelInvalidUnits( nScriptID );

	CScriptGroup &scriptGroup = pScripts->groups[nScriptID];

	for ( std::list<CPtr<CUpdatableObj> >::iterator it = scriptGroup.begin(); it!= scriptGroup.end(); ++it )
		updater.AddUpdate( 0, eUpdateType, *it, nParam );

	return;
}

int CScripts::ObjectPlayAttachedEffect( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "ObjectPlayAttachedEffect: first parameter isn't a object ID", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(  ), "ObjectPlayAttachedEffect: second parameter isn't a number", script );

	const int nScriptID = script.GetObject( 1 );
	const int nEffectNum = script.GetObject( 2 );

	SendUpdateToObjects( nScriptID, ACTION_NOTIFY_PLAY_ATTACHED_EFFECT, nEffectNum );
	return 0;
}

int CScripts::ObjectStopAttachedEffect( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "ObjectStopAttachedEffect: first parameter isn't a object ID", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(  ), "ObjectStopAttachedEffect: second parameter isn't a number", script );

	const int nScriptID = script.GetObject( 1 );
	const int nEffectNum = script.GetObject( 2 );

	SendUpdateToObjects( nScriptID, ACTION_NOTIFY_STOP_ATTACHED_EFFECT, nEffectNum );
	return 0;
}

int CScripts::ObjectPlayAnimation( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "ObjectPlayAnimation: first parameter isn't a object ID", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(  ), "ObjectPlayAnimation: second parameter isn't a number", script );

	const int nScriptID = script.GetObject( 1 );
	const int nAnimationNum = script.GetObject( 2 );

	SendUpdateToObjects( nScriptID, ACTION_NOTIFY_ANIMATION_CHANGED, nAnimationNum );
	return 0;
}

int CScripts::WaitForGroupInArea( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "WaitForGroupInArea: first parameter isn't a player", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(  ), "WaitForGroupInArea: 2nd parameter isn't ScriptID", script );

	const int nPlayer = script.GetObject( 1 );
	const int nScriptGroup = script.GetObject( 2 );

	float fR = 0;
	CVec2 vCenter = VNULL2;

	for ( bool bAllInarea = false; !bAllInarea; )
	{
		Sleep( 100 + NRandom::Random( 1000 ) );
		if ( script.GetObject( 3 ).IsNumber() )
		{
			CHECK_ERROR( script.GetObject( 3 ).IsNumber(), "WaitForGroupInArea: 2nd parameter isn't a X", script );
			CHECK_ERROR( script.GetObject( 4 ).IsNumber(), "WaitForGroupInArea: 3rd parameter isn't a Y", script );
			CHECK_ERROR( script.GetObject( 5 ).IsNumber(), "WaitForGroupInArea: 4thparameter isn't a R", script );
			vCenter = CVec2( script.GetObject( 3 ), script.GetObject( 4 ) );
			fR = script.GetObject( 5 );
		}
		else if ( !script.GetObject( 3 ).IsNumberOnly() )
		{
			const std::string szName = script.GetObject( 3 ).GetString();
			CHECK_ERROR( pScripts->areas.find( szName ) != pScripts->areas.end(), StrFmt( "WaitForGroupInArea: wrong script area name (%s)", szName.c_str() ), script );
			
			const NDb::SScriptArea &area = pScripts->areas[szName];
			
			CScriptGroups::const_iterator groupPos =  groups.find( nScriptGroup );
			int nUnits = 0;
			if ( groupPos != groups.end() )
			{
				const CScriptGroup &group = groupPos->second;
				SRect areaRect;
				if ( area.eType == EAT_RECTANGLE )
				{
					areaRect.InitRect( area.vCenter, CVec2( 1, 0 ), fabs(area.vAABBHalfSize.x), fabs(area.vAABBHalfSize.y) );
					for ( CScriptGroup::const_iterator it = group.begin(); it != group.end(); ++it )
					{
						CDynamicCast<CCommonUnit> pUnit( *it );
						if ( pUnit && pUnit->IsAlive() && pUnit->GetPlayer() == nPlayer &&
								 !pUnit->GetUnitRect().IsIntersected( areaRect ) )
								continue;
					}
				}
				else
				{
					vCenter = area.vCenter;
					fR = area.fR;
				}
			}
		}
		CScriptGroups::const_iterator groupPos =  groups.find( nScriptGroup );
		const CScriptGroup &group = groupPos->second;
		bool bFoundNotInArea = false;
		for ( CScriptGroup::const_iterator it = group.begin(); !bFoundNotInArea && it != group.end(); ++it )
		{
			CDynamicCast<CCommonUnit> pUnit( *it );
			if ( !pUnit->GetUnitRect().IsIntersectCircle( vCenter, fR ) )
				bFoundNotInArea = true;
		}
		if ( bFoundNotInArea )
			continue;
		bAllInarea = true;
	}

	return 0;
}

//int CScripts::PlayVoice( struct lua_State *pState )
//{
//	Script script( pState );
//
//	CHECK_ERROR( script.GetObject( 1 ).IsNumber(  ), "PlayVoice: first parameter isn't a Voice ID", 1 );
//	const int nVoiceID = script.GetObject( 1 );
//	Singleton<IMusicSystem>()->PlayVoice( NDb::Get<NDb::SVoice>( nVoiceID ) );
//	return 0;
//}

int CScripts::SCRunTime( struct lua_State *pState )
{
	Script script( pState );

	CPtr<SScriptCameraRunUpdate> pUpdate = new SScriptCameraRunUpdate;

	CHECK_ERROR( script.GetObject(1).IsString(), "SCRunTime: parameter 1 isn't Camera Name", script );
	CHECK_ERROR( script.GetObject(2).IsString(), "SCRunTime: parameter 2 isn't Camera Name", script );
	CHECK_ERROR( script.GetObject(3).IsNumber(), "SCRunTime: parameter 3 isn't Time", script );
	const std::string szStart = script.GetObject(1);
	const std::string szFinish = script.GetObject(2);
	pUpdate->szStartCam = szStart;
	pUpdate->szFinishCam = szFinish;
	pUpdate->fTime = float(script.GetObject(3).GetNumber());
	pUpdate->eRunType = SCRT_DIRECT_MOVE;
	pUpdate->fLinSpeed = 0;
	pUpdate->fAngle = 0;
	pUpdate->nTargetID = -1;
	NI_ASSERT( pUpdate->fTime >= 0, "Script movie length < 0!\n" );
	if ( szFinish == szStart )
		pUpdate->eRunType = SCRT_DIRECT_ROTATE;
	//
	if ( script.GetTop() > 3 )
	{
		CHECK_ERROR( script.GetObject(4).IsNumber(), "SCRunTime: parameter 4 isn't MoveType", script );
		CHECK_ERROR( script.GetObject(5).IsNumber(), "SCRunTime: parameter 5 isn't number", script );
		const NDb::EScriptCameraRunType eRunType( NDb::EScriptCameraRunType(script.GetObject(4).GetInteger()) );
		pUpdate->eRunType = eRunType;

		switch ( eRunType )
		{
		case NDb::SCRT_DIRECT_ROTATE:
			{
				pUpdate->fAngle = float(script.GetObject(5).GetNumber());
				updater.AddUpdate( pUpdate, ACTION_NOTIFY_SCAMERA_RUN, 0, 0 );
			}
			break;
			//
		case NDb::SCRT_DIRECT_FOLLOW:
			{
				int nScriptID = script.GetObject(5).GetInteger();
				pScripts->DelInvalidUnits( nScriptID );

				if ( pScripts->groups.find( nScriptID ) != pScripts->groups.end() && !pScripts->groups[nScriptID].empty() )
				{
					CCommonUnit *pUnit = dynamic_cast_ptr<CCommonUnit*>(pScripts->groups[nScriptID].front());
					if ( pUnit )
					{
						pUpdate->nTargetID = pUnit->GetUniqueID();
						updater.AddUpdate( pUpdate, ACTION_NOTIFY_SCAMERA_RUN, 0, 0 );
					}
				}
			}
			break;
			//
		case NDb::SCRT_SPLINE:
			{
				pUpdate->fSpline1 = float(script.GetObject(5).GetNumber());
				pUpdate->fSpline2 = float(script.GetObject(6).GetNumber());
				updater.AddUpdate( pUpdate, ACTION_NOTIFY_SCAMERA_RUN, 0, 0 );
			}
			break;
		}
	}
	else
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_SCAMERA_RUN, 0, 0 );

	return 0;
}

int CScripts::SCRunSpeed( struct lua_State *pState )
{
	Script script( pState );

	CPtr<SScriptCameraRunUpdate> pUpdate = new SScriptCameraRunUpdate;

	CHECK_ERROR( script.GetObject(1).IsString(), "SCRunSpeed: parameter 1 isn't Camera Name", script );
	CHECK_ERROR( script.GetObject(2).IsString(), "SCRunSpeed: parameter 2 isn't Camera Name", script );
	CHECK_ERROR( script.GetObject(3).IsNumber(), "SCRunSpeed: parameter 3 isn't Speed", script );
	const std::string szStart = script.GetObject(1);
	const std::string szFinish = script.GetObject(2);
	pUpdate->szStartCam = szStart;
	pUpdate->szFinishCam = szFinish;
	pUpdate->fLinSpeed = float(script.GetObject(3).GetNumber());
	pUpdate->eRunType = SCRT_DIRECT_MOVE;
	pUpdate->fTime = 0;
	pUpdate->fAngle = 0;
	pUpdate->nTargetID = -1;
	if ( szFinish == szStart )
		pUpdate->eRunType = SCRT_DIRECT_ROTATE;
	//
	if ( script.GetTop() > 3 )
	{
		CHECK_ERROR( script.GetObject(4).IsNumber(), "SCRunSpeed: parameter 4 isn't MoveType", script );
		CHECK_ERROR( script.GetObject(5).IsNumber(), "SCRunSpeed: parameter 5 isn't number", script );
		const NDb::EScriptCameraRunType eRunType( NDb::EScriptCameraRunType(script.GetObject(4).GetInteger()) );
		pUpdate->eRunType = eRunType;

		switch ( eRunType )
		{
		case NDb::SCRT_DIRECT_ROTATE:
			{
				pUpdate->fAngle = float(script.GetObject(5).GetNumber());
				updater.AddUpdate( pUpdate, ACTION_NOTIFY_SCAMERA_RUN, 0, 0 );
			}
			break;
			//
		case NDb::SCRT_DIRECT_FOLLOW:
			{
				int nScriptID = script.GetObject(5).GetInteger();
				pScripts->DelInvalidUnits( nScriptID );

				if ( pScripts->groups.find( nScriptID ) != pScripts->groups.end() && !pScripts->groups[nScriptID].empty() )
				{
					if ( CCommonUnit *pUnit = dynamic_cast_ptr<CCommonUnit*>(pScripts->groups[nScriptID].front()) )
						nScriptID = pUnit->GetUniqueID();
					pUpdate->nTargetID = nScriptID;
					updater.AddUpdate( pUpdate, ACTION_NOTIFY_SCAMERA_RUN, 0, 0 );
				}
			}
			break;
			//
		case NDb::SCRT_SPLINE:
			{
				pUpdate->fSpline1 = float(script.GetObject(5).GetNumber());
				pUpdate->fSpline2 = float(script.GetObject(6).GetNumber());
				updater.AddUpdate( pUpdate, ACTION_NOTIFY_SCAMERA_RUN, 0, 0 );
			}
			break;
		}
	}
	else
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_SCAMERA_RUN, 0, 0 );

	return 0;
}

int CScripts::SCReset( struct lua_State *pState )
{
	Script script( pState );

	CPtr<SScriptCameraResetUpdate> pUpdate = new SScriptCameraResetUpdate;

	updater.AddUpdate( pUpdate, ACTION_NOTIFY_SCAMERA_RESET, 0, 0 );
	return 0;
}

int CScripts::SCStartMovie( struct lua_State *pState )
{
	Script script( pState );

	CHECK_ERROR( script.GetObject(1).IsNumber(), "SCStartMovie: the 1st parameter should be integer (movie index)", script )
	const int nMovieIndex = script.GetObject(1).GetNumber();

	std::string szCallbackFuncName = "";
	if ( script.GetTop() > 1 )
		szCallbackFuncName = script.GetObject(2).GetString();

	bool bLoopPlayback = false;
	if ( script.GetTop() > 2 )
		 bLoopPlayback = ( script.GetObject(3) == 1 );

	CPtr<SScriptCameraStartMovieUpdate> pUpdate = new SScriptCameraStartMovieUpdate;
	pUpdate->nMovieIndex = nMovieIndex;
	pUpdate->bLoopPlayback = bLoopPlayback;
	pUpdate->szCallbackFuncName = szCallbackFuncName;
	updater.AddUpdate( pUpdate, ACTION_NOTIFY_SC_START_MOVIE, 0, 0 );

	return 0;
}

int CScripts::SCStopMovie( struct lua_State *pState )
{
	Script script( pState );

	CPtr<SScriptCameraStopMovieUpdate> pUpdate = new SScriptCameraStopMovieUpdate;
	updater.AddUpdate( pUpdate, ACTION_NOTIFY_SC_STOP_MOVIE, 0, 0 );

	return 0;
}

int CScripts::AttackGroupCreate( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "AttackGroupCreate: parameter 1 isn't AttackGroupID", script );
	const int nID = script.GetObject( 1 );
	CPtr<CExecutorAttackGroup> pExecutor = new CExecutorAttackGroup( nID ); 
	
	theExecutorContainer.Add( pExecutor );
	pExecutor->RegisterOnEvents( &theExecutorContainer );
	attackGroups[nID] = pExecutor->GetID();

	return 0;
}

void CScripts::AttackGroupAddUnit( CCommonUnit * pUnit, int nAttackGroupID )
{
	if ( pUnit )
	{
		SExecutorEventParam param( EID_ATTACKGROUP_ADD_UNIT, attackGroups[nAttackGroupID], 0 );
		CExecutorAttackGroupAddUnitEvent event( param, pUnit->GetUniqueId() );
		theExecutorContainer.RaiseEvent( event );
	}
}

int CScripts::AttackGroupAddUnit( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "AttackGroupAddUnit: parameter 1 isn't AttackGroupID", script );
	CHECK_ERROR( script.GetObject(2).IsNumber(), "AttackGroupAddUnit: parameter 2 isn't ScriptID", script );
	CHECK_ERROR( script.GetObject(3).IsNumber(), "AttackGroupAddUnit: parameter 3 isn't UniqueID", script );

	const int nID = script.GetObject( 1 );
	CAttackGroup::const_iterator pos = attackGroups.find( nID );

	if ( pos != attackGroups.end() )
	{
		const int nScriptGroup = script.GetObject(2);
		const int nUniqueID = script.GetObject(3);
		if ( nScriptGroup != -1 )
		{
			CScriptGroups::const_iterator groupPos =  groups.find( nScriptGroup );
			if ( groupPos != groups.end() )
			{
				const CScriptGroup &group = groupPos->second;
				for ( CScriptGroup::const_iterator it = group.begin(); it != group.end(); ++it )
					AttackGroupAddUnit( dynamic_cast_ptr<CCommonUnit*>( *it ), nID );
			}
		}
		if ( nUniqueID != -1 )
		{
			CCommonUnit * pUnit = dynamic_cast<CCommonUnit*>( CLinkObject::GetObjectByUniqueIdSafe( nUniqueID ) );
			if ( pUnit )
				AttackGroupAddUnit( pUnit, nID );
		}
	}
	return 0;
}

int CScripts::AttackGroupStartAttack( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "AttackGroupStartAttack: parameter 1 isn't AttackGroupID", script );
	CHECK_ERROR( script.GetObject(2).IsNumber(), "AttackGroupStartAttack: parameter 2 isn't X", script );
	CHECK_ERROR( script.GetObject(3).IsNumber(), "AttackGroupStartAttack: parameter 3 isn't Y", script );
	CHECK_ERROR( script.GetObject(4).IsNumber(), "AttackGroupStartAttack: parameter 4 isn't Range", script );

	const int nID = script.GetObject( 1 );
	CAttackGroup::const_iterator pos = attackGroups.find( nID );

	if ( pos != attackGroups.end() )
	{
		SExecutorEventParam param( EID_ATTACKGROUP_ATTACK, attackGroups[nID], 0 );
		CExecutorEventAttackGroupAttackPoint event( nID, CVec2( script.GetObject(2), script.GetObject(3) ), script.GetObject(4), param );
		theExecutorContainer.RaiseEvent( event );
	}

	return 0;
}

int CScripts::AttackGroupDelete( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "AttackGroupCreate: parameter 1 isn't AttackGroupID", script );

	const int nID = script.GetObject( 1 );
	CAttackGroup::iterator pos = attackGroups.find( nID );

	if ( pos != attackGroups.end() )
	{
		SExecutorEventParam param( EID_ATTACKGROUP_DELETE, attackGroups[nID], 0 );
		CExecutorSimpleEvent event( param );
		theExecutorContainer.RaiseEvent( event );
		attackGroups.erase( pos );
	}

	return 0;
}

int CScripts::PlayerCanSee( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "PlayerCanSee: parameter 1 isn't Player", script );
	CHECK_ERROR( script.GetObject(2).IsNumber(), "PlayerCanSee: parameter 2 isn't UniqueID", script );
	const int nPlayer = script.GetObject( 1 );
	const int nUniqueID = script.GetObject( 2 );

	CDynamicCast<CCommonUnit> pUnit = CLinkObject::GetObjectByUniqueIdSafe( nUniqueID );
	if ( pUnit )
		script.PushNumber( pUnit->IsVisible( theDipl.GetNParty( nPlayer ) ) );
	else 
		script.PushNumber( false );
	return 1;
}

int CScripts::UnitCanSee( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "UnitCanSee: parameter 1 isn't Looker unique ID", script );
	CHECK_ERROR( script.GetObject(2).IsNumber(), "UnitCanSee: parameter 2 isn't UniqueID", script );
	const int nUniqueIDLooker = script.GetObject( 1 );
	const int nTargetID = script.GetObject( 2 );
	const bool bUniqueID = script.GetObject( 3 ).IsNumber() ? script.GetObject( 3 ) : true;


	CDynamicCast<CCommonUnit> pLooker( CLinkObject::GetObjectByUniqueIdSafe( nUniqueIDLooker ) );
	if ( bUniqueID )
	{
		// nTargetID is unique ID:
		CDynamicCast<CCommonUnit> pUnit( CLinkObject::GetObjectByUniqueIdSafe( nTargetID ) );
		if ( pLooker && pUnit && pUnit->IsVisible( pLooker->GetParty() ) && 
				fabs2( pUnit->GetCenterPlain() - pLooker->GetCenterPlain()) <= sqr( pLooker->GetSightRadius() ) 
				) 
			script.PushNumber( true );
		else
			script.PushNumber( false );
	}
	else
	{
		// nTargetID is script ID
		if ( pScripts->groups.find( nTargetID ) != pScripts->groups.end() )
		{
			pScripts->DelInvalidUnits( nTargetID );
			for ( std::list<CPtr<CUpdatableObj> >::iterator iter = pScripts->groups[nTargetID].begin(); iter != pScripts->groups[nTargetID].end(); ++iter )
			{
				CDynamicCast<CCommonUnit> pUnit( *iter );
				if ( pLooker && pUnit && pUnit->IsVisible( pLooker->GetParty() ) && 
					fabs2( pUnit->GetCenterPlain() - pLooker->GetCenterPlain()) <= sqr( pLooker->GetSightRadius() ) 
					) 
					script.PushNumber( true );
				else
					script.PushNumber( false );
			}
		}
	}
	return 1;
}

int CScripts::SetAmmo( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "SetAmmo: parameter 1 isn't unique ID", script );
	CDynamicCast<CAIUnit> pUnit ( CLinkObject::GetObjectByUniqueIdSafe( script.GetObject(1) ) );
	if ( pUnit )
	{
		for ( int i = 0; i < pUnit->GetNCommonGuns(); ++i )
		{
			if ( script.GetObject( 2 + i ).IsNumber() )
			{
				const int nAmmoNew = script.GetObject( 2 + i );
				const int nAmmoOld = pUnit->GetNAmmo( i );
				pUnit->ChangeAmmo( i, nAmmoNew - nAmmoOld );
			}
		}
	}

	return 0;
}

int CScripts::UnitPlayAnimation( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "UnitPlayAnimation: parameter 1 isn't unique ID", script );
	CHECK_ERROR( script.GetObject(2).IsNumber(), "UnitPlayAnimation: parameter 2 isn't nAnimation", script );
	CDynamicCast<CAIUnit> pUnit ( CLinkObject::GetObjectByUniqueIdSafe( script.GetObject(1) ) );
	const int nAnimationType = script.GetObject(2);
	const int nAnimationVariant = script.GetObject(3).IsNumber() ? script.GetObject(3) : 0;

	if ( pUnit )
	{
		if ( pUnit->IsInfantry() )
		{
			updater.AddUpdate( 0, ACTION_NOTIFY_ANIMATION_CHANGED, pUnit, nAnimationType );
		}
		else
		{
			const SMechUnitRPGStats *pStats = dynamic_cast<const SMechUnitRPGStats*>( pUnit->GetStats() );
			if ( pStats && pStats->animdescs.size() > nAnimationType && nAnimationType >= 0 && 
				pStats->animdescs[nAnimationType].anims.size() > nAnimationVariant && nAnimationVariant >= 0 )
			{
				updater.AddUpdate( 0, ACTION_NOTIFY_ANIMATION_CHANGED, pUnit, pStats->animdescs[nAnimationType].anims[nAnimationVariant].nFrameIndex );
			}
		}
	}
	return 0;
}

int CScripts::PlayEffect( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "PlayEffect: parameter 1 isn't Effect index", script );
	CHECK_ERROR( script.GetObject(2).IsNumber(), "PlayEffect: parameter 2 isn't X", script );
	CHECK_ERROR( script.GetObject(3).IsNumber(), "PlayEffect: parameter 3 isn't Y", script );
	CHECK_ERROR( script.GetObject(4).IsNumber(), "PlayEffect: parameter 4 isn't Z", script );
	int nEffect = script.GetObject(1);
	const CVec2 vPlanePos( script.GetObject(2), script.GetObject(3) );
	CVec3 vPos;
	if ( !script.GetObject(4).IsNumber() )
		vPos = GetHeights()->Get3DPoint( vPlanePos );
	else
		vPos = CVec3( vPlanePos, script.GetObject(4) );

	if ( pMapInfo->scriptEffects.size() > nEffect )
		updater.AddUpdate( new SPlayEffectUpdate( pMapInfo->scriptEffects[nEffect], vPos ), ACTION_NOTIFY_PLAY_EFFECT, 0, -1 );
	
	return 0;
}

int CScripts::GetAmmo( struct lua_State *pState )
{
	Script script( pState );
	
	CHECK_ERROR( script.GetObject(1).IsNumber(), "SetAmmo: parameter 1 isn't unique ID", script );
	CLinkObject * pObj = CLinkObject::GetObjectByUniqueIdSafe( script.GetObject( 1 ) );
	if ( CDynamicCast<CAIUnit> pUnit = pObj )
	{
		const int nGuns = pUnit->GetNCommonGuns();
		for ( int i = 0; i < nGuns; ++i )
			script.PushNumber( pUnit->GetNAmmo( i ) );
		return nGuns;
	}
	if ( CDynamicCast<CFormation> pFormation = pObj )
	{
		int nGuns = 0;
		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			CAIUnit *pUnit = (*pFormation)[i];
			for ( int i = 0; i < pUnit->GetNCommonGuns(); ++i )
			{
				script.PushNumber( pUnit->GetNAmmo( i ) );
				++nGuns;
			}
		}
		return nGuns;
	}
	return 0;
}

int CScripts::EnablePlayerReinforcement( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "EnableReinforcement: parameter 1 isn't a reinforcement type", script );
	CHECK_ERROR( script.GetObject(2).IsNumber(), "EnableReinforcement: parameter 2 isn't bEnable", script );
	const NDb::EReinforcementType eType = NDb::EReinforcementType( int ( script.GetObject( 1 ).GetNumber() ) );
	const bool bEnable = script.GetObject( 2 ).GetNumber();
	
	// search map info for the reinforcement type
	CDBPtr<SReinforcement> pCur;
	for ( int i = 0; i < pMapInfo->players[theDipl.GetMyNumber()].reinforcementTypes.size(); ++i )
	{
		pCur = pMapInfo->players[theDipl.GetMyNumber()].reinforcementTypes[i];
		if ( pCur && eType == pCur->eType )
			break;
	}

	if ( pCur )
	{
		CPtr<SAIAvailableReinfUpdate> pUpdate = new SAIAvailableReinfUpdate;
		pUpdate->bEnabled = bEnable;
		pUpdate->pReinf = pCur;
		pUpdate->nReinforcementCallsLeft = Singleton<IAIScenarioTracker>()->GetReinforcementCallsLeft( theDipl.GetMyNumber() );
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_AVAIL_REINF, 0, -1 );		
	}
	return 0;
}

int CScripts::EnablePlayerSuperWeapon( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "EnablePlayerSuperWeapon: parameter 1 isn't a player number", script );
	CHECK_ERROR( script.GetObject(2).IsNumber(), "EnablePlayerSuperWeapon: parameter 2 isn't a bEnable", script );

	const int nPlayer = script.GetObject( 1 ).GetNumber();
	const bool bEnable = script.GetObject( 2 ).GetNumber();

	// NSuperWeapon::SuperWeaponControl( nPlayer, bEnable, -1 );
	return 0;
}

int CScripts::GiveReinforcementCalls( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "GiveReinforcementCalls: parameter 1 isn't a player number", script );
	CHECK_ERROR( script.GetObject(2).IsNumber(), "GiveReinforcementCalls: parameter 2 isn't a number of calls", script );
	const int nPlayer = int ( script.GetObject( 1 ).GetNumber() );
	const int nCalls = int ( script.GetObject( 2 ).GetNumber() );

	if ( Singleton<IAIScenarioTracker>()->GetGameType() != IAIScenarioTracker::EGT_SINGLE )
		return 0;

	if ( nCalls > 0 )
		Singleton<IAIScenarioTracker>()->IncreaseReinforcementCallsLeft( nPlayer, nCalls );
	else if ( nCalls < 0 )
		Singleton<IAIScenarioTracker>()->DecreaseReinforcementCallsLeft( nPlayer, -nCalls );

	if ( nPlayer == theDipl.GetMyNumber() )
	{
		CPtr<SAIAvailableReinfUpdate> pUpdate = new SAIAvailableReinfUpdate;
		pUpdate->nReinforcementCallsLeft = Singleton<IAIScenarioTracker>()->GetReinforcementCallsLeft( nPlayer );
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_AVAIL_REINF, 0, -1 );		
	}

	return 0;
}

int CScripts::SetLeader( struct lua_State *pState )
{
#ifndef _FINALRELEASE
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "SetLeader: parameter 1 isn't a reinforcement type", script );
	CHECK_ERROR( script.GetObject(2).IsString(), "SetLeader: parameter 2 isn't a name", script );

	NDb::EReinforcementType eType = (NDb::EReinforcementType)( (int)( script.GetObject( 1 ) ) );
	std::wstring wszName = NStr::ToUnicode( script.GetObject( 2 ).GetString() );
	Singleton<IAIScenarioTracker>()->DebugAssignLeader( eType, wszName );
#endif //_FINALRELEASE

	return 0;
}

int CScripts::GiveXPToReinforcement( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "GiveXPToReinforcement: parameter 1 isn't a player number", script );
	CHECK_ERROR( script.GetObject(2).IsNumber(), "GiveXPToReinforcement: parameter 2 isn't a reinforcement type", script );
	CHECK_ERROR( script.GetObject(3).IsNumber(), "GiveXPToReinforcement: parameter 3 isn't an xp value", script );
	const int nPlayer = script.GetObject( 1 ).GetNumber();
	const NDb::EReinforcementType eType = NDb::EReinforcementType( int ( script.GetObject( 2 ).GetNumber() ) );
	const int nXP = script.GetObject( 3 ).GetNumber();

	IAIScenarioTracker *pST = Singleton<IAIScenarioTracker>();

	if ( pST )
	{
		if ( nPlayer == theDipl.GetMyNumber() )
			updater.AddUpdate( EFB_GAIN_EXP, eType, 0 );

		if ( pST->GiveXP( nPlayer, eType, nXP ) )
		{
			CPtr<SFeedBackUnitsArray> pParam = new SFeedBackUnitsArray;
			for ( CGlobalIter iter( theDipl.GetNParty( nPlayer ), EDI_FRIEND ); !iter.IsFinished(); iter.Iterate() )
				if ( (*iter)->GetPlayer() == nPlayer && (*iter)->GetReinforcementType() == eType ) 
					pParam->unitIDs.push_back( (*iter)->GetUniqueID() );

			if ( nPlayer == theDipl.GetMyNumber() )
				updater.AddUpdate( EFB_COMMANDER_LEVELUP, eType, pParam );
		}
	}

	return 0;
}

int CScripts::GiveXPToPlayer( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "GiveXPToPlayer: parameter 1 isn't a player number", script );
	CHECK_ERROR( script.GetObject(2).IsNumber(), "GiveXPToPlayer: parameter 2 isn't an xp value", script );
	const int nPlayer = script.GetObject( 1 ).GetNumber();
	const int nXP = script.GetObject( 2 ).GetNumber();

	IAIScenarioTracker *pST = Singleton<IAIScenarioTracker>();

	if ( pST )
		pST->GiveXPToPlayer( nPlayer, nXP );

	return 0;
}

void CScripts::RemoveUnit( CAIUnit * pUnit )
{
	CTerrain * pTerrain = GetTerrain();
	
	const int nUniqueID = pUnit->GetUniqueID();

	pTerrain->RemoveTemporaryUnlocking( nUniqueID );
	theCombatEstimator.DelUnit( pUnit );
	theFeedBackSystem.RemovedAllFeedbacks( nUniqueID );
	theGroupLogic.UnregisterSegments( pUnit );
	theGroupLogic.UnregisterPathSegments( pUnit );

	updater.AddUpdate( 0, ACTION_NOTIFY_DISSAPEAR_OBJ, pUnit, 0 );
	pUnit->UnfixUnlocking();
	pUnit->UnlockTiles();
	units.DeleteUnitFromMap( pUnit );
	units.FullUnitDelete( pUnit );
	theWarFog.DeleteUnit( nUniqueID );
}

int CScripts::RemoveAllUnitsTmp( struct lua_State *pState )
{
	Script script( pState );
	std::unordered_map<int, bool> excluded;
	for ( int i = 1; script.GetObject( i ).IsNumber(); ++i )
		excluded[script.GetObject(i).GetNumber()] = true;

	for ( CGlobalIter iter( 2, EDI_NEUTRAL ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit * pUnit = *iter;
		if ( excluded.find( pScripts->GetScriptID( pUnit ) ) == excluded.end() )
			rememberedUnits.push_back( pUnit );
	}

	for ( std::list<CObj<CAIUnit> >::iterator it = rememberedUnits.begin(); it != rememberedUnits.end(); ++it )
		RemoveUnit( *it );
	return 0;
}

int CScripts::ReturnAllUnits( struct lua_State *pState )
{
	for ( std::list<CObj<CAIUnit> >::iterator it = rememberedUnits.begin(); it != rememberedUnits.end(); )
	{
		CObj<CAIUnit> pUnit = *it;
		it = rememberedUnits.erase( it );


		units.AddUnitToUnits( pUnit, pUnit->GetPlayer(), pUnit->GetStats()->etype );
		units.AddUnitToMap( pUnit );

		updater.AddUpdate( 0, ACTION_NOTIFY_NEW_UNIT, pUnit, -1 );
		updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, pUnit, -1 );
		pUnit->LockTiles();

		SWarFogUnitInfo fogInfo;
		pUnit->GetFogInfo( &fogInfo );
		theWarFog.AddUnit( pUnit->GetUniqueId(), fogInfo, pUnit->GetParty() );
		theGroupLogic.RegisterSegments( pUnit, false, true );
		theGroupLogic.RegisterPathSegments( pUnit, false );
		pUnit->UpdateVisibilityForced();
		if ( pUnit->IsInfantry() )
			updater.AddUpdate( 0, ACTION_NOTIFY_NEW_FORMATION, checked_cast_ptr<CSoldier*>( pUnit ), -1 );
	}
	return 0;
}

int CScripts::RegisterCommandObserver( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "RegisterCommandObserver: parameter 1 isn't a command number", script );
	const int nCommand = script.GetObject( 1 ).GetNumber();
	theCommandTrackerForScript.Register( nCommand );
	return 0;
}

int CScripts::SetDynamicObjectiveScriptGroup( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject(1).IsNumber(), "SetDynamicObjectiveScriptGroup: parameter 1 isn't a script ID", script );
	CHECK_ERROR( script.GetObject(2).IsNumber(), "SetDynamicObjectiveScriptGroup: parameter 2 isn't a objective Number", script );

	const int nScriptID = script.GetObject(1).GetNumber();
	const int nObjective = script.GetObject(2).GetNumber();

	if ( pScripts->groups.find( nScriptID ) != pScripts->groups.end() )
	{
		pScripts->DelInvalidUnits( nScriptID );

		for ( std::list<CPtr<CUpdatableObj> >::iterator it = pScripts->groups[nScriptID].begin();
			it != pScripts->groups[nScriptID].end(); ++it )
		{
			if ( (*it)->IsAlive() )
			{
				CDynamicCast<CCommonUnit> pUnit ( *it );
				if ( pUnit )
					theFeedBackSystem.AddFeedback( pUnit->GetUniqueID(), pUnit->GetCenterPlain(), EFB_OBJECTIVE_MOVED, nObjective );
			}
		}
	}

	return 0;
}

int CScripts::GetDifficultyLevel( struct lua_State *pState )
{
	Script script( pState );
	int nLevel = 0;
	if ( GetScenarioTracker() )
	{
		nLevel = GetScenarioTracker()->GetDifficultyLevel();
	}
	script.PushNumber( nLevel );
	return 1;
}

//    For Testers!

int CScripts::DamageAllUnits( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject( 1 ).IsNumber(), "DamageAllUnits: parameter 1 isn't a player number", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(), "DamageAllUnits: parameter 2 isn't a damage", script );
	const int nPlayer = script.GetObject( 1 ).GetNumber();
	const float fDamage = script.GetObject( 2 ).GetNumber();
	const int nNeutralPlayer = theDipl.GetNeutralPlayer();
	for ( CGlobalIter iter( theDipl.GetNParty( nPlayer ), EDI_FRIEND ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( pUnit->IsRefValid() && pUnit->IsAlive() && pUnit->GetPlayer() == nPlayer )
			pUnit->TakeDamage( fDamage, 0, nNeutralPlayer, 0 );
	}
	return 0;
}

int CScripts::AddAmmoToAllUnits( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject( 1 ).IsNumber(), "AddAmmoToAllUnits: parameter 1 isn't a player number", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(), "AddAmmoToAllUnits: parameter 2 isn't a number", script );
	const int nPlayer = script.GetObject( 1 ).GetNumber();
	std::vector<int> ammoToAdd;
	for ( int nParam = 2; script.GetObject( nParam ).IsNumber(); ++nParam )
	{
		ammoToAdd.push_back( script.GetObject( nParam ).GetNumber() );
	}
	const int nNeutralPlayer = theDipl.GetNeutralPlayer();
	for ( CGlobalIter iter( theDipl.GetNParty( nPlayer ), EDI_FRIEND ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( pUnit->IsRefValid() && pUnit->IsAlive() && pUnit->GetPlayer() == nPlayer )
		{
			const int nAmmoToAdd = min( pUnit->GetNCommonGuns(), ammoToAdd.size() );
			for ( int i = 0; i < nAmmoToAdd; ++i )
			{
				pUnit->ChangeAmmo( i, ammoToAdd[i] );
			}
		}
	}
	return 0;
}

int CScripts::BlinkActionButton( struct lua_State *pState )
{
	Script script( pState );
	CHECK_ERROR( script.GetObject( 1 ).IsNumber(), "BlinkActionButton: parameter 1 isn't a button number", script );
	CHECK_ERROR( script.GetObject( 2 ).IsNumber(), "BlinkActionButton: parameter 2 isn't a blink state", script );
	const int nButton = script.GetObject( 1 ).GetNumber();
	const int nBlinkState = script.GetObject( 2 ).GetNumber();
	NInput::PostEvent( "script_blink_action_button", nButton, nBlinkState );
	return 0;
}

#undef CHECK_ERROR

