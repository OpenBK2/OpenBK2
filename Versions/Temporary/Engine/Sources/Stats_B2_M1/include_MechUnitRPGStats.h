#include <cstdint>

// Keep derived angles deterministic until PostLoad converts the database values.
uint16_t wDivingAngle = 0;
uint16_t wClimbingAngle = 0;
uint16_t wTiltAngle = 0;
std::unordered_map</*enum EManuverID*/ int, bool> manuverMap;

void ToAIUnits( bool bInEditor ) override;

//
int GetArmor( const int n ) const override
{
	return ( armors[n].nMin + armors[n].nMax ) / 2;
}

int GetMinPossibleArmor( const int n ) const override
{
	return armors[n].nMin;
}

int GetMaxPossibleArmor( const int n ) const override
{
	return armors[n].nMax;
}

int GetRandomArmor( const int n ) const override
{
	RecordRandomCall();
	return NRandom::RandomCheck( armors[n].nMin, armors[n].nMax );
}

//
bool HasDieselEffect() const
{
	return pEffectDiesel != 0;
}

bool HasSmokeEffect() const
{
	return pEffectSmoke != 0;
}

bool HasMoveStartSound() const
{
	return pSoundMoveStart != 0;
}

bool HasMoveSound() const
{
	return pSoundMoveCycle != 0;
}

bool HasMoveStopSound() const
{
	return pSoundMoveStop != 0;
}

const float GetTurnRadius() const override
{
	return fTurnRadius;
}

//
const std::vector<CVec2> *GetGunners( const int nMode ) const
{
	return nMode < gunners.size() ? &( gunners[nMode].gunners ) : 0;
}

// A unit has configured gunners if any movement mode contains at least one gunner position.
bool HasConfiguredGunners() const
{
	for ( int nMode = 0; nMode < gunners.size(); ++nMode )
	{
		if ( !gunners[nMode].gunners.empty() )
		{
			return true;
		}
	}
	return false;
}

//

#include "include_constructorinfo.h"

const SMechUnitGun &GetGun( const int nUniqueID, const int nPlatform, const int nGun ) const override
{
	int nPlatformIdx, nGunIdx;
	GetGunPlatformIndex( nUniqueID, nPlatform, nGun, &nPlatformIdx, &nGunIdx );
	return platforms[nPlatformIdx].guns[nGunIdx];
}

const int GetGunsSize( const int nUniqueID, const int nPlatform ) const override
{
	if ( ConstructorInfo() )
	{
		const std::vector<CConstructorInfo::SUnitPlatform> *pPlatforms = 0;
		if ( ConstructorInfo()->GetUnitPlatforms( nUniqueID, &pPlatforms ) )
		{
			return ( *pPlatforms )[nPlatform].gunIndexes.size();
		}
	}
	return platforms[nPlatform].guns.size();
}

const SPlatform &GetPlatform( const int nUniqueID, const int nPlatform ) const
{
	if ( ConstructorInfo() )
	{
		const std::vector<CConstructorInfo::SUnitPlatform> *pPlatforms = 0;
		if ( ConstructorInfo()->GetUnitPlatforms( nUniqueID, &pPlatforms ) )
		{
			return platforms[( *pPlatforms )[nPlatform].nPlatformIndex];
		}
	}
	return nPlatform < platforms.size() ? platforms[nPlatform] : platforms[0];
}

const int GetPlatformsSize( const int nUniqueID ) const
{
	if ( ConstructorInfo() )
	{
		const std::vector<CConstructorInfo::SUnitPlatform> *pPlatforms = 0;
		if ( ConstructorInfo()->GetUnitPlatforms( nUniqueID, &pPlatforms ) )
		{
			return pPlatforms->size();
		}
	}
	return platforms.size();
}

bool IsAmphibious() const
{
	return ( nAIPassabilityClass & EAC_WATER ) != 0 && ( nAIPassabilityClass & EAC_TERRAIN ) != 0;
}
