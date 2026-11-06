#pragma once

#include "3Dmotor/GPixelFormat.h"
#include "Misc/2Darray.h"
#include "Misc/Geom.h"
#include "Misc/Tools.h"

#include "SceneB2_export.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>

class CDBID;

namespace NDb
{
struct SHPObjectRPGStats;
struct SModel;
} // namespace NDb

namespace NUnitIconRendering
{
constexpr int TARGET_SIZE = 512;
constexpr int ICON_SIZE = 42;

struct SCameraShot
{
	CVec3 vAnchor = VNULL3;
	float fDistance = 0.0f;
	float fPitch = 0.0f;
	float fYaw = 0.0f;
	float fFov = 0.0f;
	bool bFromStats = false;
};

// SSE4's per-unit camera and model corrections, shared by both exporters.
struct SOverride
{
	bool bFrame = false;
	bool bAngle = false;
	float fFill = 0.93f;
	bool bSetYaw = false;
	float fYaw = 0.0f;
	bool bSetPitch = false;
	float fPitch = 0.0f;
	float fYawDelta = 0.0f;
	float fPitchDelta = 0.0f;
	float fDistanceScale = 1.0f;
	std::string szLike;
};

struct SOptions
{
	int nWidth = ICON_SIZE;
	int nHeight = ICON_SIZE;
	// Zero chooses the largest integer factor that fits the capture target.
	int nSupersample = 0;
	int nGridSize = 32;
	std::string szLight = "Scene/Lights/All/Summer/Daylight_AmbientLight.xdb";
	bool bEditorDatabase = false;
	bool bLegacyImage = false;
};

struct SResult
{
	CArray2D<NGfx::SPixel8888> image;
	CArray2D<NGfx::SPixel8888> legacyImage;
	SCameraShot shot;
	std::string szCorrections;
};

inline bool ValidDimensions( int nWidth, int nHeight )
{
	return nWidth > 0 && nHeight > 0 && nWidth <= TARGET_SIZE && nHeight <= TARGET_SIZE;
}

inline int GetSupersample( int nWidth, int nHeight, int nRequested = 0 )
{
	if ( !ValidDimensions( nWidth, nHeight ) || nRequested < 0 )
	{
		return 0;
	}
	const int nMaximum = TARGET_SIZE / (std::max)( nWidth, nHeight );
	return nRequested == 0 ? nMaximum : ( nRequested <= nMaximum ? nRequested : 0 );
}

inline float GetWindowFov( float fFov, int nWindow )
{
	// A narrower lens keeps the original 42-pixel framing at higher resolution.
	const float fHalfTan = std::tan( ToRadian( fFov ) / 2.0f ) * ICON_SIZE / float( nWindow );
	return ToDegree( 2.0f * std::atan( fHalfTan ) );
}

template <class TPixel>
bool CropCapture( CArray2D<TPixel> *pImage, const CArray2D<TPixel> &texture, int nWidth, int nHeight )
{
	if ( !ValidDimensions( nWidth, nHeight ) || nWidth > texture.GetSizeX() || nHeight > texture.GetSizeY() )
	{
		return false;
	}
	const int nStartX = ( texture.GetSizeX() - nWidth ) / 2;
	const int nStartY = ( texture.GetSizeY() - nHeight ) / 2;
	pImage->SetSizes( nWidth, nHeight );
	for ( int y = 0; y < nHeight; ++y )
	{
		for ( int x = 0; x < nWidth; ++x )
		{
			( *pImage )[y][x] = texture[y + nStartY][x + nStartX];
		}
	}
	return true;
}

SCENEB2_EXPORT bool Downsample( CArray2D<NGfx::SPixel8888> *pRes, const CArray2D<NGfx::SPixel8888> &src, int nFactor );
SCENEB2_EXPORT bool
ParseOverrides( const std::string &szText, std::map<std::string, SOverride> *pRes, std::string *pszError );
SCENEB2_EXPORT bool
LoadOverrides( const std::string &szFileName, std::map<std::string, SOverride> *pRes, std::string *pszError );
SCENEB2_EXPORT const SOverride *FindOverride( const std::map<std::string, SOverride> &overrides, const CDBID &dbid );
SCENEB2_EXPORT const std::map<std::string, SOverride> &GetDefaultOverrides();
SCENEB2_EXPORT bool ReadCameraShot( const CDBID &dbid, bool bEditorDatabase, SCameraShot *pShot );
// Select the intact summer model, including fences whose main visual object is empty.
SCENEB2_EXPORT const NDb::SModel *GetIconModel( const NDb::SHPObjectRPGStats *pStats );
SCENEB2_EXPORT bool Render(
		const CDBID &dbid, const SOptions &options, const SOverride *pOverride, SResult *pResult, std::string *pszError );
} // namespace NUnitIconRendering
