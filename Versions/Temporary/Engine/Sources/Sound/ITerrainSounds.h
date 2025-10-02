#pragma once

#include "TerrainSoundDesc.h"

namespace NDb
{
	struct SComplexSoundDesc;
}

struct ITerrainSounds : public virtual CObjectBase
{
	// for sound
	//returns random sound for this terrain or 0
	virtual const NDb::SComplexSoundDesc * GetTerrainSound( int nTerrainType ) = 0;

	//virtual int GetNTerrainCycleSounds( int nTerrainType ) = 0;
	virtual const NDb::SComplexSoundDesc * GetTerrainCycleSound( int nTerrainType ) = 0;
	// scans trough visible terrain and returns data about it
	virtual void GetTerrainMassData( vector<SSoundTerrainInfo> *pData, int nMaxSize ) = 0;
	//returns relative volume of all sounds for specific terrain
	virtual float GetSoundVolume( int nTerrainType ) const = 0 ;
};


