#include "StdAfx.h"
#include "GMesh.h"
#include "..\3Dlib\Bound.h"


namespace NGScene
{

// CMeshBound

void CMeshBound::Recalc()
{
	const SSkeletonMatrices &anim = pAnimation->GetValue();
	vector<CVec3> pts;
	pts.resize( anim.size() );
	for ( int i=0; i<anim.size(); ++i )
		pts[i] = anim[i].GetTranslation();
	CalcBound( &value, pts, SGetSelf<CVec3>() );
	value.Extend( 0.5f );
}

int CMeshBound::operator&( CStructureSaver &f )
{
	f.Add( 1, &pAnimation );
	return 0;
}

} // namespace
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x12061180, CMeshBound )
