#include "stdafx.h"
#include "UI/commandparam.h"
#include "UI/dbuserinterface.h"
#include "UI/UI.h"
#include "Input/gamemessage.h"
#include "Misc/2Darray.h"
#include "3Dmotor/rectlayout.h"
#include "WindowPotentialLines.h"
#include "UI/UIVisitor.h"
#include "Misc/Bresenham.h"
#include "Image/Targa.h"
#include "UI/Background.h"
#include "GameX/DBGameRoot.h"
#include "System/Commands.h"
#include "System/VFSOperations.h"

#include <cstdint>

#include <zconf.h>

static float s_fWeightMainStrike = 0.007f;
static float s_fWeightMaskNoise = 0.00001f;
static float s_fWeightPerlinNoise = 0.0f; //0.0002f;
static float s_fWeightNodes = 1.0f;
static int s_nPerlinGridSize = 20;
static int s_nPotentialGridSize = 10;
static int s_nBorderWidth = 10;

// ************************************************************************************************************************ //
// **
// ** CWindowChapterMapLayer
// **
// **
// **
// ************************************************************************************************************************ //

void CWindowChapterMapLayer::Recalc()
{
	if ( !IsValid( pValue ) )
		pValue = NGfx::MakeTexture( GetNextPow2( nWidth ), GetNextPow2( nHeight ), 1, NGfx::SPixel8888::ID, NGfx::DYNAMIC_TEXTURE, NGfx::CLAMP );

	NGfx::CTextureLock<NGfx::SPixel8888> lock( pValue, 0, NGfx::INPLACE );
	const int nSize = sizeof( NGfx::SPixel8888 ) * nWidth;
	for ( int y = 0; y < nHeight; ++y ) 
		memcpy( lock[y], &points[y][0], nSize );
	bNeedUpdate = false;
}

void CWindowChapterMapLayer::SetSize( const int _nWidth, const int _nHeight )
{
	nWidth = _nWidth;
	nHeight = _nHeight;
	points.SetSizes( nWidth, nHeight );
	pValue = 0;
	bNeedUpdate = true;
}

void CWindowChapterMapLayer::SetPixelAlpha( const int x, const int y, const uint8_t &nAlpha )
{
	if ( y >= 0 && y < nHeight && x >= 0 && x < nWidth )
		points[y][x].a = nAlpha;
}

uint8_t CWindowChapterMapLayer::GetPixelAlpha( const int x, const int y )
{
	if ( y >= 0 && y < nHeight && x >= 0 && x < nWidth )
		return points[y][x].a;
	return 0;
}

void CWindowChapterMapLayer::PutPixel( const int x, const int y, const NGfx::SPixel8888 &color )
{
	if ( y >= 0 && y < nHeight && x >= 0 && x < nWidth )
		points[y][x] = color;
}

const NGfx::SPixel8888 CWindowChapterMapLayer::GetPixel( const int x, const int y )
{
	if ( x >= 0 && y >= 0 && x < nWidth && y < nHeight )
		return points[y][x];
	else
		return NGfx::SPixel8888( 0, 0, 0, 0 );
}

void CWindowChapterMapLayer::Clear()
{
	points.FillEvery( NGfx::SPixel8888( 0, 0, 0, 0 ) );
}

int CWindowChapterMapLayer::operator&( IBinSaver &saver )
{
	saver.Add( 1, &points );
	saver.Add( 2, &nWidth );
	saver.Add( 3, &nHeight );
	saver.Add( 4, &bNeedUpdate );
	nLastUpdateTime = 0;
	return 0;
}

static float GetOctoDistance( float fX1, float fX2, float fY1, float fY2 )
{
	const float fH = abs( fX1 - fX2 );
	const float fV = abs( fY1 - fY2 );
	const float fOrto = (std::max)( fH, fV );

	return (std::min) ( fOrto, ( fH + fV ) * 0.666f );
}

