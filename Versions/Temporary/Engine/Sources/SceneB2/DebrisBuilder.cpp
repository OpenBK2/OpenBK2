#include "stdafx.h"

#include "Misc/Win32Random.h"
#include "TerraTools.h"
#include "GenTerrain.h"

#include <cstdint>

#define DEF_DEBRIS_HEIGHT_DISPERSION 0.1f

static void SmoothMaskSharp( CArray2D<uint8_t> *pImage, const CArray2D<uint8_t> &mask, const int nSmoothRad )
{
	pImage->SetSizes( mask.GetSizeX(), mask.GetSizeY() );
	const int nSmoothRad2 = nSmoothRad * nSmoothRad;
	int nZeroCount, nAllCount;
	for ( int g = 0; g < mask.GetSizeY(); ++g )
	{
		for ( int i = 0; i < mask.GetSizeX(); ++i )
		{
			nAllCount = nZeroCount = 0;
			for ( int gg = -nSmoothRad; gg <= nSmoothRad; ++gg )
			{
				for ( int ii = -nSmoothRad; ii <= nSmoothRad; ++ii )
				{
					if ( ((i + ii) >= 0) && ((i + ii) < mask.GetSizeX()) &&
							 ((g + gg) >= 0) && ((g + gg) < mask.GetSizeY()) &&
							 ((ii * ii + gg * gg) <= nSmoothRad2) )
					{
						if ( mask[g+gg][i+ii] == 0 )
							++nZeroCount;
						++nAllCount;
					}
				}
			}
			nAllCount = ( nAllCount - nZeroCount ) * 7;
			(*pImage)[g][i] = ( (NWin32Random::Random(0.0f, 1.0f) * nZeroCount) >
													(NWin32Random::Random(0.0f, 1.0f) * nAllCount/*( nAllCount - nZeroCount )*/) ) ?
													0 : 255;
		}
	}
}

static void SmoothMaskMedium( CArray2D<uint8_t> *pImage, const CArray2D<uint8_t> &mask, const int nSmoothRad, CNoiseManager &noiseManager )
{
	pImage->SetSizes( mask.GetSizeX(), mask.GetSizeY() );
	// init noises
	std::vector<CNoiseAccessor> smoothNoises;
	smoothNoises.reserve( 2 );
	const unsigned char nSmoothNoise = 9;
	for ( int k = 0; k < 2; ++k )
		smoothNoises.push_back( noiseManager.GetNoise(nSmoothNoise) );

	const int nSmoothRad2 = nSmoothRad * nSmoothRad;
	int nZeroCount, nAllCount;
	for ( int g = 0; g < mask.GetSizeY(); ++g )
	{
		for ( int i = 0; i < mask.GetSizeX(); ++i )
		{
			nAllCount = nZeroCount = 0;
			for ( int gg = -nSmoothRad; gg <= nSmoothRad; ++gg )
			{
				for ( int ii = -nSmoothRad; ii <= nSmoothRad; ++ii )
				{
					if ( ((i + ii) >= 0) && ((i + ii) < mask.GetSizeX()) &&
							 ((g + gg) >= 0) && ((g + gg) < mask.GetSizeY()) &&
							 ((ii * ii + gg * gg) <= nSmoothRad2) )
					{
						if ( mask[g+gg][i+ii] == 0 )
							++nZeroCount;
						++nAllCount;
					}
				}
			}
			nAllCount = ( nAllCount - nZeroCount ) * 3;
			(*pImage)[g][i] = ( (int)(smoothNoises[0].GetValue(i, g) * nZeroCount) >
													(int)(smoothNoises[1].GetValue(i, g) * nAllCount) ) ?
													0 : 255;
		}
	}
}

static void SmoothMaskBlur( CArray2D<uint8_t> *pImage, const CArray2D<uint8_t> &mask, const int nSmoothRad )
{
	pImage->SetSizes( mask.GetSizeX(), mask.GetSizeY() );
	const int nSmoothRad2 = nSmoothRad * nSmoothRad;
	int nZeroCount, nAllCount;
	for ( int g = 0; g < mask.GetSizeY(); ++g )
	{
		for ( int i = 0; i < mask.GetSizeX(); ++i )
		{
			nAllCount = nZeroCount = 0;
			for ( int gg = -nSmoothRad; gg <= nSmoothRad; ++gg )
			{
				for ( int ii = -nSmoothRad; ii <= nSmoothRad; ++ii )
				{
					if ( ((i + ii) >= 0) && ((i + ii) < mask.GetSizeX()) &&
							 ((g + gg) >= 0) && ((g + gg) < mask.GetSizeY()) &&
							 ((ii * ii + gg * gg) <= nSmoothRad2) )
					{
						if ( mask[g+gg][i+ii] == 0 )
							++nZeroCount;
						++nAllCount;
					}
				}
			}
			(*pImage)[g][i] = (float)( nAllCount - nZeroCount ) / nAllCount * 255;
		}
	}
}

