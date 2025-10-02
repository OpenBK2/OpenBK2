#include "StdAfx.h"
#include "HeightContainer.h"
#include "Misc/Bresenham.h"
#include "Image/Targa.h"
#include "B2_M1_Terrain/DBVSO.h"

int CHeightContainer::STACK_SIZE = sizeof( DWORD ) * 8;
int CHeightContainer::TRACE_IMAGE_TILE_SIZE = 64;


struct STraceImageFunctor
{
	DWORD dwColor;
	CArray2D<DWORD> *pImage;
	STraceImageFunctor( DWORD _dwColor, CArray2D<DWORD> *_pImage ) : dwColor( _dwColor ), pImage( _pImage ) {}
	void operator()( int x, int y )
	{
		if ( ( x >= 0 ) && ( x < pImage->GetSizeX() ) && ( y >= 0 ) && ( y < pImage->GetSizeY() ) )
		{
			( *pImage )[y][x] = dwColor;
		}
	}
};


void CHeightContainer::MarkTraceImageTile( CArray2D<DWORD> *pImage, int x, int y, DWORD dwColor )
{
	const CTRect<int> indices( x * TRACE_IMAGE_TILE_SIZE - 2,
														 y * TRACE_IMAGE_TILE_SIZE - 2,
														 x * TRACE_IMAGE_TILE_SIZE + 2,
														 y * TRACE_IMAGE_TILE_SIZE + 2 );
	for ( int y = indices.miny; y <= indices.maxy; ++y )
	{
		for ( int x = indices.minx; x <= indices.maxx; ++x )
		{
			if ( ( x >= 0 ) && ( x < pImage->GetSizeX() ) && ( y >= 0 ) && ( y < pImage->GetSizeY() ) )
			{
				( *pImage )[y][x] = dwColor;
			}
		}
	}
}


void CHeightContainer::MarkTraceImageGrid( CArray2D<DWORD> *pImage, DWORD dwColor )
{
	for ( int x = 0; x < planeSize.x; ++x )
	{
		for ( int y = 0; y <= ( planeSize.y - 1 ) * TRACE_IMAGE_TILE_SIZE; ++y )
		{
			( *pImage )[y][x * TRACE_IMAGE_TILE_SIZE] = dwColor;
		}
	}
	for ( int y = 0; y < planeSize.y; ++y )
	{
		for ( int x = 0; x <= ( planeSize.x - 1 ) * TRACE_IMAGE_TILE_SIZE; ++x )
		{
			( *pImage )[y * TRACE_IMAGE_TILE_SIZE][x] = dwColor;
		}
	}
}


CHeightContainer::CHeightContainer( const float _fTileSize )
	: fTileSize ( _fTileSize ), planeSize( 0, 0 ), nStackCount( 0 ), bTraceToImage( false ) {}


void CHeightContainer::AddStack()
{
	++nStackCount;
	{
		vector<CArray2D<DWORD> >::iterator posPlaneStack = blackPlaneStackList.insert( blackPlaneStackList.end(), CArray2D<DWORD>() );
		posPlaneStack->SetSizes( planeSize.x, planeSize.y );
		posPlaneStack->FillZero();
	}
	{
		vector<CArray2D<DWORD> >::iterator posPlaneStack = redPlaneStackList.insert( redPlaneStackList.end(), CArray2D<DWORD>() );
		posPlaneStack->SetSizes( planeSize.x, planeSize.y );
		posPlaneStack->FillZero();
	}
	markedBitslList.push_back( 0 );
	filledBitsList.push_back( 0 );
}


void CHeightContainer::EraseStack()
{
}


int CHeightContainer::AddPlane( int nPolygonID )
{
	hash_map<int, int>::iterator posPolygonID2PlaneIndex = polygonID2PlaneIndexMap.find( nPolygonID );
	if ( posPolygonID2PlaneIndex != polygonID2PlaneIndexMap.end() )
	{
		const int nPlaneIndex = posPolygonID2PlaneIndex->second;
		ClearPlane( nPlaneIndex );
		return nPlaneIndex;
	}
	else
	{
		const int nPlaneIndex = freePlaneIndexCollector.LockID();
		const int nStackIndex = nPlaneIndex / STACK_SIZE;
		if ( nStackIndex >= nStackCount )
		{
			AddStack();
		}
		polygonID2PlaneIndexMap[nPolygonID] = nPlaneIndex;
		filledBitsList[nStackIndex] |= ( 1 << ( nPlaneIndex - ( nStackIndex * STACK_SIZE ) ) );
		return nPlaneIndex;
	}
}


