#pragma once

#include "GLightPerVertex.h"
#include "GRenderCore.h"
#include "Misc/Asserts.h" // NI_UNREACHABLE

namespace NGfx
{
class CGeometry;
}
struct SDiscretePos;
struct SBoundCalcer;

namespace NGScene
{
class CObjectInfo;

enum ETransformType
{
	TT_NONE,
	TT_SIMPLE,
	TT_SINGLE_SKIN
};

//enum ELightmapType
//{
//	LT_TNL,
//	LT_TNL_SELECTION,
//	LT_NONE,
//	LT_NORMAL,
//	LT_POSITION
//};

enum ECombinerType
{
	CT_STATIC,
	CT_DYNAMIC
};

class CMMXAnimationMatrices : public CFuncBase<std::vector<NGfx::SCompactTransformer>>
{
	OBJECT_NOCOPY_METHODS( CMMXAnimationMatrices );
	ZDATA
	CDGPtr<CFuncBase<std::vector<SHMatrix>>> pAnimation;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pAnimation );
		return 0;
	}

protected:
	bool NeedUpdate() override
	{
		return pAnimation.Refresh();
	}

	void Recalc() override;

public:
	CMMXAnimationMatrices( CFuncBase<std::vector<SHMatrix>> *p = 0 )
		: pAnimation( p )
	{
	}
};

template <class T>
class CBuffer
{
	T *pBuffer;
	int nSize;

	CBuffer( const CBuffer & )
	{
		ASSERT( 0 );
	}

	void operator=( const CBuffer & )
	{
		ASSERT( 0 );
	}

public:
	CBuffer()
		: pBuffer( 0 )
		, nSize( 0 )
	{
	}

	~CBuffer()
	{
		delete[] pBuffer;
	}

	void Clear()
	{
		if ( pBuffer )
		{
			delete[] pBuffer;
		}
		pBuffer = 0;
		nSize = 0;
	}

	void Resize( int n )
	{
		if ( nSize == n )
		{
			return;
		}
		if ( n == 0 )
		{
			Clear();
		}
		else
		{
			delete[] pBuffer;
			pBuffer = new T[n];
			nSize = n;
		}
	}

	int GetSize() const
	{
		return nSize;
	}

	T *GetData() const
	{
		return pBuffer;
	}

	T &operator[]( int i )
	{
		ASSERT( i >= 0 && i < nSize );
		return pBuffer[i];
	}

	const T &operator[]( int i ) const
	{
		ASSERT( i >= 0 && i < nSize );
		return pBuffer[i];
	}
};

class CPerMaterialCombiner;
class CAnimationWatch;

class IPart : public CObjectBase
{
	ZDATA
	CPtr<CPerMaterialCombiner> pCombiner;
	CDGPtr<CPtrFuncBase<CObjectInfo>> pObjInfo;

public:
	SCacheLightingInfo cacheLighting;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pCombiner );
		f.Add( 3, &pObjInfo );
		f.Add( 4, &cacheLighting );
		return 0;
	}

public:
	std::vector<CVec3> xformedPositions;
	CBuffer<char> gfxData;
	CVec3 vBVMin, vBVMax;
	float fAverageTriArea;

	IPart()
		: fAverageTriArea( 0 )
	{
	}

	IPart( CPtrFuncBase<CObjectInfo> *pData, CPerMaterialCombiner *_pCombiner );
	~IPart() override;
	void SetCombiner( CPerMaterialCombiner *_pCombiner );

	CPerMaterialCombiner *GetCombiner() const
	{
		return pCombiner;
	}

	bool HasLoadedObjectInfo()
	{
		pObjInfo.Refresh();
		return IsValid( pObjInfo->GetValue() );
	}

	void RefreshObjectInfo();

	CObjectInfo *GetObjectInfo()
	{
		return pObjInfo->GetValue();
	}

	CPtrFuncBase<CObjectInfo> *GetObjectInfoNode() const
	{
		return pObjInfo;
	}

	void SetObjectInfoNode( CPtrFuncBase<CObjectInfo> * ); // for tests, using of this function is not recommended
	virtual ETransformType GetTransformType() const = 0;

	// One accessor per transform type, and a part has only the one its type
	// names: a TT_SIMPLE part a position, a TT_SINGLE_SKIN part an animation.
	// Every caller switches on GetTransformType before asking, so reaching a
	// body here means a caller that did not. They cannot be pure virtual
	// instead, because a concrete part implements only its own subset.
	//
	// Each said that by returning *(T *)0. Binding a reference to a
	// dereferenced null pointer is undefined whether or not the caller reads
	// what comes back, and clang reports it. NI_UNREACHABLE says the same thing
	// to the compiler without inventing a value for a question that has none.
	virtual const SFBTransform &GetSimplePos()
	{
		NI_UNREACHABLE();
	}

	virtual const std::vector<SHMatrix> &GetAnimation()
	{
		NI_UNREACHABLE();
	}

	virtual const std::vector<NGfx::SCompactTransformer> &GetMMXAnimation()
	{
		NI_UNREACHABLE();
	}

	virtual bool Is2Sided() const
	{
		return false;
	}

	virtual intptr_t GetSortValue() const
	{
		return 0;
	}

	virtual void AddChangeTrackers( CAnimationWatch *p, bool bVertices ) {}

	void ResetCachedTransform(); // for CVBCombiner only

	void ResetCachedLighting()
	{
		gfxData.Clear();
	} // for CVBCombiner only
};

void TransformPart( IPart *p, std::vector<CVec3> *pRes, std::vector<STriangle> *pTris );

// CPerMaterialCombiner

