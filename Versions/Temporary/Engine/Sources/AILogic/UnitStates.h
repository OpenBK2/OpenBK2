#pragma once
#include "..\Stats_B2_m1\ActionCommand.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum EUnitStateNames
{
	// нумерацию не менять! используется в скриптах
	EUSN_ERROR											= 0,
	EUSN_REST												= 1,						// отдыхает
	EUSN_WAIT_FOR_PASSENGER					= 2,						// ждёт загрузки пассажира
	EUSN_REST_ON_BOARD							= 3,						// находится внутри юнита
	EUSN_LAND												= 4,						// выгружается из юнита
	EUSN_ENTER_TRANSPORT						= 5,						// загружается в транспорт
	EUSN_ENTER											= 6,						// входит в здание
	EUSN_ENTER_ENTRENCHMENT					= 7,						// входит в окоп
	EUSN_REST_IN_BUILDING						= 8,						// находится внутри здания
	EUSN_REST_ENTRENCHMENT					= 9,						// находится внутри окопа
	EUSN_GO_OUT											= 10,						// выходит из здания
	EUSN_SWARM											= 11,						// swarm в точку
	EUSN_GO_OUT_ENTRENCHMENT				= 12,						// выбегание из окопа
	EUSN_ATTACK_STAT_OBJECT					= 13,						// атака объекта
	EUSN_PARADE											= 14,						// выстроиться в формацию
	EUSN_AMBUSH											= 15,						// засада
	EUSN_RANGING										= 16,						// стрельба по пристрелянной области
	EUSN_BUILD_FENCE								= 18,						// строительство забора в прогрессе
	EUSN_BUILD_ENTRENCHMENT					= 19,						// строительство окопа
	EUSN_ATTACK_UNIT								= 21,						// атака юнита
	EUSN_ATTACK_UNIT_IN_BUILDING		= 22,						// атака юнита в здании
	EUSN_WAIT_TO_FORM								= 23,						// ожидание для сбора формации
	EUSN_BEING_TOWED								= 24,						// тащат
	EUSN_GUN_CREW_STATE							= 25,						// обслуживают пушку
	EUSN_BOMBING										= 26,						// боmбардировка
	EUSN_PARTROOP										= 27,						// сбрасывается с парашюта
	EUSN_FLY_DEAD										= 28,						// мертвый самолет улетает
	EUSN_BUILD_LONGOBJECT						= 29,						// строительство длинного обънкта(формация)
	EUSN_REPAIR_BRIDGE							= 30,
	EUSN_CLEAR_MINE									= 31,						// инженеры ищут мины
	EUSN_MOVE												= 32,
	EUSN_USE_SPYGLASS								= 33,
	EUSN_BOMBARDMANET								= 34,						// suppressive fire
	EUSN_REPAIR_BUILDING						= 35,
	EUSN_REPAIR_UNIT								= 36,
	EUSN_RESUPPLY_UNIT							= 37,
	EUSN_HUMAN_RESUPPLY							= 38,
	EUSN_BUILD_BRIDGE								= 39,
	EUSN_PLACE_ANTITANK							= 40,
	EUSN_PLACE_MINE									= 41,
	EUSN_MOVE_TO_RESUPPLY_CELL			= 42,						// special state for general's transports
	EUSN_TURN_TO_POINT							= 43,
	EUSN_USE												= 44,
	EUSN_GUN_CAPTURE								= 45,
	EUSN_MOVE_TO_GRID								= 46,
	EUSN_HOOK_ARTILLERY							= 47,
	EUSN_PLANE_LEAVE								= 48,
	EUSN_FORM_THROW_GRENADE					= 49,						// formation throw grenade
	EUSN_ATTACK_OBJECT							= 50,
	EUSN_ATTACK_IN_ENTRENCH					= 51,
	EUSN_ENTRENCH_SELF							= 52,
	EUSN_MECHUNIT_REST_ON_BOARD			= 53,
	EUSN_MOVE_BY_FORMATION					= 54,						// юниты (любые) двигаются шеренгой
	EUSN_LEAVE_SELF_ENTRENCH				= 55,
	EUSN_SUPPORT_FIRE								= 56,
	EUSN_PATROL											= 57,
	EUSN_PLANE_PATROL								= 58,
	EUSN_PLANE_SUICIDE							= 59,						// самолет совершает самоубийство (для V-2)
};
/*
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool IsRestState( const EUnitStateNames eStateName )
{
	return 
		eStateName == EUSN_REST || eStateName == EUSN_REST_ON_BOARD || 
		eStateName == EUSN_REST_ENTRENCHMENT || eStateName == EUSN_REST_IN_BUILDING;
}*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
inline bool IsRestCommand( const EActionCommand command )
{
	return 
		command == ACTION_COMMAND_IDLE_BUILDING ||
		command == ACTION_COMMAND_IDLE_TRENCH ||
		command == ACTION_COMMAND_IDLE_TRANSPORT ||
		command == ACTION_COMMAND_GUARD ;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum ETryStateInterruptResult
{
	TSIR_YES_IMMIDIATELY,
	TSIR_YES_WAIT,
	TSIR_YES_MANAGED_ALREADY,
	TSIR_NO_COMMAND_INCOMPATIBLE,
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
interface IUnitState : public CAIObjectBase
{
public:
	virtual void Segment() = 0;
	virtual EUnitStateNames GetName() { return EUSN_ERROR; }
	virtual bool IsRestState() const { return false; }
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) = 0;

	// state, в котором мы целенаправленно бежим кого-то убивать
	virtual bool IsAttackingState() const = 0;
	// точка назначения для state
	virtual const CVec2 GetPurposePoint() const = 0;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
interface IUnitAttackingState : public IUnitState
{
	virtual bool IsAttacksUnit() const = 0;
	virtual class CAIUnit* GetTargetUnit() const = 0;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CONVERT_OBJECT( CType, pObjTo, pObjFrom, msg )	\
	if ( pObjFrom == 0 ) break;														\
	CType *pObjTo = dynamic_cast<CType*>(pObjFrom);				\
	NI_ASSERT( pObjTo != 0, msg );											\
	if ( pObjTo == 0 ) break;

#define CONVERT_OBJECT_PTR( CType, pObjTo, pObjFrom, msg )		\
	if ( pObjFrom == 0 ) break;																	\
	CType *pObjTo = dynamic_cast_ptr<CType*>(pObjFrom);					\
	NI_ASSERT( pObjTo != 0, msg );														\
	if ( pObjTo == 0 ) break;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
