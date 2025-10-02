#include "StdAfx.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "misc/2darray.h"
#include "stats_b2_m1/iconsset.h"
#include "mapeditorlib/commoneditormethods.h"
#include "sceneb2/scene.h"
#include <float.h>
#include "ResourceDefines.h"
#include "libdb/ResourceManager.h"
#include "EditorMethods.h"
#include "MapObjectMultiState.h"
#include "EntrenchmentState.h"
#include "MapInfoEditor.h"

#include <zconf.h>

void CEntrenchmentState::ClearData()
{
	designTool.Clear();
}


bool CEntrenchmentState::CanBuildEntrenchment()
{
	return ( selectedEntrenchmentInfo.pEntrenchmentRPGStats != 0 );
}


void CEntrenchmentState::InsertObjectEnter()
{
	ClearData();
	RefreshSelectedEntrenchmentInfo();
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_ENTRENCHMENTS_STATE, this );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CEntrenchmentState::InsertObjectLeave()
{
	ClearData();
	Singleton<ICommandHandlerContainer>()->Remove( CHID_ENTRENCHMENTS_STATE );
}


void CEntrenchmentState::InsertObjectDraw( CPaintDC *pPaintDC )
{
	if ( !designTool.Draw( &sceneDrawTool ) )
	{
		CVec3 vPlacementPosition = pStoreInputState->lastEventInfo.vTerrainPos;
		vPlacementPosition.z = GetTerrainHeight( vPlacementPosition.x, vPlacementPosition.y );
		CVec3 vPlacementSelectorPosition = vPlacementPosition;
		sceneDrawTool.DrawCircle( vPlacementSelectorPosition, NMapInfoEditor::PLACEMENT_RADIUS0, NMapInfoEditor::PLACEMENT_PARTS, NMapInfoEditor::PLACEMENT_COLOR, false );
		sceneDrawTool.DrawCircle( vPlacementSelectorPosition, NMapInfoEditor::PLACEMENT_RADIUS1, NMapInfoEditor::PLACEMENT_PARTS, NMapInfoEditor::PLACEMENT_COLOR, false );
	}
}


bool CEntrenchmentState::InsertObjectMouseMove( UINT nFlags, const CVec3 &rTerrainPos )
{
	designTool.ProcessMovePoint( rTerrainPos ); 
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	return false;
}


bool CEntrenchmentState::InsertObjectLButtonDown( UINT nFlags, const CVec3 &rTerrainPos )
{
	bool bResult = designTool.ProcessLClickPoint( rTerrainPos ); 
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	return bResult;
}


bool CEntrenchmentState::InsertObjectRButtonUp( UINT nFlags, const CVec3 &rTerrainPos )
{
	bool bResult = designTool.ProcessRClickPoint( rTerrainPos ); 
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	return bResult;
}


bool CEntrenchmentState::InsertObjectLButtonDblClk( UINT nFlags, const CVec3 &rTerrainPos )
{
	designTool.Complete();
	InsertEntrenchment();
	return false;
}


bool CEntrenchmentState::InsertObjectKeyDown( UINT nChar, UINT nFlags, const CVec3 &rTerrainPos )
{
	if ( nChar == VK_ESCAPE )
	{
		return designTool.ProcessEscape();
	}
	else if ( nChar == VK_RETURN )
	{
		designTool.Complete();
		InsertEntrenchment();
	}
	return false;
}