#define LINE_BLUR_PART 0.3f
void CDrawMapPixelFunctional::DrawOctoCircle( int nX0, int nY0, float fR )
{
	int nX1 = nX0 - fR;
	int nX2 = nX0 + fR;
	int nY1 = nY0 - fR;
	int nY2 = nY0 + fR;

	for ( int nX = nX1; nX < nX2; ++nX )
	{
		for ( int nY = nY1; nY < nY2; ++nY )
		{
			const float fDist = GetOctoDistance( nX, nX0, nY, nY0 );
			if ( fDist < fR )
			{
				uint8_t nNewAlpha = color.a;
				uint8_t nOldAlpha = pMapLayer->GetPixelAlpha( nX, nY );
				if ( fDist > fR * LINE_BLUR_PART )
				{
					nNewAlpha = nNewAlpha * ( fR - fDist ) / ( fR * ( 1.0f - LINE_BLUR_PART ) );  
				}
				pMapLayer->PutPixel( nX, nY, color );
				pMapLayer->SetPixelAlpha( nX, nY, (std::max)( nOldAlpha, nNewAlpha ) );
			}
		}
	}
}

// ************************************************************************************************************************ //
// **
// ** CWindowPotentialLines
// **
// **
// **
// ************************************************************************************************************************ //

/*#define s_nPotentialGridSize 16
#define STEP2 8*/
#define STEP_TWIN_OFFSET 0
CWindowPotentialLines::CWindowPotentialLines()
: bValid ( false ), bLayerInit( false ), bDrawDashes( false )
{
	pTerritoryLayer = new CWindowChapterMapLayer;
}

struct STextureEntryEqual
{
	std::string szID;

	STextureEntryEqual( const std::string &_szID ) { szID = _szID; }
	bool operator()( const NDb::SUITextureEntry &value ) const
	{
		return szID == value.szTextID;
	}
};

void CWindowPotentialLines::Visit( IUIVisitor *pVisitor )
{
	Recalculate();

	if ( !IsVisible() )
		return;

	CWindow::Visit( pVisitor );

	CTRect<float> rect = GetWindowRect();
	CVec2 vPos( rect.x1, rect.y1 );

	const CTRect<float> rectTexture = CTRect<float>( 0.0f, 0.0f, vTextureSize.x, vTextureSize.y );
	CRectLayout rects;
	rects.AddRect( vPos.x, vPos.y, 
		pInstance->placement.size.Get().x, pInstance->placement.size.Get().y, rectTexture, FadeColor( 0xFFFFFFFF, GetTotalFadeValue() ) );
	VirtualToScreen( &rects );

	pVisitor->VisitUITextureRect( pTerritoryLayer, 3, rects );
	pVisitor->VisitUITextureRect( pLinesLayer, 3, rects );
	DrawArrows( pVisitor );
}

void CWindowPotentialLines::InitByDesc( const struct NDb::SUIDesc *pDesc )
{
	const NDb::SWindowPotentialLines *pWindowDesc( checked_cast<const NDb::SWindowPotentialLines*>( pDesc ) );
	pInstance = pWindowDesc->Duplicate();
	CWindow::InitByDesc( pDesc );

	pShared = checked_cast_ptr<const NDb::SWindowPotentialLinesShared *>( pWindowDesc->pShared );

	bLayerInit = false;

	ClearNodes();
}

