#include "stdafx.h"

#include "GeneralConsts.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int SGeneralConsts::TIME_DONT_SEE_ENEMY_BEFORE_FORGET = 60000; //A minute, Originally was 5000;
int SGeneralConsts::SCOUT_FREE_POINT = 300;						// скаут шлется в точку, если в этом радиусе от нее нет наших
int SGeneralConsts::SCOUT_POINTS = 4;
NTimer::STime SGeneralConsts::TIME_SONT_SEE_AA_BEFORE_FORGET = 180000;			// связать с временем регенерации самолетов

int SGeneralConsts::AVIATION_PERIOD_MAX = 60000;
int SGeneralConsts::AVIATION_PERIOD_MIN = 10000;

int SGeneralConsts::FIGHTER_PERIOD_MAX = 20000;
int SGeneralConsts::FIGHTER_PERIOD_MIN = 10000;
float SGeneralConsts::FIGHTER_INTERCEPT_OFFSET = 1000;

float SGeneralConsts::AVATION_LONG_PERIOD_PROBABILITY = 0.5f;
float SGeneralConsts::AVATION_PERIOD_MULTIPLY = 5;
float SGeneralConsts::PLAYER_FORCE_COEFFICIENT = 2.0;

const int SGeneralConsts::RESISTANCE_CELL_SIZE = 32;
float SGeneralConsts::TIME_DONT_SEE_EMPTY_ARTILLERY_BEFORE_FORGET = 20000;
float SGeneralConsts::TIME_TO_FORGET_ANTI_ARTILLERY = 240000;
float SGeneralConsts::TIME_TO_FORGET_UNIT = 180000;
float SGeneralConsts::TIME_TO_ARTILLERY_FIRE = 3000;
float SGeneralConsts::PROBABILITY_TO_SHOOT_AFTER_ARTILLERY_FIRE = 1.0f;
float SGeneralConsts::SHOOTS_OF_ARTILLERY_FIRE = 10.0f;
int SGeneralConsts::GENERAL_UPDATE_PERIOD = 1000;
float SGeneralConsts::REPAIR_STORAGE_PROBABILITY = 0.1f;
float SGeneralConsts::RECAPTURE_STORAGE_PROBALITY = 0.2f;
float SGeneralConsts::RECAPTURE_STORAGE_MAX_DISTANCE = 2000.0f;
int SGeneralConsts::RECAPTURE_ARTILLERY_TANKS_NUMBER = 2; 
int SGeneralConsts::RESUPPLY_CELL_AFTER_TRANSPORT_DEATH;
int SGeneralConsts::RESUPPLY_CELL_AFTER_TRANSPORT_DEATH_RAND;
float SGeneralConsts::INTENDANT_DANGEROUS_CELL_RADIUS = 1000;
int SGeneralConsts::SWARM_ADDITIONAL_ITERATIONS = 3;
float SGeneralConsts::MIN_WEIGHT_TO_SEND_SWARM = 30.0f;

// минимальный вес ячейки, чтобы начать обстрел артиллерией
float SGeneralConsts::MIN_WEIGHT_TO_ARTILLERY_FIRE = 50.0f;
// минимальный вес ячейки, чтобы послать бомберы
float SGeneralConsts::MIN_WEIGHT_TO_SEND_BOMBERS = 100.0f;
float SGeneralConsts::SWARM_WEIGHT_COEFFICIENT = 1.0f;

int SGeneralConsts::TIME_TO_WAIT_SWARM_READY = 10; 
int SGeneralConsts::TIME_TO_WAIT_SWARM_READY_RANDOM = 10;

