#include "StdAfx.h"

//#include "..\Image\Targa.h"
//#include "..\Misc\GeomMisc.h"
//#include "GenTerrain.h"

//#define DEF_INV_255 ( 1.0f / 255.0f )

//void CTerraGen::CalcTerraObjectInfo( STerraObjectInfoEx *pObjInfo, const STerraObjectInfo *pObjProps )
//{
//	if ( !pDesc )
//		return;
//
//	pObjInfo->nID = pObjProps->nID;
//	pObjInfo->szMaskFileName = pObjProps->szMaskFileName;
//	pObjInfo->szTexFileName = pObjProps->szTexFileName;
//
//	const int nOrgX = AI2Vis( pObjProps->vPos.x ) / DEF_PATCH_WORLD_SIZE * DEF_PATCH_TEX_SIZE;
//	const int nOrgY = AI2Vis( pObjProps->vPos.y ) / DEF_PATCH_WORLD_SIZE * DEF_PATCH_TEX_SIZE;
//
//	NImage::STGAFileHeader tgaHeader;
//	CPtr<IDataStream> pStream = szSrcPath.empty() ? OpenStream( szSrcPath + pObjProps->szMaskFileName, STREAM_PATH_RELATIVE ) :
//																									OpenStream( szSrcPath + pObjProps->szMaskFileName, STREAM_PATH_ABSOLUTE );
//	NI_ASSERT( pStream, StrFmt("Can't open object's mask: %s", pObjProps->szMaskFileName.c_str() ) );
//	NImage::LoadTGAHeader( &tgaHeader, pStream );
//
//	const int nX1 = nOrgX + pObjProps->vOrigin.x;
//	const int nY1 = nOrgY + pObjProps->vOrigin.y;
//	const int nX2 = nX1 + tgaHeader.imagespec.wImageWidth;
//	const int nY2 = nY1 + tgaHeader.imagespec.wImageHeight;
//
//	const float fCos = cos( pObjProps->fAngle );
//	const float fSin = sin( pObjProps->fAngle );
//
//	pObjInfo->x1 = (float)( nX1 - nOrgX ) * fCos - (float)( nY1 - nOrgY ) * fSin + nOrgX;
//	pObjInfo->y1 = (float)( nX1 - nOrgX ) * fSin + (float)( nY1 - nOrgY ) * fCos + nOrgY;
//	pObjInfo->x2 = (float)( nX1 - nOrgX ) * fCos - (float)( nY2 - nOrgY ) * fSin + nOrgX;
//	pObjInfo->y2 = (float)( nX1 - nOrgX ) * fSin + (float)( nY2 - nOrgY ) * fCos + nOrgY;
//	pObjInfo->x3 = (float)( nX2 - nOrgX ) * fCos - (float)( nY2 - nOrgY ) * fSin + nOrgX;
//	pObjInfo->y3 = (float)( nX2 - nOrgX ) * fSin + (float)( nY2 - nOrgY ) * fCos + nOrgY;
//	pObjInfo->x4 = (float)( nX2 - nOrgX ) * fCos - (float)( nY1 - nOrgY ) * fSin + nOrgX;
//	pObjInfo->y4 = (float)( nX2 - nOrgX ) * fSin + (float)( nY1 - nOrgY ) * fCos + nOrgY;
//}

