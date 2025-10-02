#pragma once
#include "../System/DG.h"
#include "../System/Time.hpp"
#include "../Misc/Pool.h"
#include "GPixelFormat.h"
#include "GShadowMap.h"
#include "GMaterial.hpp"
#include "../3dlib/Bound.h"

inline CVec3 MulPerComp4( const CVec3 &a, const CVec3 &b ) { return CVec3(a.x*b.x*4, a.y*b.y*4, a.z*b.z*4); }
inline CVec3 MulPerComp2( const CVec3 &a, const CVec3 &b ) { return CVec3(a.x*b.x*2, a.y*b.y*2, a.z*b.z*2); }
inline CVec3 MulPerComp( const CVec3 &a, const CVec3 &b ) { return CVec3(a.x*b.x, a.y*b.y, a.z*b.z); }
inline CVec4 MulPerComp( const CVec4 &a, const CVec4 &b ) { return CVec4(a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w); }

class CTransformStack;
namespace NGfx
{
	class CRenderContext;
	class CGeometry;
	class CTexture;
	class CCubeTexture;
	struct SEffect;
}

namespace NGScene
{
class IPart;
class CSceneFragments;
class IMaterial;
class CTransparentRenderer;

//const TPartFlags PF_ALL_PARTS = 0xffffffff;
const int N_BLOCKS_IN_PART_FLAGS = 8;//16;
const int PF_MAX_PARTS_PER_COMBINER = N_BLOCKS_IN_PART_FLAGS * 32;
class CPartFlags
{
	int flags[N_BLOCKS_IN_PART_FLAGS];

	void Fill( int n ) { memset( flags, n, sizeof(flags) ); }
public:
	void Clear() { Fill(0); }
	void TakeAll() { Fill( 0xffffffff ); }
	void Set( int nIndex ) 
	{ 
		ASSERT( nIndex >= 0 && nIndex < PF_MAX_PARTS_PER_COMBINER );
		flags[ nIndex / 32 ] |= 1 << ( nIndex & 31 ); 
	}
	void Reset( int nIndex ) 
	{ 
		ASSERT( nIndex >= 0 && nIndex < PF_MAX_PARTS_PER_COMBINER );
		flags[ nIndex / 32 ] &= ~( 1 << ( nIndex & 31 ) );
	}
	int IsSet( int nIndex ) const 
	{ 
		ASSERT( nIndex >= 0 && nIndex < PF_MAX_PARTS_PER_COMBINER ); 
		return flags[ nIndex / 32 ] & ( 1 << nIndex ); 
	}
	bool IsEmpty() const { for ( int k = 0; k < GetBlocksNumber(); ++k ) { if ( flags[k] ) return false; } return true; }
	bool IsFullGet() const { for ( int k = 0; k < GetBlocksNumber(); ++k ) { if ( flags[k] != 0xffffffff ) return false; } return true; }
	void Invert() { for ( int k = 0; k < GetBlocksNumber(); ++k ) flags[k] = ~flags[k]; }
	int GetBlocksNumber() const { return N_BLOCKS_IN_PART_FLAGS; }
	int GetBlock( int n ) const { ASSERT( n >= 0 && n < GetBlocksNumber() ); return flags[n]; }
	void SetBlock( int nIndex, int n ) { ASSERT( nIndex >= 0 && nIndex < GetBlocksNumber() ); flags[nIndex] |= n; }
	void CalcAnd( const CPartFlags &a )
	{
		ASSERT( GetBlocksNumber() == a.GetBlocksNumber() );
		for ( int k = 0; k < GetBlocksNumber(); ++k )
			flags[k] &= a.flags[k];
	}
	void CalcOr( const CPartFlags &a )
	{
		ASSERT( GetBlocksNumber() == a.GetBlocksNumber() );
		for ( int k = 0; k < GetBlocksNumber(); ++k )
			flags[k] |= a.flags[k];
	}
	CPartFlags& operator|=( const CPartFlags &a ) { CalcOr(a); return *this; }
	CPartFlags& operator&=( const CPartFlags &a ) { CalcAnd(a); return *this; }
};

inline bool operator==( const CPartFlags &a, const CPartFlags &b ) { return memcmp( &a, &b, sizeof(CPartFlags) ) == 0; }
inline bool operator!=( const CPartFlags &a, const CPartFlags &b ) { return memcmp( &a, &b, sizeof(CPartFlags) ) != 0; }
inline CPartFlags operator~( const CPartFlags &f ) { CPartFlags res( f ); res.Invert(); return res; }
inline CPartFlags operator&( const CPartFlags &a, const CPartFlags &b ) { CPartFlags r(a); r &= b; return r; }
inline CPartFlags operator|( const CPartFlags &a, const CPartFlags &b ) { CPartFlags r(a); r |= b; return r; }
inline CPartFlags TakeAllParts() { CPartFlags r; r.TakeAll(); return r; }

struct SRenderGeometryInfo;
struct SRenderPartSet
{
	CPtr<CObjectBase> pNode;
	const vector< CPtr<IPart> > *pParts;
	CPartFlags parts, opaque;
	SRenderGeometryInfo *pGeometry;
	int nFloorMask;

