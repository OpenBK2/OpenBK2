#include "stdafx.h"
#include "GParticleInfo.h"
#include "GParticleFormat.h"
#include "GfxBuffers.h"
#include "GParticleFilter.h"

#include <cstdint>

const float F_PRIORITY_DIST = 0.01f;

static CVec3 NormalizedDif( const CVec3 &a, const CVec3 &b ) { CVec3 d( a - b ); Normalize(&d); return d; }

namespace	NGScene
{
static float fRandomShifts[12] = { 0.02f, -0.05f, 0.07f, -0.12f, 0, 0.03f, 0.08f, -0.04f };

void GetTransparentTexturePlace( STransparentTexturePlace *pRes, NGfx::CTexture *pTex, float fOffset )
{
	NGfx::STexturePlaceInfo place;
	CObj<NGfx::CTexture> pHolder = NGfx::GetTextureContainer( pTex, &place );
	ASSERT( NGfx::HasSameContainer( NGfx::GetTransparentTextureCache(), pHolder ) );
	if ( !NGfx::HasSameContainer( NGfx::GetTransparentTextureCache(), pHolder ) )
	{
		memset( pRes, 0, sizeof(*pRes) );
		return;
	}
	float fU1 = 1.0f / place.size.x;
	float fV1 = 1.0f / place.size.y;
	float fUStart = ( place.place.x1 + fOffset ) * fU1;
	float fVStart = ( place.place.y1 + fOffset ) * fV1;
	float fUFinish = ( place.place.x2 - fOffset ) * fU1;
	float fVFinish = ( place.place.y2 - fOffset ) * fV1;
	NGfx::CalcTexCoords( &pRes->vUVs[0], fUStart, fVStart );//fVFinish );
	NGfx::CalcTexCoords( &pRes->vUVs[1], fUFinish, fVStart );//fVFinish );
	NGfx::CalcTexCoords( &pRes->vUVs[2], fUFinish, fVFinish );//fVStart );
	NGfx::CalcTexCoords( &pRes->vUVs[3], fUStart, fVFinish );//fVStart );
}

static void GetGrassTexturePlace( std::vector<STransparentTexturePlace> *pRes, int nGrass, NGfx::CTexture *pTex )
{
	STransparentTexturePlace toSplit;
	GetTransparentTexturePlace( &toSplit, pTex, 0 );
	NGfx::STexturePlaceInfo place;
	CObj<NGfx::CTexture> pHolder = NGfx::GetTextureContainer( pTex, &place );
	NGfx::SShortTextureUV delta;
	NGfx::CalcTexCoords( &delta, 0.5f / place.size.x, 0.5f / place.size.y );
	int nUStart = toSplit.vUVs[0].nU;
	int nUSize = toSplit.vUVs[1].nU - nUStart;
	int nVStart = toSplit.vUVs[0].nV;
	int nVSize = toSplit.vUVs[2].nV - nVStart;
	CTPoint<int> uvShift[4];
	ASSERT( nGrass > 0 );
	int nElSizeU = nUSize / nGrass;
	int nElSizeV = nVSize / nGrass;
	uvShift[0] = CTPoint<int>( delta.nU, nElSizeV - delta.nV );
	uvShift[1] = CTPoint<int>( nElSizeU - delta.nU, nElSizeV - delta.nV );
	uvShift[2] = CTPoint<int>( nElSizeU - delta.nU, delta.nV );
	uvShift[3] = CTPoint<int>( delta.nU, delta.nV );
	pRes->resize( nGrass * nGrass );
	for ( int k = 0; k < pRes->size(); ++k )
	{
		int nCornerU  = (k % nGrass) * nElSizeU;
		int nCornerV  = (k / nGrass) * nElSizeV;
		STransparentTexturePlace &res = (*pRes)[k];
		for ( int i = 0; i < 4; ++i )
		{
			int nU = nCornerU + uvShift[i].x;
			int nV = nVSize - ( nCornerV + uvShift[i].y );
			res.vUVs[i].nU = nUStart + nU;
			res.vUVs[i].nV = nVStart + nV;
		}
	}
}

static void InitTexturePlaces( std::vector<STransparentTexturePlace> *pRes,
	const std::vector<CObj<CPtrFuncBase<NGfx::CTexture> > > &tex )
{
	for ( int k = 0; k < pRes->size(); ++k )
	{
		CDGPtr<CPtrFuncBase<NGfx::CTexture> > pTex( tex[k] );
		if ( !IsValid( pTex ) )
			continue;
		pTex.Refresh();
		GetTransparentTexturePlace( &(*pRes)[k], pTex->GetValue() );
	}
}

// CStandardParticleEffect

const int N_SIN_TABLE_PERIOD = 512;
const float F_SIN_TABLE_MULT = N_SIN_TABLE_PERIOD / FP_2PI;
static float fSinTable[ N_SIN_TABLE_PERIOD * 2];
struct SSinCosTableInit
{
	SSinCosTableInit()
	{
		for ( int k = 0; k < N_SIN_TABLE_PERIOD * 2 - 1; ++k )
			fSinTable[k] = sin ( k * FP_2PI / N_SIN_TABLE_PERIOD );
	}
} sinCosTable;
inline void FastSinCos( float fAngle, float *pfSin, float *pfCos )
{
	float fA = fAngle * F_SIN_TABLE_MULT;
	int n = Float2Int( fA - 0.5f );
	float fResidual = fA - n;
	n &= N_SIN_TABLE_PERIOD - 1;
	*pfSin = fSinTable[n] + ( fSinTable[n+1] - fSinTable[n] ) * fResidual;
	n += N_SIN_TABLE_PERIOD / 4;
	*pfCos = fSinTable[n] + ( fSinTable[n+1] - fSinTable[n] ) * fResidual;
}
void CStandardParticleEffect::AddParticles( IParticleOutput *pRender )
{
	if ( !IsValid(pInfo) )
		return;
	pInfo.Refresh();
	CParticlesInfo *pParticles = pInfo->GetValue();
	if ( !pParticles )
		return;

	if ( bLeaveParticlesWhereStarted  )
	{		
		if ( transforms.size() < frames.size() )
		{
			transforms.resize( frames.size() );
			doneTransforms.resize( frames.size() );
			particlesFrames.resize( frames.size(), -1 );
		}

		static std::vector<bool> isFrameCached;
		isFrameCached.resize( frames.size() );
		for ( int iFrame = 0; iFrame < frames.size(); ++iFrame )
		{
			bool bFound = false;
			for ( int iTransformedFrame = 0; iTransformedFrame < particlesFrames.size(); ++iTransformedFrame )
			{
				if ( particlesFrames[iTransformedFrame] == frames[iFrame].nNumFrame )
				{
					transforms[iFrame].swap( transforms[iTransformedFrame] );
					doneTransforms[iFrame].swap( doneTransforms[iTransformedFrame] );
					std::swap( particlesFrames[iFrame], particlesFrames[iTransformedFrame] );
					bFound = true;;
					break;
				}				
			}
			isFrameCached[iFrame] = bFound;
		}

		for ( int iFrame = 0; iFrame < frames.size(); ++iFrame )
		{
			if ( !isFrameCached[iFrame] )
			{
				doneTransforms[iFrame].assign( pParticles->nParticles, 0 );
				transforms[iFrame].resize( pParticles->nParticles );
				particlesFrames[iFrame] = frames[iFrame].nNumFrame;
			}
		}

	}

	const SParticleOrientationInfo &or = pRender->GetOrientationInfo();
	std::vector<STransparentTexturePlace> texturePlaces( textures.size() );
	InitTexturePlaces( &texturePlaces, textures );
	
	bool bDoWrap = vWrap.x != 0;
	CVec3 vWrapCenter;
	if ( bDoWrap )
	{
		vWrapCenter = or.vBasic[3];
		const CVec3 &vDir = or.vBasic[2];
		if ( vDir.z != 0 )
			vWrapCenter -= vDir * ( vWrapCenter.z / vDir.z );
	}

	static std::vector<char> filterParticles;
	static std::vector<float> warfog;
	static std::vector<CVec3> positions;
		int nParticles = pParticles->nParticles;
	if ( filterParticles.size() < nParticles )
		filterParticles.resize( nParticles );

	float fCycleEnd = fEndCycle * pParticles->fFrameRate;
	for ( int nFrame = 0; nFrame < frames.size(); ++nFrame )
	{
		const SParticleFrame &frame = frames[nFrame];
		char *pDropParticles = &filterParticles[0];

		int nDst = 0;
		if ( positions.size() < nParticles)
			positions.resize( nParticles );
		for ( int n = 0; n < nParticles; ++n )
		{
			const SParticle &part = pParticles->particles[n];
			if ( frame.fT >= part.nTStart && frame.fT < part.nTEnd &&
				(frame.bLastCycle || part.nTStart <= fCycleEnd) && 
				( fTStopGeneration < 0 || (!frame.bLastCycle) || part.nTStart < fTStopGeneration )
				)
			{
				const SParticleFrame &frame = frames[ nFrame ];
				CVec3 pos;
				part.pos.GetValue( frame.fT, &pos );
				if ( bDoWrap )
				{
					float fWrapSizeX = vWrap.x, fWrapSizeY = vWrap.y;
					float fCenterX = vWrapCenter.x, fCenterY = vWrapCenter.y;
					pos.x += Float2Int( ( fCenterX - pos.x ) / fWrapSizeX ) * fWrapSizeX;
					pos.y += Float2Int( ( fCenterY - pos.y ) / fWrapSizeY ) * fWrapSizeY;
				}
				if ( bLeaveParticlesWhereStarted )
				{
					if ( !doneTransforms[nFrame][n] )
					{
						doneTransforms[nFrame][n] = 1;
						transforms[nFrame][n] = transform;
					}
					transforms[nFrame][n].RotateHVector( &pos, pos );
				}
				else
					transform.RotateHVector( &pos, pos );
				positions[ nDst++ ] = pos;
				pDropParticles[n] = 0;
			}
			else
				pDropParticles[n] = 1;
		}

		if ( nDst == 0 )
			continue;

		positions.resize( nDst );

		// in case filtering is enabled form packet for filter and perform it
		if ( pFilter )
		{
			static std::vector<char> filter;
			pFilter->FilterParticles( positions, filterParticles, &filter );
			nDst = 0;
			int nRes = 0;
			for ( int k = 0; k < nParticles; ++k )
			{
				if ( pDropParticles[k] )
					continue;
				++nDst;
				if ( filter[ nDst - 1 ] )
				{
					pDropParticles[k] = 1;
					continue;
				}
				positions[ nRes++ ] = positions[ nDst - 1 ];
			}
			positions.resize( nRes );
		}

		int nWarFogStepMask;
		if ( bSampleCenterWarfog )
		{
			std::vector<CVec3> cp( 1 );
			cp[0] = transform.GetTranslation();
			pRender->SampleWarFog( positions, &warfog );
			nWarFogStepMask = 0;
		}
		else
		{
			pRender->SampleWarFog( positions, &warfog );
			nWarFogStepMask = -1;
		}
		float *pWarFog, fOne;
		if ( warfog.empty() )
		{
			fOne = 1;
			pWarFog = &fOne;
			nWarFogStepMask = 0;
		}
		else 
			pWarFog = &warfog[0];

		nDst = 0;
		for ( int n = 0; n < nParticles; ++n )
		{
			const SParticle &part = pParticles->particles[n];
			if ( pDropParticles[n] )
				continue;
			++nDst;
			const SParticleFrame &frame = frames[ nFrame ];

			float rot;	
			CVec2 scale;
			short sprite;
			unsigned int nSprite;
			uint32_t dwColor;

			float fFog = pWarFog[ ( nDst - 1 ) & nWarFogStepMask ] * fFade;
			part.color.GetValue( frame.fT, fFog, &dwColor );
			if ( dwColor == 0 )
				continue;

			part.rot.GetValue( frame.fT, &rot );
			part.scale.GetValue( frame.fT, &scale );
			part.sprite.GetValue( frame.fT, &sprite );
			nSprite = sprite;
			if ( nSprite >= textures.size() )
				continue;

			// geometry
			float fSin, fCos;
			FastSinCos( rot, &fSin, &fCos );
			//float fCos = cos(rot), fSin = sin(rot);

			//CVec2 v[4];
			float fHalfScale = fScale * 0.5f;
			float fScaleX = scale.x * fHalfScale, fScaleY = scale.y * fHalfScale;
			float fPivotX = - fScaleX * pivot.x, fPivotY = -fScaleY * pivot.y;
			float fTPivotX = fCos * fPivotX - fSin * fPivotY;
			float fTPivotY = fSin * fPivotX + fCos * fPivotY;
			float fTScaleXX = fCos * fScaleX;
			float fTScaleXY = fSin * fScaleX;
			float fTScaleYX = -fSin * fScaleY;
			float fTScaleYY = fCos * fScaleY;
			//v[0].x = fPivotX - fScaleX;  v[0].y = fPivotY - fScaleY;
			//v[1].x = fPivotX + fScaleX;  v[1].y = fPivotY - fScaleY;
			//v[2].x = fPivotX + fScaleX;  v[2].y = fPivotY + fScaleY;
			//v[3].x = fPivotX - fScaleX;  v[3].y = fPivotY + fScaleY;
			float fZShiftScale = fHalfScale * ( fabs(scale.x) + fabs(scale.y) );
			int nRnd = ( n * n + nFrame ) & 7, nRndStep = n|1;

			CVec3 pos = positions[ nDst - 1 ];
			CVec3 vPos[4];
#define ONE_VERTEX( N, xx, yy )\
			{\
			float x = fTPivotX + xx * fTScaleXX + yy * fTScaleYX;\
			float y = fTPivotY + xx * fTScaleXY + yy * fTScaleYY;\
			float fDZ = fRandomShifts[ nRnd ] * fZShiftScale;\
			vPos[N].x = pos.x + x * or.vBasic[0].x + y * or.vBasic[1].x + fDZ * or.vBasic[2].x;\
			vPos[N].y = pos.y + x * or.vBasic[0].y + y * or.vBasic[1].y + fDZ * or.vBasic[2].y;\
			vPos[N].z = pos.z + x * or.vBasic[0].z + y * or.vBasic[1].z + fDZ * or.vBasic[2].z;\
			nRnd = ( nRnd + nRndStep ) & 7;\
			}
			ONE_VERTEX( 0, -1 , -1 )
			ONE_VERTEX( 1,  1 , -1 )
			ONE_VERTEX( 2,  1 ,  1 )
			ONE_VERTEX( 3, -1 ,  1 )
#undef ONE_VERTEX
			pRender->AddParticle( vPos, dwColor, texturePlaces[nSprite], pos * or.vDepth + (float)nPriority * F_PRIORITY_DIST );
		}
	}
}

const float FP_DROP_H = 1.75f; // 0.4f
const float FP_DROP_W = 0.033f; // 0.033f
static float fRainDropShifts[4][2] = { { 0, 0 }, {FP_DROP_W,0}, {FP_DROP_W, FP_DROP_H}, {0,FP_DROP_H} };
void CRainParticleEffect::AddParticles( IParticleOutput *pRender )
{
	const SParticleOrientationInfo &or = pRender->GetOrientationInfo();
	std::vector<STransparentTexturePlace> texturePlaces( textures.size() );
	InitTexturePlaces( &texturePlaces, textures );
	for ( int nParticle = 0; nParticle < positions.size(); ++nParticle )
	{
		CVec3 pos = positions[ nParticle ];
		CVec3 vSide = ( pos - or.vBasic[3] ) ^ CVec3(0,0,1);
		Normalize( &vSide );
		CVec3 vPos[4];
		for ( int i = 0; i < 4; ++i )
			vPos[i] = pos + fRainDropShifts[i][0] * vSide + directions[nParticle] * fRainDropShifts[i][1];
		pRender->AddParticle( vPos, 0xffffffff, texturePlaces[ faces[nParticle] ], pos * or.vDepth );
	}
}

}

using namespace NGScene;
BASIC_REGISTER_CLASS(_3DMOTOR, CStandardParticleEffect)
BASIC_REGISTER_CLASS(_3DMOTOR, CParticleEffect)

