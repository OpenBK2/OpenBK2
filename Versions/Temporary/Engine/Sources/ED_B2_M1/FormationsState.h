#pragma once

#include "DialogData.h"
#include "MapEditorLib/DefaultInputState.h"
#include "MapEditorLib/Interface_CommandHandler.h"

#include <cstdint>

//
//
//			FORMATIONS STATE
//
//

class CSquadEditor;
class CMaskManipulator;

class CFormationsState : public CDefaultInputState, public ICommandHandler
{
	CSquadEditor *pSquadEditor;
	CPtr<CMaskManipulator> pMaskManipulator;

	SFormationWindowDialogData currDialogData;

	std::list<int> pickObjects;

	void SetMaskManipulator( NDb::SSquadRPGStats::SFormation::EFormationMoveType eSelectedFormation );
	void ClearMaskManipulator();

	//IInputState interface
	void Enter() override;
	void Leave() override;

	void Draw( IPaintContext *pPaintDC ) override {}

	void PostDraw( IPaintContext *pPaintDC ) override;
	void OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData ) override;
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck ) override;

	// CFormationsState
	bool GetFormationWindowDialogData( SFormationWindowDialogData *pData );
	bool SetFormationWindowDialogData( SFormationWindowDialogData *pData );
	void ReloadSquad( NDb::SSquadRPGStats::SFormation::EFormationMoveType eFormation );

public:
	CFormationsState( CSquadEditor *_pSquadEditor = 0 );

	~CFormationsState() override {}

	int LoadModel( const NDb::SModel *pModel, const CVec3 &rvPosition );
	void RefreshState( bool bForce );
};
