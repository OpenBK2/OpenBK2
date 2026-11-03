#include "stdafx.h"

#include "GCombiner.h"
#include "GfxBuffers.h"
#include "GfxRender.h"
#include "GNormalTransform.h"
#include "GScene.h"
#include "GSkinning.h"
#include "GSSEtransform.h"
#include "Misc/HashFuncs.h"
#include "RadixSort.h"

#include <cstdint>
#include <thread>

#include <boost/sort/spreadsort/integer_sort.hpp>

typedef NGfx::SGeomVecFull SGfxVertex;
typedef NGfx::SGeomVecT2C1 STnLVertex;

namespace NGScene
{
extern bool bLowRAM;

// Stands in for the MMX transformer list on the path that has none. It used to
// be a null pointer dereferenced into a reference, which is undefined whether or
// not the callee ever looks at it, and licenses an optimizer that notices to
// treat the branch doing it as unreachable. A real empty vector costs one static
// object and the callee sees the same emptiness it saw before.
static const std::vector<NGfx::SCompactTransformer> noMMXTransformers;

// IPart

IPart::IPart( CPtrFuncBase<CObjectInfo> *pData, CPerMaterialCombiner *_pCombiner )
	: pObjInfo( pData )
	, pCombiner( _pCombiner )
	, fAverageTriArea( 0 )
{
	if ( IsValid( pCombiner ) )
	{
		pCombiner->AddPart( this );
	}
}

IPart::~IPart()
{
	if ( IsValid( pCombiner ) )
	{
		pCombiner->RemovePart( this );
	}
}

void IPart::RefreshObjectInfo()
{
	pObjInfo.Refresh();
	while ( !pObjInfo->GetValue() )
	{
		std::this_thread::yield();
	}
}

void IPart::ResetCachedTransform()
{
	xformedPositions.clear();
	gfxData.Clear();
	bGfxInvalidated = true;
	cacheLighting.Clear();
}

void IPart::SetCombiner( CPerMaterialCombiner *_pCombiner )
{
	if ( pCombiner == _pCombiner )
	{
		return;
	}
	if ( IsValid( pCombiner ) )
	{
		pCombiner->RemovePart( this );
	}
	pCombiner = _pCombiner;
	if ( IsValid( pCombiner ) )
	{
		pCombiner->AddPart( this );
	}
}

void IPart::SetObjectInfoNode( CPtrFuncBase<CObjectInfo> *p )
{
	pObjInfo = p;
	if ( pCombiner )
	{
		pCombiner->MarkWasted( this );
	}
	ResetCachedTransform();
}

// CAnimationWatch

bool CAnimationWatch::NeedUpdate()
{
	bool bCombiner = pCombiner.Refresh();
	const std::vector<CPtr<IPart>> &parts = pCombiner->GetValue();
	if ( bCombiner )
	{
		TWatchSet oldWatch;
		watch.swap( oldWatch );
		indices.resize( 0 );
		// collect new targets
		for ( int k = 0; k < parts.size(); ++k )
		{
			parts[k]->AddChangeTrackers( this, bAnimationOnly );
			while ( indices.size() < watch.size() )
			{
				indices.push_back( k );
			}
		}
		// sync with previous trackers
		std::unordered_map<void *, int> old;
		for ( int k = 0; k < oldWatch.size(); ++k )
		{
			old[oldWatch[k]] = k;
		}
		for ( int k = 0; k < watch.size(); ++k )
		{
			std::unordered_map<void *, int>::iterator i = old.find( watch[k] );
			if ( i != old.end() )
			{
				watch[k].Sync( oldWatch[i->second] );
			}
		}
	}
	bool bRes = false;
	for ( int k = 0; k < watch.size(); ++k )
	{
		if ( watch[k].Refresh() )
		{
			parts[indices[k]]->ResetCachedTransform();
			bRes = true;
		}
	}
	return bRes;
}

// CPerMaterialCombiner

CPerMaterialCombiner::CPerMaterialCombiner( int )
	: bHasChanged( false )
{
	pAnimation = new CAnimationWatch( this, true );
	pFullChange = new CAnimationWatch( this, false );
}

bool CPerMaterialCombiner::NeedUpdate()
{
	return bHasChanged;
}

void CPerMaterialCombiner::Recalc()
{
	bHasChanged = false;
	if ( value.size() < 4 )
	{
		return;
	}

	boost::sort::spreadsort::integer_sort(
			std::begin( value ),
			std::end( value ),
			[]( const auto &v, const unsigned offset ) { return v->GetSortValue() >> offset; },
			[]( const auto &a, const auto &b ) { return a->GetSortValue() < b->GetSortValue(); } );
}

void CPerMaterialCombiner::AddPart( IPart *pPart )
{
	ASSERT( value.size() < PF_MAX_PARTS_PER_COMBINER );
	value.push_back( pPart );
	bHasChanged = true;
}

void CPerMaterialCombiner::RemovePart( IPart *pPart )
{
	std::vector<CPtr<IPart>>::iterator i = find( value.begin(), value.end(), pPart );
	if ( i == value.end() )
	{
		return;
	}
	value.erase( i );
	bHasChanged = true;
}

void CPerMaterialCombiner::MarkWasted( IPart *pPart )
{
	Updated();
}

int CPerMaterialCombiner::operator&( CStructureSaver &f )
{
	f.Add( 1, &value );
	f.Add( 3, &pAnimation );
	f.Add( 4, &pFullChange );
	f.Add( 5, &bHasChanged );
	return 0;
}

// CMMXAnimationMatrices

void CMMXAnimationMatrices::Recalc()
{
	const std::vector<SHMatrix> &blends = pAnimation->GetValue();
	value.resize( blends.size() );
	for ( int k = 0; k < value.size(); ++k )
	{
		Assign( &value[k], blends[k] );
	}
}

CFuncBase<std::vector<NGfx::SCompactTransformer>> *MakeMMXAnimation( CFuncBase<std::vector<SHMatrix>> *pAnim )
{
	return new CMMXAnimationMatrices( pAnim );
}

static void TransformVertexT( CVec3 *pRes, const SHMatrix &m, const NGfx::SCompactVector &src )
{
	//CVec3 vRes;
	m.RotateVectorTransposed( pRes, NGfx::GetVector( src ) );
	Normalize( pRes );
	/*  //NGfx::SCompactVector *pRes
	NGfx::SCompactVector test;
	NGfx::CalcCompactVector( &test, vRes );
	NGfx::SCompactTransformer transformer;
	AssignTransposed( &transformer, m );
	MMXTransformVector( pRes, &transformer, &src );
	ASSERT( fabs( NGfx::GetVector(test) - NGfx::GetVector(*pRes) ) < 0.02f )*/
}

// CVec3 is three packed floats and SHMatrix sixteen row-major ones with the
// translation in _14, _24, _34, which is what the GSkinning.h kernels read.
static_assert( sizeof( CVec3 ) == 3 * sizeof( float ) );
static_assert( sizeof( SHMatrix ) == 16 * sizeof( float ) );
static_assert( sizeof( SRealVertexWeight ) == sizeof( NSkinning::SSkinWeights ) );

// One matrix for every position: rigid skinned parts (see CObjectInfo::GetRigidBone)
// and parts with a simple transform. NSkinning::TransformPositions keeps the matrix
// columns in registers; the glm SSEBatchTransform it replaces reloaded them per
// vertex and ran 2 to 3 times slower in the benchmark (benchmark/Skinning.cpp).
static void TransformPosition( const std::vector<CVec3> &srcPos, CVec3 *pRes, const SHMatrix &m )
{
	const int nCount = srcPos.size();
	if ( nCount > 0 )
	{
		NSkinning::TransformPositions( &srcPos[0].x, &pRes->x, nCount, &m._11 );
	}
}

static void TransformPosition( const std::vector<CVec3> &srcPos,
                               CVec3 *pRes,
                               const SRealVertexWeight *pWeight,
                               const std::vector<SHMatrix> &blends )
{
	// Blended parts: per vertex, the three used rows of up to four bone matrices are
	// blended (12 floats, where SSESkinning blended 16 after converting every bone's
	// matrix for every vertex), then applied once. No alignment or scratch needed,
	// and the cost does not grow with the skeleton. Zero float weights are skipped as
	// SSESkinning skipped them. Agrees with it within float rounding, which is all
	// rendering and culling need; the simulation never reads these positions.
	const int nCount = srcPos.size();
	if ( nCount > 0 )
	{
		NSkinning::SkinPositions(
				&srcPos[0].x, &pRes->x, reinterpret_cast<const NSkinning::SSkinWeights *>( pWeight ), nCount, &blends[0]._11 );
	}
}

template <class T>
inline void CalcPerVertexLight( T *pRes,
                                CObjectInfo *p,
                                const std::vector<CVec3> &srcPos,
                                const SUVInfo *pSrc,
                                const std::vector<NGfx::SCompactVector> &normals,
                                const SPerVertexLightState &ls,
                                SCacheLightingInfo *pCache,
                                const SBound &bv )
{
	CalcPerVertexLight(
			pRes, srcPos, pSrc, p->GetPositionIndices(), normals, p->GetAttribute( GATTR_VERTEX_COLOR ), ls, pCache, bv );
}

static std::vector<NGfx::SCompactVector> xformedNormals;

template <class TParam>
struct STGenericTransformer
{
	enum E
	{
		PASS_MMX_BLENDS = 1
	};

