#pragma once

#include "aiObjectLoader.h"

struct SStoredPiece
{
	std::vector<STriangle> tris;
	std::vector<CVec3> verts;
	std::vector<NGScene::SLoadVertexWeight> weights;
	float fVolume;
	std::vector<NAI::SJunction> juncs;
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

typedef std::unordered_map<int, SStoredPiece > CStoredPieceMap;
const int N_PIECES_CHUNK = 4;