//void CTerraGen::AddTerraObject( const STerraObjectInfo *pObjInfo )
//{
//	// check, that such object already exists
//	for ( list<STerraObjectInfoEx>::const_iterator it = terraObjsInfo.begin(); it != terraObjsInfo.end(); ++it )
//	{
//		if ( pObjInfo->nID == it->nID )
//		{
//			NI_ASSERT( pObjInfo->nID != it->nID, "Added objects is already exists" );
//			return;
//		}
//	}
//
//	STerraObjectInfoEx newObj;
//	CalcTerraObjectInfo( &newObj, pObjInfo );
//	terraObjsInfo.push_back( newObj );
//
//	// add object to patches hash
//	const CTPoint<int> size( terraObjsPatchesHash.GetSizeX() - 1, terraObjsPatchesHash.GetSizeY() - 1 );
//	const int nPatchX1 = Clamp( min( min( newObj.x1, newObj.x2 ), min( newObj.x3, newObj.x4 ) ) / DEF_PATCH_TEX_SIZE, 0, size.x );
//	const int nPatchY1 = Clamp( min( min( newObj.y1, newObj.y2 ), min( newObj.y3, newObj.y4 ) ) / DEF_PATCH_TEX_SIZE, 0, size.y );
//	const int nPatchX2 = Clamp( max( max( newObj.x1, newObj.x2 ), max( newObj.x3, newObj.x4 ) ) / DEF_PATCH_TEX_SIZE, 0, size.x );
//	const int nPatchY2 = Clamp( max( max( newObj.y1, newObj.y2 ), max( newObj.y3, newObj.y4 ) ) / DEF_PATCH_TEX_SIZE, 0, size.y );
//	for ( int g = nPatchY1; g <= nPatchY2; ++g )
//	{
//		for ( int i = nPatchX1; i <= nPatchX2; ++i )
//		{
//			terraObjsPatchesHash[g][i].push_back( newObj.nID );
//		}
//	}
//}

//void CTerraGen::UpdateTerraObject( const STerraObjectInfo *pObjInfo )
//{
//	if ( !pDesc )
//		return;
//
//	// check, that such object already exists
//	list<STerraObjectInfoEx>::iterator it = terraObjsInfo.begin();
//	for ( ; it != terraObjsInfo.end(); ++it )
//	{
//		if ( it->nID == pObjInfo->nID )
//			break;
//	}
//
//	if ( it == terraObjsInfo.end() )
//	{
//		NI_ASSERT( it != terraObjsInfo.end(), "Updated object is not existed" );
//		return;
//	}
//	const CTPoint<int> size( terraObjsPatchesHash.GetSizeX() - 1, terraObjsPatchesHash.GetSizeY() - 1 );
//	{
//		// remove old object from patches
//		const int nPatchX1 = Clamp( min( min( it->x1, it->x2 ), min( it->x3, it->x4 ) ) / DEF_PATCH_TEX_SIZE, 0, size.x );
//		const int nPatchY1 = Clamp( min( min( it->y1, it->y2 ), min( it->y3, it->y4 ) ) / DEF_PATCH_TEX_SIZE, 0, size.y );
//		const int nPatchX2 = Clamp( max( max( it->x1, it->x2 ), max( it->x3, it->x4 ) ) / DEF_PATCH_TEX_SIZE, 0, size.x );
//		const int nPatchY2 = Clamp( max( max( it->y1, it->y2 ), max( it->y3, it->y4 ) ) / DEF_PATCH_TEX_SIZE, 0, size.y ) ;
//		for ( int g = nPatchY1; g <= nPatchY2; ++g )
//		{
//			for ( int i = nPatchX1; i <= nPatchX2; ++i )
//			{
//				terraObjsPatchesHash[g][i].remove( it->nID );
//			}
//		}
//	}
//
//	STerraObjectInfoEx updObj;
//	CalcTerraObjectInfo( &updObj, pObjInfo );
//	it->x1 = updObj.x1; it->y1 = updObj.y1;
//	it->x2 = updObj.x2; it->y2 = updObj.y2;
//	it->x3 = updObj.x3; it->y3 = updObj.y3;
//	it->x4 = updObj.x4; it->y4 = updObj.y4;
//	it->szMaskFileName = updObj.szMaskFileName;
//	it->szTexFileName = updObj.szTexFileName;
//
//	{
//		// add object to patches hash
//		const int nPatchX1 = Clamp( min( min( it->x1, it->x2 ), min( it->x3, it->x4 ) ) / DEF_PATCH_TEX_SIZE, 0, size.x );
//		const int nPatchY1 = Clamp( min( min( it->y1, it->y2 ), min( it->y3, it->y4 ) ) / DEF_PATCH_TEX_SIZE, 0, size.y );
//		const int nPatchX2 = Clamp( max( max( it->x1, it->x2 ), max( it->x3, it->x4 ) ) / DEF_PATCH_TEX_SIZE, 0, size.x );
//		const int nPatchY2 = Clamp( max( max( it->y1, it->y2 ), max( it->y3, it->y4 ) ) / DEF_PATCH_TEX_SIZE, 0, size.y );
//		for ( int g = nPatchY1; g <= nPatchY2; ++g )
//		{
//			for ( int i = nPatchX1; i <= nPatchX2; ++i )
//			{
//				terraObjsPatchesHash[g][i].push_back( it->nID );
//			}
//		}
//	}
//}