class CAnimationWatch : public CVersioningBase
{
	typedef CFuncBase<std::vector<CPtr<IPart>>> TMaterialCombiner;
	typedef std::vector<CDGPtr<CVersioningBase>> TWatchSet;
	OBJECT_NOCOPY_METHODS( CAnimationWatch );
	ZDATA
	CDGPtr<TMaterialCombiner, CPtr<TMaterialCombiner>> pCombiner;
	TWatchSet watch;
	std::vector<int> indices;
	bool bAnimationOnly;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pCombiner );
		f.Add( 3, &watch );
		f.Add( 4, &indices );
		f.Add( 5, &bAnimationOnly );
		return 0;
	}

	bool NeedUpdate() override;

	void Recalc() override {}

public:
	CAnimationWatch() {}

	CAnimationWatch( TMaterialCombiner *_p, bool _bAnimationOnly )
		: pCombiner( _p )
		, bAnimationOnly( _bAnimationOnly )
	{
	}

	void AddHandle( CVersioningBase *p )
	{
		watch.push_back( p );
	}
};

class CPerMaterialCombiner : public CFuncBase<std::vector<CPtr<IPart>>>
{
	OBJECT_NOCOPY_METHODS( CPerMaterialCombiner );
	bool bHasChanged;
	CObj<CAnimationWatch> pAnimation, pFullChange;

	bool NeedUpdate() override;
	void Recalc() override;

public:
	CPerMaterialCombiner()
		: bHasChanged( false )
	{
	}

	CPerMaterialCombiner( int );
	// for use with IPart like objects only
	void AddPart( IPart *pPart );
	void RemovePart( IPart *pPart );
	void MarkWasted( IPart *pPart );

	CVersioningBase *GetAnimationTracker() const
	{
		return pAnimation;
	}

	CVersioningBase *GetFullChangeTracker() const
	{
		return pFullChange;
	}

	int GetSize() const
	{
		return value.size();
	}

	int operator&( CStructureSaver &f ) override;
};

// CVBCombiner

class CVBCombiner : public IVBCombiner
{
	OBJECT_BASIC_METHODS( CVBCombiner );
	ZDATA_( IVBCombiner )
	CDGPtr<CFuncBase<std::vector<CPtr<IPart>>>> pCombiner;
	ECombinerType ct;
	CDGPtr<CVersioningBase> pAnimation;
	CDGPtr<CFuncBase<SPerVertexLightState>> pLightState;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (IVBCombiner *)this );
		f.Add( 2, &pCombiner );
		f.Add( 3, &ct );
		f.Add( 4, &pAnimation );
		f.Add( 5, &pLightState );
		return 0;
	}

	// NeedXForm reads bNeedXForm to or a fresh result into it, and the default
	// constructor set none of the three, so the first call read whatever the
	// allocation held: UBSan reports a bool loaded as 190, which is the fill an
	// ASan allocation arrives with.
	bool bNeedXForm = false, bNeedRecalc = false, bDroppedXForm = false;
	template <class TTrans>
	void SimpleTransform( TTrans *p );

protected:
	void XFormPosition();

	bool RealNeedUpdate()
	{
		if ( pAnimation )
		{
			return pAnimation.Refresh() | pCombiner.Refresh();
		}
		return pCombiner.Refresh();
	}

	bool NeedXForm()
	{
		bool bRes = RealNeedUpdate();
		bNeedXForm |= bRes;
		return bNeedXForm;
	}

	bool NeedUpdate() override;
	void DoRecalc();
	void Recalc() override;

public:
	CVBCombiner() {}

	CVBCombiner( CFuncBase<std::vector<CPtr<IPart>>> *_pCombiner,
	             ECombinerType _ct,
	             CVersioningBase *_pAnimation,
	             CFuncBase<SPerVertexLightState> *_pLightState )
		: pCombiner( _pCombiner )
		, ct( _ct )
		, pAnimation( _pAnimation )
		, pLightState( _pLightState )
	{
	}

	const SBound &GetBound() override
	{
		if ( NeedXForm() )
		{
			XFormPosition();
		}
		return bound;
	}

	const std::vector<SSphere> &GetBounds() override
	{
		if ( NeedXForm() )
		{
			XFormPosition();
		}
		return partBVs;
	}

	CFuncBase<std::vector<CPtr<IPart>>> *GetCombiner() const override
	{
		return pCombiner;
	}

	void FreeMemory() override;
};

// CIBCombiner

enum EIBTargetType
{
	IBTT_POSITIONS,
	IBTT_VERTICES,
};

class CIBCombiner : public CFuncBase<std::vector<NGfx::STriangleList>>
{
	OBJECT_BASIC_METHODS( CIBCombiner );
	ZDATA
	CDGPtr<CFuncBase<std::vector<CPtr<IPart>>>> pCombiner;
	CDGPtr<CVersioningBase> pAnimation;
	EIBTargetType ibt;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pCombiner );
		f.Add( 3, &pAnimation );
		f.Add( 4, &ibt );
		return 0;
	}

	std::vector<STriangle> triBuffer;

protected:
	bool NeedUpdate() override
	{
		if ( pAnimation )
		{
			return pAnimation.Refresh() | pCombiner.Refresh();
		}
		return pCombiner.Refresh();
	}

	void Recalc() override;

public:
	CIBCombiner() {}

	CIBCombiner( CFuncBase<std::vector<CPtr<IPart>>> *_pCombiner, CVersioningBase *_pAnimation, EIBTargetType _ibt )
		: pCombiner( _pCombiner )
		, pAnimation( _pAnimation )
		, ibt( _ibt )
	{
	}
};

} // namespace NGScene
