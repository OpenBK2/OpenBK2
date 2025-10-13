#pragma once

#include "StaticObject.h"

#include <cstdint>

class CFence : public CCommonStaticObject
{
	OBJECT_BASIC_METHODS( CFence );

	ZDATA_( CCommonStaticObject )
		CDBPtr<SFenceRPGStats> pStats;
		SVector leftTile, rightTile;
		NDb::SFenceRPGStats::ETypesOfLife eLifeType;
		// соседние заборы
		std::list< CPtr<CFence> > neighFences;
		int nCreator;													// diplomacy of creator
		bool bSuspendAppear;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,( CCommonStaticObject *)this); f.Add(2,&pStats); f.Add(3,&leftTile); f.Add(4,&rightTile); f.Add(5,&eLifeType); f.Add(6,&neighFences); f.Add(7,&nCreator); f.Add(8,&bSuspendAppear); return 0; }

	//
	void InitDirectionInfo();
	void AnalyzeConnection( CFence *pFence );
	void DamagePartially( CFence * pDestroyedFence );

	void GetVisibility( CSmoothRotatedArray2D<uint8_t, const SHPObjectRPGStats::SByteArray2 > *visibity ) const;
	void GetPassability( CSmoothRotatedArray2D<uint8_t, const SHPObjectRPGStats::SByteArray2 > *passability ) const;

	int GetHeight() const;
public:
	CFence() { }
	CFence( const SFenceRPGStats *pStats, const CVec3 &center, const float fHP, const uint16_t wDir, const int nDiplomacy, const int nFrameIndex );
	virtual void Init();

	virtual const uint8_t GetPlayer() const { return nCreator; }

	const struct SHPObjectRPGStats *GetStats() const { return pStats; }

	virtual EStaticObjType GetObjectType() const { return ESOT_FENCE; }

	virtual void Die( const float fDamage );
	virtual void Delete();

	void GetNewUnitInfo( SNewUnitInfo *pNewUnitInfo );

	virtual bool CanUnitGoThrough( const EAIClasses &eClass ) const;
	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const;
};


