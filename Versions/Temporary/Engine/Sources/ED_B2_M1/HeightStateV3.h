#if !defined(__HEIGHT_STATE_V3__)
#define __HEIGHT_STATE_V3__
#pragma once

#include "CommandHandlerDefines.h"
#include "..\MapEditorLib\MultiInputState.h"
#include "..\MapEditorLib\Interface_CommandHandler.h"
#include "MapInfoStoreInputState.h"
#include "HeightPattern.h"
#include "Tools_SceneDraw.h"

//Mapinfo terrain height state v3 edit parameters
#define MITHV3EP_BRUSH								0x00000001
#define MITHV3EP_BRUSH_TYPE						0x00000002
#define MITHV3EP_BRUSH_SIZE						0x00000004
#define MITHV3EP_LEVEL_TO							0x00000008
#define MITHV3EP_UPDATE_HEIGHT				0x00000010
#define MITHV3EP_TILE_COUNT						0x00000020
#define MITHV3EP_TILE_INDEX						0x00000040
#define MITHV3EP_THUMBNAILS						0x00000080
#define MITHV3EP_ALL									0xFFFFFFFF

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CHeightTileStateV3 : public CDefaultInputState
{
	friend class CMultiInputState;
	friend class CHeightStateV3;
	
	class CHeightStateV3* pParentState;
	
	CHeightTileStateV3( CHeightStateV3* _pParentState = 0 ) : pParentState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, "CHeightUpStateV3(): Invalid parameter: pParentState == 0" );
	}

	//IInputState interface
	void OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint );
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CHeightUpStateV3 : public CDefaultInputState
{
	friend class CMultiInputState;
	friend class CHeightStateV3;
	
	class CHeightStateV3* pParentState;
	
	CHeightUpStateV3( CHeightStateV3* _pParentState = 0 ) : pParentState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, "CHeightUpStateV3(): Invalid parameter: pParentState == 0" );
	}

	//IInputState interface
	void OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMouseMove	( UINT nFlags, const CTPoint<int> &rMousePoint );
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CHeightDownStateV3 : public CDefaultInputState
{
	friend class CMultiInputState;
	friend class CHeightStateV3;
	
	class CHeightStateV3* pParentState;
	
	CHeightDownStateV3( CHeightStateV3* _pParentState = 0 ) : pParentState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, "CHeightDownStateV3(): Invalid parameter: pParentState == 0" );
	}

	//IInputState interface
	void OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMouseMove	( UINT nFlags, const CTPoint<int> &rMousePoint );
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CHeightRoundStateV3 : public CDefaultInputState
{
	friend class CMultiInputState;
	friend class CHeightStateV3;
	
	class CHeightStateV3* pParentState;
	
	CHeightRoundStateV3( CHeightStateV3* _pParentState = 0 ) : pParentState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, "CHeightRoundStateV3(): Invalid parameter: pParentState == 0" );
	}

	//IInputState interface
	void OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMouseMove	( UINT nFlags, const CTPoint<int> &rMousePoint );
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CHeightPlatoStateV3 : public CDefaultInputState
{
	friend class CMultiInputState;
	friend class CHeightStateV3;
	
	class CHeightStateV3* pParentState;
	
	CHeightPlatoStateV3( CHeightStateV3* _pParentState = 0 ) : pParentState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, "CHeightPlatoStateV3(): Invalid parameter: pParentState == 0" );
	}

	//IInputState interface
	void OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMouseMove	( UINT nFlags, const CTPoint<int> &rMousePoint );
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CHeightStateV3 : public CMultiInputState, public ICommandHandler
{
	friend class CMultiInputState;
	friend class CMapInfoState;
	friend class CHeightWindowV3;
	friend class CHeightTileStateV3;
	friend class CHeightUpStateV3;
	friend class CHeightDownStateV3;
	friend class CHeightRoundStateV3;
	friend class CHeightPlatoStateV3;

	static const int		BRUSH_MIN_SIZE;
	static const int		BRUSH_SIZE_STEP;
	static const float	MAX_HEIGHT;
	static const float	HEIGHT_SPEED;
	static const float	ROUND_RATIO;
	static const float	PLATO_RATIO;
	static const DWORD	BRUSH_COLOR;
	static const int		BRUSH_PARTS;
	static const int		HEIGHT_BRUSH_SIZE[5];
	static const int		TILE_BRUSH_SIZE[5];

public:
	struct SEditParameters
	{
		enum EBrush
		{
			B_TILE		= 0,
			B_UP			= 1,
			B_DOWN		= 2,
			B_ROUND		= 3,
			B_PLATO		= 4,
		};
		enum EBrushSize
		{
			BS_0			= 0,
			BS_1			= 1,
			BS_2			= 2,
			BS_3			= 3,
			BS_4			= 4,
		};
		enum EBrushType
		{
			BT_CIRCLE	= 0,
			BT_SQUARE	= 1,
		};
		UINT nFlags;
		//
		EBrush eBrush;
		EBrushSize eBrushSize;
		EBrushType eBrushType;
		float fLevelTo;
		bool bUpdateHeight;
		vector<string> tileList;
		int nTileIndex;
		bool bThumbnails;
		vector<int> tileBrushSizeList;
		vector<int> heightBrushSizeList;

		SEditParameters() 
			: nFlags( MITHV3EP_ALL ),
				eBrush( B_TILE ),
				eBrushSize( BS_1 ),
				eBrushType( BT_CIRCLE ),
				fLevelTo( 0.0f ),
				bUpdateHeight( true ),
				nTileIndex( -1 ),
				bThumbnails( false ) {}

		int operator&( IXmlSaver &xs );
	};

private:
	CMapInfoStoreInputState *pStoreInputState;
	class CMapInfoEditor *pMapInfoEditor;
	CSceneDrawTool sceneDrawTool;

	// внутренние переменные
	CTPoint<int> heightDiffPos;
	CArray2D<float> heightDiff;
	SGradient gradient;
	SHeightPattern positivePattern;
	SHeightPattern negativePattern;
	SHeightPattern levelPattern;
	bool bEscaped;

	// внутренние переменные
	CVec3 vTileDiffPos;
	CArray2D<BYTE> tileDiff;
	CArray2D<BYTE> tileBrush;

	bool bTileEditStarted;
	bool bHeightEditStarted;

	SEditParameters* GetEditParameters();
	bool CanEdit();
	void UpdatePlatoHeight();
	void GetEditParameters( UINT nFlags ); // editParameters -> editorSettings
	void SetEditParameters( UINT nFlags ); // editorSettings -> editParameters
	void CreateMapInfoController();

	int GetTileBrushSize( SEditParameters::EBrushSize eBrushSize );
	int GetHeightBrushSize( SEditParameters::EBrushSize eBrushSize );
	//
	void SetCornerTile( SHeightPattern *pPattern, const CTPoint<int> &rPatternPos );
	void ProcessTerrain( SEditParameters::EBrush eBrush );
	//
	void UpdateCommonPatterns();
	void UpdateLevelPattern( const CTPoint<int> &rPatternPos, SEditParameters::EBrush eBrush );
	
	void CreateTileBrush( CArray2D<BYTE> *pBrush, int nSize, int nTileIndex, bool bCircle );
	void CreatePatternAndModifyGeometry( const SHeightPattern &rPattern );
protected:
	//IInputState interface
	void Enter();
	void Leave();
	void Draw( class CPaintDC *pPaintDC );
	//
	void OnMouseMove	( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp	( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonUp	( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMButtonUp	( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnKeyDown		( UINT nChar, UINT nRepCnt, UINT nFlags );
	void OnKeyUp			( UINT nChar, UINT nRepCnt, UINT nFlags );

	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

public:
	CHeightStateV3( CMapInfoEditor* _pMapInfoEditor = 0 ) : pMapInfoEditor( _pMapInfoEditor ), bEscaped( false ), bTileEditStarted( false ), bHeightEditStarted( false )
	{
		NI_ASSERT( pMapInfoEditor != 0, "CHeightState(): Invalid parameter: pMapInfoEditor == 0" );
		Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_TERRAIN_HEIGHT_STATE_V3, this );
		//
		pStoreInputState = new CMapInfoStoreInputState();
		NI_ASSERT( pStoreInputState != 0, StrFmt( "CHeightState(): pStoreInputState == 0" ) );

		int nStateIndex = INVALID_INPUT_STATE_INDEX;
		//	
		CHeightTileStateV3 *pHeightTileState = new CHeightTileStateV3( this );
		nStateIndex = AddInputState( pHeightTileState );
		NI_ASSERT( nStateIndex == SEditParameters::B_TILE, StrFmt( "CHeightState(): Wrong state number: %d (%d)", nStateIndex, SEditParameters::B_TILE ) );
		//
		CHeightUpStateV3 *pHeightUpState = new CHeightUpStateV3( this );
		nStateIndex = AddInputState( pHeightUpState );
		NI_ASSERT( nStateIndex == SEditParameters::B_UP, StrFmt( "CHeightState(): Wrong state number: %d (%d)", nStateIndex, SEditParameters::B_UP ) );
		//
		CHeightDownStateV3 *pHeightDownState = new CHeightDownStateV3( this );
		nStateIndex = AddInputState( pHeightDownState );
		NI_ASSERT( nStateIndex == SEditParameters::B_DOWN, StrFmt( "CHeightState(): Wrong state number: %d (%d)", nStateIndex, SEditParameters::B_DOWN ) );
		//
		CHeightRoundStateV3 *pHeightRoundState = new CHeightRoundStateV3( this );
		nStateIndex = AddInputState( pHeightRoundState );
		NI_ASSERT( nStateIndex == SEditParameters::B_ROUND, StrFmt( "CHeightState(): Wrong state number: %d (%d)", nStateIndex, SEditParameters::B_ROUND ) );
		//
		CHeightPlatoStateV3 *pHeightPlatoState = new CHeightPlatoStateV3( this );
		nStateIndex = AddInputState( pHeightPlatoState );
		NI_ASSERT( nStateIndex == SEditParameters::B_PLATO, StrFmt( "CHeightState(): Wrong state number: %d (%d)", nStateIndex, SEditParameters::B_PLATO ) );
		//
		SetActiveInputState( SEditParameters::B_UP, true, false );
	}
	//
	~CHeightStateV3()
	{
		if ( pStoreInputState )
		{
			delete pStoreInputState;
			pStoreInputState = 0;
		}
		//
		Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_TERRAIN_HEIGHT_STATE_V3 );
	}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // !defined(__HEIGHT_STATE_V3__)


