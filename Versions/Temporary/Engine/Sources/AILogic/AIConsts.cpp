#include "stdafx.h"

#include "DBAIConsts.h"
#include "ManuverInternal.h"
#include "Stats_B2_M1/RPGStatsAutomagic.h"
#include "System/Commands.h"
#include "System/det_map.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace NAIConsts
{
const int WAR_FOG_FULL_UPDATE()
{
	return SAIConsts::WAR_FOG_FULL_UPDATE;
}

const int VIS_POWER()
{
	return SAIConsts::VIS_POWER;
}
} // namespace NAIConsts

bool g_bUseRoundUnits;

float SConsts::PLANE_SPLINE_STEP = 0.1f;
float SConsts::PLANE_SPLINE_POINT_DIST = 90.0f;
int SConsts::PLANE_TARGET_SCAN_PERIOD = 1000;

const float SConsts::SCamouflage::SNEAK_ADDITION_PER_TIME = 0.5f;
const float SConsts::SCamouflage::SNEAK_DECREASE_PER_SHOT = 0.5f;
const float SConsts::SCamouflage::SNEAK_DECREASE_PER_SECOND_MOVE = 0.5f;
const int SConsts::SCamouflage::SNEAK_ADVANCED_TIMEOUT = 6000;
const int SConsts::SCamouflage::SNEAK_ADVANCED_RESET_TIMEOUT = 2000;

const uint16_t SConsts::STANDART_VIS_ANGLE = 32768;
const int SConsts::MAX_DIST_TO_RECALC_FOG = 55 * SConsts::TILE_SIZE;
const int SConsts::TURN_TOLERANCE = 0;
const uint16_t SConsts::DIR_DIFF_TO_SMOOTH_TURNING = 2000;
const int SConsts::NUMBER_ITERS_TO_LOOK_AHEAD = 7;
const int SConsts::MAX_LEN_TO_GO_BACKWARD = 8;
const int SConsts::SPEED_FACTOR = 800;
const short int SConsts::SPLINE_STEP = 6;
const int SConsts::CELL_COEFF = 4;
const int SConsts::CELL_SIZE = SConsts::CELL_COEFF * SConsts::TILE_SIZE; // must be divisible by TILE_SIZE
const int SConsts::BIG_CELL_COEFF = 8;
const int SConsts::BIG_CELL_SIZE = SConsts::BIG_CELL_COEFF * SConsts::TILE_SIZE;
const int SConsts::HIT_CELL_COEFF = 8;
const int SConsts::HIT_CELL_SIZE = SConsts::HIT_CELL_COEFF * SConsts::TILE_SIZE;
const int SConsts::MAX_UNIT_TILE_RADIUS = 6;
const int SConsts::MAX_UNIT_RADIUS = 160;
const int SConsts::BIG_PATH_SHIFT = 10;
const int SConsts::AI_START_VECTOR_SIZE = 10;
const int SConsts::GROUP_DISTANCE = 20 * SConsts::TILE_SIZE;
const float SConsts::ANTI_ARTILLERY_SCAN_TIME = 5000;
const float SConsts::BOUND_RECT_FACTOR = 1.0f;
const float SConsts::COEFF_FOR_LOCK = 1.0f;
const float SConsts::DIST_FOR_LAND = 1.2f * SConsts::TILE_SIZE;
float SConsts::GOOD_LAND_DIST = 1.4f * SConsts::TILE_SIZE;
const int SConsts::STATIC_OBJ_CELL = 8;
const int SConsts::STATIC_CONTAINER_OBJ_CELL = 32;
float SConsts::CURE_SPEED_IN_BUILDING = 0.001f;
int SConsts::TIME_TO_RETURN_GUN = 5000;
int SConsts::NUM_TO_SCAN_IN_SEGM = 50;
int SConsts::BEH_UPDATE_DURATION = 2000;
int SConsts::SOLDIER_BEH_UPDATE_DURATION = 3000;
int SConsts::AA_BEH_UPDATE_DURATION = 200;
float SConsts::REPAIR_COST_ADJUST = 50;
int SConsts::LONG_RANGE_ARTILLERY_UPDATE_DURATION = 5000;
int SConsts::DEAD_SEE_TIME = 2000;
int SConsts::TIME_OF_BUILDING_ALARM = 8000;
int SConsts::TIME_BEFORE_CAMOUFLAGE = 2000;
int SConsts::TIME_OF_LYING_UNDER_FIRE = 2000;
float SConsts::LYING_SOLDIER_COVER = 0.7f;
int SConsts::RADIUS_OF_HIT_NOTIFY = 5 * SConsts::TILE_SIZE;
int SConsts::TIME_OF_HIT_NOTIFY = 1000;
int SConsts::MINE_VIS_RADIUS = 3 * SConsts::TILE_SIZE;
int SConsts::MINE_CLEAR_RADIUS = 7 * SConsts::TILE_SIZE;
int SConsts::RADIUS_OF_FORMATION = 10 * SConsts::TILE_SIZE;
float SConsts::GUARD_STATE_RADIUS = 10 * SConsts::TILE_SIZE;
float SConsts::LYING_SPEED_FACTOR = 0.5f;
int SConsts::CALL_FOR_HELP_RADIUS = 20 * SConsts::TILE_SIZE;
int SConsts::AI_CALL_FOR_HELP_RADIUS = 20 * SConsts::TILE_SIZE;
int SConsts::CAMPING_TIME = 2000;
float SConsts::INSIDE_OBJ_WEAPON_FACTOR = 0.5f;
int SConsts::INSIDE_OBJ_COMBAT_PERIOD = 800;
int SConsts::TIME_TO_DISAPPEAR = 0;
int SConsts::THRESHOLD_INSTALL_TIME = 2000;
int SConsts::SHOOTS_TO_RANGE = 4;
float SConsts::RANDGED_DISPERSION_RADIUS_BONUS = 0.5f;
float SConsts::RANGED_AREA_RADIUS = 5 * SConsts::TILE_SIZE;
float SConsts::RELOCATION_RADIUS = 5 * SConsts::TILE_SIZE;
float SConsts::MAX_ANTI_ARTILLERY_RADIUS = 10 * SConsts::TILE_SIZE;
float SConsts::MIN_ANTI_ARTILLERY_RADIUS = SConsts::TILE_SIZE;
int SConsts::SHOTS_TO_MINIMIZE_LOCATION_RADIUS = 5;
int SConsts::AUDIBILITY_TIME = 20000;
int SConsts::REVEAL_CIRCLE_PERIOD = 2000;
float SConsts::GOOD_ATTACK_RPOBABILITY = 0.6f;
int SConsts::FIGHTER_PATROL_TIME = 400000;
float SConsts::PARATROOPER_FALL_SPEED = 0.05f;
int SConsts::PARADROP_SPRED = 4;
int SConsts::RESUPPLY_RADIUS = 1000;
int SConsts::RESUPPLY_RADIUS_MORALE = 300;

float SConsts::ARTILLERY_CHECKFREE_PERIOD = 20000;
float SConsts::FORMATION_CHECKFREEART_PERIOD = 30000;

int SConsts::TIME_QUANT = 160;
float SConsts::RU_PER_QUANT = 10;
float SConsts::ENGINEER_RU_CARRY_WEIGHT = 3000.0f;
float SConsts::SPY_GLASS_RADIUS = 1920.f;
uint16_t SConsts::SPY_GLASS_ANGLE = 5000;
float SConsts::HOLD_SECTOR_SIGHT_BONUS = 1.2f;
float SConsts::HOLD_SECTOR_DISPERSION_BONUS = 0.5f;
int SConsts::TIME_TO_ENTER_HOLD_SECTOR = 1000;

float SConsts::TRACK_TARGETING_AIM_BONUS = 2.0f;
float SConsts::TRACK_TARGETING_DAMAGE_MODIFIER = 0.5f;
float SConsts::AREA_DAMAGE_COEFF = 0.2f;
uint16_t SConsts::MIN_ROTATE_ANGLE = 6000;
float SConsts::RADIUS_TO_START_ANTIARTILLERY_FIRE = 320.0f;
float SConsts::TRANSPORT_RU_CAPACITY = 10000.0f;
float SConsts::TIME_OF_ALARM_UNDER_FIRE = 15000.0f;
float SConsts::STORAGE_RESUPPLY_RADIUS = 1000.0f;
float SConsts::TRAJ_BOMB_ALPHA = 0.0005f;
float SConsts::TRAJECTORY_BOMB_G = 0.001f;
float SConsts::GUN_CREW_TELEPORT_RADIUS = 5.0f;

float SConsts::PLANE_PARADROP_INTERVAL = 50.0f;
float SConsts::PLANE_PARADROP_INTERVAL_PERP_MIN = 50.0f;
float SConsts::PLANE_PARADROP_INTERVAL_PERP_MAX = 50.0f;
float SConsts::PLANE_FUEL_DEC_ECONOMY = 0.5f;
float SConsts::PLANE_FUEL_DEC = 2.0f;

int SConsts::PARATROOPER_GROUND_SCAN_PERIOD = 200;

float SConsts::TRANSPORT_MOVE_BACK_DISTANCE = 700.0f * 700.0f;
int SConsts::TRIES_TO_UNHOOK_ARTILLERY = 2;
int SConsts::ENGINEER_MINE_CHECK_PERIOD;
float SConsts::FIGHTER_VERTICAL_SPEED_UP = 150.0f;
float SConsts::FIGHTER_VERTICAL_SPEED_DOWN = 300.0f;
float SConsts::PLANE_GUARD_STATE_RADIUS = 3000;

