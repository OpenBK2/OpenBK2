#pragma once

#include <algorithm>
#include <limits>

// What COcTreeNode::GetNode decided while descending from the root to a node, so
// that a caller can tell, without descending again, that a new centre and radius
// would reach the same node: every split comparison on the way goes the same way
// exactly when the centre lies in [lo, hi) on each axis, every node above descends
// and the node reached stops exactly when fRadiusMin <= radius < fRadiusMax, and
// none of it holds any more once the root has been resized (nLayout). The bounds
// are the values the descent compared with, not the node's cube, so the test is
// the descent's own and gives the same node for every input, NaN included.
struct SOcTreePlacement
{
	float lo[3] = { 0, 0, 0 }, hi[3] = { 0, 0, 0 };
	float fRadiusMin = 0, fRadiusMax = 0;
	int nLayout = -1;

	bool IsSamePlacement( const CVec3 &ptCenter, float fRadius, int nRootLayout ) const
	{
		return nLayout == nRootLayout && ptCenter.x >= lo[0] && ptCenter.x < hi[0] && ptCenter.y >= lo[1] &&
		       ptCenter.y < hi[1] && ptCenter.z >= lo[2] && ptCenter.z < hi[2] && fRadius >= fRadiusMin &&
		       fRadius < fRadiusMax;
	}
};

template <class TFinal, int N_MIN_NODE, bool B_GROW_ROOT = false>
class COcTreeNode : public CObjectBase
{
	typedef COcTreeNode<TFinal, N_MIN_NODE, B_GROW_ROOT> CSelf;
	CVec3 ptBase;
	float fSize;
	CObj<TFinal> links[8];
	CPtr<TFinal> pUpLink;
	// Bumped whenever this node is resized while it is a root (initial sizing and
	// every GrowRootToContain step), which changes the decisions GetNode makes.
	// Not saved: a loaded tree starts again at 0 and SOcTreePlacement at -1.
	int nLayoutChanges = 0;

	//! get multiplier to fSize*sqrt(3) to calc maximal element size
	float GetMaxElementSize() const
	{
		return 0.25f * fSize * sqrt( 3.0f );
	}

	// Grow the root in place so references to its update lists and existing nodes
	// stay valid. Subclasses can preserve cached subtree state on the newly
	// created parent through InitGrownChild.
	void GrowRootToContain( const CVec3 &ptCenter )
	{
		while ( ptCenter.x < ptBase.x || ptCenter.x >= ptBase.x + fSize || ptCenter.y < ptBase.y ||
		        ptCenter.y >= ptBase.y + fSize || ptCenter.z < ptBase.z || ptCenter.z >= ptBase.z + fSize )
		{
			CVec3 ptNewBase = ptBase;
			int nOldRootIndex = 0;
			if ( ptCenter.x < ptBase.x )
			{
				ptNewBase.x -= fSize;
				nOldRootIndex |= 1;
			}
			if ( ptCenter.y < ptBase.y )
			{
				ptNewBase.y -= fSize;
				nOldRootIndex |= 2;
			}
			if ( ptCenter.z < ptBase.z )
			{
				ptNewBase.z -= fSize;
				nOldRootIndex |= 4;
			}

			// Put the old children beneath a node with their original extent.
			// Root-owned objects remain on the now larger root; moving them would
			// invalidate external owners and updatable-object lists.
			CObj<TFinal> pOldRoot = new TFinal;
			pOldRoot->SetSize( ptBase, fSize );
			pOldRoot->SetUpLink( static_cast<TFinal *>( this ) );
			InitGrownChild( pOldRoot );
			for ( int i = 0; i < 8; ++i )
			{
				pOldRoot->links[i] = links[i];
				if ( links[i] )
				{
					links[i]->SetUpLink( pOldRoot );
				}
				links[i] = 0;
			}
			links[nOldRootIndex] = pOldRoot;
			SetSize( ptNewBase, fSize * 2 );
		}
	}

protected:
	// The default tree has no cached state associated with its child links.
	virtual void InitGrownChild( TFinal *pChild ) const {}

	void SetUpLink( TFinal *_p )
	{
		pUpLink = _p;
	}

public:
	void SetSize( const CVec3 &_ptBase, float _fSize )
	{
		ptBase = _ptBase;
		fSize = _fSize;
		if ( !pUpLink )
		{
			++nLayoutChanges;
		}
	}

	int GetLayoutChanges() const
	{
		return nLayoutChanges;
	}

	float GetSize() const
	{
		return fSize;
	}

	bool Walk()
	{
		bool bRes = IsEmpty();
		for ( int i = 0; i < 8; ++i )
		{
			if ( links[i] )
			{
				if ( links[i]->Walk() )
				{
					links[i] = 0;
				}
				else
				{
					bRes = false;
				}
			}
		}
		return bRes;
	}

	virtual bool IsEmpty()
	{
		return false;
	}

	void GetBound( SSphere *pRes ) const
	{
		pRes->ptCenter.x = ptBase.x + fSize * 0.5f;
		pRes->ptCenter.y = ptBase.y + fSize * 0.5f;
		pRes->ptCenter.z = ptBase.z + fSize * 0.5f;
		pRes->fRadius = fSize * sqrt( 3.0f ) * 0.5f + GetMaxElementSize();
	}