void CWindowPotentialLines::Recalculate()
{
	if ( bValid )
		return;

	if ( !bLayerInit )
	{
		CTRect<float> rect = GetWindowRect();
		int nSizeX = rect.GetSizeX();
		int nSizeY = rect.GetSizeY();
		
		vTextureSize.x = nSizeX;
		vTextureSize.y = nSizeY;

		pLinesLayer = new CWindowChapterMapLayer();
		pLinesLayer->SetSize( nSizeX, nSizeY );
		pts.SetSizes( nSizeX / s_nPotentialGridSize + 1, nSizeY / s_nPotentialGridSize + 1 );

		bLayerInit = true;
	}

	// Clear points
	for ( int i = 0; i < pts.GetSizeY(); ++i )
	{
		for ( int j = 0; j < pts.GetSizeX(); ++j )
		{
			pts[i][j].eType = EIT_NONE;
		}
	}

	pLinesLayer->Clear();
	CDrawMapPixelFunctional funcDraw1( pLinesLayer, colourBorder1, &seaMask, s_nBorderWidth );

	// Recalc
	int nY = -s_nPotentialGridSize;
	for ( int i = 0; i < pts.GetSizeY(); ++i, nY += s_nPotentialGridSize )
	{
		int nX = -s_nPotentialGridSize;
		for ( int j = 0; j < pts.GetSizeX(); ++j, nX += s_nPotentialGridSize )
		{
			pts[i][j].fValue = GetValue( nX + s_nPotentialGridSize, nY + s_nPotentialGridSize );

			if ( i > 0 && j > 0 )			// Quad is filled, determine intersection type
			{
				SPointDesc &pt = pts[i - 1][j - 1];

				/* Encode
				 | |
				-3-2-
				-4-1-
				 | |
				into a byte like this:
				00001234
				A bit is set to 1 if the value is positive
				*/
				float fValues[4] = { pts[i][j].fValue, pts[i - 1][j].fValue, pts[i - 1][j - 1].fValue, pts[i][j - 1].fValue };
				uint8_t nType = 0;
				if ( fValues[0] > 0 )
					nType |= 1;
				nType <<= 1;
				if ( fValues[1] > 0 )
					nType |= 1;
				nType <<= 1;
				if ( fValues[2] > 0 )
					nType |= 1;
				nType <<= 1;
				if ( fValues[3] > 0 )
					nType |= 1;

				// Set Dark layer values (needs to be done before lines
				for ( int nDarkX = nX; nDarkX < nX + s_nPotentialGridSize; ++nDarkX )
				{
					for ( int nDarkY = nY; nDarkY < nY + s_nPotentialGridSize; ++nDarkY )
					{
						bool bUse1 = false;

						if ( nType == 0 )					// All < 0
							bUse1 = false;
						else if ( nType == 15 )		// All > 0
							bUse1 = true;
						else											// Intersection, compare
							if ( GetValue( nDarkX, nDarkY ) > 0.0f )
								bUse1 = true;
							else
								bUse1 = false;

						if ( seaMask.IsEmpty() )
							pTerritoryLayer->SetPixelAlpha( nDarkX, nDarkY, 255 );
						else if ( bUse1 || seaMask.GetData( nDarkX, nDarkY ) )
							pTerritoryLayer->SetPixelAlpha( nDarkX, nDarkY, 0 );
						else
							pTerritoryLayer->SetPixelAlpha( nDarkX, nDarkY, 255 );

						//if ( nDarkX == nX || nDarkY == nY )
						//pLinesLayer->PutPixel( nDarkX, nDarkY, 0xffffffff );
					}
				}

				// Add (draw if not dashed) lines
				switch ( nType )
				{
				case 1:
				case 14:
					{
						pt.eType = EIT_CORNER_SW;
						const float fPart1 = s_nPotentialGridSize * fabs( fValues[3] / ( fValues[3] - fValues[0] ) );		// S wall
						const float fPart2 = s_nPotentialGridSize * fabs( fValues[2] / ( fValues[2] - fValues[3] ) );		// W wall
						MakeLine2( nX, nY + fPart2, nX + fPart1, nY + s_nPotentialGridSize, funcDraw1 );
						break;
					}
				case 2:
				case 13:
					{
						pt.eType = EIT_CORNER_NW;
						const float fPart1 = s_nPotentialGridSize * fabs( fValues[2] / ( fValues[2] - fValues[1] ) );		// N wall
						const float fPart2 = s_nPotentialGridSize * fabs( fValues[2] / ( fValues[2] - fValues[3] ) );		// W wall
						MakeLine2( nX, nY + fPart2, nX + fPart1, nY, funcDraw1 );
						break;
					}
				case 3:
				case 12:
					{
						pt.eType = EIT_LINE_V;
						const float fPart1 = s_nPotentialGridSize * fabs( fValues[3] / ( fValues[3] - fValues[0] ) );		// S wall
						const float fPart2 = s_nPotentialGridSize * fabs( fValues[2] / ( fValues[2] - fValues[1] ) );		// N wall
						MakeLine2( nX + fPart2, nY, nX + fPart1, nY + s_nPotentialGridSize, funcDraw1 );
						break;
					}
				case 4:
				case 11:
					{
						pt.eType = EIT_CORNER_NE;
						const float fPart1 = s_nPotentialGridSize * fabs( fValues[2] / ( fValues[2] - fValues[1] ) );		// N wall
						const float fPart2 = s_nPotentialGridSize * fabs( fValues[1] / ( fValues[1] - fValues[0] ) );		// E wall
						MakeLine2( nX + fPart1, nY, nX + s_nPotentialGridSize, nY + fPart2, funcDraw1 );
						break;
					}
				case 5:
				case 10:
					{
						pt.eType = EIT_CROSS;				
						const float fPart1 = s_nPotentialGridSize * fabs( fValues[3] / ( fValues[3] - fValues[0] ) );		// S wall
						const float fPart2 = s_nPotentialGridSize * fabs( fValues[2] / ( fValues[2] - fValues[3] ) );		// W wall
						const float fPart3 = s_nPotentialGridSize * fabs( fValues[2] / ( fValues[2] - fValues[1] ) );		// N wall
						const float fPart4 = s_nPotentialGridSize * fabs( fValues[1] / ( fValues[1] - fValues[0] ) );		// E wall
						MakeLine2( nX, nY + fPart2, nX + fPart1, nY + s_nPotentialGridSize, funcDraw1 );
						MakeLine2( nX + fPart3, nY, nX + s_nPotentialGridSize, nY + fPart4, funcDraw1 );
						break;
					}
				case 6:
				case 9:
					{
						pt.eType = EIT_LINE_H;
						const float fPart1 = s_nPotentialGridSize * fabs( fValues[2] / ( fValues[2] - fValues[3] ) );		// W wall
						const float fPart2 = s_nPotentialGridSize * fabs( fValues[1] / ( fValues[1] - fValues[0] ) );		// E wall
						MakeLine2( nX, nY + fPart1, nX + s_nPotentialGridSize, nY + fPart2, funcDraw1 );
						break;
					}
				case 7:
				case 8:
					{
						pt.eType = EIT_CORNER_SE;
						const float fPart1 = s_nPotentialGridSize * fabs( fValues[3] / ( fValues[3] - fValues[0] ) );		// S wall
						const float fPart2 = s_nPotentialGridSize * fabs( fValues[1] / ( fValues[1] - fValues[0] ) );		// E wall
						MakeLine2( nX + fPart1, nY + s_nPotentialGridSize, nX + s_nPotentialGridSize, nY + fPart2, funcDraw1 );
						break;
					}
				default:
					{
						pt.eType = EIT_NONE;
					}
				}
			}
		}
	}

	pLinesLayer->SetNeedUpdate();
	pTerritoryLayer->SetNeedUpdate();
	bValid = true;
}