float SConsts::TANK_TRACK_HIT_POINTS;

float SConsts::TRAJECTORY_LOW_LINE_RATIO = 0.6f;

const int SConsts::NUMBER_SOLDIER_DIRS = 8;

float SConsts::PLANE_DIVE_FINISH_DISTANCE_SQR = 100.0f * 100.0f;

float SConsts::PLANES_HEAVY_FORMATION_SIZE = 2.0f;
float SConsts::PLANES_SMALL_FORMATION_SIZE = 1.2f;
float SConsts::PLANES_START_RANDOM;

float SConsts::SNIPER_CAMOUFLAGE_DECREASE_PER_SHOOT = 0.1f;
float SConsts::SNIPER_CAMOUFLAGE_INCREASE = 0.0004f;
float SConsts::AMBUSH_ATTACK_BEGIN_CIRTERIA = 0.5f;
float SConsts::ARTILLERY_REVEAL_COEEFICIENT = 200.0f;
float SConsts::dispersionRatio[12][2] = {
	{ 1.0f, 1.0f },
	{ 1.0f, 2.0f },
	{ 1.0f, 3.0f },
	{ 1.0f, 4.0f },
	{ 1.0f, 4.0f },
	{ 1.0f, 1.0f },
	{ 1.0f, 1.0f },
	{ 1.0f, 1.0f },
	{ 1.0f, 1.0f },
	{ 1.0f, 1.0f }, // ATGM line
	{ 1.0f, 1.0f }, // ATGM top attack
	{ 1.0f, 1.0f }, // SAM
};

float SConsts::COEFF_FOR_RANDOM_DELAY = 1.21f;

float SConsts::HEIGHT_FOR_VIS_RADIUS_INC = 1.0f;

float SConsts::BURNING_SPEED = 0.002f;

float SConsts::FOLLOW_STOP_RADIUS = SConsts::TILE_SIZE * 9;
float SConsts::FOLLOW_EQUALIZE_SPEED_RADIUS = SConsts::TILE_SIZE * 12;
float SConsts::FOLLOW_GO_RADIUS = SConsts::TILE_SIZE * 11;
float SConsts::FOLLOW_WAIT_RADIUS = SConsts::TILE_SIZE * 24;

float SConsts::TRANSPORT_LOAD_RU_DISTANCE = 100.0f;
int SConsts::RESUPPLY_MAX_PATH = 30;
float SConsts::FATALITY_PROBABILITY = 0.1f;
float SConsts::DAMAGE_FOR_MASSIVE_DAMAGE_FATALITY = 0.7f;
float SConsts::MASSIVE_DAMAGE_FATALITY_PROBABILITY = 0.8f;

float SConsts::BOMB_START_HEIGHT;
float SConsts::STAND_LIE_RANDOM_DELAY = 500;
float SConsts::TRANSPORT_RESUPPLY_OFFSET = 100;
float SConsts::HP_BALANCE_COEFF = 500.0f;
int SConsts::SQUAD_MEMBER_LEAVE_INTERVAL = 200;
float SConsts::SOLDIER_RU_PRICE = 50;
float SConsts::LOW_HP_PERCENTAGE = 0.2f;
float SConsts::DIRECT_HIT_DAMAGE_COMBAT_SITUATION = 100;
int SConsts::DIRECT_HIT_TIME_COMBAT_SITUATION = 1000;
int SConsts::NUMBER_ENEMY_MECH_MOVING_TO_COMBAT_SITUATION = 3;
int SConsts::NUMBER_ENEMY_INFANTRY_MOVING_TO_COMBAT_SITUATION = 20;

float SConsts::OFFICER_COEFFICIENT_FOR_SCAN = 1.5f;
float SConsts::MAIN_STORAGE_HEALING_SPEED = 10.0f;
float SConsts::RADIUS_TO_TAKE_STORAGE_OWNERSHIP = 100.0f;
float SConsts::TANKPIT_COVER = 0.5f;
const float SConsts::CLOSEST_TO_RAILROAD_POINT_TOLERANCE = 100.0f;

float SConsts::FENCE_SEGMENT_RU_PRICE = 100;
float SConsts::ENTRENCHMENT_SEGMENT_RU_PRICE = 100;
float SConsts::MINE_RU_PRICE[] = { 1000, 2000 };
float SConsts::ANTITANK_RU_PRICE = 100;
int SConsts::COMPLEX_OBSTACLE_ANTITANK_PERIOD = 2;

int SConsts::RESIDUAL_VISIBILITY_TIME = 4000;
int SConsts::MED_TRUCK_HEAL_RADIUS = 1000;
float SConsts::MED_TRUCK_HEAL_PER_UPDATEDURATION = 0.5f;

int SConsts::PERIOD_OF_PATH_TO_FORMATION_SEARCH = 3000;
int SConsts::ENTRENCH_SELF_TIME = 10000;

// Changed from 30 -> 128
int SConsts::N_SCANNING_UNITS_IN_SEGMENT = 128;
int SConsts::GENERAL_CELL_SIZE = 0;

float SConsts::FLAG_RADIUS = 320.0f;
float SConsts::FLAG_POINTS_SPEED = 10.0f;
float SConsts::PLAYER_POINTS_SPEED = 10.0f;

float SConsts::FLAG_POINTS_TO_REINFORCEMENT = 30.0f;
float SConsts::FLAG_TIME_TO_CAPTURE = 5000.0f;

int SConsts::ARMOR_FOR_AREA_DAMAGE = 10;
float SConsts::BUILDING_FIREPLACE_DEFAULT_COVER = 0.5f;

int SConsts::DIVE_BEFORE_EXPLODE_TIME = 5000;
int SConsts::DIVE_AFTER_EXPLODE_TIME = 150;

int SConsts::WEATHER_TIME = 10000;
int SConsts::WEATHER_TIME_RANDOM = 20000;
int SConsts::WEATHER_TURN_PERIOD = 30000;
int SConsts::WEATHER_TURN_PERIOD_RANDOM = 30000;

float SConsts::BAD_WEATHER_FIRE_RANGE_COEFFICIENT = 0.5f;
int SConsts::TIME_TO_WEATHER_FADE_OFF = 5;
int SConsts::AA_AIM_ITERATIONS = 3;

float SConsts::COEFF_TO_LOW_MORALE_WITHOUT_OFFICER = 2.0f;

float SConsts::MAX_DISTANCE_TO_THROW_GRENADE = 320.0f;

float SConsts::MORALE_DISPERSION_COEFF = 1.0f;
float SConsts::MORALE_RELAX_COEFF = 1.0f;
float SConsts::MORALE_AIMING_COEFF = 1.0f;

float SConsts::TR_DISTANCE_TO_CENTER_FACTOR = 10;
float SConsts::TR_GUNPLANE_ALPHA_ATTACK_1 = 1.0f;
float SConsts::TR_GUNPLANE_ALPHA_ATTACK_2 = 0.3f;
float SConsts::TR_GUNPLANE_ALPHA_GO = 0.005f;
float SConsts::TR_GUNPLANE_ALPHA_KILL = 1.0f;
float SConsts::TR_GUNPLANE_ALPHA_PRICE = 1.0f;
float SConsts::TR_GUNPLANE_LIMIT_TIME = 1000.0f;

float SConsts::MAX_FIRE_RANGE_TO_SHOOT_BY_LINE = 1536.0f;

float SConsts::HP_PERCENT_TO_ESCAPE_FROM_BUILDING = 0.1f;

int SConsts::SHOW_ALL_TIME_COEFF = 5;

int SConsts::TIME_TO_ALLOW_KILL_CRUSHED_OBJ = 20000;

det_map<int, SConsts::SRevealInfo> SConsts::REVEAL_INFO;
det_map<NDb::EDesignUnitType, int, SEnumHash> SConsts::PRIORITIES;

float SConsts::REINFORCEMENT_GROUP_DISTANCE = 900.0f;
NTimer::STime SConsts::INFANTRY_FULL_HEAL_TIME = 2000;
//float SConsts::TILE_HEIGHT_DIFF_TO_LOCK = 10.0f;
NTimer::STime SConsts::STORAGE_CAPTURE_TIME = 5000;

//REGISTER_VAR_EX( "gfx_16bit_mode", NGlobal::VarBoolHandler, &bUse16BitMode, 0, true )
float fSpyGlassAngle = 45;
float fMinRotateAngle;
int nFighterPatrolTime;

namespace
{
bool bUsingPropertyConsts = false;
det_map<int, SConsts::SRevealInfo> legacyRevealInfo;

struct IPropertyBinding
{
	virtual ~IPropertyBinding() = default;
	virtual void SelectSource( const NDb::SAIPropertyConsts *pProperties, bool bWasUsingProperties ) = 0;
	virtual void CopyTo( NDb::SAIPropertyConsts *pProperties ) const = 0;
};

std::vector<std::unique_ptr<IPropertyBinding>> &PropertyBindings()
{
	static std::vector<std::unique_ptr<IPropertyBinding>> bindings;
	return bindings;
}

template <class TValue, class TProperty>
struct SPropertyBinding : IPropertyBinding
{
	TValue *pEffective;
	TValue legacyValue;
	TProperty NDb::SAIPropertyConsts::*pMember;
	NGlobal::VarHandler handler;

