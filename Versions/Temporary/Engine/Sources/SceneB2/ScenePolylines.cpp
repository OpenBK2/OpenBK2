#include "StdAfx.h"

#include "SceneInternal.h"

int CScene::AddPolyline( const int nID, const vector<CVec3> &_points, const CVec4 &vColor, bool bDepthCheck )
{
	// translate to Vis coords
	vector<CVec3> points( _points.size() );
	for ( int i = 0; i < _points.size(); ++i )
		AI2Vis( &(points[i]), _points[i] );

	//	vector<CVec3> points( _points );
	//
	vector<unsigned short> indices;
	indices.reserve( (points.size() - 1) * 2 );
	for ( int k = 0; k < points.size() - 1; ++k )
	{
		indices.push_back( k );
		indices.push_back( k + 1 );
	}
	return AddIndexedPolylineInternal( nID, points, indices, vColor, bDepthCheck );
}

int CScene::AddIndexedPolyline( const int nID, const vector<CVec3> &_points, const vector<WORD> &indices, const CVec4 &vColor, bool bDepthCheck )
{
	// translate to Vis coords
	vector<CVec3> points( _points.size() );
	for ( int i = 0; i < _points.size(); ++i )
		AI2Vis( &(points[i]), _points[i] );
	//
	return AddIndexedPolylineInternal( nID, points, indices, vColor, bDepthCheck );
}

int CScene::AddIndexedPolylineInternal( const int nID, const vector<CVec3> &points, const vector<WORD> &indices, const CVec4 &vColor, bool bDepthCheck )
{
	const int nLineID = GetID( nID );
	data[eScene]->polylines[nLineID] = data[eScene]->GetGScene()->CreatePolyline( points, indices, vColor, bDepthCheck );
	return nLineID;
}

void CScene::RemovePolyline( const int nID )
{
	data[eScene]->polylines.erase( nID );
}

