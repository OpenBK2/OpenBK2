#pragma once
#include "3DLib_export.h"


class _3DLIB_EXPORT CMemObject: public CObjectBase
{
	OBJECT_BASIC_METHODS(CMemObject);
	ZDATA
	std::vector<CVec3> resPoints;
	std::vector<CVec3> resNormals;
	std::vector<STriangle> resTris;
	bool bPolyLine;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&resPoints); f.Add(3,&resNormals); f.Add(4,&resTris); f.Add(5,&bPolyLine); return 0; }
public:
	CMemObject(): bPolyLine(false) {}
	void Clear();
	void CreateCube( const CVec3 &base, const CVec3 &size, bool bTwoSided = false );
	void CreateSphere( const CVec3 &ptCenter, float fRadius, int nSubs = 2 );
	void CreateCylinder( const CVec3 &ptStart, const CVec3 &ptEnd, float fRadius, int nSubs = 2, bool bClose = false );
	void CreatePolyline( const std::vector<CVec3> &points );
	void CreatePolygone( const std::vector<CVec2> &points, float fZ );
	void CreateIsoscelesColumn( const CVec3 &ptBase, float fHeight, float fBase );
	void Create( const std::vector<CVec3> &points, const std::vector<STriangle> &tris );
	void CalcBound( SSphere *pRes ) const;
	void CalcBound( SBound *pRes ) const;
	bool IsPolyLine() const { return bPolyLine; }
	const std::vector<CVec3>& GetPoints() const { return resPoints; }
	const std::vector<CVec3>& GetNormals() const { return resNormals; }
	const std::vector<STriangle>& GetTris() const { return resTris; }
	friend class CMemObjectBuilder;
};


