#pragma once

// automatically generated file, don't change manually!


struct IXmlSaver;

namespace NDb
{
	enum EUnitAckType;
	struct SComplexSoundDesc;

	enum EUnitAckType
	{
		ACK_NONE = 0,
		ACK_BORED_ATTACK = 1,
		ACK_BORED_IDLE = 2,
		ACK_BUILDING_FINISHED = 3,
		ACK_CANNOT_FINISH_BUILD = 4,
		ACK_CANNOT_MOVE_TRACK_DAMAGED = 5,
		ACK_CANNOT_PIERCE = 6,
		ACK_DONT_SEE_THE_ENEMY = 7,
		ACK_GOING_TO_STORAGE = 8,
		ACK_INVALID_TARGET = 9,
		ACK_KILLED_ENEMY = 10,
		ACK_NEGATIVE = 11,
		ACK_NEGATIVE_NOTIFICATION = 12,
		ACK_NO_AMMO = 13,
		ACK_NO_RESOURCES_CANT_FIND_DEPOT = 14,
		ACK_NOT_IN_ATTACK_ANGLE = 15,
		ACK_NOT_IN_FIRE_RANGE = 16,
		ACK_PLANE_LEAVING = 17,
		ACK_PLANE_REACH_POINT_START_ATTACK = 18,
		ACK_POSITIVE = 19,
		ACK_SELECTED = 20,
		ACK_SELECTION_TO_MUCH = 21,
		ACK_START_SERVICE_REPAIR = 22,
		ACK_START_SERVICE_RESUPPLY = 23,
		ACK_UNIT_DIED = 24,
		ACK_BEING_ATTACKED_BY_AVIATION = 25,
		____ACK_B2_SPECIFIC = 26,
		ACK_MOVE_END = 27,
		ACK_UNDER_ATTACK = 28,
		ACK_ENEMY_FOUND = 29,
		ACK_ORDER_FINISHED = 30,
		ACK_REINFORCEMENT_ARRIVED = 31,
		ACK_MINE_FOUND = 32,
		ACK_START_SERVICE_BUILDING = 33,
		ACK_LOW_AMMO = 34,
		ACK_LOW_HIT_POINTS = 35,
		____ACK_M1_SPECIFIC = 36,
		ACK_BORED_LOW_AMMO = 37,
		ACK_BORED_LOW_HIT_POINTS = 38,
		ACK_ENEMY_IS_TO_CLOSE = 39,
		ACK_ATTACKING_AVIATION = 40,
		ACK_BORED_INFANTRY_TRAVEL = 41,
		ACK_BORED_MINIMUM_MORALE = 42,
		ACK_BORED_NO_AMMO = 43,
		ACK_BORED_RUSH = 44,
		ACK_BORED_SNIPER_SNEAK = 45,
		ACK_CANNOT_FIND_PATH_TO_TARGET = 46,
		ACK_CANNOT_MOVE_NEED_TO_BE_TOWED_TO_MOVE = 47,
		ACK_CANNOT_MOVE_WAITING_FOR_LOADERS = 48,
		ACK_CANNOT_START_BUILD = 49,
		ACK_CANNOT_SUPPLY_NOT_PATH = 50,
		ACK_DIVEBOMBER_CANNOT_DIVE = 51,
		ACK_ENEMY_ISNT_IN_FIRE_SECTOR = 52,
		ACK_GETTING_AMMO = 53,
		ACK_KILLED_ENEMY_AVIATION = 54,
		ACK_KILLED_ENEMY_INFANTRY = 55,
		ACK_KILLED_ENEMY_TANK = 56,
		ACK_NEED_INSTALL = 57,
		ACK_NO_CANNOT_HOOK_ARTILLERY_NO_PATH = 58,
		ACK_NO_CANNOT_UNHOOK_ARTILLERY_HERE = 59,
		ACK_NO_ENGINEERS_CANNOT_REACH_BUILDPOINT = 60,
		ACK_NO_RESOURCES_CANT_FIND_PATH_TO_DEPOT = 61,
		ACK_NO_TOO_HEAVY_ARTILLERY_FOR_TRANSPORT = 62,
		ACK_NOT_TRACEABLE = 63,
		ACK_PLANE_TAKING_OFF = 64,
		ACK_CANNOT_ENTER = 65,
		ACK_CANNOT_STORM = 66,
		ACK_UNIT_BUSY = 67,
		ACK_CANNOT_LOAD = 68,
		ACK_CANNOT_UNLOAD = 69,
	};

	enum EAckClass
	{
		ACKT_POSITIVE = 0,
		ACKT_NEGATIVE = 1,
		ACKT_SELECTION = 2,
		ACKT_NOTIFY = 3,
		ACKT_BORED = 4,
	};

	enum EAckPosition
	{
		ACK_POS_UNIT = 0,
		ACK_POS_INTERFACE = 1,
	};

	struct SAckType
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		EUnitAckType eAckType;
		CDBPtr< SComplexSoundDesc > pAck;

		SAckType() :
			__dwCheckSum( 0 ),
			eAckType( ACK_NONE )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};
}

namespace NDb
{
	string EnumToString( NDb::EUnitAckType eValue );
	EUnitAckType StringToEnum_NDb_EUnitAckType( const string &szValue );
}

template <>
struct SKnownEnum<NDb::EUnitAckType>
{
	enum { isKnown = 1 };
	static string ToString( NDb::EUnitAckType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EUnitAckType ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_EUnitAckType( szValue ); }
};

namespace NDb
{
	string EnumToString( NDb::EAckClass eValue );
	EAckClass StringToEnum_NDb_EAckClass( const string &szValue );
}

template <>
struct SKnownEnum<NDb::EAckClass>
{
	enum { isKnown = 1 };
	static string ToString( NDb::EAckClass eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EAckClass ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_EAckClass( szValue ); }
};

namespace NDb
{
	string EnumToString( NDb::EAckPosition eValue );
	EAckPosition StringToEnum_NDb_EAckPosition( const string &szValue );
}

template <>
struct SKnownEnum<NDb::EAckPosition>
{
	enum { isKnown = 1 };
	static string ToString( NDb::EAckPosition eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EAckPosition ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_EAckPosition( szValue ); }
};

