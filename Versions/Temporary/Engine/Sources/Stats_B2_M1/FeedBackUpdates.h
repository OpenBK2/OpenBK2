#pragma once

#include "AIUpdates.h"

enum EFeedBack
{
	EFB_WIN		= 0,
	EFB_LOOSE = 1,
	EFB_DRAW = 2,
	EFB_MP_GAME_FINISHED,

	EFB_SCOUT_DISABLED,
	EFB_PARADROPERS_DISABLED,
	EFB_FIGHTERS_DISABLED,
	EFB_BOMBERS_DISABLED,
	EFB_SHTURMOVIKS_DISABLED,

	EFB_SCOUT_ENABLED,
	EFB_FIGHTERS_ENABLED,
	EFB_BOMBERS_ENABLED,
	EFB_PARADROPS_ENABLED,
	EFB_SHTURMOVIKS_ENABLED,

	EFB_AVIA_DISABLED,
	EFB_AVIA_ENABLED,

	EFB_OBJECTIVE_CHANGED,

	EFB_REINFORCEMENT_ARRIVED,						// "reinforcement has arrived" notification

	EFB_YOU_LOST_STORAGE,
	EFB_YOU_CAPTURED_STORAGE,

	EFB_UPDATE_TEAM_F_L_AGS,							// nParam == current team fLags
	EFB_UPDATE_TEAM_F_R_AGS,							// nParam == current team fRags
	EFB_UPDATE_TIME_BEFORE_CAPTURE,				// nParam == seconds before capture object

	EFB_ENEMY_AVIATION_CALLED,						// nParam == makelong( x y )
	EFB_ENEMY_STARTED_ANTIARTILLERY,			// nParam == makelong( x y )

	EFB_PLACE_MARKER,											// nParam == makelong( x y )

	EFB_AVIA_KILLED,											// nParam == SUCAviation::AIRCRAFT_TYPE | nType << 16. our player's = 1, our party's = 2, enemy's = 3

	EFB_ASK_FOR_WARFOG,

	EFB_REINFORCEMENT_CENTER_LOCAL_PLAYER,// nParam == makelong( x y )
	EFB_SCENARIO_UNIT_DEAD,								// nParam == makelong( x y )
	EFB_SNIPER_DEAD,											// nParam == makelong( x y )
	EFB_TROOPS_PASSED,										// nParam == makelong( fromPlayer toPlayer )
	EFB_AMMUNITION_ARRIVED,
	EFB_UNABLE_TO_LAND,
	EFB_PLAY_EFFECT,
	EFB_NONE,

	// new feedback system
	_EFB_NEW_START,
	EFB_REMOVE_FEEDBACK = _EFB_NEW_START,									// nParam == id to remove
	
	EFB_KEYBUILDING_ATTACKED,
	EFB_COMMANDER_LEVELUP,

	EFB_AAGUN_FIRED,
	EFB_HOWITZER_GUN_FIRED,
	EFB_ENEMY_SIGHTED,										// enemy list is passed
	EFB_UNDER_ATTACK,
	EFB_TRACK_BROKEN,
	EFB_NO_AMMO, 
	EFB_MINE_SIGHTED,											// mines list is passed
	EFB_ENGINEER_WORK_FINISHED,
	EFB_WORK_TERMINATED,									// nParam == makelong( x y )

	EFB_GAIN_EXP,													// reinforcement gain some XP
	EFB_RIVER_POINT,											// nParam == makelong( x y )
	EFB_LOCAL_PLAYER_UNITS_PRESENT,				// nParam == bPresent
	EFB_OBJECTIVE_MOVED,									// units list is passed
	EFB_UNITS_GIVEN,											// nParam == makelong( x y )
};

struct SFeedBackObjectiveState : public CObjectBase
{
	OBJECT_BASIC_METHODS( SFeedBackObjectiveState )
		CVec2 vPos;
	int nState;
public:
	SFeedBackObjectiveState() {  }
	SFeedBackObjectiveState( int _nState, const CVec2 &_vPos ) : nState( _nState ), vPos( _vPos ) {  }
};

struct SFeedBackUnitsArray : public CObjectBase
{
	OBJECT_BASIC_METHODS( SFeedBackUnitsArray )
public:
	ZDATA
		std::vector<int> unitIDs;
	int nUpdateID;
	CVec2 vCenterCamera;
	int nParam;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&unitIDs); f.Add(3,&nUpdateID); f.Add(4,&vCenterCamera); f.Add( 5, &nParam);return 0; }

	SFeedBackUnitsArray() : nParam( -1 ) { }
};

struct SAIFeedBack
{
	ZDATA
		EFeedBack feedBackType;
	uint32_t nParam;
	CPtr<CObjectBase> pParam;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&feedBackType); f.Add(3,&nParam); f.Add(4,&pParam); return 0; }
public:
	SAIFeedBack() { }
	SAIFeedBack( const EFeedBack _feedBackType ) : feedBackType( _feedBackType ), nParam( uint32_t(-1) ) { }
	SAIFeedBack( const EFeedBack _feedBackType, uint32_t _nParam, CObjectBase *_pParam )
		: feedBackType( _feedBackType ), nParam( _nParam ), pParam( _pParam ) { }

	SAIFeedBack( const SAIFeedBack &feedBack ) : 
	feedBackType( feedBack.feedBackType ), 
		nParam( feedBack.nParam ), pParam( feedBack.pParam ) { }
	const SAIFeedBack& operator=( const SAIFeedBack &feedBack ) { feedBackType = feedBack.feedBackType; nParam = feedBack.nParam; return *this; }
};

struct SAIFeedbackUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAIFeedbackUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		SAIFeedBack info;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); return 0; }
};