	typedef TParam TRes;
	const SPerVertexLightState &ls;

	STGenericTransformer( const SPerVertexLightState &_ls )
		: ls( _ls )
	{
	}

	void CopyTransform( CObjectInfo *pObjInfo,
	                    const SUVInfo *pSrc,
	                    int nVertices,
	                    SCacheLightingInfo *pCache,
	                    const SBound &bv,
	                    TRes *pRes )
	{
		xformedNormals.resize( nVertices );
		for ( int k = 0, nSize = xformedNormals.size(); k < nSize; ++k )
		{
			xformedNormals[k] = pSrc[k].normal;
		}
		CalcPerVertexLight( pRes, pObjInfo, pObjInfo->GetPositions(), pSrc, xformedNormals, ls, pCache, bv );
	}

	void SimpleTransform( CObjectInfo *pObjInfo,
	                      const std::vector<CVec3> &transformed,
	                      const SUVInfo *pSrc,
	                      int nVertices,
	                      SCacheLightingInfo *pCache,
	                      const SBound &bv,
	                      const SFBTransform &trans,
	                      TRes *pRes )
	{
		SHMatrix transformer;
		Transpose( &transformer, trans.backward );

		xformedNormals.resize( nVertices );
		// One matrix for every normal: four at a time, with the matrix prepared once
		// per part instead of once per normal. Bit-exact with MMXTransformVector, see
		// GNormalTransform.h.
		if ( nVertices > 0 )
		{
			NNormals::TransformNormalsRigid(
					&pSrc[0].normal, sizeof( SUVInfo ), &xformedNormals[0].dw, nVertices, &transformer._11 );
		}

		CalcPerVertexLight( pRes, pObjInfo, transformed, pSrc, xformedNormals, ls, pCache, bv );
	}