	SPropertyBinding( TValue *pValue, TProperty NDb::SAIPropertyConsts::*pField, NGlobal::VarHandler valueHandler )
		: pEffective( pValue )
		, legacyValue( *pValue )
		, pMember( pField )
		, handler( valueHandler )
	{
	}

	static void SetLegacyValue( const std::string &name, const NGlobal::CValue &value, void *pContext )
	{
		auto *pBinding = static_cast<SPropertyBinding *>( pContext );
		pBinding->handler( name, value, &pBinding->legacyValue );
		// Console/profile changes still belong to the fallback, never to an active data resource.
		if ( !bUsingPropertyConsts )
		{
			*pBinding->pEffective = pBinding->legacyValue;
		}
	}

	void SelectSource( const NDb::SAIPropertyConsts *pProperties, bool bWasUsingProperties ) override
	{
		if ( pProperties )
		{
			if ( !bWasUsingProperties )
			{
				// Preserve actual runtime values, including legacy Load() adjustments.
				legacyValue = *pEffective;
			}
			*pEffective = static_cast<TValue>( pProperties->*pMember );
		}
		else if ( bWasUsingProperties )
		{
			*pEffective = legacyValue;
		}
	}

	void CopyTo( NDb::SAIPropertyConsts *pProperties ) const override
	{
		pProperties->*pMember = static_cast<TProperty>( *pEffective );
	}
};

template <class TBound, class TValue, class TProperty>
void RegisterAIProperty( const char *name,
                         NGlobal::TVarBinding<TBound> binding,
                         TValue *pValue,
                         const NGlobal::CValue &registeredDefault,
                         TProperty NDb::SAIPropertyConsts::*pMember )
{
	static_assert( NGlobal::TVarBindingAccepts<TBound, TValue>::value, "AI property handler type mismatch" );
	using TBinding = SPropertyBinding<TValue, TProperty>;
	auto context = std::make_unique<TBinding>( pValue, pMember, NGlobal::GetVarHandlerFn( binding ) );
	NGlobal::RegisterVar( name, &TBinding::SetLegacyValue, context.get(), registeredDefault, STORAGE_NONE );
	PropertyBindings().push_back( std::move( context ) );
}
} // namespace

START_REGISTER( AILogicConsts )
RegisterAIProperty( "plane_min_height",
                    NGlobal::VarFloatHandler,
                    &SPlanesConsts::MIN_HEIGHT,
                    500.0f,
                    &NDb::SAIPropertyConsts::fPlaneMinHeight );
RegisterAIProperty( "plane_max_height",
                    NGlobal::VarFloatHandler,
                    &SPlanesConsts::MAX_HEIGHT,
                    1500.0f,
                    &NDb::SAIPropertyConsts::fPlaneMaxHeight );

RegisterAIProperty( "AI.Infantry.TimeOfLyingUnderFire",
                    NGlobal::VarIntHandler,
                    &SConsts::TIME_OF_LYING_UNDER_FIRE,
                    20000,
                    &NDb::SAIPropertyConsts::nInfantryTimeOfLyingUnderFire );
RegisterAIProperty( "AI.Infantry.LyingSoldierCover",
                    NGlobal::VarFloatHandler,
                    &SConsts::LYING_SOLDIER_COVER,
                    0.7f,
                    &NDb::SAIPropertyConsts::fInfantryLyingSoldierCover );
RegisterAIProperty( "AI.Infantry.RadiusOfFormation",
                    NGlobal::VarIntHandler,
                    &SConsts::RADIUS_OF_FORMATION,
                    320,
                    &NDb::SAIPropertyConsts::nInfantryRadiusOfFormation );
RegisterAIProperty( "AI.Infantry.LyingSpeedFactor",
                    NGlobal::VarFloatHandler,
                    &SConsts::LYING_SPEED_FACTOR,
                    0.5f,
                    &NDb::SAIPropertyConsts::fInfantryLyingSpeedFactor );
RegisterAIProperty( "AI.Infantry.SpyGlassRadius",
                    NGlobal::VarFloatHandler,
                    &SConsts::SPY_GLASS_RADIUS,
                    1920.f,
                    &NDb::SAIPropertyConsts::fInfantrySpyGlassRadius );

RegisterAIProperty( "AI.Engineers.MineVisRadius",
                    NGlobal::VarIntHandler,
                    &SConsts::MINE_VIS_RADIUS,
                    96,
                    &NDb::SAIPropertyConsts::nEngineersMineVisRadius );
RegisterAIProperty( "AI.Engineers.MineClearRadius",
                    NGlobal::VarIntHandler,
                    &SConsts::MINE_CLEAR_RADIUS,
                    224,
                    &NDb::SAIPropertyConsts::nEngineersMineClearRadius );
RegisterAIProperty( "AI.Engineers.EngineerMineCheckPeriod",
                    NGlobal::VarIntHandler,
                    &SConsts::ENGINEER_MINE_CHECK_PERIOD,
                    1000,
                    &NDb::SAIPropertyConsts::nEngineersEngineerMineCheckPeriod );
RegisterAIProperty( "AI.Engineers.TimeQuant",
                    NGlobal::VarIntHandler,
                    &SConsts::TIME_QUANT,
                    160,
                    &NDb::SAIPropertyConsts::nEngineersTimeQuant );
RegisterAIProperty( "AI.Engineers.RuPerQuant",
                    NGlobal::VarFloatHandler,
                    &SConsts::RU_PER_QUANT,
                    20,
                    &NDb::SAIPropertyConsts::fEngineersRuPerQuant );

RegisterAIProperty( "AI.Engineers.EngineerRuCarryWeight",
                    NGlobal::VarFloatHandler,
                    &SConsts::ENGINEER_RU_CARRY_WEIGHT,
                    500,
                    &NDb::SAIPropertyConsts::fEngineersEngineerRuCarryWeight );

RegisterAIProperty( "AI.Aviation.PlaneGuardStateRadius",
                    NGlobal::VarFloatHandler,
                    &SConsts::PLANE_GUARD_STATE_RADIUS,
                    3000.0f,
                    &NDb::SAIPropertyConsts::fAviationPlaneGuardStateRadius );

RegisterAIProperty( "AI.Aviation.FuelDecreaseEconomy",
                    NGlobal::VarFloatHandler,
                    &SConsts::PLANE_FUEL_DEC_ECONOMY,
                    0.5f,
                    &NDb::SAIPropertyConsts::fAviationFuelDecreaseEconomy );
RegisterAIProperty( "AI.Aviation.FuelDecrease",
                    NGlobal::VarFloatHandler,
                    &SConsts::PLANE_FUEL_DEC,
                    2.0f,
                    &NDb::SAIPropertyConsts::fAviationFuelDecrease );

RegisterAIProperty( "AI.Buildings.CureSpeedInBuilding",
                    NGlobal::VarFloatHandler,
                    &SConsts::CURE_SPEED_IN_BUILDING,
                    0.001f,
                    &NDb::SAIPropertyConsts::fBuildingsCureSpeedInBuilding );
RegisterAIProperty( "AI.Buildings.TimeOfBuildingAlarm",
                    NGlobal::VarIntHandler,
                    &SConsts::TIME_OF_BUILDING_ALARM,
                    8000,
                    &NDb::SAIPropertyConsts::nBuildingsTimeOfBuildingAlarm );
RegisterAIProperty( "AI.Buildings.CampingTime",
                    NGlobal::VarIntHandler,
                    &SConsts::CAMPING_TIME,
                    2000,
                    &NDb::SAIPropertyConsts::nBuildingsCampingTime );
RegisterAIProperty( "AI.Buildings.InsideObjWeaponFactor",
                    NGlobal::VarFloatHandler,
                    &SConsts::INSIDE_OBJ_WEAPON_FACTOR,
                    0.5f,
                    &NDb::SAIPropertyConsts::fBuildingsInsideObjWeaponFactor );
RegisterAIProperty( "AI.Buildings.InsideObjCombatPeriod",
                    NGlobal::VarIntHandler,
                    &SConsts::INSIDE_OBJ_COMBAT_PERIOD,
                    800,
                    &NDb::SAIPropertyConsts::nBuildingsInsideObjCombatPeriod );

RegisterAIProperty( "AI.Artillery.ThresholdInstallTime",
                    NGlobal::VarIntHandler,
                    &SConsts::THRESHOLD_INSTALL_TIME,
                    2000,
                    &NDb::SAIPropertyConsts::nArtilleryThresholdInstallTime );
RegisterAIProperty( "AI.Artillery.ShootsToRange",
                    NGlobal::VarIntHandler,
                    &SConsts::SHOOTS_TO_RANGE,
                    4,
                    &NDb::SAIPropertyConsts::nArtilleryShootsToRange );
RegisterAIProperty( "AI.Artillery.RangedDispersionRadiusBonus",
                    NGlobal::VarFloatHandler,
                    &SConsts::RANDGED_DISPERSION_RADIUS_BONUS,
                    0.5f,
                    &NDb::SAIPropertyConsts::fArtilleryRangedDispersionRadiusBonus );
