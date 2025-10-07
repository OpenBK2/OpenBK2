#pragma once
#include "GParticleInfo.h"
#include "GRenderCore.h"
#include "GfxBuffers.h"

const int N_PARTICLES_BUFFER_SIZE = 4096;
const float F_MARGIN = 0.45f;
#pragma warning( disable : 4799 )

namespace NGScene
{

class CBaseParticlesGeometry : public IVBCombiner, public IParticleOutput
{
protected:
	struct SEffectInfo
	{
		ZDATA
		CObj<CParticleEffect> pEffect;
		int nStartParticle;
		SParticleLightInfo lmPlacement;
		DWORD dwPColor;
		int nBufferPlace;
		ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pEffect); f.Add(3,&nStartParticle); f.Add(4,&lmPlacement); f.Add(5,&dwPColor); f.Add(6,&nBufferPlace); return 0; }

		SEffectInfo() {}
		SEffectInfo( CParticleEffect *_pEffect, int _nStartParticle, const SParticleLightInfo &lm, DWORD _dwPColor, int _nBufferPlace )
			: pEffect(_pEffect) , nStartParticle(_nStartParticle), lmPlacement(lm), dwPColor(_dwPColor), nBufferPlace(_nBufferPlace) {}
	};
	ZDATA
	SParticleOrientationInfo orientation;
	std::vector<SEffectInfo> effects;
	SParticleLightInfo pl;
	int nSkipParticles;
	SBoundCalcer bc, bcPart;
public:
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&orientation); f.Add(3,&effects); f.Add(4,&pl); f.Add(5,&nSkipParticles); f.Add(6,&bc); f.Add(7,&bcPart); return 0; }
private:
	IParticleOutput *pRootOutput;
protected:
	DWORD dwPColor;
	bool bIsAddingPart;
public:
	CBaseParticlesGeometry( IParticleOutput *_pRootOutput = 0 ) : bIsAddingPart(false), pRootOutput(_pRootOutput) {}
	void RealStart( CParticleEffect *pEffect, int nStartParticle, const SBound &bv, const SParticleLightInfo &lm,
		DWORD _dwPColor, int nBufferPlace )
	{
		ASSERT(!bIsAddingPart);
		//bc.Add( bv );
		//partBVs.push_back( bv.s );
		effects.push_back( SEffectInfo( pEffect, nStartParticle, lm, dwPColor, nBufferPlace ) );
		pl = lm;
		bcPart.Clear();
		bIsAddingPart = true;
		dwPColor = _dwPColor;
	}
	virtual void Start( CParticleEffect *pEffect, int nStartParticle, const SBound &bv, const SParticleLightInfo &lm,
		DWORD dwPColor ) = 0;
	void FinishPart( SBound *pTrueBound, SParticleLightInfo *pLM )
	{
		ASSERT( bIsAddingPart );
		bIsAddingPart = false;
		bc.Add( bcPart );
		bcPart.Make( pTrueBound );
		partBVs.push_back( pTrueBound->s );
		*pLM = pl;
	}
	virtual void FreeWriteBuffer() = 0;
	virtual void SampleWarFog( const std::vector<CVec3> &vPos, std::vector<float> *pRes )
	{ 
		if ( pRootOutput )
			pRootOutput->SampleWarFog( vPos, pRes );
		else
			pRes->clear();
	}

	virtual const SParticleOrientationInfo& GetOrientationInfo() const { return orientation; }
};

template<class TFormat = NGfx::SGeomVecFull>
class CParticlesGeometry : public CBaseParticlesGeometry
{
	//OBJECT_NOCOPY_METHODS(CParticlesGeometry);
protected:
	ZDATA_(CBaseParticlesGeometry)
public:
	ZEND int operator&( CStructureSaver &f ) { f.Add(1,(CBaseParticlesGeometry*)this); return 0; }
protected:
	struct SWriteBuffer
	{
		NGfx::CBufferLock<TFormat> res;
		int nTarget;

