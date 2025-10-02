#include "stdafx.h"

#include "StaticPathInternal.h"
#include "CommonPathFinder.h"

#include "Common_RTS_AI/AIMap.h"

const int DIRECTION_OFFSET = 32;

REGISTER_SAVELOAD_CLASS( 0x3008CB00, CCommonStaticPath );
BASIC_REGISTER_CLASS( IStaticPath );

//*******************************************************************
//*												CCommonStaticPath													*
//*******************************************************************

CCommonStaticPath::CCommonStaticPath( CCommonPathFinder *pStaticPathFinder, CAIMap *_pAIMap )
: nLen( pStaticPathFinder->GetPathLength() ),
	startTile( pStaticPathFinder->GetStartTile() ), finishTile( pStaticPathFinder->GetFinishTile() ),
	finishPoint( pStaticPathFinder->GetFinishPoint() ), pAIMap( _pAIMap )
{
	NI_ASSERT( nLen >= 0 && nLen <= pStaticPathFinder->GetPathLength(), "Wrong length" );

	if ( path.size() < nLen )
		path.resize( nLen * 1.5, SVector(0,0) );

	if ( nLen > 0 )
		pStaticPathFinder->GetTiles( &(path[0]), nLen );
}

void CCommonStaticPath::MoveStartTileTo( const int nStart )
{
	const int nDelta = Min( nLen, nStart );
	nLen -= nDelta;

	startTile = path[nDelta];
	path.erase( path.begin(), path.begin() + nDelta );
}

void CCommonStaticPath::MoveFinishTileTo( const int nFinish )
{
	nLen = Clamp( nFinish, 1, nLen );
	finishTile = path[nLen - 1];
	finishPoint = pAIMap->GetPointByTile( finishTile );
}

void CCommonStaticPath::MoveFinishPointBy( const CVec2 &vMove ) 
{ 
	if ( pAIMap->GetTile( finishPoint + vMove ) == finishTile )
		finishPoint += vMove;
}

bool CCommonStaticPath::MergePath( IStaticPath *pAppendant, const int _nStartTile )
{
	if ( !pAppendant || pAppendant->GetLength() <= _nStartTile )
		return false;
	if ( mDistance( pAppendant->GetTile( _nStartTile ), path[nLen-1] ) > 2 )
		return false;
	const int nStartTile = pAppendant->GetTile( _nStartTile ) == path[nLen-1] ? _nStartTile + 1 : _nStartTile;
	const int nOldSize = path.size();
	nLen = nOldSize + pAppendant->GetLength() - nStartTile;
	path.resize( path.size() + pAppendant->GetLength() - nStartTile + 1 );
	for ( int i = nStartTile; i < pAppendant->GetLength(); ++i )
		path[nOldSize+i-nStartTile] = pAppendant->GetTile( i );
	finishTile = path[nLen - 1];
	finishPoint = pAIMap->GetPointByTile( finishTile );
	return true;
}

int CCommonStaticPath::MarkStaticPath( const int nID, const NDebugInfo::EColor color ) const
{
	vector<SVector> tiles;
	for ( int i = 0; i < nLen; ++i )
		tiles.push_back( path[i] );

	return DebugInfoManager()->CreateMarker( nID, tiles, color );
}


