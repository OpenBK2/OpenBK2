#ifndef __ACTION_NOTIFY__
#define __ACTION_NOTIFY__

#pragma once

// ************************************************************************************************************************ //
// **
// ** notify structures
// **
// **
// **
// ************************************************************************************************************************ //

enum EActionNotify
{
	// ( EActionNotify & 1 ) == 1 => идут в UpdateActions
	// ( EActionNotify & 1 ) == 0 => разбрасvва_тся по различнvм функциям

	// actions
	ACTION_NOTIFY_IDLE												= 0x001,
	ACTION_NOTIFY_MOVE												= 0x011,
	ACTION_NOTIFY_PLACEMENT										= 0x000,
	ACTION_NOTIFY_ST_OBJ_PLACEMENT						= 0x010,

	ACTION_NOTIFY_AIM													= 0x021,
	ACTION_NOTIFY_MECH_SHOOT									= 0x020,
	ACTION_NOTIFY_INFANTRY_SHOOT							= 0x170,

	ACTION_NOTIFY_RPG_CHANGED									= 0x030,
	ACTION_NOTIFY_DIE													= 0x0e1, // умер, update на исчезновение не будет

	// Specific soldier actions
	ACTION_NOTIFY_CRAWL												= 0x031,
	ACTION_NOTIFY_IDLE_LYING									= 0x041,
	ACTION_NOTIFY_AIM_LYING										= 0x051,
	ACTION_NOTIFY_SHOOT_LYING									= 0x050, // только солдатv такое могут
	ACTION_NOTIFY_THROW												= 0x060,
	ACTION_NOTIFY_DIE_LYING										= 0x061, // умер, update на исчезновение не будет

	ACTION_NOTIFY_IDLE_TRENCH									= 0x071,
	ACTION_NOTIFY_AIM_TRENCH									= 0x081,
	ACTION_NOTIFY_SHOOT_TRENCH								= 0x070, // только солдатv такое могут
	ACTION_NOTIFY_THROW_TRENCH								= 0x080, // только солдатv такое могут
	ACTION_NOTIFY_DIE_TRENCH									= 0x090, // умер, update на исчезновение не будет

	ACTION_NOTIFY_SHOOT_BUILDING							=	0x0a0, // только солдатv такое могут
	ACTION_NOTIFY_THROW_BUILDING							= 0x160, // только солдатv такое могут
	ACTION_NOTIFY_DIE_BUILDING								=	0x0b0, // умер, update на исчезновение не будет

	ACTION_NOTIFY_IDLE_TRANSPORT							= 0x0c1,
	ACTION_NOTIFY_DIE_TRANSPORT								= 0x0d1, // умер, update на исчезновение не будет

	// Specific cannonry actions
	ACTION_NOTIFY_INSTALL_ROTATE							= 0x0181,
	ACTION_NOTIFY_UNINSTALL_ROTATE						= 0x0191,
	ACTION_NOTIFY_INSTALL_TRANSPORT						= 0x01a1,
	ACTION_NOTIFY_UNINSTALL_TRANSPORT 				= 0x01b1,
	ACTION_NOTIFY_FINISH_INSTALL_ROTATE				= 0x01c1,
	ACTION_NOTIFY_FINISH_UNINSTALL_ROTATE			= 0x01d1,
	ACTION_NOTIFY_FINISH_INSTALL_TRANSPORT		= 0x01e1,
	ACTION_NOTIFY_FINISH_UNINSTALL_TRANSPORT	= 0x01f1,

	// CRAP{ unnecessary, need to be deleted
	ACTION_NOTIFY_INSTALL											= 0x0f1,
	ACTION_NOTIFY_UNINSTALL										= 0x101,
	// CRAP}
	ACTION_NOTIFY_HIT													= 0x0f0,

	//
	ACTION_NOTIFY_NEW_PROJECTILE							= 0x100,
	ACTION_NOTIFY_DEAD_PROJECTILE							= 0x110,
	ACTION_NOTIFY_NEW_ST_OBJ									=	0x120,
	ACTION_NOTIFY_DELETED_ST_OBJ							= 0x130,
	ACTION_NOTIFY_NEW_UNIT										= 0x140,
	ACTION_NOTIFY_DISSAPEAR_OBJ								= 0x150,
	ACTION_NOTIFY_NEW_ENTRENCHMENT						= 0x1a0,
	ACTION_NOTIFY_NEW_FORMATION								= 0x1b0,

	ACTION_NOTIFY_ENTRANCE_STATE							= 0x190,	// войти куда-либо

	// всяческие support-действия типа постановка/снятие мин, ремонт/перезарядка техники и т.д.
	ACTION_NOTIFY_USE_UP											= 0x091,
	ACTION_NOTIFY_USE_DOWN										= 0x0a1,

