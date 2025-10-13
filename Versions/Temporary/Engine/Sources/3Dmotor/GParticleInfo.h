#pragma once

#include "System/Dg.h"
#include "GPixelFormat.h"

#include <cstdint>

namespace NGfx
{
	class CGeometry;
	class CTexture;
}
namespace NGScene
{

struct SParticleLightInfo
{
	// size of single particle in texels
	CVec2 vLightSize; 
	// current alloc position and size of whole stuff
	NGfx::SShortTextureUV vStart, vSize, vStep, vTrueStep;
	short nCheckStart;

	void Init( int nSizeX, int nSizeY, int nTotalX, int nTotalY, bool bIncremental )
	{
		vLightSize = CVec2( nSizeX, nSizeY );
		int nDU = 65536 / nTotalX, nDV = 65536 / nTotalY;
		vStart.nU = nDU / 2 - 4 - 0x8000; vStart.nV = nDV / 2 - 4 - 0x8000;
		nCheckStart = vStart.nU;
		vSize.nU = nDU * (nSizeX - 1) + 8;
		vSize.nV = nDV * (nSizeY - 1) + 8;
		vTrueStep.nU = nDU * nSizeX; 
		vTrueStep.nV = nDV * nSizeY;
		if ( bIncremental )
		{
			vStep = vTrueStep;
		}
		else
			StopIncrementing();
	}
	void StopIncrementing() { vStep.nU = 0; vStep.nV = 0; }
	void ForcedInc()
	{
		vStart.nU += vTrueStep.nU;
		if ( vStart.nU == nCheckStart )
			vStart.nV += vTrueStep.nV;
	}
	void Inc() 
	{ 
		vStart.nU += vStep.nU;
		if ( vStart.nU == nCheckStart )
			vStart.nV += vStep.nV;
	}
};

struct STransparentTexturePlace
{
	NGfx::SShortTextureUV vUVs[4];
};

struct SParticleOrientationInfo
{
	CVec3 vBasic[4];
	CVec3 vDepth;
};

struct SRenderGeometryInfo;
class IMaterial;
const float F_PARTICLE_OVER = 1e10f;
class IParticleOutput
{
public:
	virtual const SParticleOrientationInfo& GetOrientationInfo() const = 0;
	const CVec3& GetDepth() const { return GetOrientationInfo().vDepth; }
	virtual void AddParticle( const CVec3 vPos[4], uint32_t dwColor, const STransparentTexturePlace &tex,
		float fDepth ) = 0;
	virtual void SampleWarFog( const std::vector<CVec3> &vPos, std::vector<float> *pRes ) = 0;
};

class IParticleFilter;
class CParticleEffect : public CObjectBase
{
public:
	bool bEnd; // true if effect finished
	int nGrassSize;
	std::vector<CObj<CPtrFuncBase<NGfx::CTexture> > > textures;
	CObj<IParticleFilter> pFilter;

	// function should be const but it is impossible due to DGPtrs presence
	virtual void AddParticles( IParticleOutput *pRender ) = 0;
	virtual bool IsEmpty() const { return false; }
	virtual void SetFade( float fVal ) {}
};

class CParticlesInfo;

struct SParticleFrame
{
	float fT;
	bool bLastCycle;
	int nNumFrame;
};

class CStandardParticleEffect: public CParticleEffect
{
	OBJECT_BASIC_METHODS(CStandardParticleEffect);
protected:
	CStandardParticleEffect() : fFade(1.0f) {}
public:
	CVec2 pivot;
	float fEndCycle;
	float fScale;
	bool bLeaveParticlesWhereStarted, bSampleCenterWarfog;

	std::vector<std::vector<SHMatrix> > transforms;
	std::vector<std::vector<char> > doneTransforms;
	std::vector<int> particlesFrames;

	float fTStopGeneration;
	int nStopCycle;

	SHMatrix transform;
	std::vector<SParticleFrame> frames;
	CDGPtr< CPtrFuncBase<CParticlesInfo>, CPtr<CPtrFuncBase<CParticlesInfo> > > pInfo;
	CVec2 vWrap;

	float fFade;
	int nPriority;

	CStandardParticleEffect( int _nPriority ) : fFade(1.0f), nPriority(_nPriority) {}
	void AddParticles( IParticleOutput *pRender );
	virtual bool IsEmpty() const { return frames.empty(); }
	void SetFade( float fVal ) { fFade = fVal; }
};

class CRainParticleEffect : public CParticleEffect
{
	OBJECT_BASIC_METHODS(CRainParticleEffect );
	void AddParticles( IParticleOutput *pRender );
public:
	std::vector<CVec3> positions;
	std::vector<char> faces;
	std::vector<CVec3> directions;
//	CDGPtr< CPtrFuncBase<CParticlesInfo>, CPtr<CPtrFuncBase<CParticlesInfo> > > pInfo;
};

void GetTransparentTexturePlace( STransparentTexturePlace *pRes, NGfx::CTexture *pTex, float fOffset = 0.5f );

}

