#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/CommonEditorMethods.h"
#include "Misc/2Darray.h"
#include "Image/Targa.h"
#include "Stats_B2_M1/IconsSet.h"
#include "System/Time.h"
#include "ResourceDefines.h"

#include "MapEditorLib/Interface_UserData.h"
#include "EditorMethods.h"
#include "HeightStateV3.h"
#include "MapInfoEditor.h"

#include "EditorScene.h"
#include "Misc/Win32Helper.h"
#include "System/VFSOperations.h"

#include <cstdint>

#include <zconf.h>

#define HEIGHT_MARK ( 1.0f )

const int CHeightStateV3::BRUSH_MIN_SIZE	= 3;
const int CHeightStateV3::BRUSH_SIZE_STEP	= 2;
const float CHeightStateV3::MAX_HEIGHT		= AI_TILE_SIZE * 10.0f;
const float CHeightStateV3::HEIGHT_SPEED	= 15.0f * VIS_TILE_SIZE / ( AI_TILE_SIZE * AI_TILES_IN_VIS_TILE );
const float CHeightStateV3::ROUND_RATIO		= 0.1f;
const float CHeightStateV3::PLATO_RATIO		= 0.5f;
const uint32_t CHeightStateV3::BRUSH_COLOR		= 0xFFFF0000;
const int CHeightStateV3::BRUSH_PARTS			= 32;
const int CHeightStateV3::HEIGHT_BRUSH_SIZE[5] = { 3, 5, 9, 13, 17 };
const int	CHeightStateV3::TILE_BRUSH_SIZE[5] = { 2, 4, 8, 12, 16 };


void CHeightTileStateV3::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_TILE );
	}
}


void CHeightTileStateV3::OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit()  && !pParentState->bEscaped )
	{
		if ( nFlags & MK_LBUTTON )
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_TILE );
		}
	}
}


// CHeightUpStateV3

void CHeightUpStateV3::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		if ( nFlags & MK_RBUTTON )
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_ROUND );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_ROUND );
		}
		else
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_UP );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_UP );
		}
	}
}


void CHeightUpStateV3::OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		if ( nFlags & MK_LBUTTON )
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_ROUND );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_ROUND );
		}
		else
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_DOWN );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_DOWN );
		}
	}
}


void CHeightUpStateV3::OnMButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_ROUND );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_ROUND );
	}
}


void CHeightUpStateV3::OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit()  && !pParentState->bEscaped )
	{
		if ( ( ( nFlags & MK_LBUTTON ) && ( nFlags & MK_RBUTTON ) ) || ( nFlags & MK_MBUTTON ) )
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_ROUND );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_ROUND );
		}
		else if ( nFlags & MK_LBUTTON )
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_UP );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_UP );
		}
		else if ( nFlags & MK_RBUTTON )
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_DOWN );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_DOWN );
		}
	}
}


// CHeightDownState

void CHeightDownStateV3::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		if ( nFlags & MK_RBUTTON )
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_ROUND );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_ROUND );
		}
		else
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_DOWN );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_DOWN );
		}
	}
}


void CHeightDownStateV3::OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		if ( nFlags & MK_LBUTTON )
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_ROUND );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_ROUND );
		}
		else
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_UP );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_UP );
		}
	}
}


void CHeightDownStateV3::OnMButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_ROUND );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_ROUND );
	}
}


void CHeightDownStateV3::OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit()  && !pParentState->bEscaped )
	{
		if ( ( ( nFlags & MK_LBUTTON ) && ( nFlags & MK_RBUTTON ) ) || ( nFlags & MK_MBUTTON ) )
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_ROUND );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_ROUND );
		}
		else if ( nFlags & MK_LBUTTON )
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_DOWN );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_DOWN );
		}
		else if ( nFlags & MK_RBUTTON )
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_UP );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_UP );
		}
	}
}


// CHeightRoundState

void CHeightRoundStateV3::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		if ( nFlags & MK_RBUTTON )
		{
			pParentState->UpdatePlatoHeight();
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_PLATO );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_PLATO );
		}
		else
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_ROUND );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_ROUND );
		}
	}
}


void CHeightRoundStateV3::OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		if ( nFlags & MK_LBUTTON )
		{
			pParentState->UpdatePlatoHeight();
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_PLATO );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_PLATO );
		}
		else
		{
			pParentState->UpdatePlatoHeight();
		}
	}
}


void CHeightRoundStateV3::OnMButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->UpdatePlatoHeight();
		pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_PLATO );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_PLATO );
	}
}


void CHeightRoundStateV3::OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit()  && !pParentState->bEscaped )
	{
		if ( ( ( nFlags & MK_LBUTTON ) && ( nFlags & MK_RBUTTON ) ) || ( nFlags & MK_MBUTTON ) )
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_PLATO );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_PLATO );
		}
		else if ( nFlags & MK_LBUTTON )
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_ROUND );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_ROUND );
		}
		else if ( nFlags & MK_RBUTTON )
		{
			pParentState->UpdatePlatoHeight();
		}
	}
}


// CHeightPlatoState

void CHeightPlatoStateV3::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		if ( nFlags & MK_RBUTTON )
		{
			pParentState->UpdatePlatoHeight();
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_PLATO );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_PLATO );
		}
		else
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_PLATO );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_PLATO );
		}
	}
}


void CHeightPlatoStateV3::OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		if ( nFlags & MK_LBUTTON )
		{
			pParentState->UpdatePlatoHeight();
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_PLATO );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_PLATO );
		}
		else
		{
			pParentState->UpdatePlatoHeight();
		}
	}
}


void CHeightPlatoStateV3::OnMButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->UpdatePlatoHeight();
		pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_PLATO );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_PLATO );
	}
}


void CHeightPlatoStateV3::OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit()  && !pParentState->bEscaped )
	{
		if ( ( ( nFlags & MK_LBUTTON ) && ( nFlags & MK_RBUTTON ) ) || ( nFlags & MK_MBUTTON ) )
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_PLATO );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_PLATO );
		}
		else if ( nFlags & MK_LBUTTON )
		{
			pParentState->ProcessTerrain( CHeightStateV3::SEditParameters::B_PLATO );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_SET_TIMER, CHeightStateV3::SEditParameters::B_PLATO );
		}
		else if ( nFlags & MK_RBUTTON )
		{
			pParentState->UpdatePlatoHeight();
		}
	}
}


// Версии с проверкой на балланс красного и черного c простым уровнем
// Simple Plane SP
// Check Red Black Ballance CRBB
// Create Height Pattern To Level CHPTL
// Calculate Average Height CAH

// паттерн для заравнивания TerrainHeight
struct S_SP_CRBB_CHPTL_Functional
{
	CHeightContainer *pHeightContainer;
	SHeightPattern *pHeightPattern;
	ITerraManager *pTerraManager;
	float fAverageHeight;
	float fLevelRatio;
	//
	S_SP_CRBB_CHPTL_Functional( CHeightContainer *_pHeightContainer,
															SHeightPattern *_pHeightPattern,
															float _fAverageHeight,
															float _fLevelRatio )
		:	pHeightContainer( _pHeightContainer ),
			pHeightPattern( _pHeightPattern ),
			fAverageHeight( _fAverageHeight ),
			fLevelRatio( _fLevelRatio )			
	{
		pTerraManager = EditorScene()->GetTerraManager();
		NI_ASSERT( pHeightContainer != 0, "Wrong parameter: pHeightContainer == 0" );
		NI_ASSERT( pHeightPattern != 0, "Wrong parameter: pHeightPattern == 0" );
		NI_ASSERT( pTerraManager != 0, "Wrong parameter: pHeightPattern == 0" );
	}
	//
	bool operator()( int nXIndex, int nYIndex, float fValue )
	{ 
		if ( ( fValue == HEIGHT_MARK ) && pHeightContainer->Compare( nXIndex, nYIndex ) )
		{
			pHeightPattern->heights[nYIndex - pHeightPattern->pos.y][nXIndex - pHeightPattern->pos.x] = ( fAverageHeight - pTerraManager->GetTerraHeightFast( nXIndex, nYIndex ) ) * fLevelRatio;
		}
		else
		{
			pHeightPattern->heights[nYIndex - pHeightPattern->pos.y][nXIndex - pHeightPattern->pos.x] = 0.0f;
		}
		return true;
	}
};


