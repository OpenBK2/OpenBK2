#pragma once

#include "MapInfoEditor.h"
#include "ScriptCameraEditorData.h"
#include "MoviesEditorData.h"

#include <cstdint>

//
//
//		SCRIPT CAMERA STATE
//
//

class CScriptCameraState : public CDefaultInputState, public ICommandHandler
{
	struct SCameraMarker
	{
		int nCameraID;
		int nMarkerID;
	};

	CMapInfoEditor *pMapInfoEditor;
	CMapInfoStoreInputState *pStoreInputState;
	CSceneDrawTool sceneDrawTool;
  
	std::vector<SCameraMarker> cameraMarkers;

	SScriptCameraWindowData dialogData;
	SScriptMovieEditorData moviesData;
	SScriptCameraRunDlgData runDialogData;
	float fYaw, fPitch;
	int nFOV;
	bool bDrawMarkers;

	void RefreshScriptCameraMarkers();
	void DeleteScriptCameraMarkers();

	void SetCameraPlacement( const NCamera::CCameraPlacement &camera );
	CMapInfoEditor* GetMapInfoEditor() { return pMapInfoEditor; }

	void SetMovie( int nMovieIndex );
	int SelectCameraByMovieParams( int nMovieID, float fTime ) const;

	void FixDBMoviesIndexes( int nDeletedKey );

protected:
	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	// IInputState interface
	virtual void Enter();
	virtual void Leave();
	virtual void Draw( class CPaintDC *pPaintDC );
	virtual void PostDraw( class CPaintDC *pPaintDC );
	void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
	void OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint );

	bool IsCameraPlacementInDB( int nCamera );
	void GetCameraPlacementByID( NCamera::CCameraPlacement *pCamera, int nID );
	void GetDBData();

	bool UpdateScriptPlacement( int nCamera );
	bool DeleteScriptPlacement( int nCamera );
	bool AddScriptPlacement();
	void ScriptCameraRun( NDb::EScriptCameraRunType eRunType );

	// new movies engine
	bool AddPosKey( float fTime, int nSeqIndex );
	bool DeleteKeys( const CArray1Bit &delList, int nSeqIndex, bool bDeleteWholeSequence );
	bool MoveKeys( const CArray1Bit &moveList, float fMoveValue, int nSeqIndex );
	bool AddSequence();
	bool DeleteSequence( int nSeqIndex );
	bool SetSequenceLength( float fNewLength, int nSeqIndex );
	bool KeySetup( const CArray1Bit &actKeys, int nSeqIndex );

	void ChangeCurrentFOV( float fFOVdelta );

	void RefreshDialogData( bool bNeedUpdateFromDB );

public:
	CScriptCameraState( CMapInfoEditor *_pMapInfoEditor = 0 );
	virtual ~CScriptCameraState() {};
};


