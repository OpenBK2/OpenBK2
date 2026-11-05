#include "stdafx.h"

#include "3DLib/Transform.h"
#include "3Dmotor/FrameTransition.h"
#include "3Dmotor/GAnimation.hpp"
#include "3Dmotor/GfxBuffers.h"
#include "3Dmotor/GRTShare.h"
#include "3Dmotor/GView.h"
#include "Camera.h"
#include "Image/Image.h"
#include "Image/ImageDDS.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "Scene.h"
#include "Stats_B2_M1/DBVisObj.h"
#include "Stats_B2_M1/RPGStats.h"
#include "Stats_B2_M1/SceneModes.h"
#include "Stats_B2_M1/Vis2AI.h"
#include "System/Commands.h"
#include "System/Dg.h"
#include "System/VFSOperations.h"
#include "System/XMLReader.h"
#include "UnitIconOverrides.h"
#include "UnitIconRendering.h"

#include <cstdint>
#include <locale>
#include <memory>
#include <sstream>
#include <stdexcept>

#include <fmt/format.h>

namespace NUnitIconRendering
{
namespace
{
	std::string Lower( const std::string &szValue )
	{
		std::string szRes;
		NStr::ToLower( &szRes, szValue );
		return szRes;
	}

	bool ReadVFSFile( const std::string &szPath, std::string *pRes )
	{
		std::unique_ptr<CDataStream> pStream( NVFS::GetMainVFS()->OpenFile( szPath ) );
		if ( !pStream || !pStream->IsOk() )
		{
			return false;
		}
		pRes->resize( pStream->GetSize() );
		if ( !pRes->empty() )
		{
			pStream->Read( &( *pRes )[0], pRes->size() );
		}
		return true;
	}

	float ReadFloatChild( const NXml::CXmlNode *pNode, const char *pszName )
	{
		const NXml::CXmlNode *pChild = pNode ? pNode->FindChild( pszName ) : 0;
		if ( !pChild )
		{
			return 0.0f;
		}
		return NStr::ToFloat( pChild->GetValue().ToString() );
	}

	// CameraPlacementIconShot is [hidden; noCode] in RPGStats.cll, so the game's
	// SMechUnitRPGStats has no member for it: it is read from the record's XML.
	bool ReadCameraShotXML( const std::string &szStatsPath, SCameraShot *pShot )
	{
		std::string szText;
		if ( !ReadVFSFile( szStatsPath, &szText ) )
		{
			return false;
		}
		CPtr<NXml::CXmlReader> pReader = new NXml::CXmlReader( szText.data(), szText.data() + szText.size() );
		const NXml::CXmlNode *pRoot = pReader->GetRootElement();
		if ( !pRoot )
		{
			return false;
		}
		const NXml::CXmlNode *pStats = pRoot->FindChild( "MechUnitRPGStats" );
		if ( !pStats )
		{
			pStats = pRoot;
		}
		const NXml::CXmlNode *pShotNode = pStats->FindChild( "CameraPlacementIconShot" );
		if ( !pShotNode )
		{
			return false;
		}
		const NXml::CXmlNode *pAnchor = pShotNode->FindChild( "Anchor" );
		pShot->vAnchor =
				CVec3( ReadFloatChild( pAnchor, "x" ), ReadFloatChild( pAnchor, "y" ), ReadFloatChild( pAnchor, "z" ) );
		pShot->fDistance = ReadFloatChild( pShotNode, "Distance" );
		pShot->fPitch = ReadFloatChild( pShotNode, "Pitch" );
		pShot->fYaw = ReadFloatChild( pShotNode, "Yaw" );
		pShot->fFov = ReadFloatChild( pShotNode, "Fov" );
		// RestoreCameraParametersFromStats treats a zero distance as "never saved"
		return pShot->fDistance > FP_EPSILON;
	}

	const NDb::SModel *GetSummerModel( const NDb::SMechUnitRPGStats *pStats )
	{
		// The model editor's choice (CModelState::UpdateModels): the summer model,
		// or the first one when there is none.
		const NDb::SVisObj *pVisObj = pStats->pvisualObject;
		if ( !pVisObj || pVisObj->models.empty() )
		{
			return 0;
		}
		for ( const auto &model : pVisObj->models )
		{
			if ( model.eSeason == NDb::SEASON_SUMMER && model.pModel )
			{
				return model.pModel;
			}
		}
		return pVisObj->models.front().pModel;
	}