	SRenderPartSet() {}
	SRenderPartSet( CObjectBase *_pNode, const vector< CPtr<IPart> > *_pParts, SRenderGeometryInfo *_pGeometry, int _nFloorMask )
		: pNode(_pNode), pParts(_pParts), pGeometry(_pGeometry), nFloorMask(_nFloorMask)
	{ 
		parts.Clear();
	}
	//SRenderPartSet( CObjectBase *_pNode ): pNode(_pNode) {}
	IPart* GetPart( int nIndex ) const { return (*pParts)[ nIndex ]; }
};

struct SGroupSelect;
struct SPerVertexLightState;
class IRender
{
public:
	enum EDepthType
	{
		DT_STATIC,
		DT_DYNAMIC,
		DT_ALL
	};
	
	virtual CFuncBase<SPerVertexLightState> *GetLightState() const = 0;
	virtual void FormPartList( CTransformStack *pTS, list<SRenderPartSet> *pRes, EDepthType dt, const SGroupSelect &mask ) = 0;
	virtual void FormDepthList( CTransformStack *pTS, const CVec3 &vDir, EDepthType dt, CSceneFragments *pRes ) = 0;
	virtual void FormRenderList( CTransformStack *pTS, CSceneFragments *pRes, CTransparentRenderer *pTransparentRender ) = 0;
	virtual void GetNotLoaded( vector<IPart*> *pRes ) = 0;
	virtual void RenderPostProcess( CTransformStack *pTS, NGfx::CRenderContext *pRC ) = 0;
	virtual CTransparentRenderer *CreateTransparentRenderer( CTransformStack *pTS, bool bLitParticles ) = 0;
};

// node that can render something somewhere
enum ETrilistType
{
	TLT_POSITION,
	TLT_GEOM,
	TLT_NUMBER
};
class IVBCombiner : public CPtrFuncBase<NGfx::CGeometry>
{
protected:
	ZDATA
	SBound bound;
	vector<SSphere> partBVs;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&bound); f.Add(3,&partBVs); return 0; }
	virtual const SBound& GetBound() { ASSERT( IsFrameMatch() ); return bound; }
	virtual const vector<SSphere>& GetBounds() { ASSERT( IsFrameMatch() ); return partBVs; }
	int GetPartsNum() const { return partBVs.size(); }
	virtual CFuncBase<vector< CPtr<IPart> > > * GetCombiner() const { return 0; }
	virtual void FreeMemory() {}
	bool IsValidValue() const { return IsValid( pValue ); }
};
struct SRenderGeometryInfo
{
	CDGPtr<IVBCombiner> pVertices;
	CDGPtr< CFuncBase<vector<NGfx::STriangleList> > > pTriLists[TLT_NUMBER];

	int operator&( CStructureSaver &f ) 
	{ 
		f.Add(1,&pVertices); 
		for ( int k = 0; k < TLT_NUMBER; ++k )
			f.Add(2,&pTriLists[k], k + 1 );
		return 0;
	}
};

