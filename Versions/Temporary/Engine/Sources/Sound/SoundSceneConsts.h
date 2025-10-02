#pragma once


struct SSoundSceneConsts
{
	static void Load();

	static float SS_SOUND_CELL_SIZE;									// длина стороны в порции
	static float SS_TILE_SIZE;
	static NTimer::STime SS_MIX_DELTA;							// максимальная разница во времени 
	// появления звуков при SFX_MIX_IF_TIME_EQUALS
	static NTimer::STime SS_UPDATE_PERIOD;					// в милисекундах
	static NTimer::STime SS_SOUND_DIM_TIME;								// время затухания звука при удалении

	static NTimer::STime SS_AMBIENT_SOUND_CHANGE_RANDOM;//for changing looped sounds from time to time
	static NTimer::STime SS_AMBIENT_SOUND_CHANGE;			//for changing not looped sounds from time to time
	static int SS_AMBIENT_TERRAIN_SOUNDS;						//number of playing terrain sounds simualteniously 
	static float TERRAIN_SOUND_RADIUS_MIN;					//in percent of screen size
	static float TERRAIN_SOUND_RADIUS_MAX;
	static float TERRAIN_CRITICAL_WEIGHT;
	static float DEFAULT_SCREEN_WIDTH;
	static float DEFAULT_SCREEN_HEIGHT;
	static int MAP_SOUND_CELL;

	static int TERRAIN_NONCYCLED_SOUNDS_MIN_RADIUS;
	static int TERRAIN_NONCYCLED_SOUNDS_MAX_RADIUS;

	static NTimer::STime MAP_SOUNDS_UPDATE;

	static float COMBAT_SOUND_FEAR_RADIUS;
	static float COMBAT_FEAR_TIME;

	static NTimer::STime SS_MAP_SOUND_PERIOND;
	static NTimer::STime SS_MAP_SOUND_PERIOND_RANDOM;

	static int MIN_SOUND_COUNT_TO_PLAY_LOOPED;
};

