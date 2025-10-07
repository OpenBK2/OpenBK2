#include "stdafx.h"
#include "GParticleFormat.h"

#include "GPixelFormat.h"

namespace NGScene
{

// CParticlesInfo

void CParticlesInfo::CalcBound( SBound *pRes )
{
	CVec3 ptMin, ptMax;
	if ( nParticles )
	{
		float fMaxSize = 0;
		ptMin.x = ptMin.y = ptMin.z = 1e10f;
		ptMax.x = ptMax.y = ptMax.z = -1e10f;
		for ( int nP = 0; nP < nParticles; ++nP )
		{
			SParticle &part = particles[nP];
			if ( !part.pos.nKeys )
				continue;
			for ( int i = 0; i < part.pos.nKeys; ++i )
			{
				CVec3 pos = part.pos.keys[i].value;
				ptMin.Minimize( pos );
				ptMax.Maximize( pos );
			}
			for ( int i = 0; i < part.scale.nKeys; ++i )
			{
				CVec2 scale = part.scale.keys[i].value;
				fMaxSize = (std::max)( static_cast<float>(fabs(scale.x)), fMaxSize );
				fMaxSize = (std::max)( static_cast<float>(fabs(scale.y)), fMaxSize );
			}
		}
		fMaxSize *= (FP_SQRT_2 * 0.5f);
		CVec3 edge( fMaxSize, fMaxSize, fMaxSize );
		ptMin -= edge;
		ptMax += edge;
	}
	else
	{
		ptMin = VNULL3;
		ptMax = VNULL3;
	}
	pRes->BoxInit( ptMin, ptMax );
}

//static bool IsInRange( DWORD a, DWORD b, DWORD c ) { if ( a < b ) return c >= a && c <= b; return c >= b && c <= a; }
template<>
void Interpolate( const DWORD &v1, const DWORD &v2, float f1, float f2, DWORD *pRes )
{
	NGfx::SPixel8888 c1(v1), c2(v2), out;

	out.r = Float2Int(c1.r * f1 + c2.r * f2);
	out.g = Float2Int(c1.g * f1 + c2.g * f2);
	out.b = Float2Int(c1.b * f1 + c2.b * f2);
	out.a = Float2Int(c1.a * f1 + c2.a * f2);

	*pRes = out.dwColor;
}
template<>
void Scale( const DWORD &v, float f, DWORD *pRes )
{
	NGfx::SPixel8888 c(v), out;

	out.r = Float2Int(c.r * f);
	out.g = Float2Int(c.g * f);
	out.b = Float2Int(c.b * f);
	out.a = Float2Int(c.a * f);

	*pRes = out.dwColor;
}

// CParticlesLoader

CFileRequest* CParticlesLoader::CreateRequest()
{
	return CreateFileRequiest( "Effects", GetKey() );
}

void CParticlesLoader::RecalcValue( CFileRequest *pRequest )
{
	pValue = new CParticlesInfo;
	pValue->pData = pRequest;
	CMemoryStream *pFileData = pRequest->GetStream();
	if ( pFileData->GetSize() == 0 )
		return;
	//CFileRequest &file = *pRequest;

	//file->Read( &pValue->nBytes, sizeof(int) );
	//pValue->pData.resize( pValue->nBytes );
	char *pData = (char*)pFileData->GetBufferForWrite();
	pValue->nBytes = *(int*)pData;
	pData += 4;
	//file->Read( pData, pValue->nBytes );

	char *p = pData;
	pValue->fTEnd = *( (float*)p );
	p += sizeof(float);
	pValue->fFrameRate = *( (float*)p );
	p += sizeof(float);
	pValue->nParticles = *( (int*)p );
	p += sizeof(int);
	pValue->particles = (SParticle*)p;

	for ( int nP = 0; nP < pValue->nParticles; ++nP )
	{
		SParticle &particle = pValue->particles[nP];
		particle.pos.keys = (TKey<CVec3>*)(pData + (int)particle.pos.keys);
		particle.rot.keys = (TKey<float>*)(pData + (int)particle.rot.keys);
		particle.scale.keys = (TKey<CVec2>*)(pData + (int)particle.scale.keys);
		particle.color.keys = (TKey<DWORD>*)(pData + (int)particle.color.keys);
		particle.sprite.keys = (TKey<short>*)(pData + (int)particle.sprite.keys);
	}
}

}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x02541140, CParticlesLoader );