const float CWindowPotentialLines::GetValue( int nX, int nY ) const
{
	float fResult = 0.0f;
	if ( nX >= 0 && nX < noise.GetSizeX() && nY >= 0 && nY < noise.GetSizeY() )
		fResult = noise[nY][nX];

	for ( CNodeList::const_iterator it = nodes.begin(); it != nodes.end(); ++it )
	{
		const SNodeDesc &node = *it;

		fResult += s_fWeightNodes * node.fValue / ( 1.0f + GetSquaredDistanceToSegment( node, nX, nY ) );		// Quadratic
	}

	return fResult;
}

float CWindowPotentialLines::GetSquaredDistanceToSegment( const SNodeDesc &node, int nX, int nY ) const
{
	float fT = node.nEndOffsetX * ( nX - node.nX ) + node.nEndOffsetY * ( nY - node.nY );

	if ( node.nEndOffsetX == 0 && node.nEndOffsetY == 0 )
		fT = 0.0f;
	else
		fT /= node.nEndOffsetX * node.nEndOffsetX + node.nEndOffsetY * node.nEndOffsetY;

	fT = Clamp( fT, 0.0f, 1.0f );
	float fDX = ( node.nX + node.nEndOffsetX * fT ) - nX;
	float fDY = ( node.nY + node.nEndOffsetY * fT ) - nY;

	return fDX * fDX + fDY * fDY;
}

void CWindowPotentialLines::ClearNodes()
{
	nodes.clear();

	bValid = false;
}