		SWriteBuffer( CObj<NGfx::CGeometry> *pGeom ) : res( pGeom, N_PARTICLES_BUFFER_SIZE ), nTarget(0) {}
	};
	SWriteBuffer *pWriteBuffer;
protected:
	void Recalc()
	{
		if ( pWriteBuffer )
		{
			// refresh during creation?
			ASSERT(0);
			return;
		}
		bc.Clear();
		if ( effects.empty() )
			return;
		for ( int k = 0; k < effects.size(); ++k )
		{
			SEffectInfo &eff = effects[k];
			pl = eff.lmPlacement;
			if ( !pWriteBuffer )
				pWriteBuffer = new SWriteBuffer( &pValue );
			nSkipParticles = eff.nStartParticle;
			dwPColor = eff.dwPColor;
			ASSERT( pWriteBuffer->nTarget == eff.nBufferPlace );
			pWriteBuffer->nTarget = eff.nBufferPlace;
			eff.pEffect->AddParticles( this );
		}
		FreeWriteBuffer();
	}
public:
	CParticlesGeometry( IParticleOutput *_pRootOutput = 0 ) : CBaseParticlesGeometry(_pRootOutput), pWriteBuffer(0) {}

	virtual void Start( CParticleEffect *pEffect, int nStartParticle, const SBound &bv, const SParticleLightInfo &lm,
		DWORD dwPColor )
	{
		ASSERT( ( effects.empty() && pWriteBuffer == 0 ) || (!effects.empty() && pWriteBuffer != 0 ) );
		if ( !pWriteBuffer )
			pWriteBuffer = new SWriteBuffer( &pValue );
		RealStart( pEffect, nStartParticle, bv, lm, dwPColor, pWriteBuffer->nTarget );
	}
	void FreeWriteBuffer() 
	{ 
		ASSERT(!bIsAddingPart);
		if ( pWriteBuffer ) 
			delete pWriteBuffer; 
		pWriteBuffer = 0; 
		bc.Make( &bound ); 
	}
};

class CShaderParticlesGeometry : public CParticlesGeometry<NGfx::SGeomVecFull>
{
	OBJECT_NOCOPY_METHODS(CShaderParticlesGeometry);
	ZDATA_(CParticlesGeometry<NGfx::SGeomVecFull>)
	NGfx::SCompactVector vNormal;
	ZEND int operator&( CStructureSaver &f ) { f.Add(1,(CParticlesGeometry<NGfx::SGeomVecFull>*)this); f.Add(2,&vNormal); return 0; }
public:
	CShaderParticlesGeometry() {}
	CShaderParticlesGeometry( IParticleOutput *_pRootOutput, const SParticleOrientationInfo &_orientation, const NGfx::SCompactVector &_vNormal )
		: CParticlesGeometry<NGfx::SGeomVecFull>( _pRootOutput )
	{
		orientation = _orientation;
		vNormal = _vNormal;
	}
	__forceinline void WriteParticle( NGfx::SGeomVecFull *pRes, const CVec3 &v, const NGfx::SShortTextureUV &tex, 
		short nLU, short nLV, DWORD dwColor, CVec3 & vMin, CVec3 & vMax )
	{
		UpdateBounds(vMin, vMax, v);

		pRes->pos = v;
		pRes->normal = vNormal;
		pRes->tex = tex;
		pRes->texLM.nU = nLU;
		pRes->texLM.nV = nLV;
		pRes->texU.dw = dwColor;
		pRes->texV.dw = 0;
	}
	void AddParticle( const CVec3 vPos[4], DWORD dwColor, const STransparentTexturePlace &tPlace,
		float fDepth )
	{
		if ( --nSkipParticles < 0 && pWriteBuffer->nTarget < N_PARTICLES_BUFFER_SIZE )
			RealAddParticle( vPos, dwColor, tPlace, fDepth );
	}
	__forceinline void RealAddParticle( const CVec3 vPos[4], DWORD dwColor, const STransparentTexturePlace &tPlace,
		float fDepth )
	{
		NGfx::SGeomVecFull *pRes = &pWriteBuffer->res[pWriteBuffer->nTarget];
		pWriteBuffer->nTarget += 4;
		short nU = pl.vStart.nU, nV = pl.vStart.nV;
		short nU1 = pl.vStart.nU + pl.vSize.nU, nV1 = pl.vStart.nV + pl.vSize.nV;
		WriteParticle( pRes + 0, vPos[0], tPlace.vUVs[0], nU,  nV1, dwColor, bcPart.ptMin, bcPart.ptMax );
		WriteParticle( pRes + 1, vPos[1], tPlace.vUVs[1], nU1, nV1, dwColor, bcPart.ptMin, bcPart.ptMax );
		WriteParticle( pRes + 2, vPos[2], tPlace.vUVs[2], nU1, nV,  dwColor, bcPart.ptMin, bcPart.ptMax );
		WriteParticle( pRes + 3, vPos[3], tPlace.vUVs[3], nU,  nV,  dwColor, bcPart.ptMin, bcPart.ptMax );
		pl.Inc();
	}
};