	// Keep render nodes and camera private: neither caller needs to replace its scene.
	struct SRenderContext
	{
		CObj<NGScene::IGameView> pView = NGScene::CreateNewView();
		CObj<ICamera> pCamera = CreateCamera();
		float fDistance = 0.0f;
	};

	bool RenderTarget( SRenderContext &render, CArray2D<NGfx::SPixel8888> *pRes, float fFov )
	{
		NGScene::CRTPtr *pTarget = NGScene::GetFrameTransitionCapture2();
		if ( !pTarget || !pTarget->GetTexture() )
		{
			return false;
		}
		render.pCamera->SetFOV( fFov );
		const float fNear = NGlobal::GetVar( "camera_near_plane", 5.0f );
		const float fFar = (std::max)( float( NGlobal::GetVar( "camera_far_plane", 500.0f ) ), render.fDistance * 2.0f );
		render.pCamera->SetPerspectiveTransform( TARGET_SIZE, TARGET_SIZE, fNear, fFar );
		MarkNewDGFrame();
		render.pCamera->Update();
		render.pView->WaitForLoad( true );
		NGScene::IGameView::SDrawInfo drawInfo;
		drawInfo.pTS = &render.pCamera->GetTransform();
		drawInfo.pTarget = pTarget->GetTexture();
		drawInfo.rtClear.vColor = CVec4( 0.5f, 0.5f, 0.5f, 0.0f );
		drawInfo.bUseDefaultClearColor = false;
		render.pView->Draw( drawInfo );
		NGfx::GetRenderTargetData( pRes, pTarget->GetTexture() );
		return pRes->GetSizeX() == TARGET_SIZE && pRes->GetSizeY() == TARGET_SIZE;
	}

	bool RenderShot( SRenderContext &render, CArray2D<NGfx::SPixel8888> *pRes, float fFov, int nWidth, int nHeight )
	{
		CArray2D<NGfx::SPixel8888> target;
		return RenderTarget( render, &target, fFov ) && CropCapture( pRes, target, nWidth, nHeight );
	}

	// Where the shipped icons put the unit, measured over all of them: the longer
	// side of the opaque part spans about 93% of the 42 pixels (fill in SOverride),
	// centred horizontally and a pixel below the middle vertically.
	const float F_ICON_CENTRE_DX = 0.5f;
	const float F_ICON_CENTRE_DY = 1.0f;

	// The silhouettes compared by FitAngle are resampled to this many cells a side.
	const int N_SILHOUETTE_SIZE = 24;

	inline int GetAlpha( const NGfx::SPixel8888 &pixel )
	{
		return pixel.a;
	}

	inline int GetAlpha( const uint32_t dwColor )
	{
		return dwColor >> 24;
	}

	// The bounding box of the opaque pixels, the end coordinates exclusive.
	struct SBox
	{
		int nX0 = 0;
		int nY0 = 0;
		int nX1 = 0;
		int nY1 = 0;

		bool IsEmpty() const
		{
			return nX1 <= nX0 || nY1 <= nY0;
		}

		float GetCentreX() const
		{
			return ( nX0 + nX1 ) / 2.0f;
		}

		float GetCentreY() const
		{
			return ( nY0 + nY1 ) / 2.0f;
		}

		int GetSize() const
		{
			return (std::max)( nX1 - nX0, nY1 - nY0 );
		}

		bool TouchesEdge( const int nSizeX, const int nSizeY ) const
		{
			return nX0 <= 0 || nY0 <= 0 || nX1 >= nSizeX || nY1 >= nSizeY;
		}
	};

	template <class TPixel>
	SBox GetOpaqueBox( const CArray2D<TPixel> &image )
	{
		SBox box;
		box.nX0 = image.GetSizeX();
		box.nY0 = image.GetSizeY();
		for ( int y = 0; y < image.GetSizeY(); ++y )
		{
			for ( int x = 0; x < image.GetSizeX(); ++x )
			{
				if ( GetAlpha( image[y][x] ) >= 128 )
				{
					box.nX0 = (std::min)( box.nX0, x );
					box.nY0 = (std::min)( box.nY0, y );
					box.nX1 = (std::max)( box.nX1, x + 1 );
					box.nY1 = (std::max)( box.nY1, y + 1 );
				}
			}
		}
		return box;
	}

