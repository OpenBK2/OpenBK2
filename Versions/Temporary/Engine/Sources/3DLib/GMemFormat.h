#pragma once
#include "System/DG.h"

namespace NGScene
{
class CObjectInfo;

class CMemObjectInfo: public CPtrFuncBase<CObjectInfo>
{
	OBJECT_BASIC_METHODS(CMemObjectInfo);
	ZDATA
	vector<CVec3> points, normals;
	vector<STriangle> tris;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&points); f.Add(3,&normals); f.Add(4,&tris); return 0; }
protected:
	virtual void Recalc();
public:
	CMemObjectInfo() {}
	CMemObjectInfo( const vector<STriangle> &tris, const vector<CVec3> &points, const vector<CVec3> &normals );
};

}

