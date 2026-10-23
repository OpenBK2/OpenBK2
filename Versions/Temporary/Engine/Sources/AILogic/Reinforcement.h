#pragma once

namespace NDb
{
struct SAIGameConsts;
enum EDBUnitRPGType : int;
enum EReinforcementType : int;
enum EUnitRPGType : int;
} // namespace NDb

namespace NReinforcement
{
inline int SelectByCumulativeWeight( const float *pWeights, int nCount, float fSample )
{
	// Each cumulative value is the upper bound of its own candidate's interval.
	// The RNG includes its upper endpoint, which belongs to the last nonzero interval.
	int nLastPositive = -1;
	float fPrevious = 0.0f;
	for ( int i = 0; i < nCount; ++i )
	{
		if ( pWeights[i] > fPrevious )
		{
			nLastPositive = i;
			if ( fSample < pWeights[i] )
			{
				return i;
			}
		}
		fPrevious = pWeights[i];
	}
	// Preserve the last-candidate fallback when all available weights are zero.
	return nLastPositive >= 0 ? nLastPositive : nCount - 1;
}

void InitReinforcementTypes( const NDb::SAIGameConsts *pConsts );
const NDb::EReinforcementType GetReinforcementTypeByUnitRPGType( const NDb::EUnitRPGType eType );
const NDb::EReinforcementType GetReinforcementType( const NDb::EDBUnitRPGType eUnitRpgType );
const float GetReinforcementExpediency( const NDb::EReinforcementType eMyType,
                                        const NDb::EReinforcementType eEnemyType );
} // namespace NReinforcement
