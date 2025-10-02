#pragma once
#include "..\System\DG.h"

namespace NGfx
{
	class CGeometry;
};

namespace NGScene
{
class CObjectInfo;

// special class for polylines
class CMemGeometry: public CPtrFuncBase<NGfx::CGeometry>
{
	OBJECT_BASIC_METHODS(CMemGeometry);
	ZDATA
	vector<CVec3> points;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&points); return 0; }
protected:
	virtual void Recalc();
public:
	CMemGeometry() {}
	CMemGeometry( const vector<CVec3> &points );
};
}