// паттерн для вычисления средней высоты TerrainHeight
struct S_SP_CRBB_CAH_Functional
{
	CHeightContainer *pHeightContainer;
	ITerraManager *pTerraManager;
	float fTotalHeight;
	int nPointCount;
	//	
	S_SP_CRBB_CAH_Functional( CHeightContainer *_pHeightContainer )
		: pHeightContainer( _pHeightContainer ),
			fTotalHeight( 0.0f ),
			nPointCount( 0 )
	{
		pTerraManager = EditorScene()->GetTerraManager();
		NI_ASSERT( pHeightContainer != 0, "Wrong parameter: pHeightContainer == 0" );
		NI_ASSERT( pTerraManager != 0, "Wrong parameter: pHeightPattern == 0" );
	}
	//
	bool operator()( int nXIndex, int nYIndex, float fValue )
	{ 
		if ( ( fValue == HEIGHT_MARK ) && pHeightContainer->Compare( nXIndex, nYIndex ) )
		{
			fTotalHeight += pTerraManager->GetTerraHeightFast( nXIndex, nYIndex );
			++nPointCount;
		}
		return true;
	}
	//
	inline float GetAverageHeight() const { return ( ( nPointCount > 0 ) ? ( fTotalHeight / nPointCount ) : 0.0f ); }
};


void CalculateMatrix( CArray2D<double> *pMatrix, SDoubleVec3 *pvRightSide, int nXIndex, int nYIndex, ITerraManager *pTerraManager )
{
	const double fX = nXIndex * VIS_TILE_SIZE;
	const double fY = nYIndex * VIS_TILE_SIZE;
	const double fZ = pTerraManager->GetTerraHeightFast( nXIndex, nYIndex );
	( *pMatrix )[0][0] += fX * fX; ( *pMatrix )[0][1] += fX * fY; ( *pMatrix )[0][2] += fX;
	( *pMatrix )[1][0] += fX * fY; ( *pMatrix )[1][1] += fY * fY; ( *pMatrix )[1][2] += fY;
	( *pMatrix )[2][0] += fX; ( *pMatrix )[2][1] += fY; ( *pMatrix )[2][2] += 1.0f;
	( pvRightSide->x ) += fZ * fX; ( pvRightSide->y ) += fZ * fY; ( pvRightSide->z ) += fZ;
}


// Версии без проверки на балланс красного и черного c простым уровнем
// Simple Plane SP
// Create Height Pattern To Level CHPTL
// Calculate Average Height CAH

// паттерн для заравнивания TerrainHeight
struct S_SP_CHPTL_Functional
{
	SHeightPattern *pHeightPattern;
	ITerraManager *pTerraManager;
	float fAverageHeight;
	float fLevelRatio;
	//
	S_SP_CHPTL_Functional( SHeightPattern *_pHeightPattern,
												 float _fAverageHeight,
												 float _fLevelRatio )
		:	pHeightPattern( _pHeightPattern ),
			fAverageHeight( _fAverageHeight ),
			fLevelRatio( _fLevelRatio )			
	{
		pTerraManager = EditorScene()->GetTerraManager();
		NI_ASSERT( pHeightPattern != 0, "Wrong parameter: pHeightPattern == 0" );
		NI_ASSERT( pTerraManager != 0, "Wrong parameter: pHeightPattern == 0" );
	}
	//
	bool operator()( int nXIndex, int nYIndex, float fValue )
	{ 
		if ( fValue == HEIGHT_MARK )
		{
			pHeightPattern->heights[nYIndex - pHeightPattern->pos.y][nXIndex - pHeightPattern->pos.x] = ( fAverageHeight - pTerraManager->GetTerraHeightFast( nXIndex, nYIndex ) ) * fLevelRatio;
		}
		else
		{
			pHeightPattern->heights[nYIndex - pHeightPattern->pos.y][nXIndex - pHeightPattern->pos.x] = 0.0f;
		}
		return true;
	}
};


// паттерн для вычисления средней высоты TerrainHeight
struct S_SP_CAH_Functional
{
	ITerraManager *pTerraManager;
	float fTotalHeight;
	int nPointCount;
	//	
	S_SP_CAH_Functional()
		:	fTotalHeight( 0.0f ),
			nPointCount( 0 )
	{
		pTerraManager = EditorScene()->GetTerraManager();
		NI_ASSERT( pTerraManager != 0, "Wrong parameter: pHeightPattern == 0" );
	}
	//
	bool operator()( int nXIndex, int nYIndex, float fValue )
	{ 
		if ( fValue == HEIGHT_MARK )
		{
			fTotalHeight += pTerraManager->GetTerraHeightFast( nXIndex, nYIndex );
			++nPointCount;
		}
		return true;
	}
	//
	inline float GetAverageHeight() const { return ( ( nPointCount > 0 ) ? ( fTotalHeight / nPointCount ) : 0.0f ); }
};


// Версии с проверкой на балланс красного и черного c простым уровнем
// Complex Plane CP
// Check Red Black Ballance CRBB
// Create Height Pattern To Level CHPTL
// Calculate Average Plane CAP

// паттерн для заравнивания TerrainHeight
struct S_CP_CRBB_CHPTL_Functional
{
	CHeightContainer *pHeightContainer;
	SHeightPattern *pHeightPattern;
	ITerraManager *pTerraManager;
	SDoubleVec3 vPlane;
	float fLevelRatio;
	//
	S_CP_CRBB_CHPTL_Functional( CHeightContainer *_pHeightContainer,
															SHeightPattern *_pHeightPattern,
															const SDoubleVec3 &rvPlane,
															float _fLevelRatio )
		:	pHeightContainer( _pHeightContainer ),
			pHeightPattern( _pHeightPattern ),
			vPlane( rvPlane ),
			fLevelRatio( _fLevelRatio )			
	{
		pTerraManager = EditorScene()->GetTerraManager();
		NI_ASSERT( pHeightContainer != 0, "Wrong parameter: pHeightContainer == 0" );
		NI_ASSERT( pHeightPattern != 0, "Wrong parameter: pHeightPattern == 0" );
		NI_ASSERT( pTerraManager != 0, "Wrong parameter: pHeightPattern == 0" );
	}
	//
	bool operator()( int nXIndex, int nYIndex, float fValue )
	{ 
		if ( ( fValue == HEIGHT_MARK ) && pHeightContainer->Compare( nXIndex, nYIndex ) )
		{
			const double fAverageHeight = vPlane.x * ( nXIndex * VIS_TILE_SIZE ) + vPlane.y * ( nYIndex * VIS_TILE_SIZE ) + vPlane.z;
			pHeightPattern->heights[nYIndex - pHeightPattern->pos.y][nXIndex - pHeightPattern->pos.x] = ( fAverageHeight - pTerraManager->GetTerraHeightFast( nXIndex, nYIndex ) ) * fLevelRatio;
		}
		else
		{
			pHeightPattern->heights[nYIndex - pHeightPattern->pos.y][nXIndex - pHeightPattern->pos.x] = 0.0f;
		}
		return true;
	}
};


