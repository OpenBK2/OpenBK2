#pragma once

#include "StaticObjects.h"

//
// Итераторы, не cэйвятся!
// 

extern CStaticObjects theStatObjs;

template<bool bOnlyContainers>
class CStObjIter
{
	int y, x;
	int downX, downY, upX, upY;

	CStaticObjects::StaticObjectsAreaMap::CDataList::iterator iter;

	//
	void NextXY();
	void IterateToNextCell();

	CStObjIter( const CStObjIter &iter );
	CStObjIter& operator=( const CStObjIter &iter );
protected:
	CStaticObjects::StaticObjectsAreaMap& GetAreaMap();
	int GetCellSize() const;
public:
	CStObjIter() : x( 0 ), y( 0 ), downX( 0 ), downY( 0 ), upX( 0 ), upY( 0 ) { }
	CStObjIter( const int downX, const int upX, const int downY, const int upY );
	~CStObjIter() { theStatObjs.SetIterCreated( false ); }

	void Iterate();
	bool IsFinished() const;
	void Reset() 
	{
		theStatObjs.SetIterCreated( false );
		Init( downX, upX, downY, upY );
	}
	void Init( const int _downX, const int _upX, const int _downY, const int _upY );

	class CExistingObject* operator*();
};

// очень медленный!
template<bool bOnlyContainers>
class CStObjGlobalIter : public CStObjIter<bOnlyContainers>
{
	CStObjGlobalIter( const CStObjGlobalIter &iter );
	CStObjGlobalIter& operator=( const CStObjGlobalIter &iter );
public:
	CStObjGlobalIter();
};

template<bool bOnlyContainers>
class CStObjCircleIter : public CStObjIter<bOnlyContainers>
{
	CStObjCircleIter( const CStObjCircleIter &iter );
	CStObjCircleIter& operator=( const CStObjCircleIter &iter );
public:
	CStObjCircleIter( const CVec2 &vCenter, const float fR );
};

// итератор по минам. ходит только по минам, которые находятся
// в заданном радиусе от центра и невидимы данному игроку.
// если bAllMines, то берутся все мины, в не зависимости от игрока и видимости
//CRAP {
// использует стандартный итератор, выбирая из него мины. может быть очень медленным.
//CRAP }
class CMinesIter : protected CStObjCircleIter<false>
{
	int nParty;
	const bool bAllMines;
	
	void IterateToNextMine();
public:
	// итерировать по минам, невидимым nParty
	CMinesIter( const CVec2 &vCenter, float fR, const int nParty, const bool bAllMines = false );
	void Iterate();
	bool IsFinished() const;
	void Reset() ;
	class CMineStaticObject* operator->();
	class CMineStaticObject* operator*();
};

//
// implementation
//

//*******************************************************************
//*											CStObjIter																	*
//*******************************************************************

template<bool bOnlyContainers>
inline CStaticObjects::StaticObjectsAreaMap& CStObjIter<bOnlyContainers>::GetAreaMap()
{
	return theStatObjs.GetAreaMap();
}

template<>
inline CStaticObjects::StaticObjectsAreaMap& CStObjIter<true>::GetAreaMap()
{
	return theStatObjs.GetContainersAreaMap();
}

template<bool bOnlyContainers>
inline int CStObjIter<bOnlyContainers>::GetCellSize() const
{
	return SConsts::STATIC_OBJ_CELL;
}

template<>
inline int CStObjIter<true>::GetCellSize() const
{
	return SConsts::STATIC_CONTAINER_OBJ_CELL;
}

template<bool bOnlyContainers>
CStObjIter<bOnlyContainers>::CStObjIter( const int downX, const int upX, const int downY, const int upY )
{ 
	Init( downX, upX, downY, upY );
}

extern NTimer::STime curTime;

template<bool bOnlyContainers>
void CStObjIter<bOnlyContainers>::Init( const int _downX, const int _upX, const int _downY, const int _upY )
{
	downX = Clamp( _downX, 0, GetAreaMap().GetSizeX() - 1 );
	upX = Clamp( _upX, 0, GetAreaMap().GetSizeX() - 1 );
  downY = Clamp( _downY, 0, GetAreaMap().GetSizeY() - 1 );
	upY = Clamp( _upY, 0, GetAreaMap().GetSizeY() - 1 );

#if !defined(_FINALRELEASE) && !defined(_BETARELEASE)
	NI_ASSERT( !theStatObjs.IsIterCreated(), "Can't create two iterators concurrently" ); 
	theStatObjs.SetIterCreated( true );
#endif // #if !defined(_FINALRELEASE) && !defined(_BETARELEASE)

	CExistingObject::UpdateGlobalMark();

	y = downY; x = downX;
	if ( GetAreaMap().GetSizeX() > 0 && GetAreaMap().GetSizeY() > 0 && !GetAreaMap()[y][x].empty() )
		iter = GetAreaMap()[y][x].begin();
	else
		IterateToNextCell();

	static int nUpdate = 0;
	if ( !IsFinished() )
	{
		(*iter)->SetGlobalUpdated();
	}
}

template<bool bOnlyContainers>
void CStObjIter<bOnlyContainers>::IterateToNextCell()
{
	do
	{
		NextXY();
	} while ( !IsFinished() && GetAreaMap()[y][x].empty() );

	if ( !IsFinished() )
		iter = GetAreaMap()[y][x].begin();
}

template<bool bOnlyContainers>
void CStObjIter<bOnlyContainers>::NextXY()
{
	if ( ++x > upX )
	{
		++y;
		x = downX;
	}
}

template<bool bOnlyContainers>
bool CStObjIter<bOnlyContainers>::IsFinished() const
{
	return y > upY;
}

template<bool bOnlyContainers>
void CStObjIter<bOnlyContainers>::Iterate()
{ 
	do
	{
		if ( ++iter == GetAreaMap()[y][x].end() )
			IterateToNextCell();
	} while ( !IsFinished() && (*iter)->IsGlobalUpdated() );

	if ( !IsFinished() )
	{
		(*iter)->SetGlobalUpdated();
	}
}

template<bool bOnlyContainers>
CExistingObject* CStObjIter<bOnlyContainers>::operator*() 
{ 
	NI_ASSERT( !IsFinished(), "Can't perform operator *" ); 
	return *iter;
}

//*******************************************************************
//*											CStObjGlobalIter														*
//*******************************************************************

template<bool bOnlyContainers>
CStObjGlobalIter<bOnlyContainers>::CStObjGlobalIter()
: CStObjIter<bOnlyContainers>( 0, this->GetAreaMap().GetSizeX() - 1, 0, this->GetAreaMap().GetSizeY() - 1 )
{
}

//*******************************************************************
//*											CStObjCircleIter														*
//*******************************************************************

template<bool bOnlyContainers>
CStObjCircleIter<bOnlyContainers>::CStObjCircleIter( const CVec2 &vCenter, const float fR )
{
	const int nBigCellSize = SConsts::TILE_SIZE * this->GetCellSize();

	const int nMinX = (std::max)( 0, int( (vCenter.x - fR) / nBigCellSize ) );
	const int nMaxX = (std::min)( this->GetAreaMap().GetSizeX() - 1, int( (vCenter.x + fR) / nBigCellSize ) );
	const int nMinY = (std::max)( 0, int( ( vCenter.y - fR ) / nBigCellSize ) );
	const int nMaxY = (std::min)( this->GetAreaMap().GetSizeY() - 1, int( ( vCenter.y + fR ) / nBigCellSize ) );

	CStObjIter<bOnlyContainers>::Init( nMinX, nMaxX, nMinY, nMaxY );
}