//void CTerraGen::RemoveTerraObject( const int nID )
//{
//	if ( !pDesc )
//		return;
//
//	// check, that such object already exists
//	list<STerraObjectInfoEx>::iterator it = terraObjsInfo.begin();
//	for ( ; it != terraObjsInfo.end(); ++it )
//	{
//		if ( it->nID == nID )
//			break;
//	}
//
//	if ( it == terraObjsInfo.end() )
//	{
//		NI_ASSERT( it != terraObjsInfo.end(), "Removed object is not existed" );
//		return;
//	}
//	const CTPoint<int> size( terraObjsPatchesHash.GetSizeX() - 1, terraObjsPatchesHash.GetSizeY() - 1 );
//	const int nPatchX1 = Clamp( min( min( it->x1, it->x2 ), min( it->x3, it->x4 ) ) / DEF_PATCH_TEX_SIZE, 0, size.x );
//	const int nPatchY1 = Clamp( min( min( it->y1, it->y2 ), min( it->y3, it->y4 ) ) / DEF_PATCH_TEX_SIZE, 0, size.y );
//	const int nPatchX2 = Clamp( max( max( it->x1, it->x2 ), max( it->x3, it->x4 ) ) / DEF_PATCH_TEX_SIZE, 0, size.x );
//	const int nPatchY2 = Clamp( max( max( it->y1, it->y2 ), max( it->y3, it->y4 ) ) / DEF_PATCH_TEX_SIZE, 0, size.y );
//	for ( int g = nPatchY1; g <= nPatchY2; ++g )
//	{
//		for ( int i = nPatchX1; i <= nPatchX2; ++i )
//		{
//			terraObjsPatchesHash[g][i].remove( it->nID );
//		}
//	}
//
//	for ( list<STerraObjectInfoEx>::iterator itDel = terraObjsInfo.begin(); itDel != terraObjsInfo.end(); )
//	{
//		if ( itDel->nID == nID )
//			itDel = terraObjsInfo.erase( itDel );
//		else
//			++itDel;
//	}
//}