// паттерн для вычисления параметров плоскости TerrainHeight
struct S_CP_CRBB_CAP_Functional
{
	CHeightContainer *pHeightContainer;
	ITerraManager *pTerraManager;
	CArray2D<double> matrix;
	SDoubleVec3 vRightSide;
	//	
	S_CP_CRBB_CAP_Functional( CHeightContainer *_pHeightContainer )
		: pHeightContainer( _pHeightContainer )
	{
		pTerraManager = EditorScene()->GetTerraManager();
		//
		matrix.SetSizes( 3, 3 );
		matrix.FillZero();
		//
		vRightSide.x = 0.0;
		vRightSide.y = 0.0;
		vRightSide.z = 0.0;
		//
		NI_ASSERT( pHeightContainer != 0, "Wrong parameter: pHeightContainer == 0" );
		NI_ASSERT( pTerraManager != 0, "Wrong parameter: pHeightPattern == 0" );
	}
	//
	bool operator()( int nXIndex, int nYIndex, float fValue )
	{ 
		if ( ( fValue == HEIGHT_MARK ) && pHeightContainer->Compare( nXIndex, nYIndex ) )
		{
			CalculateMatrix( &matrix, &vRightSide, nXIndex, nYIndex, pTerraManager );
		}
		return true;
	}
	//
	inline bool GetPlane( SDoubleVec3 *pvPlane ) const
	{ 
		CArray2D<double> invertMatrix;
		invertMatrix.SetSizes( 3, 3 );
		invertMatrix.FillZero();
		if ( Invert3x3Matrix( &invertMatrix, matrix ) )
		{
			if ( pvPlane )
			{
				Multiply3x3Matrix( pvPlane, invertMatrix, vRightSide );
				return true;
			}
		}
		return false;
	}
};


// Версии без проверки на балланс красного и черного c простым уровнем
// Cimple Plane CP
// Create Height Pattern To Level CHPTL
// Calculate Average Plane CAP

// паттерн для заравнивания TerrainHeight
struct S_CP_CHPTL_Functional
{
	SHeightPattern *pHeightPattern;
	ITerraManager *pTerraManager;
	SDoubleVec3 vPlane;
	float fLevelRatio;
	//
	S_CP_CHPTL_Functional( SHeightPattern *_pHeightPattern,
												 const SDoubleVec3 &rvPlane,
												 float _fLevelRatio )
		:	pHeightPattern( _pHeightPattern ),
			vPlane( rvPlane ),
			fLevelRatio( _fLevelRatio )			
	{
		pTerraManager = EditorScene()->GetTerraManager();
		NI_ASSERT( pHeightPattern != 0, "Wrong parameter: pHeightPattern == 0" );
		NI_ASSERT( pTerraManager != 0, "Wrong parameter: pHeightPattern == 0" );
	}
	//
	bool operator()( int nXIndex, int nYIndex, float fValue )
	{ 
		if ( fValue == HEIGHT_MARK )
		{
			const double fAverageHeight = vPlane.x * ( nXIndex * VIS_TILE_SIZE ) + vPlane.y * ( nYIndex * VIS_TILE_SIZE ) + vPlane.z;
			pHeightPattern->heights[nYIndex - pHeightPattern->pos.y][nXIndex - pHeightPattern->pos.x] = ( fAverageHeight - pTerraManager->GetTerraHeightFast( nXIndex, nYIndex ) ) * fLevelRatio;
		}
		else
		{
			pHeightPattern->heights[nYIndex - pHeightPattern->pos.y][nXIndex - pHeightPattern->pos.x] = 0.0f;
		}
		return true;
	}
};


// паттерн для вычисления параметров плоскости TerrainHeight
struct S_CP_CAP_Functional
{
	ITerraManager *pTerraManager;
	CArray2D<double> matrix;
	SDoubleVec3 vRightSide;
	//	
	S_CP_CAP_Functional()
	{
		pTerraManager = EditorScene()->GetTerraManager();
		//
		matrix.SetSizes( 3, 3 );
		matrix.FillZero();
		//
		vRightSide.x = 0.0;
		vRightSide.y = 0.0;
		vRightSide.z = 0.0;
		//
		NI_ASSERT( pTerraManager != 0, "Wrong parameter: pHeightPattern == 0" );
	}
	//
	bool operator()( int nXIndex, int nYIndex, float fValue )
	{ 
		if ( fValue == HEIGHT_MARK )
		{
			CalculateMatrix( &matrix, &vRightSide, nXIndex, nYIndex, pTerraManager );
		}
		return true;
	}
	//
	inline bool GetPlane( SDoubleVec3 *pvPlane ) const
	{ 
		CArray2D<double> invertMatrix;
		invertMatrix.SetSizes( 3, 3 );
		invertMatrix.FillZero();
		if ( Invert3x3Matrix( &invertMatrix, matrix ) )
		{
			if ( pvPlane )
			{
				Multiply3x3Matrix( pvPlane, invertMatrix, vRightSide );
				return true;
			}
		}
		return false;
	}
};


// CHeightStateV3

CHeightStateV3::SEditParameters* CHeightStateV3::GetEditParameters()
{ 
	return ( ( pMapInfoEditor != 0 ) ? &( pMapInfoEditor->editorSettings.epHeightStateV3 ) : 0 );
}


bool CHeightStateV3::CanEdit()
{
	return true;
}


int CHeightStateV3::GetTileBrushSize( SEditParameters::EBrushSize eBrushSize )
{ 
	const int nSizeIndex = Clamp<int>( eBrushSize, 0, 4 );
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		if ( nSizeIndex < pEditParameters->tileBrushSizeList.size() )
		{
			const int nBrushSize = pEditParameters->tileBrushSizeList[nSizeIndex];
			if ( nBrushSize > 0 )
			{
				return nBrushSize;
			}
		}
	}
	return TILE_BRUSH_SIZE[nSizeIndex];
}


int CHeightStateV3::GetHeightBrushSize( SEditParameters::EBrushSize eBrushSize )
{ 
	const int nSizeIndex = Clamp<int>( eBrushSize, 0, 4 );
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		if ( nSizeIndex < pEditParameters->heightBrushSizeList.size() )
		{
			const int nBrushSize = pEditParameters->heightBrushSizeList[nSizeIndex];
			if ( nBrushSize > 0 )
			{
				return nBrushSize;
			}
		}
	}
	return HEIGHT_BRUSH_SIZE[nSizeIndex];
}


void CHeightStateV3::UpdatePlatoHeight()
{
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		pEditParameters->fLevelTo = GetTerrainHeight( pStoreInputState->lastEventInfo.vTerrainPos.x, pStoreInputState->lastEventInfo.vTerrainPos.y )  * VIS_TILE_SIZE / ( AI_TILE_SIZE * AI_TILES_IN_VIS_TILE );
		pEditParameters->nFlags = MITHV3EP_LEVEL_TO;
		::SetEditParameters( *pEditParameters, CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3 );
	}
}


void CHeightStateV3::SetCornerTile( SHeightPattern *pPattern, const CTPoint<int> &rPatternPos )
{
	NI_ASSERT( pPattern != 0, ( "Wrong parameter: pPattern == 0" ) );
	if ( pPattern == 0 )
	{
		return;
	}
	if ( pPattern->bOdd )
	{
		pPattern->pos.x = rPatternPos.x - pPattern->heights.GetSizeX() / 2;
		pPattern->pos.y = rPatternPos.y - pPattern->heights.GetSizeY() / 2;
		//DebugTrace( "CHeightStateV3::SetCornerTile: odd: (%d, %d)", pPattern->pos.x, pPattern->pos.y );
	}
	else
	{
		pPattern->pos.x = rPatternPos.x - pPattern->heights.GetSizeX() / 2 + 1;
		pPattern->pos.y = rPatternPos.y - pPattern->heights.GetSizeY() / 2 + 1;
		//DebugTrace( "CHeightStateV3::SetCornerTile: (%d, %d)", pPattern->pos.x, pPattern->pos.y );
	}
}


