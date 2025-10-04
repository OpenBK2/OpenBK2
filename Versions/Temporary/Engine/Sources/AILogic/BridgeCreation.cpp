#include "stdafx.h"
#include "./bridgecreation.h"
#include "Bridge.h"
#include "CommonUnit.h"
#include "Common_RTS_AI/PathFinder.h"

REGISTER_SAVELOAD_CLASS( 0x1508D49E, CBridgeCreation );

//*******************************************************************
//*												  CBridgeCreation*
//*******************************************************************

bool CBridgeCreation::SBridgeSpanSort::operator()( const CObj<CBridgeSpan> &s1, const CObj<CBridgeSpan> &s2 )
{
	const CVec2 v1 = CVec2(s1->GetCenter().x,s1->GetCenter().y);
	const CVec2 v2 = CVec2(s2->GetCenter().x,s2->GetCenter().y);

	if ( s1->GetBridgeStats()->edirection == NDb::SBridgeRPGStats::VERTICAL )
		return v1.y > v2.y;
	else
		return v1.x > v2.x;
}

CVec2 CBridgeCreation::SortBridgeSpans( vector< CObj<CBridgeSpan> > *spans, class CCommonUnit *pUnit )
{
	if ( 1 >= spans->size() ) return VNULL2;

	SBridgeSpanSort pr;
	sort( spans->begin(), spans->end(), pr );
	// проверить, к какому идти меньше - к первому или к последнему
	const CBridgeSpan *s1 = *spans->begin();
	const CBridgeSpan *s2 = (*spans)[spans->size()-1];

	CVec2 vFrom1to2( CVec2(s2->GetCenter().x,s2->GetCenter().y) - CVec2(s1->GetCenter().x,s1->GetCenter().y) );
	Normalize( &vFrom1to2 );
	// найти точки, близко к s1 и s2, лежащие вне моста
	SRect r1, r2;
	s1->GetBoundRect( &r1 );
	s2->GetBoundRect( &r2 );
	const CVec2 v1( r1.center - Max(r1.lengthAhead,Max(r1.lengthBack,r1.width))*vFrom1to2 );
	const CVec2 v2( r2.center + Max(r2.lengthAhead,Max(r2.lengthBack,r2.width))*vFrom1to2 );


	CPtr<IStaticPath> pPath1 = CreateStaticPathToPoint( v1, VNULL2, pUnit, true, GetAIMap() );
	CPtr<IStaticPath> pPath2 = CreateStaticPathToPoint( v2, VNULL2, pUnit, true, GetAIMap() );

	const float fDiff1 = pPath1 ? fabs2( pPath1->GetFinishPoint() - v1 ) : 1000000;
	const float fDiff2 = pPath2 ? fabs2( pPath2->GetFinishPoint() - v2 ) : 1000000;

	if (	pPath1->GetLength() <= pPath2->GetLength() && fDiff1 < sqr( SConsts::TILE_SIZE * 10 ) )
	{
		// первый span - ближайший.
		return v1;
	}
	else if ( pPath1->GetLength() >= pPath2->GetLength() && fDiff2 < sqr( SConsts::TILE_SIZE * 10 ) )
	{
		// последний span - ближайший.
		reverse( spans->begin(), spans->end() );
		return v2;
	}
	else if ( fDiff1 < sqr( SConsts::TILE_SIZE * 10 ) ) // по первому пути хоть дойти можно
	{
		// первый span - ближайший.
		return v1;
	}
	else if ( fDiff2 < sqr( SConsts::TILE_SIZE * 10 ) )
	{
		reverse( spans->begin(), spans->end() );
		return v2;
	}
	else 
		return VNULL2;
}

bool CBridgeCreation::IsFirstSegmentBuilt() const
{
	return spans[0]->GetHitPoints() >= 0;
}

