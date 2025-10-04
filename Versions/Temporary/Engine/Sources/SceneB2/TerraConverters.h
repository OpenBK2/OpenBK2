#pragma once

#include "Image/Targa.h"
#include "TerrainInfo.h"

void LegacyLoadTileMap( const string &szMapFilesPath, int nX, int nY, CArray2D<BYTE> *pTileTerraMap )
{
	CFileStream streamTileMap( szMapFilesPath + "zonemap.tga", CFileStream::WIN_READ_ONLY );
	if ( streamTileMap.IsOk() )
		NImage::LoadTGAImage( (*pTileTerraMap), &streamTileMap );

	if ( (pTileTerraMap->GetSizeX() != (nX * DEF_PATCH_SIZE + 1)) ||
			 (pTileTerraMap->GetSizeY() != (nY * DEF_PATCH_SIZE + 1)) )
	{
		//ConvertZoneMaskToTileMap();
		//SaveTileMap();
		pTileTerraMap->SetSizes( nX * DEF_PATCH_SIZE + 1, nY * DEF_PATCH_SIZE + 1 );
		pTileTerraMap->FillZero();
	}
}


