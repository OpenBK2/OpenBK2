#pragma once
#include "../3Dlib/GGeometry.h"

namespace NGScene
{

struct SLoadVertexWeight
{
	float fWeight;
	int nVertex;
	int nBone;
};

inline void ConvertWeights( vector<SVertexWeight> *pRes, const vector<SLoadVertexWeight> &wghts, 
	int nVertices )
{
	vector<SVertexWeight> &r = *pRes;
	// convert weights
	r.resize( nVertices );
	memset( &r[0], 0, sizeof(SVertexWeight) * nVertices );
	
	int nMax = nVertices;
	int nCount = wghts.size();
	int i, nVertex = 0, j = 0;
	for ( i = 0; i < nCount && nVertex < nMax; ++i )
	{
		if ( wghts[i].nVertex != nVertex )
		{
			nVertex++;
			j = 0;
		}
		if ( j > 3 )
			continue;
		r[nVertex].fWeights[j] = wghts[i].fWeight;
		r[nVertex].cBoneIndices[j] = wghts[i].nBone;
		j++;
	}
}

} // namespace


