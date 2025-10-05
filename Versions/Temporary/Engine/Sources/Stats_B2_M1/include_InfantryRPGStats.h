virtual void ToAIUnits( bool bInEditor );
//
virtual int GetArmor( const int n ) const { return 0; }
virtual int GetMinPossibleArmor( const int n ) const { return 0; }
virtual int GetMaxPossibleArmor( const int n ) const { return 0; }
virtual int GetRandomArmor( const int n ) const { return 0; }

#include "include_constructorinfo.h"
virtual const SInfantryGun& GetGun( const int nUniqueID, const int nPlatform, const int nGun ) const
{
	int nPlatformIdx, nGunIdx;
	GetGunPlatformIndex( nUniqueID, nPlatform, nGun, &nPlatformIdx, &nGunIdx );
	if ( guns.size() > nGunIdx )
		return guns[nGunIdx];
	else 
	{
		NI_ASSERT( false, StrFmt( "wrong gun index, %i", nGunIdx ) );
		return guns[0];
	}
}

virtual const int GetGunsSize( const int nUniqueID, const int nPlatform ) const
{
	const std::vector<CConstructorInfo::SUnitPlatform> *pPlatforms = 0;
	return 
		ConstructorInfo() && ConstructorInfo()->GetUnitPlatforms( nUniqueID, &pPlatforms ) ?
		(*pPlatforms)[nPlatform].gunIndexes.size() : guns.size();
}

