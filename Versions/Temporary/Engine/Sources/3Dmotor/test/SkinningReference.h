#pragma once
// Frozen copies of the position skinning code the engine ran when the kernels in
// GSkinning.h and test/SkinningVariants.h were measured: SSESkinning and
// SSEBatchTransform from GSSEtransform.h, glm generic code without
// GLM_FORCE_INTRINSICS. Copied rather than included so that the comparison keeps
// its baseline when GSSEtransform.h changes; the only edit is that the bone matrices
// come as a pointer instead of a std::vector, which changes nothing in the loop.
//
// Needs the engine's CVec3 and SHMatrix (Misc/Geom.h through stdafx.h) and glm, so
// unlike the variants header it does not compile on its own.

#include "3Dmotor/GSkinning.h"

#include <glm/glm.hpp>

namespace NSkinningReference
{

// GSSEtransform.h LoadMatrix: SHMatrix is row-major, glm column-major.
inline glm::mat4 LoadMatrix( const SHMatrix &m )
{
	return glm::mat4{
		m._11,
		m._21,
		m._31,
		m._41,
		m._12,
		m._22,
		m._32,
		m._42,
		m._13,
		m._23,
		m._33,
		m._43,
		m._14,
		m._24,
		m._34,
		m._44,
	};
}

// GSSEtransform.h SSESkinning.
inline void SSESkinning(
		const CVec3 *pSrc, CVec3 *pRes, const NSkinning::SSkinWeights *pWeight, const SHMatrix *pBlends, int nCount )
{
	for ( int k = 0; k < nCount; ++k, ++pSrc, ++pRes, ++pWeight )
	{
		glm::mat4 blended{ 0.0f };

		for ( int i = 0; i < 4; ++i )
		{
			float w = pWeight->fWeights[i];
			if ( w == 0.0f )
			{
				continue;
			}
			int boneIndex = pWeight->cBoneIndices[i];
			glm::mat4 m = LoadMatrix( pBlends[boneIndex] );

			blended += w * m;
		}

		glm::vec4 v( pSrc->x, pSrc->y, pSrc->z, 1.0f );
		glm::vec4 r = blended * v;

		pRes->x = r.x;
		pRes->y = r.y;
		pRes->z = r.z;
	}
}

// GSSEtransform.h SSEBatchTransform (MatrixMultiplyVec3 inlined), the TT_SIMPLE path.
inline void SSEBatchTransform( const SHMatrix &m, const CVec3 *vin, CVec3 *vout, int len )
{
	const glm::mat4 matrix = LoadMatrix( m );
	for ( int i = 0; i < len; ++i )
	{
		const glm::vec4 v{ vin[i].x, vin[i].y, vin[i].z, 1.0f };
		const glm::vec4 res = matrix * v;
		vout[i].x = res.x;
		vout[i].y = res.y;
		vout[i].z = res.z;
	}
}

} // namespace NSkinningReference