template<class TTriangle>
static bool RasterizeObjectMask( const std::vector<CVec3> &verts, const std::vector<TTriangle> &trgs,
	const CVec3 &vMin, const CVec3 &vMax, CArray2D<uint8_t> *pMask, CVec2 *pOrigin,
	NDebrisBuilder::EMaskType maskType, int nSmoothRadius )
{
	// A vertical/empty mesh has no ground footprint; avoid zero-sized rasterization.
	if ( verts.empty() || trgs.empty() || vMax.x <= vMin.x || vMax.y <= vMin.y || nSmoothRadius < 0 ) return false;
	const float fCoeffX = ( vMax.x - vMin.x ) * DEF_PATCH_TEX_SIZE / DEF_PATCH_WORLD_SIZE;
	const float fCoeffY = ( vMax.y - vMin.y ) * DEF_PATCH_TEX_SIZE / DEF_PATCH_WORLD_SIZE;

	switch ( maskType )
	{
	case NDebrisBuilder::MASK_STATIC:
		/*pMask->SetSizes( fCoeffX * ( 2 * nSmoothRadius / DEF_DYNAMIC_DEBRIS_TEX_SIZE + 1 ),
									 fCoeffY * ( 2 * nSmoothRadius / DEF_DYNAMIC_DEBRIS_TEX_SIZE + 1 ) );
		pOrigin->Set( vMin.x * DEF_PATCH_TEX_SIZE / DEF_PATCH_WORLD_SIZE - fCoeffX * nSmoothRadius / DEF_DYNAMIC_DEBRIS_TEX_SIZE,
									vMin.y * DEF_PATCH_TEX_SIZE / DEF_PATCH_WORLD_SIZE - fCoeffY * nSmoothRadius / DEF_DYNAMIC_DEBRIS_TEX_SIZE );*/
		pMask->SetSizes( 2 * nSmoothRadius + (std::max)(1, int(fCoeffX)), 2 * nSmoothRadius + (std::max)(1, int(fCoeffY)) );
		pOrigin->Set( vMin.x * DEF_PATCH_TEX_SIZE / DEF_PATCH_WORLD_SIZE - nSmoothRadius,
									vMin.y * DEF_PATCH_TEX_SIZE / DEF_PATCH_WORLD_SIZE - nSmoothRadius );
		break;

	case NDebrisBuilder::MASK_DYNAMIC:
		pMask->SetSizes( DEF_DYNAMIC_DEBRIS_TEX_SIZE, DEF_DYNAMIC_DEBRIS_TEX_SIZE );
		pOrigin->Set( vMin.x * DEF_DYNAMIC_DEBRIS_TEX_SIZE / DEF_PATCH_WORLD_SIZE - nSmoothRadius,
									vMin.y * DEF_DYNAMIC_DEBRIS_TEX_SIZE / DEF_PATCH_WORLD_SIZE - nSmoothRadius );
		break;

	case NDebrisBuilder::MASK_AI_PASSABILITY:
		const float fAISizeX = Vis2AI( vMax.x - vMin.x ) / AI_TILE_SIZE;
		const int nAISizeX = fAISizeX;
		const float fAISizeY = Vis2AI( vMax.y - vMin.y ) / AI_TILE_SIZE;
		const int nAISizeY = fAISizeY;
		pMask->SetSizes( (std::max)(1, fabs( fAISizeX - nAISizeX ) < DEF_EPS ? nAISizeX : nAISizeX + 1),
									 (std::max)(1, fabs( fAISizeY - nAISizeY ) < DEF_EPS ? nAISizeY : nAISizeY + 1) );
		//pOrigin->Set( vMin.x, vMax.y );
		pOrigin->Set( -Vis2AI(vMin.x), -Vis2AI(vMin.y) );
		pOrigin->x *= fAISizeX / (float)pMask->GetSizeX();
		pOrigin->y *= fAISizeY / (float)pMask->GetSizeY();
		break;
	}
	const int nTexOffs = maskType == NDebrisBuilder::MASK_AI_PASSABILITY ? 0 : nSmoothRadius;
	// create mask
	pMask->FillZero();
	const float fScaleCoeffX = (float)( pMask->GetSizeX() - 2 * nTexOffs ) / ( vMax.x - vMin.x );
	const float fScaleCoeffY = (float)( pMask->GetSizeY() - 2 * nTexOffs ) / ( vMax.y - vMin.y );

	const float fDist = maskType != NDebrisBuilder::MASK_DYNAMIC ? DEF_DEBRIS_HEIGHT_DISPERSION : ( vMax.z - vMin.z ) + DEF_DEBRIS_HEIGHT_DISPERSION;

	for ( auto it = trgs.begin(); it != trgs.end(); ++it )
	{
		const int x1 = Clamp( int((verts[it->i1].x - vMin.x) * fScaleCoeffX) + nTexOffs, 0, pMask->GetSizeX() - 1 );
		const int y1 = Clamp( int((verts[it->i1].y - vMin.y) * fScaleCoeffY) + nTexOffs, 0, pMask->GetSizeY() - 1 );
		const int x2 = Clamp( int((verts[it->i2].x - vMin.x) * fScaleCoeffX) + nTexOffs, 0, pMask->GetSizeX() - 1 );
		const int y2 = Clamp( int((verts[it->i2].y - vMin.y) * fScaleCoeffY) + nTexOffs, 0, pMask->GetSizeY() - 1 );
		const int x3 = Clamp( int((verts[it->i3].x - vMin.x) * fScaleCoeffX) + nTexOffs, 0, pMask->GetSizeX() - 1 );
		const int y3 = Clamp( int((verts[it->i3].y - vMin.y) * fScaleCoeffY) + nTexOffs, 0, pMask->GetSizeY() - 1 );
		if ( (fabs(verts[it->i1].z - vMin.z) <= fDist) && (fabs(verts[it->i2].z - vMin.z) <= fDist) )
			DrawLine( pMask, x1, y1, x2, y2, 1 );
		if ( (fabs(verts[it->i2].z - vMin.z) <= fDist) && (fabs(verts[it->i3].z - vMin.z) <= fDist) )
			DrawLine( pMask, x2, y2, x3, y3, 1 );
		if ( (fabs(verts[it->i3].z - vMin.z) <= fDist) && (fabs(verts[it->i1].z - vMin.z) <= fDist) )
			DrawLine( pMask, x3, y3, x1, y1, 1 );
	}

	WiseFill( pMask, 1 );

	return true;
}

