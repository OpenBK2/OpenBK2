#include "StdAfx.h"
#include "GParticleFormat.h"
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
				fMaxSize = Max( static_cast<float>(fabs(scale.x)), fMaxSize );
				fMaxSize = Max( static_cast<float>(fabs(scale.y)), fMaxSize );
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
	int n1 = Float2Int( f1 * 0x4000 ), n2 = Float2Int( ( f1 + f2 ) * 0x4000 ) - n1;
	_asm
	{
		mov esi, v1
		movd mm0, [esi]
		mov esi, v2
		movd mm1, [esi]
		punpcklbw mm0, mm0
		punpcklbw mm1, mm1
		psrlw mm0, 2
		psrlw mm1, 2
		movd mm3, n1
		punpcklwd mm3, mm3
		punpckldq mm3, mm3
		movd mm4, n2
		punpcklwd mm4, mm4
		punpckldq mm4, mm4
		pmulhw mm0, mm3
		pmulhw mm1, mm4
		paddw mm0, mm1
		psrlw mm0, 4
		packuswb mm0, mm0
		mov esi, pRes
		movd [esi], mm0
		emms
	}
	//ASSERT( IsInRange( v1 & 0xff, v2 & 0xff, (*pRes) & 0xff ) );
	//ASSERT( IsInRange( v1 & 0xff00, v2 & 0xff00, (*pRes) & 0xff00 ) );
	//ASSERT( IsInRange( v1 & 0xff0000, v2 & 0xff0000, (*pRes) & 0xff0000 ) );
	//ASSERT( IsInRange( v1 & 0xff000000, v2 & 0xff000000, (*pRes) & 0xff000000 ) );
	/*DWORD b = Float2Int( (v1 & 0x000000FF) * (1 - fAlpha) + (v2 & 0x000000FF) * fAlpha );
	DWORD g = Float2Int( (v1 & 0x0000FF00) * (1 - fAlpha) + (v2 & 0x0000FF00) * fAlpha );
	DWORD r = Float2Int( (v1 & 0x00FF0000) * (1 - fAlpha) + (v2 & 0x00FF0000) * fAlpha );
	DWORD a = Float2Int( (v1 & 0xFF000000) * (1 - fAlpha) + (v2 & 0xFF000000) * fAlpha );
	*pRes = b | (g & 0x0000FF00) | (r & 0x00FF0000) | (a & 0xFF000000);*/
}
template<>
void Scale( const DWORD &v, float f, DWORD *pRes )
{
	int n1 = Float2Int( f * 0x4000 );
	_asm
	{
		mov esi, v
		movd mm0, [esi]
		punpcklbw mm0, mm0
		psrlw mm0, 2
		movd mm3, n1
		punpcklwd mm3, mm3
		punpckldq mm3, mm3
		pmulhw mm0, mm3
		psrlw mm0, 4
		packuswb mm0, mm0
		mov esi, pRes
		movd [esi], mm0
		emms
	}
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

