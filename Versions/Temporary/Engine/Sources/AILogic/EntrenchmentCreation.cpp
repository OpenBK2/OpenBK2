#include "StdAfx.h"

#include "..\misc\bresenham.h"
#include "EntrenchmentCreation.h"
#include "UnitsIterators2.h"
#include "AIUnit.h"
#include "UnitStates.h"
#include "UnitCreation.h"
#include "StaticObjectsIters.h"
#include "..\Stats_B2_M1\AIUpdates.h"
#include "Entrenchment.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const static float fNearToNormale = 0.05f; 
extern CStaticObjects theStatObjs;
extern CUnitCreation theUnitCreation;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_SAVELOAD_CLASS( 0x1508D4AA, CEntrenchmentCreation );
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CEntrenchmentCreation::OnSerialize( IBinSaver &saver )
{
	if ( saver.IsReading() ) 
	{
		if ( pEntrenchmentStats == 0 ) 
			InitConsts();
		else
			nTermInd = pEntrenchmentStats->GetTerminatorIndex();
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CEntrenchmentCreation::CEntrenchmentCreation( const int nPlayer, const bool bAllowAIModification )
: CLongObjectCreation( nPlayer, bAllowAIModification ), nStartIndex( 0 ), nCurIndex( -1 ), 
	line( 0, 0, 0), wAngle( 0 ), bCannot( false ), bSayAck( false )
{
	InitConsts();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CEntrenchmentCreation::InitConsts()
{
	pEntrenchmentStats = theUnitCreation.GetEntrenchment();
	nTermInd = pEntrenchmentStats == 0 ? 0 : pEntrenchmentStats->GetTerminatorIndex();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CEntrenchmentCreation::SearchTrenches( const CVec2 &vCenter, const SRect &rectToTest )
{
	const float fMaxSize = Max( rectToTest.lengthAhead, Max(rectToTest.lengthBack, rectToTest.width ) ) + 2 * SConsts::TILE_SIZE;
	// просканировать в радиусе на наличие окопов
	for ( CStObjCircleIter<false> iter( vCenter, fMaxSize ); !iter.IsFinished(); iter.Iterate() )
	{
		CStaticObject *pObj = *iter;
		if ( ESOT_ENTR_PART == pObj->GetObjectType() )
		{
			SRect objRect;
			pObj->GetBoundRect( &objRect );
			if ( rectToTest.IsIntersected( objRect ) )
				return true;
		}
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CEntrenchmentCreation::GetEntrenchmentID() const
{
	return pFullEntrenchment->GetUniqueId();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CEntrenchmentCreation::CreateObjects( SAIObjectsUnderConstructionUpdate * pUpdate )
{
	if ( vPoints.empty() )
		return;
	// add all objects, check if tiles are locked and mark objects and tiles.
	pUpdate->objects.resize( parts.size() + 2 );
	
	// add terminators
	pUpdate->objects[parts.size()].bCanBuild = true;
	pUpdate->objects[parts.size()+1].bCanBuild = true;
	pBeginTerminator->GetNewUnitInfo( &pUpdate->objects[parts.size()].info );
	pEndTerminator->GetNewUnitInfo( &pUpdate->objects[parts.size()+1].info );

	pUpdate->bCanBuild = false;
	CDBPtr<SEntrenchmentRPGStats> pRPG = theUnitCreation.GetEntrenchment();
	for ( int i = 0; i < parts.size(); ++i )
	{
		// check part
		const SRect rect = CEntrenchmentPart::CalcBoundRect( vPoints[i], wAngle, pRPG->segments[parts[i]->GetFrameIndex()] );
		list<SVector> tiles;
		GetAIMap()->GetTilesCoveredByRect( rect, &tiles );
		bool bPossibleBecauseOfTiles = true;
		for ( list<SVector>::iterator it = tiles.begin(); it!= tiles.end(); ++it )
		{
			if ( !CanDigTile( *it ) )
				pUpdate->impossibleToBuildTiles.push_back( CVec2(it->x, it->y) );
			else
			{
				pUpdate->buildTiles.push_back( CVec2(it->x, it->y) );
				bPossibleBecauseOfTiles  = false;
			}
		}

		parts[i]->GetNewUnitInfo( &pUpdate->objects[i].info );
		pUpdate->objects[i].bCanBuild = bPossibleBecauseOfTiles && CanDigBecauseOfOtherTrenches( rect, vPoints[i] );
		
		pUpdate->bCanBuild |= pUpdate->objects[i].bCanBuild;
	}
	if ( !pUpdate->objects[0].bCanBuild )
		pUpdate->bCanBuild = false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CEntrenchmentCreation::CanDigTile( const SVector &tile ) const
{
	return GetAIMap()->IsTileInside( tile ) && GetTerrain()->CanDigEntrenchment( tile.x, tile.y ) &&
				 0 == (EAC_TERRAIN & GetTerrain()->GetTileLockInfo( tile ));
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CEntrenchmentCreation::PreCreate( const CVec2 &vFrom, const CVec2 &vTo, const bool bCheckLock )
{
	bCannot = false;
	bSayAck = false;
	nCurIndex = 0;
	nStartIndex = 0;
	line = CLine2( vFrom, vTo );
	//NI_ASSERT( vPoints.empty() && parts.empty(), "repeative calls are not allowed" );
	vPoints.clear();
	parts.clear();

	CDBPtr<SEntrenchmentRPGStats> pRPG = theUnitCreation.GetEntrenchment();
  int nTermInd = pRPG->GetTerminatorIndex( 0 );

	const float fTrenchWidth = GetTrenchWidth( 0 );
	SplitLineToSegrments( &vPoints, vFrom, vTo, fTrenchWidth );

	wAngle = GetLineAngle( vFrom, vTo );
	
	if ( vPoints.size() <= 1 ) return false;

	//lines and fireplaces
	bool switcher = false;
	int nFrameIndex;
	for ( int i = 0; i < vPoints.size() - 1; ++i )
	{
		switcher = ((i+1)%3) == 0;
		//решить что стрoить - fire place or line
		nFrameIndex = switcher ? pRPG->GetFirePlaceIndex( 0 ) : pRPG->GetLineIndex( 0 );
		const CVec2	pt = ( vPoints[i] + vPoints[i + 1] ) / 2.0f;
		//
		if ( !bCheckLock || CanDig( pRPG, pt, wAngle, nFrameIndex ) )
			parts.push_back( AddElement( pRPG, CVec3(pt,0.0f), wAngle, nFrameIndex, GetPlayer() ) );
		else
			parts.push_back( 0 );
		//
	}
	if ( parts.empty() ) 
		return false;

	if ( IsAIModificationAllowed() )
	{
		do 
		{
			CalcTilesUnder();
		} while ( !CanBuildNextInner() && ++nCurIndex < parts.size()  );
	}

	nStartIndex = nCurIndex;
	if ( nStartIndex >= parts.size() )
		return false;

	pBeginTerminator = AddElement( pRPG, CVec3(vPoints[nCurIndex],0.0f), wAngle+ 65535/2, nTermInd, GetPlayer() );
	if ( !bCheckLock )
		pEndTerminator = AddElement( pRPG, CVec3(vPoints[vPoints.size()-1],0.0f), wAngle, nTermInd, GetPlayer() );
	CreateNewEndTerminator();
	CalcTilesUnder();
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int CEntrenchmentCreation::GetMaxIndex() const
{
	return parts.size() - nStartIndex;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int CEntrenchmentCreation::GetCurIndex() const
{
	return nCurIndex - nStartIndex;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CVec2 CEntrenchmentCreation::GetNextPoint( const int nPlace, const int nMaxPlace ) const
{
	NI_ASSERT( nMaxPlace, "builders number = 0" );

	const CVec2 vDir = GetVectorByDirection( wAngle + 65535/4 );
	SRect rect;
	parts[nCurIndex]->GetBoundRect( &rect );
	return CVec2(parts[nCurIndex]->GetCenter().x,parts[nCurIndex]->GetCenter().y) + vDir * ( rect.lengthAhead+rect.lengthBack) * (nPlace -nMaxPlace/2) / nMaxPlace;

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CVec2 CEntrenchmentCreation::GetBuildPointForIndex( const int nPlace, const int nMaxPlace, const int _nIndex ) const
{
	NI_ASSERT( nMaxPlace, "builders number = 0" );
	NI_ASSERT( _nIndex < GetMaxIndex() && _nIndex >= GetCurIndex(), "invalid index" );

	const int nIndex = _nIndex + nStartIndex;

	const CVec2 vDir = GetVectorByDirection( wAngle + 65535/4 );
	SRect rect;
	parts[nIndex]->GetBoundRect( &rect );
	return CVec2(parts[nIndex]->GetCenter().x,parts[nIndex]->GetCenter().y) + vDir * ( rect.lengthAhead+rect.lengthBack) * (nPlace -nMaxPlace/2) / nMaxPlace;

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CEntrenchmentCreation::LockCannotBuild()
{ 
	bCannot = true; 
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CEntrenchmentCreation::BuildNext()
{
	CLongObjectCreation::BuildNext();
	if ( nStartIndex == nCurIndex )
		theStatObjs.AddEntrencmentPart( pBeginTerminator, true );
	theStatObjs.AddEntrencmentPart( parts[nCurIndex], true );
	
	//
	++nCurIndex;
	//
	
	if ( pFullEntrenchment )
	{
		theStatObjs.DeleteInternalEntrenchmentInfo( pFullEntrenchment );
		pFullEntrenchment = 0;
	}
	if ( pEndTerminator )
	{
		theStatObjs.DeleteInternalObjectInfo( pEndTerminator );
		pEndTerminator = 0;
	}
	

	vector<CObjectBase*> vEntr;
	
	vEntr.push_back( pBeginTerminator );

	pEndTerminator = pNewEndTerminator;
	CreateNewEndTerminator();
	vEntr.push_back( pEndTerminator );

	for ( int i = nStartIndex; i < nCurIndex; ++i )
		vEntr.push_back( parts[i] );

	pFullEntrenchment = checked_cast<CEntrenchment*>(theStatObjs.AddNewEntrencment( &vEntr[0], vEntr.size(), 0, true ));

	if ( GetCurIndex() < GetMaxIndex() )
		CalcTilesUnder();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CEntrenchmentCreation::BuildAll( const int _nMinIndex, const int _nMaxIndex )
{
  const int nMinIndex = _nMinIndex + nStartIndex;
	const int nMaxIndex = _nMaxIndex + nStartIndex;

	if ( nMinIndex == nStartIndex )
		theStatObjs.AddEntrencmentPart( pBeginTerminator, false );

	for ( int nIndex = nMinIndex; nIndex < nMaxIndex; ++nIndex )
	{
		theStatObjs.AddEntrencmentPart( parts[nIndex], false );
	}
	if ( pFullEntrenchment )
	{
		theStatObjs.DeleteInternalEntrenchmentInfo( pFullEntrenchment );
		pFullEntrenchment = 0;
	}
	if ( pEndTerminator )
	{
		theStatObjs.DeleteInternalObjectInfo( pEndTerminator );
		pEndTerminator = 0;
	}

	CDBPtr<SEntrenchmentRPGStats> pRPG = theUnitCreation.GetEntrenchment();
	vector<CObjectBase*> vEntr;

	const int nTermInd = pRPG->GetTerminatorIndex( 0 );
	pEndTerminator = AddElement( pRPG, CVec3( vPoints[nMaxIndex], 0.0f ), wAngle, nTermInd, GetPlayer() );
	vEntr.push_back( pBeginTerminator );
	vEntr.push_back( pEndTerminator );

	theStatObjs.AddEntrencmentPart( pEndTerminator, false );

	for ( int nIndex = nMinIndex; nIndex < nMaxIndex; ++nIndex )
	{
		vEntr.push_back( parts[nIndex] );
	}

	pFullEntrenchment = checked_cast<CEntrenchment*>(theStatObjs.AddNewEntrencment( &vEntr[0], vEntr.size(), 0, false ));

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CEntrenchmentCreation::CalcTilesUnder()
{
	GetTilesUnderForIndex( &tilesUnder, GetCurIndex() );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CEntrenchmentCreation::GetTilesUnderForIndex( list<SVector> *pTiles, const int _nIndex ) const
{
	const int nIndex = _nIndex + nStartIndex;
	if ( parts[nIndex] )
		parts[nIndex]->GetCoveredTiles( pTiles );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CEntrenchmentCreation::CreateNewEndTerminator()
{
  CDBPtr<SEntrenchmentRPGStats> pRPG = theUnitCreation.GetEntrenchment();
	
  int nTermInd = pRPG->GetTerminatorIndex( 0 );

	if ( nCurIndex + 1 < vPoints.size() )
		pNewEndTerminator = AddElement( pRPG, CVec3(vPoints[nCurIndex+1],0.0f), wAngle, nTermInd, GetPlayer() );
	else 
		pNewEndTerminator = 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CEntrenchmentCreation::GetUnitsPreventing( list< CPtr<CAIUnit> > *units )
{
	return GetUnitsPreventingByIndex( units, GetCurIndex() );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CEntrenchmentCreation::GetUnitsPreventingByIndex( list< CPtr<CAIUnit> > *units, const int _nIndex )
{
	const int nIndex = _nIndex + nStartIndex;
	SRect r1, r2, r3 ;
	parts[nIndex]->GetBoundRect( &r1 );
	pNewEndTerminator->GetBoundRect( &r2 );
	if ( nStartIndex == nIndex ) 
		pBeginTerminator->GetBoundRect( &r3 );

	const float fRadius = r1.lengthAhead + r1.lengthBack + r1.width + r2.lengthAhead + r2.lengthBack + r2.width + SConsts::TILE_SIZE * 5;
	for ( CUnitsIter<0,2> iter( 0, ANY_PARTY, vPoints[nIndex], fRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		if ( !(*iter)->GetStats()->IsInfantry() )
		{
			const SRect rect = (*iter)->GetUnitRect();
			if ( r1.IsIntersected( rect ) || r2.IsIntersected( rect ) || 
				 ( nStartIndex == nIndex ? r3.IsIntersected( rect ) : false ) 
				 )
				units->push_back( *iter );
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CEntrenchmentCreation::IsAnyUnitPrevent() const
{
	return IsAnyUnitPreventByIndex( GetCurIndex() );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CEntrenchmentCreation::IsAnyUnitPreventByIndex( const int _nIndex ) const
{
	const int nIndex = _nIndex + nStartIndex;
	SRect r1, r2, r3 ;
	parts[nIndex]->GetBoundRect( &r1 );
	pNewEndTerminator->GetBoundRect( &r2 );
	if ( nStartIndex == nIndex ) 
		pBeginTerminator->GetBoundRect( &r3 );

	const float fRadius = r1.lengthAhead + r1.lengthBack + r1.width + r2.lengthAhead + r2.lengthBack + r2.width + SConsts::TILE_SIZE * 5;
	for ( CUnitsIter<0,2> iter( 0, ANY_PARTY, vPoints[nIndex], fRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		if ( !(*iter)->GetStats()->IsInfantry() )
		{
			const SRect rect = (*iter)->GetUnitRect();
			if ( r1.IsIntersected( rect ) || r2.IsIntersected( rect ) || 
				( nStartIndex == nIndex ? r3.IsIntersected( rect ) : false ) 
				)
				return true;
		}
	}
	return false;

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CEntrenchmentCreation::CanBuildNext() const
{
	if ( bCannot || !pNewEndTerminator ) 
		return false;

	return CanBuildNextInner();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CEntrenchmentCreation::CanBuildByIndexSlow( const int _nIndex ) const
{
	if ( bCannot || !pNewEndTerminator ) 
		return false;

	return CanBuildNextInnerSlow( _nIndex + nStartIndex );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CEntrenchmentCreation::CanBuildNextInner() const
{
	if ( !parts[nCurIndex] )
		return false;

	SRect r1;
	parts[nCurIndex]->GetBoundRect( &r1 );
	if ( !GetAIMap()->IsRectInside( r1 ) )
		return false;

	const float fRadius = r1.lengthAhead + r1.lengthBack + r1.width + SConsts::TILE_SIZE * 5;
	// пробежаться по юнитам, все разлокать.
	for ( CUnitsIter<0,2> iter( 0, ANY_PARTY, vPoints[nCurIndex], fRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit * pUnit = *iter;
		if ( !pUnit->GetStats()->IsInfantry() )
		{
			SRect rect = pUnit->GetUnitRect();
			rect.InitRect( rect.center, rect.dir, rect.lengthAhead + SConsts::TILE_SIZE, rect.lengthBack + SConsts::TILE_SIZE, rect.width + SConsts::TILE_SIZE );

			if ( r1.IsIntersected( rect ) )
			{
				if ( pUnit->GetPlayer() == GetPlayer() && 
					EUSN_REST == pUnit->GetState()->GetName() &&
					pUnit->CanMove() )
					(*iter)->UnlockTiles();
			}
		}
	}

	// теперь проверить, можно ли строить
	for ( list<SVector>::const_iterator it = tilesUnder.begin(); it != tilesUnder.end(); ++it )
	{
		if ( !CanDigTile( *it ) )
			return false;
	}
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CEntrenchmentCreation::CanBuildNextInnerSlow( const int nIndex ) const
{
	if ( !parts[nIndex] )
		return false;

	SRect r1;
	parts[nIndex]->GetBoundRect( &r1 );
	if ( !GetAIMap()->IsRectInside( r1 ) )
		return false;

	const float fRadius = r1.lengthAhead + r1.lengthBack + r1.width + SConsts::TILE_SIZE * 5;
	// пробежаться по юнитам, все разлокать.
	for ( CUnitsIter<0,2> iter( 0, ANY_PARTY, vPoints[nIndex], fRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit * pUnit = *iter;
		if ( !pUnit->GetStats()->IsInfantry() )
		{
			SRect rect = pUnit->GetUnitRect();
			rect.InitRect( rect.center, rect.dir, rect.lengthAhead + SConsts::TILE_SIZE, rect.lengthBack + SConsts::TILE_SIZE, rect.width + SConsts::TILE_SIZE );

			if ( r1.IsIntersected( rect ) )
			{
				if ( pUnit->GetPlayer() == GetPlayer() && 
					EUSN_REST == pUnit->GetState()->GetName() &&
					pUnit->CanMove() )
					(*iter)->UnlockTiles();
			}
		}
	}

	list<SVector> tiles;
	GetTilesUnderForIndex( &tiles, nIndex - nStartIndex );

	// теперь проверить, можно ли строить
	for ( list<SVector>::const_iterator it = tilesUnder.begin(); it != tilesUnder.end(); ++it )
	{
		if ( !CanDigTile( *it ) )
			return false;
	}
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float CEntrenchmentCreation::GetPrice()
{
	return SConsts::ENTRENCHMENT_SEGMENT_RU_PRICE;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CEntrenchmentCreation::LockNext()
{
	parts[nCurIndex]->LockTiles();
	pNewEndTerminator->LockTiles();
	if ( nStartIndex == nCurIndex )
		pBeginTerminator->LockTiles();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CEntrenchmentPart * CEntrenchmentCreation::AddElement( const SEntrenchmentRPGStats *pRPG, const CVec3 &pt, WORD angle, int nFrameIndex, int nPlayer )
{
	//create entrenchments
	CEntrenchmentPart *ptr = new CEntrenchmentPart( pRPG, pt, angle, nFrameIndex, pRPG->fMaxHP, nPlayer, true );
	if ( IsAIModificationAllowed() )
		ptr->Mem2UniqueIdObjs();

	ptr->Init();
	return ptr;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CEntrenchmentCreation::CanDigBecauseOfOtherTrenches( const SRect &rect, const CVec2 &pt ) const
{
	if ( !GetAIMap()->IsRectInside( rect ) )
		return false;

	/*const CVec3 vNormal =  DWORDToVec3( GetHeights()->GetNormal( pt ) );
	if ( fabs(vNormal.x) > fNearToNormale * fabs(vNormal.z) &&
		fabs(vNormal.y) > fNearToNormale * fabs(vNormal.z) )
		return false;*/

	return !SearchTrenches( pt, rect );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CEntrenchmentCreation::CanDig( const SEntrenchmentRPGStats *pRPG, const CVec2 &pt, WORD angle, int nFrameIndex )
{
	SRect rect = CEntrenchmentPart::CalcBoundRect( pt, angle, pRPG->segments[nFrameIndex] );
	if ( !CanDigBecauseOfOtherTrenches( rect, pt ) )
		return false;

	list<SVector> tiles;
	GetAIMap()->GetTilesCoveredByRect( rect, &tiles );
	bool bPossible = true;
	for ( list<SVector>::iterator i = tiles.begin(); i!= tiles.end(); ++i )
	{
		if ( !CanDigTile( *i ) )
		{
			bPossible = false;
			break;
		}
	}

	return bPossible;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*												  CEntrenchmentCreation										*
//*******************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float CEntrenchmentCreation::GetTrenchWidth( int nType )// 0 - секция , 1 - поворот
{
	const SEntrenchmentRPGStats *pRPG = theUnitCreation.GetEntrenchment();

	int nFrameIndex = 0;
	if ( nType == 0 )
		nFrameIndex = pRPG->GetLineIndex( 0 );
	if ( nType == 1 )
		nFrameIndex = pRPG->GetArcIndex( 0 );

	return pRPG->segments[nFrameIndex].vAABBHalfSize.x * 2;
}
