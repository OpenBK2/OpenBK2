#pragma once

#include "StaticObject.h"

class CFakeCorpseStaticObject : public CCommonStaticObject
{
	OBJECT_BASIC_METHODS( CFakeCorpseStaticObject );

	ZDATA_( CCommonStaticObject )
		std::list<SObjTileInfo> tilesToLock;
		CObj<CUpdatableObj> pDeadObj;
		EStaticObjType eType;
		CPtr<CObjectProfile> pPassProfile;
		bool bDestructByTracks;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CCommonStaticObject *)this); f.Add(2,&tilesToLock); f.Add(3,&pDeadObj); f.Add(4,&eType); f.Add(5,&pPassProfile); return 0; }

	//
	CFakeCorpseStaticObject( const CVec3 &center, const WORD wDir, const float fHP, const int nFrameIndex,
													 const std::list<SObjTileInfo> &tiles, const bool bDestructByTracks,
													 CUpdatableObj* pDeadObj, CObjectProfile *pPassProfile );
	CFakeCorpseStaticObject() { }
public:
	static void CreateFakeCorpseStaticObject( class CExistingObject *pObj );
	static void CreateFakeCorpseStaticObject( class CAIUnit *pUnit, const std::list<SObjTileInfo> &tiles, const bool bCantCrushForSomeTime );

	virtual void Init() { }

	virtual const BYTE GetPlayer() const; 
	virtual const SHPObjectRPGStats* GetStats() const { return 0; }
	virtual bool CanUnitGoThrough( const EAIClasses &eClass ) const { return bDestructByTracks && (eClass & EAC_TRACK); }
	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const;

	virtual void GetCoveredTiles( std::list<SVector> *pTiles ) const;
	virtual void LockTiles();
	virtual void UnlockTiles();
	virtual void CreateLockedTilesInfo( std::list<SObjTileInfo> *pTiles );
	virtual void SetTransparencies() {}
	virtual void RemoveTransparencies() {}
	virtual void RestoreTransparenciesImmidiately() {}
	virtual bool ProcessAreaDamage( const class CExplosion *pExpl, const int nArmorDir, const float fRadius, const float fSmallRadius );
	virtual void GetTilesForVisibility( CTilesSet *pTiles ) const {}

	virtual void Delete();

	bool CanFall() { return false; }

	virtual EStaticObjType GetObjectType() const { return eType; }
	void ChangeType( const EStaticObjType eNewType ) { eType = eNewType; }

	virtual CObjectProfile* GetPassProfile() const { return pPassProfile; }
};