// Exporters can supply decoded model geometry; legacy callers keep their file loader.
bool NDebrisBuilder::CreateMask( const std::vector<CVec3> &vertices, const std::vector<SModelTriangle> &triangles,
	const CVec3 &minimum, const CVec3 &maximum, CArray2D<uint8_t> *image, CVec2 *origin,
	EMaskType type, int smoothRadius )
{
	CArray2D<uint8_t> mask;
	if ( !RasterizeObjectMask(vertices, triangles, minimum, maximum, &mask, origin, type, smoothRadius) ) return false;
	if ( type == MASK_AI_PASSABILITY ) image->Swap(mask);
	else SmoothMaskSharp(image, mask, smoothRadius);
	return true;
}

void CTerraGen::CreateDebris( const std::string &szFileName, CArray2D<uint8_t> *pImage, CVec2 *pOrigin,
															const NDebrisBuilder::EMaskType maskType, const int nSmoothRadius,
															const NDebrisBuilder::EMaskSmoothType smoothType )
{
	// load model
	std::vector<CVec3> verts;
	std::vector<STriangle> trgs;
	CVec3 vMin, vMax;
	LoadGrannyModel( szFileName, &verts, &trgs, &vMin, &vMax );
	if ( verts.empty() || trgs.empty() )
		return;

	CArray2D<uint8_t> mask;
	if ( !RasterizeObjectMask(verts, trgs, vMin, vMax, &mask, pOrigin, maskType, nSmoothRadius) ) return;

	if ( maskType == NDebrisBuilder::MASK_AI_PASSABILITY )
	{
		pImage->Swap( mask );
		return;
	}

	switch ( smoothType )
	{
	case NDebrisBuilder::MASK_SMOOTH_SHARP:
		SmoothMaskSharp( pImage, mask, nSmoothRadius );
		break;
	case NDebrisBuilder::MASK_SMOOTH_MEDIUM:
		SmoothMaskMedium( pImage, mask, nSmoothRadius, noiseManager );
		break;
	case NDebrisBuilder::MASK_SMOOTH_BLUR:
		SmoothMaskBlur( pImage, mask, nSmoothRadius );
		break;
	}
}


