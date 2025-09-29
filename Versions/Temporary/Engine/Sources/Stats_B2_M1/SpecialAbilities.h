#pragma once

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*enum EUnitSpecialAbility
{
	EUSA_NOTABILITY,
	EUSA_CAMOFLAGE,
};*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum EAbilitySwitchState
{
	EASS_DISABLE				= 0, // disabled by external conditions.
	EASS_ACTIVE					= 1,
	EASS_OFF						= 2,			
	EASS_SWITCHING_ON		= 3,
	EASS_SWITCHING_OFF	= 4,
	EASS_READY_TO_ON		= 5, // regenereated, but not switched on
	EASS_CHANGE_AUTOCAST_ONLY			= 6, // не менять текущее состояние абилити, только автокаст
};

struct SAbilitySwitchState 
{
	union 
	{
		struct  
		{
			unsigned						eState		: 8;	//EAbilitySwitchState
			unsigned						bAutocast	: 8;
			unsigned						crap				: 16;
		};
		DWORD dwStateValue;
	};
	SAbilitySwitchState( const SAbilitySwitchState &src ) { dwStateValue = src.dwStateValue; }
	SAbilitySwitchState( const EAbilitySwitchState _eState ) : eState(_eState), bAutocast(false), crap( 0 ) { }
	SAbilitySwitchState() : eState(EASS_READY_TO_ON), bAutocast(false), crap( 0 ) {}
};


