#if !defined(__FORMATIONS_STATE__)
#define __FORMATIONS_STATE__
#pragma once

#include "../MapEditorLib/Interface_CommandHandler.h"
#include "../MapEditorLib/DefaultInputState.h"
#include "DialogData.h"


//
//
//			FORMATIONS STATE
//
//

class CSquadEditor;
class CMaskManipulator;
class CFormationsState : public CDefaultInputState, public ICommandHandler
{
	CSquadEditor* pSquadEditor;
	CPtr<CMaskManipulator> pMaskManipulator;

	SFormationWindowDialogData currDialogData;

	list<int> pickObjects;

	void SetMaskManipulator( NDb::SSquadRPGStats::SFormation::EFormationMoveType eSelectedFormation );
	void ClearMaskManipulator();

	//IInputState interface
	void Enter();
	void Leave();
	void Draw( CPaintDC *pPaintDC ) {}

	void PostDraw( CPaintDC *pPaintDC );
	void OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint );
		
	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

	// CFormationsState 
	bool GetFormationWindowDialogData( SFormationWindowDialogData *pData );
	bool SetFormationWindowDialogData( SFormationWindowDialogData *pData );
	void ReloadSquad( NDb::SSquadRPGStats::SFormation::EFormationMoveType eFormation );

public:
	CFormationsState( CSquadEditor *_pSquadEditor = 0 );
	virtual ~CFormationsState() {}

	int LoadModel( const NDb::SModel *pModel, const CVec3 &rvPosition );
	void RefreshState( bool bForce );
};

#endif//#if !defined(__FORMATIONS_STATE__)