	ACTION_NOTIFY_STOP												= 0x0b1,
	ACTION_NOTIFY_NEW_BRIDGE_SPAN							= 0x1c0,
	ACTION_NOTIFY_REVEAL_ARTILLERY						= 0x1d0,
	ACTION_NOTIFY_SET_CAMOUFLAGE							= 0x111,
	ACTION_NOTIFY_REMOVE_CAMOUFLAGE						= 0x121,
	ACTION_NOTIFY_SET_AMBUSH									= 0x131,
	ACTION_NOTIFY_REMOVE_AMBUSH								= 0x141,
	ACTION_NOTIFY_BREAK_TRACK									= 0x151,
	ACTION_NOTIFY_REPAIR_TRACK								= 0x161,
	ACTION_NOTIFY_UPDATE_DIPLOMACY						= 0x1e0,
	ACTION_NOTIFY_USE_SPYGLASS								= 0x171,
	ACTION_NOTIFY_USE_SPYGLASS_LYING					= 0x173,

	ACTION_NOTIFY_REPAIR_STATE_BEGIN					=	0x181,
	ACTION_NOTIFY_REPAIR_STATE_END						=	0x191,
	ACTION_NOTIFY_RESUPPLY_STATE_BEGIN				=	0x201,
	ACTION_NOTIFY_RESUPPLY_STATE_END					=	0x211,

	ACTION_NOTIFY_SHOOT_AREA									= 0x1f0,

	ACTION_NOTIFY_RANGE_AREA									= 0x200,

	ACTION_NOTIFY_TURRET_HOR_TURN							= 0x0e0,
	ACTION_NOTIFY_TURRET_VERT_TURN						= 0x220,

	ACTION_NOTIFY_DEAD_UNIT										= 0x230,

	ACTION_NOTIFY_ANIMATION_CHANGED						= 0x250,

	ACTION_NOTIFY_SELECTABLE_CHANGED					= 0x261,

	ACTION_NOTIFY_INSTALL_MOVE								= 0x271,
	ACTION_NOTIFY_UNINSTALL_MOVE							= 0x281,
	ACTION_NOTIFY_FINISH_INSTALL_MOVE					= 0x2a1,
	ACTION_NOTIFY_FINISH_UNINSTALL_MOVE				= 0x2b1,

	ACTION_NOTIFY_CHANGE_VISIBILITY						= 0x301,
	//
	ACTION_NOTIFY_DELAYED_SHOOT								= 0x291,
	ACTION_NOTIFY_STATE_CHANGED								= 0x311,
	ACTION_NOTIFY_SILENT_DEATH								= 0x321,
	ACTION_NOTIFY_SERVED_ARTILLERY						= 0x331,

	ACTION_NOTIFY_OPEN_PARASHUTE							=	0x341,
	ACTION_NOTIFY_PARASHUTE										=	0x351,
	ACTION_NOTIFY_CLOSE_PARASHUTE							=	0x361,
	ACTION_NOTIFY_CHANGE_DBID									= 0x371,
	ACTION_NOTIFY_CHANGE_FRAME_INDEX					= 0x381,
	ACTION_NOTIFY_FALLING											= 0x391,

	ACTION_NOTIFY_STORAGE_CONNECTED						= 0x3a1,
	ACTION_NOTIFY_START_BUILD_PIT							= 0x3b1,
	ACTION_NOTIFY_START_LEAVE_PIT							= 0x3c1,
	ACTION_NOTIFY_THROW_LYING									= 0x260,

	ACTION_NOTIFY_STAYING_TO_LYING						= 0x3d1,
	ACTION_NOTIFY_LYING_TO_STAYING						= 0x3e1,

	ACTION_NOTIFY_SHELLTYPE_CHANGED						= 0x3f1,
	ACTION_NOTIFY_DEADPLANE										= 0x401,

	ACTION_NOTIFY_CHANGE_SELECTION						= 0x421,		// nParam == 1 - select specified unit, nParam == 0 - deselect specified unit

	ACTION_NOTIFY_SIDE_CHANGED								= 0x431,		// player diplomacy changed. nParam - new player index

	ACTION_NOTIFY_ENTRENCHMENT_STARTED				= 0x451,

	ACTION_NOTIFY_REINF_RECYCLE								= 0x461,		
	ACTION_SET_SELECTION_GROUP								= 0x471,		
	//ACTION_NOTIFY_RU_STORAGE_AREA						= 0x210,
	ACTION_NOTIFY_CHANGE_SCENARIO_INDEX				= 0x481,
	// нужен только для отложеннvх updates
	ACTION_NOTIFY_FEEDBACK										= 0x491,
	ACTION_NOTIFY_AVAIL_REINF									= 0x4a1,
	ACTION_NOTIFY_REINF_POINT									= 0x4b1,

	ACTION_NOTIFY_SPECIAL_ABLITY							= 0x4c1,

	ACTION_NOTIFY_TREE_BROKEN									= 0x4d1,