int SGeneralConsts::TIME_SWARM_DURATION;
int SGeneralConsts::TIME_SWARM_DURATION_RANDOM;
int SGeneralConsts::MIN_UNIT_WEIGHT_TO_SEND_GUNPLANES;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SGeneralConsts::Init()
{
	TIME_DONT_SEE_ENEMY_BEFORE_FORGET = NGlobal::GetVar( "AI.General.TimeDontSeeTheEnemyBeforeForget", 10000 );
	
	SCOUT_FREE_POINT = NGlobal::GetVar( "AI.General.Aviation.ScoutFreePoint", 300 );
	SCOUT_POINTS = NGlobal::GetVar( "AI.General.Aviation.ScoutPointsPerMap", 4 );

	TIME_SONT_SEE_AA_BEFORE_FORGET = NGlobal::GetVar( "AI.General.TimeDontSeeAABeforeForget", 10000 );
	FIGHTER_INTERCEPT_OFFSET = NGlobal::GetVar( "AI.General.Aviation.FighterInterceptOffset", 1000 );

	AVIATION_PERIOD_MAX = NGlobal::GetVar( "AI.General.Aviation.PeriodMax", 30000 );
	AVIATION_PERIOD_MIN = NGlobal::GetVar( "AI.General.Aviation.PeriodMin", 10000 );

	FIGHTER_PERIOD_MAX = NGlobal::GetVar( "AI.General.Aviation.FighterPeriodMax", 10000 );
	FIGHTER_PERIOD_MIN = NGlobal::GetVar( "AI.General.Aviation.FighterPeriodMin", 5000 );

	AVATION_LONG_PERIOD_PROBABILITY = NGlobal::GetVar( "AI.General.Aviation.LongPeriodProbability", 0.5f );
	AVATION_PERIOD_MULTIPLY = NGlobal::GetVar( "AI.General.Aviation.PeriodMultiply", 5.0f );
	PLAYER_FORCE_COEFFICIENT = NGlobal::GetVar( "AI.General.PlayerForceMultiply", 2.0f );

	TIME_TO_FORGET_ANTI_ARTILLERY = NGlobal::GetVar( "AI.General.Artillery.TimeToForgetAntiArtillery", 240000 );
	TIME_TO_FORGET_UNIT = NGlobal::GetVar( "AI.General.Artillery.TimeToForgetUnit", 120000 );
	TIME_TO_ARTILLERY_FIRE = NGlobal::GetVar( "AI.General.Artillery.TimeToArtilleryFire", 3000.0f );
	PROBABILITY_TO_SHOOT_AFTER_ARTILLERY_FIRE = NGlobal::GetVar( "AI.General.Artillery.ProbabilityToShootAfterArtilleryFire", 1.0f );
	SHOOTS_OF_ARTILLERY_FIRE = NGlobal::GetVar( "AI.General.Artillery.ShootsOfArtilleryFire", 10.0f );
	MIN_WEIGHT_TO_ARTILLERY_FIRE = NGlobal::GetVar( "AI.General.Artillery.MinWeightToArtilleryFire", 3.0f );
	MIN_WEIGHT_TO_SEND_BOMBERS = NGlobal::GetVar( "AI.General.Artillery.MinWeightToSendBombers", 100.0f );

	
	GENERAL_UPDATE_PERIOD = NGlobal::GetVar( "AI.General.UpdatePeriod", 3000 );
	REPAIR_STORAGE_PROBABILITY = NGlobal::GetVar( "AI.General.Intendant.ProbabilityToRepairStorage", 0.1f );
	RECAPTURE_STORAGE_PROBALITY = NGlobal::GetVar( "AI.General.Intendant.ProbabilityToRecaptureStorage", 0.4f );

	SWARM_ADDITIONAL_ITERATIONS = NGlobal::GetVar( "AI.General.Swarm.Iterations", 3 );
	MIN_WEIGHT_TO_SEND_SWARM = NGlobal::GetVar( "AI.General.Swarm.MinWeight", 30.0f );
	SWARM_WEIGHT_COEFFICIENT = NGlobal::GetVar( "AI.General.Swarm.WeightCoefficient", 1.0f );

	RESUPPLY_CELL_AFTER_TRANSPORT_DEATH = NGlobal::GetVar( "AI.General.Intendant.ResupplyCellPeriodAfterDeath", 10000 );
	RESUPPLY_CELL_AFTER_TRANSPORT_DEATH_RAND = NGlobal::GetVar( "AI.General.Intendant.ResupplyCellPeriodAfterDeathRandom", 20000 );
	INTENDANT_DANGEROUS_CELL_RADIUS = NGlobal::GetVar( "AI.General.Intendant.DangerousCellRadius", 1000 );

	RECAPTURE_STORAGE_MAX_DISTANCE = NGlobal::GetVar( "AI.General.Intendant.RecaptureStorageMaxDistance", 2000.0f );
	RECAPTURE_ARTILLERY_TANKS_NUMBER = NGlobal::GetVar( "AI.General.Intendant.RecaptureStorageMaxUnits", 2 );
	TIME_DONT_SEE_EMPTY_ARTILLERY_BEFORE_FORGET = NGlobal::GetVar( "AI.General.Intendant.TimeDontSeeEmptyArtilleryBeforeForget", 2000 );

	TIME_TO_WAIT_SWARM_READY = NGlobal::GetVar( "AI.General.Swarm.WaitTime", 10 );
	TIME_TO_WAIT_SWARM_READY_RANDOM = NGlobal::GetVar( "AI.General.Swarm.WaitTimeRandom", 10 );

	TIME_SWARM_DURATION = NGlobal::GetVar( "AI.General.Swarm.IterationDuration", 20 );
	TIME_SWARM_DURATION_RANDOM = NGlobal::GetVar( "AI.General.Swarm.IterationDurationRandom", 20 );
	MIN_UNIT_WEIGHT_TO_SEND_GUNPLANES = NGlobal::GetVar( "AI.General.Gunplanes.MinUnitWeight", 50 );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
