#ifndef __AIGEOMETRYFORMAT_H_
#define __AIGEOMETRYFORMAT_H_

#include "aiObjectLoader.h"

struct SStoredPiece
{
	vector<STriangle> tris;
	vector<CVec3> verts;		
	vector<NGScene::SLoadVertexWeight> weights;
	float fVolume;
	vector<NAI::SJunction> juncs;
	//vector<CPtr<NAI::CPrecalcSpheres> > precalc;

	int operator&( CStructureSaver &f )
	{ 
		f.Add( 1, &verts );
		f.Add( 2, &tris );
		f.Add( 3, &weights );
		f.Add( 10, &fVolume );
		f.Add( 11, &juncs );
//		f.Add( 13, &precalc );
		return 0;
	}
};

typedef hash_map<int, SStoredPiece > CStoredPieceMap;
const int N_PIECES_CHUNK = 4;

#endif // __AIGEOMETRYFORMAT_H_
