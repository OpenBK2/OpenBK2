// IconRender: re-renders the unit icons through the game's own renderer.
//
// The shipped icon of a vehicle is a 42x42 picture the model editor made
// (ED_B2_M1/IconsCreation.cpp, CModelEditor::CreateIcon): it drew the unit into
// the 512x512 FrameTransitionSrc2 target over a grey background with zero
// alpha and cut out the centre 42x42. The camera it used is kept in the hidden
// CameraPlacementIconShot field of every MechUnitRPGStats record, so the shot
// can be replayed for all units at once.
//
// For each unit this writes, under <out>/<unit folder>/:
//   orig.dds  the shipped icon, copied out of the VFS unchanged
//   x1.tga    the editor's shot reproduced exactly: same target, same FOV, same
//             42x42 crop. Comparing it with orig.dds says whether the scene
//             (light, model, camera) matches what the editor had
//   hq.tga    the same view at <scale> times the size. The FOV is narrowed so
//             the 42 pixel window fills <scale>*<supersample> pixels of the
//             target, and the result is box filtered down by <supersample>
// and one line per unit in <out>/manifest.tsv.
//
// scripts/iconrender/icon_report.py turns that into a comparison page.
//
// The data comes from the installation the executable is in (or --install),
// and the profiles are the user's own, as for the game.
//
//   IconRender [--out=DIR] [--filter=TEXT] [--scale=N] [--supersample=N]
//              [--light=DBID] [--limit=N] [--mod=NAME] [--overrides=FILE]
//              [--install=DIR]
//
// --mod attaches a folder under MODs, as the game's own --mod does.
//
// --install takes the data from another installation than the one the
// executable is in, such as a Fall of the Reich install next to the original
// game. The profiles stay the user's own.
//
// --overrides names a text file of per-unit corrections, for the few units
// whose saved camera does not suit (or that never had one). One unit per line:
// the stats path, then any of
//   frame           place the camera so the unit fills the window the way the
//                   shipped icons do: its longer side about 93% of it, centred
//   fill=F          with frame, the share of the window the longer side fills
//   angle           search yaw and pitch for the view that best matches the
//                   silhouette of the shipped icon
//   yaw=A pitch=A   set the angle, in degrees as the stats store them
//   yaw+=D pitch+=D turn the saved angle by D degrees
//   distance*=K     move the camera K times further out
//   like=PATH       draw another unit's model with that unit's saved camera,
//                   for a record whose own visual object is not a vehicle
//                   (tm_2_12_super shows the railway gun's shell and borrows
//                   tm_2_12's icon); the other corrections then apply on top
// Everything after a # is a comment. Whatever order they are written in, like=
// picks the model and camera first, then the angle is set, turned and moved
// out, then searched (angle), and framing comes last.

#include "stdafx.h"

#include "3Dmotor/FrameTransition.h"
#include "3Dmotor/Gfx.h"
#include "3Dmotor/GfxBuffers.h"
#include "3Dmotor/GRTShare.h"
#include "3Dmotor/GView.h"
#include "AILogic/CreateAI.h"
#include "Image/Image.h" // EImageType, which ImageDDS.h uses without including
#include "Image/ImageDDS.h"
#include "Image/ImageTGA.h"
#include "libdb/Db.h"
#include "Main/MainLoop.h"
#include "Main/MODs.h"
#include "Main/Profiles.h"
#include "Misc/2Darray.h"
#include "Misc/StrProc.h"
#include "port/messagebox.h"
#include "SceneB2/Camera.h"
#include "SceneB2/Scene.h"
#include "Stats_B2_M1/AnimModes.h"
#include "Stats_B2_M1/DBVisObj.h"
#include "Stats_B2_M1/RPGStats.h"
#include "Stats_B2_M1/SceneModes.h"
#include "Stats_B2_M1/Vis2AI.h"
#include "System/Dg.h"
#include "System/FilePath.h"
#include "System/GeneralSettings.h"
#include "System/GResource.h"
#include "System/PlatformPaths.h"
#include "System/Text.h"
#include "System/VFSOperations.h"
#include "System/WinFrame.h"
#include "System/XMLReader.h"

