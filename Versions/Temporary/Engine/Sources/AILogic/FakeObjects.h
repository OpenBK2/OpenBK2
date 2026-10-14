#pragma once

#include "StaticObject.h"

#include <cstdint>

class CFakeCorpseStaticObject : public CCommonStaticObject
{
	OBJECT_BASIC_METHODS( CFakeCorpseStaticObject );

	ZDATA_( CCommonStaticObject )
	std::list<SObjTileInfo> tilesToLock;
	CObj<CUpdatableObj> pDeadObj;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	EStaticObjType eType = {};
	CPtr<CObjectProfile> pPassProfile;
	bool bDestructByTracks = {};

	// Track-crushability affects pathfinding and must survive save/load.
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CCommonStaticObject *)this );
		f.Add( 2, &tilesToLock );
		f.Add( 3, &pDeadObj );
		f.Add( 4, &eType );
		f.Add( 5, &pPassProfile );
		f.Add( 6, &bDestructByTracks );
		return 0;
	}

	//
	CFakeCorpseStaticObject( const CVec3 &center,
	                         const uint16_t wDir,
	                         const float fHP,
	                         const int nFrameIndex,
	                         const std::list<SObjTileInfo> &tiles,
	                         const bool bDestructByTracks,
	                         CUpdatableObj *pDeadObj,
	                         CObjectProfile *pPassProfile );

	CFakeCorpseStaticObject()
		: bDestructByTracks( false )
	{
	}

public:
	static void CreateFakeCorpseStaticObject( class CExistingObject *pObj );
	static void CreateFakeCorpseStaticObject( class CAIUnit *pUnit,
	                                          const std::list<SObjTileInfo> &tiles,
	                                          const bool bCantCrushForSomeTime );

	void Init() override {}

	const uint8_t GetPlayer() const override;

	const SHPObjectRPGStats *GetStats() const override
	{
		return 0;
	}

	bool CanUnitGoThrough( const EAIClasses &eClass ) const override
	{
		return bDestructByTracks && ( eClass & EAC_TRACK );
	}

	bool ShouldSuspendAction( const EActionNotify &eAction ) const override;

	void GetCoveredTiles( std::list<SVector> *pTiles ) const override;
	void LockTiles() override;
	void UnlockTiles() override;
	void CreateLockedTilesInfo( std::list<SObjTileInfo> *pTiles ) override;

	void SetTransparencies() override {}

	void RemoveTransparencies() override {}

	void RestoreTransparenciesImmidiately() override {}

	bool ProcessAreaDamage( const class CExplosion *pExpl,
	                        const int nArmorDir,
	                        const float fRadius,
	                        const float fSmallRadius ) override;

	void GetTilesForVisibility( CTilesSet *pTiles ) const override {}

	void Delete() override;

	bool CanFall() override
	{
		return false;
	}

	EStaticObjType GetObjectType() const override
	{
		return eType;
	}

	void ChangeType( const EStaticObjType eNewType )
	{
		eType = eNewType;
	}

	CObjectProfile *GetPassProfile() const override
	{
		return pPassProfile;
	}
};