//! intermediate per fragment operations representation
enum EStencilBlendingOp
{
	//	STM_TEST = 1,
	STM_NONE = 0,
	STM_LIGHT = 1, // set 0x80 bit
	STM_INCREMENT = 2,
	///STM_STENCIL_LIGHT = 2,
	//STM_TEST_STENCIL_LIGHT = 3,
	STM_MARK = 4,
	STM_TEST_CLEAR_MARK = 5, // test highest bit and clear
	STM_MARK_2 = 6, // set 0x40
	STM_TEST_MARK_2 = 7,
	STM_MASK = 7,

	DPM_NORMAL   = 0,
	DPM_EQUAL    = 8,
	DPM_TESTONLY = 16,
	DPM_NONE     = 24,
	DPM_NORMAL_NOTEQ = 32,
	DPM_MASK     = 56,

	ABM_NONE     = 0,
	ABM_ZERO     = 64,
	ABM_ALPHA_ADD = 128,
	ABM_MUL      = 192,
	ABM_SRC_AMUL = 256,
	ABM_ALPHA_BLEND = 320,
	ABM_SMART = 384,
	ABM_ADD_SRC_AMUL = 448,
	ABM_MASK     = 448,

	FOG_NONE = 0,
	FOG_NORMAL = 512,
	FOG_BLACK = 1024,
	FOG_MASK = 1536,

	SHADOW_CULL_CCW = 2048,
};

struct SDirectionalDepthInfo
{
	CVec4 vDepth;
	CVec4 vVecU, vVecV;
};

struct SPerspDirectionalDepthInfo
{
	CVec4 vDepth;
	SNLProjectionInfo nlp;
	SPerspDirectionalDepthInfo() : vDepth(0,0,0,1) {}
};

struct SSkyDepth3Info
{
	const SDirectionalDepthInfo *channels[3];
	CVec3 vDirs[3];
	int nResolution;
	NGfx::CTexture *pAdd, *pDepth;
};

struct SFillDiffuseBumpSpecInfo
{
	NGfx::CTexture *pDiffuse, *pBump, *pSpecular;
	CVec4 vSpecularColor, vDiffuseColor;
	float fAlphaTest;
};

struct SPPSpecularInfo
{
	NGfx::CTexture *pBump, *pSpecular;
	CVec3 vSpecularColor;
	float fBumpMult;
};

struct SFastGf3RenderInfo
{
	NGfx::CTexture *pTexture, *pDepth, *pTexture2, *pSpecular, *pLM;
	CVec4 vColor;
	const SPerspDirectionalDepthInfo *pPersp;
	float fSpecPower, fSecondUVMult;
	unsigned char nAlphaTest;
	bool bPreferPerVertexTransp, bTranslucent;
	STime tTime;
};

struct SPntLightShadowedInfo
{
	NGfx::CTexture *pAdd;
	NGfx::CCubeTexture *pDepth;
	float fResolution;
};

enum ERenderOperation
{
	RO_NOP,
	//
	// TnL path
	RO_TNL_SOLID_COLOR,
	RO_TNL_LIT_TEXTURE,
	RO_TNL_TEXTURE,
	RO_TNL_SLIDING_TEXTURE,
	// general ops
	RO_SOLID_COLOR,
	RO_TEXTURE_AT,
	RO_TEXTURE_AT_NLP,
	// directional
	RO_DIR_PARTICLE_LM_SOFT_SHADOW_TEST,
	RO_DIR_DEPTH,
	RO_DIR_DEPTH_AT_NLP,
	RO_DIR_DEPTH_16,
	RO_DIR_DEPTH_16_AT_NLP,
	RO_DIR_COLOR,
	RO_LP_DEPTH,
	RO_G3_SHOW_LM,
	// lightmaps
	RO_CL_SKY_3LIGHT,
	RO_CL_SKY_3LIGHT_TRANSLUCENT,
	RO_CL_CUBEMAP_DEPTH,
	RO_CL_PNT_LIGHT_SHADOWED,
	RO_WRITE_Z,
	// user specific
	RO_USER = 100
};

struct SRenderStaticInfo
{
	CPtr<CObjectBase> pHandle;
	SBound bv;
};