void CHeightContainer::ErasePlane( int nPolygonID )
{
	hash_map<int, int>::iterator posPolygonID2PlaneIndex = polygonID2PlaneIndexMap.find( nPolygonID );
	if ( posPolygonID2PlaneIndex != polygonID2PlaneIndexMap.end() )
	{
		const int nPlaneIndex = posPolygonID2PlaneIndex->second;
		const int nStackIndex = nPlaneIndex / STACK_SIZE;
		ClearPlane( nPlaneIndex );
		filledBitsList[nStackIndex] &= ~( 1 << ( nPlaneIndex - ( nStackIndex * STACK_SIZE ) ) );
		if ( filledBitsList[nStackCount - 1] == 0 )
		{
			EraseStack();
		}
		freePlaneIndexCollector.FreeID( nPlaneIndex );
		polygonID2PlaneIndexMap.erase( posPolygonID2PlaneIndex );
	}
}


void CHeightContainer::ClearPlane( int nPlaneIndex )
{
	const int nStackIndex = nPlaneIndex / STACK_SIZE;
	const DWORD dwMask =  ~( 1 << ( nPlaneIndex - ( nStackIndex * STACK_SIZE ) ) );
	for ( int y = 0; y < planeSize.y; ++y )
	{
		for ( int x = 0; x < planeSize.x; ++x )
		{
			blackPlaneStackList[nStackIndex][y][x] &= dwMask;
			redPlaneStackList[nStackIndex][y][x] &= dwMask;
		}
	}
}


//функционал изменяющий pLockArray на заданное значение
struct SModifyBitFunctional
{
	int nBit;
	CArray2D<DWORD> *pPlaneArray;

	SModifyBitFunctional( const int _nBit, CArray2D<DWORD> *_pPlaneArray )
		: nBit( _nBit ), pPlaneArray( _pPlaneArray )
	{
	}
	bool operator()( int nXIndex, int nYIndex )
	{ 
		( *pPlaneArray )[nYIndex][nXIndex] |= ( 1 << nBit );
		return true;
	}
};


void CHeightContainer::FillPlane( int nPlaneIndex, const vector<CVec2> &rBlackPolygon, const vector<CVec2> &rRedPolygon )
{
	const int nStackIndex = nPlaneIndex / STACK_SIZE;
	const CTRect<int> rect( 0, 0, planeSize.x, planeSize.y );
	ApplyPointsInPolygon<SModifyBitFunctional, vector<CVec2>, CVec2>( rect, rBlackPolygon, fTileSize, SModifyBitFunctional( nPlaneIndex - ( nStackIndex * STACK_SIZE ), &( blackPlaneStackList[nStackIndex] ) ) );  
	ApplyPointsInPolygon<SModifyBitFunctional, vector<CVec2>, CVec2>( rect, rRedPolygon, fTileSize, SModifyBitFunctional( nPlaneIndex - ( nStackIndex * STACK_SIZE ), &( redPlaneStackList[nStackIndex] ) ) );  
}


void CHeightContainer::GetBits( vector<DWORD> *pBitsList, const int x, const int y )
{
	pBitsList->resize( nStackCount );
	for ( int nStackIndex = 0; nStackIndex < nStackCount; ++nStackIndex )
	{
		( *pBitsList )[nStackIndex] = blackPlaneStackList[nStackIndex][y][x];
	}
}


void CHeightContainer::AddBitsToString( string *pszMessage, const DWORD dwBits ) const
{
	for ( int nBit = 0; nBit < STACK_SIZE; ++nBit )
	{
		( *pszMessage ) += ( ( dwBits & ( 1 << nBit ) ) != 0 ) ? "1" : "0";
	}
}