CBridgeCreation::CBridgeCreation( class CFullBridge *pBridge, class CCommonUnit *pUnit, const bool bAllowAIModification )
: pFullBridge( pBridge ),
CLongObjectCreation( pUnit->GetPlayer(), bAllowAIModification )
{
	// просто отсортировать по координатам все участки
	pBridge->EnumSpans( &spans );
	NI_ASSERT( spans.size() >= 2, "bridge witout at least 2 spans" );
	// посчитать длину пути до 2 крайних точек, выбрать к какой ехать.
	vStartPoint = SortBridgeSpans( &spans, pUnit );

	// разделить на уже построенные и еще не построенные.
	for ( nCurIndex = 0; nCurIndex < spans.size(); ++nCurIndex )
	{
		if ( spans[nCurIndex]->GetHitPoints() < 0.0f )
			break;
	}
	line = CLine2( vStartPoint, CVec2(spans[spans.size()-1]->GetCenter().x,spans[spans.size()-1]->GetCenter().y) );

	SRect r1;
	SRect r0;
	spans[1]->GetBoundRect( &r1 );
	spans[0]->GetBoundRect( &r0 );
	wDir = GetDirectionByVector( r1.center - r0.center );
}

const CVec2 & CBridgeCreation::GetStartPoint() const
{
	return vStartPoint;
}

CLine2 CBridgeCreation::GetCurLine() 
{ 
	return line; 
}

const int CBridgeCreation::GetMaxIndex() const
{
	return spans.size();
}

const int CBridgeCreation::GetCurIndex() const
{
	return nCurIndex;
}

const CVec2 CBridgeCreation::GetNextPoint( const int nPlace, const int nMaxPlace ) const
{
	SRect rect;
	spans[nCurIndex]->GetBoundRect( &rect );

	CVec2 vertexes[2];
	// нужно подойти к краю объекта.
	int nVertIndex = 0;
	for ( int i = 0; i < 4; ++i )
	{
		if ( DirsDifference( 65535/2+wDir, GetDirectionByVector( rect.v[i] - rect.center) ) < 65535/4 )
		{
			NI_ASSERT( nVertIndex < 2, "nakosyachil" );
			vertexes[nVertIndex] = rect.v[i];
			++nVertIndex;
		}
	}
	NI_ASSERT( nVertIndex == 2, "nakosyachil" );


	const CVec2 vOffset( (vertexes[0] - vertexes[1])/2 );
	return (vertexes[0] + vertexes[1])/2 + vOffset * ( 1.0f * nPlace / nMaxPlace ) - SConsts::TILE_SIZE * GetVectorByDirection( wDir );
}

void CBridgeCreation::BuildNext()
{
	CLongObjectCreation::BuildNext();
	if ( GetCurIndex() != 0 )
	{
		pFullBridge->UnlockSpan( spans[nCurIndex] );
	}
	// перевести сегмент в достроенное состояние
	spans[nCurIndex]->Build();
	const SHPObjectRPGStats * pStats = spans[nCurIndex]->GetStats();
	spans[nCurIndex]->SetHitPoints( pStats->fMaxHP );
	++nCurIndex;


	if ( GetCurIndex() < GetMaxIndex() )
	{
		// проверить, если следуюший сегмент разрушен полностью, то
		// сделать его полуразрушенным
		const float fNextSpanHP = spans[nCurIndex]->GetHitPoints();
		if ( 0 ==  fNextSpanHP )
		{
			spans[nCurIndex]->SetHitPoints( spans[nCurIndex]->GetStats()->fMaxHP * 0.1 );
			spans.clear(); // строительство закончено
			pFullBridge->UnlockAllSpans();
		}
		else if ( 0 < fNextSpanHP )
		{
			spans.clear(); // строительство закончено
			pFullBridge->UnlockAllSpans();
		}
		else
		{
			// залокать следующий сегмент
			pFullBridge->LockSpan( spans[nCurIndex], wDir );
		}
	}
	else
	{
		pFullBridge->UnlockAllSpans();
	}
}

float CBridgeCreation::GetPrice()
{
	const SHPObjectRPGStats *pStats = spans[nCurIndex]->GetStats();
	return pStats->fMaxHP * pStats->fRepairCost * SConsts::REPAIR_COST_ADJUST/ pFullBridge->GetNSpans();
}


