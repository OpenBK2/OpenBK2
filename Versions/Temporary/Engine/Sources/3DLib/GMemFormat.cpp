#include "stdafx.h"
#include "GMemFormat.h"
#include "GGeometry.h"

namespace NGScene
{

template<class T>
static void CalcDU( T *pRes, const CVec3 &vNormal )
{
	NGfx::CalcCompactVector( &pRes->normal, vNormal );
	CVec3 vTexU = vNormal ^ CVec3(0.3f,0.3f,0.3f);
	if ( fabs2( vTexU ) < 0.01f )
		vTexU = vNormal ^ CVec3(0,1,0);
	Normalize( &vTexU );
	CVec3 vTexV = vTexU ^ vNormal;
	NGfx::CalcCompactVector( &pRes->texU, vTexU );
	NGfx::CalcCompactVector( &pRes->texV, vTexV );
}

// CMemObjectInfo

CMemObjectInfo::CMemObjectInfo( const std::vector<STriangle> &_tris, const std::vector<CVec3> &_points,
	const std::vector<CVec3> &_normals ) : tris(_tris), points(_points), normals(_normals)
{
}

void CMemObjectInfo::Recalc()
{
	pValue = new CObjectInfo;
	CObjectInfo::SData resData;
	resData.verts.resize( points.size() );
	for ( int i = 0; i < points.size(); ++i )
		resData.verts[i].pos = points[i];
	for ( int i = 0; i < normals.size(); ++i )
		CalcDU( &resData.verts[i], normals[i] );
	//
	resData.geometry = tris;
	pValue->Assign( &resData, false );
}

} // namespace
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x02911172, CMemObjectInfo )

