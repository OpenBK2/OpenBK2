#include "stdafx.h"
/**

#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../stats_b2_m1/iconsset.h"
#include "../sceneb2/scene.h"
#include "../libdb/resourcemanager.h"
#include "../MapEditorLib/Interface_MainFrame.h"
#include "EnterNameDialog.h"
#include "EntrenchmentMnemonics.h"
#include "MapClip.h"

bool CMapClip::SaveMapClipToDB(  CObjectBaseController *pObjectController ) const
{
	if ( !pObjectController )
		return false;
	CPtr<IFolderCallback> pFcb = Singleton<IFolderCallback>();
	if ( !pFcb )
		return false;

	CEnterNameDialog enterNameDialog( Singleton<IMainFrameContainer>()->GetSECWorkbook(), "Clip name", "Clip name" );
	if ( enterNameDialog.DoModal() != IDOK )
		return false;
  string szClipName;
	enterNameDialog.GetName( &szClipName );
	if ( szClipName.empty() )
		return false;
	//
	if ( !pFcb->InsertObject( "MapClip", szClipName ) )
		return false;
	CPtr<IResourceManager> pRM = Singleton<IResourceManager>();
	if ( !pRM )
		return false;
	CPtr<IManipulator> pManipulator = pRM->CreateObjectManipulator( "MapClip", szClipName );
	if ( !pManipulator )
		return false;

	bool bResult = true;

	if ( !SavePasteRegionToDB(  pObjectController, pManipulator ) )
	{
		bResult = false;
		NI_ASSERT( bResult, "CMapClip::SaveMapClipToDB(): SavePasteRegionToDB" );
	}
	if ( !SaveHeightsToDB(  pObjectController, pManipulator ) )
	{
		bResult = false;
		NI_ASSERT( bResult, "CMapClip::SaveMapClipToDB(): SaveHeightsToDB" );
	}
  if ( !SaveTilesTypeToDB(  pObjectController, pManipulator ) )
	{
		bResult = false;
		NI_ASSERT( bResult, "CMapClip::SaveMapClipToDB(): SaveTilesTypeToDB" );
	}
	if ( !SaveVSOToDB(  pObjectController, pManipulator ) )
	{
		bResult = false;
		NI_ASSERT( bResult, "CMapClip::SaveMapClipToDB(): SaveVSOToDB" );
	}
	if ( !SaveEntrenchmentsToDB(  pObjectController, pManipulator ) )
	{
		bResult = false;
		NI_ASSERT( bResult, "CMapClip::SaveMapClipToDB(): SaveEntrenchmentsToDB" );
	}
  if ( !SaveBridgesToDB(  pObjectController, pManipulator ) )
	{
		bResult = false;
		NI_ASSERT( bResult, "CMapClip::SaveMapClipToDB(): SaveBridgesToDB" );
	}
	if ( !SaveSpotsToDB(  pObjectController, pManipulator ) )
	{
		bResult = false;
		NI_ASSERT( bResult, "CMapClip::SaveMapClipToDB(): SaveSpotsToDB" );
	}
	if ( !SaveObjectsToDB( pObjectController, pManipulator ) )
	{
		bResult = false;
		NI_ASSERT( bResult, "CMapClip::SaveMapClipToDB(): SaveObjectsToDB" );
	}
		return bResult;
}


bool CMapClip::LoadMapClipFromDB( int nMapClipID )
{
	Clear();

	CPtr<IResourceManager> pRM = Singleton<IResourceManager>();
	if ( !pRM )
		return false;

	CPtr<IManipulator> pManipulator = pRM->CreateObjectManipulator( "MapClip", nMapClipID );
	if ( !pManipulator )
		return false;

	bool bResult = true;

	if ( !LoadPasteRegionFromDB(  pManipulator ) )
	{
		bResult = false;
		NI_ASSERT( bResult, "CMapClip::LoadMapClipFromDB(): LoadPasteRegionFromDB" );
	}
	if ( !LoadHeightsFromDB(  pManipulator ) )
	{
		bResult = false;
		NI_ASSERT( bResult, "CMapClip::LoadMapClipFromDB(): LoadHeightsFromDB" );
	}
	if ( !LoadTilesTypeFromDB(  pManipulator ) )
	{
		bResult = false;
		NI_ASSERT( bResult, "CMapClip::LoadMapClipFromDB(): LoadTilesTypeFromDB" );
	}
  if ( !LoadVSOFromDB(  pManipulator ) )
	{
		bResult = false;
		NI_ASSERT( bResult, "CMapClip::LoadMapClipFromDB(): LoadVSOFromDB" );
	}
  if ( !LoadEntrenchmentsFromDB(  pManipulator ) )
	{
		bResult = false;
		NI_ASSERT( bResult, "CMapClip::LoadMapClipFromDB(): LoadEntrenchmentsFromDB" );
	}
	if ( !LoadBridgesFromDB(  pManipulator ) )
	{
		bResult = false;
		NI_ASSERT( bResult, "CMapClip::LoadMapClipFromDB(): LoadBridgesFromDB" );
	}
  if ( !LoadSpotsFromDB(  pManipulator ) )
	{
		bResult = false;
		NI_ASSERT( bResult, "CMapClip::LoadMapClipFromDB(): LoadSpotsFromDB" );
	}
  if ( !LoadObjectsFromDB( pManipulator ) )
	{
		bResult = false;
		NI_ASSERT( bResult, "CMapClip::LoadMapClipFromDB(): LoadObjectsFromDB" );
	}
	return bResult;
}


bool CMapClip::SavePasteRegionToDB(  CObjectBaseController *pObjectController, IManipulator *pManipulator ) const
{
	bool bResult = true;
	const vector<CVec3> &region =	GetRegion();
	bResult = bResult  && pObjectController->AddRemoveOperation( "PasteRegion", -1, pManipulator );
	for ( int i = 0; i < region.size(); ++i )
	{
		CVec2 v = CVec2( region[i].x, region[i].y );
		const string szObjectPrefix = StrFmt( "PasteRegion.[%d]", i );
		bResult = bResult && pObjectController->AddInsertOperation( "PasteRegion", NODE_ADD_INDEX, pManipulator );
		bResult = bResult && pObjectController->AddChangeVec2Operation<CVec2,float>( szObjectPrefix, v, pManipulator );
		if ( !bResult )
		{
			pObjectController->Undo( true, true, 0 );
			return false;
		}
	}
	return true;
}


bool CMapClip::LoadPasteRegionFromDB( IManipulator *pManipulator )
{
	int nPointsNum = 0;
	if ( !CManipulatorManager::GetValue( &nPointsNum, pManipulator, "PasteRegion" ) )
		return false;

	for ( int i = 0; i < nPointsNum; ++i )
	{
		CVec2 p; 
		string szDBA = StrFmt( "PasteRegion.[%d]", i );
		if ( !CManipulatorManager::GetVec2<CVec2,float>( &p, pManipulator, szDBA ) )
			return false;
		pasteRegion.push_back( CVec3( p.x, p.y, 0 ) );
	}
	return true;
}


bool CMapClip::SaveHeightsToDB(  CObjectBaseController *pObjectController, IManipulator *pManipulator ) const
{
	CArray2D<int> intHeights;
	intHeights.SetSizes( heights.GetSizeX(), heights.GetSizeY() );
	for ( int y = 0; y < heights.GetSizeY(); ++y )
	{
		for ( int x = 0; x < heights.GetSizeX(); ++x )
		{
			if ( heights[y][x] > -1e10 )
				intHeights[y][x] = ( heights[y][x] * 100000.0f );
			else
				intHeights[y][x] = 0x80000000;
		}
	}
  bool bResult = CManipulatorManager::Set2DArray( intHeights, pManipulator, "Heights" );
	return bResult;
}


bool CMapClip::LoadHeightsFromDB( IManipulator *pManipulator )
{
	CArray2D<int> intHeights;
	bool bResult = CManipulatorManager::Get2DArray( &intHeights, pManipulator, "Heights", 0 );
	if ( !bResult )
		return false;
	heights.SetSizes( intHeights.GetSizeX(), intHeights.GetSizeY() );
	for ( int y = 0; y < intHeights.GetSizeY(); ++y )
	{
		for ( int x = 0; x < intHeights.GetSizeX(); ++x )
		{
			if ( intHeights[y][x] != 0x80000000 )
			{
				heights[y][x] = ( intHeights[y][x] / 100000.0f );
			}
			else
			{
				heights[y][x] = -1e10;
			}
		}
	}
	return true;
}


bool CMapClip::SaveTilesTypeToDB(  CObjectBaseController *pObjectController, IManipulator *pManipulator ) const
{
  bool bResult = CManipulatorManager::Set2DArray( tilesType, pManipulator, "TileTypes" );
	return bResult;
}


bool CMapClip::LoadTilesTypeFromDB( IManipulator *pManipulator )
{
	tilesType.Clear();
	CArray2D<int> intTiles;
	bool bResult = CManipulatorManager::Get2DArray( &intTiles, pManipulator, "TileTypes", 0 );
	if ( !bResult )
		return false;
	tilesType.SetSizes( intTiles.GetSizeX(), intTiles.GetSizeY() );
	for ( int y = 0; y < intTiles.GetSizeY(); ++y )
	{
		for ( int x = 0; x < intTiles.GetSizeX(); ++x )
		{
			tilesType[y][x] = (char)intTiles[y][x];
		}
	}
	return true;
}


bool CMapClip::SaveVSOToDB(  CObjectBaseController *pObjectController, IManipulator *pManipulator ) const
{
	if ( !pObjectController || !pManipulator )
		return false;

	bool bResult = true;
	for ( int i = 0; i < vsoArray.size(); ++i )
	{
		const SClipboardVSO &vso = vsoArray[i];
		const NDb::SVSOInstance &instance = vso.vsoInstance;

		bResult = bResult && pObjectController->AddInsertOperation( "vsoArray", NODE_ADD_INDEX, pManipulator );

		string szDBA = StrFmt( "vsoArray.[%d]", i );
		bResult = bResult && pObjectController->AddChangeOperation( szDBA + ".Type", vso.szType, pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szDBA + ".TypeID", vso.nTypeID, pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szDBA + ".DescID", vso.nDescID, pManipulator );

		szDBA = StrFmt( "vsoArray.[%d].vsoInstance", i );
		string szDescriptor = StrFmt( "%s%c%c%d", vso.szType.c_str(), TYPE_SEPARATOR_CHAR, ID_PREFIX_CHAR, vso.nDescID );
		bResult = bResult && pObjectController->AddChangeOperation( szDBA + ".Descriptor", szDescriptor, pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szDBA + ".VSOID", -1, pManipulator );

		for ( int p = 0; p < instance.controlPoints.size(); ++p )
		{
			bResult = bResult && pObjectController->AddInsertOperation( szDBA + ".ControlPoints", NODE_ADD_INDEX, pManipulator );
			string szPointDBA = szDBA + StrFmt( ".ControlPoints.[%d]", p );
			bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3,float>( szPointDBA, instance.controlPoints[p], pManipulator );
		}

		for ( int p = 0; p < instance.points.size(); ++p )
		{
			bResult = bResult && pObjectController->AddInsertOperation( szDBA + ".points", NODE_ADD_INDEX, pManipulator );
			string szPointDBA = szDBA + StrFmt( ".points.[%d]", p );
			bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3,float>( szPointDBA + ".Pos", instance.points[p].vPos, pManipulator );
			bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3,float>( szPointDBA + ".Norm", instance.points[p].vNorm, pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szPointDBA + ".Width", instance.points[p].fWidth, pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szPointDBA + ".Opacity", instance.points[p].fOpacity, pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szPointDBA + ".KeyPoint", instance.points[p].bKeyPoint, pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szPointDBA + ".Radius", instance.points[p].fRadius, pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szPointDBA + ".Reserved", instance.points[p].fReserved, pManipulator );
		}
	}
	return bResult;
}


bool CMapClip::LoadVSOFromDB( IManipulator *pManipulator )
{
	if ( !pManipulator )
		return false;

	bool bResult = true;

	int nNumVSO = 0;
	bResult = bResult && CManipulatorManager::GetValue( &nNumVSO, pManipulator, "vsoArray" ); 	
	for ( int i = 0; i < nNumVSO; ++i )
	{
		SClipboardVSO vso;
		NDb::SVSOInstance &instance = vso.vsoInstance;

		string szDBA = StrFmt( "vsoArray.[%d]", i );
		bResult = bResult && CManipulatorManager::GetValue( &vso.szType, pManipulator, szDBA + ".Type" );
		bResult = bResult && CManipulatorManager::GetValue( &vso.nTypeID, pManipulator, szDBA + ".TypeID" );
		bResult = bResult && CManipulatorManager::GetValue( &vso.nDescID, pManipulator, szDBA + ".DescID" );

		szDBA = StrFmt( "vsoArray.[%d].vsoInstance", i );

		int nCtrlPointsNum = 0;
		bResult = bResult && CManipulatorManager::GetValue( &nCtrlPointsNum, pManipulator, szDBA + ".ControlPoints" );
		for ( int p = 0; p < nCtrlPointsNum; ++p )
		{
			CVec3 vPoint;
			bResult = bResult && CManipulatorManager::GetVec3<CVec3,float>( &vPoint, pManipulator, szDBA + StrFmt( ".ControlPoints.[%d]", p ) );
			instance.controlPoints.push_back( vPoint );
		}

		int nPointsNum = 0;
		bResult = bResult && CManipulatorManager::GetValue( &nPointsNum, pManipulator, szDBA + ".points" );
		for ( int p = 0; p < nPointsNum; ++p )
		{
			string szDBAPoint = szDBA + StrFmt( ".points.[%d]", p );

			NDb::SVSOPoint vsoPoint;
			bResult = bResult && CManipulatorManager::GetVec3<CVec3,float>( &vsoPoint.vPos, pManipulator, szDBAPoint  + ".Pos" );
			bResult = bResult && CManipulatorManager::GetVec3<CVec3,float>( &vsoPoint.vNorm, pManipulator, szDBAPoint  + ".Norm" );
			bResult = bResult && CManipulatorManager::GetValue( &vsoPoint.fWidth, pManipulator, szDBAPoint  + ".Width" );
			bResult = bResult && CManipulatorManager::GetValue( &vsoPoint.fOpacity, pManipulator, szDBAPoint + ".Opacity" );
			bResult = bResult && CManipulatorManager::GetValue( &vsoPoint.bKeyPoint, pManipulator, szDBAPoint + ".KeyPoint" );
			bResult = bResult && CManipulatorManager::GetValue( &vsoPoint.fRadius, pManipulator, szDBAPoint + ".Radius" );
			bResult = bResult && CManipulatorManager::GetValue( &vsoPoint.fReserved, pManipulator, szDBAPoint + ".Reserved" );

			instance.points.push_back( vsoPoint );
		}

		if ( bResult )
			AddVso( vso.vsoInstance, vso.szType, vso.nTypeID, vso.nDescID );

	}
	return bResult;
}


bool CMapClip::SaveEntrenchmentsToDB(  CObjectBaseController *pObjectController, IManipulator *pManipulator ) const
{
	if ( !pObjectController || !pManipulator )
		return false;

	bool bResult = true;
	for ( int i = 0 ; i < entrenchments.size(); ++i )
	{
		const SClipboardEntrenchment &tr = entrenchments[i];

		const string szEntrenchmentPrefix = StrFmt( "Entrenchments.[%d]", i );
		const string szObjectPrefix = szEntrenchmentPrefix + ".ObjectInfo";

		bResult = bResult && pObjectController->AddInsertOperation( "Entrenchments", NODE_ADD_INDEX, pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Object", 
			string( StrFmt( "%s%c%c%d", tr.szRPGStatsTypeName.c_str(), TYPE_SEPARATOR_CHAR, 
			ID_PREFIX_CHAR, tr.nRPGStatsID ) ), pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".FrameIndex", (int)( tr.nFrameIndex ), pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Player", (int)( tr.nPlayer ), pManipulator );
		bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3,float>( szObjectPrefix + ".Pos", tr.vPosition, pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Dir", tr.fDirection, pManipulator );
		//
//		if ( bResult )
//		{
//			const int nObjectID = pManipulator->GetID( szObjectPrefix );
//			const int nObjectLinkID = nObjectID;
//			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Link.LinkID", nObjectLinkID, pManipulator );
//		}
		//
		vector<NMapInfoEditor::SEntrenchmentSegInfo> segmentsInfo( tr.segmentsInfo.begin(), tr.segmentsInfo.end() );
		for ( int p = 0; p < segmentsInfo.size(); ++p )
		{
			NMapInfoEditor::SEntrenchmentSegInfo seg = segmentsInfo[p];
			bResult = bResult && pObjectController->AddInsertOperation( szEntrenchmentPrefix + ".segmentsInfo", NODE_ADD_INDEX, pManipulator );
			string szDBAPoint = szEntrenchmentPrefix + StrFmt( ".segmentsInfo.[%d]", p );

			string szSegType = typeEntrenchmentSegment.GetMnemonic( seg.eSegType );
			bResult = bResult && pObjectController->AddChangeOperation( szDBAPoint + ".SegType", szSegType, pManipulator );
			bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3,float>( szDBAPoint + ".Pos0", seg.vPos0, pManipulator );
			bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3,float>( szDBAPoint + ".Pos1", seg.vPos1, pManipulator );
			bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3,float>( szDBAPoint + ".PosCenter", seg.vPosCenter, pManipulator );
			bResult = bResult && pObjectController->AddChangeOperation( szDBAPoint + ".DirAngle", seg.fDirAngle, pManipulator );
			bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3,float>( szDBAPoint + ".AABBSize", seg.vAABBSize, pManipulator );
			bResult = bResult && pObjectController->AddChangeVec2Operation<CVec2,float>( szDBAPoint + ".AABBCenter", seg.vAABBCenter, pManipulator );
		}
		//
		if ( !bResult )
		{
			pObjectController->Undo( true, true, 0 );
			return false;	
		}
	}
	return bResult;
}


bool CMapClip::LoadEntrenchmentsFromDB( IManipulator *pManipulator )
{
	if ( !pManipulator )
		return false;

	bool bResult = true;
	int nEntrenchmentNum = 0;
	bResult = bResult && CManipulatorManager::GetValue( &nEntrenchmentNum, pManipulator, "Entrenchments" );

	for ( int i = 0 ; i < nEntrenchmentNum; ++i )
	{
		SClipboardEntrenchment tr;

		const string szEntrenchmentPrefix = StrFmt( "Entrenchments.[%d]", i );
		const string szObjectPrefix = szEntrenchmentPrefix + ".ObjectInfo";

		bResult = bResult && CManipulatorManager::GetParamsFromReference( szObjectPrefix + ".Object", pManipulator,
			&tr.szRPGStatsTypeName, 0, &tr.nRPGStatsTypeID, &tr.nRPGStatsID, 0 );
		bResult = bResult && CManipulatorManager::GetValue( &tr.nFrameIndex, pManipulator, szObjectPrefix + ".FrameIndex" );
		bResult = bResult && CManipulatorManager::GetValue( &tr.nPlayer, pManipulator, szObjectPrefix + ".Player" );
		bResult = bResult && CManipulatorManager::GetVec3<CVec3,float>( &tr.vPosition, pManipulator, szObjectPrefix + ".Pos" );
		bResult = bResult && CManipulatorManager::GetValue( &tr.fDirection, pManipulator, szObjectPrefix + ".Dir" );
		//
		int nSegNum = 0;
		bResult = bResult && CManipulatorManager::GetValue( &nSegNum, pManipulator, szEntrenchmentPrefix + ".segmentsInfo" );
		for ( int p = 0; p < nSegNum; ++p )
		{
			NMapInfoEditor::SEntrenchmentSegInfo seg;

			string szDBAPoint = szEntrenchmentPrefix + StrFmt( ".segmentsInfo.[%d]", p );

			string szSegType;
			bResult = bResult && CManipulatorManager::GetValue( &szSegType, pManipulator, szDBAPoint + ".SegType" );
			seg.eSegType = static_cast<NDb::EEntrenchSegmType>( typeEntrenchmentSegment.GetValue( szSegType ) );
			
			bResult = bResult && CManipulatorManager::GetVec3<CVec3,float>( &seg.vPos0, pManipulator, szDBAPoint + ".Pos0" );
			bResult = bResult && CManipulatorManager::GetVec3<CVec3,float>( &seg.vPos1, pManipulator, szDBAPoint + ".Pos1" );
			bResult = bResult && CManipulatorManager::GetVec3<CVec3,float>( &seg.vPosCenter, pManipulator, szDBAPoint + ".PosCenter" );
			bResult = bResult && CManipulatorManager::GetValue( &seg.fDirAngle, pManipulator, szDBAPoint + ".DirAngle" );
			bResult = bResult && CManipulatorManager::GetVec3<CVec3,float>( &seg.vAABBSize, pManipulator, szDBAPoint + ".AABBSize" );
			bResult = bResult && CManipulatorManager::GetVec2<CVec2,float>( &seg.vAABBCenter, pManipulator, szDBAPoint + ".AABBCenter" );

			if ( bResult ) 
			{
				tr.segmentsInfo.push_back( seg );
			}
		}
		//
		if ( bResult )
			entrenchments.push_back( tr );	
	}
	return bResult;
}


bool CMapClip::SaveBridgesToDB(  CObjectBaseController *pObjectController, IManipulator *pManipulator ) const
{
	if ( !pObjectController || !pManipulator )
		return false;

	bool bResult = true;
	for ( int i = 0 ; i < bridges.size(); ++i )
	{
		const SClipboardBridge &br = bridges[i];

		const string szBridgePrefix = StrFmt( "Bridges.[%d]", i );
		const string szObjectPrefix = szBridgePrefix + ".ObjectInfo";

		bResult = bResult && pObjectController->AddInsertOperation( "Bridges", NODE_ADD_INDEX, pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Object", 
			string( StrFmt( "%s%c%c%d", br.szRPGStatsTypeName.c_str(), TYPE_SEPARATOR_CHAR, 
			ID_PREFIX_CHAR, br.nRPGStatsID ) ), pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".FrameIndex", (int)( br.nFrameIndex ), pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Player", (int)( br.nPlayer ), pManipulator );
		bResult = bResult && pObjectController->AddChangeVec2Operation<CVec2,float>( szObjectPrefix + ".Pos", CVec2(br.vPosition.x,br.vPosition.y), pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Dir", (int)Vis2AIRad(br.fDirection), pManipulator );
		//
		bResult = bResult && pObjectController->AddChangeVec2Operation<CVec2,float>( szBridgePrefix + ".CenterSize", br.vCenterSize, pManipulator );
		bResult = bResult && pObjectController->AddChangeVec2Operation<CVec2,float>( szBridgePrefix + ".EndSize", br.vEndSize, pManipulator );
		vector<CVec3> centerPointList( br.centerPointList.begin(), br.centerPointList.end() );
		for ( int p = 0; p < centerPointList.size(); ++p )
		{
			bResult = bResult && pObjectController->AddInsertOperation( szBridgePrefix + ".centerPointList", NODE_ADD_INDEX, pManipulator );
			string szDBAPoint = szBridgePrefix + StrFmt( ".centerPointList.[%d]", p );
			bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3,float>( szDBAPoint, centerPointList[p], pManipulator );
		}
		//
		if ( !bResult )
		{
			pObjectController->Undo( true, true, 0 );
			return false;	
		}

	}
	return bResult;
}


bool CMapClip::LoadBridgesFromDB( IManipulator *pManipulator )
{
	if ( !pManipulator )
		return false;

	bool bResult = true;

	int nNumBridges = 0;
	bResult = bResult && CManipulatorManager::GetValue( &nNumBridges, pManipulator, "Bridges" );

	for ( int i = 0 ; i < nNumBridges; ++i )
	{
		SClipboardBridge br;

		const string szBridgePrefix = StrFmt( "Bridges.[%d]", i );
		const string szObjectPrefix = szBridgePrefix + ".ObjectInfo";

		bResult = bResult && CManipulatorManager::GetParamsFromReference( szObjectPrefix + ".Object", pManipulator, &br.szRPGStatsTypeName, 0,
			&br.nRPGStatsTypeID, &br.nRPGStatsID, 0 );
		bResult = bResult && CManipulatorManager::GetValue( &br.nFrameIndex, pManipulator, szObjectPrefix + ".FrameIndex" );
		bResult = bResult && CManipulatorManager::GetValue( &br.nPlayer, pManipulator, szObjectPrefix + ".Player" );
		bResult = bResult && CManipulatorManager::GetVec3<CVec3,float>( &br.vPosition, pManipulator, szObjectPrefix + ".Pos" );
		WORD nDir = 0;
		bResult = bResult && CManipulatorManager::GetValue( &nDir, pManipulator, szObjectPrefix + ".Dir" );
		br.fDirection = AI2VisRad( nDir );
		//
		bResult = bResult && CManipulatorManager::GetVec2<CVec2,float>( &br.vCenterSize, pManipulator, szBridgePrefix + ".CenterSize" );
		bResult = bResult && CManipulatorManager::GetVec2<CVec2,float>( &br.vEndSize, pManipulator, szBridgePrefix + ".EndSize" );

		int nNumCenterPoints = 0;
		bResult = bResult && CManipulatorManager::GetValue( &nNumCenterPoints, pManipulator, szBridgePrefix + ".centerPointList" );
		for ( int p = 0; p < nNumCenterPoints; ++p )
		{
			CVec3 vPos = VNULL3;
			string szDBAPoint = szBridgePrefix + StrFmt( ".centerPointList.[%d]", p );
			bResult = bResult && CManipulatorManager::GetVec3<CVec3,float>( &vPos, pManipulator, szDBAPoint );
			br.centerPointList.push_back( vPos );
		}

		if ( bResult )
			bridges.push_back( br );
	}
	return bResult;
}


bool CMapClip::SaveSpotsToDB(  CObjectBaseController *pObjectController, IManipulator *pManipulator ) const
{
	bool bResult = true;
	for ( int nNewSpotIndex = 0; nNewSpotIndex < GetSpotsNum(); ++nNewSpotIndex )
	{
		const SClipboardTerraSpot *pSpot = GetSpot( nNewSpotIndex );
		if ( !pSpot )
			continue;
		bResult = bResult && pObjectController->AddInsertOperation( "TerraSpots", NODE_ADD_INDEX, pManipulator );
		const string szNewSpotProperty = StrFmt( "TerraSpots.[%d].", nNewSpotIndex );
		// Insert
		bResult = bResult && pObjectController->AddChangeOperation( szNewSpotProperty + "spotInstance.Descriptor", string( StrFmt( "%s%c%c%d", pSpot->szType.c_str(), TYPE_SEPARATOR_CHAR, ID_PREFIX_CHAR, pSpot->nDescID ) ), pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szNewSpotProperty + "spotInstance.SpotID", int(0), pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szNewSpotProperty + "Type", pSpot->szType, pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szNewSpotProperty + "TypeID", pSpot->nTypeID, pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szNewSpotProperty + "DescID", pSpot->nDescID, pManipulator );
		for ( int nPointIndex = 0; nPointIndex < pSpot->spotInstance.points.size(); ++nPointIndex )
		{
			bResult = bResult && pObjectController->AddInsertOperation( szNewSpotProperty + "spotInstance.points", NODE_ADD_INDEX, pManipulator );
			bResult = bResult && pObjectController->AddChangeVec2Operation<CVec2, float>( szNewSpotProperty + StrFmt( "spotInstance.points.[%d]", nPointIndex ), pSpot->spotInstance.points[nPointIndex], pManipulator );
			if ( !bResult )
			{
				break;
			}
		}
	}
	return bResult;
}


bool CMapClip::LoadSpotsFromDB( IManipulator *pManipulator )
{
	bool bResult = true;
	int nNumSpots = 0;
	bResult = bResult && CManipulatorManager::GetValue( &nNumSpots, pManipulator, "TerraSpots" );
	for ( int i = 0; i < nNumSpots; ++i )
	{
		const string szSpotProperty = StrFmt( "TerraSpots.[%d].", i );
		SClipboardTerraSpot spot;
		spot.spotInstance.pDescriptor = 0; // дескриптор будет получен позже по DescID
		spot.spotInstance.nSpotID = 0; // не имеет значения
		bResult = bResult && CManipulatorManager::GetValue( &spot.szType, pManipulator, ( szSpotProperty + "Type" ) );
		bResult = bResult && CManipulatorManager::GetValue( &spot.nTypeID, pManipulator, ( szSpotProperty + "TypeID" ) );
		bResult = bResult && CManipulatorManager::GetValue( &spot.nDescID, pManipulator, ( szSpotProperty + "DescID" ) );
		int nPointsNum = 0;
		bResult = bResult && CManipulatorManager::GetValue( &nPointsNum, pManipulator, (szSpotProperty + "spotInstance.points") );
		for ( int p = 0; p < nPointsNum; ++p )
		{
			string szDBA = (szSpotProperty + StrFmt( "spotInstance.points.[%d]", p ));
			CVec2 v = VNULL2;
			bResult = bResult && CManipulatorManager::GetVec2<CVec2,float>( &v, pManipulator, szDBA );
			spot.spotInstance.points.push_back( v );
		}
		if ( bResult )
		{
			AddSpot( spot );
		}
	}
	return bResult;
}


bool CMapClip::SaveObjectsToDB(  CObjectBaseController *pObjectController, IManipulator *pManipulator ) const
{
	bool bResult = true;
	for ( int nObjectIndex = 0; nObjectIndex < GetObjNum(); ++nObjectIndex )
	{
		const SClipboardObjectInfo &oi = (*GetObj(nObjectIndex));
		//
		const string szObjectPrefix = StrFmt( "Objects.[%d]", nObjectIndex );
		bResult = bResult && pObjectController->AddInsertOperation( "Objects", NODE_ADD_INDEX, pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Object", 
			string( StrFmt( "%s%c%c%d", oi.szRPGStatsTypeName.c_str(), TYPE_SEPARATOR_CHAR, 
			ID_PREFIX_CHAR, oi.nRPGStatsID ) ), pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".FrameIndex", (int)( oi.nFrameIndex ), pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Player", (int)( oi.nPlayer ), pManipulator );
		bResult = bResult && pObjectController->AddChangeVec2Operation<CVec2,float>( szObjectPrefix + ".Pos", CVec2(oi.vPosition.x,oi.vPosition.y), pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Dir", (int)Vis2AIRad(oi.fDirection), pManipulator );
		//
		if ( ( bResult ) && ( pMapInfo != 0 ) )
		{
			const int nLinkID = pMapInfo->linkIDCollector.LockID();
			bResult = bResult && pObjectController->AddChangeOperation( szObjectPrefix + ".Link.LinkID", nLinkID, pManipulator );
		}
		//
		if ( !bResult )
		{
			pObjectController->Undo( true, true, 0 );
			return false;	
		}
	}
	return true;
}


bool CMapClip::LoadObjectsFromDB( IManipulator *pManipulator )
{
	int nObjNum = 0;
	if ( !CManipulatorManager::GetValue( &nObjNum, pManipulator, "Objects" ) )
		return false;

	bool bResult = true;

	for ( int i = 0; i < nObjNum; ++i )
	{
		string szDBA = StrFmt( "Objects.[%d]", i );
		SClipboardObjectInfo oi;

		bResult = bResult && CManipulatorManager::GetVec3<CVec3,float>( &oi.vPosition, pManipulator, szDBA + ".Pos" );

		WORD nDir = 0;
		bResult = bResult && CManipulatorManager::GetValue( &nDir, pManipulator, szDBA + ".Dir" );
		oi.fDirection = AI2VisRad( nDir );

		bResult = bResult && CManipulatorManager::GetValue( &oi.nPlayer, pManipulator, szDBA + ".Player" );
		bResult = bResult && CManipulatorManager::GetValue( &oi.nFrameIndex, pManipulator, szDBA + ".FrameIndex" );
		bResult = bResult && CManipulatorManager::GetParamsFromReference( szDBA + ".Object", pManipulator, 
			&oi.szRPGStatsTypeName, &oi.szName, &oi.nRPGStatsTypeID, &oi.nRPGStatsID, 0 );

		AddObj( oi );

		if ( !bResult )
			return false;
	}
	return true;
}


/**/
