#pragma once

#include "GRenderCore.h"
#include "GLightPerVertex.h"

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

class CMMXAnimationMatrices : public CFuncBase<vector<NGfx::SCompactTransformer> >
{
	OBJECT_NOCOPY_METHODS(CMMXAnimationMatrices);
	ZDATA
	CDGPtr<CFuncBase< vector<SHMatrix> > > pAnimation;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pAnimation); return 0; }
protected:
	virtual bool NeedUpdate() { return pAnimation.Refresh(); }
	virtual void Recalc();
public:
	CMMXAnimationMatrices( CFuncBase< vector<SHMatrix> > *p = 0 ) : pAnimation(p) {}
};

template<class T>
class CBuffer
{
	T *pBuffer;
	int nSize;
	CBuffer(const CBuffer& ) {ASSERT(0);}
	void operator=(const CBuffer&) {ASSERT(0);}
public:
	CBuffer() : pBuffer(0), nSize(0) {}
	~CBuffer() { delete[] pBuffer; }
	void Clear() { if ( pBuffer ) delete[] pBuffer; pBuffer = 0; nSize = 0; }
	void Resize( int n ) { if ( nSize == n ) return; if( n == 0 ) Clear(); else { delete[] pBuffer; pBuffer = new T[n]; nSize = n; } }
	int GetSize() const { return nSize; }
	T* GetData() const { return pBuffer; }
	T& operator[]( int i ) { ASSERT( i >= 0 && i < nSize ); return pBuffer[i]; }
	const T& operator[]( int i ) const { ASSERT( i >= 0 && i < nSize ); return pBuffer[i]; }
};

class CPerMaterialCombiner;
class CAnimationWatch;
class IPart: public CObjectBase
{
	ZDATA
	CPtr<CPerMaterialCombiner> pCombiner;
	CDGPtr< CPtrFuncBase<CObjectInfo> > pObjInfo;
public:
	SCacheLightingInfo cacheLighting;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pCombiner); f.Add(3,&pObjInfo); f.Add(4,&cacheLighting); return 0; }
public:
	vector<CVec3> xformedPositions;
	CBuffer<char> gfxData;
	CVec3 vBVMin, vBVMax;
	float fAverageTriArea;

	IPart() : fAverageTriArea(0) {}
	IPart( CPtrFuncBase<CObjectInfo> *pData, CPerMaterialCombiner *_pCombiner );
	~IPart();
	void SetCombiner( CPerMaterialCombiner *_pCombiner );
	CPerMaterialCombiner *GetCombiner() const { return pCombiner; }
	bool HasLoadedObjectInfo() { pObjInfo.Refresh(); return IsValid( pObjInfo->GetValue() ); }
	void RefreshObjectInfo();
	CObjectInfo* GetObjectInfo() { return pObjInfo->GetValue(); }
	CPtrFuncBase<CObjectInfo>* GetObjectInfoNode() const { return pObjInfo; }
	void SetObjectInfoNode( CPtrFuncBase<CObjectInfo>* ); // for tests, using of this function is not recommended
	virtual ETransformType GetTransformType() const = 0;
	virtual const SFBTransform& GetSimplePos() { return *(SFBTransform*)0; }
	virtual const vector<SHMatrix>& GetAnimation() { return *(vector<SHMatrix>*)0; }
	virtual const vector<NGfx::SCompactTransformer>& GetMMXAnimation() { return *(vector<NGfx::SCompactTransformer>*)0; }
	virtual bool Is2Sided() const { return false; }
	virtual int GetSortValue() const { return 0; }
	virtual void AddChangeTrackers( CAnimationWatch *p, bool bVertices ) {}
	void ResetCachedTransform(); // for CVBCombiner only
	void ResetCachedLighting() { gfxData.Clear(); } // for CVBCombiner only
};

void TransformPart( IPart *p, vector<CVec3> *pRes, vector<STriangle> *pTris );

// CPerMaterialCombiner

class CAnimationWatch : public CVersioningBase
{
	typedef CFuncBase< vector< CPtr<IPart> > > TMaterialCombiner;
	typedef vector<CDGPtr<CVersioningBase> > TWatchSet;
	OBJECT_NOCOPY_METHODS(CAnimationWatch);
	ZDATA
	CDGPtr<TMaterialCombiner, CPtr<TMaterialCombiner> > pCombiner;
	TWatchSet watch;
	vector<int> indices;
	bool bAnimationOnly;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pCombiner); f.Add(3,&watch); f.Add(4,&indices); f.Add(5,&bAnimationOnly); return 0; }
	bool NeedUpdate();
	void Recalc() {}