// RANGED_AREA_RADIUS is a float, and VarIntHandler writes an int through the
// void * context, so consts.cfg's "setvar AI.Artillery.RangedAreaRadius = 716"
// stored the bit pattern of 716 and the radius read back as 1.0e-42. Squared,
// that is exactly zero, so the zeroing radius and the circle at
// ArtilleryStates.cpp:1156 and :1337 were both zero on every run.
RegisterAIProperty( "AI.Artillery.RangedAreaRadius",
                    NGlobal::VarFloatHandler,
                    &SConsts::RANGED_AREA_RADIUS,
                    160.0f,
                    &NDb::SAIPropertyConsts::fArtilleryRangedAreaRadius );
RegisterAIProperty( "AI.Artillery.MaxAntiArtilleryRadius",
                    NGlobal::VarFloatHandler,
                    &SConsts::MAX_ANTI_ARTILLERY_RADIUS,
                    320,
                    &NDb::SAIPropertyConsts::fArtilleryMaxAntiArtilleryRadius );
RegisterAIProperty( "AI.Artillery.MinAntiArtilleryRadius",
                    NGlobal::VarFloatHandler,
                    &SConsts::MIN_ANTI_ARTILLERY_RADIUS,
                    32,
                    &NDb::SAIPropertyConsts::fArtilleryMinAntiArtilleryRadius );
RegisterAIProperty( "AI.Artillery.ShotsToMinimizeLocationRadius",
                    NGlobal::VarIntHandler,
                    &SConsts::SHOTS_TO_MINIMIZE_LOCATION_RADIUS,
                    5,
                    &NDb::SAIPropertyConsts::nArtilleryShotsToMinimizeLocationRadius );
RegisterAIProperty( "AI.Artillery.RevealCirclePeriod",
                    NGlobal::VarIntHandler,
                    &SConsts::REVEAL_CIRCLE_PERIOD,
                    2000,
                    &NDb::SAIPropertyConsts::nArtilleryRevealCirclePeriod );
RegisterAIProperty( "AI.Artillery.RadiusToStartAntiartilleryFire",
                    NGlobal::VarFloatHandler,
                    &SConsts::RADIUS_TO_START_ANTIARTILLERY_FIRE,
                    320.0f,
                    &NDb::SAIPropertyConsts::fArtilleryRadiusToStartAntiartilleryFire );
// Same fault, same cause: a float written through VarIntHandler. With the
// radius reading back as 2.2e-43, AntiArtillery.cpp:123 compared the distance
// against zero, so a gun that had moved at all forgot where it last heard
// enemy fire instead of forgetting only past 160.
RegisterAIProperty( "AI.Artillery.RelocationRadius",
                    NGlobal::VarFloatHandler,
                    &SConsts::RELOCATION_RADIUS,
                    160.0f,
                    &NDb::SAIPropertyConsts::fArtilleryRelocationRadius );
RegisterAIProperty( "AI.Artillery.AudibilityTime",
                    NGlobal::VarIntHandler,
                    &SConsts::AUDIBILITY_TIME,
                    20000,
                    &NDb::SAIPropertyConsts::nArtilleryAudibilityTime );

RegisterAIProperty( "AI.Artillery.CheckFreePeriod",
                    NGlobal::VarFloatHandler,
                    &SConsts::ARTILLERY_CHECKFREE_PERIOD,
                    20000,
                    &NDb::SAIPropertyConsts::fArtilleryCheckFreePeriod );
RegisterAIProperty( "AI.Formation.CheckFreeArtPeriod",
                    NGlobal::VarFloatHandler,
                    &SConsts::FORMATION_CHECKFREEART_PERIOD,
                    30000,
                    &NDb::SAIPropertyConsts::fFormationCheckFreeArtPeriod );

RegisterAIProperty( "AI.Paratroopers.ParatrooperFallSpeed",
                    NGlobal::VarFloatHandler,
                    &SConsts::PARATROOPER_FALL_SPEED,
                    0.05f,
                    &NDb::SAIPropertyConsts::fParatroopersParatrooperFallSpeed );
RegisterAIProperty( "AI.Paratroopers.ParadropSpred",
                    NGlobal::VarIntHandler,
                    &SConsts::PARADROP_SPRED,
                    4,
                    &NDb::SAIPropertyConsts::nParatroopersParadropSpred );
RegisterAIProperty( "AI.Paratroopers.PlaneParadropInterval",
                    NGlobal::VarFloatHandler,
                    &SConsts::PLANE_PARADROP_INTERVAL,
                    20.0f,
                    &NDb::SAIPropertyConsts::fParatroopersPlaneParadropInterval );
RegisterAIProperty( "AI.Paratroopers.PlaneParadropIntervalPerpMin",
                    NGlobal::VarFloatHandler,
                    &SConsts::PLANE_PARADROP_INTERVAL_PERP_MIN,
                    200.0f,
                    &NDb::SAIPropertyConsts::fParatroopersPlaneParadropIntervalPerpMin );
RegisterAIProperty( "AI.Paratroopers.PlaneParadropIntervalPerpMax",
                    NGlobal::VarFloatHandler,
                    &SConsts::PLANE_PARADROP_INTERVAL_PERP_MAX,
                    500.0f,
                    &NDb::SAIPropertyConsts::fParatroopersPlaneParadropIntervalPerpMax );

RegisterAIProperty( "AI.Paratroopers.ParatrooperGroundScanPeriod",
                    NGlobal::VarIntHandler,
                    &SConsts::PARATROOPER_GROUND_SCAN_PERIOD,
                    200,
                    &NDb::SAIPropertyConsts::nParatroopersParatrooperGroundScanPeriod );

RegisterAIProperty( "AI.TransportAndResupply.ResupplyRadius",
                    NGlobal::VarIntHandler,
                    &SConsts::RESUPPLY_RADIUS,
                    1000,
                    &NDb::SAIPropertyConsts::nTransportAndResupplyResupplyRadius );
RegisterAIProperty( "AI.Morale.ResupplyRadius",
                    NGlobal::VarIntHandler,
                    &SConsts::RESUPPLY_RADIUS_MORALE,
                    300,
                    &NDb::SAIPropertyConsts::nMoraleResupplyRadius );

RegisterAIProperty( "AI.TransportAndResupply.TransportRuCapacity",
                    NGlobal::VarFloatHandler,
                    &SConsts::TRANSPORT_RU_CAPACITY,
                    10000.0f,
                    &NDb::SAIPropertyConsts::fTransportAndResupplyTransportRuCapacity );
RegisterAIProperty( "AI.TransportAndResupply.TransportLoadRuDistance",
                    NGlobal::VarFloatHandler,
                    &SConsts::TRANSPORT_LOAD_RU_DISTANCE,
                    200.0f,
                    &NDb::SAIPropertyConsts::fTransportAndResupplyTransportLoadRuDistance );
RegisterAIProperty( "AI.TransportAndResupply.LandDistance",
                    NGlobal::VarFloatHandler,
                    &SConsts::GOOD_LAND_DIST,
                    50.0f,
                    &NDb::SAIPropertyConsts::fTransportAndResupplyLandDistance );

RegisterAIProperty( "AI.Common.NumToScanInSegm",
                    NGlobal::VarIntHandler,
                    &SConsts::NUM_TO_SCAN_IN_SEGM,
                    50,
                    &NDb::SAIPropertyConsts::nCommonNumToScanInSegm );
RegisterAIProperty( "AI.Common.BehUpdateDuration",
                    NGlobal::VarIntHandler,
                    &SConsts::BEH_UPDATE_DURATION,
                    2000,
                    &NDb::SAIPropertyConsts::nCommonBehUpdateDuration );
RegisterAIProperty( "AI.Common.SoldierBehUpdateDuration",
                    NGlobal::VarIntHandler,
                    &SConsts::SOLDIER_BEH_UPDATE_DURATION,
                    3000,
                    &NDb::SAIPropertyConsts::nCommonSoldierBehUpdateDuration );
RegisterAIProperty( "AI.Common.AABehUpdateDuration",
                    NGlobal::VarIntHandler,
                    &SConsts::AA_BEH_UPDATE_DURATION,
                    200,
                    &NDb::SAIPropertyConsts::nCommonAABehUpdateDuration );
RegisterAIProperty( "AI.Common.DeadSeeTime",
                    NGlobal::VarIntHandler,
                    &SConsts::DEAD_SEE_TIME,
                    2000,
                    &NDb::SAIPropertyConsts::nCommonDeadSeeTime );
RegisterAIProperty( "AI.Common.TimeToReturnGun",
                    NGlobal::VarIntHandler,
                    &SConsts::TIME_TO_RETURN_GUN,
                    2000,
                    &NDb::SAIPropertyConsts::nCommonTimeToReturnGun );
RegisterAIProperty( "AI.Common.TimeBeforeCamouflage",
                    NGlobal::VarIntHandler,
                    &SConsts::TIME_BEFORE_CAMOUFLAGE,
                    2000,
                    &NDb::SAIPropertyConsts::nCommonTimeBeforeCamouflage );
RegisterAIProperty( "AI.Common.RadiusOfHitNotify",
                    NGlobal::VarIntHandler,
                    &SConsts::RADIUS_OF_HIT_NOTIFY,
                    160,
                    &NDb::SAIPropertyConsts::nCommonRadiusOfHitNotify );