typedef unsigned char TPartPriority;
struct SPerPartVariables
{
	ZDATA
	float fFade;
	TPartPriority nPriority;
	CObj<CPtrFuncBase<NGfx::CTexture> > pLM;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&fFade); f.Add(3,&nPriority); f.Add(4,&pLM); return 0; }

	SPerPartVariables() : fFade(1), nPriority(0) {}
	int GetSortValue() const { return (*(int*)&fFade) ^ nPriority ^ ((int)pLM.GetPtr()); }
	bool operator == ( const SPerPartVariables &a ) const { return fFade == a.fFade && nPriority == a.nPriority && pLM == a.pLM; }
};

struct SRenderFragmentInfo
{
	struct SElement
	{
		short nGeometry, nBlock;
		int nFlags;

		SElement() {}
		SElement( short _nGeom, short _nBlock, int _nFlags ) : nGeometry(_nGeom), nBlock(_nBlock), nFlags(_nFlags) {}
	};
	vector<SElement> elements;
	CPtr<IMaterial> pMaterial;
	SPerPartVariables vars;

	SRenderFragmentInfo() {}//: pLM(0) {}
};
struct SRenderFragmentKey
{
	IMaterial *pMat;
	float fFade;
	int nPriority;
	CPtrFuncBase<NGfx::CTexture> *pLM;
	bool operator==( const SRenderFragmentKey &a ) const { return pMat == a.pMat && fFade == a.fFade && nPriority == a.nPriority && pLM == a.pLM; }
};
struct SRenderFragmentHash
{
	int operator()( const SRenderFragmentKey &a ) const 
	{
		ASSERT( sizeof(SRenderFragmentKey) == sizeof(int) * 4 );
		int *p = (int*)&a;
		return p[0] + p[1] + p[2] + p[3];
	}
};

enum EFragmentsSplit
{
	FST_ACCEPT,
	FST_REJECT,
	FST_SPLIT
};
class CSceneFragments
{
private:
	int nSceneTris;
	CPool<SRenderStaticInfo,128> staticInfos;
	CPool<SRenderGeometryInfo,128> geometryInfos;
	CPool<SRenderFragmentInfo,128> fragmentInfos;
	
	typedef hash_map<SRenderFragmentKey, int, SRenderFragmentHash> CFragmentHash;
	CFragmentHash fragmentHash;
	vector<SRenderStaticInfo*> statics;
	vector<SRenderGeometryInfo*> geometries;
	vector<SRenderFragmentInfo*> fragments;
	SBoundCalcer bc;
	vector<char> filterFragment;
	vector<char> filterGeometry;
	vector<CPartFlags> selectedParts;
	bool bNeedHSR;
public:
	CSceneFragments();
	int AddGeometry( CObjectBase *pHandle, SRenderGeometryInfo *pGeometry, const SBound &_bv, bool bNotAddBound );
	void AddElement( int nGeometryIndex, const CPartFlags &_parts, IMaterial *pMaterial, const SPerPartVariables &_vars );
	void AddLitParticles( IVBCombiner *pCombiner, CFuncBase<vector<NGfx::STriangleList> > *pTris, int nPart, const SBound &_bv );
	void SetLitParticlesMaterial( IMaterial *p );
	
	int GetSceneTris() const { return nSceneTris; }
	// starts with 1st element, 0th is lit particles
	bool IsFilteredFragment( unsigned int n ) const { if ( n >= filterFragment.size() ) return false; return filterFragment[n]; }
	EFragmentsSplit GetGeometryFlags( unsigned int n ) const { if ( n >= filterGeometry.size() ) return FST_ACCEPT; return (EFragmentsSplit)filterGeometry[n]; }
	const CPartFlags& GetGeometryParts( unsigned int n ) const { return selectedParts[n]; }
	bool HasSelectedFragments() const;
	const vector<SRenderFragmentInfo*>& GetFragments() const { return fragments; }
	const SRenderFragmentInfo& GetLitParticles() const { return *fragments[0]; }
	const SRenderStaticInfo& GetStaticInfo( int nGeom ) const { return *statics[ nGeom ]; }
	SRenderGeometryInfo* GetGeometryInfo( int nGeom ) const { return geometries[ nGeom ]; }
	int GetGeometriesNum() const { ASSERT( statics.size() == geometries.size() ); return geometries.size(); }
	void HideGeometry( const vector<CPartFlags> &flags );
	void SetNeedHSR( bool _b ) { bNeedHSR = _b; }
	bool NeedHSR() const { return bNeedHSR; }
	void AddGeomBound( const SBound &_b ) { bc.Add( _b ); }
	void GetBound( SBound *pRes ) { bc.Make( pRes ); }
	friend class CSelectFragments;
	friend class CSelectGeometries;
};

