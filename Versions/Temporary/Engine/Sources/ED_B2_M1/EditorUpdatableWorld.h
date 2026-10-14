#pragma once

#include "B2_M1_World/UpdatableWorld.h"

class CEditorUpdatableWorld : public CUpdatableWorld
{
	OBJECT_NOCOPY_METHODS( CEditorUpdatableWorld );

public:
	CEditorUpdatableWorld()
	{
		bEditor = true;
	}

	~CEditorUpdatableWorld() override {}

	void PlayerObjectiveChanged( const int nObjective, const enum EMissionObjectiveState eState ) override {}

	void Select( CMapObj *pMapObj ) override {}

	void DeSelect( CMapObj *pMapObj ) override {}

	void DeSelectDead( CMapObj *pMapObj ) override {}

	void RemoveFromSelectionGroup( CMapObj *pMopObj ) override {}

	bool IsActive( CMapObj *pMopObj ) override
	{
		return false;
	}

	bool IsSuperActive( CMapObj *pMopObj ) override
	{
		return false;
	}

	void DoUpdateSpecialAbility( CMapObj *pMO ) override {}

	void DoUpdateObjectStats( CMapObj *pMO ) override {}

	void OnUpdateDiplomacy( CMapObj *pMO, const int nNewPlayer ) override {}

	virtual void ProcessEditorUpdate( struct SAIBasicUpdate *pEditorUpdate )
	{
		PerformUpdate( pEditorUpdate );
	}
};

//
namespace NCreateUpdate
{
// map editor update
CObjectBase *InsertObject( int nObjectID,
                           int nTypeID,
                           int nRecordID,
                           const CVec3 &rvPlacement,
                           const CQuat &rRotation,
                           float fHP,
                           int nPlayer,
                           int nDiplInfo,
                           int nFrameIndex );
CObjectBase *RemoveObject( int nObjectID );
CObjectBase *MoveObject( int nObjectID, const CVec3 &rvPlacement, const CQuat &rRotation );
}; // namespace NCreateUpdate
