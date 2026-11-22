#include "stdafx.h"

#include "GParticleFormat.h"
#include "GPixelFormat.h"

#include "3Dmotor_export.h"

#include <cmath>
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
				fMaxSize = (std::max)( static_cast<float>( std::fabs( scale.x ) ), fMaxSize );
				fMaxSize = (std::max)( static_cast<float>( std::fabs( scale.y ) ), fMaxSize );
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

namespace
{
	// Read packed fields without binding a misaligned reference.
	template <class T>
	T ReadBlobField( const unsigned char *&cursor )
	{
		T value;
		memcpy( &value, cursor, sizeof( value ) );
		cursor += sizeof( value );
		return value;
	}

	bool Finite( const CVec3 &v )
	{
		return std::isfinite( v.x ) && std::isfinite( v.y ) && std::isfinite( v.z );
	}

	bool Finite( const CVec2 &v )
	{
		return std::isfinite( v.x ) && std::isfinite( v.y );
	}

	template <class T>
	bool Finite( T value )
	{
		return std::isfinite( value );
	}

	template <class T>
	bool
	ReadTrack( TKeyTrack<T> *track, const unsigned char *&cursor, const unsigned char *payload, int tableEnd, int size )
	{
		const short count = ReadBlobField<short>( cursor );
		const int offset = ReadBlobField<int32_t>( cursor );
		if ( count <= 0 || offset < tableEnd || offset > size || offset % alignof( TKey<T> ) != 0 ||
		     size_t( count ) > size_t( size - offset ) / sizeof( TKey<T> ) )
		{
			return false;
		}
		track->nKeys = count;
		track->keys = reinterpret_cast<const TKey<T> *>( payload + offset );
		for ( int i = 0; i < count; ++i )
		{
			if ( ( i > 0 && track->keys[i].nT <= track->keys[i - 1].nT ) || !Finite( track->keys[i].GetValue() ) )
			{
				return false;
			}
		}
		return true;
	}
} // namespace

bool CParticlesInfo::ReadClip( const CDataStream &stream )
{
	// Offsets start after the size prefix. Each directory entry holds two frame
	// numbers and five (int16 key count, int32 offset) pairs.
	constexpr int header = 12;
	constexpr int record = 34;
	const unsigned char *cursor = stream.GetBuffer();
	if ( !cursor || stream.GetSize() < 4 + header )
	{
		return false;
	}
	const int size = ReadBlobField<int32_t>( cursor );
	const unsigned char *payload = cursor;
	const float duration = ReadBlobField<float>( cursor );
	const float rate = ReadBlobField<float>( cursor );
	const int count = ReadBlobField<int32_t>( cursor );
	if ( size != stream.GetSize() - 4 || count < 0 || count > ( size - header ) / record || !std::isfinite( duration ) ||
	     duration < 0 || !std::isfinite( rate ) || rate <= 0 )
	{
		return false;
	}
	const int tableEnd = header + record * count;
	std::vector<SParticle> parsed( count );
	for ( auto &part : parsed )
	{
		part.nTStart = ReadBlobField<short>( cursor );
		part.nTEnd = ReadBlobField<short>( cursor );
		if ( part.nTEnd < part.nTStart || !ReadTrack( &part.pos, cursor, payload, tableEnd, size ) ||
		     !ReadTrack( &part.rot, cursor, payload, tableEnd, size ) ||
		     !ReadTrack( &part.scale, cursor, payload, tableEnd, size ) ||
		     !ReadTrack( &part.color, cursor, payload, tableEnd, size ) ||
		     !ReadTrack( &part.sprite, cursor, payload, tableEnd, size ) )
		{
			return false;
		}
	}
	particles.swap( parsed );
	nParticles = count;
	nBytes = size;
	fTEnd = duration;
	fFrameRate = rate;
	return true;
}

void CParticlesLoader::RecalcValue( CFileRequest *pRequest )
{
	pValue = new CParticlesInfo;
	pValue->pData = pRequest;
	pValue->ReadClip( *pRequest->GetStream() );
}

} // namespace NGScene

using namespace NGScene;
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x02541140, CParticlesLoader );
