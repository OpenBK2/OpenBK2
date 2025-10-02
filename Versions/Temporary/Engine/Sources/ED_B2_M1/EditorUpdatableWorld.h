#if !defined(__EDITOR_UPDATATABLE_WORLD__)
#define __EDITOR_UPDATATABLE_WORLD__
#pragma once

#include "../B2_M1_World/UpdatableWorld.h"

class CEditorUpdatableWorld : public CUpdatableWorld
{
	OBJECT_NOCOPY_METHODS( CEditorUpdatableWorld );

public:
	CEditorUpdatableWorld() { bEditor = true; }
	~CEditorUpdatableWorld() {}

	void PlayerObjectiveChanged( const int nObjective, const enum EMissionObjectiveState eState ) {}

	void Select( CMapObj *pMapObj ) {}
	void DeSelect( CMapObj *pMapObj ) {}
	void DeSelectDead( CMapObj *pMapObj ) {}
	void RemoveFromSelectionGroup( CMapObj *pMopObj ) {}
	bool IsActive( CMapObj *pMopObj ) { return false; }
	bool IsSuperActive( CMapObj *pMopObj ) { return false; }
	void DoUpdateSpecialAbility( CMapObj *pMO ) {}
	void DoUpdateObjectStats( CMapObj *pMO ) {}
	virtual void OnUpdateDiplomacy( CMapObj *pMO, const int nNewPlayer ) {}

	virtual void ProcessEditorUpdate( struct SAIBasicUpdate *pEditorUpdate ) { PerformUpdate( pEditorUpdate ); }
};
//
namespace NCreateUpdate
{
	// map editor update
	CObjectBase* InsertObject( int nObjectID, int nTypeID, int nRecordID, const CVec3 &rvPlacement, const CQuat &rRotation, float fHP, int nPlayer, int nDiplInfo, int nFrameIndex );
	CObjectBase* RemoveObject( int nObjectID );
	CObjectBase* MoveObject( int nObjectID, const CVec3 &rvPlacement, const CQuat &rRotation );
};

#endif // !defined(__EDITOR_UPDATATABLE_WORLD__)