void CHeightContainer::Clear()
{
	planeSize.x = 0;
	planeSize.y = 0;
	nStackCount = 0;
	blackPlaneStackList.clear();
	redPlaneStackList.clear();
	markedBitslList.clear();
	filledBitsList.clear();
	freePlaneIndexCollector.Clear();
	polygonID2PlaneIndexMap.clear();
	//
	bTraceToImage = false;
}


void CHeightContainer::SetSize( const int x, const int y, bool _bTraceToImage )
{
	Clear();
	//
	planeSize.x = x;
	planeSize.y = y;
	//
	bTraceToImage = _bTraceToImage;
}


void CHeightContainer::Mark( const int x, const int y )
{
	CTPoint<int> point( Clamp( x, 0, planeSize.x - 1 ), Clamp( y, 0, planeSize.y - 1 ) ); 
	GetBits( &markedBitslList, point.x, point.y );
	/**
	string szMessage = StrFmt( "CHeightContainer::Mark( %d, %d ), marked Bits: ", x, y );
	for ( int nStackIndex = 0; nStackIndex < nStackCount; ++nStackIndex )
	{
		if ( nStackIndex != 0 )
		{
			szMessage += "|";
		}
		AddBitsToString( &szMessage, markedBitslList[nStackIndex] );
	}
	DebugTrace( "%s", szMessage.c_str() );
	/**/
}


bool CHeightContainer::Compare( const int x, const int y )
{
	CTPoint<int> point( Clamp( x, 0, planeSize.x - 1 ), Clamp( y, 0, planeSize.y - 1 ) ); 
	vector<DWORD> bitsList;
	GetBits( &bitsList, point.x, point.y );
	for ( int nStackIndex = 0; nStackIndex < nStackCount; ++nStackIndex )
	{
		if ( markedBitslList[nStackIndex] != bitsList[nStackIndex] )
		{
			return false;
		}
	}
	return true;
}


void CHeightContainer::InsertPolygon( const vector<CVec2> &rBlackPolygon, const vector<CVec2> &rRedPolygon, int nPolygonID )
{
	if ( ( planeSize.x == 0 ) || ( planeSize.y == 0 ) )
	{
		return;
	}
	//
	const int nPlaneIndex = AddPlane( nPolygonID );
	FillPlane( nPlaneIndex, rBlackPolygon, rRedPolygon );
	//
	/**
	if ( bTraceToImage )
	{
		CArray2D<DWORD> traceImage;
		traceImage.SetSizes( ( planeSize.x - 1 ) * TRACE_IMAGE_TILE_SIZE + 1, ( planeSize.y - 1 ) * TRACE_IMAGE_TILE_SIZE + 1 );
		traceImage.FillEvery( 0xFFFFFFFF );
		//
		MarkTraceImageGrid( &traceImage, 0xFF000000 );
		//
		const int nStackIndex = nPlaneIndex / STACK_SIZE;
		const DWORD dwMask = ( 1 << ( nPlaneIndex - ( nStackIndex * STACK_SIZE ) ) );
		for ( int y = 0; y < planeSize.y; ++y )
		{
			for ( int x = 0; x < planeSize.x; ++x )
			{
				if ( ( blackPlaneStackList[nStackIndex][y][x] & dwMask ) && ( redPlaneStackList[nStackIndex][y][x] & dwMask ) )
				{
					MarkTraceImageTile( &traceImage, x, y, 0xFF00FF00 );
				}
				else if ( blackPlaneStackList[nStackIndex][y][x] & dwMask )
				{
					MarkTraceImageTile( &traceImage, x, y, 0xFF000000 );
				}
				else if ( redPlaneStackList[nStackIndex][y][x] & dwMask )
				{
					MarkTraceImageTile( &traceImage, x, y, 0xFFFF0000 );
				}
			}
		}
		{
			const int nPointCount = rRedPolygon.size();
			for ( int nPointIndex = 0; nPointIndex < nPointCount; ++nPointIndex )
			{
				const int nPointIndex2 = ( nPointIndex == ( nPointCount - 1 ) ) ? 0 : nPointIndex + 1;
				CTPoint<int> start( rRedPolygon[nPointIndex].x * TRACE_IMAGE_TILE_SIZE / fTileSize, rRedPolygon[nPointIndex].y * TRACE_IMAGE_TILE_SIZE / fTileSize );
				CTPoint<int> finish( rRedPolygon[nPointIndex2].x * TRACE_IMAGE_TILE_SIZE / fTileSize, rRedPolygon[nPointIndex2].y * TRACE_IMAGE_TILE_SIZE / fTileSize );
				MakeLine2( start.x, start.y, finish.x, finish.y, STraceImageFunctor( 0xFFFF0000, &traceImage ) );
			}
		}
		{
			const int nPointCount = rBlackPolygon.size();
			for ( int nPointIndex = 0; nPointIndex < nPointCount; ++nPointIndex )
			{
				const int nPointIndex2 = ( nPointIndex == ( nPointCount - 1 ) ) ? 0 : nPointIndex + 1;
				CTPoint<int> start( rBlackPolygon[nPointIndex].x * TRACE_IMAGE_TILE_SIZE / fTileSize, rBlackPolygon[nPointIndex].y * TRACE_IMAGE_TILE_SIZE / fTileSize );
				CTPoint<int> finish( rBlackPolygon[nPointIndex2].x * TRACE_IMAGE_TILE_SIZE / fTileSize, rBlackPolygon[nPointIndex2].y * TRACE_IMAGE_TILE_SIZE / fTileSize );
				MakeLine2( start.x, start.y, finish.x, finish.y, STraceImageFunctor( 0xFF0000FF, &traceImage ) );
			}
		}
		const string szFileName = StrFmt( "C:\\B2\\Editor\\HeighContainer\\bigPlane%03d.tga", nPlaneIndex );
		CFileStream imageStream( szFileName, CFileStream::WIN_CREATE );
		NImage::FlipY( traceImage );
		NImage::SaveAsTGA( traceImage, &imageStream );
		NImage::FlipY( traceImage );
		DebugTrace( "%s created", szFileName.c_str() );
	}
	/**/
}