/*void CTerraGen::ApplyTerraObjects( CImageAccessor &patchTex, const int nPatchX, const int nPatchY )
{
	if ( !pDesc )
		return;

	if ( terraObjsInfo.empty() )
		return;

	if ( terraObjsPatchesHash[nPatchY][nPatchX].empty() )
		return;

	CArray2D<BYTE> objMask;
	CArray2D<SColor24> objTex;

	//// load patch tex image
	//CPtr<IDataStream> pPatchTGAStream = szSrcPath.empty() ? OpenStream( MakePreSrcTextureName(nPatchX, nPatchY), STREAM_PATH_RELATIVE ) :
	//																												OpenStream( MakePreSrcTextureName(nPatchX, nPatchY), STREAM_PATH_ABSOLUTE );
	//NI_ASSERT( pPatchTGAStream, StrFmt("Can't open texture for patch %dx%d", nPatchX, nPatchY ) );
	//NImage::LoadTGAImage( texPatch, pPatchTGAStream );

	const int nOffsX = nPatchX * DEF_PATCH_TEX_SIZE;
	const int nOffsY = nPatchY * DEF_PATCH_TEX_SIZE;

	// process all objects, which are placed on this patch
	const list<int> &curPatch = terraObjsPatchesHash[nPatchY][nPatchX];
	for ( list<int>::const_iterator it = curPatch.begin(); it != curPatch.end(); ++it )
	{
		list<STerraObjectInfoEx>::const_iterator itObjInfo = terraObjsInfo.begin();
		for ( ; itObjInfo != terraObjsInfo.end(); ++itObjInfo )
		{
			if ( itObjInfo->nID == *it )
				break;;
		}
		if ( itObjInfo == terraObjsInfo.end() )
		{
			NI_ASSERT( itObjInfo != terraObjsInfo.end(), StrFmt( "Unknown terra object %d", *it ) );
			continue;
		}

		// load object's mask image
		CPtr<IDataStream> pObjMaskStream = szSrcPath.empty() ? OpenStream( szSrcPath + itObjInfo->szMaskFileName, STREAM_PATH_RELATIVE ) :
																													 OpenStream( szSrcPath + itObjInfo->szMaskFileName, STREAM_PATH_ABSOLUTE );
		NI_ASSERT( pObjMaskStream, StrFmt("Can't open mask for terra object %d", itObjInfo->nID ) );
		NImage::LoadTGAImage( objMask, pObjMaskStream );

		// load object's texture image
		CPtr<IDataStream> pObjTexStream = szSrcPath.empty() ? OpenStream( szSrcPath + itObjInfo->szTexFileName, STREAM_PATH_RELATIVE ) :
																													OpenStream( szSrcPath + itObjInfo->szTexFileName, STREAM_PATH_ABSOLUTE );
		NI_ASSERT( pObjTexStream, StrFmt("Can't open texture for terra object %d", itObjInfo->nID ) );
		NImage::LoadTGAImage( objTex, pObjTexStream );

		// apply image
		const int nMinX = Clamp( min( min( itObjInfo->x1-nOffsX, itObjInfo->x2-nOffsX ),
																	min( itObjInfo->x3-nOffsX, itObjInfo->x4-nOffsX ) ), 0, DEF_PATCH_TEX_SIZE - 1 );
		const int nMinY = Clamp( min( min( itObjInfo->y1-nOffsY, itObjInfo->y2-nOffsY ),
																	min( itObjInfo->y3-nOffsY, itObjInfo->y4-nOffsY ) ), 0, DEF_PATCH_TEX_SIZE - 1 );
		const int nMaxX = Clamp( max( max( itObjInfo->x1-nOffsX, itObjInfo->x2-nOffsX ),
																	max( itObjInfo->x3-nOffsX, itObjInfo->x4-nOffsX ) ), 0, DEF_PATCH_TEX_SIZE - 1 );
		const int nMaxY = Clamp( max( max( itObjInfo->y1-nOffsY, itObjInfo->y2-nOffsY ),
																	max( itObjInfo->y3-nOffsY, itObjInfo->y4-nOffsY ) ), 0, DEF_PATCH_TEX_SIZE - 1 );
		const int v0x = itObjInfo->x1-nOffsX;
		const int v0y = itObjInfo->y1-nOffsY;
		const int v1x = itObjInfo->x4 - itObjInfo->x1;
		const int v1y = itObjInfo->y4 - itObjInfo->y1;
		const int v2x = itObjInfo->x2 - itObjInfo->x1;
		const int v2y = itObjInfo->y2 - itObjInfo->y1;
		if ( ( v1x * v2y - v1y * v2x ) == 0 )
			continue;
		const float fDet = 1.0f / ( (float)v1x * v2y - v1y * v2x );

		for ( int y = nMinY; y <= nMaxY; ++y )
		{
			for ( int x = nMinX; x <= nMaxX; ++x )
			{
				const float fTx = (float)( ( x - v0x ) * v2y - ( y - v0y ) * v2x ) * fDet;
				const float fTy = -(float)( ( x - v0x ) * v1y - ( y - v0y ) * v1x ) * fDet;
				if ( ( fTx >= 0.0f ) && ( fTx <= 1.0f ) && ( fTy >= 0.0f ) && ( fTy <= 1.0f ) )
				{
					const float fAlpha = (float)objMask[int(fTy*(objMask.GetSizeY()-1))][int(fTx*(objMask.GetSizeX()-1))] * DEF_INV_255;
					patchTex[y][x].r = Clamp( int( (float)patchTex[y][x].r * (1.0f - fAlpha ) +
						(float)objTex[int(fTy*(objTex.GetSizeY()-1))][int(fTx*(objTex.GetSizeX()-1))].r * fAlpha ), 0, 255 );
					patchTex[y][x].g = Clamp( int( (float)patchTex[y][x].g * (1.0f - fAlpha ) +
						(float)objTex[int(fTy*(objTex.GetSizeY()-1))][int(fTx*(objTex.GetSizeX()-1))].g * fAlpha ), 0, 255 );
					patchTex[y][x].b = Clamp( int( (float)patchTex[y][x].b * (1.0f - fAlpha ) +
						(float)objTex[int(fTy*(objTex.GetSizeY()-1))][int(fTx*(objTex.GetSizeX()-1))].b * fAlpha ), 0, 255 );
				}
			}
		}
	}

	//// save patch tex image
	//pPatchTGAStream = 0;
	//pPatchTGAStream = szSrcPath.empty() ? CreateStream( MakePreSrcTextureName(nPatchX, nPatchY), STREAM_PATH_RELATIVE ) :
	//																			CreateStream( MakePreSrcTextureName(nPatchX, nPatchY), STREAM_PATH_ABSOLUTE );
	//NI_ASSERT( pPatchTGAStream, StrFmt("Can't create texture for patch %dx%d", nPatchX, nPatchY ) );
	//NImage::SaveAsTGA( texPatch, pPatchTGAStream );
}*/