void CHeightStateV3::GetEditParameters( unsigned nFlags )
{
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		if ( pEditParameters->nFlags & MITHV3EP_BRUSH )
		{
			SetActiveInputState( pEditParameters->eBrush, true, false );
		}
		if ( pEditParameters->nFlags & ( MITHV3EP_BRUSH_SIZE | MITHV3EP_BRUSH_TYPE ) )
		{
			UpdateCommonPatterns();
		}
		if ( pEditParameters->nFlags & MITHV3EP_LEVEL_TO )
		{
		}
		if ( pEditParameters->nFlags & MITHV3EP_UPDATE_HEIGHT )
		{
		}
		if ( pEditParameters->nFlags & MITHV3EP_TILE_COUNT )
		{
		}
		if ( pEditParameters->nFlags & MITHV3EP_THUMBNAILS )
		{
		}
		if ( pEditParameters->nFlags & MITHV3EP_TILE_INDEX )
		{
			CreateTileBrush( &tileBrush, GetTileBrushSize( pEditParameters->eBrushSize ), pEditParameters->nTileIndex, pEditParameters->eBrushType == SEditParameters::BT_CIRCLE );
			if ( pEditParameters->eBrush != SEditParameters::B_TILE )
			{
				pEditParameters->eBrush = SEditParameters::B_TILE;
				SetActiveInputState( pEditParameters->eBrush, true, false );
				pEditParameters->nFlags = MITHV3EP_BRUSH;
				::SetEditParameters( *pEditParameters, CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3 );
			}
		}
	}
}


void CHeightStateV3::SetEditParameters( unsigned nFlags )
{
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		for ( int nSizeIndex = pEditParameters->tileBrushSizeList.size(); nSizeIndex < 5; ++nSizeIndex )
		{
			pEditParameters->tileBrushSizeList.push_back( TILE_BRUSH_SIZE[nSizeIndex] );	
		}
		for ( int nSizeIndex = pEditParameters->heightBrushSizeList.size(); nSizeIndex < 5; ++nSizeIndex )
		{
			pEditParameters->heightBrushSizeList.push_back( HEIGHT_BRUSH_SIZE[nSizeIndex] );	
		}
		//
		if ( pEditParameters->nFlags & MITHV3EP_BRUSH )
		{
			SetActiveInputState( pEditParameters->eBrush, true, false );
		}
		if ( pEditParameters->nFlags & MITHV3EP_TILE_COUNT )
		{
			//if ( pEditParameters->tileList.empty() )
			{
				if ( CPtr<IManipulator> pTerrainSetManipulator = CManipulatorManager::CreateManipulatorFromReference( "TerraSet", pMapInfoEditor->GetViewManipulator(), 0, 0, 0 ) )
				{
					pEditParameters->tileList.clear();
					CManipulatorManager::GetArray<std::vector<std::string>, std::string>( &( pEditParameters->tileList ), pTerrainSetManipulator, "TerraTypes" );
				}
			}
		}
		if ( pEditParameters->nFlags & MITHV3EP_TILE_INDEX )
		{
			if ( ( pEditParameters->nTileIndex < 0 ) || ( pEditParameters->nTileIndex >= pEditParameters->tileList.size() ) )
			{
				pEditParameters->nTileIndex = 0;
			}
		}
	}
}


void CHeightStateV3::UpdateCommonPatterns()
{
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		positivePattern.CreateByGradient( HEIGHT_SPEED, GetHeightBrushSize( pEditParameters->eBrushSize ), gradient );
		negativePattern = positivePattern;
		for ( int x = 0; x < negativePattern.heights.GetSizeX(); ++x )
		{
			for ( int y = 0; y < negativePattern.heights.GetSizeY(); ++y )
			{
				negativePattern.heights[y][x] = negativePattern.heights[y][x] * ( -1.0f );
			}
		}
		CreateTileBrush( &tileBrush, GetTileBrushSize( pEditParameters->eBrushSize ), pEditParameters->nTileIndex, pEditParameters->eBrushType == SEditParameters::BT_CIRCLE );
	}
}


void CHeightStateV3::UpdateLevelPattern( const CTPoint<int> &rPatternPos, SEditParameters::EBrush eBrush )
{
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		if ( eBrush == SEditParameters::B_TILE )
		{
			return;
		}
		//
		NWin32Helper::CPrecisionControl precisionControl( NWin32Helper::CPrecisionControl::PCM_HIGH );
		const int nBrushSize = GetHeightBrushSize( pEditParameters->eBrushSize );
		const CTRect<int> heightsRect( 0, 0, pMapInfoEditor->pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH + 1, pMapInfoEditor->pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH + 1 );
		bool bUseSimplePlaneMethod = ( nBrushSize < 5 ) || ( eBrush == SEditParameters::B_PLATO );
		SDoubleVec3 vPlane;
		vPlane.x = 0.0;
		vPlane.y = 0.0;
		vPlane.z = 0.0;
		//
		SHeightPattern heightPattern;
		heightPattern.CreateByValue( HEIGHT_MARK, nBrushSize, pEditParameters->eBrushType == SEditParameters::BT_CIRCLE );
		SetCornerTile( &heightPattern, rPatternPos );
		//
		bool bBlackRedBallance = pMapInfoEditor->heightContainer.GetBlackRedBallance( CTRect<int>( heightPattern.pos.x,
																																															heightPattern.pos.y,
																																															heightPattern.pos.x + heightPattern.heights.GetSizeX(),
																																															heightPattern.pos.y + heightPattern.heights.GetSizeY() ) );
		//
		switch( eBrush )
		{
			case SEditParameters::B_TILE:
			{
				return;
			}
			case SEditParameters::B_UP:
			case SEditParameters::B_DOWN:
			case SEditParameters::B_ROUND:
			{
				if ( !bUseSimplePlaneMethod )
				{
					if ( bBlackRedBallance )
					{
						S_CP_CRBB_CAP_Functional functional( &( pMapInfoEditor->heightContainer ) );
						ApplyHeightPattern( heightsRect, heightPattern, functional, true );
						if ( !functional.GetPlane( &vPlane ) )
						{
							bUseSimplePlaneMethod = true;
						}
					}
					else
					{
						S_CP_CAP_Functional functional;
						ApplyHeightPattern( heightsRect, heightPattern, functional, true );
						if ( !functional.GetPlane( &vPlane ) )
						{
							bUseSimplePlaneMethod = true;
						}
					}
				}
				if ( bUseSimplePlaneMethod )
				{
					if ( bBlackRedBallance )
					{
						S_SP_CRBB_CAH_Functional functional( &( pMapInfoEditor->heightContainer ) );
						ApplyHeightPattern( heightsRect, heightPattern, functional, true );
						pEditParameters->fLevelTo = functional.GetAverageHeight();
					}
					else
					{
						S_SP_CAH_Functional functional;
						ApplyHeightPattern( heightsRect, heightPattern, functional, true );
						pEditParameters->fLevelTo = functional.GetAverageHeight();
					}
					pEditParameters->nFlags = MITHV3EP_LEVEL_TO;
					::SetEditParameters( *pEditParameters, CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3 );
				}
				break;
			}
			default:
				break;
		}
		levelPattern.CreateByValue( HEIGHT_MARK, nBrushSize, pEditParameters->eBrushType == SEditParameters::BT_CIRCLE );
		SetCornerTile( &levelPattern, rPatternPos );
		if ( bUseSimplePlaneMethod )
		{
			if ( bBlackRedBallance )
			{
				S_SP_CRBB_CHPTL_Functional functional( &( pMapInfoEditor->heightContainer ),
																							&levelPattern,
																							pEditParameters->fLevelTo,
																							( eBrush == SEditParameters::B_PLATO ) ? PLATO_RATIO : ROUND_RATIO );
				ApplyHeightPattern( heightsRect, levelPattern, functional, true );
			}
			else
			{
				S_SP_CHPTL_Functional functional( &levelPattern,
																					pEditParameters->fLevelTo,
																					( eBrush == SEditParameters::B_PLATO ) ? PLATO_RATIO : ROUND_RATIO );
				ApplyHeightPattern( heightsRect, levelPattern, functional, true );
			}
		}
		else
		{
			if ( bBlackRedBallance )
			{
				S_CP_CRBB_CHPTL_Functional functional( &( pMapInfoEditor->heightContainer ),
																							&levelPattern,
																							vPlane,
																							( eBrush == SEditParameters::B_PLATO ) ? PLATO_RATIO : ROUND_RATIO );
				ApplyHeightPattern( heightsRect, levelPattern, functional, true );
			}
			else
			{
				S_CP_CHPTL_Functional functional( &levelPattern,
																					vPlane,
																					( eBrush == SEditParameters::B_PLATO ) ? PLATO_RATIO : ROUND_RATIO );
				ApplyHeightPattern( heightsRect, levelPattern, functional, true );
			}
		}
	}
}


