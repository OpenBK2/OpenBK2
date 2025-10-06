#pragma once
#include "System/GResource.h"

namespace NGScene
{

template <class TValue>
inline void Interpolate( const TValue &v1, const TValue &v2, float f1, float f2, TValue *pRes )
{
	*pRes = v1 * f1 + v2 * f2;
}
template<>
inline void Interpolate( const short &v1, const short &v2, float f1, float f2, short *pRes ) { *pRes = v1; }
template<>
void Interpolate( const DWORD &v1, const DWORD &v2, float f1, float f2, DWORD *pRes );

template<class TValue>
inline void Scale( const TValue &v, float f, TValue *pRes ) { *pRes = v * f; }
template<>
inline void Scale( const short &v, float f, short *pRes ) { *pRes = v; }
template<>
void Scale( const DWORD &v, float f, DWORD *pRes );

#pragma pack( push, 2 )

template <class TValue>
class TKey
{
public:
	short nT;
	TValue value;
};

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
			*pRes = keys[0].value;
			return;
		}
		GetValueBinSearch( fT, 1, pRes );
	}
	void GetValue( float fT, float fScale, TValue *pRes ) const
	{
		if ( nKeys == 1 )
		{
			Scale( keys[0].value, fScale, pRes );
			return;
		}
		GetValueBinSearch( fT, fScale, pRes );
	}
private:
	__forceinline void GetValueBinSearch( float fT, float fScale, TValue *pRes ) const
	{
		int s = 0, e = nKeys - 1;
		short nT = Float2Int( fT - 0.5f );
		while( e - s > 1 )
		{
			int n = (s + e) / 2;
			if ( keys[n].nT <= nT )
				s = n;
			else
				e = n;
		}
		const TKey<TValue> &start = keys[s];
		const TKey<TValue> &end = keys[e];
		float fAlpha = fScale * Clamp( (fT - start.nT) / (end.nT - start.nT), 0.0f, 1.0f );
		Interpolate( start.value, end.value, fScale - fAlpha, fAlpha, pRes );
	}
};

struct SParticle
{
	short nTStart;
	short nTEnd;
	TKeyTrack<CVec3> pos;
	TKeyTrack<float> rot;
	TKeyTrack<CVec2> scale;
	TKeyTrack<DWORD> color;
	TKeyTrack<short> sprite;
};

#pragma pack( pop )

class CParticlesInfo: public CObjectBase
{
	OBJECT_BASIC_METHODS(CParticlesInfo);
public:
	CObj<CFileRequest> pData;
	//vector<char> pData;
	int nBytes;
	
	float fTEnd;
	float fFrameRate;
	int nParticles;
	SParticle *particles;

	CParticlesInfo() { nBytes = 0; nParticles = 0; fTEnd = 0; fFrameRate = 1; }
	void CalcBound( SBound *pRes );
};

class CParticlesLoader: public CLazyResourceLoader<int, CParticlesInfo>
{
	OBJECT_BASIC_METHODS(CParticlesLoader);
	virtual CFileRequest* CreateRequest();
	virtual void RecalcValue( CFileRequest *p );
};

}