void CWindowPotentialLines::SetNode( int nX, int nY, int nEndOffsetX, int nEndOffsetY, float fValue )
{
	for ( CNodeList::iterator it = nodes.begin(); it != nodes.end(); ++it )
	{
		SNodeDesc &node = *it;

		if ( node.nX == nX && node.nY == nY )
		{
			node.fValue = fValue;
			bValid = false;
			return;
		}
	}

	SNodeDesc node;
	node.nX = nX;
	node.nY = nY;
	node.fValue = fValue;
	node.nEndOffsetX = nEndOffsetX;
	node.nEndOffsetY = nEndOffsetY;
	nodes.push_back( node );

	bValid = false;
}

void CWindowPotentialLines::SetParams( const std::string &szMask, const std::string &szDiffColourMap, const CVec2 &_vMainStrike, const uint32_t _dwBorderColour1, const uint32_t _dwBorderColour2 )
{
	vMainStrike = _vMainStrike;
	szMaskFile = szMask;
	szDiffColourMapFile = szDiffColourMap;

	colourBorder1 = _dwBorderColour1;
	colourBorder2 = _dwBorderColour2;

	SetupNoise();
}

void CWindowPotentialLines::AddArrow( const std::vector<CVec2> &arrowTraj, float fArrowWidth, const NDb::STexture *pArrowTexture, uint32_t dwArrowColour  )
{
	if ( pArrowTexture->IsRefInvalid() )
		return;

	SArrowDesc newDesc;
	newDesc.pts = arrowTraj;
	newDesc.fWidth = fArrowWidth;
	newDesc.dwColour = dwArrowColour;
	newDesc.pTexture = pArrowTexture;

	arrows.push_back( newDesc );
}

void CWindowPotentialLines::DrawArrows( struct IUIVisitor *pVisitor )
{
	CVec2 sPos[4];
	CVec2 sScreenPos[4];

	std::vector<float> segmentLengths;
	CTRect<float> rect = GetWindowRect();
	CVec2 vPos( rect.x1, rect.y1 ); 
	CVec2 vSizeMultiplier( rect.Width() / vTextureSize.x, rect.Height() / vTextureSize.y );

	for ( CArrowList::iterator itArrow = arrows.begin(); itArrow != arrows.end(); ++itArrow )
	{
		CTRect<float> rectTexture;
		SArrowDesc &arrow = *itArrow;
		std::vector<CVec2> &arrowPts = arrow.pts;
		NGfx::SPixel8888 sColors[4] = { arrow.dwColour, arrow.dwColour, arrow.dwColour, arrow.dwColour };
		float fArrowWidth = arrow.fWidth / 2.0f;		// Half-width, actually
		float fArrowTextureLength = arrow.pTexture->nHeight - 4.0f;

		//rect.Set( 0.5f, 0.5f, arrow.pTexture->nWidth - 1.5f, fArrowTextureLength + 0.5f );

		// Calculate lengths
		segmentLengths.resize( arrowPts.size() - 1 );

		float fArrowLength = 0.0f;
		for ( int i = 0; i < segmentLengths.size(); ++i )
		{
			segmentLengths[i] = fabs( arrowPts[i + 1].x - arrowPts[i].x, arrowPts[i + 1].y - arrowPts[i].y );
			fArrowLength += segmentLengths[i];
		}

		fArrowLength = fArrowTextureLength / fArrowLength;

		float fSegmentTextureLength = segmentLengths[0] * fArrowLength;
		CTRect<float> rectWindow( 0.5f, 0.5f, arrow.pTexture->nWidth - 1.5f, fSegmentTextureLength + 0.5f );

		// Draw segments
		for ( int i = 0; i < segmentLengths.size(); ++i )
		{
			// Test show Center line
			//MakeLine2( arrowPts[i].x, arrowPts[i].y, arrowPts[i + 1].x, arrowPts[i + 1].y, funcDraw );

			CVec2 vTmp( arrowPts[i + 1].y - arrowPts[i].y, arrowPts[i].x - arrowPts[i + 1].x );	// Perpendicular to the segment

			Normalize( &vTmp );
			vTmp *= fArrowWidth;

			if ( i == 0 )		// Init first points
			{
				sPos[0] = arrowPts[i] + vTmp;
				sPos[1] = arrowPts[i] - vTmp;

				// Test show First points
				//MakeLine2( sPos[0].x, sPos[0].y, sPos[1].x, sPos[1].y, funcDraw );
			}
			else						// Copy points from previous iteration
			{
				sPos[0] = sPos[3];
				sPos[1] = sPos[2];
			}

			// Calculate next points
			sPos[3] = arrowPts[i + 1] + vTmp;
			sPos[2] = arrowPts[i + 1] - vTmp;

			// Test show Next points
			//MakeLine2( sPos[1].x, sPos[1].y, sPos[2].x, sPos[2].y, funcDraw );
			//MakeLine2( sPos[2].x, sPos[2].y, sPos[3].x, sPos[3].y, funcDraw );
			//MakeLine2( sPos[0].x, sPos[0].y, sPos[3].x, sPos[3].y, funcDraw );

			// Draw texture bit
			fSegmentTextureLength = segmentLengths[i] * fArrowLength;
			for ( int j = 0; j < 4; ++j )
			{
				sScreenPos[j].x = sPos[( j + 1 ) % 4].x * vSizeMultiplier.x + vPos.x;
				sScreenPos[j].y = sPos[( j + 1 ) % 4].y * vSizeMultiplier.y + vPos.y;
			}
			/*sScreenPos[0] = sPos[1] + vPos;
			sScreenPos[1] = sPos[2] + vPos;
			sScreenPos[2] = sPos[3] + vPos;
			sScreenPos[3] = sPos[0] + vPos;*/
			VirtualToScreen( &sScreenPos[0] );
			VirtualToScreen( &sScreenPos[1] );
			VirtualToScreen( &sScreenPos[2] );
			VirtualToScreen( &sScreenPos[3] );
			rectWindow.y2 = (std::min)( rectWindow.y1 + fSegmentTextureLength, fArrowTextureLength );

			pVisitor->VisitUIRect( arrow.pTexture, 3, sScreenPos, sColors, rectWindow );

			rectWindow.y1 += fSegmentTextureLength;
		}
	}
}