	// The opaque part of the box stretched to N_SILHOUETTE_SIZE square, each cell
	// the share of it that is opaque (4x4 samples). Stretching takes the framing
	// out of the comparison, so the angle can be judged on its own.
	template <class TPixel>
	void GetSilhouette( std::vector<float> *pRes, const CArray2D<TPixel> &image, const SBox &box )
	{
		const int nSamples = 4;
		pRes->assign( N_SILHOUETTE_SIZE * N_SILHOUETTE_SIZE, 0.0f );
		const float fStepX = float( box.nX1 - box.nX0 ) / ( N_SILHOUETTE_SIZE * nSamples );
		const float fStepY = float( box.nY1 - box.nY0 ) / ( N_SILHOUETTE_SIZE * nSamples );
		for ( int cy = 0; cy < N_SILHOUETTE_SIZE; ++cy )
		{
			for ( int cx = 0; cx < N_SILHOUETTE_SIZE; ++cx )
			{
				int nOpaque = 0;
				for ( int sy = 0; sy < nSamples; ++sy )
				{
					for ( int sx = 0; sx < nSamples; ++sx )
					{
						const int x = box.nX0 + int( ( cx * nSamples + sx + 0.5f ) * fStepX );
						const int y = box.nY0 + int( ( cy * nSamples + sy + 0.5f ) * fStepY );
						if ( GetAlpha( image[y][x] ) >= 128 )
						{
							++nOpaque;
						}
					}
				}
				( *pRes )[cy * N_SILHOUETTE_SIZE + cx] = float( nOpaque ) / ( nSamples * nSamples );
			}
		}
	}

	// 1 for the same shape, towards 0 the less alike: the overlap of the two
	// stretched silhouettes, scaled down by the difference of their proportions,
	// which the stretching hid.
	float
	CompareSilhouettes( const std::vector<float> &a, const SBox &boxA, const std::vector<float> &b, const SBox &boxB )
	{
		float fMin = 0.0f;
		float fMax = 0.0f;
		for ( int i = 0; i < a.size(); ++i )
		{
			fMin += (std::min)( a[i], b[i] );
			fMax += (std::max)( a[i], b[i] );
		}
		if ( fMax <= 0.0f )
		{
			return 0.0f;
		}
		const float fAspectA = float( boxA.nX1 - boxA.nX0 ) / ( boxA.nY1 - boxA.nY0 );
		const float fAspectB = float( boxB.nX1 - boxB.nX0 ) / ( boxB.nY1 - boxB.nY0 );
		const float fAspect = (std::min)( fAspectA, fAspectB ) / (std::max)( fAspectA, fAspectB );
		return fMin / fMax * fAspect;
	}

	void ApplyShot( SRenderContext &render, const SCameraShot &shot )
	{
		render.fDistance = shot.fDistance;
		render.pCamera->SetAnchor( shot.vAnchor );
		render.pCamera->SetPlacement( shot.fDistance, shot.fPitch, shot.fYaw );
	}

	bool RenderBox( SRenderContext &render, SBox *pBox, const SCameraShot &shot, const float fFov )
	{
		ApplyShot( render, shot );
		CArray2D<NGfx::SPixel8888> target;
		if ( !RenderTarget( render, &target, fFov ) )
		{
			return false;
		}
		*pBox = GetOpaqueBox( target );
		return true;
	}