void CEntrenchmentState::InsertEntrenchment()
{
	if ( CanEdit() )
	{
		CMapObjectMultiState::SEditParameters *pEditParameters = GetParentState()->GetEditParameters();
		CWaitCursor wcur;
		CPtr<IEditorScene> pScene = EditorScene();
		CPtr<ICamera> pCamera = Camera();
		CPtr<IResourceManager> pResourceManager = Singleton<IResourceManager>();
		CPtr<IManipulator> pManipulator = GetMapInfoEditor()->GetViewManipulator();

		if ( ( pScene == 0 ) || ( pCamera == 0 ) || ( pResourceManager == 0 ) || ( pManipulator == 0 ) )
		{
			return;
		}
		if ( !CanBuildEntrenchment() )
		{
			return;
		}
		//
		bool bResult = true;
		//
		pEditParameters->nFlags = MIMOSEP_PLAYER_INDEX;
		GetEditParameters( pEditParameters, CHID_MAPINFO_MAPOBJECT_WINDOW );
		//
		NMapInfoEditor::SEntrenchmentCreateInfo createInfo;
		createInfo.szRPGStatsTypeName = selectedEntrenchmentInfo.szRPGStatsTypeName;
		createInfo.rpgStatsDBID = selectedEntrenchmentInfo.rpgStatsDBID;
		createInfo.vPosition = VNULL3;
		createInfo.fDirection = 0.0f;
		createInfo.nPlayer = pEditParameters->nPlayerIndex;
		createInfo.fHP = 1.0f;
		createInfo.szRPGStatsTypeName = "EntrenchmentRPGStats";
		designTool.GetSegmentsInfo( &createInfo.segmentsInfo );
		if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
		{
			UINT nEntrenchmentInfoID = INVALID_NODE_ID;
			if ( NMapInfoEditor::SEntrenchmentInfo *pEntrenchmentInfo = 
				GetMapInfoEditor()->objectInfoCollector.Insert( static_cast<NMapInfoEditor::SEntrenchmentInfo*>( 0 ), &nEntrenchmentInfoID ) )
			{
				if ( pEntrenchmentInfo->Create( &createInfo, pScene, pObjectController, pManipulator ) )
				{
					pObjectController->Redo( false, true, GetMapInfoEditor() );
					Singleton<IControllerContainer>()->Add( pObjectController );
				}
				else
				{
					pObjectController->Undo( true, false, GetMapInfoEditor() );
				}
				//pEntrenchmentInfo->Trace();
			}
		}
		//
		ClearData();
		RefreshSelectedEntrenchmentInfo();
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


bool CEntrenchmentState::HandleCommand( UINT nCommandID, DWORD dwData )
{
	if ( CMapObjectState::HandleCommand( nCommandID, dwData ) )
	{
		return true;
	}
	//
	/*
	switch( nCommandID ) 
	{
	default:
		{
			return false;
		}
	}
	*/
	return false;
}


bool CEntrenchmentState::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CEntrenchmentState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CEntrenchmentState::UpdateCommand(), pbCheck == 0" );
	//
	if ( CMapObjectState::UpdateCommand( nCommandID, pbEnable, pbCheck ) )
	{
		return true;
	}
	//
	/*
	switch( nCommandID ) 
	{
	default:
		{
			return false;
		}
	}
	*/
	return false;
}


void CEntrenchmentState::RefreshSelectedEntrenchmentInfo()
{
	CWaitCursor waitCursor;
	selectedEntrenchmentInfo = SSelectedEntrenchmentInfo();
	//
	SObjectSet objectSet;
	if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, 
		ID_OS_GET_OBJECTSET, reinterpret_cast<DWORD>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
	{
		bool bRes = ( objectSet.szObjectTypeName == "EntrenchmentRPGStats" ) && ( !objectSet.objectNameSet.empty() );
		if ( !bRes ) 
			return;
		selectedEntrenchmentInfo.szRPGStatsTypeName = objectSet.szObjectTypeName;
		selectedEntrenchmentInfo.rpgStatsDBID = objectSet.objectNameSet.begin()->first;
		selectedEntrenchmentInfo.pEntrenchmentRPGStats= dynamic_cast<const NDb::SEntrenchmentRPGStats*>( NDb::GetObject( selectedEntrenchmentInfo.rpgStatsDBID ) );
		if ( selectedEntrenchmentInfo.pEntrenchmentRPGStats == 0 )
		{
			return;
		}
		for ( int i = 0; i < selectedEntrenchmentInfo.pEntrenchmentRPGStats->segments.size(); ++i )
		{
			const NDb::SEntrenchmentRPGStats::SEntrenchSegmentRPGStats &seg = 
				selectedEntrenchmentInfo.pEntrenchmentRPGStats->segments[i];
			CVec3 vSize = 2.0f * seg.vAABBHalfSize;
			CVec2 vCenter = seg.vAABBCenter;

			designTool.SetSegAABBSize( seg.eType, vSize );
			designTool.SetSegAABBCenter( seg.eType, vCenter );
		}
	}
}





//
//
//
//										ENTRENCHMENT DESIGN TOOL
//
//

const DWORD CEntrenchmentDesignTool::segColors[CEntrenchmentDesignTool::ST_COUNT] = 
{
	NMapInfoEditor::PLACEMENT_COLOR,
	NMapInfoEditor::PLACEMENT_COLOR,
	0xFF00FF00,
	0xFF00FF00
};

CEntrenchmentDesignTool::CEntrenchmentDesignTool()
{
	Clear();
}


void CEntrenchmentDesignTool::Clear()
{ 
	// половина острого угла в модели Arc (15градусов)
	const float F_ARC_ANGLE = (15.0f * FP_2PI)/360.0f;
	fArcDeltaAngle = F_ARC_ANGLE;
	segments.clear();
	bComplete = false;
	nLock = 0;
	vStartPos = VNULL3;
	fStartDir = 0;
	vDirMarker = VNULL3;
	memset( segAABBSizes, 0, sizeof(CVec3)*ST_COUNT );
}


bool CEntrenchmentDesignTool::Draw( CSceneDrawTool *pDrawTool )
{
	if ( segments.empty() )
	{
		return false;
	}
	for ( int i = 0; i < segments.size(); ++i )
	{
		CVec3 dir = GetVecDir( GetSegDir( i ) );
		CVec3 p0 = GetSegPos( i, true );
		float fSegLen = GetSegAABBSize( segments[i] ).x;
		CVec3 p1 = p0 + dir * fSegLen;

		DrawSeg( pDrawTool, p0, p1, segColors[segments[i]] );
	}
	//
	if ( !IsComplete() )
	{
		float fSegLen = GetSegAABBSize( segments[nLock] ).x;
		CVec3 vLockDir = GetVecDir( GetSegDir( nLock ) );
		CVec3 vLockMarkerStart = GetSegPos( nLock, true ) + fSegLen * vLockDir;
		UpdateTerrainHeight( &vLockMarkerStart );
		{
			CVec3 vLockMarkerFinish = vLockMarkerStart + vLockDir * NMapInfoEditor::PLACEMENT_DIRECTION_RADIUS;
			UpdateTerrainHeight( &vLockMarkerFinish );
			pDrawTool->DrawLine( vLockMarkerStart, vLockMarkerFinish, NMapInfoEditor::PLACEMENT_COLOR, false );
		}
		{
			CVec3 vLockMarkerFinish = vLockMarkerStart + vDirMarker * NMapInfoEditor::PLACEMENT_DIRECTION_RADIUS;
			UpdateTerrainHeight( &vLockMarkerFinish );
			pDrawTool->DrawLine( vLockMarkerStart, vLockMarkerFinish, NMapInfoEditor::PLACEMENT_COLOR, false );
		}
	}
	return true;
}


void CEntrenchmentDesignTool::DrawSeg( CSceneDrawTool *pDrawTool, const CVec3 &cp, const CVec3 &dp, DWORD clr )
{
	CVec3 vNormale = GetNormal( dp - cp );
	Normalize( &vNormale );
	vNormale *= AI_TILE_SIZE / 2.0f;
	//
	CVec3 v0 = cp - vNormale;
	CVec3 v1 = cp + vNormale;
	CVec3 v2 = dp + vNormale;
	CVec3 v3 = dp - vNormale;
	UpdateTerrainHeight( &v0 );
	UpdateTerrainHeight( &v1 );
	UpdateTerrainHeight( &v2 );
	UpdateTerrainHeight( &v3 );
	//
	list<CVec3> pointList;
	pointList.push_back( v0 );
	pointList.push_back( v1 );
	pointList.push_back( v2 );
	pointList.push_back( v3 );
	//
	pDrawTool->DrawPolyline( pointList, clr, true, false );
}


float CEntrenchmentDesignTool::GetSegDir( int nSegIndex )
{
	if ( nSegIndex < 0 || nSegIndex >= segments.size() )
		return 0;

	float fAngle = fStartDir;

	if ( nSegIndex == 0 && segments[nSegIndex] == ST_TERMINATOR )
	{
		// начальный терминатор
		return fAngle;
	}

	ESegType ePrevSegType = ST_TERMINATOR; // начальный терминатор

	for ( int i = 1; i <= nSegIndex; ++i )
	{
		// при переходе от линейного сегмента к поворотному направление меняется на 15 градусов
		// при последовательных поворотных - на 30
		// и т.д.
		ESegType eSegType = segments[i];
		switch ( eSegType )
		{
		case ST_TERMINATOR:
		case ST_LINE:
			{
				switch ( ePrevSegType )
				{
				case ST_LEFT_ARC:
					fAngle += fArcDeltaAngle;
					break;
				case ST_RIGHT_ARC:
					fAngle -= fArcDeltaAngle;
					break;
				}
			}
			break;
		case ST_LEFT_ARC:
			{
				switch ( ePrevSegType )
				{
				case ST_TERMINATOR:
					fAngle += fArcDeltaAngle;
					break;
				case ST_LINE:
					fAngle += fArcDeltaAngle;
					break;
				case ST_LEFT_ARC:
					fAngle += 2.0f * fArcDeltaAngle;
					break;
				}
			}
			break;
		case ST_RIGHT_ARC:
			{
				switch ( ePrevSegType )
				{
				case ST_TERMINATOR:
					fAngle -= fArcDeltaAngle;
					break;
				case ST_LINE:
					fAngle -= fArcDeltaAngle;
					break;
				case ST_RIGHT_ARC:
					fAngle -= 2.0f * fArcDeltaAngle;
					break;
				}
			}
			break;
		}

		ePrevSegType = eSegType;
	}

	return fAngle;	
}


CVec3 CEntrenchmentDesignTool::GetSegPos( int nSegIndex, bool bStart )
{
	// позиция начала сегмента

	if ( nSegIndex < 0 || nSegIndex >= segments.size() )
		return VNULL3;

	CVec3 cp = vStartPos;

	for ( int i = 0; i < nSegIndex; ++i )
	{
		float fSegLen = GetSegAABBSize( segments[i] ).x;
		cp += GetVecDir( GetSegDir( i ) ) * fSegLen;
	}
	if ( !bStart )
	{
		float fSegLen = GetSegAABBSize( segments[nSegIndex] ).x;
		cp += GetVecDir( GetSegDir( nSegIndex ) ) * fSegLen;
	}
	return cp;
}


bool CEntrenchmentDesignTool::ProcessLClickPoint( const CVec3 &p )
{
	if ( bComplete )
	{
		return true;
	}

	if ( segments.empty() )
	{
		segments.push_back( ST_TERMINATOR );
		vStartPos = p;
		fStartDir = 0;
		ProcessMovePoint( p );
		return false;
	}
	else
	{
		nLock = segments.size() - 1;
		ProcessMovePoint( p );
		return false;
	}
}


bool CEntrenchmentDesignTool::ProcessRClickPoint( const CVec3 &p )
{
	if ( bComplete )
	{
		return true;
	}
	if ( segments.empty() )
	{
		return true;
	}
	if ( nLock > 0 )
	{
		bool bNotLineFound = false;
		while ( ( nLock > 0 ) && ( nLock < segments.size() ) )
		{
			--nLock;
			if ( segments[nLock] == ST_LINE )
			{
				if ( bNotLineFound )
				{
					break;
				}
			}
			else
			{
				bNotLineFound = true;
			}
		}
		if ( nLock == 0 )
		{
			segments.clear();
			segments.push_back( ST_TERMINATOR );
		}
		ProcessMovePoint( p );
	}
	else
	{
		segments.clear();
	}
	return false;
}


bool CEntrenchmentDesignTool::ProcessEscape()
{
	if ( bComplete )
	{
		return true;
	}
	if ( segments.empty() )
	{
		return true;
	}
	nLock = 0;
	segments.clear();
	return false;
}


void CEntrenchmentDesignTool::ProcessMovePoint( const CVec3 &p )
{
	if ( bComplete  || segments.empty() )
	{
		return;
	}
	segments.resize( nLock + 1 );
	//
	CVec3 vLockPos = GetSegPos( nLock, nLock == 0 );
	CVec3 vLockDir = GetVecDir( GetSegDir( nLock ) );					// направление последнего блокированного сегмента
	CVec3 dir = p - vLockPos;							// направление от блока к курсору мыши	
	//
	if ( fabs( dir ) <= FLT_EPSILON )
	{
		if ( nLock == 0 )									// первый линейный сегмент вращается вокруг первого терминатора
		{
			segments.push_back( ST_LINE );
		}
		return;
	}
	Normalize( &dir );
	vDirMarker = dir;
	DebugTrace( "Angle: %g", atan2( dir.y, dir.x ) * 180 / FP_PI );

	if ( nLock == 0 )									// первый линейный сегмент вращается вокруг первого терминатора
	{
		fStartDir = atan2( dir.y, dir.x ); 
		vLockDir = dir;
		segments.push_back( ST_LINE );
	}

	float fDA = AngleDiff( dir, vLockDir );

	int n = fabs( fDA ) / ( 2.0f * fArcDeltaAngle );			// сколько arc-ов нужно добавить, чтобы (приблизительно)
	// выйти на текущее направление мыши
	float fSign = fDA < 0 ? -1.0f : 1.0f;
	for ( int i = 0; i < n; ++i )
	{
		segments.push_back( fSign < 0 ? ST_RIGHT_ARC : ST_LEFT_ARC );
	}
	//
	int nLastSeg = segments.size() - 1;
	CVec3 cp = GetSegPos( nLastSeg, true );
	CVec3 cd = GetVecDir( GetSegDir( nLastSeg ) );
	//
	CVec3 shift = p - cp;
	float fShLen = fabs( shift );
	Normalize( &shift );
	float fShDA = AngleDiff( shift, cd );
	fShLen *= cos( fShDA );
	float fSegLen = GetSegAABBSize( ST_LINE ).x;
	int m = fShLen / fSegLen;
	//
	for ( int i = 0; i < m; ++i )
	{
		segments.push_back( ST_LINE );
	}
	if ( ( n > 0 ) && ( m == 0 ) )
	{
		segments.push_back( ST_LINE );
	}
}


void CEntrenchmentDesignTool::Complete()
{
	if ( bComplete )
	{
		return;
	}
	if ( segments.empty() )
	{
		return;
	}
	segments.push_back( ST_TERMINATOR );
	bComplete = true;
}


bool CEntrenchmentDesignTool::IsComplete() 
{ 
	return bComplete; 
}


void CEntrenchmentDesignTool::GetSegmentsInfo( list<NMapInfoEditor::SEntrenchmentSegInfo> *pEntrenchmentElementsInfo )
{
	pEntrenchmentElementsInfo->clear();

	for ( int i = 0; i < segments.size(); ++i )
	{
		NMapInfoEditor::SEntrenchmentSegInfo sg;

		sg.vPos0 = GetSegPos( i, true );

		sg.fDirAngle = GetSegDir(i);

		CQuat q( sg.fDirAngle, V3_AXIS_Z );
		CVec3 dir = V3_AXIS_X;
		q.Rotate( &dir, dir );
		float fSegLen = GetSegAABBSize( segments[i] ).x;
		sg.vPos1 = sg.vPos0 + dir * fSegLen;

		if ( i == 0 )
		{
			sg.vPosCenter = sg.vPos1;
		}
		else if ( i == ( segments.size() - 1 ) )
		{
			sg.vPosCenter = sg.vPos0;
		}
		else
		{
			sg.vPosCenter = 0.5f * ( sg.vPos0 + sg.vPos1 );
		}

		sg.vAABBSize = GetSegAABBSize( segments[i] );

		CVec3 vAABBC = CVec3( GetSegAABBCenter( segments[i] ).x, GetSegAABBCenter( segments[i] ).y, 0 );
		q.Rotate( &vAABBC, vAABBC );
		sg.vAABBCenter = CVec2( vAABBC.x, vAABBC.y );

		switch ( segments[i] )
		{
		case ST_TERMINATOR:
			sg.eSegType = NDb::EST_TERMINATOR;
			if ( i == 0 )
			{
				// первый терминатор
				sg.fDirAngle += FP_PI;
				sg.vAABBCenter = -sg.vAABBCenter; 
			}
			break;
		case ST_LINE:
			sg.eSegType = NDb::EST_LINE;
			break;
		case ST_LEFT_ARC:
			sg.eSegType = NDb::EST_ARC;
			break;
		case ST_RIGHT_ARC:
			sg.eSegType = NDb::EST_ARC;
			sg.fDirAngle += FP_PI;	// ST_RIGHT_ARC и ST_LEFT_ARC - одна модель но развернутая на 180 град.
			break;
		}
		pEntrenchmentElementsInfo->push_back(sg);
	}
}


CVec3 CEntrenchmentDesignTool::GetSegAABBSize( ESegType eSegType )
{
	if ( eSegType < 0 || eSegType >= ST_COUNT )
		return VNULL3;

	// ! модели окопов ориентированы вдоль оси X 
	return segAABBSizes[eSegType]; 
}


CVec2 CEntrenchmentDesignTool::GetSegAABBCenter( ESegType eSegType )
{
	if ( eSegType < 0 || eSegType >= ST_COUNT )
		return VNULL2;

	return segAABBCentr[eSegType];
}


void CEntrenchmentDesignTool::SetSegAABBSize( NDb::EEntrenchSegmType eSegType, const CVec3 &v )
{
	switch ( eSegType )
	{
	case NDb::EST_LINE:
		segAABBSizes[ST_LINE] = v;
		break;
	case NDb::EST_ARC:
		segAABBSizes[ST_LEFT_ARC] = v;
		segAABBSizes[ST_RIGHT_ARC] = v;
		break;
	case NDb::EST_TERMINATOR: 
		segAABBSizes[ST_TERMINATOR] = v;
		break;
	}
}


void CEntrenchmentDesignTool::SetSegAABBCenter( NDb::EEntrenchSegmType eSegType, const CVec2 &v )
{
	switch ( eSegType )
	{
	case NDb::EST_LINE:
		segAABBCentr[ST_LINE] = v;
		break;
	case NDb::EST_ARC:
		segAABBCentr[ST_LEFT_ARC] = v;
		segAABBCentr[ST_RIGHT_ARC] = v;
		break;
	case NDb::EST_TERMINATOR: 
		segAABBCentr[ST_TERMINATOR] = v;
		break;
	}
}