void CWindowPotentialLines::DrawNoise( const CVec2 &vMainStrike )
{
	int nX, nY;

	CArray2D<CVec2> gridVectors;
	int nRandValue = 0;

	const int nMaxX = noise.GetSizeX();
	const int nMaxY	= noise.GetSizeY();

	gridVectors.SetSizes( nMaxX / s_nPerlinGridSize + 2, nMaxX / s_nPerlinGridSize + 2 );
	for ( nX = 0; nX < gridVectors.GetSizeX(); ++nX )
	{
		for ( nY = 0; nY < gridVectors.GetSizeY(); ++nY )
		{
			nRandValue = ( nRandValue * 17 + 111 ) & 0xffff;			// Pseudorandom, but repeatable

			gridVectors[nY][nX] = GetVectorByDirection( nRandValue );
		}
	}

	for ( nX = 0; nX < nMaxX; ++nX )
	{
		float fXGrad = vMainStrike.x * ( nX + nX - nMaxX ) / nMaxX;

		for ( nY = 0; nY < nMaxY; ++nY )
		{
			float fYGrad = -vMainStrike.y * ( nY + nY - nMaxY ) / nMaxY;

			float fNoiseValue = 0.5f;

			// ------------------- Assign fNoiseValue in (0..1)
			int nX0 = nX / s_nPerlinGridSize;
			int nY0 = nY / s_nPerlinGridSize;
			float fXp = float( nX % s_nPerlinGridSize ) / ( s_nPerlinGridSize - 1 );
			float fYp = float( nY % s_nPerlinGridSize ) / ( s_nPerlinGridSize - 1 );

			float fS = gridVectors[nY0][nX0].x * fXp + gridVectors[nY0][nX0].y * fYp;
			float fT = gridVectors[nY0][nX0 + 1].x * ( fXp - 1 ) + gridVectors[nY0][nX0 + 1].y * fYp;
			float fU = gridVectors[nY0 + 1][nX0].x * fXp + gridVectors[nY0 + 1][nX0].y * ( fYp - 1 );
			float fV = gridVectors[nY0 + 1][nX0 + 1].x * ( fXp - 1 ) + gridVectors[nY0 + 1][nX0 + 1].y * ( fYp - 1 );

			float fSx = 3 * fXp * fXp - 2 * fXp * fXp * fXp;
			float fSy = 3 * fYp * fYp - 2 * fYp * fYp * fYp;
			float fA = fS + fSx * ( fT - fS );
			float fB = fU + fSx * ( fV - fU );
			
			fNoiseValue = fA + fSy * ( fB - fA );			// Is between -1 and 1

			noise[nY][nX] += fNoiseValue * s_fWeightPerlinNoise + ( fXGrad + fYGrad ) * s_fWeightMainStrike;
		}
	}
}