	// Moves the camera until the unit sits in the 42 pixel window the way the
	// shipped icons have it. The window is looked at nZoom times enlarged, so the
	// placement is measured to a fraction of an icon pixel. Distance sets the size;
	// the anchor, moved across the ground, sets the position, through the screen
	// shift a small move of it causes, measured each time rather than worked out
	// from the camera's axes.
	void FrameShotAtZoom( SRenderContext &render, SCameraShot *pShot, const float fFill, const int nZoom )
	{
		const float fFov = GetWindowFov( pShot->fFov, ICON_SIZE * nZoom );
		const float fWantSize = fFill * ICON_SIZE * nZoom;
		const float fWantX = TARGET_SIZE / 2.0f + F_ICON_CENTRE_DX * nZoom;
		const float fWantY = TARGET_SIZE / 2.0f + F_ICON_CENTRE_DY * nZoom;
		for ( int nStep = 0; nStep < 16; ++nStep )
		{
			SBox box;
			if ( !RenderBox( render, &box, *pShot, fFov ) )
			{
				return;
			}
			if ( box.IsEmpty() || box.TouchesEdge( TARGET_SIZE, TARGET_SIZE ) )
			{
				// Out of view or bigger than the target: step back and look again
				pShot->fDistance *= 1.5f;
				continue;
			}
			const float fErrX = box.GetCentreX() - fWantX;
			const float fErrY = box.GetCentreY() - fWantY;
			const float fSizeRatio = box.GetSize() / fWantSize;
			if ( std::fabs( box.GetSize() - fWantSize ) < 1.0f && std::fabs( fErrX ) < 1.0f && std::fabs( fErrY ) < 1.0f )
			{
				return;
			}
			// The screen shift per unit of anchor movement along x and along y
			const float fProbe = pShot->fDistance * 0.02f;
			SCameraShot probe = *pShot;
			SBox boxX;
			SBox boxY;
			probe.vAnchor.x += fProbe;
			const bool bX = RenderBox( render, &boxX, probe, fFov ) && !boxX.IsEmpty();
			probe.vAnchor.x -= fProbe;
			probe.vAnchor.y += fProbe;
			const bool bY = RenderBox( render, &boxY, probe, fFov ) && !boxY.IsEmpty();
			if ( bX && bY )
			{
				const float a = ( boxX.GetCentreX() - box.GetCentreX() ) / fProbe;
				const float c = ( boxX.GetCentreY() - box.GetCentreY() ) / fProbe;
				const float b = ( boxY.GetCentreX() - box.GetCentreX() ) / fProbe;
				const float d = ( boxY.GetCentreY() - box.GetCentreY() ) / fProbe;
				const float fDet = a * d - b * c;
				if ( std::fabs( fDet ) > FP_EPSILON )
				{
					pShot->vAnchor.x += ( -fErrX * d + fErrY * b ) / fDet;
					pShot->vAnchor.y += ( -fErrY * a + fErrX * c ) / fDet;
				}
			}
			// The unit's size on screen goes as one over the distance
			pShot->fDistance *= fSizeRatio;
		}
	}

	void FrameShot( SRenderContext &render, SCameraShot *pShot, const float fFill )
	{
		// Coarse at the editor's own scale, which sees a unit far out of place,
		// then fine
		FrameShotAtZoom( render, pShot, fFill, 1 );
		FrameShotAtZoom( render, pShot, fFill, 4 );
	}

	// Turns the camera to the yaw and pitch whose silhouette best matches the
	// shipped icon: a coarse sweep, then a fine one around the best. The unit must
	// already be framed, so it stays in view while the camera turns.
	void FitAngle( SRenderContext &render, SCameraShot *pShot, const CArray2D<uint32_t> &original )
	{
		const SBox originalBox = GetOpaqueBox( original );
		if ( originalBox.IsEmpty() )
		{
			return;
		}
		std::vector<float> originalSilhouette;
		GetSilhouette( &originalSilhouette, original, originalBox );

		// Twice the icon's size leaves room in the target for the unit to move as it turns
		const float fFov = GetWindowFov( pShot->fFov, ICON_SIZE * 2 );
		auto score = [&]( const float fYaw, const float fPitch ) -> float
		{
			SCameraShot probe = *pShot;
			probe.fYaw = fYaw;
			probe.fPitch = fPitch;
			ApplyShot( render, probe );
			CArray2D<NGfx::SPixel8888> target;
			if ( !RenderTarget( render, &target, fFov ) )
			{
				return -1.0f;
			}
			const SBox box = GetOpaqueBox( target );
			if ( box.IsEmpty() || box.TouchesEdge( target.GetSizeX(), target.GetSizeY() ) )
			{
				return -1.0f;
			}
			std::vector<float> silhouette;
			GetSilhouette( &silhouette, target, box );
			return CompareSilhouettes( silhouette, box, originalSilhouette, originalBox );
		};
		float fBestYaw = pShot->fYaw;
		float fBestPitch = pShot->fPitch;
		float fBest = score( fBestYaw, fBestPitch );
		const float fStartYaw = pShot->fYaw;
		for ( int nYaw = -45; nYaw <= 45; nYaw += 5 )
		{
			for ( int nPitch = 5; nPitch <= 50; nPitch += 5 )
			{
				const float fScore = score( fStartYaw + nYaw, float( nPitch ) );
				if ( fScore > fBest )
				{
					fBest = fScore;
					fBestYaw = fStartYaw + nYaw;
					fBestPitch = float( nPitch );
				}
			}
		}
		const float fCoarseYaw = fBestYaw;
		const float fCoarsePitch = fBestPitch;
		for ( int nYaw = -4; nYaw <= 4; ++nYaw )
		{
			for ( int nPitch = -4; nPitch <= 4; ++nPitch )
			{
				const float fScore = score( fCoarseYaw + nYaw, fCoarsePitch + nPitch );
				if ( fScore > fBest )
				{
					fBest = fScore;
					fBestYaw = fCoarseYaw + nYaw;
					fBestPitch = fCoarsePitch + nPitch;
				}
			}
		}
		pShot->fYaw = fBestYaw;
		pShot->fPitch = fBestPitch;
	}