RegisterAIProperty( "AI.Common.TimeOfHitNotify",
                    NGlobal::VarIntHandler,
                    &SConsts::TIME_OF_HIT_NOTIFY,
                    1000,
                    &NDb::SAIPropertyConsts::nCommonTimeOfHitNotify );
RegisterAIProperty( "AI.Common.CallForHelpRadius",
                    NGlobal::VarIntHandler,
                    &SConsts::CALL_FOR_HELP_RADIUS,
                    640,
                    &NDb::SAIPropertyConsts::nCommonCallForHelpRadius );
RegisterAIProperty( "AI.Common.AICallForHelpRadius",
                    NGlobal::VarIntHandler,
                    &SConsts::AI_CALL_FOR_HELP_RADIUS,
                    640,
                    &NDb::SAIPropertyConsts::nCommonAICallForHelpRadius );
RegisterAIProperty( "AI.Common.TimeToDisappear",
                    NGlobal::VarIntHandler,
                    &SConsts::TIME_TO_DISAPPEAR,
                    0,
                    &NDb::SAIPropertyConsts::nCommonTimeToDisappear );
// Same fault again. consts.cfg sets this to 960 and the script sets it to 960
// a second time, both through VarIntHandler, so the float held 1.3e-42 and the
// test at CommonUnit.cpp:104, "is this point inside my guard radius", was
// distance squared below zero: false for every point. A resting unit had no
// guard radius at all.
RegisterAIProperty( "AI.Common.GuardStateRadius",
                    NGlobal::VarFloatHandler,
                    &SConsts::GUARD_STATE_RADIUS,
                    640.0f,
                    &NDb::SAIPropertyConsts::fCommonGuardStateRadius );
RegisterAIProperty( "AI.Common.GoodAttackProbability",
                    NGlobal::VarFloatHandler,
                    &SConsts::GOOD_ATTACK_RPOBABILITY,
                    0.6f,
                    &NDb::SAIPropertyConsts::fCommonGoodAttackProbability );
RegisterAIProperty( "AI.Common.AreaDamageCoeff",
                    NGlobal::VarFloatHandler,
                    &SConsts::AREA_DAMAGE_COEFF,
                    0.2f,
                    &NDb::SAIPropertyConsts::fCommonAreaDamageCoeff );
RegisterAIProperty( "AI.Common.TankTrackHitPoints",
                    NGlobal::VarFloatHandler,
                    &SConsts::TANK_TRACK_HIT_POINTS,
                    0.1f,
                    &NDb::SAIPropertyConsts::fCommonTankTrackHitPoints );
RegisterAIProperty( "AI.Common.TrajectoryLineRatio",
                    NGlobal::VarFloatHandler,
                    &SConsts::TRAJECTORY_LOW_LINE_RATIO,
                    0.7f,
                    &NDb::SAIPropertyConsts::fCommonTrajectoryLineRatio );
RegisterAIProperty( "AI.Common.TrajectoryBombG",
                    NGlobal::VarFloatHandler,
                    &SConsts::TRAJECTORY_BOMB_G,
                    0.7f,
                    &NDb::SAIPropertyConsts::fCommonTrajectoryBombG );

RegisterAIProperty( "AI.Aviation.HeavyFormationDistance",
                    NGlobal::VarFloatHandler,
                    &SConsts::PLANES_HEAVY_FORMATION_SIZE,
                    2.0f,
                    &NDb::SAIPropertyConsts::fAviationHeavyFormationDistance );
RegisterAIProperty( "AI.Aviation.LightFormationDistance",
                    NGlobal::VarFloatHandler,
                    &SConsts::PLANES_SMALL_FORMATION_SIZE,
                    1.5f,
                    &NDb::SAIPropertyConsts::fAviationLightFormationDistance );
RegisterAIProperty( "AI.Aviation.StartRandom",
                    NGlobal::VarFloatHandler,
                    &SConsts::PLANES_START_RANDOM,
                    3.1f,
                    &NDb::SAIPropertyConsts::fAviationStartRandom );

RegisterAIProperty( "AI.Infantry.SniperCamouflageDecreasePerShoot",
                    NGlobal::VarFloatHandler,
                    &SConsts::SNIPER_CAMOUFLAGE_DECREASE_PER_SHOOT,
                    0.1f,
                    &NDb::SAIPropertyConsts::fInfantrySniperCamouflageDecreasePerShoot );
RegisterAIProperty( "AI.Infantry.SniperCamouflageIncrease",
                    NGlobal::VarFloatHandler,
                    &SConsts::SNIPER_CAMOUFLAGE_INCREASE,
                    0.014f,
                    &NDb::SAIPropertyConsts::fInfantrySniperCamouflageIncrease );
RegisterAIProperty( "AI.Common.AmbushBeginAttackCriteria",
                    NGlobal::VarFloatHandler,
                    &SConsts::AMBUSH_ATTACK_BEGIN_CIRTERIA,
                    0.5f,
                    &NDb::SAIPropertyConsts::fCommonAmbushBeginAttackCriteria );
RegisterAIProperty( "AI.Artillery.ArtilleryRevealCoefficient",
                    NGlobal::VarFloatHandler,
                    &SConsts::ARTILLERY_REVEAL_COEEFICIENT,
                    200.0f,
                    &NDb::SAIPropertyConsts::fArtilleryArtilleryRevealCoefficient );

RegisterAIProperty( "AI.Artillery.DispersionRatio.LineMin",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[0][0],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioLineMin );
RegisterAIProperty( "AI.Artillery.DispersionRatio.LineMax",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[0][1],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioLineMax );

RegisterAIProperty( "AI.Artillery.DispersionRatio.HowitserMin",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[1][0],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioHowitserMin );
RegisterAIProperty( "AI.Artillery.DispersionRatio.HowitserMax",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[1][1],
                    2.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioHowitserMax );

RegisterAIProperty( "AI.Artillery.DispersionRatio.BombMin",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[2][0],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioBombMin );
RegisterAIProperty( "AI.Artillery.DispersionRatio.BombMax",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[2][1],
                    3.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioBombMax );

RegisterAIProperty( "AI.Artillery.DispersionRatio.CannonMin",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[3][0],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioCannonMin );
RegisterAIProperty( "AI.Artillery.DispersionRatio.CannonMax",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[3][1],
                    4.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioCannonMax );

RegisterAIProperty( "AI.Artillery.DispersionRatio.RocketMin",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[4][0],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioRocketMin );
RegisterAIProperty( "AI.Artillery.DispersionRatio.RocketMax",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[4][1],
                    4.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioRocketMax );

RegisterAIProperty( "AI.Artillery.DispersionRatio.GrenadeMin",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[5][0],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioGrenadeMin );
RegisterAIProperty( "AI.Artillery.DispersionRatio.GrenadeMax",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[5][1],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioGrenadeMax );

RegisterAIProperty( "AI.Artillery.DispersionRatio.TorpedoMin",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[6][0],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioTorpedoMin );
RegisterAIProperty( "AI.Artillery.DispersionRatio.TorpedoMax",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[6][1],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioTorpedoMax );

RegisterAIProperty( "AI.Artillery.DispersionRatio.AARocketMin",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[7][0],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioAARocketMin );
RegisterAIProperty( "AI.Artillery.DispersionRatio.AARocketMax",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[7][1],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioAARocketMax );

RegisterAIProperty( "AI.Artillery.DispersionRatio.FlameMin",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[8][0],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioFlameMin );
RegisterAIProperty( "AI.Artillery.DispersionRatio.FlameMax",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[8][1],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioFlameMax );
RegisterAIProperty( "AI.Artillery.DispersionRatio.ATGMLineMin",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[9][0],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioATGMLineMin );
RegisterAIProperty( "AI.Artillery.DispersionRatio.ATGMLineMax",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[9][1],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioATGMLineMax );
RegisterAIProperty( "AI.Artillery.DispersionRatio.ATGMTopAttackMin",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[10][0],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioATGMTopAttackMin );
RegisterAIProperty( "AI.Artillery.DispersionRatio.ATGMTopAttackMax",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[10][1],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioATGMTopAttackMax );
RegisterAIProperty( "AI.Artillery.DispersionRatio.SAMMin",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[11][0],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioSAMMin );
RegisterAIProperty( "AI.Artillery.DispersionRatio.SAMMax",
                    NGlobal::VarFloatHandler,
                    &SConsts::dispersionRatio[11][1],
                    1.0f,
                    &NDb::SAIPropertyConsts::fArtilleryDispersionRatioSAMMax );

RegisterAIProperty( "AI.Common.CoeffForRandomDelay",
                    NGlobal::VarFloatHandler,
                    &SConsts::COEFF_FOR_RANDOM_DELAY,
                    1.2f,
                    &NDb::SAIPropertyConsts::fCommonCoeffForRandomDelay );
RegisterAIProperty( "AI.Common.HeightForVisRadiusInc",
                    NGlobal::VarFloatHandler,
                    &SConsts::HEIGHT_FOR_VIS_RADIUS_INC,
                    10.0f,
                    &NDb::SAIPropertyConsts::fCommonHeightForVisRadiusInc );
RegisterAIProperty( "AI.Buildings.BurningSpeed",
                    NGlobal::VarFloatHandler,
                    &SConsts::BURNING_SPEED,
                    0.002f,
                    &NDb::SAIPropertyConsts::fBuildingsBurningSpeed );