class CSelectGeometries
{
	CSceneFragments *pScene;
	vector<char> holdFlags;
	vector<CPartFlags> holdParts;
public:
	template<class T>
		CSelectGeometries( CSceneFragments *_pScene, const T &select ) : pScene(_pScene) 
	{
		holdFlags = pScene->filterGeometry;
		holdParts = pScene->selectedParts;
		if ( holdFlags.empty() )
		{
			pScene->filterGeometry.resize( pScene->geometries.size(), 0 );
			CPartFlags f;
			f.TakeAll();
			pScene->selectedParts.resize( pScene->geometries.size(), f );
		}
		for ( int k = 0; k < pScene->filterGeometry.size(); ++k )
		{
			if ( pScene->filterGeometry[k] == FST_REJECT )
				continue;
			CPartFlags &parts = pScene->selectedParts[k];
			EFragmentsSplit res = select( pScene->statics[k], pScene->geometries[k], &parts );
			if ( res == FST_SPLIT && parts.IsEmpty() )
				res = FST_REJECT;
			pScene->filterGeometry[k] = res;
		}
	}
	~CSelectGeometries() { pScene->filterGeometry = holdFlags; pScene->selectedParts = holdParts; }
};

class CSelectFragments
{
	CSceneFragments *pScene;
	vector<char> holdFilter;
public:
	template<class T>
		CSelectFragments( CSceneFragments *_pScene, const T &select ) : pScene(_pScene) 
	{
		holdFilter = pScene->filterFragment;
		if ( holdFilter.empty() )
			pScene->filterFragment.resize( pScene->fragments.size(), 0 );
		for ( int k = 0; k < pScene->filterFragment.size(); ++k )
		{
			if ( pScene->filterFragment[k] )
				continue;
			pScene->filterFragment[k] = select( pScene->fragments[k] );
		}
	}
	~CSelectFragments() { pScene->filterFragment = holdFilter; }
};

struct SBoundIntersectFilter
{
	SBound bv;
	SBoundIntersectFilter( const SBound &_bv ) : bv(_bv) {}
	EFragmentsSplit operator()( SRenderStaticInfo *pStatic, SRenderGeometryInfo *pGeom, CPartFlags *pRes ) const;
};
struct SFrustrumFilter
{
	CTransformStack *pTS;
	SFrustrumFilter( CTransformStack *_pTS ) : pTS(_pTS) {}
	EFragmentsSplit operator()( SRenderStaticInfo *pStatic, SRenderGeometryInfo *pGeom, CPartFlags *pRes ) const;
};
struct SSphereFilter
{
	SBound bound;
	SSphere sph;
	SSphereFilter( const SSphere &_sph ) : sph(_sph) { bound.SphereInit( sph.ptCenter, sph.fRadius ); }
	EFragmentsSplit operator()( SRenderStaticInfo *pStatic, SRenderGeometryInfo *pGeom, CPartFlags *pRes ) const;
};
struct SIgnorePartsInfo
{
	// bit set to 1 means part is ignored
	ZDATA
	CPartFlags flags;
	CDGPtr<CFuncBase<vector<CPtr<IPart> > > > pTrackCombiner;
	vector<CPtr<IPart> > ignore;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&flags); f.Add(3,&pTrackCombiner); f.Add(4,&ignore); return 0; }
	void Init( const CPartFlags &_flags, const vector<CPtr<IPart> > *pParts );
};
typedef hash_map<CPtr<CObjectBase>,SIgnorePartsInfo,SPtrHash> CFullIgnorePartsHash;
struct SIgnoredSphereFilter
{
	CFullIgnorePartsHash *pIgnoreList;
	SSphereFilter sph;
	SIgnoredSphereFilter( CFullIgnorePartsHash *_pIgnoreList, const SSphere &_sph ) : pIgnoreList(_pIgnoreList), sph(_sph) {}
	EFragmentsSplit operator()( SRenderStaticInfo *pStatic, SRenderGeometryInfo *pGeom, CPartFlags *pRes ) const;
};