public:
	CAnimationWatch() {}
	CAnimationWatch( TMaterialCombiner *_p, bool _bAnimationOnly ) : pCombiner(_p), bAnimationOnly(_bAnimationOnly) {}
	void AddHandle( CVersioningBase *p ) { watch.push_back( p ); }
};

class CPerMaterialCombiner: public CFuncBase< vector< CPtr<IPart> > >
{
	OBJECT_NOCOPY_METHODS(CPerMaterialCombiner);
	bool bHasChanged;
	CObj<CAnimationWatch> pAnimation, pFullChange;

	virtual bool NeedUpdate();
	virtual void Recalc();
public:
	CPerMaterialCombiner() : bHasChanged(false) {}
	CPerMaterialCombiner( int );
	// for use with IPart like objects only
	void AddPart( IPart *pPart );
	void RemovePart( IPart *pPart );
	void MarkWasted( IPart *pPart );
	CVersioningBase *GetAnimationTracker() const { return pAnimation; }
	CVersioningBase *GetFullChangeTracker() const { return pFullChange; }
	int GetSize() const { return value.size(); }
	int operator&( CStructureSaver &f );
};

// CVBCombiner

class CVBCombiner: public IVBCombiner
{
	OBJECT_BASIC_METHODS(CVBCombiner);
	ZDATA_(IVBCombiner)
	CDGPtr< CFuncBase< vector< CPtr<IPart> > > > pCombiner;
	ECombinerType ct;
	CDGPtr<CVersioningBase> pAnimation;
	CDGPtr<CFuncBase<SPerVertexLightState> > pLightState;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(IVBCombiner*)this); f.Add(2,&pCombiner); f.Add(3,&ct); f.Add(4,&pAnimation); f.Add(5,&pLightState); return 0; }
	bool bNeedXForm, bNeedRecalc, bDroppedXForm;
	template<class TTrans>
		void SimpleTransform( TTrans *p );
protected:
	void XFormPosition();
	bool RealNeedUpdate() { if ( pAnimation ) return pAnimation.Refresh() | pCombiner.Refresh(); return pCombiner.Refresh(); }
	bool NeedXForm() { bool bRes = RealNeedUpdate(); bNeedXForm |= bRes; return bNeedXForm; }
	virtual bool NeedUpdate();
	void DoRecalc();
	virtual void Recalc();
public:
	CVBCombiner() {}
	CVBCombiner( CFuncBase< vector< CPtr<IPart> > > *_pCombiner, ECombinerType _ct, CVersioningBase *_pAnimation, CFuncBase<SPerVertexLightState> *_pLightState )
		: pCombiner(_pCombiner), ct(_ct), pAnimation(_pAnimation), pLightState(_pLightState) {}
	virtual const SBound& GetBound() { if ( NeedXForm() ) XFormPosition(); return bound; }
	virtual const vector<SSphere>& GetBounds() { if ( NeedXForm() ) XFormPosition(); return partBVs; }
	virtual CFuncBase<vector< CPtr<IPart> > > * GetCombiner() const { return pCombiner; }
	virtual void FreeMemory();
};

// CIBCombiner

enum EIBTargetType
{
	IBTT_POSITIONS,
	IBTT_VERTICES,
};
class CIBCombiner : public CFuncBase<vector<NGfx::STriangleList> >
{
	OBJECT_BASIC_METHODS(CIBCombiner);
	ZDATA
	CDGPtr< CFuncBase< vector< CPtr<IPart> > > > pCombiner;
	CDGPtr<CVersioningBase> pAnimation;
	EIBTargetType ibt;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pCombiner); f.Add(3,&pAnimation); f.Add(4,&ibt); return 0; }
	vector<STriangle> triBuffer;
protected:
	virtual bool NeedUpdate() {  if ( pAnimation ) return pAnimation.Refresh() | pCombiner.Refresh(); return pCombiner.Refresh(); }
	virtual void Recalc();
public:
	CIBCombiner() {}
	CIBCombiner( CFuncBase< vector< CPtr<IPart> > > *_pCombiner, CVersioningBase *_pAnimation, EIBTargetType _ibt )
		: pCombiner(_pCombiner), pAnimation(_pAnimation), ibt(_ibt) {}
};

}