void CWindowPotentialLines::OnSerialize( IBinSaver &f ) 
{
	if ( f.IsReading() ) 
	{ 
		bValid = false; 
		bLayerInit = false; 

		// regenerate layers and noise
		SetupNoise();
	}
}

void CWindowPotentialLines::SetupNoise()
{
	CArray2D<uint32_t> noiseMask;

	{
		CFileStream stream( NVFS::GetMainVFS(), szMaskFile );
		if ( stream.IsOk() )
		{
			NImage::LoadTGAImage( noiseMask, &stream );

			noise.SetSizes( noiseMask.GetSizeX(), noiseMask.GetSizeY() );
			noise.FillZero();
			seaMask.SetSizes( noiseMask.GetSizeX(), noiseMask.GetSizeY() );
			seaMask.FillZero();

			for ( int i = 0; i < noiseMask.GetSizeY(); ++i )
			{
				for ( int j = 0; j < noiseMask.GetSizeX(); ++j )
				{
					NGfx::SPixel8888 v( noiseMask[i][j] );
					if ( v.a == 0xFF )
						seaMask.SetData( j, i );
					else if ( v.a == 0x00 )
						noise[i][j] = 0.0f;
					else
					{
						float fValue = ( float( v.a ) - 127.0f ) * s_fWeightMaskNoise; // 1 / 500000.0f;
						noise[i][j] = fValue;
					}
				}
			}
		}
		else
		{
			int nX, nY;
			GetPlacement( 0, 0, &nX, &nY );
			noise.SetSizes( nX, nY );
			seaMask.Clear();
		}
		DrawNoise( vMainStrike );
	}

	{
		// Read differently coloured map
		CFileStream stream( NVFS::GetMainVFS(), szDiffColourMapFile );
		if ( stream.IsOk() )
		{
			NImage::LoadTGAImage( noiseMask, &stream );

			pTerritoryLayer->SetSize( noiseMask.GetSizeX(), noiseMask.GetSizeY() );
			for ( int i = 0; i < noiseMask.GetSizeX(); ++i )
			{
				for ( int j = 0; j < noiseMask.GetSizeY(); ++j )
					pTerritoryLayer->PutPixel( i, j, NGfx::SPixel8888( noiseMask[j][i] ) );
			}
		}
		else
		{
			pTerritoryLayer->SetSize( 1, 1 );
			pTerritoryLayer->Clear();
		}
	}
}

REGISTER_SAVELOAD_CLASS( 0x191B5380, CWindowPotentialLines )
REGISTER_SAVELOAD_CLASS( 0x191B5B80, CWindowChapterMapLayer )

START_REGISTER(WindowPotentialLines)
REGISTER_VAR_EX( "frontlines_weight_mask", NGlobal::VarFloatHandler, &s_fWeightMaskNoise, 0.00001f, STORAGE_NONE );
REGISTER_VAR_EX( "frontlines_weight_nodes", NGlobal::VarFloatHandler, &s_fWeightNodes, 1.0f, STORAGE_NONE );
REGISTER_VAR_EX( "frontlines_weight_perlin", NGlobal::VarFloatHandler, &s_fWeightPerlinNoise, 0.0f /*0.0002f*/, STORAGE_NONE );
REGISTER_VAR_EX( "frontlines_weight_gradient", NGlobal::VarFloatHandler, &s_fWeightMainStrike, 0.007f, STORAGE_NONE );
REGISTER_VAR_EX( "frontlines_perlin_gridsize", NGlobal::VarIntHandler, &s_nPerlinGridSize, 20, STORAGE_NONE );
REGISTER_VAR_EX( "frontlines_gridsize", NGlobal::VarIntHandler, &s_nPotentialGridSize, 10, STORAGE_NONE );
REGISTER_VAR_EX( "frontlines_border_width", NGlobal::VarIntHandler, &s_nPotentialGridSize, 10, STORAGE_NONE );
FINISH_REGISTER


