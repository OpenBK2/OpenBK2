void ToAIUnits( bool bInEditor ) override;

//
int GetArmor( const int n ) const override
{
	return 0;
}

int GetMinPossibleArmor( const int n ) const override
{
	return 0;
}

int GetMaxPossibleArmor( const int n ) const override
{
	return 0;
}

int GetRandomArmor( const int n ) const override
{
	return 0;
}

#include "include_constructorinfo.h"

const SInfantryGun &GetGun( const int nUniqueID, const int nPlatform, const int nGun ) const override
{
	int nPlatformIdx, nGunIdx;
	GetGunPlatformIndex( nUniqueID, nPlatform, nGun, &nPlatformIdx, &nGunIdx );
	if ( guns.size() > nGunIdx )
	{
		return guns[nGunIdx];
	}
	else
	{
		NI_ASSERT( false, fmt::format( "wrong gun index, {}", nGunIdx ) );
		return guns[0];
	}
}

const int GetGunsSize( const int nUniqueID, const int nPlatform ) const override
{
	const std::vector<CConstructorInfo::SUnitPlatform> *pPlatforms = 0;
	return ConstructorInfo() && ConstructorInfo()->GetUnitPlatforms( nUniqueID, &pPlatforms )
	           ? ( *pPlatforms )[nPlatform].gunIndexes.size()
	           : guns.size();
}