void WriteParticle(NGfx::SGeomVecT2C1 * p_res, CVec3 v, float f_u, float f_v, DWORD dword, CVec3 vec3,
                  CVec3 pt_max);

class CTnLParticlesGeometry : public CParticlesGeometry<NGfx::SGeomVecT2C1>
{
	OBJECT_NOCOPY_METHODS(CTnLParticlesGeometry);
public:
	CTnLParticlesGeometry() {}
	CTnLParticlesGeometry( IParticleOutput *_pRootOutput, const SParticleOrientationInfo &_orientation )
		: CParticlesGeometry<NGfx::SGeomVecT2C1>( _pRootOutput )
	{
		orientation = _orientation;
	}
	__forceinline void WriteParticle( NGfx::SGeomVecT2C1 *pRes, const CVec3 &v, float fU, float fV,
		DWORD dwColor, CVec3 & vMin, CVec3 & vMax )
	{
		UpdateBounds(vMin, vMax, v);

		pRes->pos = v;
		pRes->color.dwColor = dwColor;
		pRes->tex1.x = fU;
		pRes->tex1.y = fV;
		pRes->tex2.x = 0;
		pRes->tex2.y = 0;
	}
	void AddParticle( const CVec3 vPos[4], DWORD dwColor, const STransparentTexturePlace &tPlace,
		float fDepth )
	{
		if ( --nSkipParticles < 0 && pWriteBuffer->nTarget < N_PARTICLES_BUFFER_SIZE )
			RealAddParticle( vPos, dwColor, tPlace, fDepth );
	}

	__forceinline DWORD BlendParticleColor(DWORD dwColor, DWORD dwPColor) {
		uint8_t R1 = (dwColor >> 16) & 0xFF;
		uint8_t G1 = (dwColor >> 8) & 0xFF;
		uint8_t B1 = (dwColor >> 0) & 0xFF;

		uint8_t R2 = (dwPColor >> 16) & 0xFF;
		uint8_t G2 = (dwPColor >> 8) & 0xFF;
		uint8_t B2 = (dwPColor >> 0) & 0xFF;

		uint8_t R = static_cast<uint8_t>((R1 * R2) / 255);
		uint8_t G = static_cast<uint8_t>((G1 * G2) / 255);
		uint8_t B = static_cast<uint8_t>((B1 * B2) / 255);
		uint8_t A = 0x7F;

		return (A << 24) | (R << 16) | (G << 8) | B;
	}

	__forceinline void RealAddParticle( const CVec3 vPos[4], DWORD dwColor, const STransparentTexturePlace &tPlace,
		float fDepth )
	{
		float fU1 = tPlace.vUVs[3].nU * (1.0f/NGfx::N_VEC_FULL_TEX_SIZE);
		float fV1 = tPlace.vUVs[3].nV * (1.0f/NGfx::N_VEC_FULL_TEX_SIZE);
		float fU2 = tPlace.vUVs[1].nU * (1.0f/NGfx::N_VEC_FULL_TEX_SIZE);
		float fV2 = tPlace.vUVs[1].nV * (1.0f/NGfx::N_VEC_FULL_TEX_SIZE);
		DWORD dwResColor = BlendParticleColor(dwColor, dwPColor);
		NGfx::SGeomVecT2C1 *pRes = &pWriteBuffer->res[pWriteBuffer->nTarget];
		pWriteBuffer->nTarget += 4;
		WriteParticle( pRes + 0, vPos[0], fU1, fV2, dwResColor, bcPart.ptMin, bcPart.ptMax );
		WriteParticle( pRes + 1, vPos[1], fU2, fV2, dwResColor, bcPart.ptMin, bcPart.ptMax );
		WriteParticle( pRes + 2, vPos[2], fU2, fV1, dwResColor, bcPart.ptMin, bcPart.ptMax );
		WriteParticle( pRes + 3, vPos[3], fU1, fV1, dwResColor, bcPart.ptMin, bcPart.ptMax );
	}
};

class CParticlesTriList : public CFuncBase<std::vector<NGfx::STriangleList> >
{
	OBJECT_BASIC_METHODS(CParticlesTriList);
	ZDATA
	std::vector<int> particles;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&particles); return 0; }
	bool bNeedUpdate;
protected: 
	void Recalc();
	bool NeedUpdate() { return bNeedUpdate; }
public:
	void AddPart( int nParticles ) { particles.push_back( nParticles ); bNeedUpdate = true; }
};
#pragma warning( default : 4799 )

}

