#include "stdafx.h"
#include "aiInterval.h"

#include <algorithm>

namespace NAI
{

static bool CmpCrosses( const SInterval::SCrossPoint &a, const SInterval::SCrossPoint &b )
{
	return a.fT < b.fT;
}
static bool CmpFloat( const float &a, const float &b )
{
	return a < b;
}
template<class T, class TFunc, class TCompare>
void CalcResult( 
	std::vector<T> *pEnter,
	std::vector<T> *pExit,
	bool bTerrain,
	TFunc make, TCompare compare )
{
	std::vector<T> &enter = *pEnter;
	std::vector<T> &exit = *pExit;
	if ( !bTerrain )
	{
		if ( enter.size() != exit.size() )
		{
			//ASSERT( 0 );
			OutputDebugString( "non closed AI model encountered\n" );
		}
	}
	else
	{
		if ( enter.size() < exit.size() || (enter.size() && enter.size() == exit.size() && compare(exit[0], enter[0])) )
		{
			enter.resize( enter.size() + 1 );
			for ( int i = enter.size() - 1; i > 0; --i )
				enter[i] = enter[i-1];
			enter[0] = make( -1e10f );
		}
		if ( enter.size() > exit.size() )
			exit.push_back( make( 1e10f ) );
		if ( enter.size() != exit.size() )
		{
			//ASSERT( 0 );
			OutputDebugString( "trace does not support fragmented non closed models\n" );
		}
	}
}
static SInterval::SCrossPoint MakeCross( float f ) { return SInterval::SCrossPoint( f, CVec3(0,0,1) ); }
static float MakeFloat( float f ) { return f; }

void FillIntersectionResults( std::vector<SInterval> *pRes,
	std::vector<SInterval::SCrossPoint> *pEnter,
	std::vector<SInterval::SCrossPoint> *pExit,
	const SSourceInfo &_src, int _nUserID, bool bTerrain )
{
	std::vector<SInterval::SCrossPoint> &enter = *pEnter;
	std::vector<SInterval::SCrossPoint> &exit = *pExit;
	std::sort( enter.begin(), enter.end(), CmpCrosses );
	std::sort( exit.begin(), exit.end(), CmpCrosses );
	CalcResult( &enter, &exit, bTerrain, MakeCross, CmpCrosses );
	for ( int i = 0; i < (std::min)( enter.size(), exit.size() ); ++i )
	{
		// due to cheating with degenerate cases and computation errors this might happen
		//ASSERT( enter[i].fT <= exit[i].fT );
		if ( enter[i].fT > exit[i].fT )
			OutputDebugString( "AI tracing, something went wrong\n" );
		exit[i].fT = (std::max)( enter[i].fT, exit[i].fT ); // this is it Beavis, correct wrong results so it seams less buggy
		pRes->push_back( SInterval( _src, _nUserID, enter[i], exit[i] ) );
	}
}	

void FillIntersectionResults( std::vector<SSimpleInterval> *pRes,
	std::vector<float> *pEnter,
	std::vector<float> *pExit,
	const SSourceInfo &_src, int _nUserID, bool bTerrain )
{
	std::vector<float> &enter = *pEnter;
	std::vector<float> &exit = *pExit;
	std::sort( enter.begin(), enter.end() );
	std::sort( exit.begin(), exit.end() );
	CalcResult( &enter, &exit, bTerrain, MakeFloat, CmpFloat );
	for ( int i = 0; i < (std::min)( enter.size(), exit.size() ); ++i )
	{
		exit[i] = (std::max)( enter[i], exit[i] ); // this is it Beavis, correct wrong results so it seams less buggy
		pRes->push_back( SSimpleInterval( _src, _nUserID, enter[i], exit[i] ) );
	}
}

static bool CompareSimpleIntervals( const SSimpleInterval &i1, const SSimpleInterval &i2 )
{
	return i1.fEnter < i2.fEnter;
}

void SortSimpleIntervals( std::vector<SSimpleInterval> *pRes )
{
	std::sort( pRes->begin(), pRes->end(), CompareSimpleIntervals );
}

static bool CompareIntervals( const SInterval &a, const SInterval &b )
{
	return a.enter.fT < b.enter.fT;
}

void SortIntervals( std::vector<SInterval> *pRes )
{
	std::sort( pRes->begin(), pRes->end(), CompareIntervals );
}

}


