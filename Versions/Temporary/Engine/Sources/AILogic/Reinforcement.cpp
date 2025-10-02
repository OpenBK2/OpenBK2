#include "stdafx.h"

#include "Reinforcement.h"
#include "..\Misc\2Darray.h"
#include "DBAIConsts.h"

namespace NReinforcement
{

static hash_map<int/*NDb::EUnitRPGType*/, NDb::EReinforcementType> reinfTypes;

// reinf.type X beats reinf.type Y with effectiveness eV[X][Y]  (0..1)
// 0 is completely inefficient, 1 is "eliminates without losses", 0.5 is "balance each other"
// These are default values, real values are taken from DB (AIConsts base)
// (NB: Table is not exactly symmetric, to allow for ineffective calls, i.e. fighter planes vs land units)
#define NUMBER_OF_REINF_TYPES			20
static CArray2D<float> expediencyValues;

void InitReinforcementTypes( const NDb::SAIGameConsts *_pConsts )
{
	for ( int i = 0; i < _pConsts->reinforcementTypes.size(); ++i )
		reinfTypes[int(ReMapRPGType(_pConsts->reinforcementTypes[i].eUnitRPGType))] = _pConsts->reinforcementTypes[i].eReinfType;

	//Init reinf.expediency matrix
	expediencyValues.SetSizes( NUMBER_OF_REINF_TYPES, NUMBER_OF_REINF_TYPES );
	expediencyValues.FillZero();
	for ( int i = 0; i < _pConsts->reinfExpediency.size(); ++i )
	{
		const vector<float> &expSet = _pConsts->reinfExpediency[i].expediency;
		int nSize = expSet.size();
		for ( int j = 0; j < nSize; ++j )
		{
			float fValue = expSet[ j ];
			if ( fValue >= 0.0f && fValue <= 1.0f ) 
				expediencyValues[ i ][ j ] = fValue;
			else
				NI_ASSERT( false, StrFmt( "Invalid Expediency value (%d, %d)", i, j ) );
		}
	}
}

const NDb::EReinforcementType GetReinforcementTypeByUnitRPGType( const NDb::EUnitRPGType eType )
{
	hash_map<int/*NDb::EUnitRPGType*/, NDb::EReinforcementType>::const_iterator pos = reinfTypes.find( eType );
	if ( pos == reinfTypes.end() )
	{
		NI_ASSERT( 0, StrFmt( "Reinforcement type not found for NDb::EUnitRPGType: %d", eType ) );
		return NDb::_RT_NONE;
	}
	return pos->second;
}

const NDb::EReinforcementType GetReinforcementType( const NDb::EDBUnitRPGType eUnitRpgType )
{
	hash_map<int/*NDb::EUnitRPGType*/, NDb::EReinforcementType>::const_iterator pos = reinfTypes.find( int(ReMapRPGType(eUnitRpgType)) );
	if ( pos != reinfTypes.end() )
		return pos->second;
	NI_ASSERT( 0, StrFmt( "Reinforcement type not found for NDb::EDBUnitRPGType: %d", eUnitRpgType ) );
	return NDb::_RT_NONE;
}

const float GetReinforcementExpediency( const NDb::EReinforcementType eMyType, const NDb::EReinforcementType eEnemyType )
{
	if ( NUMBER_OF_REINF_TYPES > eEnemyType && NUMBER_OF_REINF_TYPES > eMyType )
		return expediencyValues[eMyType][eEnemyType];
	return 0.0f;
}

}


