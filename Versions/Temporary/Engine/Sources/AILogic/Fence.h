#pragma once

#include "StaticObject.h"

#include <cstdint>

class CFence : public CCommonStaticObject
{
	OBJECT_BASIC_METHODS( CFence );

	ZDATA_( CCommonStaticObject )
	CDBPtr<SFenceRPGStats> pStats;
	SVector leftTile, rightTile;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	NDb::SFenceRPGStats::ETypesOfLife eLifeType = {};
	// соседние заборы
	std::list<CPtr<CFence>> neighFences;
	int nCreator = {}; // diplomacy of creator
	bool bSuspendAppear = {};

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CCommonStaticObject *)this );
		f.Add( 2, &pStats );
		f.Add( 3, &leftTile );
		f.Add( 4, &rightTile );
		f.Add( 5, &eLifeType );
		f.Add( 6, &neighFences );
		f.Add( 7, &nCreator );
		f.Add( 8, &bSuspendAppear );
		return 0;
	}

	//
	void InitDirectionInfo();
	void AnalyzeConnection( CFence *pFence );
	void DamagePartially( CFence *pDestroyedFence );

	void GetVisibility( CSmoothRotatedArray2D<uint8_t, const SHPObjectRPGStats::SByteArray2> *visibity ) const override;
	void
	GetPassability( CSmoothRotatedArray2D<uint8_t, const SHPObjectRPGStats::SByteArray2> *passability ) const override;

	int GetHeight() const override;

public:
	CFence() {}

	CFence( const SFenceRPGStats *pStats,
	        const CVec3 &center,
	        const float fHP,
	        const uint16_t wDir,
	        const int nDiplomacy,
	        const int nFrameIndex );
	void Init() override;

	const uint8_t GetPlayer() const override
	{
		return nCreator;
	}

	const struct SHPObjectRPGStats *GetStats() const override
	{
		return pStats;
	}

	EStaticObjType GetObjectType() const override
	{
		return ESOT_FENCE;
	}

	void Die( const float fDamage ) override;
	void Delete() override;

	void GetNewUnitInfo( SNewUnitInfo *pNewUnitInfo ) override;

	bool CanUnitGoThrough( const EAIClasses &eClass ) const override;
	bool ShouldSuspendAction( const EActionNotify &eAction ) const override;
};