RegisterAIProperty( "AI.Follow.StopRadius",
                    NGlobal::VarFloatHandler,
                    &SConsts::FOLLOW_STOP_RADIUS,
                    288.0f,
                    &NDb::SAIPropertyConsts::fFollowStopRadius );
RegisterAIProperty( "AI.Follow.EqualizeSpeedRadius",
                    NGlobal::VarFloatHandler,
                    &SConsts::FOLLOW_EQUALIZE_SPEED_RADIUS,
                    384.0f,
                    &NDb::SAIPropertyConsts::fFollowEqualizeSpeedRadius );
RegisterAIProperty( "AI.Follow.GoRadius",
                    NGlobal::VarFloatHandler,
                    &SConsts::FOLLOW_GO_RADIUS,
                    352.0f,
                    &NDb::SAIPropertyConsts::fFollowGoRadius );
RegisterAIProperty( "AI.Follow.WaitRadius",
                    NGlobal::VarFloatHandler,
                    &SConsts::FOLLOW_WAIT_RADIUS,
                    786.0f,
                    &NDb::SAIPropertyConsts::fFollowWaitRadius );

RegisterAIProperty( "AI.Common.FatalityProbability",
                    NGlobal::VarFloatHandler,
                    &SConsts::FATALITY_PROBABILITY,
                    0.1f,
                    &NDb::SAIPropertyConsts::fCommonFatalityProbability );
RegisterAIProperty( "AI.Common.DamageForMassiveDamageFatality",
                    NGlobal::VarFloatHandler,
                    &SConsts::DAMAGE_FOR_MASSIVE_DAMAGE_FATALITY,
                    0.7f,
                    &NDb::SAIPropertyConsts::fCommonDamageForMassiveDamageFatality );
RegisterAIProperty( "AI.Common.MassiveDamageFatalityProbability",
                    NGlobal::VarFloatHandler,
                    &SConsts::MASSIVE_DAMAGE_FATALITY_PROBABILITY,
                    0.8f,
                    &NDb::SAIPropertyConsts::fCommonMassiveDamageFatalityProbability );

RegisterAIProperty( "AI.Aviation.PlanesBombHeight",
                    NGlobal::VarFloatHandler,
                    &SConsts::BOMB_START_HEIGHT,
                    0.5f,
                    &NDb::SAIPropertyConsts::fAviationPlanesBombHeight );

RegisterAIProperty( "AI.Infantry.StandLieRandomDelay",
                    NGlobal::VarFloatHandler,
                    &SConsts::STAND_LIE_RANDOM_DELAY,
                    500.0f,
                    &NDb::SAIPropertyConsts::fInfantryStandLieRandomDelay );

RegisterAIProperty( "AI.TransportAndResupply.ResupplyOffset",
                    NGlobal::VarFloatHandler,
                    &SConsts::TRANSPORT_RESUPPLY_OFFSET,
                    100.0f,
                    &NDb::SAIPropertyConsts::fTransportAndResupplyResupplyOffset );
RegisterAIProperty( "AI.TransportAndResupply.ResupplyBalanceCoeff",
                    NGlobal::VarFloatHandler,
                    &SConsts::HP_BALANCE_COEFF,
                    500.0f,
                    &NDb::SAIPropertyConsts::fTransportAndResupplyResupplyBalanceCoeff );
RegisterAIProperty( "AI.Infantry.SquadMemberLeaveInterval",
                    NGlobal::VarIntHandler,
                    &SConsts::SQUAD_MEMBER_LEAVE_INTERVAL,
                    200,
                    &NDb::SAIPropertyConsts::nInfantrySquadMemberLeaveInterval );
RegisterAIProperty( "AI.TransportAndResupply.SoldierRUPrice",
                    NGlobal::VarFloatHandler,
                    &SConsts::SOLDIER_RU_PRICE,
                    50.0f,
                    &NDb::SAIPropertyConsts::fTransportAndResupplySoldierRUPrice );
RegisterAIProperty( "AI.Common.HpPercentageToWaitMedic",
                    NGlobal::VarFloatHandler,
                    &SConsts::LOW_HP_PERCENTAGE,
                    0.5f,
                    &NDb::SAIPropertyConsts::fCommonHpPercentageToWaitMedic );

RegisterAIProperty( "AI.CombatSituation.Damage",
                    NGlobal::VarFloatHandler,
                    &SConsts::DIRECT_HIT_DAMAGE_COMBAT_SITUATION,
                    100.0f,
                    &NDb::SAIPropertyConsts::fCombatSituationDamage );
RegisterAIProperty( "AI.CombatSituation.TimeDamage",
                    NGlobal::VarIntHandler,
                    &SConsts::DIRECT_HIT_TIME_COMBAT_SITUATION,
                    1000,
                    &NDb::SAIPropertyConsts::nCombatSituationTimeDamage );
RegisterAIProperty( "AI.CombatSituation.MovingEnemyMechNumber",
                    NGlobal::VarIntHandler,
                    &SConsts::NUMBER_ENEMY_MECH_MOVING_TO_COMBAT_SITUATION,
                    3,
                    &NDb::SAIPropertyConsts::nCombatSituationMovingEnemyMechNumber );
RegisterAIProperty( "AI.CombatSituation.MovingEnemyInfantryNumber",
                    NGlobal::VarIntHandler,
                    &SConsts::NUMBER_ENEMY_INFANTRY_MOVING_TO_COMBAT_SITUATION,
                    20,
                    &NDb::SAIPropertyConsts::nCombatSituationMovingEnemyInfantryNumber );
RegisterAIProperty( "AI.TransportAndResupply.MainStorageHealingSpeed",
                    NGlobal::VarFloatHandler,
                    &SConsts::MAIN_STORAGE_HEALING_SPEED,
                    10.0f,
                    &NDb::SAIPropertyConsts::fTransportAndResupplyMainStorageHealingSpeed );

RegisterAIProperty( "AI.TransportAndResupply.ResupplyMaxPathLenght",
                    NGlobal::VarIntHandler,
                    &SConsts::RESUPPLY_MAX_PATH,
                    60,
                    &NDb::SAIPropertyConsts::nTransportAndResupplyResupplyMaxPathLenght );
RegisterAIProperty( "AI.TransportAndResupply.TakeStorageOwnershipRadius",
                    NGlobal::VarFloatHandler,
                    &SConsts::RADIUS_TO_TAKE_STORAGE_OWNERSHIP,
                    100.0f,
                    &NDb::SAIPropertyConsts::fTransportAndResupplyTakeStorageOwnershipRadius );

RegisterAIProperty( "AI.Common.TankPitCover",
                    NGlobal::VarFloatHandler,
                    &SConsts::TANKPIT_COVER,
                    0.5f,
                    &NDb::SAIPropertyConsts::fCommonTankPitCover );
RegisterAIProperty( "AI.Common.FenceSegmentRuPrice",
                    NGlobal::VarFloatHandler,
                    &SConsts::FENCE_SEGMENT_RU_PRICE,
                    100.0f,
                    &NDb::SAIPropertyConsts::fCommonFenceSegmentRuPrice );
RegisterAIProperty( "AI.Common.TrenchSegmentRuPrice",
                    NGlobal::VarFloatHandler,
                    &SConsts::ENTRENCHMENT_SEGMENT_RU_PRICE,
                    100.0f,
                    &NDb::SAIPropertyConsts::fCommonTrenchSegmentRuPrice );
RegisterAIProperty( "AI.Engineers.MineAPersRuPrice",
                    NGlobal::VarFloatHandler,
                    &SConsts::MINE_RU_PRICE[0],
                    1000,
                    &NDb::SAIPropertyConsts::fEngineersMineAPersRuPrice );
RegisterAIProperty( "AI.Engineers.MineATankRuPrice",
                    NGlobal::VarFloatHandler,
                    &SConsts::MINE_RU_PRICE[1],
                    1000,
                    &NDb::SAIPropertyConsts::fEngineersMineATankRuPrice );
RegisterAIProperty( "AI.Engineers.AntitankRuPrice",
                    NGlobal::VarFloatHandler,
                    &SConsts::ANTITANK_RU_PRICE,
                    1000,
                    &NDb::SAIPropertyConsts::fEngineersAntitankRuPrice );
RegisterAIProperty( "AI.Engineers.ComplexObstacleAntitankPeriod",
                    NGlobal::VarIntHandler,
                    &SConsts::COMPLEX_OBSTACLE_ANTITANK_PERIOD,
                    2,
                    &NDb::SAIPropertyConsts::nEngineersComplexObstacleAntitankPeriod );

RegisterAIProperty( "AI.TransportAndResupply.MedicalTruckHealRadius",
                    NGlobal::VarIntHandler,
                    &SConsts::MED_TRUCK_HEAL_RADIUS,
                    1000,
                    &NDb::SAIPropertyConsts::nTransportAndResupplyMedicalTruckHealRadius );
// MED_TRUCK_HEAL_PER_UPDATEDURATION is a float (AIInternalConsts.h:351) and
// had the int handler. Latent rather than live: no shipped config sets this
// one, so it keeps its initialiser, and only a console setvar or a new config
// line would have turned the healing rate into a denormal.
RegisterAIProperty( "AI.TransportAndResupply.MedicalTruckHealPerUpdateDuration",
                    NGlobal::VarFloatHandler,
                    &SConsts::MED_TRUCK_HEAL_PER_UPDATEDURATION,
                    1.0f,
                    &NDb::SAIPropertyConsts::fTransportAndResupplyMedicalTruckHealPerUpdateDuration );