	void GetBound( SBound *pRes ) const
	{
		float fHalfSize = fSize * 0.5;
		float fS = fHalfSize + GetMaxElementSize();
		pRes->BoxExInit( ptBase + CVec3( fHalfSize, fHalfSize, fHalfSize ), CVec3( fS, fS, fS ) );
	}

	TFinal *GetUpLink() const
	{
		return pUpLink;
	}

	TFinal *GetNode( int nIdx )
	{
		ASSERT( nIdx >= 0 && nIdx < 8 );
		return links[nIdx];
	}

	TFinal *GetNode( const CVec3 &ptCenter, float fRadius )
	{
		// Otherwise centers past the initial [-128, 896) extent get assigned to
		// edge cells whose culling bounds do not contain the geometry.
		if constexpr ( B_GROW_ROOT )
		{
			if ( !pUpLink )
			{
				GrowRootToContain( ptCenter );
			}
		}
		TFinal *pThis = static_cast<TFinal *>( this );
		float fLowerSize = 0.5f * GetMaxElementSize();
		if ( fSize <= N_MIN_NODE || fRadius >= fLowerSize )
		{
			return pThis;
		}
		else
		{
			int nIdx = 0;
			if ( ptCenter.x >= ptBase.x + fSize * 0.5f )
			{
				nIdx += 1;
			}
			if ( ptCenter.y >= ptBase.y + fSize * 0.5f )
			{
				nIdx += 2;
			}
			if ( ptCenter.z >= ptBase.z + fSize * 0.5f )
			{
				nIdx += 4;
			}
			return GetOrCreateChild( nIdx )->GetNode( ptCenter, fRadius );
		}
	}

	// GetNode, also recording in *pPlace the ranges of centre and radius for which
	// it would return the same node (SOcTreePlacement). Same comparisons on the same
	// values, in the same order, so it returns what GetNode returns.
	TFinal *GetNode( const CVec3 &ptCenter, float fRadius, SOcTreePlacement *pPlace )
	{
		const float fInf = std::numeric_limits<float>::infinity();
		for ( int j = 0; j < 3; ++j )
		{
			pPlace->lo[j] = -fInf;
			pPlace->hi[j] = fInf;
		}
		if constexpr ( B_GROW_ROOT )
		{
			if ( !pUpLink )
			{
				GrowRootToContain( ptCenter );
				// GrowRootToContain stops once this holds; outside it, it would grow.
				pPlace->lo[0] = ptBase.x;
				pPlace->lo[1] = ptBase.y;
				pPlace->lo[2] = ptBase.z;
				pPlace->hi[0] = ptBase.x + fSize;
				pPlace->hi[1] = ptBase.y + fSize;
				pPlace->hi[2] = ptBase.z + fSize;
			}
		}
		pPlace->nLayout = nLayoutChanges;
		pPlace->fRadiusMax = fInf;
		CSelf *pNode = this;
		for ( ;; )
		{
			const float fLowerSize = 0.5f * pNode->GetMaxElementSize();
			if ( pNode->fSize <= N_MIN_NODE || fRadius >= fLowerSize )
			{
				pPlace->fRadiusMin = pNode->fSize <= N_MIN_NODE ? -fInf : fLowerSize;
				return static_cast<TFinal *>( pNode );
			}
			// Descending here needs fRadius < fLowerSize.
			pPlace->fRadiusMax = (std::min)( pPlace->fRadiusMax, fLowerSize );
			const float fSplit[3] = { pNode->ptBase.x + pNode->fSize * 0.5f,
			                          pNode->ptBase.y + pNode->fSize * 0.5f,
			                          pNode->ptBase.z + pNode->fSize * 0.5f };
			const float fCenter[3] = { ptCenter.x, ptCenter.y, ptCenter.z };
			int nIdx = 0;
			for ( int j = 0; j < 3; ++j )
			{
				if ( fCenter[j] >= fSplit[j] )
				{
					nIdx += 1 << j;
					pPlace->lo[j] = (std::max)( pPlace->lo[j], fSplit[j] );
				}
				else
				{
					pPlace->hi[j] = (std::min)( pPlace->hi[j], fSplit[j] );
				}
			}
			pNode = pNode->GetOrCreateChild( nIdx );
		}
	}

private:
	TFinal *GetOrCreateChild( int nIdx )
	{
		if ( !links[nIdx] )
		{
			CVec3 ptNewBase( ptBase );
			if ( nIdx & 1 )
			{
				ptNewBase.x += fSize * 0.5f;
			}
			if ( nIdx & 2 )
			{
				ptNewBase.y += fSize * 0.5f;
			}
			if ( nIdx & 4 )
			{
				ptNewBase.z += fSize * 0.5f;
			}
			TFinal *pNewFinal = new TFinal;
			// Uplink first, so that sizing a new child does not count as a root
			// layout change.
			pNewFinal->SetUpLink( static_cast<TFinal *>( this ) );
			pNewFinal->SetSize( ptNewBase, fSize * 0.5f );
			links[nIdx] = pNewFinal;
		}
		return links[nIdx];
	}

public:
	int operator&( CStructureSaver &f ) override
	{
		f.Add( 1, &ptBase );
		f.Add( 2, &fSize );
		for ( int i = 0; i < 8; ++i )
		{
			f.Add( 10 + i, &links[i] );
		}
		f.Add( 20, &pUpLink );
		return 0;
	}
};