//void CTerraGen::ApplyTerraObjects( CArray2D<DWORD> &patchTex, const int nPatchX, const int nPatchY )
//{
//	if ( !pDesc )
//		return;
//
//	if ( terraObjsInfo.empty() )
//		return;
//
//	if ( terraObjsPatchesHash[nPatchY][nPatchX].empty() )
//		return;
//
//	CArray2D<BYTE> objMask;
//	CArray2D<SColor24> objTex;
//
//	const int nOffsX = nPatchX * DEF_PATCH_TEX_SIZE;
//	const int nOffsY = nPatchY * DEF_PATCH_TEX_SIZE;
//
//	// process all objects, which are placed on this patch
//	const list<int> &curPatch = terraObjsPatchesHash[nPatchY][nPatchX];
//	for ( list<int>::const_iterator it = curPatch.begin(); it != curPatch.end(); ++it )
//	{
//		list<STerraObjectInfoEx>::const_iterator itObjInfo = terraObjsInfo.begin();
//		for ( ; itObjInfo != terraObjsInfo.end(); ++itObjInfo )
//		{
//			if ( itObjInfo->nID == *it )
//				break;;
//		}
//		if ( itObjInfo == terraObjsInfo.end() )
//		{
//			NI_ASSERT( itObjInfo != terraObjsInfo.end(), StrFmt( "Unknown terra object %d", *it ) );
//			continue;
//		}
//
//		// load object's mask image
//		CPtr<IDataStream> pObjMaskStream = szSrcPath.empty() ? OpenStream( szSrcPath + itObjInfo->szMaskFileName, STREAM_PATH_RELATIVE ) :
//																													 OpenStream( szSrcPath + itObjInfo->szMaskFileName, STREAM_PATH_ABSOLUTE );
//		NI_ASSERT( pObjMaskStream, StrFmt("Can't open mask for terra object %d", itObjInfo->nID ) );
//		NImage::LoadTGAImage( objMask, pObjMaskStream );
//
//		// load object's texture image
//		CPtr<IDataStream> pObjTexStream = szSrcPath.empty() ? OpenStream( szSrcPath + itObjInfo->szTexFileName, STREAM_PATH_RELATIVE ) :
//		OpenStream( szSrcPath + itObjInfo->szTexFileName, STREAM_PATH_ABSOLUTE );
//		NI_ASSERT( pObjTexStream, StrFmt("Can't open texture for terra object %d", itObjInfo->nID ) );
//		NImage::LoadTGAImage( objTex, pObjTexStream );
//
//		// apply image
//		const int nMinX = Clamp( min( min( itObjInfo->x1-nOffsX, itObjInfo->x2-nOffsX ),
//			min( itObjInfo->x3-nOffsX, itObjInfo->x4-nOffsX ) ), 0, DEF_PATCH_TEX_SIZE - 1 );
//		const int nMinY = Clamp( min( min( itObjInfo->y1-nOffsY, itObjInfo->y2-nOffsY ),
//			min( itObjInfo->y3-nOffsY, itObjInfo->y4-nOffsY ) ), 0, DEF_PATCH_TEX_SIZE - 1 );
//		const int nMaxX = Clamp( max( max( itObjInfo->x1-nOffsX, itObjInfo->x2-nOffsX ),
//			max( itObjInfo->x3-nOffsX, itObjInfo->x4-nOffsX ) ), 0, DEF_PATCH_TEX_SIZE - 1 );
//		const int nMaxY = Clamp( max( max( itObjInfo->y1-nOffsY, itObjInfo->y2-nOffsY ),
//			max( itObjInfo->y3-nOffsY, itObjInfo->y4-nOffsY ) ), 0, DEF_PATCH_TEX_SIZE - 1 );
//		const int v0x = itObjInfo->x1-nOffsX;
//		const int v0y = itObjInfo->y1-nOffsY;
//		const int v1x = itObjInfo->x4 - itObjInfo->x1;
//		const int v1y = itObjInfo->y4 - itObjInfo->y1;
//		const int v2x = itObjInfo->x2 - itObjInfo->x1;
//		const int v2y = itObjInfo->y2 - itObjInfo->y1;
//		if ( ( v1x * v2y - v1y * v2x ) == 0 )
//			continue;
//		const float fDet = 1.0f / ( (float)v1x * v2y - v1y * v2x );
//
//		for ( int y = nMinY; y <= nMaxY; ++y )
//		{
//			for ( int x = nMinX; x <= nMaxX; ++x )
//			{
//				const float fTx = (float)( ( x - v0x ) * v2y - ( y - v0y ) * v2x ) * fDet;
//				const float fTy = -(float)( ( x - v0x ) * v1y - ( y - v0y ) * v1x ) * fDet;
//				if ( ( fTx >= 0.0f ) && ( fTx <= 1.0f ) && ( fTy >= 0.0f ) && ( fTy <= 1.0f ) )
//				{
//					const float fAlpha = (float)objMask[Float2Int( fTy * ( objMask.GetSizeY() - 1 ) )][Float2Int( fTx * ( objMask.GetSizeX() - 1 ) )] * DEF_INV_255;
//					const DWORD &col = patchTex[y][x];
//					const SColor24 &objCol = objTex[Float2Int( fTy * ( objTex.GetSizeY() - 1 ) )][Float2Int( fTx * ( objTex.GetSizeX() - 1 ) )];
//					const BYTE rr = Clamp( Float2Int( (float)UnpackBYTE2(col) * (1.0f - fAlpha ) + (float)objCol.r * fAlpha ), 0, 255 );
//					const BYTE gg = Clamp( Float2Int( (float)UnpackBYTE1(col) * (1.0f - fAlpha ) + (float)objCol.g * fAlpha ), 0, 255 );
//					const BYTE bb = Clamp( Float2Int( (float)UnpackBYTE0(col) * (1.0f - fAlpha ) + (float)objCol.b * fAlpha ), 0, 255 );
//					patchTex[y][x] = PackDWORD( 0xff, rr, gg, bb );
//				}
//			}
//		}
//	}
//}