	void SingleSkinTransform( CObjectInfo *pObjInfo,
	                          const std::vector<CVec3> &transformed,
	                          const SUVInfo *_pSrc,
	                          int nVerties,
	                          SCacheLightingInfo *pCache,
	                          const SBound &bv,
	                          const SRealVertexWeight *_pWeight,
	                          const std::vector<SHMatrix> &blends,
	                          const std::vector<NGfx::SCompactTransformer> &matrices,
	                          TRes *pRes )
	{
		if ( blends.empty() )
		{
			ASSERT( 0 );
			return CopyTransform( pObjInfo, _pSrc, nVerties, pCache, bv, pRes );
		}

		const std::vector<uint16_t> &posIndices = pObjInfo->GetPositionIndices();
		ASSERT( nVerties == posIndices.size() );
		xformedNormals.resize( posIndices.size() );
		const int nNormals = static_cast<int>( xformedNormals.size() );
		// A rigid part (CObjectInfo::GetRigidBone, 98.8% of the shipped parts) moves
		// every normal with one bone: transformed four at a time with the matrix
		// prepared once, bit-exact with the per-normal MMXTransformVector the loop
		// below runs for single-influence vertices.
		const int nRigidBone = pObjInfo->GetRigidBone();
		if ( nRigidBone >= 0 && nRigidBone < static_cast<int>( blends.size() ) )
		{
			if ( nNormals > 0 )
			{
				NNormals::TransformNormalsRigid(
						&_pSrc[0].normal, sizeof( SUVInfo ), &xformedNormals[0].dw, nNormals, &blends[nRigidBone]._11 );
			}
			CalcPerVertexLight( pRes, pObjInfo, transformed, _pSrc, xformedNormals, ls, pCache, bv );
			return;
		}
		const SUVInfo *pSrc = _pSrc;
		for ( int k = 0; k < nNormals; ++k, pSrc++ )
		{
			const SRealVertexWeight *pWeight = _pWeight + posIndices[k];
			if ( pWeight->nWeights[1] == 0 )
			{
				const auto &blend = blends[pWeight->cBoneIndices[0]];
				MMXTransformVector( xformedNormals[k], pSrc->normal, blend );
			}
			else if ( pWeight->nWeights[2] == 0 )
			{
				uint8_t nW1 = pWeight->nWeights[0], nW2 = pWeight->nWeights[1];

				const auto &blend1 = blends[pWeight->cBoneIndices[0]];
				const auto &blend2 = blends[pWeight->cBoneIndices[1]];
				// Bit-exact with MMXTransformVector2, about twice as fast (GNormalTransform.h).
				xformedNormals[k].dw = NNormals::TransformNormalBlend2( pSrc->normal.dw, &blend1._11, nW1, &blend2._11, nW2 );
			}
			else
			{
				const auto &blend1 = blends[pWeight->cBoneIndices[0]];
				const auto &blend2 = blends[pWeight->cBoneIndices[1]];
				const auto &blend3 = blends[pWeight->cBoneIndices[2]];

				uint8_t nW1 = pWeight->nWeights[0], nW2 = pWeight->nWeights[1], nW3 = pWeight->nWeights[2];

				// Bit-exact with MMXTransformVector3, about twice as fast (GNormalTransform.h).
				xformedNormals[k].dw =
						NNormals::TransformNormalBlend3( pSrc->normal.dw, &blend1._11, nW1, &blend2._11, nW2, &blend3._11, nW3 );
			}
		}
		CalcPerVertexLight( pRes, pObjInfo, transformed, _pSrc, xformedNormals, ls, pCache, bv );
	}
};

struct SGenericPosTransformer
{
	typedef CVec3 TRes;