	// Applies one unit's corrections to its shot and says which were applied, for
	// the manifest.
	std::string ApplyOverride( SRenderContext &render,
	                           SCameraShot *pShot,
	                           const SOverride &over,
	                           const CArray2D<uint32_t> *pOriginal )
	{
		std::string szApplied;
		if ( over.bSetYaw )
		{
			pShot->fYaw = over.fYaw;
			szApplied += fmt::format( " yaw={}", over.fYaw );
		}
		if ( over.bSetPitch )
		{
			pShot->fPitch = over.fPitch;
			szApplied += fmt::format( " pitch={}", over.fPitch );
		}
		if ( over.fYawDelta != 0.0f )
		{
			pShot->fYaw += over.fYawDelta;
			szApplied += fmt::format( " yaw+={}", over.fYawDelta );
		}
		if ( over.fPitchDelta != 0.0f )
		{
			pShot->fPitch += over.fPitchDelta;
			szApplied += fmt::format( " pitch+={}", over.fPitchDelta );
		}
		if ( over.fDistanceScale != 1.0f )
		{
			pShot->fDistance *= over.fDistanceScale;
			szApplied += fmt::format( " distance*={}", over.fDistanceScale );
		}
		if ( over.bAngle )
		{
			if ( pOriginal )
			{
				// Framed first, so the unit is in view and centred while it turns
				FrameShot( render, pShot, over.fFill );
				FitAngle( render, pShot, *pOriginal );
				szApplied += " angle";
			}
			else
			{
				szApplied += " angle(no shipped icon)";
			}
		}
		if ( over.bFrame || over.bAngle )
		{
			FrameShot( render, pShot, over.fFill );
			szApplied += fmt::format( " frame(fill={})", over.fFill );
		}
		if ( !szApplied.empty() )
		{
			szApplied.erase( 0, 1 );
		}
		return szApplied;
	}

} // namespace

// Box filter by nFactor with premultiplied alpha, so the grey the background is
// cleared to does not bleed into the edge of the model. Pixels that end up fully
// transparent get the editor's background grey back, as the shipped icons have.
bool Downsample( CArray2D<NGfx::SPixel8888> *pRes, const CArray2D<NGfx::SPixel8888> &src, const int nFactor )
{
	if ( !ValidDimensions( src.GetSizeX(), src.GetSizeY() ) || nFactor <= 0 || nFactor > src.GetSizeX() ||
	     nFactor > src.GetSizeY() || src.GetSizeX() % nFactor || src.GetSizeY() % nFactor )
	{
		return false;
	}
	const int nSizeX = src.GetSizeX() / nFactor;
	const int nSizeY = src.GetSizeY() / nFactor;
	pRes->SetSizes( nSizeX, nSizeY );
	const int nCount = nFactor * nFactor;
	for ( int y = 0; y < nSizeY; ++y )
	{
		for ( int x = 0; x < nSizeX; ++x )
		{
			// A 1x1 export can average the whole 512x512 target; RGB sums exceed 32 bits.
			uint64_t r = 0;
			uint64_t g = 0;
			uint64_t b = 0;
			uint64_t a = 0;
			for ( int dy = 0; dy < nFactor; ++dy )
			{
				for ( int dx = 0; dx < nFactor; ++dx )
				{
					const NGfx::SPixel8888 &p = src[y * nFactor + dy][x * nFactor + dx];
					r += p.r * p.a;
					g += p.g * p.a;
					b += p.b * p.a;
					a += p.a;
				}
			}
			NGfx::SPixel8888 &res = ( *pRes )[y][x];
			if ( a == 0 )
			{
				res.dwColor = 0x00808080;
			}
			else
			{
				res.r = ( r + a / 2 ) / a;
				res.g = ( g + a / 2 ) / a;
				res.b = ( b + a / 2 ) / a;
				res.a = ( a + nCount / 2 ) / nCount;
			}
		}
	}
	return true;
}

bool ParseOverrides( const std::string &szText, std::map<std::string, SOverride> *pRes, std::string *pszError )
{
	// Parse transactionally: a bad custom file must not leave half its corrections active.
	std::map<std::string, SOverride> parsed;
	std::vector<std::string> lines;
	NStr::SplitString( szText, &lines, '\n' );
	for ( int nLine = 0; nLine < lines.size(); ++nLine )
	{
		std::string szLine = lines[nLine];
		const size_t nComment = szLine.find( '#' );
		if ( nComment != std::string::npos )
		{
			szLine.resize( nComment );
		}
		// The path is everything up to its .xdb, so it may hold spaces
		const size_t nXDB = Lower( szLine ).find( ".xdb" );
		if ( nXDB == std::string::npos )
		{
			if ( szLine.find_first_not_of( " \t\r" ) != std::string::npos )
			{
				*pszError = fmt::format( "line {}: no stats path", nLine + 1 );
				return false;
			}
			continue;
		}
		std::string szPath = szLine.substr( 0, nXDB + 4 );
		szPath.erase( 0, szPath.find_first_not_of( " \t" ) );
		NStr::ReplaceAllChars( &szPath, '\\', '/' );
		SOverride &over = parsed[Lower( szPath )];
		std::string szRest = szLine.substr( nXDB + 4 );
		NStr::ReplaceAllChars( &szRest, '\t', ' ' );
		NStr::ReplaceAllChars( &szRest, '\r', ' ' );
		std::vector<std::string> tokens;
		NStr::SplitString( szRest, &tokens, ' ' );
		for ( const std::string &szToken : tokens )
		{
			if ( szToken.empty() )
			{
				continue;
			}
			const size_t nEq = szToken.find( '=' );
			const std::string szKey = szToken.substr( 0, nEq );
			float fValue = 0.0f;
			const bool bFlag = szKey == "frame" || szKey == "angle";
			const bool bLike = szKey == "like";
			bool bValid = bFlag ? nEq == std::string::npos : nEq != std::string::npos && nEq + 1 < szToken.size();
			if ( bValid && !bFlag && !bLike )
			{
				std::istringstream value( szToken.substr( nEq + 1 ) );
				value.imbue( std::locale::classic() );
				bValid = bool( value >> fValue ) && value.eof() && std::isfinite( fValue );
				if ( szKey == "fill" )
				{
					bValid = bValid && fValue > 0.0f && fValue <= 1.0f;
				}
				if ( szKey == "distance*" )
				{
					bValid = bValid && fValue > 0.0f;
				}
			}
			if ( !bValid )
			{
				*pszError = fmt::format( "line {}: invalid correction {}", nLine + 1, szToken );
				return false;
			}
			if ( szKey == "frame" )
			{
				over.bFrame = true;
			}
			else if ( szKey == "angle" )
			{
				over.bAngle = true;
			}
			else if ( szKey == "fill" )
			{
				over.fFill = fValue;
			}
			else if ( szKey == "yaw" )
			{
				over.bSetYaw = true;
				over.fYaw = fValue;
			}
			else if ( szKey == "pitch" )
			{
				over.bSetPitch = true;
				over.fPitch = fValue;
			}
			else if ( szKey == "yaw+" )
			{
				over.fYawDelta = fValue;
			}
			else if ( szKey == "pitch+" )
			{
				over.fPitchDelta = fValue;
			}
			else if ( szKey == "distance*" )
			{
				over.fDistanceScale = fValue;
			}
			else if ( szKey == "like" && nEq != std::string::npos )
			{
				over.szLike = szToken.substr( nEq + 1 );
			}
			else
			{
				*pszError = fmt::format( "line {}: unknown correction {}", nLine + 1, szToken );
				return false;
			}
		}
	}
	pRes->swap( parsed );
	return true;
}

bool LoadOverrides( const std::string &szFileName, std::map<std::string, SOverride> *pRes, std::string *pszError )
{
	CFileStream file( szFileName, CFileStream::WIN_READ_ONLY );
	if ( !file.IsOk() )
	{
		*pszError = "cannot read " + szFileName;
		return false;
	}
	std::string szText( file.GetSize(), '\0' );
	if ( !szText.empty() )
	{
		file.Read( &szText[0], szText.size() );
	}
	if ( !ParseOverrides( szText, pRes, pszError ) )
	{
		*pszError = szFileName + ": " + *pszError;
		return false;
	}
	return true;
}

const std::map<std::string, SOverride> &GetDefaultOverrides()
{
	// Compiled from the original text file, so installations need no extra data files.
	static const std::map<std::string, SOverride> defaults = []
	{
		std::map<std::string, SOverride> result;
		std::string error;
		if ( !ParseOverrides( SZ_UNIT_ICON_OVERRIDES, &result, &error ) )
		{
			throw std::runtime_error( "invalid bundled icon corrections: " + error );
		}
		return result;
	}();
	return defaults;
}

const SOverride *FindOverride( const std::map<std::string, SOverride> &overrides, const CDBID &dbid )
{
	std::string path = dbid.ToString();
	NStr::ReplaceAllChars( &path, '\\', '/' );
	const auto it = overrides.find( Lower( path ) );
	return it == overrides.end() ? nullptr : &it->second;
}

bool ReadCameraShot( const CDBID &dbid, bool bEditorDatabase, SCameraShot *pShot )
{
	*pShot = SCameraShot();
	if ( bEditorDatabase )
	{
		// Read live metadata, including unsaved editor changes; game structs omit these fields.
		NDb::IObjMan *pManipulator = NDb::GetManipulator( dbid );
		if ( !pManipulator || !pManipulator->GetValue( "CameraPlacementIconShot.Anchor", &pShot->vAnchor ) ||
		     !pManipulator->GetValue( "CameraPlacementIconShot.Distance", &pShot->fDistance ) ||
		     !pManipulator->GetValue( "CameraPlacementIconShot.Pitch", &pShot->fPitch ) ||
		     !pManipulator->GetValue( "CameraPlacementIconShot.Yaw", &pShot->fYaw ) ||
		     !pManipulator->GetValue( "CameraPlacementIconShot.Fov", &pShot->fFov ) )
		{
			return false;
		}
	}
	else if ( !ReadCameraShotXML( dbid.ToString(), pShot ) )
	{
		return false;
	}
	pShot->bFromStats = std::isfinite( pShot->vAnchor.x ) && std::isfinite( pShot->vAnchor.y ) &&
	                    std::isfinite( pShot->vAnchor.z ) && std::isfinite( pShot->fDistance ) &&
	                    std::isfinite( pShot->fPitch ) && std::isfinite( pShot->fYaw ) && std::isfinite( pShot->fFov ) &&
	                    pShot->fDistance > FP_EPSILON && pShot->fFov > FP_EPSILON && pShot->fFov < 179.0f;
	return pShot->bFromStats;
}

bool Render(
		const CDBID &dbid, const SOptions &options, const SOverride *pOverride, SResult *pResult, std::string *pszError )
{
	*pResult = SResult();
	pszError->clear();
	const int nSupersample = GetSupersample( options.nWidth, options.nHeight, options.nSupersample );
	if ( nSupersample == 0 )
	{
		*pszError = "invalid icon size or supersampling factor";
		return false;
	}
	const NDb::SMechUnitRPGStats *pStats = NDb::Get<NDb::SMechUnitRPGStats>( dbid );
	if ( !pStats )
	{
		*pszError = "no mech unit stats";
		return false;
	}
	const CDBID modelDBID = pOverride && !pOverride->szLike.empty() ? CDBID( pOverride->szLike ) : dbid;
	const NDb::SMechUnitRPGStats *pModelStats = NDb::Get<NDb::SMechUnitRPGStats>( modelDBID );
	const NDb::SModel *pModel = pModelStats ? GetSummerModel( pModelStats ) : nullptr;
	if ( !pModel || !pModel->pGeometry )
	{
		*pszError = "no usable model or geometry for " + modelDBID.ToString();
		return false;
	}
	NGScene::CRTPtr *pTarget = NGScene::GetFrameTransitionCapture2();
	if ( !pTarget || !pTarget->GetTexture() || !NGScene::Is3DActive() )
	{
		*pszError = "icon render target is unavailable";
		return false;
	}
	const NDb::SAmbientLight *pLight = NDb::Get<NDb::SAmbientLight>( CDBID( options.szLight ) );
	if ( !pLight )
	{
		*pszError = "no icon lighting: " + options.szLight;
		return false;
	}
	SRenderContext render;
	render.pView->SetAmbient( pLight );
	// Match Scene::AddObject: queue every mesh/material before creating the render nodes.
	// Otherwise the first capture can race lazy resource loading and contain only the clear color.
	CObj<CObjectBase> pPrecache = render.pView->Precache( pModel );
	render.pCamera->SetHandleType( true );
	render.pCamera->SwitchAutoPositioning( false );
	render.pCamera->SwitchManualScrolling( "IconRendering", false );
	const float fGridCentre = Clamp<int>( options.nGridSize, 8, 64 ) * VIS_TILE_SIZE * 0.5f;
	const CVec3 vPosition =
			CVec3( fGridCentre, fGridCentre, 0 ) + CVec3( VIS_TILE_SIZE / 8.0f, VIS_TILE_SIZE / 8.0f, VIS_TILE_SIZE / 8.0f );
	const SFBTransform transform = MakeTransform( vPosition );
	const bool bAnimated = pModel->pSkeleton && !pModel->pSkeleton->animations.empty();
	CObj<NAnimation::ISkeletonAnimator> pAnimator;
	CObj<CCSTime> pTime = new CCSTime();
	pTime->Set( 0 );
	CObj<CCSFBTransform> pTransform = new CCSFBTransform();
	pTransform->Set( transform );
	if ( bAnimated )
	{
		NAnimation::SGrannySkeletonHandle handle;
		handle.pSkeleton = pModel->pSkeleton;
		handle.nModelInFile = 0;
		pAnimator = NAnimation::CreateSkeletonAnimator( handle, pTime );
		if ( !pAnimator )
		{
			*pszError = "could not load the model skeleton";
			return false;
		}
		pAnimator->SetGlobalTransform( pTransform );
	}
	NGScene::IGameView::SMeshInfo meshInfo;
	render.pView->CreateMeshInfo( pModel, &meshInfo, bAnimated );
	SBound bound;
	bound.BoxExInit( vPosition + pModel->pGeometry->vCenter, pModel->pGeometry->vSize * 0.5f * FP_SQRT_2 );
	CObj<CObjectBase> pMesh =
			render.pView->CreateMesh( meshInfo, pTransform.GetPtr(), bound, NGScene::CMeshAnimStuff( pModel, pAnimator ) );
	if ( !pMesh )
	{
		*pszError = "could not create the model render mesh";
		return false;
	}
	// WaitForLoad only sets a flag for Draw. Finish loading here, then let RenderTarget
	// advance the dependency-graph frame so bounds/transforms see the loaded geometry.
	render.pView->LoadEverything();

	SCameraShot &shot = pResult->shot;
	if ( !ReadCameraShot( modelDBID, options.bEditorDatabase, &shot ) )
	{
		// Fit the geometry's bounding sphere into the old 42-pixel window.
		// This conservative fit also covers tall units and long gun barrels.
		shot.vAnchor = vPosition + pModel->pGeometry->vCenter;
		shot.fPitch = 20.0f;
		shot.fYaw = 205.2f;
		shot.fFov = 54.0f;
		const float fRadius = (std::max)( 0.1f, fabs( pModel->pGeometry->vSize ) * 0.5f );
		const float fHalfAngle = std::atan( std::tan( ToRadian( shot.fFov ) * 0.5f ) * ICON_SIZE / float( TARGET_SIZE ) );
		shot.fDistance = fRadius / std::sin( fHalfAngle ) / 0.9f;
	}
	if ( pOverride )
	{
		if ( !pOverride->szLike.empty() )
		{
			pResult->szCorrections = "like=" + pOverride->szLike;
		}
		CArray2D<uint32_t> original;
		bool bOriginal = false;
		if ( pOverride->bAngle && pStats->pIconTexture )
		{
			// Follow the texture's real destination; exported DDS files need not sit beside the record.
			std::unique_ptr<CDataStream> stream( NVFS::GetMainVFS()->OpenFile( pStats->pIconTexture->szDestName ) );
			bOriginal = stream && stream->IsOk() && NImage::LoadImageDDS( &original, stream.get() );
		}
		const std::string applied = ApplyOverride( render, &shot, *pOverride, bOriginal ? &original : nullptr );
		if ( !applied.empty() )
		{
			pResult->szCorrections += ( pResult->szCorrections.empty() ? "" : " " ) + applied;
		}
	}
	ApplyShot( render, shot );
	if ( options.bLegacyImage && !RenderShot( render, &pResult->legacyImage, shot.fFov, ICON_SIZE, ICON_SIZE ) )
	{
		*pszError = "legacy icon readback failed";
		return false;
	}
	const int nWidth = options.nWidth * nSupersample;
	const int nHeight = options.nHeight * nSupersample;
	CArray2D<NGfx::SPixel8888> capture;
	if ( !RenderShot( render, &capture, GetWindowFov( shot.fFov, (std::min)( nWidth, nHeight ) ), nWidth, nHeight ) ||
	     !Downsample( &pResult->image, capture, nSupersample ) )
	{
		*pszError = "icon readback failed";
		return false;
	}
	for ( int y = 0; y < pResult->image.GetSizeY(); ++y )
	{
		for ( int x = 0; x < pResult->image.GetSizeX(); ++x )
		{
			if ( pResult->image[y][x].a != 0 )
			{
				return true;
			}
		}
	}
	*pszError = "rendered icon is empty; check the model and saved camera";
	return false;
}
} // namespace NUnitIconRendering
