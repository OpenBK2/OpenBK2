#pragma once

namespace NDb
{
	struct SAIGameConsts;
	enum EDBUnitRPGType : int;
	enum EReinforcementType : int;
	enum EUnitRPGType : int;
}

namespace NReinforcement
{
	void InitReinforcementTypes( const NDb::SAIGameConsts *pConsts );
	const NDb::EReinforcementType GetReinforcementTypeByUnitRPGType( const NDb::EUnitRPGType eType );
	const NDb::EReinforcementType GetReinforcementType( const NDb::EDBUnitRPGType eUnitRpgType );
	const float GetReinforcementExpediency( const NDb::EReinforcementType eMyType, const NDb::EReinforcementType eEnemyType );
}