	enum E
	{
		PASS_MMX_BLENDS = 0
	};

	void CopyTransform(
			CObjectInfo *pObjInfo, const SUVInfo *pSrc, int nSize, SCacheLightingInfo *pCache, const SBound &bv, TRes *pRes )
	{
		const std::vector<CVec3> &srcPos = pObjInfo->GetPositions();
		for ( int k = 0; k < srcPos.size(); ++k )
		{
			pRes[k] = srcPos[k];
		}
	}

	void SimpleTransform( CObjectInfo *pObjInfo,
	                      const std::vector<CVec3> &_fake,
	                      const SUVInfo *pSrc,
	                      int nVertices,
	                      SCacheLightingInfo *pCache,
	                      const SBound &bv,
	                      const SFBTransform &trans,
	                      TRes *pRes )
	{
		const std::vector<CVec3> &srcPos = pObjInfo->GetPositions();
		TransformPosition( srcPos, pRes, trans.forward );
	}

	void SingleSkinTransform( CObjectInfo *pObjInfo,
	                          const std::vector<CVec3> &_fake,
	                          const SUVInfo *pSrc,
	                          int nVertices,
	                          SCacheLightingInfo *pCache,
	                          const SBound &bv,
	                          const SRealVertexWeight *pWeight,
	                          const std::vector<SHMatrix> &blends,
	                          const std::vector<NGfx::SCompactTransformer> &matrices,
	                          TRes *pRes )
	{
		const std::vector<CVec3> &srcPos = pObjInfo->GetPositions();
		if ( blends.empty() )
		{
			ASSERT( 0 );
			return CopyTransform( pObjInfo, pSrc, nVertices, pCache, bv, pRes );
		}
		// A rigid part (one bone, weight 1, for every position) is one matrix applied
		// to all its positions: 94% of the positions of the shipped models.
		const int nRigidBone = pObjInfo->GetRigidBone();
		if ( nRigidBone >= 0 && nRigidBone < static_cast<int>( blends.size() ) )
		{
			TransformPosition( srcPos, pRes, blends[nRigidBone] );
			return;
		}
		TransformPosition( srcPos, pRes, pWeight, blends );
	}
};

template <class T>
struct SPartTransformer : public T
{
	SPartTransformer() {}