void CHeightContainer::ErasePolygon( int nPolygonID )
{
	if ( nStackCount == 0 )
	{
		return;	
	}
	if ( ( planeSize.x == 0 ) || ( planeSize.y == 0 ) )
	{
		return;
	}
	ErasePlane( nPolygonID );
}


bool CHeightContainer::GetBlackRedBallance( const CTRect<int> &rRect )
{
	if ( nStackCount == 0 )
	{
		return false;
	}
	if ( ( planeSize.x == 0 ) || ( planeSize.y == 0 ) )
	{
		return false;
	}
	CTRect<int> rect( rRect );	
	ValidateRect( CTRect<int>( 0, 0, planeSize.x, planeSize.y ), &rect );
	//
	vector<DWORD> blackBitsList;
	vector<DWORD> redBitsList;
	blackBitsList.resize( nStackCount, 0 );
	redBitsList.resize( nStackCount, 0 );
	for ( int y = rect.miny; y < rect.maxy; ++y )
	{
		for ( int x = rect.minx; x < rect.maxx; ++x )
		{
			for ( int nStackIndex = 0; nStackIndex < nStackCount; ++nStackIndex )
			{
				blackBitsList[nStackIndex] |= blackPlaneStackList[nStackIndex][y][x];
				redBitsList[nStackIndex] |= redPlaneStackList[nStackIndex][y][x];
				if ( ( blackBitsList[nStackIndex] & redBitsList[nStackIndex] ) > 0 )
				{
					return true;
				}
			}
		}
	}
	return false;
}


void CHeightContainer::InsertVSO( const NDb::SVSOInstance &rVSO )
{
	if ( const NDb::SCragDesc *pCragDesc = checked_cast<const NDb::SCragDesc*>( &( *( rVSO.pDescriptor ) ) ) )
	{
		vector<CVec2> blackPolygon;
		vector<CVec2> redPolygon;
		CVSOManager::GetCragBoundingPolygon( &blackPolygon, rVSO.points, pCragDesc->bLeftSided ? CVSOManager::PT_OPNORMALE : CVSOManager::PT_NORMALE, 1.0f, rVSO.nVSOID );
		CVSOManager::GetCragBoundingPolygon( &redPolygon, rVSO.points, pCragDesc->bLeftSided ? CVSOManager::PT_NORMALE : CVSOManager::PT_OPNORMALE, 1.0f, rVSO.nVSOID );
		InsertPolygon( blackPolygon, redPolygon, rVSO.nVSOID );
	}
}


