#pragma once
#include "3DLib/GGeometry.h"

namespace NMeshData
{

struct SMeshData
{
  SMeshData() {}
  SMeshData( const int nVertsReserve, const int nTrgsReserve )
  {
    vertices.reserve( nVertsReserve );
    triangles.reserve( nTrgsReserve );
  }
  //
	vector<NGScene::SVertex> vertices;
	vector<STriangle> triangles;
  vector<NGScene::CObjectInfo::SStream> attributes;
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &vertices );
		saver.Add( 2, &triangles );
    saver.Add( 3, &attributes );
		return 0;
	}
	void Clear() { vertices.clear(); attributes.clear(); triangles.clear(); }
};

struct SMeshDataTex2
{
  SMeshDataTex2() {}
  SMeshDataTex2( const int nVertsReserve, const int nTrgsReserve, const int nSecondTexReserve )
  {
    vertices.reserve( nVertsReserve );
    triangles.reserve( nTrgsReserve );
    secondTex.reserve( nSecondTexReserve );
  }
  //
  vector<NGScene::SVertex> vertices;
  vector<STriangle> triangles;
  vector<CVec2> secondTex;
  vector<NGScene::CObjectInfo::SStream> attributes;
  //
  int operator&( IBinSaver &saver )
  {
    saver.Add( 1, &vertices );
    saver.Add( 2, &triangles );
    saver.Add( 3, &secondTex );
    saver.Add( 4, &attributes );
    return 0;
  }
	void Clear() { vertices.clear(); triangles.clear(); secondTex.clear(); attributes.clear(); }
};

template <class T>
struct SPatchHolder
{
	CObj<T> pPatch;
	CObj<CObjectBase> pHolder;
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &pPatch );
		saver.Add( 2, &pHolder );
		return 0;
	}
};

}


