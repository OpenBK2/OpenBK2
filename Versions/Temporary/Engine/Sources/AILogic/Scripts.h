#ifndef __SCRIPT_FUNCTIONS_H__
#define __SCRIPT_FUNCTIONS_H__

#pragma ONCE

#include "../Stats_B2_M1/DBMapInfo.h"

namespace NDb
{
	struct SScriptArea;
}
class CUpdatableObj;
class CAIUnit;
interface IScriptWrapper;
class Script;
struct SRegFunction;
interface IGlobeScriptHandler;
enum EActionNotify;

interface IEnumerator
{
	// return true if enumeration should be finished
	virtual bool AddUnit( CAIUnit *pUnit ) = 0;
	virtual void Done() = 0;
};

interface IScriptAreaEnumerator : public IEnumerator
{
	virtual const NDb::SScriptArea &GetArea() const = 0;
};

typedef list<CPtr<CUpdatableObj> > CScriptGroup;
typedef hash_map<int, CScriptGroup > CScriptGroups;
typedef hash_map<int/*attackGroupID*/, int/*ExecutorID*/> CAttackGroup;

class CScripts
{
	public: int operator&( IBinSaver &saver ); private:;

	static const int TIME_TO_CHECK_SUSPENDED_REINF;

	struct SScriptInfo
	{
		ZDATA
		NTimer::STime period;
		NTimer::STime lastUpdate;
		int nRepetitions;

		string szName;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&period); f.Add(3,&lastUpdate); f.Add(4,&nRepetitions); f.Add(5,&szName); return 0; }

		SScriptInfo() : period( 0 ), lastUpdate( 0 ), nRepetitions( -1 ), szName( "" ) {}
	};

	//Script script;
	CPtr<IScriptWrapper> pScript;
	CPtr<IGlobeScriptHandler> pGlobeScriptHandler;

	// номер группы - юниты
	static CScriptGroups groups;

	// номер reinforcement - reinforcement object
	struct SReinforcementObject
	{
		ZDATA
		SMapObjectInfo mapObject;
		CDBPtr<SHPObjectRPGStats> pStats;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&mapObject); f.Add(3,&pStats); return 0; }
		SReinforcementObject() { }
		SReinforcementObject( const SMapObjectInfo &_mapObject, const SHPObjectRPGStats *_pStats/*, IScenarioUnit *_pScenarioUnit*/ )
			: mapObject( _mapObject ), pStats( _pStats )/*, pScenarioUnit( _pScenarioUnit ) */{ }
	};
	typedef list<SReinforcementObject> CReinfList;
	hash_map<int, CReinfList> reinforcs;
	// отложенные (некуда поставить) подкрепления
	CReinfList suspendedReinforcs;
	CReinfList::iterator reinforcsIter;
	NTimer::STime lastTimeToCheckSuspendedReinforcs;

	hash_map<int, int> reservePositions;

	// юнит - номер скриптовой группы
	hash_map< int, int> groupUnits;
	
	// для сегмента
	hash_map<int, SScriptInfo>::iterator segmIter;

	hash_map<string, NDb::SScriptArea> areas;

	CPtr<IConsoleBuffer> pConsole;
	bool bShowErrors;
	static CDBPtr<NDb::SMapInfo> pMapInfo;
	static CAttackGroup attackGroups;
	static list<CObj<CAIUnit> > rememberedUnits;
	
	//
	// удалить все невалидные юниты в начале данной группы
	void DelInvalidBegin( const int targetId );

	// вывести сообщение об ошибке
	void OutScriptError( const char *pszString );

	// проставить новые линки подкреплению
	void SetNewLinksToReinforcement( CReinfList *pReinf, hash_map<int, int> *pOld2NewLinks );
	//
	bool CanLandWithShift( const SMapObjectInfo &mapObject, CVec2 *pvShift );
	bool CanFormationLand( const SMapObjectInfo &mapObject, const CVec2 &vShift = VNULL2 );
	bool CanUnitLand( const SMapObjectInfo &mapObject, const CVec2 &vShift = VNULL2 );

	void LandReinforcementWithoutLandCheck( CReinfList *pReinf, const CVec2 &vShift );
	void LandSuspendedReiforcements();
	
	//
	// return false if command is invalid
	static bool ParseCommand( struct SAIUnitCmd *pCmd, Script &sctipt, bool bIDsAreScriptIDs );
	static int ProcessCommand( struct lua_State *state, const bool bPlaceInQueue, const bool b2NdParamIsScriptID );
	//	
	interface ICheckObjects
	{ 
		virtual bool IsGoodObj( class CExistingObject *pObj ) const = 0; 
	};
	int GetCheckObjectsInScriptArea( const NDb::SScriptArea &area, const interface ICheckObjects &check );
	
	void SendShowReinoforcementPlacementFeedback( list<CVec2> *pCenters );

	static void EnumUnitsInScriptArea( IScriptAreaEnumerator *pEnumerator, const int nPlayer, bool bCountPlanes );
	static void EnumUnitsInRect( IEnumerator *pEnumerator, const int nPlayer, bool bCountPlanes, const SRect &rect );
	static void EnumUnitsInCircle( IEnumerator *pEnumerator, const int nPlayer, bool bCountPlanes, const CVec2 &vCenter, const float fRadius );
	static bool IsUnitInCirlceArea( CAIUnit *pUnit, const CVec2 &vCenter, const int fR, const int nPlayer );
	static bool IsUnitInRectArea( CAIUnit *pUnit, const SRect &rect, const int nPlayer );

	static void SendUpdateToObjects( int nScriptID, EActionNotify eUpdateType, int nParam );
	
	static void AttackGroupAddUnit( class CCommonUnit * pUnit, int nAttackGroupID );
	static void RemoveUnit( CAIUnit * pUnit );
	//
	void RunScriptFromFile( const string &szScriptFileName );
