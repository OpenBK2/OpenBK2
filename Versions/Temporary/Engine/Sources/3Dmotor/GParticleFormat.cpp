#include "stdafx.h"
#include "GParticleFormat.h"

#include "GPixelFormat.h"

#include "3Dmotor_export.h"

#include <cstdint>

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
			{
				continue;
			}
			for ( int i = 0; i < part.pos.nKeys; ++i )
			{
				CVec3 pos = part.pos.keys[i].GetValue();
				ptMin.Minimize( pos );
				ptMax.Maximize( pos );
			}
			for ( int i = 0; i < part.scale.nKeys; ++i )
			{
				CVec2 scale = part.scale.keys[i].GetValue();
				fMaxSize = (std::max)( static_cast<float>( fabs( scale.x ) ), fMaxSize );
				fMaxSize = (std::max)( static_cast<float>( fabs( scale.y ) ), fMaxSize );
			}
		}
		fMaxSize *= ( FP_SQRT_2 * 0.5f );
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

//static bool IsInRange( uint32_t a, uint32_t b, uint32_t c ) { if ( a < b ) return c >= a && c <= b; return c >= b && c <= a; }
template <>
void Interpolate( const uint32_t &v1, const uint32_t &v2, float f1, float f2, uint32_t *pRes )
{
	NGfx::SPixel8888 c1( v1 ), c2( v2 ), out;

	out.r = Float2Int( c1.r * f1 + c2.r * f2 );
	out.g = Float2Int( c1.g * f1 + c2.g * f2 );
	out.b = Float2Int( c1.b * f1 + c2.b * f2 );
	out.a = Float2Int( c1.a * f1 + c2.a * f2 );

	*pRes = out.dwColor;
}
template <>
void Scale( const uint32_t &v, float f, uint32_t *pRes )
{
	NGfx::SPixel8888 c( v ), out;

	out.r = Float2Int( c.r * f );
	out.g = Float2Int( c.g * f );
	out.b = Float2Int( c.b * f );
	out.a = Float2Int( c.a * f );

	*pRes = out.dwColor;
}

// CParticlesLoader

CFileRequest *CParticlesLoader::CreateRequest()
{
	return CreateFileRequiest( "Effects", GetKey() );
}

//! Reads one field out of the particle resource blob and advances the cursor.
//!
//! The records are packed: three shorts then a four byte offset, then a short
//! and an offset four times over. So every offset lands two bytes into a record,
//! and casting the cursor to int32_t and loading through it was a load at an
//! address that type does not permit, which is what UBSan reports once per
//! offset. Copying the bytes is defined wherever the cursor happens to sit, and
//! it also stops each of these reads from aliasing the byte buffer as another
//! type, which the shorts and floats were doing too without being reported.
template <class T>
inline T ReadBlobField( char *&p )
{
	T value;
	memcpy( &value, p, sizeof( value ) );
	p += sizeof( value );
	return value;
}

void CParticlesLoader::RecalcValue( CFileRequest *pRequest )
{
	pValue = new CParticlesInfo;
	pValue->pData = pRequest;
	CMemoryStream *pFileData = pRequest->GetStream();
	if ( pFileData->GetSize() == 0 )
	{
		return;
	}
	//CFileRequest &file = *pRequest;

	//file->Read( &pValue->nBytes, sizeof(int) );
	//pValue->pData.resize( pValue->nBytes );
	char *pData = (char *)pFileData->GetBufferForWrite();
	pValue->nBytes = ReadBlobField<int>( pData );
	//file->Read( pData, pValue->nBytes );

	char *p = pData;
	pValue->fTEnd = ReadBlobField<float>( p );
	pValue->fFrameRate = ReadBlobField<float>( p );
	pValue->nParticles = ReadBlobField<int>( p );
	pValue->particles.resize( pValue->nParticles );

	for ( int nP = 0; nP < pValue->nParticles; ++nP )
	{
		SParticle &particle = pValue->particles[nP];
		particle.nTStart = ReadBlobField<short>( p );
		particle.nTEnd = ReadBlobField<short>( p );

		particle.pos.nKeys = ReadBlobField<short>( p );
		int32_t offset = ReadBlobField<int32_t>( p );
		particle.pos.keys = reinterpret_cast<TKey<CVec3> *>( pData + offset );

		particle.rot.nKeys = ReadBlobField<short>( p );
		offset = ReadBlobField<int32_t>( p );
		particle.rot.keys = reinterpret_cast<TKey<float> *>( pData + offset );

		particle.scale.nKeys = ReadBlobField<short>( p );
		offset = ReadBlobField<int32_t>( p );
		particle.scale.keys = reinterpret_cast<TKey<CVec2> *>( pData + offset );

		particle.color.nKeys = ReadBlobField<short>( p );
		offset = ReadBlobField<int32_t>( p );
		particle.color.keys = reinterpret_cast<TKey<uint32_t> *>( pData + offset );

		particle.sprite.nKeys = ReadBlobField<short>( p );
		offset = ReadBlobField<int32_t>( p );
		particle.sprite.keys = reinterpret_cast<TKey<short> *>( pData + offset );
	}
}

}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x02541140, CParticlesLoader );