void CHeightStateV3::CreatePatternAndModifyGeometry( const SHeightPattern &rPattern )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
		{
			SHeightPattern pattern = rPattern;
			for ( int y = 0;  y < pattern.heights.GetSizeY(); ++y )
			{
				for ( int x = 0;  x < pattern.heights.GetSizeX(); ++x )
				{
					if ( !pMapInfoEditor->heightContainer.Compare( x + pattern.pos.x, y + pattern.pos.y ) )
					{
						pattern.heights[y][x] = 0.0f;
					}
				}
			}
			pTerraManager->ModifyTerraGeometryByBrush( pattern.pos.x,
																								 pattern.pos.y,
																								 false,
																								 pattern.heights,
																								 NTerraBrush::TERRA_BRUSH_OVERRIDE );
		}
	}
}


void CHeightStateV3::ProcessTerrain( SEditParameters::EBrush eBrush )
{
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		if ( IEditorScene *pScene = EditorScene() )
		{
			if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
			{
				if ( eBrush == SEditParameters::B_TILE )
				{
					const float fRadius = ( GetTileBrushSize( pEditParameters->eBrushSize ) / 2.0f - 1 ) * AI_TILE_SIZE * 2.0f;
					CVec3 vBrushPos = pStoreInputState->lastEventInfo.vTerrainPos;
					vBrushPos.x -= fRadius;
					vBrushPos.y -= fRadius;
					if ( !bTileEditStarted )
					{
						pTerraManager->GetTileTypeUpdateDifferences( &( vTileDiffPos.x ), &( vTileDiffPos.y ), &tileDiff );
						bTileEditStarted = true;
					}
					pTerraManager->UpdateTileAreaType( vBrushPos.x, vBrushPos.y, tileBrush, NTerraBrush::TERRA_BRUSH_OVERRIDE );
				}
				else
				{
					if ( !bHeightEditStarted )
					{
						pTerraManager->GetTerraGeometryUpdateDifferences( &( heightDiffPos.x ), &( heightDiffPos.y ), &heightDiff );
						bHeightEditStarted = true;
					}
					switch( eBrush )
					{
						default:
						case SEditParameters::B_UP:
						{
							SetCornerTile( &positivePattern, pStoreInputState->lastEventInfo.gridPos );
							if ( pMapInfoEditor->heightContainer.GetBlackRedBallance( CTRect<int>( positivePattern.pos.x,
																																										positivePattern.pos.y,
																																										positivePattern.pos.x + positivePattern.heights.GetSizeX(),
																																										positivePattern.pos.y + positivePattern.heights.GetSizeY() ) ) )
							{
								CreatePatternAndModifyGeometry( positivePattern );
							}
							else
							{
								pTerraManager->ModifyTerraGeometryByBrush( positivePattern.pos.x,
																													 positivePattern.pos.y,
																													 false,
																													 positivePattern.heights,
																													 NTerraBrush::TERRA_BRUSH_OVERRIDE );
							}
							//pMapInfoEditor->bNeedSave = true;
							break;
						}
						case SEditParameters::B_DOWN:
						{
							SetCornerTile( &negativePattern, pStoreInputState->lastEventInfo.gridPos );
							if ( pMapInfoEditor->heightContainer.GetBlackRedBallance( CTRect<int>( negativePattern.pos.x,
																																										negativePattern.pos.y,
																																										negativePattern.pos.x + negativePattern.heights.GetSizeX(),
																																										negativePattern.pos.y + negativePattern.heights.GetSizeY() ) ) )
							{
								CreatePatternAndModifyGeometry( negativePattern );
							}
							else
							{
								pTerraManager->ModifyTerraGeometryByBrush( negativePattern.pos.x,
																													 negativePattern.pos.y,
																													 false,
																													 negativePattern.heights,
																													 NTerraBrush::TERRA_BRUSH_OVERRIDE );
							}
							//pMapInfoEditor->bNeedSave = true;
							break;
						}
						case SEditParameters::B_ROUND:
						case SEditParameters::B_PLATO:
						{
							UpdateLevelPattern( pStoreInputState->lastEventInfo.gridPos, eBrush );
							pTerraManager->ModifyTerraGeometryByBrush( levelPattern.pos.x,
																												 levelPattern.pos.y,
																												 false,
																												 levelPattern.heights,
																												 NTerraBrush::TERRA_BRUSH_OVERRIDE );
							//pMapInfoEditor->bNeedSave = true;
							break;
						}
					}
				}
				//	process units
				//if ( IEditorScene *pScene = EditorScene() )
				//{
				//	const CVec3 vMin( 0, 0, 0 );
				//	const CVec3 vMax( 100, 100, 0 );
				//	//
				//	std::list<unsigned> objectsSceneIDList;
				//	pMapInfoEditor->objectInfoCollector.CreateSelection( objectsSceneIDList );
				//	//
				//	NMapInfoEditor::SObjectEditInfo objectEditInfo;
				//	objectEditInfo.bRotateTo90Degree = pMapInfoEditor->editorSettings.bRotateTo90Degree;
				//	objectEditInfo.bFitToGrid = pMapInfoEditor->editorSettings.bFitToGrid;
				//	//
				//	CPtr<CObjectBaseController> pObjectController = pMapInfoEditor->CreateController();
				//	pMapInfoEditor->objectInfoCollector.MoveSelection( &objectEditInfo, VNULL3, false, false, true,
				//																											true, pScene,
				//																											false, pObjectController, pMapInfoEditor->GetViewManipulator() );
				//	if ( pObjectController )
				//	{
				//		pObjectController->Redo( false, true, pMapInfoEditor );
				//		Singleton<IControllerContainer>()->Add( pObjectController );
				//	}
				//	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
				//	//
				//	pMapInfoEditor->objectInfoCollector.CreateSelection( objectsSceneIDList );
				//}

				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			}
		}
	}
}