class CRenderCmdList
{
public:
	union UParameter
	{
		const CVec3 *pVec3;
		const CVec4 *pVec4;
		NGfx::CTexture *pTex;
		NGfx::CCubeTexture *pCubeTex;
		const SDirectionalDepthInfo *pDirDepth;
		const SPerspDirectionalDepthInfo *pPDirDepth;
		const SSkyDepth3Info *pSkyDepth3;
		const SFillDiffuseBumpSpecInfo *pFill;
		const SPPSpecularInfo *pPPSpec;
		const SFastGf3RenderInfo *pFastInfo;
		const SPntLightShadowedInfo *pPSInfo;
		IMaterial *pMaterial;
		float f;
		int nData;

		UParameter() : f(0) {}
		UParameter( const CVec3 *v3 ): pVec3(v3) {}
		UParameter( const CVec4 *v4 ): pVec4(v4) {}
		UParameter( NGfx::CTexture *_pTex ): pTex(_pTex) {}
		UParameter( NGfx::CCubeTexture *_pTex ): pCubeTex(_pTex) {}
		UParameter( const SDirectionalDepthInfo *_pDirDepth ): pDirDepth(_pDirDepth) {}
		UParameter( const SPerspDirectionalDepthInfo *_pDirDepth ): pPDirDepth(_pDirDepth) {}
		UParameter( const SSkyDepth3Info *_p ): pSkyDepth3(_p) {}
		UParameter( const SFillDiffuseBumpSpecInfo *_pFill ): pFill(_pFill) {}
		UParameter( const SPPSpecularInfo *_pPPSpec ): pPPSpec(_pPPSpec) {}
		UParameter( const SFastGf3RenderInfo *_pFastInfo ): pFastInfo(_pFastInfo) {}
		UParameter( const SPntLightShadowedInfo *_pInfo ): pPSInfo(_pInfo) {}
		UParameter( IMaterial *_pMaterial ): pMaterial(_pMaterial) {}
		UParameter( float _f ): f(_f) {}
	};
	struct SOperation
	{
		const SRenderFragmentInfo *pFrag;
		unsigned char op;
		unsigned char nPass;
		char nDestRegister;
		TPartPriority nPartPriority;
		int nStencilBlendMode; // stencil mode & blend mode
		UParameter p1, p2, p3;

		SOperation() {}
		SOperation( const SRenderFragmentInfo *_pFrag, unsigned char _op, TPartPriority _nPartPriority, unsigned char _nPass, 
			int _nSBM, unsigned char _nRegister, UParameter _p1 = UParameter(float(0)), UParameter _p2 = UParameter(float(0)), UParameter _p3 = UParameter(float(0)) )
			: pFrag(_pFrag), op(_op), nPass(_nPass), nStencilBlendMode(_nSBM), nDestRegister(_nRegister),
			nPartPriority(_nPartPriority),
			p1(_p1), p2(_p2), p3(_p3) {}
		bool IsSame( const SOperation &a ) const
		{
			ASSERT( sizeof(p1.f) == sizeof(p1) );
			return op == a.op && nPass == a.nPass && nStencilBlendMode == a.nStencilBlendMode && 
				nDestRegister == a.nDestRegister && nPartPriority == a.nPartPriority && 
				p1.nData == a.p1.nData && p2.nData == a.p2.nData && p3.nData == a.p3.nData;
		}
	};
	vector<SOperation> ops;

	CRenderCmdList() {}
	bool IsEmpty() const { return ops.empty(); }
};

class COpGenContext
{
	vector<CRenderCmdList::SOperation> *pRes;
	const SRenderFragmentInfo *pCurFragment;
	bool bHasAddedOps;
	int nPartPriority;

public:
	COpGenContext( vector<CRenderCmdList::SOperation> *_pRes, const SRenderFragmentInfo *_pTarget )
		: pRes(_pRes), pCurFragment(_pTarget), bHasAddedOps(false), nPartPriority( _pTarget->vars.nPriority ) {}
	bool HasAddedOps() const { return bHasAddedOps; }
	void AddOperation(
		unsigned char _op, unsigned char _nPass, int nSBM, char nDestRegister, 
		CRenderCmdList::UParameter p1 = CRenderCmdList::UParameter(), 
		CRenderCmdList::UParameter p2 = CRenderCmdList::UParameter(), 
		CRenderCmdList::UParameter p3 = CRenderCmdList::UParameter() )
	{
		ASSERT( pCurFragment );
		pRes->push_back( 
			CRenderCmdList::SOperation( pCurFragment, _op, nPartPriority, _nPass, nSBM, nDestRegister, p1, p2, p3 )
			);
		bHasAddedOps = true;
	}
	const SRenderFragmentInfo *GetCurFragment() const { return pCurFragment; }
};

