#pragma once

#include "System/GResource.h"

#include <cstdint>
#include <cstring>
#include <cstddef>

#include <boost/config.hpp>

#include "3Dmotor_export.h"

namespace NGScene
{

template <class TValue>
inline void Interpolate( const TValue &v1, const TValue &v2, float f1, float f2, TValue *pRes )
{
	*pRes = v1 * f1 + v2 * f2;
}
template <>
inline void Interpolate( const short &v1, const short &v2, float f1, float f2, short *pRes )
{
	*pRes = v1;
}
template <>
void Interpolate( const uint32_t &v1, const uint32_t &v2, float f1, float f2, uint32_t *pRes );

template <class TValue>
inline void Scale( const TValue &v, float f, TValue *pRes )
{
	*pRes = v * f;
}
template <>
inline void Scale( const short &v, float f, short *pRes )
{
	*pRes = v;
}
template <>
void Scale( const uint32_t &v, float f, uint32_t *pRes );

#pragma pack( push, 2 )

template <class TValue>
class TKey
{
public:
	short nT;
	// Held as bytes rather than as a TValue. These are packed to two bytes and
	// the arrays are read straight out of a resource blob, so the value lands two
	// bytes into the key while a CVec3 or a float wants four: declaring it TValue
	// made every read of it bind a reference to an object at an address its own
	// type does not permit, which is undefined, and is what UBSan reports both
	// here and through all the vector arithmetic downstream of it.
	//
	// The size and the offset are the ones the member had, so the layout is
	// untouched, which matters twice over: these arrays are reinterpreted in
	// place out of a file, and TKeyVector serializes a vector of them as one
	// block of bytes because TKey has no operator& of its own.
	unsigned char valueBytes[sizeof( TValue )];

	TValue GetValue() const
	{
		TValue value;
		memcpy( &value, valueBytes, sizeof( value ) );
		return value;
	}
	void SetValue( const TValue &value ) { memcpy( valueBytes, &value, sizeof( value ) ); }
};

// The key arrays are reinterpreted in place out of a resource blob and a vector
// of keys is serialized as one block of bytes, so this layout is part of two
// file formats. Stated rather than assumed, so that a change to it stops the
// build instead of quietly reading old data as something else.
template <class TValue>
struct SKeyLayoutCheck
{
	static_assert( sizeof( TKey<TValue> ) == sizeof( short ) + sizeof( TValue ),
	               "TKey is the time followed by the value with nothing padding it" );
	static_assert( offsetof( TKey<TValue>, valueBytes ) == sizeof( short ), "the value follows the time immediately" );
};
template struct SKeyLayoutCheck<CVec3>;
template struct SKeyLayoutCheck<CVec2>;
template struct SKeyLayoutCheck<float>;
template struct SKeyLayoutCheck<uint32_t>;
template struct SKeyLayoutCheck<short>;

// could use coherence between fetches to make key retrieve faster
template <class TValue>
class TKeyTrack
{
public:
	short nKeys;
	TKey<TValue> *keys;

	void GetValue( float fT, TValue *pRes ) const
	{
		if ( nKeys == 1 )
		{
			*pRes = keys[0].GetValue();
			return;
		}
		GetValueBinSearch( fT, 1, pRes );
	}
	void GetValue( float fT, float fScale, TValue *pRes ) const
	{
		if ( nKeys == 1 )
		{
			Scale( keys[0].GetValue(), fScale, pRes );
			return;
		}
		GetValueBinSearch( fT, fScale, pRes );
	}

private:
	BOOST_FORCEINLINE void GetValueBinSearch( float fT, float fScale, TValue *pRes ) const
	{
		int s = 0, e = nKeys - 1;
		short nT = Float2Int( fT - 0.5f );
		while ( e - s > 1 )
		{
			int n = ( s + e ) / 2;
			if ( keys[n].nT <= nT )
				s = n;
			else
				e = n;
		}
		const TKey<TValue> &start = keys[s];
		const TKey<TValue> &end = keys[e];
		float fAlpha = fScale * Clamp( ( fT - start.nT ) / ( end.nT - start.nT ), 0.0f, 1.0f );
		Interpolate( start.GetValue(), end.GetValue(), fScale - fAlpha, fAlpha, pRes );
	}
};

struct SParticle
{
	short nTStart;
	short nTEnd;
	TKeyTrack<CVec3> pos;
	TKeyTrack<float> rot;
	TKeyTrack<CVec2> scale;
	TKeyTrack<uint32_t> color;
	TKeyTrack<short> sprite;
};

#pragma pack( pop )

class _3DMOTOR_EXPORT CParticlesInfo : public CObjectBase
{
	OBJECT_BASIC_METHODS( CParticlesInfo );

public:
	CObj<CFileRequest> pData;
	//vector<char> pData;
	int nBytes;

	float fTEnd;
	float fFrameRate;
	int nParticles;
	std::vector<SParticle> particles;

	CParticlesInfo()
	{
		nBytes = 0;
		nParticles = 0;
		fTEnd = 0;
		fFrameRate = 1;
	}
	void CalcBound( SBound *pRes );
};

class _3DMOTOR_EXPORT CParticlesLoader : public CLazyResourceLoader<int, CParticlesInfo>
{
	OBJECT_BASIC_METHODS( CParticlesLoader );
	virtual CFileRequest *CreateRequest();
	virtual void RecalcValue( CFileRequest *p );
};

}