void CHeightStateV3::CreateMapInfoController()
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
		{
			//записываем изменения в Undo Buffer
			if ( CPtr<CMapInfoController> pMapInfoController = pMapInfoEditor->CreateController() )
			{
				bool bSaveChanges = false;
				pTerraManager->GetTileTypeUpdateDifferences( &( vTileDiffPos.x ), &( vTileDiffPos.y ), &tileDiff );
				if ( ( tileDiff.GetSizeX() > 0 ) && (tileDiff.GetSizeY() > 0 ) )
				{
					pMapInfoController->AddChangeTileOperation( tileDiff, vTileDiffPos );
					bSaveChanges = true;
				}
				pTerraManager->GetTerraGeometryUpdateDifferences( &( heightDiffPos.x ), &( heightDiffPos.y ), &heightDiff );
				if ( ( heightDiff.GetSizeX() > 0 ) && ( heightDiff.GetSizeY() > 0 ) )
				{
					pMapInfoController->AddChangeHeightOperation( heightDiff, heightDiffPos );
					bSaveChanges = true;
				}
				if ( bSaveChanges )
				{
					pTerraManager->UpdateAfterTilesModifying();
					pMapInfoEditor->wndMiniMap.LoadMap( pMapInfoEditor->pMapInfo );
					pMapInfoEditor->wndMiniMap.UpdateWindow();
					pMapInfoEditor->SetModified( true );
					//
					Singleton<IControllerContainer>()->Add( pMapInfoController );
				}
			}
		}
		bTileEditStarted =false;
		bHeightEditStarted = false;
	}
}


void CHeightStateV3::CreateTileBrush( CArray2D<uint8_t> *pBrush, int nSize, int nTileIndex, bool bCircle )
{
	NI_ASSERT( pBrush != 0, "CreateBrush(): Invalid parameter: pBrush == 0" );

	pBrush->SetSizes( nSize, nSize );
	NI_ASSERT( nSize > 0, "CreateBrush(): nSize < 1" );
	for ( int x = 0; x < nSize; ++x )
	{
		for ( int y = 0; y < nSize; ++y )
		{
			if ( bCircle )
			{
				const float fSize = fabs( x - ( nSize / 2.0f ), y - ( nSize / 2.0f ) ) / ( nSize / 2.0f );
				( *pBrush )[y][x] = ( fSize > 1.0f ) ? NO_AFFECTED_TILE : nTileIndex;
			}
			else
			{
				( *pBrush )[y][x] = nTileIndex;
			}
		}
	}
}


void CHeightStateV3::Enter()
{
	heightDiffPos = CTPoint<int>( 0, 0 );
	heightDiff.Clear();

	vTileDiffPos = VNULL3;
	tileBrush.Clear();
	tileDiff.Clear();

	//
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	// создаем градиент
	CArray2D<uint32_t> image;
	{
		CFileStream stream( pUserData->constUserData.szStartFolder + "Editor\\profile.tga", CFileStream::WIN_READ_ONLY );
		if ( stream.IsOk() )
		{
			if ( NImage::LoadTGAImage( image, &stream ) )
			{
				gradient.CreateFromImage( image, CTPoint<float>( 0.0f, 1.0f ), CTPoint<float>( 0.0f, 1.0f ) );
				UpdateCommonPatterns();
			}
		}
	}
	pStoreInputState->SetSizes( CTPoint<int>( pMapInfoEditor->pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH,
																						pMapInfoEditor->pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH ),
															true );
	sceneDrawTool.Clear();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	//
	CMultiInputState::Enter();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_KILL_TIMER, 0 );
}


void CHeightStateV3::Leave()
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_KILL_TIMER, 0 );
	CMultiInputState::Leave();
	//
	heightDiffPos = CTPoint<int>( 0, 0 );
	heightDiff.Clear();
	//
	vTileDiffPos = VNULL3;
	tileBrush.Clear();
	tileDiff.Clear();
	//
	sceneDrawTool.Clear();
}


