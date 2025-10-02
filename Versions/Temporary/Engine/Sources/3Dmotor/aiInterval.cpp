#include "StdAfx.h"
#include "aiInterval.h"
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
	vector<T> *pEnter, 
	vector<T> *pExit, 
	bool bTerrain,
	TFunc make, TCompare compare )
{
	vector<T> &enter = *pEnter;
	vector<T> &exit = *pExit;
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

void FillIntersectionResults( vector<SInterval> *pRes,
	vector<SInterval::SCrossPoint> *pEnter,
	vector<SInterval::SCrossPoint> *pExit,
	const SSourceInfo &_src, int _nUserID, bool bTerrain )
{
	vector<SInterval::SCrossPoint> &enter = *pEnter;
	vector<SInterval::SCrossPoint> &exit = *pExit;
	sort( enter.begin(), enter.end(), CmpCrosses );
	sort( exit.begin(), exit.end(), CmpCrosses );
	CalcResult( &enter, &exit, bTerrain, MakeCross, CmpCrosses );
	for ( int i = 0; i < Min( enter.size(), exit.size() ); ++i )
	{
		// due to cheating with degenerate cases and computation errors this might happen
		//ASSERT( enter[i].fT <= exit[i].fT );
		if ( enter[i].fT > exit[i].fT )
			OutputDebugString( "AI tracing, something went wrong\n" );
		exit[i].fT = Max( enter[i].fT, exit[i].fT ); // this is it Beavis, correct wrong results so it seams less buggy
		pRes->push_back( SInterval( _src, _nUserID, enter[i], exit[i] ) );
	}
}	

void FillIntersectionResults( vector<SSimpleInterval> *pRes,
	vector<float> *pEnter, 
	vector<float> *pExit,
	const SSourceInfo &_src, int _nUserID, bool bTerrain )
{
	vector<float> &enter = *pEnter;
	vector<float> &exit = *pExit;
	sort( enter.begin(), enter.end() );
	sort( exit.begin(), exit.end() );
	CalcResult( &enter, &exit, bTerrain, MakeFloat, CmpFloat );
	for ( int i = 0; i < Min( enter.size(), exit.size() ); ++i )
	{
		exit[i] = Max( enter[i], exit[i] ); // this is it Beavis, correct wrong results so it seams less buggy
		pRes->push_back( SSimpleInterval( _src, _nUserID, enter[i], exit[i] ) );
	}
}

static bool CompareSimpleIntervals( const SSimpleInterval &i1, const SSimpleInterval &i2 )
{
	return i1.fEnter < i2.fEnter;
}

void SortSimpleIntervals( vector<SSimpleInterval> *pRes )
{
	sort( pRes->begin(), pRes->end(), CompareSimpleIntervals );
}

static bool CompareIntervals( const SInterval &a, const SInterval &b )
{
	return a.enter.fT < b.enter.fT;
}

void SortIntervals( vector<SInterval> *pRes )
{
	sort( pRes->begin(), pRes->end(), CompareIntervals );
}

}