void MakeSingleOp( CRenderCmdList *pRes, CSceneFragments &src, bool bTakeLitParticles, float fMinFade, 
	SPerspDirectionalDepthInfo *pDepthInfo, unsigned char op,
	CRenderCmdList::UParameter _p1 = CRenderCmdList::UParameter(),
	CRenderCmdList::UParameter _p2 = CRenderCmdList::UParameter(),
	CRenderCmdList::UParameter _p3 = CRenderCmdList::UParameter(),
	int nStencilOp = 0 );

struct SRenderPathContext
{
	SPerspDirectionalDepthInfo depthInfo;
	NGfx::CTexture *pCurrentDepthTexture;
	NGfx::CTexture *pCurrentWaterReflectionTexture;
	CPool<SFastGf3RenderInfo> fastGF3infos;
	CPool<CVec3> vec3Pool;
	bool bTnL;

	SRenderPathContext( bool _bTnL, NGfx::CTexture *_pDepthTex, NGfx::CTexture *_pCurrentWaterReflectionTexture ) : bTnL(_bTnL), 
		pCurrentDepthTexture(_pDepthTex), pCurrentWaterReflectionTexture(_pCurrentWaterReflectionTexture) {}
};

struct SLightInfo;

class IMaterial: public CObjectBase
{
public:
	virtual bool DoesCastShadow() const { return false; }
	virtual bool IsOpaque() const { return false; }
	virtual bool IsAlphaTest() const { return false; }
	virtual NGfx::CTexture *GetAlphaTestTex() { return 0; }
	virtual bool IsTransparent() const { return false; }
	virtual bool SetRenderMode( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo, int nROP, CRenderCmdList::UParameter p1, CRenderCmdList::UParameter p2 ) { return false; }
	virtual void AddATOperations( COpGenContext *p, const SPerspDirectionalDepthInfo *pDepthInfo ) {}
	virtual void AddOperations( COpGenContext *pOp, SRenderPathContext *pRPC ) = 0;
	virtual void AddCustomOperation( COpGenContext *pOp, const ERenderOperation &op, unsigned char nPass,
		int nSBM, char nDestRegister, CRenderCmdList::UParameter p = CRenderCmdList::UParameter() ) = 0;
	virtual void SetTransparentRenderMode( NGfx::CRenderContext *pRC, const SPerPartVariables &vars, const SLightInfo &lightInfo, SRenderPathContext *pRPC ) {}
	virtual bool Is2Sided() const { return false; }
	virtual bool IsSelfIllum() const { return false; }
	virtual void Precache() {}
	virtual CVec3 GetTranslucentColor() const { return CVec3(0,0,0); }
	virtual bool IsProjectOnTerrain() const { return false; }
	virtual bool DoesSupportLightmaps() const { return false; }
	virtual bool IsUsingWaterReflection() const { return false; }
	virtual bool DoesIgnoreZ() const { return false; }
	virtual bool DoesBackFaceCastShadow() const { return false; }
	virtual float GetReflectionZ()const { return 0.0f; }
	virtual IMaterial *GetWindAffected(){return 0;};
	virtual IMaterial *GetNoReceiveShadows(){return 0;};
	virtual void SetPriority( int nP ){};
	virtual EDynamicType GetDynamicType(){ return DT_DONT_CARE; };
	
};

struct SParticleLMRenderTargetInfo
{
	SHMatrix rootTransform;
	CObj<NGfx::CTexture> pParticleLMs;
	CVec2 vParticleLMSize, vKernelSize;

	SParticleLMRenderTargetInfo() : vParticleLMSize(0,0), vKernelSize(0,0) {}
};

}

