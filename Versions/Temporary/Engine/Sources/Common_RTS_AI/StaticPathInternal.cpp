#include "stdafx.h"

#include "StaticPathInternal.h"
#include "CommonPathFinder.h"

#include "Common_RTS_AI/AIMap.h"

#include <limits>

const int DIRECTION_OFFSET = 32;

REGISTER_SAVELOAD_CLASS( COMMON_RTS_AI, 0x3008CB00, CCommonStaticPath );
BASIC_REGISTER_CLASS( COMMON_RTS_AI, IStaticPath );

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
	const int nDelta = (std::min)( nLen, nStart );
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
	if ( !pAppendant || !pAIMap || nLen <= 0 || nLen > path.size() || _nStartTile < 0 )
		return false;
	const int nAppendantLength = pAppendant->GetLength();
	if ( nAppendantLength <= _nStartTile )
		return false;
	if ( mDistance( pAppendant->GetTile( _nStartTile ), path[nLen-1] ) > 2 )
		return false;
	const int nStartTile = pAppendant->GetTile( _nStartTile ) == path[nLen-1] ? _nStartTile + 1 : _nStartTile;
	const int nTilesToAppend = nAppendantLength - nStartTile;
	if ( nTilesToAppend > (std::numeric_limits<int>::max)() - nLen )
		return false;
	const int nNewLength = nLen + nTilesToAppend;
	// path can contain allocation padding or a trimmed suffix; only nLen tiles
	// belong to the route. Keep one extra endpoint for MoveStartTileTo(nLen).
	if ( path.size() <= nNewLength )
		path.resize( size_t(nNewLength) + 1 );
	// Keep both logical lengths unchanged while copying, including self-appends.
	for ( int i = nStartTile; i < nAppendantLength; ++i )
		path[nLen + (i - nStartTile)] = pAppendant->GetTile( i );
	nLen = nNewLength;
	finishTile = path[nLen - 1];
	path[nLen] = finishTile;
	finishPoint = pAIMap->GetPointByTile( finishTile );
	return true;
}

int CCommonStaticPath::MarkStaticPath( const int nID, const NDebugInfo::EColor color ) const
{
	std::vector<SVector> tiles;
	for ( int i = 0; i < nLen; ++i )
		tiles.push_back( path[i] );

	return DebugInfoManager()->CreateMarker( nID, tiles, color );
}


