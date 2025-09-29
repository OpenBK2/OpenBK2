#ifndef __STATISTICS_H__
#define __STATISTICS_H__
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma ONCE
namespace NDb
{
	enum EReinforcementType;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CStatistics
{
	public: int operator&( IBinSaver &saver ); private:;

	bool bEnablePlayerExp;										// can we add player exp? (false in tutorial mode - initialized in the Init())

public:
	CStatistics() : bEnablePlayerExp( false ) {  }

	void Init();

	const int GetXPLevel( const int nPlayer, const NDb::EReinforcementType eType );
	const int GetAbilityLevel( const int nPlayer, const NDb::EReinforcementType eType );		// Different from XP level, because of leaders

	float GetValue( const int nValue, const int nPlayer ) const;
	int GetUnitLevel( const NDb::EReinforcementType eType, const int nPlayer ) const;
	// player captured oter player's unit
	void UnitCaptured( const int nPlayer );
	// игрок nPlayer уничтожил юниты игрока nKilledUnitsPlayer, fTotalAIPrice - их price
	void UnitKilled( const int nPlayer, const int nKilledUnitsPlayer, const float fTotalAIPrice, const EReinforcementType eKillerType, const EReinforcementType eDeadType, const bool bInfantry );
	// unit умер
	void UnitDead( class CCommonUnit *pUnit );
	// игрок nPlayer уничтожил house
	void ObjectDestroyed( const int nPlayer );
	// игрок nPlayer вызвал авиацию
	void AviationCalled( const int nPlayer );
	// игрок nPlayer использовал reinforcement
	void ReinforcementUsed( const int nPlayer );
	// игрок nPlayer использовал ресурсы
	void ResourceUsed( const int nPlayer, const float fResources );
	//player's experience
	void IncreasePlayerExperience( const int nPlayer, const NDb::EReinforcementType eType, const float fPrice ) ;

	void SetFlagPoints( const int nParty, const float fPoints );
	void SetCapturedFlags( const int nParty, const int nFlags );

};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // __STATISTICS_H__