RegisterAIProperty( "AI.Common.UnitEntrenchTime",
                    NGlobal::VarIntHandler,
                    &SConsts::ENTRENCH_SELF_TIME,
                    1000,
                    &NDb::SAIPropertyConsts::nCommonUnitEntrenchTime );

RegisterAIProperty(
		"AI.Flags.Radius", NGlobal::VarFloatHandler, &SConsts::FLAG_RADIUS, 320.0f, &NDb::SAIPropertyConsts::fFlagsRadius );
RegisterAIProperty( "AI.Flags.PointsSpeed",
                    NGlobal::VarFloatHandler,
                    &SConsts::FLAG_POINTS_SPEED,
                    10.0f,
                    &NDb::SAIPropertyConsts::fFlagsPointsSpeed );
RegisterAIProperty( "AI.Flags.PlayerPointsSpeed",
                    NGlobal::VarFloatHandler,
                    &SConsts::PLAYER_POINTS_SPEED,
                    10.0f,
                    &NDb::SAIPropertyConsts::fFlagsPlayerPointsSpeed );

RegisterAIProperty( "AI.Flags.PointsToReinforcement",
                    NGlobal::VarFloatHandler,
                    &SConsts::FLAG_POINTS_TO_REINFORCEMENT,
                    30.0f,
                    &NDb::SAIPropertyConsts::fFlagsPointsToReinforcement );
RegisterAIProperty( "AI.Flags.TimeToCapture",
                    NGlobal::VarFloatHandler,
                    &SConsts::FLAG_TIME_TO_CAPTURE,
                    5000.0f,
                    &NDb::SAIPropertyConsts::fFlagsTimeToCapture );

RegisterAIProperty( "AI.Common.ArmorForAreaDamage",
                    NGlobal::VarIntHandler,
                    &SConsts::ARMOR_FOR_AREA_DAMAGE,
                    10,
                    &NDb::SAIPropertyConsts::nCommonArmorForAreaDamage );

RegisterAIProperty( "AI.Buildings.DefaultFireplaceCoverage",
                    NGlobal::VarFloatHandler,
                    &SConsts::BUILDING_FIREPLACE_DEFAULT_COVER,
                    0.5f,
                    &NDb::SAIPropertyConsts::fBuildingsDefaultFireplaceCoverage );

RegisterAIProperty( "AI.Aviation.DiveBeforeExplodeTime",
                    NGlobal::VarIntHandler,
                    &SConsts::DIVE_BEFORE_EXPLODE_TIME,
                    5000,
                    &NDb::SAIPropertyConsts::nAviationDiveBeforeExplodeTime );
RegisterAIProperty( "AI.Aviation.DiveAfterExplodeTime",
                    NGlobal::VarIntHandler,
                    &SConsts::DIVE_AFTER_EXPLODE_TIME,
                    150,
                    &NDb::SAIPropertyConsts::nAviationDiveAfterExplodeTime );

RegisterAIProperty(
		"AI.Weather.Time", NGlobal::VarIntHandler, &SConsts::WEATHER_TIME, 20000, &NDb::SAIPropertyConsts::nWeatherTime );
RegisterAIProperty( "AI.Weather.TimeRandom",
                    NGlobal::VarIntHandler,
                    &SConsts::WEATHER_TIME_RANDOM,
                    20000,
                    &NDb::SAIPropertyConsts::nWeatherTimeRandom );

RegisterAIProperty( "AI.Weather.Period",
                    NGlobal::VarIntHandler,
                    &SConsts::WEATHER_TURN_PERIOD,
                    20000,
                    &NDb::SAIPropertyConsts::nWeatherPeriod );
RegisterAIProperty( "AI.Weather.PeriodRandom",
                    NGlobal::VarIntHandler,
                    &SConsts::WEATHER_TURN_PERIOD_RANDOM,
                    20000,
                    &NDb::SAIPropertyConsts::nWeatherPeriodRandom );

RegisterAIProperty( "AI.Weather.FireRangeCoefficient",
                    NGlobal::VarFloatHandler,
                    &SConsts::BAD_WEATHER_FIRE_RANGE_COEFFICIENT,
                    0.5f,
                    &NDb::SAIPropertyConsts::fWeatherFireRangeCoefficient );
// The mismatch the other way round: TIME_TO_WEATHER_FADE_OFF is an int
// (AIInternalConsts.h:393) and had the float handler, which stores a float bit
// pattern, so setting this to 3000 would have read back as about 1.16 billion.
// The registered default was 0.5f, copied from the float registration above it
// and meaningless for a count of weather updates; it is now the 5 this
// variable is actually initialised to at AIConsts.cpp:248. Nothing was writing
// it, since RegisterVar records the default and never runs the handler, so
// only getvar and a reset ever saw the old one.
RegisterAIProperty( "AI.Weather.TimeToFadeOff",
                    NGlobal::VarIntHandler,
                    &SConsts::TIME_TO_WEATHER_FADE_OFF,
                    5,
                    &NDb::SAIPropertyConsts::nWeatherTimeToFadeOff );

RegisterAIProperty( "AI.AntiAviationArtillery.AimIterations",
                    NGlobal::VarIntHandler,
                    &SConsts::AA_AIM_ITERATIONS,
                    3,
                    &NDb::SAIPropertyConsts::nAntiAviationArtilleryAimIterations );

RegisterAIProperty( "AI.Morale.CoeffToLowMoraleWithoutOfficer",
                    NGlobal::VarFloatHandler,
                    &SConsts::COEFF_TO_LOW_MORALE_WITHOUT_OFFICER,
                    2.0f,
                    &NDb::SAIPropertyConsts::fMoraleCoeffToLowMoraleWithoutOfficer );

RegisterAIProperty( "AI.Infantry.MaxDistanceToThrowGrenade",
                    NGlobal::VarFloatHandler,
                    &SConsts::MAX_DISTANCE_TO_THROW_GRENADE,
                    320.0f,
                    &NDb::SAIPropertyConsts::fInfantryMaxDistanceToThrowGrenade );

RegisterAIProperty( "AI.Morale.DispersionCoeff",
                    NGlobal::VarFloatHandler,
                    &SConsts::MORALE_DISPERSION_COEFF,
                    1.0f,
                    &NDb::SAIPropertyConsts::fMoraleDispersionCoeff );
RegisterAIProperty( "AI.Morale.RelaxCoeff",
                    NGlobal::VarFloatHandler,
                    &SConsts::MORALE_RELAX_COEFF,
                    1.0f,
                    &NDb::SAIPropertyConsts::fMoraleRelaxCoeff );
RegisterAIProperty( "AI.Morale.AimingCoeff",
                    NGlobal::VarFloatHandler,
                    &SConsts::MORALE_AIMING_COEFF,
                    1.0f,
                    &NDb::SAIPropertyConsts::fMoraleAimingCoeff );

RegisterAIProperty( "AI.TargetResolution.Gunplane.DistanceToCenter",
                    NGlobal::VarFloatHandler,
                    &SConsts::TR_DISTANCE_TO_CENTER_FACTOR,
                    10.0f,
                    &NDb::SAIPropertyConsts::fTargetResolutionGunplaneDistanceToCenter );
RegisterAIProperty( "AI.TargetResolution.Gunplane.AlphaAttack1",
                    NGlobal::VarFloatHandler,
                    &SConsts::TR_GUNPLANE_ALPHA_ATTACK_1,
                    1.0f,
                    &NDb::SAIPropertyConsts::fTargetResolutionGunplaneAlphaAttack1 );
RegisterAIProperty( "AI.TargetResolution.Gunplane.AlphaAttack2",
                    NGlobal::VarFloatHandler,
                    &SConsts::TR_GUNPLANE_ALPHA_ATTACK_2,
                    3.0f,
                    &NDb::SAIPropertyConsts::fTargetResolutionGunplaneAlphaAttack2 );
RegisterAIProperty( "AI.TargetResolution.Gunplane.AlphaGo",
                    NGlobal::VarFloatHandler,
                    &SConsts::TR_GUNPLANE_ALPHA_GO,
                    0.005f,
                    &NDb::SAIPropertyConsts::fTargetResolutionGunplaneAlphaGo );
RegisterAIProperty( "AI.TargetResolution.Gunplane.AlphaKill",
                    NGlobal::VarFloatHandler,
                    &SConsts::TR_GUNPLANE_ALPHA_KILL,
                    10.0f,
                    &NDb::SAIPropertyConsts::fTargetResolutionGunplaneAlphaKill );
RegisterAIProperty( "AI.TargetResolution.Gunplane.AlphaPrice",
                    NGlobal::VarFloatHandler,
                    &SConsts::TR_GUNPLANE_ALPHA_PRICE,
                    1.0f,
                    &NDb::SAIPropertyConsts::fTargetResolutionGunplaneAlphaPrice );
RegisterAIProperty( "AI.TargetResolution.Gunplane.LimitTime",
                    NGlobal::VarFloatHandler,
                    &SConsts::TR_GUNPLANE_LIMIT_TIME,
                    1000.0f,
                    &NDb::SAIPropertyConsts::fTargetResolutionGunplaneLimitTime );