void CHeightContainer::Trace()
{
	DebugTrace( "CHeightContainer, begin" );
	DebugTrace( "STACK_SIZE: %d", STACK_SIZE );
	DebugTrace( "plane size: (%d, %d)", planeSize.x, planeSize.y );
	DebugTrace( "stack count: %d", nStackCount );
	DebugTrace( "tile size: %g", fTileSize );
	string szMessage = "marked Bits: ";
	for ( int nStackIndex = 0; nStackIndex < nStackCount; ++nStackIndex )
	{
		if ( nStackIndex != 0 )
		{
			szMessage += "|";
		}
		AddBitsToString( &szMessage, markedBitslList[nStackIndex] );
	}
	DebugTrace( "%s", szMessage.c_str() );
	szMessage = "filled Bits: ";
	for ( int nStackIndex = 0; nStackIndex < nStackCount; ++nStackIndex )
	{
		if ( nStackIndex != 0 )
		{
			szMessage += "|";
		}
		AddBitsToString( &szMessage, filledBitsList[nStackIndex] );
	}
	DebugTrace( "%s", szMessage.c_str() );
	freePlaneIndexCollector.Trace();
	DebugTrace( "polygonID2PlaneIndexMap [PolygonID] = planeIndex, begin" );
	for ( hash_map<int, int>::const_iterator posPolygonID2PlaneIndex = polygonID2PlaneIndexMap.begin(); posPolygonID2PlaneIndex != polygonID2PlaneIndexMap.end(); ++posPolygonID2PlaneIndex )
	{
		DebugTrace( "[%d] = %d", posPolygonID2PlaneIndex->first, posPolygonID2PlaneIndex->second );
	}
	DebugTrace( "polygonID2PlaneIndexMap, end" );

	/**
	for ( int nStackIndex = 0; nStackIndex < nStackCount; ++nStackIndex )
	{
		for ( int nBit = 0; nBit < STACK_SIZE; ++nBit )
		{
			if ( filledBitsList[nStackIndex] & ( 1 << nBit ) )
			{
				CArray2D<DWORD> blackPlane( planeSize.y, planeSize.x );
				CArray2D<DWORD> redPlane( planeSize.y, planeSize.x );
				for ( int y = 0; y < planeSize.y; ++y )
				{
					for ( int x = 0; x < planeSize.x; ++x )
					{
						blackPlane[y][x] = ( blackPlaneStackList[nStackIndex][y][x] & ( 1 << nBit ) ) ? 0xFF000000 : 0xFFFFFFFF;
						redPlane[y][x] = ( redPlaneStackList[nStackIndex][y][x] & ( 1 << nBit ) ) ? 0xFF000000 : 0xFFFFFFFF;
					}
				}
				{
					const string szFileName = StrFmt( "C:\\B2\\Editor\\HeighContainer\\blackPlane%03d.tga", nStackIndex * STACK_SIZE + nBit );
					CFileStream imageStream( szFileName, CFileStream::WIN_CREATE );
					NImage::FlipY( blackPlane );
					NImage::SaveAsTGA( blackPlane, &imageStream );
					DebugTrace( "%s created", szFileName.c_str() );
				}
				{
					const string szFileName = StrFmt( "C:\\B2\\Editor\\HeighContainer\\redPlane%03d.tga", nStackIndex * STACK_SIZE + nBit );
					CFileStream imageStream( szFileName, CFileStream::WIN_CREATE );
					NImage::FlipY( redPlane );
					NImage::SaveAsTGA( redPlane, &imageStream );
					DebugTrace( "%s created", szFileName.c_str() );
				}
			}
		}
	}
	/**/
	DebugTrace( "CHeightContainer, end" );
}

// basement storage  