#include "GameX_export.h"

#include <cmath>
#include <cstdint>
#include <map>
#include <memory>

#if BOOST_OS_WINDOWS
#include <shellapi.h> // CommandLineToArgvW
#endif

#include <fmt/format.h>

namespace NGameX
{
bool Initialize();
}

namespace
{

// The editor's shot: the size of the target it drew into and of the crop it kept
// (N_TARGET_SIZE_X and N_ICONS_SIZE_X in ED_B2_M1/IconsCreation.cpp).
const int N_TARGET_SIZE = 512;
const int N_ICON_SIZE = 42;

// Where the model editor puts the model: the middle of its default 32 tile grid,
// moved by CModelEditorSettings::vShift. The saved anchors are relative to this.
const int N_EDITOR_GRID_SIZE = 32;

// The light the model editor falls back to when none is picked: the Misc.Light
// entry of Editor/EditorOptions.xdb.
const char *const SZ_DEFAULT_LIGHT = "Scene/Lights/All/Summer/Daylight_AmbientLight.xdb";

struct SOptions
{
	std::string szOutDir;
	std::string szFilter;
	std::string szLight = SZ_DEFAULT_LIGHT;
	int nScale = 4;
	int nSupersample = 3;
	int nLimit = 0;
	std::string szMod;
	std::string szOverrides;
	std::string szInstall;
};

// One line of the --overrides file, see the comment at the top.
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

struct SCameraShot
{
	CVec3 vAnchor = VNULL3;
	float fDistance = 0.0f;
	float fPitch = 0.0f;
	float fYaw = 0.0f;
	float fFov = 0.0f;
	bool bFromStats = false;
};

std::string Lower( const std::string &szValue )
{
	std::string szRes;
	NStr::ToLower( &szRes, szValue );
	return szRes;
}

bool EndsWithNoCase( const std::string &szValue, const std::string &szTail )
{
	if ( szValue.size() < szTail.size() )
	{
		return false;
	}
	return Lower( szValue.substr( szValue.size() - szTail.size() ) ) == Lower( szTail );
}

bool ParseInt( const std::string &szValue, int *pnRes )
{
	if ( szValue.empty() || !NStr::IsDecNumber( szValue ) )
	{
		return false;
	}
	*pnRes = NStr::ToInt( szValue );
	return true;
}

bool ParseCommandLine( const std::vector<std::string> &arguments, SOptions *pOptions, std::string *pszError )
{
	for ( const std::string &szArg : arguments )
	{
		const size_t nEq = szArg.find( '=' );
		const std::string szName = szArg.substr( 0, nEq );
		const std::string szValue = nEq == std::string::npos ? std::string() : szArg.substr( nEq + 1 );
		bool bOk = true;
		if ( szName == "--out" )
		{
			pOptions->szOutDir = szValue;
		}
		else if ( szName == "--filter" )
		{
			pOptions->szFilter = szValue;
		}
		else if ( szName == "--light" )
		{
			pOptions->szLight = szValue;
		}
		else if ( szName == "--scale" )
		{
			bOk = ParseInt( szValue, &pOptions->nScale ) && pOptions->nScale >= 1;
		}
		else if ( szName == "--supersample" )
		{
			bOk = ParseInt( szValue, &pOptions->nSupersample ) && pOptions->nSupersample >= 1;
		}
		else if ( szName == "--mod" )
		{
			pOptions->szMod = szValue;
		}
		else if ( szName == "--overrides" )
		{
			pOptions->szOverrides = szValue;
		}
		else if ( szName == "--install" )
		{
			pOptions->szInstall = szValue;
		}
		else if ( szName == "--limit" )
		{
			bOk = ParseInt( szValue, &pOptions->nLimit ) && pOptions->nLimit >= 0;
		}
		else
		{
			bOk = false;
		}
		if ( !bOk )
		{
			*pszError = "bad argument: " + szArg;
			return false;
		}
	}
	// The high resolution window has to fit in the one target the scene can draw
	// into, see RenderShot.
	if ( N_ICON_SIZE * pOptions->nScale * pOptions->nSupersample > N_TARGET_SIZE )
	{
		*pszError = fmt::format( "--scale={} --supersample={} needs a {} pixel window, the target is {}",
		                         pOptions->nScale,
		                         pOptions->nSupersample,
		                         N_ICON_SIZE * pOptions->nScale * pOptions->nSupersample,
		                         N_TARGET_SIZE );
		return false;
	}
	return true;
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
bool ReadCameraShot( const std::string &szStatsPath, SCameraShot *pShot )
{
	std::string szText;
	if ( !ReadVFSFile( szStatsPath, &szText ) )
	{
		return false;
	}
	CPtr<NXml::CXmlReader> pReader = new NXml::CXmlReader( szText.data(), szText.data() + szText.size() );
	const NXml::CXmlNode *pRoot = pReader->GetRootElement();
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

// The most common placement among the units that have one, for the few that do
// not. Marked in the manifest, since the framing is a guess.
void SetFallbackShot( SCameraShot *pShot )
{
	pShot->vAnchor = CVec3( 44.5392f, 44.0619f, 1.44657f );
	pShot->fDistance = 61.5f;
	pShot->fPitch = 15.0f;
	pShot->fYaw = 205.2f;
	pShot->fFov = 54.0f;
	pShot->bFromStats = false;
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
		if ( model.eSeason == NDb::SEASON_SUMMER )
		{
			return model.pModel;
		}
	}
	return pVisObj->models.front().pModel;
}

// Draws the scene into the editor's target, the whole of it. fFov is the full
// field of view of the target in degrees.
bool RenderTarget( CArray2D<NGfx::SPixel8888> *pRes, const float fFov )
{
	NGScene::CRTPtr *pTarget = NGScene::GetFrameTransitionCapture2();
	if ( !pTarget || !pTarget->GetTexture() )
	{
		return false;
	}
	Camera()->SetFOV( fFov );
	// Load everything the frame needs before drawing it, rather than drawing
	// placeholders while the loading thread catches up.
	Scene()->GetGView()->WaitForLoad( true );
	Scene()->Draw( pTarget );
	NGfx::GetRenderTargetData( pRes, pTarget->GetTexture() );
	return pRes->GetSizeX() > 0 && pRes->GetSizeY() > 0;
}

// Draws the scene into the editor's target and returns the centre nCrop x nCrop
// pixels.
bool RenderShot( CArray2D<NGfx::SPixel8888> *pRes, const float fFov, const int nCrop )
{
	CArray2D<NGfx::SPixel8888> target;
	if ( !RenderTarget( &target, fFov ) )
	{
		return false;
	}
	if ( target.GetSizeX() < nCrop || target.GetSizeY() < nCrop )
	{
		return false;
	}
	const int nStartX = ( target.GetSizeX() - nCrop ) / 2;
	const int nStartY = ( target.GetSizeY() - nCrop ) / 2;
	pRes->SetSizes( nCrop, nCrop );
	for ( int y = 0; y < nCrop; ++y )
	{
		for ( int x = 0; x < nCrop; ++x )
		{
			( *pRes )[y][x] = target[y + nStartY][x + nStartX];
		}
	}
	return true;
}

// Box filter by nFactor with premultiplied alpha, so the grey the background is
// cleared to does not bleed into the edge of the model. Pixels that end up fully
// transparent get the editor's background grey back, as the shipped icons have.
void Downsample( CArray2D<NGfx::SPixel8888> *pRes, const CArray2D<NGfx::SPixel8888> &src, const int nFactor )
{
	const int nSizeX = src.GetSizeX() / nFactor;
	const int nSizeY = src.GetSizeY() / nFactor;
	pRes->SetSizes( nSizeX, nSizeY );
	const int nCount = nFactor * nFactor;
	for ( int y = 0; y < nSizeY; ++y )
	{
		for ( int x = 0; x < nSizeX; ++x )
		{
			uint32_t r = 0;
			uint32_t g = 0;
			uint32_t b = 0;
			uint32_t a = 0;
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
float CompareSilhouettes( const std::vector<float> &a, const SBox &boxA, const std::vector<float> &b, const SBox &boxB )
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

void ApplyShot( const SCameraShot &shot )
{
	Camera()->SetAnchor( shot.vAnchor );
	Camera()->SetPlacement( shot.fDistance, shot.fPitch, shot.fYaw );
}

// The FOV that shows the editor's 42 pixel window over nWindow pixels of the
// target: a centred crop of a perspective view is the view through a narrower
// lens, whose half angle has the tangent scaled by the ratio of the two widths.
float GetWindowFov( const float fFov, const int nWindow )
{
	const float fHalfTan = std::tan( ToRadian( fFov ) / 2.0f ) * N_ICON_SIZE / float( nWindow );
	return ToDegree( 2.0f * std::atan( fHalfTan ) );
}

bool RenderBox( SBox *pBox, const SCameraShot &shot, const float fFov )
{
	ApplyShot( shot );
	CArray2D<NGfx::SPixel8888> target;
	if ( !RenderTarget( &target, fFov ) )
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
void FrameShotAtZoom( SCameraShot *pShot, const float fFill, const int nZoom )
{
	const float fFov = GetWindowFov( pShot->fFov, N_ICON_SIZE * nZoom );
	const float fWantSize = fFill * N_ICON_SIZE * nZoom;
	const float fWantX = N_TARGET_SIZE / 2.0f + F_ICON_CENTRE_DX * nZoom;
	const float fWantY = N_TARGET_SIZE / 2.0f + F_ICON_CENTRE_DY * nZoom;
	for ( int nStep = 0; nStep < 16; ++nStep )
	{
		SBox box;
		if ( !RenderBox( &box, *pShot, fFov ) )
		{
			return;
		}
		if ( box.IsEmpty() || box.TouchesEdge( N_TARGET_SIZE, N_TARGET_SIZE ) )
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
		const bool bX = RenderBox( &boxX, probe, fFov ) && !boxX.IsEmpty();
		probe.vAnchor.x -= fProbe;
		probe.vAnchor.y += fProbe;
		const bool bY = RenderBox( &boxY, probe, fFov ) && !boxY.IsEmpty();
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

void FrameShot( SCameraShot *pShot, const float fFill )
{
	// Coarse at the editor's own scale, which sees a unit far out of place,
	// then fine
	FrameShotAtZoom( pShot, fFill, 1 );
	FrameShotAtZoom( pShot, fFill, 4 );
}

// Turns the camera to the yaw and pitch whose silhouette best matches the
// shipped icon: a coarse sweep, then a fine one around the best. The unit must
// already be framed, so it stays in view while the camera turns.
void FitAngle( SCameraShot *pShot, const CArray2D<uint32_t> &original )
{
	const SBox originalBox = GetOpaqueBox( original );
	if ( originalBox.IsEmpty() )
	{
		return;
	}
	std::vector<float> originalSilhouette;
	GetSilhouette( &originalSilhouette, original, originalBox );

	// Twice the icon's size leaves room in the target for the unit to move as it turns
	const float fFov = GetWindowFov( pShot->fFov, N_ICON_SIZE * 2 );
	auto score = [&]( const float fYaw, const float fPitch ) -> float
	{
		SCameraShot probe = *pShot;
		probe.fYaw = fYaw;
		probe.fPitch = fPitch;
		ApplyShot( probe );
		CArray2D<NGfx::SPixel8888> target;
		if ( !RenderTarget( &target, fFov ) )
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
				*pszError = fmt::format( "{}:{}: no stats path", szFileName, nLine + 1 );
				return false;
			}
			continue;
		}
		std::string szPath = szLine.substr( 0, nXDB + 4 );
		szPath.erase( 0, szPath.find_first_not_of( " \t" ) );
		NStr::ReplaceAllChars( &szPath, '\\', '/' );
		SOverride &over = ( *pRes )[Lower( szPath )];
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
			const float fValue = nEq == std::string::npos ? 0.0f : NStr::ToFloat( szToken.substr( nEq + 1 ) );
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
				*pszError = fmt::format( "{}:{}: unknown correction {}", szFileName, nLine + 1, szToken );
				return false;
			}
		}
	}
	return true;
}

// Applies one unit's corrections to its shot and says which were applied, for
// the manifest.
std::string ApplyOverride( SCameraShot *pShot, const SOverride &over, const CArray2D<uint32_t> *pOriginal )
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
			FrameShot( pShot, over.fFill );
			FitAngle( pShot, *pOriginal );
			szApplied += " angle";
		}
		else
		{
			szApplied += " angle(no shipped icon)";
		}
	}
	if ( over.bFrame || over.bAngle )
	{
		FrameShot( pShot, over.fFill );
		szApplied += fmt::format( " frame(fill={})", over.fFill );
	}
	if ( !szApplied.empty() )
	{
		szApplied.erase( 0, 1 );
	}
	return szApplied;
}

bool WriteTGA( const std::string &szFileName, const CArray2D<NGfx::SPixel8888> &image )
{
	CArray2D<uint32_t> data;
	data.SetSizes( image.GetSizeX(), image.GetSizeY() );
	for ( int y = 0; y < image.GetSizeY(); ++y )
	{
		for ( int x = 0; x < image.GetSizeX(); ++x )
		{
			data[y][x] = image[y][x].dwColor;
		}
	}
	CFileStream file( szFileName, CFileStream::WIN_CREATE );
	if ( !file.IsOk() )
	{
		return false;
	}
	return NImage::SaveImageAsTGA( &file, data );
}

bool WriteFile( const std::string &szFileName, const std::string &szData )
{
	CFileStream file( szFileName, CFileStream::WIN_CREATE );
	if ( !file.IsOk() )
	{
		return false;
	}
	file.Write( szData.data(), szData.size() );
	return true;
}

// The manifest is tab separated, so a name must not carry one.
std::string CleanField( std::string szValue )
{
	for ( char &c : szValue )
	{
		if ( c == '\t' || c == '\n' || c == '\r' )
		{
			c = ' ';
		}
	}
	return szValue;
}

std::string GetFolder( const std::string &szPath )
{
	const size_t nSlash = szPath.find_last_of( "/\\" );
	return nSlash == std::string::npos ? std::string() : szPath.substr( 0, nSlash + 1 );
}

std::vector<std::string> FindUnitStats( const std::string &szFilter )
{
	std::vector<std::string> fileNames;
	NVFS::GetMainVFS()->GetAllFileNames( &fileNames, "Units/" );
	std::vector<std::string> result;
	const std::string szLowFilter = Lower( szFilter );
	for ( const std::string &szFile : fileNames )
	{
		if ( !EndsWithNoCase( szFile, ".xdb" ) )
		{
			continue;
		}
		if ( !szLowFilter.empty() && Lower( szFile ).find( szLowFilter ) == std::string::npos )
		{
			continue;
		}
		// A zip entry named in the zip format's default CP437 rather than UTF-8
		// (MOD-18's "38_cm_Sturmm\x94rser_Sturmtiger") is not a valid UTF-8
		// path, and opening it throws. The VFS cannot reach such a file anyway,
		// so it is reported and skipped rather than ending the run.
		try
		{
			if ( NDb::GetClassTypeName( CDBID( szFile ) ) == "MechUnitRPGStats" )
			{
				result.push_back( szFile );
			}
		}
		catch ( const std::exception &error )
		{
			fmt::print( stderr, "skipped, cannot open: {} ({})\n", szFile, error.what() );
		}
	}
	return result;
}

bool InitGame( const SOptions &options, std::string *pszError )
{
	// The same start-up as Game/main.cpp, up to the point where the game would
	// enter its main loop, but with the editor's configs (MapEditorApp.cpp and
	// CCFCSceneB2::OnCreateChildFrameWnd). game.cfg is not one of them: it moves
	// the near plane to 40, which clips the small vehicles the editor shot from
	// closer than that, and it ends by opening the main menu.
	const bool bSettings =
			options.szInstall.empty()
					? NSettings::InitializeGeneralSettings( pszError )
					: NSettings::InitializeGeneralSettings(
								NSettings::CGeneralSettings( options.szInstall,
	                                           NFile::JoinPath( NSettings::GetApplicationDataDirectory(), "Profiles" ) ),
								pszError );
	if ( !bSettings )
	{
		return false;
	}
	NGlobal::LoadConfig( "Profiles/startup.cfg" );
	NGlobal::LoadConfig( "Profiles/editor.cfg" );
	NGScene::SFLB3_RunResourceLoadingThread();
	NGameX::Initialize();
	CreateAI();
	if ( !NWinFrame::SFLB1_InitApplication( " IconRender", "NIVAL_RTS_ENGINE", 0 ) )
	{
		*pszError = "cannot create the window";
		return false;
	}
	if ( !NGfx::Init3D( NWinFrame::GetWnd() ) )
	{
		*pszError = "failed to initialize Direct3D9";
		return false;
	}
	NMOD::InstantAttachMOD( "", NDb::DATABASE_MODE_GAME );
	NProfile::LoadProfile();
	// A mod, the way Game/main.cpp attaches the one --mod names
	if ( !options.szMod.empty() )
	{
		NMOD::InstantAttachMOD( NFile::JoinPath( NMainLoop::GetBaseDir(), NFile::DIR_MODS, options.szMod ),
		                        NDb::DATABASE_MODE_GAME );
	}
	NGlobal::LoadConfig( "Profiles/autoexec.cfg" );
	// What the editor does before it sets its mode (CCFCSceneB2::OnCreateChildFrameWnd)
	NGlobal::ProcessCommand( L"set_quality 1" );
	// Editor mode, as in the model editor: it draws a scene with no map in it.
	if ( !Scene()->SetupMode( SCENE_MODE_WINDOWED, true ) )
	{
		*pszError = "cannot set the video mode";
		return false;
	}
	// The model editor's camera handling (CModelEditor), which applies a
	// placement as given rather than within the game's limits.
	Camera()->SetHandleType( true );
	return true;
}

// Game/main.cpp's shutdown, so the singletons go in the order they expect.
void DoneGame()
{
	ClearHoldQueue();
	Scene()->Clear();
	NVFS::SetMainVFS( 0 );
	NVFS::SetMainFileCreator( 0 );
	NGfx::Done3D();
	NDb::CloseDatabase();
	NSingleton::DoneSingletons();
}

int RenderAll( const SOptions &_options );

int Run( const std::vector<std::string> &arguments )
{
	SOptions options;
	std::string szError;
	if ( !ParseCommandLine( arguments, &options, &szError ) )
	{
		fmt::print( stderr, "{}\n", szError );
		return 1;
	}
	if ( !InitGame( options, &szError ) )
	{
		fmt::print( stderr, "{}\n", szError );
		MessageBox( 0, szError.c_str(), "IconRender", MB_OK | MB_ICONERROR );
		return 1;
	}
	const int nRes = RenderAll( options );
	DoneGame();
	return nRes;
}

int RenderAll( const SOptions &_options )
{
	SOptions options = _options;
	if ( options.szOutDir.empty() )
	{
		options.szOutDir = NFile::JoinPath( NMainLoop::GetBaseDir(), "IconRender" );
	}
	NFile::AppendSlash( &options.szOutDir, '/' );
	NFile::CreatePath( options.szOutDir );

	const NDb::SAmbientLight *pLight = NDb::Get<NDb::SAmbientLight>( CDBID( options.szLight ) );
	if ( !pLight )
	{
		fmt::print( stderr, "no AmbientLight {}\n", options.szLight );
		return 1;
	}
	Scene()->SetLight( pLight );
	// CreateScreenShot's background: the editor's grey with zero alpha, and the
	// projection taken from the target rather than the window.
	Scene()->SetBackgroundColor( CVec4( 0.5f, 0.5f, 0.5f, 0.0f ) );
	Scene()->ToggleGetSizeFromTarget( true );

	std::map<std::string, SOverride> overrides;
	if ( !options.szOverrides.empty() )
	{
		std::string szError;
		if ( !LoadOverrides( options.szOverrides, &overrides, &szError ) )
		{
			fmt::print( stderr, "{}\n", szError );
			return 1;
		}
	}

	std::vector<std::string> units = FindUnitStats( options.szFilter );
	if ( options.nLimit > 0 && units.size() > options.nLimit )
	{
		units.resize( options.nLimit );
	}
	fmt::print( "{} units, light {}, output {}\n", units.size(), options.szLight, options.szOutDir );

	std::string szManifest =
			"folder\tstats\tname\tcamera\tanchor_x\tanchor_y\tanchor_z\tdistance\tpitch\tyaw\tfov\tlight\tscale\tstatus\tcorrections\n";
	const int nHQWindow = N_ICON_SIZE * options.nScale * options.nSupersample;
	int nDone = 0;
	for ( const std::string &szStatsPath : units )
	{
		NWinFrame::PumpMessages();

		// The VFS names files with backslashes; the output tree and the manifest
		// use forward ones, which every platform and the report script accept.
		std::string szPortablePath = szStatsPath;
		NStr::ReplaceAllChars( &szPortablePath, '\\', '/' );
		// One output folder per stats record: the unit's folder, and the record's
		// name below it when that is not the usual MechUnitRPGStats.xdb, since
		// some folders hold a second record (Fall of the Reich's "New Object.xdb").
		std::string szFolder = GetFolder( szPortablePath );
		const std::string szRecord = szPortablePath.substr( szFolder.size() );
		if ( Lower( szRecord ) != "mechunitrpgstats.xdb" )
		{
			szFolder += szRecord.substr( 0, szRecord.size() - 4 ) + "/";
		}
		const NDb::SMechUnitRPGStats *pStats = NDb::Get<NDb::SMechUnitRPGStats>( CDBID( szStatsPath ) );
		std::string szStatus = "ok";
		std::string szName;
		std::string szCorrections;
		SCameraShot shot;
		if ( !pStats )
		{
			szStatus = "no stats";
		}
		else
		{
			if ( !pStats->szLocalizedNameFileRef.empty() )
			{
				szName = NStr::ToMBCS( NText::GetText( pStats->szLocalizedNameFileRef ) );
			}
			shot.bFromStats = ReadCameraShot( szStatsPath, &shot );
			if ( !shot.bFromStats )
			{
				SetFallbackShot( &shot );
			}
		}
		const auto itOverride = overrides.find( Lower( szPortablePath ) );
		const SOverride *pOverride = itOverride != overrides.end() ? &itOverride->second : 0;
		// The record whose model and camera are drawn: this one, or the one like= names
		const NDb::SMechUnitRPGStats *pModelStats = pStats;
		if ( pStats && pOverride && !pOverride->szLike.empty() )
		{
			pModelStats = NDb::Get<NDb::SMechUnitRPGStats>( CDBID( pOverride->szLike ) );
			shot = SCameraShot();
			shot.bFromStats = pModelStats && ReadCameraShot( pOverride->szLike, &shot );
			if ( !shot.bFromStats )
			{
				SetFallbackShot( &shot );
			}
			szCorrections = "like=" + pOverride->szLike;
		}
		const NDb::SModel *pModel = pModelStats ? GetSummerModel( pModelStats ) : 0;
		if ( pStats && !pModel )
		{
			szStatus = "no model";
		}
		if ( pModel )
		{
			const std::string szUnitDir = options.szOutDir + szFolder;
			NFile::CreatePath( szUnitDir );
			// The shipped icon, from wherever the VFS finds it (a mod or a patch
			// pak may replace it). Its .dds sits beside the texture record.
			CArray2D<uint32_t> original;
			bool bOriginal = false;
			if ( pStats->pIconTexture )
			{
				std::string szDDSPath = pStats->pIconTexture->GetDBID().ToString();
				if ( EndsWithNoCase( szDDSPath, ".xdb" ) )
				{
					szDDSPath = szDDSPath.substr( 0, szDDSPath.size() - 4 ) + ".dds";
				}
				std::string szDDS;
				if ( ReadVFSFile( szDDSPath, &szDDS ) )
				{
					WriteFile( szUnitDir + "orig.dds", szDDS );
					std::unique_ptr<CDataStream> pStream( NVFS::GetMainVFS()->OpenFile( szDDSPath ) );
					bOriginal = pStream && NImage::LoadImageDDS( &original, pStream.get() );
				}
			}

			// The model where the model editor puts it: CModelState::UpdateModels
			const float fGridCentre = N_EDITOR_GRID_SIZE * VIS_TILE_SIZE / 2.0f;
			const float fShift = VIS_TILE_SIZE / 8.0f;
			CVec3 vPos( fGridCentre + fShift, fGridCentre + fShift, fShift );
			Vis2AIFast( &vPos );
			const int nObjectID = Scene()->AddObject( -1, pModel, vPos, QNULL, CVec3( 1, 1, 1 ), OBJ_ANIM_MODE_DEFAULT, 0 );

			if ( pOverride )
			{
				const std::string szApplied = ApplyOverride( &shot, *pOverride, bOriginal ? &original : 0 );
				if ( !szApplied.empty() )
				{
					szCorrections += ( szCorrections.empty() ? "" : " " ) + szApplied;
				}
			}
			ApplyShot( shot );

			// The editor's shot, pixel for pixel
			CArray2D<NGfx::SPixel8888> x1;
			const float fHQFov = GetWindowFov( shot.fFov, nHQWindow );
			CArray2D<NGfx::SPixel8888> hqFull;
			CArray2D<NGfx::SPixel8888> hq;
			if ( !RenderShot( &x1, shot.fFov, N_ICON_SIZE ) || !RenderShot( &hqFull, fHQFov, nHQWindow ) )
			{
				szStatus = "render failed";
			}
			else
			{
				Downsample( &hq, hqFull, options.nSupersample );
				if ( !WriteTGA( szUnitDir + "x1.tga", x1 ) || !WriteTGA( szUnitDir + "hq.tga", hq ) )
				{
					szStatus = "write failed";
				}
			}
			Scene()->RemoveObject( nObjectID );
		}

		szManifest += fmt::format( "{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\n",
		                           CleanField( szFolder ),
		                           CleanField( szPortablePath ),
		                           CleanField( szName ),
		                           shot.bFromStats ? "stats" : "fallback",
		                           shot.vAnchor.x,
		                           shot.vAnchor.y,
		                           shot.vAnchor.z,
		                           shot.fDistance,
		                           shot.fPitch,
		                           shot.fYaw,
		                           shot.fFov,
		                           options.szLight,
		                           options.nScale,
		                           szStatus,
		                           szCorrections );
		++nDone;
		fmt::print( "[{}/{}] {} {} {}\n", nDone, units.size(), szStatus, szStatsPath, szCorrections );
	}
	if ( !WriteFile( options.szOutDir + "manifest.tsv", szManifest ) )
	{
		fmt::print( stderr, "cannot write {}manifest.tsv\n", options.szOutDir );
		return 1;
	}
	return 0;
}

} // namespace

#if BOOST_OS_WINDOWS

int main()
{
	// UTF-8 arguments, the same way Game/main.cpp reads them
	std::vector<std::string> arguments;
	int nArgs = 0;
	LPWSTR *pArgs = CommandLineToArgvW( GetCommandLineW(), &nArgs );
	if ( pArgs != nullptr )
	{
		for ( int i = 1; i < nArgs; ++i )
		{
			arguments.push_back( NStr::ToMBCS( std::wstring( pArgs[i] ) ) );
		}
		LocalFree( pArgs );
	}
	return Run( arguments );
}

#else

int main( int argc, char *argv[] )
{
	return Run( std::vector<std::string>( argv + 1, argv + argc ) );
}

#endif