public:
	~CScripts();
	static void RegisterScriptForSaveLoad();

	int GetScriptID( CUpdatableObj *pObj ) const;
	void AddObjToScriptGroup( CUpdatableObj *pObj, const int nGroup );
	void AddUnitToReinforcGroup( const SMapObjectInfo &mapObject, const int nGroup, const struct SHPObjectRPGStats *pStats/*, IScenarioUnit *pScenarioUnit */);
	// удалить все невалидные юниты в группе, 
	void DelInvalidUnits( const int scriptId );
	
	void Init( const NDb::SMapInfo* pMapInfo );
	void Clear();
	void InitAreas( const NDb::SScriptArea scriptAreas[], const int nLen );
	void Load( const string &szScriptFileName, const NDb::SAIGameConsts *pConsts );

	void Segment();

	void CallScriptFunction( const char *pszCommand );

	void SetGlobeScriptHandler( IGlobeScriptHandler *pHandler ) { pGlobeScriptHandler = pHandler; }
	IGlobeScriptHandler* GetGlobeScriptHandler() const { return pGlobeScriptHandler; }

	//
	// script functions
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// return 1 if object or unit is alive or squad has 1 memeber at least
	static int IsAlive( struct lua_State *state );

	// music system commands
	static int PlayVoice( struct lua_State *pState );

	// return time in seconds
	static int GetGameTime( struct lua_State *state );
	// params: <number of player> <x coord of the circle> <y coord> <radius>; returns: number of units;
	static int GetNUnitsInCircle( struct lua_State *state );
	//
	// params: <nPlayer, nReinforcementType, bEnable> ; returns: nothing;
	static int CallReinforcement( struct lua_State *state );
	static int GetReinforcementCallsLeft( struct lua_State *state );
	static int IsReinforcementAvailable( struct lua_State *state );

	// sounds
	//static int StartSound( struct lua_State *pState );
	//static int StopSound( struct lua_State *pState );

	// params: < nPlayer, nReinforcement, nDeploy, nReinfPoint > <"name of script area">; returns: nothing;
	static int LandReinforcement( struct lua_State *state );
	static int LandReinforcementFromMap( struct lua_State *state );
	// params: <player, factoryNumber, recycletime>; returns : nothing

	// params: <number of player> <"name of script area">; returns: number of units;
	static int GetNUnitsInArea( struct lua_State *state );
	static int IsSomeUnitInArea ( struct lua_State *state );
	static int IsSomeBodyAlive( struct lua_State *pState );
	// params: <number of player> <"name of script area">; returns: units in script area link ids ;
	static int GetUnitListInArea( struct lua_State *state );
	static int GetUnitListOfPlayer( struct lua_State *state );
	// params: <number of player> <"name of script area">; returns: bool is unit in this area;
	static int IsUnitInArea( struct lua_State *state );
	// params: <ScriptID>; returns: LinkIDs of units, that have this ScriptID
	static int GetObjectList( struct lua_State *state );
	// params: <ScriptArea> | <center>, <radius>, returns sorted (nearest first) list of free artillery
	static int GetFreeArtillery( struct lua_State *state );
	//
	// params: <number of script group> <"name of script area">; returns: number of units;
	static int GetNScriptUnitsInArea( struct lua_State *state );
	//
	// params: <number of script group>; returns: number of units;
	static int GetNUnitsInScriptGroup( struct lua_State *state );
	//	
	// params: <number of reinforcement>; returns: none;
	static int LandReinforcementOLD( struct lua_State *state );
	//
	// params: <number of winner party>; returns: none;
	static int Win( struct lua_State *state );
	
	// no params
	static int Draw( struct lua_State *state );
	//
	// params: none; returns: none;
	static int Loose( struct lua_State *state );
	//
	// params: <command> <script group id> <necessary command parameters>; returns: none;
	static int GiveCommand( struct lua_State *state );

	// params: <command> <script group id> <necessary command parameters>; returns: none;
	static int GiveQCommand( struct lua_State *state );

	// params: <command> <unit unique ID> <necessary command parameters>; returns: none;
	static int GiveUnitCommand( struct lua_State *state );
	// params: <command> <unit unique ID> <necessary command parameters>; returns: none;
	static int GiveUnitQCommand( struct lua_State *state );


	// params: none; returns: none;
	static int ShowActiveScripts( struct lua_State *state );
	
	// params: <party of warfog>; returns: none;
	static int ChangeWarFog( struct lua_State *state );

	// params: <number of script group> <number of new player>; returns: none;
	static int ChangePlayer( struct lua_State *state );
	
	// params: <number of player> <number of mode>; returns: none;
	// nMode = 0 - снять god mode полностью
	// nMode = 1 - неубиваемость
	// nMode = 2 - неубиваемость и убийство с первого раза
	// nMode = 3 - убийство с первого раза
	// nMode = 4 - снять только неубиваемость
	// nMode = 5 - снять только убийство с первого раза
	static int God( struct lua_State *state );

	// params: <name of global var> <integer value of global var>; returns: none;
	static int SetIGlobalVar( struct lua_State *state );
	// params: <name of global var> <float value of global var>; returns: none;
	static int SetFGlobalVar( struct lua_State *state );
	// params: <name of global var> <string value of global var>; returns: none;
	static int SetSGlobalVar( struct lua_State *state );
	
	// params: <name of global var> <default interger value of global var>; returns: <integer value of global var>;
	static int GetIGlobalVar( struct lua_State *state );
	// params: <name of global var> <default float value of global var>; returns: <float value of global var>;
	static int GetFGlobalVar( struct lua_State *state );
	// params: <name of global var> <default string value of global var>; returns: <string value of global var>;
	static int GetSGlobalVar( struct lua_State *state );
	
	// params: <object's script id>; returns: <float value of hps>
	static int GetObjectHPs( struct lua_State *state );

	// params: <number of party>; returns: <number of units in party>
	static int GetNUnitsInParty( struct lua_State *state );
	static int IsSomeUnitInParty( struct lua_State *state );
	static int IsSomePlayerUnit( struct lua_State *pState );
	static int IsUnitNearScriptObject( struct lua_State *state );
	// формация считается за один юнит
	static int GetNUnitsInPartyUF( struct lua_State *pState );
	// формация считается за один юнит
	static int GetNUnitsInPlayerUF( struct lua_State *pState );
	
	// params: <script id of squad> <number of new formation>; returns: none;
	static int ChangeFormation( struct lua_State *state );
	
	// params: <format string> <float parameter>...<float parameter>; returns: <none>
	// out trace info to the console
	static int Trace( struct lua_State *state );
	// params: <format string> <float parameter>...<float parameter>; returns: <none>
	// out trace info on display
	static int DisplayTrace( struct lua_State *state );
	
	// params: <number of objective> <new value of objective>; returns: none;
	static int ObjectiveChanged( struct lua_State *state );
	
	// params: <script id of unit>; returns: <number of primary ammo> <number of secondary ammo>
	static int GetNAmmo( struct lua_State *state );
	
	// params: <script id>; returns: <party>
	static int GetPartyOfUnits( struct lua_State *state );
	
	// params: <script id> <damage value>; returns: none;
	// если damage == 0, то объект уничтожается
	// если damage < 0, то объект лечится
	static int DamageObject( struct lua_State *pState );

	// params: <script id> <catch flag>; returns: none;
	static int SetCatchArtFlag( struct lua_State *pState );

	// params: <unique id>; returns: <id of unit state>
	// returns 0, if state is unknown or not set
	// returns -1, if unit doesn't exist
	static int GetUnitState( struct lua_State *pState );

	// params: <unique id>;
	// returns array of states
	// returns -1, if unit doesn't exist
	static int GetSquadStates( struct lua_State *pState );
	
	// params: <unique id>; returns: <id of unit state>
	static int GetUnitRPGStats( struct lua_State *pState );
	
	// params: <unique id> <new state 0 or 1>; returns: none;
	static int SwitchUnitLightFX( struct lua_State *pState );

	// params: <script id> <new state 0 or 1>; returns: none;
	static int SwitchSquadLightFX( struct lua_State *pState );

	// params: object id, num in array, returns none;	
	static int ObjectPlayAttachedEffect( struct lua_State *pState );
	static int ObjectStopAttachedEffect( struct lua_State *pState );

	static int ObjectPlayAnimation( struct lua_State *pState );
	
	// params: <script id>; returns: <squad info>
	// returns -3, is the object doesn't exist,
	// returns -2, if it isn't a squad
	// returns -1, if it is a disbanded squad,
	// returns number of squad order, if it's a squad
	static int GetSquadInfo( struct lua_State *pState );
	
	// params: <script id>; returns: <follow info>
	// returns -1, is the object doesn't exist or not a unit
	// returns 0, if isn't following
	// returns 1, if is following
	static int IsFollowing( struct lua_State *pState );
	
	// params: <script id>; returns: <directions in range [0, 65535]>
	// returns -1 if unit doesn't exist or the object isn't a unit
	static int GetFrontDir( struct lua_State *pState );
	
	// params: <script id>; returns: <shell type>
	static int GetActiveShellType( struct lua_State *pState );

	// params: <request string>; returns: none;
	static int AskClient( struct lua_State *pState );

	// params: none; returns: float random in [0, 1]
	static int RandomFloat( struct lua_State *pState );
	// params: n; returns: int random in [0, n-1]
	static int RandomInt( struct lua_State *pState );

	// params: <script id> <nParam>; returns: none;
	// nParam == 1 - select, nParam == 0 - deselect
	static int ChangeSelection( struct lua_State *pState );
	
	// params: <none>; returns: <mask of existing players>
	static int GetPlayersMask( struct lua_State *pState );
	// params: <player>; returns: 0 or 1
	static int IsPlayerPresent( struct lua_State *pState );
	
	// params: <script id>; returns: x y
	// if script group doesn't exist, return ( -1, -1 )
	static int ObjectGetCoord( struct lua_State *pState );

	// params: <name of script area>; returns: x y halflength halfwidth
	static int GetScriptAreaParams( struct lua_State *pState );
	
	// params: <bool>; returns: none
	static int SwitchWeather( struct lua_State *pState );
	// params: <bool>; returns: none
	static int SwitchWeatherAutomatic( struct lua_State *pState );

	// params: <diplomatic side>; returns: number of units, belonging to the side
	static int GetNUnitsInSide( struct lua_State *pState );
	
	// params: <script id>; returns: none
	static int AddIronMan( struct lua_State *state );

	// params: <difficulty level>; returns: none
	static int SetDifficultyLevel( struct lua_State *state );
	static int SetCheatDifficultyLevel( struct lua_State *state );

	// params: <script id of reinforcement to delete>; returns: none
	static int UnitRemove( struct lua_State *pState );
	
	// params: <"name of script area"> <1 - open, 0 - close>; returns: none
	static int ViewZone( struct lua_State *pState );
	
	// params: <script id of unit>; returns: 0 or 1
	static int IsStandGround( struct lua_State *pState );
	// params: <script id of unit>; returns: 0 or 1
	static int IsEntrenched( struct lua_State *pState );

	// params: <script area name>; return: number of antiperson fences
	static int GetNAPFencesInScriptArea( struct lua_State *pState );
	// params: <script area name>; return: number of antitanks
	static int GetNAntitankInScriptArea( struct lua_State *pState );
	// params: <script area name>; return: number of fences
	static int GetNFencesInScriptArea( struct lua_State *pState );
	// params: <script area name>; return: number of trenches
	static int GetNTrenchesInScriptArea( struct lua_State *pState );
	// params: <script area name>; return: number of mines
	static int GetNMinesInScriptArea( struct lua_State *pState );
	static int GetPassangers( struct lua_State *state );

	// params player's number. returns type of last aviation called or -1 if no aviation was called
	//static int GetAviationState( struct lua_State *state );
	
	static int Password( struct lua_State *pState );
	//
	// for internal usage
	static int ReturnScriptIDs( struct lua_State *pState );
	
	//
	static int SetGameSpeed( struct lua_State *pState );

	// params: <name of unit type> <number of party>; return: number of units
	// работает медленно!
	static int GetNUnitsOfType( struct lua_State *pState );
	// params: none; returns: <xsize ysize> in AI points
	static int GetMapSize( struct lua_State *pState );
	
	// check for mission bonus availibility
	static int CheckMissionBonus( struct lua_State *pState );
	//
	// for debug
	// params: none; return: none;
	static int CallAssert( struct lua_State *pState );
		
	// params: <none> <number of party>; return: none
	static int StartSequence( struct lua_State *pState );

	static int StartSequenceWOMovieBorder( struct lua_State *pState );
	static int EndSequenceWOMovieBorder( struct lua_State *pState );

	static int ShowMovieBorder( struct lua_State *pState );
	static int HideMovieBorder( struct lua_State *pState );
	
	// params: <none> <number of party>; return: none
	//static int EndSequence( struct lua_State *pState );
	// params: <int cameraPositionNumber, int millisecondsToMove> <number of party>; return: none
	//static int CameraMove( struct lua_State *pState );

	static int GameMesage( struct lua_State *pState );
	//static int AddChatMessage( struct lua_State *pState );
	static int WaitForGroupInArea( struct lua_State *pState );

	// params: <player no> <reinforcement type number> <amount of xp>; returns: none
	static int GiveXPToReinforcement( struct lua_State *pState );
	// params: <player no> <amount of xp>; returns: none
	static int GiveXPToPlayer( struct lua_State *pState );

	static int GlobeCommand( struct lua_State *pState );
	//
	// script camera
	static int SCRunTime( struct lua_State *pState );
	static int SCRunSpeed( struct lua_State *pState );
	static int SCReset( struct lua_State *pState );
	static int SCStartMovie( struct lua_State *pState );
	static int SCStopMovie( struct lua_State *pState );
	// attack group
	static int AttackGroupCreate( struct lua_State *pState );
	static int AttackGroupAddUnit( struct lua_State *pState );
	static int AttackGroupStartAttack( struct lua_State *pState );
	static int AttackGroupDelete( struct lua_State *pState );

	//
	static int PlayerCanSee( struct lua_State *pState );
	static int UnitCanSee( struct lua_State *pState );
	static int SetAmmo( struct lua_State *pState );
	static int UnitPlayAnimation( struct lua_State *pState );
	static int PlayEffect( struct lua_State *pState );
	static int GetAmmo( struct lua_State *pState );

	static int IsImmobilized( struct lua_State * pState );
	static int EnablePlayerReinforcement( struct lua_State *pState );
	static int EnablePlayerSuperWeapon( struct lua_State *pState );
	static int GiveReinforcementCalls( struct lua_State *pState );
	static int RemoveAllUnitsTmp( struct lua_State *pState );
	static int ReturnAllUnits( struct lua_State *pState );
	// debug
	static int SetLeader( struct lua_State *pState );
	static int RegisterCommandObserver( struct lua_State *pState );
	static int GetDifficultyLevel( struct lua_State *pState );
	static int SetDynamicObjectiveScriptGroup( struct lua_State *pState );
	
	// For testers
	static int DamageAllUnits( struct lua_State *pState );
	static int AddAmmoToAllUnits( struct lua_State *pState );

	static int BlinkActionButton( struct lua_State *pState );

	static SRegFunction pRegList[];
};

#endif // __SCRIPT_FUNCTIONS_H__
