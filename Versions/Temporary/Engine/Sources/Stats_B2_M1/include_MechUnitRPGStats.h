WORD wDivingAngle;
WORD wClimbingAngle;
WORD wTiltAngle;
hash_map</*enum EManuverID*/int, bool> manuverMap;

virtual void ToAIUnits( bool bInEditor );
//
virtual int GetArmor( const int n ) const { return ( armors[n].nMin + armors[n].nMax ) / 2; }
virtual int GetMinPossibleArmor( const int n ) const { return armors[n].nMin; }
virtual int GetMaxPossibleArmor( const int n ) const { return armors[n].nMax; }
virtual int GetRandomArmor( const int n ) const { return NRandom::RandomCheck( armors[n].nMin, armors[n].nMax ); }
//
bool HasDieselEffect() const { return pEffectDiesel != 0; }
bool HasSmokeEffect() const { return pEffectSmoke != 0; }
bool HasMoveStartSound() const { return pSoundMoveStart != 0; }
bool HasMoveSound() const { return pSoundMoveCycle != 0; }
bool HasMoveStopSound() const { return pSoundMoveStop != 0; }

virtual const float GetTurnRadius() const { return fTurnRadius; }
//
const vector<CVec2>* GetGunners( const int nMode ) const { return nMode < gunners.size() ? &(gunners[nMode].gunners) : 0; }
//

#include "include_constructorinfo.h"
virtual const SMechUnitGun& GetGun( const int nUniqueID, const int nPlatform, const int nGun ) const
{
	int nPlatformIdx, nGunIdx;
	GetGunPlatformIndex( nUniqueID, nPlatform, nGun, &nPlatformIdx, &nGunIdx );
	return platforms[nPlatformIdx].guns[nGunIdx];
}

virtual const int GetGunsSize( const int nUniqueID, const int nPlatform ) const
{
	if ( ConstructorInfo() )
	{
		const vector<CConstructorInfo::SUnitPlatform> *pPlatforms = 0;
		if ( ConstructorInfo()->GetUnitPlatforms( nUniqueID, &pPlatforms ) )
			return (*pPlatforms)[nPlatform].gunIndexes.size();
	}
	return platforms[nPlatform].guns.size();
}

const SPlatform& GetPlatform( const int nUniqueID, const int nPlatform ) const
{
	if ( ConstructorInfo() )
	{
		const vector<CConstructorInfo::SUnitPlatform> *pPlatforms = 0;
		if ( ConstructorInfo()->GetUnitPlatforms( nUniqueID, &pPlatforms ) )
			return platforms[(*pPlatforms)[nPlatform].nPlatformIndex] ;
	}
	return nPlatform < platforms.size() ? platforms[nPlatform] : platforms[0];
}

const int GetPlatformsSize( const int nUniqueID ) const
{
	if ( ConstructorInfo() )
	{
		const vector<CConstructorInfo::SUnitPlatform> *pPlatforms = 0;
		if ( ConstructorInfo()->GetUnitPlatforms( nUniqueID, &pPlatforms ) )
			return pPlatforms->size();
	}
	return platforms.size();
}