	SPartTransformer( const SPerVertexLightState &_ls )
		: T( _ls )
	{
	}

	int DoTransform( IPart *p, typename T::TRes *pRes, const std::vector<CVec3> &transformed )
	{
		CObjectInfo *pObjInfo = p->GetObjectInfo();
		if ( !pObjInfo )
		{
			return 0;
		}
		const std::vector<SUVInfo> &srcVerts = pObjInfo->GetVertices();
		if ( srcVerts.empty() )
		{
			return 0;
		}
		SBound bv;
		bv.BoxInit( p->vBVMin, p->vBVMax );
		switch ( p->GetTransformType() )
		{
			case TT_NONE:
				this->CopyTransform( pObjInfo, &srcVerts[0], srcVerts.size(), &p->cacheLighting, bv, pRes );
				break;
			case TT_SIMPLE:
				this->SimpleTransform(
						pObjInfo, transformed, &srcVerts[0], srcVerts.size(), &p->cacheLighting, bv, p->GetSimplePos(), pRes );
				break;
			case TT_SINGLE_SKIN:
				// The vertex path (PASS_MMX_BLENDS) used to pass p->GetMMXAnimation()
				// here. No SingleSkinTransform reads those fixed-point matrices (the
				// normals are transformed with the blends), and asking for them rebuilt
				// a copy of every bone on each animation change, so both paths pass none.
				this->SingleSkinTransform( pObjInfo,
				                           transformed,
				                           &srcVerts[0],
				                           srcVerts.size(),
				                           &p->cacheLighting,
				                           bv,
				                           &( pObjInfo->GetWeights()[0] ),
				                           p->GetAnimation(),
				                           noMMXTransformers,
				                           pRes );
				break;
			default:
				ASSERT( 0 );
				break;
		}
		return srcVerts.size();
	}
};

template <class TVertex>
struct STGfxCacheTransformer : public SPartTransformer<STGenericTransformer<TVertex>>
{
	typedef SPartTransformer<STGenericTransformer<TVertex>> TParent;
	NGfx::CBufferLock<TVertex> geom;
	int nVert;

	STGfxCacheTransformer( CObj<NGfx::CGeometry> *pGeom,
	                       int nTotal,
	                       const SPerVertexLightState &_ls,
	                       NGfx::EBufferUsage usage )
		: TParent( _ls )
		, geom( pGeom, nTotal, usage )
		, nVert( 0 )
	{
	}