void CHeightStateV3::Draw( CPaintDC *pPaintDC )
{
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		if ( pStoreInputState->lastEventInfo.isValid )
		{
			if ( pEditParameters->eBrush == SEditParameters::B_TILE )
			{
				CVec3 vSceneBrushPos = pStoreInputState->lastEventInfo.vTerrainPos;
				vSceneBrushPos.z = GetTerrainHeight( vSceneBrushPos.x, vSceneBrushPos.y );

				const float fRadius = GetTileBrushSize( pEditParameters->eBrushSize ) * AI_TILE_SIZE;
				//
				sceneDrawTool.DrawLine( vSceneBrushPos + CVec3( -fRadius, 0.0f, 0.0f ), vSceneBrushPos + CVec3( fRadius, 0.0f, 0.0f ), BRUSH_COLOR, false );
				sceneDrawTool.DrawLine( vSceneBrushPos + CVec3( 0.0f, -fRadius, 0.0f ), vSceneBrushPos + CVec3( 0.0f, fRadius, 0.0f ), BRUSH_COLOR, false );
				//
				if ( pEditParameters->eBrushType == SEditParameters::BT_SQUARE )
				{
					sceneDrawTool.DrawLine( vSceneBrushPos + CVec3( -fRadius, -fRadius, 0.0f ), vSceneBrushPos + CVec3(  fRadius, -fRadius, 0.0f ), BRUSH_COLOR, false );
					sceneDrawTool.DrawLine( vSceneBrushPos + CVec3(  fRadius, -fRadius, 0.0f ), vSceneBrushPos + CVec3(  fRadius,  fRadius, 0.0f ), BRUSH_COLOR, false );
					sceneDrawTool.DrawLine( vSceneBrushPos + CVec3(  fRadius,  fRadius, 0.0f ), vSceneBrushPos + CVec3( -fRadius,  fRadius, 0.0f ), BRUSH_COLOR, false );
					sceneDrawTool.DrawLine( vSceneBrushPos + CVec3( -fRadius,  fRadius, 0.0f ), vSceneBrushPos + CVec3( -fRadius, -fRadius, 0.0f ), BRUSH_COLOR, false );
				}
				else
				{
					sceneDrawTool.DrawCircle( vSceneBrushPos, fRadius, BRUSH_PARTS, BRUSH_COLOR, false );
				}
			}
			else
			{
				const int nBrushSize = GetHeightBrushSize( pEditParameters->eBrushSize ) / 2;
				//
				const float fRadius = nBrushSize * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE;
				const CVec2	center( pStoreInputState->lastEventInfo.gridPos.x * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE,
														pStoreInputState->lastEventInfo.gridPos.y * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE );
				if ( pEditParameters->eBrushType == SEditParameters::BT_CIRCLE )
				{
					sceneDrawTool.Draw3DCircle( center, fRadius, BRUSH_PARTS, BRUSH_COLOR, false ); 		
				}
				else
				{
					sceneDrawTool.Draw3DLine( CVec2( center.x - fRadius, center.y - fRadius ),
																		CVec2( center.x + fRadius, center.y - fRadius ),
																		BRUSH_COLOR,
																		false,
																		BRUSH_PARTS / 4 ); 		
					sceneDrawTool.Draw3DLine( CVec2( center.x + fRadius, center.y - fRadius ),
																		CVec2( center.x + fRadius, center.y + fRadius ),
																		BRUSH_COLOR,
																		false,
																		BRUSH_PARTS / 4 ); 		
					sceneDrawTool.Draw3DLine( CVec2( center.x + fRadius, center.y + fRadius ),
																		CVec2( center.x - fRadius, center.y + fRadius ),
																		BRUSH_COLOR,
																		false,
																		BRUSH_PARTS / 4 ); 		
					sceneDrawTool.Draw3DLine( CVec2( center.x - fRadius, center.y + fRadius ),
																		CVec2( center.x - fRadius, center.y - fRadius ),
																		BRUSH_COLOR,
																		false,
																		BRUSH_PARTS / 4 ); 		
				}
				sceneDrawTool.Draw3DLine( CTPoint<int>( pStoreInputState->lastEventInfo.gridPos.x - nBrushSize,
																								pStoreInputState->lastEventInfo.gridPos.y ),
																	CTPoint<int>( pStoreInputState->lastEventInfo.gridPos.x + nBrushSize,
																								pStoreInputState->lastEventInfo.gridPos.y ),
																	BRUSH_COLOR,
																	true,
																	true,
																	false );
				sceneDrawTool.Draw3DLine( CTPoint<int>( pStoreInputState->lastEventInfo.gridPos.x,
																								pStoreInputState->lastEventInfo.gridPos.y - nBrushSize ),
																	CTPoint<int>( pStoreInputState->lastEventInfo.gridPos.x,
																								pStoreInputState->lastEventInfo.gridPos.y + nBrushSize ),
																	BRUSH_COLOR,
																	true,
																	true,
																	false );
			}
		}
	}

	//extra drawing
	//if ( IEditorScene *pScene = EditorScene() )
	//{
	//	if ( ITerraManager *pTerraManager = pScene->GetTerrain() )
	//	{
	//		if ( const NDb::STerrain *pTerraDesc = pTerraManager->GetDesc() )
	//		{
	//			for ( std::vector< NDb::SVSOInstance >::const_iterator it = pTerraDesc->crags.begin(); it != pTerraDesc->crags.end(); ++it )
	//			{
	//				const float DEF_TILE_SIZE = 2.75f;
	//				std::vector<CVec3> vVerts;
	//				std::vector<CVec3> vNorms;
	//				std::vector<CVec3> vRidges;
	//				std::vector<float> vHeights;
	//				std::vector<NDb::SVSOPoint> vPoints;
	//				pTerraManager->GetCragPrecVerts( &vVerts, it->nVSOID );
	//				pTerraManager->GetCragPrecNorms( &vRidges, it->nVSOID );
	//				pTerraManager->GetCragHeights( &vHeights, it->nVSOID );
	//				pTerraManager->GetCragRidge( &vRidges, it->nVSOID );			// special size
	//				pTerraManager->GetCragSampPoints( &vPoints, it->nVSOID );
	//				const int nVertsCount = vVerts.size();
	//				const int nNormCount = vNorms.size();
	//				const int nHeightsCount = vHeights.size();
	//				const int nRidgesCount = vRidges.size();
	//				const int nSampPointsCount = vPoints.size();
	//				if ( nRidgesCount == 0 )
	//					continue;
	//				//
	//				int i = 0;
	//				while ( i < nVertsCount )
	//				{
	//					CVec3 vVert1( vVerts[i] );
	//					//CVec3 vRH1( vVerts[i].x, vVerts[i].y, pTerraManager->GetTerraHeight(vVerts[i].x, vVerts[i].y) );
	//					//CVec3 vHeight1( vVerts[i].x, vVerts[i].y, vHeights[i] );
	//					CVec3 vRidge1( vRidges[i].x, vRidges[i].y, vRidges[i].z );// + vVerts[i].z );
	//					CVec3 vNorm1( vVerts[i] + vPoints[i].vNorm );
	//					CVec3 vUp1( vVerts[i] );
	//					{
	//						CVec3 vUpP1( vVerts[i] + vPoints[i].vNorm * DEF_TILE_SIZE );
	//						vUp1.z = pTerraManager->GetTerraHeight( vUpP1.x, vUpP1.y );
	//					}
	//					CVec3 vDown1( vVerts[i] );
	//					{
	//						CVec3 vDownP1( vVerts[i] - vPoints[i].vNorm * DEF_TILE_SIZE );
	//						vDown1.z = pTerraManager->GetTerraHeight( vDownP1.x, vDownP1.y );
	//					}
	//					//CVec3 vSPos1( vPoints[i].vPos );
	//					//
	//					Vis2AI( &vVert1 );
	//					//Vis2AI( &vRH1 );
	//					//Vis2AI( &vHeight1 );
	//					Vis2AI( &vRidge1 );
	//					Vis2AI( &vNorm1 );
	//					Vis2AI( &vUp1 );
	//					Vis2AI( &vDown1 );
	//					//Vis2AI( &vSPos1 );
	//					//Vis2AI( &vHeightPt1 );
	//					//
	//					++i;
	//					if ( i >= nVertsCount )
	//						break;
	//					///
	//					CVec3 vVert2( vVerts[i] );
	//					//CVec3 vRH2( vVerts[i].x, vVerts[i].y, pTerraManager->GetTerraHeight(vVerts[i].x, vVerts[i].y) );
	//					//CVec3 vHeight2( vVerts[i].x, vVerts[i].y, vHeights[i] );
	//					CVec3 vRidge2( vRidges[i].x, vRidges[i].y, vRidges[i].z );// + vVerts[i].z );
	//					CVec3 vNorm2( vVerts[i] + vPoints[i].vNorm );
	//					CVec3 vUp2( vVerts[i] );
	//					{
	//						CVec3 vUpP2( vVerts[i] + vPoints[i].vNorm * DEF_TILE_SIZE );
	//						vUp2.z = pTerraManager->GetTerraHeight( vUpP2.x, vUpP2.y );
	//					}
	//					CVec3 vDown2( vVerts[i] );
	//					{
	//						CVec3 vDownP2( vVerts[i] - vPoints[i].vNorm * DEF_TILE_SIZE );
	//						vDown2.z = pTerraManager->GetTerraHeight( vDownP2.x, vDownP2.y );
	//					}
	//					//CVec3 vSPos2( vPoints[i].vPos );
	//					//
	//					Vis2AI( &vVert2 );
	//					//Vis2AI( &vRH2 );
	//					//Vis2AI( &vHeight2 );
	//					Vis2AI( &vRidge2 );
	//					Vis2AI( &vNorm2 );
	//					Vis2AI( &vUp2 );
	//					Vis2AI( &vDown2 );
	//					//Vis2AI( &vSPos2 );
	//					//Vis2AI( &vHeightPt2 );
	//					//
	//					sceneDrawTool.DrawLine( vVert1, vVert2, SELECTION_COLOR, false ); // green
	//					//sceneDrawTool.DrawLine( vHoleH1, VNULL3, PARCEL_COLOR_UNKNOWN, false );
	//					//sceneDrawTool.DrawLine( vHeight1, vHeight2, PARCEL_COLOR_REINFORCE, false ); // yellow
	//					sceneDrawTool.DrawLine( vRidge1, vRidge2, OBJECT_LINK_COLOR, false ); // pink
	//					sceneDrawTool.DrawLine( vVert1, vNorm1, OBJECT_LINK_COLOR, false ); // pink
	//					sceneDrawTool.DrawLine( vVert2, vNorm2, OBJECT_LINK_COLOR, false ); // pink
	//					//sceneDrawTool.DrawLine( vSPos1, vSPos2, PLACEMENT_COLOR, false ); // red - unknown curve
	//					sceneDrawTool.DrawLine( vUp1, vUp2, PLACEMENT_COLOR, false ); // red - unknown curve
	//					sceneDrawTool.DrawLine( vDown1, vDown2, PARCEL_COLOR_REINFORCE, false ); // yellow
	//				}
	//			}
	//		}
	//	}
	//}

	sceneDrawTool.Draw();
}