RegisterAIProperty( "AI.Common.MaxFireRangeToShootByLine",
                    NGlobal::VarFloatHandler,
                    &SConsts::MAX_FIRE_RANGE_TO_SHOOT_BY_LINE,
                    1536.0f,
                    &NDb::SAIPropertyConsts::fCommonMaxFireRangeToShootByLine );

RegisterAIProperty( "AI.Buildings.HPPercentToEscapeFromBuilding",
                    NGlobal::VarFloatHandler,
                    &SConsts::HP_PERCENT_TO_ESCAPE_FROM_BUILDING,
                    0.1f,
                    &NDb::SAIPropertyConsts::fBuildingsHPPercentToEscapeFromBuilding );
RegisterAIProperty( "AI.Infantry.SpyGlassAngle",
                    NGlobal::VarFloatHandler,
                    &fSpyGlassAngle,
                    45.0f,
                    &NDb::SAIPropertyConsts::fInfantrySpyGlassAngle );
RegisterAIProperty( "AI.Common.MinRotateAngle",
                    NGlobal::VarFloatHandler,
                    &fMinRotateAngle,
                    0.1f,
                    &NDb::SAIPropertyConsts::fCommonMinRotateAngle );
RegisterAIProperty( "AI.Aviation.FighterPatrolTime",
                    NGlobal::VarIntHandler,
                    &nFighterPatrolTime,
                    400,
                    &NDb::SAIPropertyConsts::nAviationFighterPatrolTime );
RegisterAIProperty( "AI.Infantry.FullHealthTime",
                    NGlobal::VarIntHandler,
                    &SConsts::INFANTRY_FULL_HEAL_TIME,
                    2000,
                    &NDb::SAIPropertyConsts::nInfantryFullHealthTime );

RegisterAIProperty( "AI.Engineers.RepairCostAdjust",
                    NGlobal::VarFloatHandler,
                    &SConsts::REPAIR_COST_ADJUST,
                    50.0f,
                    &NDb::SAIPropertyConsts::fEngineersRepairCostAdjust );

// TIME_TO_ALLOW_KILL_CRUSHED_OBJ is an int (AIInternalConsts.h:424) and had
// the float handler, so setting it would have stored a float bit pattern.
// Latent: no shipped config names it. The 20000 default already agrees with
// the initialiser, so only the handler changes.
RegisterAIProperty( "AI.Common.TimeToAllowKillCrushedObj",
                    NGlobal::VarIntHandler,
                    &SConsts::TIME_TO_ALLOW_KILL_CRUSHED_OBJ,
                    20000,
                    &NDb::SAIPropertyConsts::nCommonTimeToAllowKillCrushedObj );
//REGISTER_VAR_EX( "AI.Common.TileHeightDiffToLock", NGlobal::VarFloatHandler, &SConsts::TILE_HEIGHT_DIFF_TO_LOCK, 10.0f, STORAGE_NONE );
RegisterAIProperty( "AI.Flags.StorageCaptureTime",
                    NGlobal::VarIntHandler,
                    &SConsts::STORAGE_CAPTURE_TIME,
                    5000,
                    &NDb::SAIPropertyConsts::nFlagsStorageCaptureTime );

RegisterAIProperty(
		"use_round_units", NGlobal::VarBoolHandler, &g_bUseRoundUnits, false, &NDb::SAIPropertyConsts::bUseRoundUnits );

FINISH_REGISTER

bool SConsts::UsesPropertyConsts()
{
	return bUsingPropertyConsts;
}

void SConsts::GetEffectivePropertyConsts( NDb::SAIPropertyConsts *pProperties )
{
	// A snapshot in authored units, useful for comparing migrated data with the legacy profile.
	for ( const auto &binding : PropertyBindings() )
	{
		binding->CopyTo( pProperties );
	}
	pProperties->revealInfo.clear();
	CPtr<IRPGStatsAutomagic> types = MakeObject<IRPGStatsAutomagic>( IRPGStatsAutomagic::tidTypeID );
	for ( const auto &entry : REVEAL_INFO )
	{
		NDb::SAIPropertyRevealInfo info;
		info.szUnitType = types->ToStr( entry.first );
		info.fQuery = entry.second.fRevealByQuery;
		info.fMovingOff = entry.second.fRevealByMovingOff;
		info.fDistance = entry.second.fForgetRevealDistance;
		info.nTime = entry.second.nTimeOfReveal;
		pProperties->revealInfo.push_back( info );
	}
}

void SConsts::Load( const NDb::SAIPropertyConsts *pProperties )
{
	if ( pProperties && !bUsingPropertyConsts )
	{
		legacyRevealInfo = REVEAL_INFO;
	}
	else if ( !pProperties && bUsingPropertyConsts )
	{
		REVEAL_INFO = std::move( legacyRevealInfo );
		legacyRevealInfo.clear();
	}
	for ( const auto &binding : PropertyBindings() )
	{
		binding->SelectSource( pProperties, bUsingPropertyConsts );
	}
	bUsingPropertyConsts = pProperties != nullptr;
	if ( pProperties )
	{
		// Clear previous missions' entries; missing types have zero reveal behavior.
		REVEAL_INFO.clear();
		CPtr<IRPGStatsAutomagic> types = MakeObject<IRPGStatsAutomagic>( IRPGStatsAutomagic::tidTypeID );
		for ( const auto &entry : pProperties->revealInfo )
		{
			const int type = types->ToInt( entry.szUnitType.c_str() );
			if ( type < 0 )
			{
				const auto message = fmt::format( "Unknown AIPropertyConsts reveal unit type: {}", entry.szUnitType );
				CONSOLE_BUFFER_LOG2( PIPE_CHAT, message.c_str(), 0xffff0000, true );
				continue;
			}
			SRevealInfo &info = REVEAL_INFO[type];
			info.fRevealByQuery = entry.fQuery;
			info.fRevealByMovingOff = entry.fMovingOff;
			info.fForgetRevealDistance = entry.fDistance;
			info.nTimeOfReveal = entry.nTime;
		}
	}
	else
	{
		LoadRevealInfo();
	}

	const int nAngle = fSpyGlassAngle / 365 * 65536 / 2;
	SPY_GLASS_ANGLE = ( nAngle > 32768 ) ? 32768 : nAngle;

	SConsts::GENERAL_CELL_SIZE = SConsts::RESUPPLY_RADIUS / 2.0f;

	const int nMinRotateAngle = fMinRotateAngle / 365 * 65536;
	MIN_ROTATE_ANGLE = ( nMinRotateAngle > 65535 ) ? 65535 : nMinRotateAngle;

	if ( PLANE_PARADROP_INTERVAL_PERP_MAX <=
	     PLANE_PARADROP_INTERVAL_PERP_MIN ) // to avoid zero division during random form min to max
	{
		PLANE_PARADROP_INTERVAL_PERP_MAX += 2.0f;
	}

	FIGHTER_PATROL_TIME = 1000 * nFighterPatrolTime;

	//
	if ( AI_CALL_FOR_HELP_RADIUS > 3840 )
	{
		AI_CALL_FOR_HELP_RADIUS = 3840;
		const auto message = fmt::format( "AICallForHelpRadius is too big, reduced to {}", AI_CALL_FOR_HELP_RADIUS );
		CONSOLE_BUFFER_LOG2( PIPE_CHAT, message.c_str(), 0xffff0000, true );
	}
	if ( CALL_FOR_HELP_RADIUS > 3840 )
	{
		CALL_FOR_HELP_RADIUS = 3840;
		const auto message = fmt::format( "CallForHelpRadius is too big, reduced to {}", CALL_FOR_HELP_RADIUS );
		CONSOLE_BUFFER_LOG2( PIPE_CHAT, message.c_str(), 0xffff0000, true );
	}
}

void SConsts::LoadRevealInfo()
{
	CPtr<IRPGStatsAutomagic> pAutoMagic = MakeObject<IRPGStatsAutomagic>( IRPGStatsAutomagic::tidTypeID );

	std::string szStatsIter = pAutoMagic->GetFirstStr();
	do
	{
		const int nStats = pAutoMagic->ToInt( szStatsIter.c_str() );
		std::string szName = "AI.RevealInfo." + szStatsIter + ".Query";
		REVEAL_INFO[nStats].fRevealByQuery = NGlobal::GetVar( szName, 0.0f );

		szName = "AI.RevealInfo." + szStatsIter + ".MovingOff";
		REVEAL_INFO[nStats].fRevealByMovingOff = NGlobal::GetVar( szName, 0.0f );

		szName = "AI.RevealInfo." + szStatsIter + ".Distance";
		REVEAL_INFO[nStats].fForgetRevealDistance = NGlobal::GetVar( szName, 0.0f );

		szName = "AI.RevealInfo." + szStatsIter + ".Time";
		REVEAL_INFO[nStats].nTimeOfReveal = NGlobal::GetVar( szName, 0 );

		szStatsIter = pAutoMagic->GetNextStr( szStatsIter.c_str() );
	} while ( !pAutoMagic->IsLastStr( szStatsIter.c_str() ) );
}

void SConsts::InitPriorities( const std::vector<NDb::SUnitTypePriority> &priorities )
{
	PRIORITIES.clear();
	for ( std::vector<SUnitTypePriority>::const_iterator it = priorities.begin(); it != priorities.end(); ++it )
	{
		PRIORITIES[it->eUnitType] = it->nPriority;
	}
}
