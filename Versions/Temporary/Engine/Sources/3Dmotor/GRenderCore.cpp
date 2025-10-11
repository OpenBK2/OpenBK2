#include "stdafx.h"
#include "GRenderCore.h"
#include "3DLib/Transform.h"

inline bool DoesIntersect( const SSphere &a, const SSphere &b )
{
	CVec3 ptDif = a.ptCenter - b.ptCenter;
	return fabs2(ptDif) < sqr( a.fRadius + b.fRadius );
}
namespace NGScene
{

// CSceneFragments

CSceneFragments::CSceneFragments() : nSceneTris(0), bNeedHSR(false)//, pRejected(0), nRejectedUsed(0) 
{
	fragments.push_back( fragmentInfos.Alloc() );
	ASSERT( fragments.size() == 1 );
}

int CSceneFragments::AddGeometry( CObjectBase *pHandle, SRenderGeometryInfo *pGeometry, const SBound &_bv, bool bNotAddBound )
{
	SRenderStaticInfo *pRes = staticInfos.Alloc();
	pRes->pHandle = pHandle;
	pRes->bv = _bv;
	statics.push_back( pRes );
	geometries.push_back( pGeometry );

	if ( !bNotAddBound )
		bc.Add( _bv );

	return geometries.size() - 1;
}

void CSceneFragments::AddElement( int _nGeometryIndex, const CPartFlags &_parts, 
	IMaterial *pMaterial, const SPerPartVariables &_vars )
{
	if ( _parts.IsEmpty() )
		return;
	// search for suitable fragment
	SRenderFragmentKey k;
	k.pMat = pMaterial;
	k.fFade = _vars.fFade;
	k.nPriority = _vars.nPriority;
	k.pLM = _vars.pLM;
	CFragmentHash::iterator i = fragmentHash.find( k );
	SRenderFragmentInfo *pFragment;
	if ( i == fragmentHash.end() )
	{
		fragmentHash[k] = fragments.size();
		pFragment = fragmentInfos.Alloc();
		pFragment->pMaterial = pMaterial;
		pFragment->vars = _vars;
		fragments.push_back( pFragment );
		pFragment->elements.reserve( 8 );
	}
	else
		pFragment = fragments[ i->second ];
	for ( int i = 0; i < _parts.GetBlocksNumber(); ++i )
	{
		int nFlags = _parts.GetBlock( i );
		if ( nFlags )
			pFragment->elements.push_back( SRenderFragmentInfo::SElement( _nGeometryIndex, i, nFlags ) );
	}
}

void CSceneFragments::AddLitParticles( IVBCombiner *pCombiner, CFuncBase<std::vector<NGfx::STriangleList> > *pTris, int nPart, const SBound &_bv )
{
	SRenderGeometryInfo *pGeom = geometryInfos.Alloc();
	pGeom->pTriLists[TLT_POSITION] = pTris;
	pGeom->pTriLists[TLT_GEOM] = pTris;
	pGeom->pVertices = pCombiner;
	pGeom->pVertices.Refresh();
	int nGeom = AddGeometry( 0, pGeom, _bv, false );
	int nBlock = nPart / 32, nShift = nPart & 31;
	fragments[0]->elements.push_back( SRenderFragmentInfo::SElement( nGeom, nBlock, 1<<nShift ) );
}

void CSceneFragments::SetLitParticlesMaterial( IMaterial *p )
{
	ASSERT( fragments[0]->pMaterial == 0 || fragments[0]->pMaterial == p );
	fragments[0]->pMaterial = p;
}

// not exact realisation
bool CSceneFragments::HasSelectedFragments() const
{
	if ( fragments.size() == 1 && fragments[0]->elements.empty() )
		return false;
	if ( filterFragment.size() )
	{
		bool bAllAreFiltered = true;
		for ( int k = 0; k < fragments.size(); ++k )
		{
			if ( filterFragment[k] == 0 && !fragments[k]->elements.empty() )
			{
				bAllAreFiltered = false;
				break;
			}
		}
		if ( bAllAreFiltered )
			return false;
	}
	if ( filterGeometry.size() )
	{
		bool bAllAreFiltered = true;
		for ( int k = 0; k < filterGeometry.size(); ++k )
		{
			if ( filterGeometry[k] != FST_REJECT )
			{
				bAllAreFiltered = false;
				break;
			}
		}
		if ( bAllAreFiltered )
			return false;
	}
	return true;
}

void CSceneFragments::HideGeometry( const std::vector<CPartFlags> &flags )
{
	int n = (std::min)( geometries.size(), flags.size() );
	if ( filterGeometry.empty() )
	{
		filterGeometry.resize( geometries.size(), FST_ACCEPT );
		selectedParts.resize( geometries.size(), TakeAllParts() );
	}
	for ( int k = 0; k < n; ++k )
	{
		CPartFlags filter = flags[k];
		filter.Invert(); // taken are set
		char &f = filterGeometry[ k ];
		if ( f == FST_REJECT )
			continue;
		if ( f == FST_SPLIT )
			filter.CalcAnd( selectedParts[k] );
		if ( filter.IsEmpty() )
			f = FST_REJECT;
		else
		{
			f = FST_SPLIT;
			selectedParts[ k ] = filter;
		}
	}
}


inline EFragmentsSplit GetIntersectLevel( const SBound &bound, const SBound &test )
{
	CVec3 ptDif = bound.s.ptCenter - test.s.ptCenter;
	// fast outside
	float fDif2 = fabs2(ptDif);
	if ( fDif2 > sqr( bound.s.fRadius + test.s.fRadius ) )
		return FST_REJECT;
	// fast inside, fDif + test.s.fRadius < bound.s.fRadius
	float fRDif = bound.s.fRadius - test.s.fRadius; 
	if ( fDif2 < fRDif * fabs( fRDif ) )
		return FST_ACCEPT;
	if ( 
		fabs(ptDif.x) + test.ptHalfBox.x < bound.ptHalfBox.x && 
		fabs(ptDif.y) + test.ptHalfBox.y < bound.ptHalfBox.y && 
		fabs(ptDif.z) + test.ptHalfBox.z < bound.ptHalfBox.z )
		return FST_ACCEPT;
	return FST_SPLIT;
}

inline EFragmentsSplit GetIntersectLevel( const SSphere &bound, const SSphere &test )
{
	CVec3 ptDif = bound.ptCenter - test.ptCenter;
	// fast outside
	float fDif2 = fabs2(ptDif);
	if ( fDif2 > sqr( bound.fRadius + test.fRadius ) )
		return FST_REJECT;
	// fast inside, fDif + test.s.fRadius < bound.s.fRadius
	float fRDif = bound.fRadius - test.fRadius; 
	if ( fDif2 < fRDif * fabs( fRDif ) )
		return FST_ACCEPT;
	return FST_SPLIT;
}

// Filter ops

EFragmentsSplit SBoundIntersectFilter::operator()( SRenderStaticInfo *pStatic, SRenderGeometryInfo *pGeom, CPartFlags *pRes ) const
{
	EFragmentsSplit res = GetIntersectLevel( bv, pStatic->bv );
	if ( res == FST_SPLIT )
	{
		const std::vector<SSphere> &bounds = pGeom->pVertices->GetBounds();
		for ( int k = 0; k < bounds.size(); ++k )
		{
			if ( !pRes->IsSet( k ) )
				continue;
			if ( !DoesIntersect( bounds[k], bv ) )
				pRes->Reset( k );
		}
	}
	return res;
}

EFragmentsSplit SFrustrumFilter::operator()( SRenderStaticInfo *pStatic, SRenderGeometryInfo *pGeom, CPartFlags *pRes ) const
{
	if ( !pTS->PushClipHint( pStatic->bv ) )
		return FST_REJECT;
	if ( pTS->IsFullGet() )
	{
		pTS->PopClipHint();
		return FST_ACCEPT;
	}
	const std::vector<SSphere> &bounds = pGeom->pVertices->GetBounds();
	for ( int k = 0; k < bounds.size(); ++k )
	{
		if ( !pRes->IsSet( k ) )
			continue;
		if ( !pTS->IsIn( bounds[k] ) )
			pRes->Reset( k );
	}
	pTS->PopClipHint();
	return FST_SPLIT;
}

EFragmentsSplit SSphereFilter::operator()( SRenderStaticInfo *pStatic, SRenderGeometryInfo *pGeom, CPartFlags *pRes ) const
{
//	EFragmentsSplit res = GetIntersectLevel( sph, pStatic->bv.s );
	EFragmentsSplit res = GetIntersectLevel( bound, pStatic->bv );
	if ( res == FST_SPLIT )
	{
		const std::vector<SSphere> &bounds = pGeom->pVertices->GetBounds();
/*		CPartFlags res1 = *pRes;
		for ( int k = 0; k < bounds.size(); ++k )
		{
			if ( !res1.IsSet( k ) )
				continue;
			if ( !DoesIntersect( bounds[k], sph ) )
				res1.Reset( k );
		}*/
		//CPartFlags res2 = *pRes;
		CPartFlags &res2 = *pRes;
		int nPartsNum = bounds.size();
		for ( int k = 0; k < nPartsNum; k += 32 )
		{
			int nBlock = res2.GetBlock( k >> 5 );
			if ( nBlock == 0 )
				continue;
			int nFinal = (std::min)( k + 32, nPartsNum );
			int nStart = k;
			for ( int i = k, nTest = 1; i < nFinal; ++i )
			{
				if ( nBlock & nTest )
				{
					if ( !DoesIntersect( bounds[i], sph ) )
						nBlock &= ~nTest;
				}
				nTest <<= 1;
			}
			res2.SetBlock( k >> 5, nBlock );
		}
//		ASSERT( res1 == res2 );
		//*pRes = res2;
	}
	return res;
}

void SIgnorePartsInfo::Init( const CPartFlags &_flags, const std::vector<CPtr<IPart> > *pParts )
{
	flags = _flags;
	ignore.resize( pParts->size() );
	int nRes = 0;
	for ( int k = 0; k < pParts->size(); ++k )
	{
		if ( !_flags.IsSet( k ) )
			ignore[nRes++] = (*pParts)[k];
	}
	ignore.resize( nRes );
}

EFragmentsSplit SIgnoredSphereFilter::operator()( SRenderStaticInfo *pStatic, SRenderGeometryInfo *pGeom, CPartFlags *pRes ) const
{
	if ( !pIgnoreList )
		return sph( pStatic, pGeom, pRes );
	CFullIgnorePartsHash::iterator i = pIgnoreList->find( pStatic->pHandle );
	if ( i == pIgnoreList->end() )
		return FST_REJECT;//sph( pStatic, pGeom, pRes );
	SIgnorePartsInfo &ipi = i->second;
	if ( !ipi.pTrackCombiner )
		ipi.pTrackCombiner = pGeom->pVertices->GetCombiner();
	ASSERT( ipi.pTrackCombiner == pGeom->pVertices->GetCombiner() );
	if ( ipi.pTrackCombiner == 0 )
	{
		ASSERT(0);
		pIgnoreList->erase( i );
		return FST_ACCEPT;
	}
	if ( ipi.pTrackCombiner != 0 && ipi.pTrackCombiner.Refresh() )
	{
		ipi.flags.TakeAll();
		const std::vector< CPtr<IPart> > &parts = ipi.pTrackCombiner->GetValue();
		typedef std::unordered_map<IPart*, bool> CPtrSet;
		CPtrSet ignore;
		int nDst = 0;
		for ( int k = 0; k < ipi.ignore.size(); ++k )
		{
			IPart *p = ipi.ignore[k];
			ipi.ignore[ nDst ] = p;
			if ( !IsValid( p ) )
				continue;
			ignore[p];
			++nDst;
		}
		ipi.ignore.resize( nDst );
		const std::vector<SSphere> &bounds = pGeom->pVertices->GetBounds();
		for ( int k = 0; k < parts.size(); ++k )
		{
			if ( ignore.find( parts[k] ) != ignore.end() )
				ipi.flags.Reset( k );
			if ( !DoesIntersect( bounds[k], sph.sph ) )
				ipi.flags.Reset( k );
		}
	}
	pRes->CalcAnd( ipi.flags );
	return FST_SPLIT;
}


void MakeSingleOp( CRenderCmdList *pRes, CSceneFragments &src, bool bTakeLitParticles, float fMinFade, 
	SPerspDirectionalDepthInfo *pDepthInfo, unsigned char op,
	CRenderCmdList::UParameter _p1,
	CRenderCmdList::UParameter _p2,
	CRenderCmdList::UParameter _p3,
	int nStencilOp )
{
	const std::vector<SRenderFragmentInfo*> &fragments = src.GetFragments();
	for ( int k = bTakeLitParticles ? 0 : 1; k < fragments.size(); ++k )
	{
		if ( src.IsFilteredFragment( k ) )
			continue;
		const SRenderFragmentInfo &f = *fragments[k];
		if ( f.vars.fFade < fMinFade )
			continue;
		
		COpGenContext fi( &pRes->ops, &f );		
		if ( f.pMaterial )
		{
			f.pMaterial->AddATOperations( &fi, pDepthInfo );
			if ( fi.HasAddedOps() )//bAlphaTest )
			{
				fi.AddOperation( op, 100, nStencilOp|DPM_EQUAL, 0, _p1, _p2, _p3 );
				continue;
			}

			extern bool bNewShadows;
			if( !bNewShadows )
			{
		
			if ( f.pMaterial->DoesBackFaceCastShadow() )
				fi.AddOperation( op, 100, nStencilOp|SHADOW_CULL_CCW, 0, _p1, _p2, _p3 );
			else
				fi.AddOperation( op, 100, nStencilOp, 0, _p1, _p2, _p3 );

			continue;
			}
		}	

		fi.AddOperation( op, 100, nStencilOp, 0, _p1, _p2, _p3 );
	}
}

}

using namespace NGScene;

