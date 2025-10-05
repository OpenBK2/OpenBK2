#pragma once
#include "System/Dg.h"

namespace NGScene
{
class CObjectInfo;

class CMemObjectInfo: public CPtrFuncBase<CObjectInfo>
{
	OBJECT_BASIC_METHODS(CMemObjectInfo);
	ZDATA
	std::vector<CVec3> points, normals;
	std::vector<STriangle> tris;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&points); f.Add(3,&normals); f.Add(4,&tris); return 0; }
protected:
	virtual void Recalc();
public:
	CMemObjectInfo() {}
	CMemObjectInfo( const std::vector<STriangle> &tris, const std::vector<CVec3> &points, const std::vector<CVec3> &normals );
};

}

