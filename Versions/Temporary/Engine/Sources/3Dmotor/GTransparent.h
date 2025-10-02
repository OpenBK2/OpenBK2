#pragma once
#include "Cache.h"
#include "GParticleInfo.h"
#include "GRenderCore.h"

class CTransformStack;
namespace NGfx
{
	class CRenderContext;
	class CTexture;
	class CTriList;
	class CCubeTexture;
}

namespace NGScene
{
class CParticleEffect;
class IVBCombiner;
class IMaterial;
struct SRenderGeometryInfo;
struct SRenderFragmentInfo;

struct SMaterialParams
{
	IMaterial *pMaterial;
	SPerPartVariables vars;

	SMaterialParams() : pMaterial(0) {}
	bool operator==( const SMaterialParams &a ) const { return a.pMaterial == pMaterial && a.vars == vars; }
	bool operator!=( const SMaterialParams &a ) const { return !( *this == a ); }
	void Clear() { pMaterial = 0; }
	bool IsEmpty() const { return pMaterial == 0; }
};

struct STransparentObjectInfo
{
	SRenderGeometryInfo *pGeometry;
	SMaterialParams material;
};

class IParticles: public CObjectBase
{
public:
	virtual CParticleEffect* GetEffect() = 0;
	virtual void SetFade( float fVal ) = 0;
	virtual void Unlink() = 0;
};

class CTranspLightTexture : 
	public NCache::CGatherElementBase<NCache::CShortPtrAllocator,NCache::CQuadTreeElement, CTranspLightTexture>
{
	typedef NCache::CGatherElementBase<NCache::CShortPtrAllocator,NCache::CQuadTreeElement, CTranspLightTexture> CBase;
	OBJECT_NOCOPY_METHODS(CTranspLightTexture);
public:
	CTRect<int> region;
};

class IReportParticlesGeometry
{
public:
	virtual void AddParticles( IVBCombiner *pVertices, CFuncBase<vector<NGfx::STriangleList> > *pTrilists, 
		int nPart, int nParticles, const SBound &bv ) = 0;
};

struct STransparentInfo
{
	CObj<IVBCombiner> pGeom;
	STransparentObjectInfo *pObjectInfo;
	float fDepth;
	int nOffset; // address of first particle or part in object combiner
};

class CBaseParticlesGeometry;
class CShaderParticlesGeometry;
class CTnLParticlesGeometry;
class CParticlesTriList;
enum ERenderPath;
struct SLightInfo;
struct SRenderPathContext;
struct STransparentRenderContext
{
	bool bTnLMode;
	NGfx::CRenderContext *pRC;
	NGfx::CTexture *pParticleLight;
	NGfx::CCubeTexture *pSky;
	ERenderPath renderPath;
	SRenderPathContext *pRPC;
	const SLightInfo &lightInfo;

	STransparentRenderContext( bool _bTnLMode, NGfx::CRenderContext *_pRC, NGfx::CTexture *_pParticleLight, 
		NGfx::CCubeTexture *_pSky, ERenderPath _renderPath,
		SRenderPathContext *_pRPC, const SLightInfo &_lightInfo ) 
		: bTnLMode(_bTnLMode), pRC(_pRC), pParticleLight(_pParticleLight), pSky(_pSky), renderPath(_renderPath), pRPC(_pRPC), lightInfo(_lightInfo) {}
};

class CTransparentRenderer : public CObjectBase, public IParticleOutput
{
	OBJECT_NOCOPY_METHODS(CTransparentRenderer);
	SParticleLightInfo litParticlesAlloc, kernel, *pLMAlloc;
	SParticleOrientationInfo orientation;

	vector<STransparentInfo> infos;
	CPool<STransparentObjectInfo> objInfoPool;
	vector<float> depths;
	vector<int> sourcePtrs;
	vector<int> infoStartIdx;
	unsigned int nElementPtr, nInfoIdx;
	bool bUseFakeLM;

	NGfx::SCompactVector vNormal;
	CObj<CBaseParticlesGeometry> pParticlesGeometry;
	CObj<CShaderParticlesGeometry> pShaderParticlesGeometry;
	CObj<CTnLParticlesGeometry> pTnLParticlesGeometry;
	CObj<CParticlesTriList> pParticlesTrilist;
	int nTargetParticle, nTargetStart, nPieceStart;//nEffectParticle;
	STransparentInfo *pWriteParticles;
	CParticleEffect *pCurrentEffect;
	SBound currentEffectBV;
	DWORD dwCurrentParticleColor;
	IReportParticlesGeometry *pReportParticles;
	int nTotalParticles, nLitParticles;
	SBoundCalcer bc;
	bool bTnLMode;
	DWORD dwLitColor, dwNormalColor;
	const SPerVertexLightState *pLightState;

	STransparentInfo* AddFragment();
	void AllocParticlesWriteBuffer();
	void FinishParticlesPiece();
	void StartParticlesPiece();
	void RealRender( const STransparentRenderContext &trc );
	void AddParticleOverflow( const CVec3 vPos[4], DWORD dwColor, const STransparentTexturePlace &tPlace,
		float fDepth );
	ZDATA
public:
	ZEND int operator&( CStructureSaver &f ) { return 0; }
public:
	CTransparentRenderer() : nTotalParticles(0), nLitParticles(0) {}
	CTransparentRenderer( const CTransformStack &ts, const CTPoint<int> &vLightBuffersize, bool bUseFakeLM,
		DWORD dwLitColor, DWORD dwNormalColor, const SPerVertexLightState *_pLightState );
	virtual const SParticleOrientationInfo& GetOrientationInfo() const { return orientation; }
	virtual const SParticleLightInfo& GetKernelLightInfo() const { return kernel; }
	virtual void AddParticle( const CVec3 vPos[4], DWORD dwColor, const STransparentTexturePlace &tex,
		float fDepth );
	virtual void SampleWarFog( const vector<CVec3> &vPos, vector<float> *pRes );

	void AddParticles( IParticles *pParticles, bool bIsLit, const SBound &bv, IReportParticlesGeometry *pStore );
	void AddElement( SRenderGeometryInfo *pGeometry, IMaterial *pMaterial, const SPerPartVariables &vars, int nIndex, float fDepth );
	void FinishParticles();
	void Render( const STransparentRenderContext &trc );
	int GetTotalParticles() const { return nTotalParticles; }
	int GetLitParticles() const { return nLitParticles; }
	void GetBound( SBound *pRes ) { bc.Make( pRes ); }
	bool IsEmpty() { return bc.IsEmpty(); }
};

}