bool CHeightStateV3::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		switch( nCommandID ) 
		{
			case ID_GET_EDIT_PARAMETERS:
			{
				pEditParameters->nFlags = dwData;
				::GetEditParameters( pEditParameters, CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3 );
				GetEditParameters( dwData );
				break;
			}
			case ID_SET_EDIT_PARAMETERS:
			{
				pEditParameters->nFlags = dwData;
				SetEditParameters( dwData );
				::SetEditParameters( *pEditParameters, CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3 );
				break;
			}
			case ID_MITHV3_ON_TIMER:
			{
				ProcessTerrain( static_cast<SEditParameters::EBrush>( Clamp<uint32_t>( dwData, SEditParameters::B_TILE, SEditParameters::B_PLATO ) ) );
				return true;
			}
			case ID_MITHV3_UPDATE_HEIGHT:
			{
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_EDITOR, ID_TOOLS_UPDATE_VSO, 0 );
				break;
			}
			default:
				break;
		} 
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
	return true;
}


bool CHeightStateV3::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CHeightStateV3::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CHeightStateV3::UpdateCommand(), pbCheck == 0" );
	//
	( *pbEnable ) = false;
	( *pbCheck ) = false;
	switch( nCommandID ) 
	{
		case ID_GET_EDIT_PARAMETERS:
		case ID_SET_EDIT_PARAMETERS:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_MITHV3_ON_TIMER:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_MITHV3_UPDATE_HEIGHT:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		default:
			return false;
	}
	return false;
}


void CHeightStateV3::OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	pStoreInputState->OnMouseMove( nFlags, rMousePoint );
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		if ( CanEdit() )
		{
			CMultiInputState::OnMouseMove( nFlags, rMousePoint );
			//
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CHeightStateV3::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	pStoreInputState->OnLButtonDown( nFlags, rMousePoint );
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		if ( CanEdit() )
		{
			pMapInfoEditor->heightContainer.Mark( pStoreInputState->lastEventInfo.gridPos.x, pStoreInputState->lastEventInfo.gridPos.y );
		}
		bEscaped = false;
		CMultiInputState::OnLButtonDown( nFlags, rMousePoint );
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CHeightStateV3::OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	pStoreInputState->OnRButtonDown( nFlags, rMousePoint );
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		if ( CanEdit() )
		{
			pMapInfoEditor->heightContainer.Mark( pStoreInputState->lastEventInfo.gridPos.x, pStoreInputState->lastEventInfo.gridPos.y );
		}
		bEscaped = false;
		CMultiInputState::OnRButtonDown( nFlags, rMousePoint );
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CHeightStateV3::OnMButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	pStoreInputState->OnMButtonDown( nFlags, rMousePoint );
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		if ( CanEdit() )
		{
			pMapInfoEditor->heightContainer.Mark( pStoreInputState->lastEventInfo.gridPos.x, pStoreInputState->lastEventInfo.gridPos.y );
		}
		bEscaped = false;
		CMultiInputState::OnMButtonDown( nFlags, rMousePoint );
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CHeightStateV3::OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	pStoreInputState->OnLButtonUp( nFlags, rMousePoint );
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		if ( CanEdit() )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_KILL_TIMER, 0 );
			CreateMapInfoController();
		}
		bEscaped = false;
		CMultiInputState::OnLButtonUp( nFlags, rMousePoint );
		//
		if ( pEditParameters->bUpdateHeight )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_EDITOR, ID_TOOLS_UPDATE_VSO, 0 );
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CHeightStateV3::OnRButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	pStoreInputState->OnRButtonUp( nFlags, rMousePoint );
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		if ( CanEdit() )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_KILL_TIMER, 0 );
			CreateMapInfoController();
		}
		bEscaped = false;
		CMultiInputState::OnRButtonUp( nFlags, rMousePoint );
		//
		if ( pEditParameters->bUpdateHeight )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_EDITOR, ID_TOOLS_UPDATE_VSO, 0 );
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CHeightStateV3::OnMButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	pStoreInputState->OnMButtonUp( nFlags, rMousePoint );
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		if ( CanEdit() )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_KILL_TIMER, 0 );
			CreateMapInfoController();
		}
		bEscaped = false;
		CMultiInputState::OnMButtonUp( nFlags, rMousePoint );
		//
		if ( pEditParameters->bUpdateHeight )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_EDITOR, ID_TOOLS_UPDATE_VSO, 0 );
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CHeightStateV3::OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	pStoreInputState->OnKeyDown( nChar, nRepCnt, nFlags );
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		if ( CanEdit() )
		{
			if ( ( nChar >= 0x31 ) && ( nChar <= 0x35 ) )
			{
				pEditParameters->eBrush = static_cast<SEditParameters::EBrush>( Clamp<uint32_t>( nChar - 0x31, SEditParameters::B_TILE, SEditParameters::B_PLATO ) );
				SetActiveInputState( pEditParameters->eBrush, true, false );
				pEditParameters->nFlags = MITHV3EP_BRUSH;
				::SetEditParameters( *pEditParameters, CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3 );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			}
			else if ( nChar == VK_ESCAPE )
			{
				if ( pStoreInputState->lastEventInfo.isValid )
				{
					if ( IEditorScene *pScene = EditorScene() )
					{
						if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
						{
							bEscaped = true;
							Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_TERRAIN_HEIGHT_WINDOW_V3, ID_MITHV3_KILL_TIMER, 0 );
							pTerraManager->GetTileTypeUpdateDifferences( &( vTileDiffPos.x ), &( vTileDiffPos.y ), &tileDiff );
							if ( ( tileDiff.GetSizeX() > 0 ) && ( tileDiff.GetSizeY() > 0 ) )
							{
								pTerraManager->UpdateTileAreaType( vTileDiffPos.x, vTileDiffPos.y, tileDiff, NTerraBrush::TERRA_BRUSH_SUB );
							}
							pTerraManager->GetTerraGeometryUpdateDifferences( &( heightDiffPos.x ), &( heightDiffPos.y ), &heightDiff );
							if ( ( heightDiff.GetSizeX() > 0 ) && ( heightDiff.GetSizeY() > 0 ) )
							{
								pTerraManager->ModifyTerraGeometryByBrush( heightDiffPos.x,
																													 heightDiffPos.y,
																													 true,
																													 heightDiff,
																													 NTerraBrush::TERRA_BRUSH_SUB );
							}
							bTileEditStarted =false;
							bHeightEditStarted = false;
						}
					}
				}
			}
		}
		CMultiInputState::OnKeyDown( nChar, nRepCnt, nFlags );
	}
}


void CHeightStateV3::OnKeyUp( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	pStoreInputState->OnKeyUp( nChar, nRepCnt, nFlags );
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		if ( CanEdit() )
		{
			CMultiInputState::OnKeyUp( nChar, nRepCnt, nFlags );
		}
	}
}


int CHeightStateV3::SEditParameters::operator&( IXmlSaver &xs )
{
	xs.Add( "Brush", &eBrush );
	xs.Add( "BrushSize", &eBrushSize );
	xs.Add( "BrushType", &eBrushType );
	xs.Add( "LevelTo", &fLevelTo );
	xs.Add( "UpdateHeight", &bUpdateHeight );
	xs.Add( "TileIndex", &nTileIndex );
	xs.Add( "Thumbnails", &bThumbnails );
	xs.Add( "TileBrushSizeList", &tileBrushSizeList );
	xs.Add( "HeightBrushSizeList", &heightBrushSizeList );
	//
	//do not serialise this fields:
	// unsigned nFlags;
	// std::vector<std::string> tileList;
	return 0;
}

// basement storage  


