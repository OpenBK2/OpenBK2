#pragma once

#include "SimpleObjectState.h"
#include "EntrenchmentInfoData.h"

//
//
//
//										ENTRENCHMENT DESIGN TOOL
//
//

class CEntrenchmentDesignTool
{
public:
	enum ESegType 
	{
		ST_TERMINATOR	= 0,	// терминатор
		ST_LINE				= 1,	// линейный сегмент
		ST_LEFT_ARC		= 2,	// поворот налево
		ST_RIGHT_ARC	= 3,	// поворот направо
		ST_COUNT			= 4,	// count
	};
	///
private:
	///
	static const DWORD segColors[ST_COUNT];

	float fArcDeltaAngle;									// половина острого угла конуса в модели Arc
	CVec3 segAABBSizes[ST_COUNT];					// размеры соответствующих типов сегментов
	CVec2 segAABBCentr[ST_COUNT];					// центры соответствующих типов сегментов 
	///
	bool bComplete;										// окоп завершен (оба терминатора поставлены)
	///
	CVec3 vStartPos;									// позиция стартового терминатора
	float fStartDir;									// направление стартового терминатора 	
	vector<ESegType> segments;				// типы сегментов составляющие окоп
	int nLock;												// блок (все сегменты с индексом <= блока считаются нередактируемыми)
	///
	CVec3 vDirMarker;									// вектор напраления от блока к курсору мыши
	///
	void DrawSeg( CSceneDrawTool *pDrawTool, const CVec3 &cp, const CVec3 &dp, DWORD clr );
	CVec3 GetSegPos( int nSegIndex, bool bStart );
	float GetSegDir( int nSegIndex );
	///
public:
	///
	CEntrenchmentDesignTool();
	///
	void Clear();
	bool Draw( CSceneDrawTool *pDrawTool );
	///
	bool ProcessLClickPoint( const CVec3 &p );
	bool ProcessRClickPoint( const CVec3 &p );
	bool ProcessEscape();
	void ProcessMovePoint( const CVec3 &p );
	void Complete();
	bool IsComplete();
	///
	void GetSegmentsInfo( list<NMapInfoEditor::SEntrenchmentSegInfo> *pEntrenchmentElementsInfo );
	///
	void SetSegAABBSize( NDb::EEntrenchSegmType eSegType, const CVec3 &v );
	void SetSegAABBCenter( NDb::EEntrenchSegmType eSegType, const CVec2 &v );
	CVec3 GetSegAABBSize( ESegType eSegType );
	CVec2 GetSegAABBCenter( ESegType eSegType );
	///
};

//
//
//
//						ENTRENCHMENT STATE 
//
//
//

class CEntrenchmentState : public CMapObjectState
{
	friend class CMultiInputState;
	friend class CMapInfoState;
	friend class CMapObjectMultiState;
	///
	CEntrenchmentDesignTool	designTool;
	///
	/// выбранный для постановки окоп
	struct SSelectedEntrenchmentInfo
	{ 
		string szRPGStatsTypeName;
		CDBID rpgStatsDBID;
		const NDb::SEntrenchmentRPGStats *pEntrenchmentRPGStats;
		//
		SSelectedEntrenchmentInfo() : pEntrenchmentRPGStats( 0 ) {}
	};	
	SSelectedEntrenchmentInfo selectedEntrenchmentInfo;
	void RefreshSelectedEntrenchmentInfo();
	///
	void ClearData();
	//
	bool CanBuildEntrenchment();
	void InsertEntrenchment();
	///
	CEntrenchmentState::CEntrenchmentState( CMapObjectMultiState* _pParentState = 0 ) : CMapObjectState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, StrFmt( "CEntrenchmentState(): pParentState == 0" ) );
		ClearData();
	}
protected:
	///
	bool IsDrawSceneDrawTool() { return true; }
	///
	void InsertObjectEnter();
	void InsertObjectLeave();
	void InsertObjectDraw( class CPaintDC *pPaintDC );
	///
	bool InsertObjectMouseMove( UINT nFlags, const CVec3 &rTerrainPos );
	bool InsertObjectLButtonDown( UINT nFlags, const CVec3 &rTerrainPos );
	bool InsertObjectRButtonUp( UINT nFlags, const CVec3 &rTerrainPos );
	bool InsertObjectKeyDown( UINT nChar, UINT nFlags, const CVec3 &rTerrainPos );
	bool InsertObjectLButtonDblClk	( UINT nFlags, const CVec3 &rTerrainPos );
	///
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
	///
};