	void Transform( IPart *p, const std::vector<CVec3> &transformed )
	{
		CObjectInfo *pObjInfo = p->GetObjectInfo();
		if ( !pObjInfo )
		{
			return;
		}
		int nPartVerts = pObjInfo->GetVertices().size();
		if ( nPartVerts == 0 )
		{
			p->gfxData.Clear();
			return;
		}
		int nSize = sizeof( TVertex ) * nPartVerts;
		if ( bLowRAM )
		{
			int nTransformed = this->DoTransform( p, &geom[nVert], transformed );
			ASSERT( nTransformed == nPartVerts );
		}
		else if ( p->gfxData.GetSize() == nSize )
		{
			// Unchanged since its last build: copy the cached vertices.
			std::memcpy( &geom[nVert], &p->gfxData[0], nSize );
		}
		else
		{
			// Rebuild. A part dropped for a change on its last build as well keeps
			// changing, typically every frame (a moving unit folds its placement into
			// its bone matrices). Caching its vertices in gfxData only to copy them
			// into the vertex buffer cost a second pass over every vertex and a free and
			// allocation per rebuild, so such a part is built in place: the writes are
			// sequential and never read back, which suits the write-combined memory a
			// dynamic buffer may live in. A part built for the first time, or after a
			// stretch without changes, still goes through gfxData, so one that then
			// stays put is copied, not rebuilt, on the next recalc.
			const bool bChanged = p->bGfxInvalidated;
			p->bGfxInvalidated = false;
			p->nGfxChangeRun = bChanged ? static_cast<uint8_t>( std::min( p->nGfxChangeRun + 1, 2 ) ) : 0;
			if ( p->nGfxChangeRun >= 2 )
			{
				p->gfxData.Clear();
				int nTransformed = this->DoTransform( p, &geom[nVert], transformed );
				ASSERT( nTransformed == nPartVerts );
			}
			else
			{
				p->gfxData.Resize( nSize );
				int nTransformed = this->DoTransform( p, (TVertex *)&p->gfxData[0], transformed );
				ASSERT( nTransformed == nPartVerts );
				std::memcpy( &geom[nVert], &p->gfxData[0], nSize );
			}
		}
		nVert += nPartVerts;
		ASSERT( nVert <= geom.GetSize() );
	}
};

typedef STGfxCacheTransformer<SGfxVertex> SGfxCacheTransformer;
typedef STGfxCacheTransformer<STnLVertex> STnlCacheTransformer;

void TransformPart( IPart *p, std::vector<CVec3> *pRes, std::vector<STriangle> *pTris )
{
	p->RefreshObjectInfo();
	CObjectInfo *pObjInfo = p->GetObjectInfo();
	if ( pTris )
	{
		pObjInfo->GetPosTriangles( pTris );
	}
	pRes->resize( pObjInfo->GetPositions().size() );
	if ( pRes->empty() )
	{
		return;
	}
	SPartTransformer<SGenericPosTransformer> trans;
	trans.DoTransform( p, &( ( *pRes )[0] ), *pRes );
}

// CVBCombiner

void CVBCombiner::XFormPosition()
{
	const std::vector<CPtr<IPart>> &parts = pCombiner->GetValue();
	partBVs.resize( parts.size() );

	SBoundCalcer bcAll;
	for ( int i = 0; i < parts.size(); ++i )
	{
		IPart *pPart = parts[i];
		pPart->RefreshObjectInfo();
		CObjectInfo *pObjInfo = pPart->GetObjectInfo();
		Zero( partBVs[i] ); //SBoundCalcer bc; bc.Add( CVec3(0,0,0) ); bc.Make( &partBVs[i] );
		if ( !pObjInfo )
		{
			continue;
		}
		// xform
		std::vector<CVec3> *pRes = &pPart->xformedPositions;
		int nSize = pObjInfo->GetPositions().size();
		if ( nSize != pRes->size() )
		{
			pRes->resize( nSize );
			if ( nSize > 0 )
			{
				SPartTransformer<SGenericPosTransformer> trans;
				trans.DoTransform( pPart, &( ( *pRes )[0] ), *pRes );
				// calc bv
				// SBoundCalcer starts min and max at +-1e38. This used to set the max to
				// 0 first, a leftover from the original MMX bound, which compared float
				// bits as integers and needed that start; with float compares it raised
				// the max to 0 on any axis where the whole part is negative, such as an
				// aircraft arriving from beyond the lower map edge.
				SBoundCalcer bc;

				for ( int k = 0; k < nSize; ++k )
				{
					const CVec3 &v = ( *pRes )[k];

					UpdateBounds( bc.ptMin, bc.ptMax, v );
				}
				pPart->vBVMin = bc.ptMin;
				pPart->vBVMax = bc.ptMax;

				pPart->fAverageTriArea = pObjInfo->GetAverageTriArea();
			}
		}
		if ( nSize > 0 )
		{
			SBoundCalcer bc;
			bc.Add( pPart->vBVMin );
			bc.Add( pPart->vBVMax );
			bc.Make( &partBVs[i] );
			bcAll.Add( bc );
		}
	}
	bcAll.Make( &bound );
	bNeedXForm = false;
	bNeedRecalc = true;
	bDroppedXForm = false;
}

template <class TTrans>
void CVBCombiner::SimpleTransform( TTrans *p )
{
	const std::vector<CPtr<IPart>> &parts = pCombiner->GetValue();
	TTrans &trans = *p;
	for ( int i = 0; i < parts.size(); ++i )
	{
		IPart *p = parts[i];
		trans.Transform( p, p->xformedPositions );
	}
}

// possible optimization: amount of animated geometry could be reduced by separating maximal octree node size
// for static & dynamic geometry
void CVBCombiner::DoRecalc()
{
	if ( bNeedXForm || bDroppedXForm )
	{
		XFormPosition();
	}
	bNeedRecalc = false;
	ASSERT( NGfx::N_VEC_FULL_TEX_SIZE == N_VERTEX_TEX_SIZE );
	const std::vector<CPtr<IPart>> &parts = pCombiner->GetValue();
	partBVs.resize( parts.size() );
	std::vector<CPtr<IPart>>::const_iterator i = parts.begin();
	int nVerts = 0, nPositions = 0;

	for ( ; i != parts.end(); ++i )
	{
		( *i )->RefreshObjectInfo();
		CObjectInfo *pObjInfo = ( *i )->GetObjectInfo();
		if ( !pObjInfo )
		{
			continue;
		}
		nPositions += pObjInfo->GetPositions().size();
		nVerts += pObjInfo->GetVertices().size();
	}

	if ( !nVerts )
	{
		pValue = 0;
		bound.BoxInit( CVec3( 0, 0, 0 ), CVec3( 0, 0, 0 ) );
		for ( int k = 0; k < partBVs.size(); ++k )
		{
			partBVs[k] = bound.s;
		}
		return;
	}
	NGfx::EBufferUsage bufUsage = NGfx::DYNAMIC; //ct == CT_STATIC ? NGfx::STATIC : NGfx::DYNAMIC;
	if ( NGfx::IsTnLDevice() )
	{
		STnlCacheTransformer trans( &pValue, nVerts, pLightState->GetValue(), bufUsage );
		SimpleTransform( &trans );
	}
	else
	{
		SGfxCacheTransformer trans( &pValue, nVerts, pLightState->GetValue(), bufUsage );
		SimpleTransform( &trans );
	}
}

bool CVBCombiner::NeedUpdate()
{
	bool bLightState = pLightState.Refresh();
	if ( bLightState )
	{
		const std::vector<CPtr<IPart>> &parts = pCombiner->GetValue();
		for ( int i = 0; i < parts.size(); ++i )
		{
			if ( parts[i] )
			{
				parts[i]->ResetCachedLighting();
			}
		}
	}
	return NeedXForm() | bNeedRecalc | bLightState;
}

void CVBCombiner::Recalc()
{
	DoRecalc();
	if ( bLowRAM )
	{
		FreeMemory();
	}
}

void CVBCombiner::FreeMemory()
{
	if ( bDroppedXForm )
	{
		return;
	}
	const std::vector<CPtr<IPart>> &parts = pCombiner->GetValue();
	for ( int i = 0; i < parts.size(); ++i )
	{
		if ( parts[i] )
		{
			parts[i]->ResetCachedTransform();
		}
	}
	bDroppedXForm = true;
}

// CIBCombiner

void CIBCombiner::Recalc()
{
	const std::vector<CPtr<IPart>> &parts = pCombiner->GetValue();
	//vector< CPtr<IPart> >::const_iterator i;// = parts.begin();
	value.resize( parts.size() );
	int nTotalTris = 0;
	std::vector<char> bIs2Sided( parts.size(), (char)0 );
	for ( int i = 0; i < parts.size(); ++i ) //i = parts.begin(); i != parts.end(); ++i )
	{
		parts[i]->RefreshObjectInfo();
		CObjectInfo *pObjInfo = parts[i]->GetObjectInfo();
		if ( !pObjInfo )
		{
			continue;
		}
		bIs2Sided[i] = parts[i]->Is2Sided();
		if ( bIs2Sided[i] )
		{
			nTotalTris += pObjInfo->GetTrisCount() * 2;
		}
		else
		{
			nTotalTris += pObjInfo->GetTrisCount();
		}
	}

	triBuffer.resize( nTotalTris + 1 );
	STriangle *pFill = &triBuffer[0];
	//
	int nOffset = 0, nLargeOffset = 0;
	for ( int i = 0; i < parts.size(); ++i )
	{
		parts[i]->RefreshObjectInfo();
		CObjectInfo *pObjInfo = parts[i]->GetObjectInfo();
		value[i].pTri = pFill; //&triBuffer[nTriFill];
		if ( !pObjInfo )
		{
			value[i].nTris = 0;
			continue;
		}
		if ( nOffset + pObjInfo->GetVertices().size() > 65535 )
		{
			nLargeOffset += nOffset;
			nOffset = 0;
		}
		std::vector<STriangle> tris;
		if ( ibt == IBTT_POSITIONS )
		{
			pObjInfo->GetVxPositionTriangles( &tris );
		}
		else if ( ibt == IBTT_VERTICES )
		{
			tris = pObjInfo->GetGeometry();
		}
		else
		{
			ASSERT( 0 );
		}
		bool b2Sided = bIs2Sided[i];
		if ( b2Sided )
		{
			value[i].nTris = tris.size() * 2;
		}
		else
		{
			value[i].nTris = tris.size();
		}
		value[i].nOffset = nOffset;
		value[i].nBaseIndex = nLargeOffset; //nOffset;
		unsigned short nUseOffset = nOffset;
		if ( b2Sided )
		{
			for ( std::vector<STriangle>::iterator k = tris.begin(), kEnd = tris.end(); k != kEnd; ++k )
			{
				STriangle &tDest = *pFill++, &tSrc = *k;
				tDest.i1 = tSrc.i1 + nUseOffset;
				tDest.i2 = tSrc.i3 + nUseOffset;
				tDest.i3 = tSrc.i2 + nUseOffset;
			}
		}
		for ( std::vector<STriangle>::iterator k = tris.begin(), kEnd = tris.end(); k != kEnd; ++k )
		{
			STriangle &tDest = *pFill++, &tSrc = *k;
			tDest.i1 = tSrc.i1 + nUseOffset;
			tDest.i2 = tSrc.i2 + nUseOffset;
			tDest.i3 = tSrc.i3 + nUseOffset;
		}
		nOffset += pObjInfo->GetVertices().size();
	}
	ASSERT( pFill <= &triBuffer[triBuffer.size() - 1] );
	// These triangles stay as they are until the next Recalc, which only a part
	// being added, removed or replaced triggers, never an animation. Publish the
	// array so the renderer can keep it in the static index cache and draw it with
	// BaseVertexIndex rather than rebasing every index into the dynamic index buffer
	// on every frame. Set drops the slot of the previous array.
	if ( !pStatic )
	{
		pStatic = new NGfx::CStaticTriangles();
	}
	pStatic->Set( &triBuffer[0], static_cast<int>( pFill - &triBuffer[0] ) );
	for ( int i = 0; i < value.size(); ++i )
	{
		value[i].pStatic = pStatic;
	}
}

} // namespace NGScene

using namespace NGScene;
BASIC_REGISTER_CLASS( _3DMOTOR, IPart )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x02741133, CPerMaterialCombiner )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x02741134, CVBCombiner )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x02741135, CIBCombiner )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x009c2130, CMMXAnimationMatrices )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x2013EC82, CAnimationWatch )