	ACTION_NOTIFY_POINTLIGHT									= 0x4e1,
	ACTION_NOTIFY_HEADLIGHT										= 0x4f1,
	ACTION_NOTIFY_TOGGLE_DAY_NIGHT_WINDOWS		= 0x501,
	ACTION_NOTIFY_BREAK_WINDOW								= 0x511,

	ACTION_NOTIFY_MODIFY_ENTRANCE_STATE				= 0x512,  // разрешить/запретить вvход
	ACTION_NOTIFY_OBJECTS_UNDER_CONSTRUCTION	= 0x513, // разрешить/запретить вvход

	ACTION_NOTIFY_KEY_BUILDING_CAPTURED				= 0x520,	// A Key Building changed ownership
	ACTION_NOTIFY_KEY_BUILDING_LOST						= 0x530,	// A Key Building changed ownership
	ACTION_NOTIFY_KEY_CAPTURE_PROGRESS				= 0x531,	// A Key Building is being captured
	ACTION_NOTIFY_NEW_KEY_BUILDING						= 0x540,
	
	ACTION_NOTIFY_PARADROP_STARTED						= 0x541,

	ACTION_NOTIFY_DAMAGE										  = 0x600,  // нанести повреждение по определенной точке

	ACTION_NOTIFY_SCAMERA_RUN									= 0x700,  // script camera
	ACTION_NOTIFY_SCAMERA_RESET								= 0x701,  // script camera reset

	ACTION_NOTIFY_DISABLE_ACTION							= 0x702,
	ACTION_NOTIFY_ENABLE_ACTION								= 0x703,
	ACTION_NOTIFY_PARENT_OF_ATOM_OBJ				  = 0x704,

	ACTION_NOTIFY_WEATHER_CHANGED							= 0x705,  // weather changes
	ACTION_NOTIFY_PLAY_EFFECT									= 0x706,

	ACTION_NOTIFY_SC_START_MOVIE							= 0x707,  // start movie sequence
	ACTION_NOTIFY_SC_STOP_MOVIE								= 0x708,  // stop movie sequence
	ACTION_NOTIFY_PLANE_RETURNS_DUE_WEATHER		= 0x709,

	ACTION_NOTIFY_UPDATE_STATUS								= 0x800,
	ACTION_NOTIFY_SUPERWEAPON_CONTROL					= 0x801,
	ACTION_NOTIFY_SUPERWEAPON_RECYCLE					= 0x802,

	ACTION_NOTIFY_DIVEBOMBER_DIVE							= 0x803,	// nParam = bDive

	ACTION_NOTIFY_PLAY_ATTACHED_EFFECT				= 0x890,	// nParam = num effect in the attached lights array
	ACTION_NOTIFY_STOP_ATTACHED_EFFECT				= 0x8a0,	// nParam = num effect in the attached lights array

	ACTION_NOTIFY_GET_DEAD_UNITS_UPDATE				= 0xfffffffe,

	ACTION_NOTIFY_NONE												= 0xffffffff,
};

inline bool IsDyingAction( const EActionNotify eAction )
{
	return
		eAction == ACTION_NOTIFY_DIE ||
		eAction == ACTION_NOTIFY_DIE_LYING ||
		eAction == ACTION_NOTIFY_DIE_TRENCH ||
		eAction == ACTION_NOTIFY_DIE_BUILDING ||
		eAction == ACTION_NOTIFY_DIE_TRANSPORT;
}

// check, do we need animation + action or animation only (for the actions, which have animation)
inline bool DoWeNeedAction( const int nAction )
{
	switch ( nAction )
	{
	case ACTION_NOTIFY_IDLE:
	case ACTION_NOTIFY_IDLE_LYING:
	case ACTION_NOTIFY_MOVE:
	case ACTION_NOTIFY_CRAWL:
	case ACTION_NOTIFY_DIE:
	case ACTION_NOTIFY_DIE_LYING:
	case ACTION_NOTIFY_IDLE_TRENCH:
	case ACTION_NOTIFY_SHOOT_LYING:
	case ACTION_NOTIFY_SHOOT_TRENCH:
	case ACTION_NOTIFY_USE_UP:
	case ACTION_NOTIFY_USE_DOWN:
	case ACTION_NOTIFY_INSTALL:
	case ACTION_NOTIFY_UNINSTALL:
	case ACTION_NOTIFY_UNINSTALL_ROTATE:
	case ACTION_NOTIFY_INSTALL_ROTATE:
	case ACTION_NOTIFY_UNINSTALL_TRANSPORT:
	case ACTION_NOTIFY_INSTALL_TRANSPORT:
	case ACTION_NOTIFY_INSTALL_MOVE:
	case ACTION_NOTIFY_UNINSTALL_MOVE:
	case ACTION_NOTIFY_OPEN_PARASHUTE:
	case ACTION_NOTIFY_PARASHUTE:
	case ACTION_NOTIFY_CLOSE_PARASHUTE:
	case ACTION_NOTIFY_FALLING:
		return true;
	}
	return false;
}

#endif

