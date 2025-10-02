#pragma once

enum EAIClasses
{
	EAC_NONE  = 0x00,
	EAC_WHELL = 0x01,
	EAC_TRACK = 0x02,
	EAC_HUMAN = 0x04,
	EAC_RIVER = 0x08,
	EAC_SEA   = 0x10,

	EAC_COUNT = 5,

	EAC_WATER = EAC_RIVER | EAC_SEA, 
	EAC_TERRAIN = EAC_WHELL | EAC_TRACK | EAC_HUMAN,

	EAC_ANY   = EAC_WATER | EAC_TERRAIN, // в Terrain.cpp считается, что EAC_ANY имеет самое большое значение (нужно в classIndices  !!!!

	EAC_FORCE_DWORD = 0x7FFFFFFF,
};

inline const int GetClassIndex( const BYTE aiClass )
{
	switch( aiClass ) 
	{
	case EAC_WHELL:    return 0;
	case EAC_TRACK:    return 1;
	case EAC_HUMAN:    return 2;
	case EAC_RIVER:    return 3;
	case EAC_SEA:      return 4;
	case EAC_WATER:    return 5;
	case EAC_TERRAIN:  return 6;
	case EAC_ANY:      return 7;  // это для амфибий
	case EAC_COUNT:    return 8;  // на EAC_COUNT необходимо вернуть количество индексов, это нужно для 
                                // инициализации массивов толщины юнитов
	default:
// при вычислении пути от колонны разнотипных юнитов можно легко получить EAC_WHELL | EAC_TRACK
//		NI_ASSERT( 0, StrFmt( "Unknown (%d) AIClass for passability", aiClass ) );
		return -1;
	}
}

