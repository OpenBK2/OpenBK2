#include "StdAfx.h"
#include "soundsceneconsts.h"


//*******************************************************************
//*															SSoundSceneConsts										*
//*******************************************************************

float SSoundSceneConsts::SS_SOUND_CELL_SIZE;									// длина стороны в порции
float SSoundSceneConsts::SS_TILE_SIZE;
NTimer::STime SSoundSceneConsts::SS_MIX_DELTA;							// максимальная разница во времени 
NTimer::STime SSoundSceneConsts::SS_UPDATE_PERIOD;					// в милисекундах
NTimer::STime SSoundSceneConsts::SS_SOUND_DIM_TIME;								// время затухания звука при удалении
NTimer::STime SSoundSceneConsts::SS_AMBIENT_SOUND_CHANGE_RANDOM;//for changing looped sounds from time to time
NTimer::STime SSoundSceneConsts::SS_AMBIENT_SOUND_CHANGE;			//for changing not looped sounds from time to time
int SSoundSceneConsts::SS_AMBIENT_TERRAIN_SOUNDS;						//number of playing terrain sounds simualteniously 
float SSoundSceneConsts::TERRAIN_SOUND_RADIUS_MIN;					// radius of constant volume for sounds from terrain(with respect to screen size)
float SSoundSceneConsts::TERRAIN_SOUND_RADIUS_MAX;
float SSoundSceneConsts::TERRAIN_CRITICAL_WEIGHT;						// terrain weight needed to set volume of 1.0f

float SSoundSceneConsts::DEFAULT_SCREEN_WIDTH;
float SSoundSceneConsts::DEFAULT_SCREEN_HEIGHT;
NTimer::STime SSoundSceneConsts::MAP_SOUNDS_UPDATE;

float SSoundSceneConsts::COMBAT_SOUND_FEAR_RADIUS;
float SSoundSceneConsts::COMBAT_FEAR_TIME;
int SSoundSceneConsts::TERRAIN_NONCYCLED_SOUNDS_MIN_RADIUS;
int SSoundSceneConsts::TERRAIN_NONCYCLED_SOUNDS_MAX_RADIUS;

NTimer::STime SSoundSceneConsts::SS_MAP_SOUND_PERIOND;
NTimer::STime SSoundSceneConsts::SS_MAP_SOUND_PERIOND_RANDOM;
int SSoundSceneConsts::MAP_SOUND_CELL;

int SSoundSceneConsts::MIN_SOUND_COUNT_TO_PLAY_LOOPED;

void SSoundSceneConsts::Load()
{
	MAP_SOUND_CELL = NGlobal::GetVar( "Scene.Sound.SpeedTuning.MapSoundCellSize", 500 );
	SS_SOUND_CELL_SIZE = NGlobal::GetVar( "Scene.Sound.SpeedTuning.SoundCellSize", 10.0f );
	SS_TILE_SIZE = 32;

	SS_MIX_DELTA = NGlobal::GetVar( "Scene.Sound.MixDelta", 100);
	SS_UPDATE_PERIOD = NGlobal::GetVar( "Scene.Sound.UpdatePeriod", 150);
	
	SS_SOUND_DIM_TIME = NGlobal::GetVar( "Scene.Sound.DimTime", 100);
	
	SS_AMBIENT_SOUND_CHANGE_RANDOM = NGlobal::GetVar( "Scene.Sound.TerrainSounds.Pause", 1000 );
	SS_AMBIENT_SOUND_CHANGE = NGlobal::GetVar( "Scene.Sound.TerrainSounds.PauseRandom", 5000);
	SS_AMBIENT_TERRAIN_SOUNDS = NGlobal::GetVar( "Scene.Sound.TerrainSounds.NumSounds", 2 );
	TERRAIN_SOUND_RADIUS_MIN	= NGlobal::GetVar( "Scene.Sound.TerrainSounds.MinRadius", 0.5f );
	TERRAIN_SOUND_RADIUS_MAX = NGlobal::GetVar( "Scene.Sound.TerrainSounds.MaxRadius", 0.8f );
	
	TERRAIN_CRITICAL_WEIGHT = NGlobal::GetVar( "Scene.Sound.TerrainSounds.CriticalWeight", 1000 );
	MAP_SOUNDS_UPDATE = NGlobal::GetVar( "Scene.Sound.MapSounds.UpdateTime", 1500 );
	
	DEFAULT_SCREEN_WIDTH = NGlobal::GetVar( "Scene.Sound.ScreenWidth", 1024 );
	DEFAULT_SCREEN_HEIGHT = NGlobal::GetVar( "Scene.Sound.ScreenHeight", 768 );
	
	COMBAT_SOUND_FEAR_RADIUS = NGlobal::GetVar( "Scene.Sound.CombatSounds.FearRadius", 15 );
	COMBAT_FEAR_TIME = NGlobal::GetVar( "Scene.Sound.CombatSounds.FearTime", 3000 );
	TERRAIN_NONCYCLED_SOUNDS_MIN_RADIUS = NGlobal::GetVar( "Scene.Sound.TerrainSounds.NonCycledMinRadius", 15 );
	TERRAIN_NONCYCLED_SOUNDS_MAX_RADIUS = NGlobal::GetVar( "Scene.Sound.TerrainSounds.NonCycledMaxRadius", 30 );
	SS_MAP_SOUND_PERIOND = NGlobal::GetVar( "Scene.Sound.MapSounds.Period", 3000 );
	SS_MAP_SOUND_PERIOND_RANDOM = NGlobal::GetVar( "Scene.Sound.MapSounds.PeriodRandom", 3000 );
	
	MIN_SOUND_COUNT_TO_PLAY_LOOPED = NGlobal::GetVar( "Scene.Sound.MapSounds.MinCountToPlayLooped", 1 );
	
}
