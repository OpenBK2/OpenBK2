#pragma once
#include "3Dmotor_export.h"

#include "GScene.h"

namespace NGScene
{
struct SVec4Hash
{
	// Hashing the bit patterns is what a hash keyed on an exact vector wants, but
	// reading the floats through an int pointer aliased one type as another and
	// then mixed them with signed arithmetic, so an ordinary component like 1.0f,
	// whose bits are 0x3F800000, overflowed on the multiply. Copying the bits out
	// and mixing them unsigned is defined and scrambles them just as well.
	size_t operator()( const CVec4 &a ) const
	{
		static_assert( sizeof( a ) == 4 * sizeof( uint32_t ), "CVec4 is four 32 bit components" );
		uint32_t p[4] = {};
		memcpy( p, &a, sizeof( p ) );
		return p[0] ^ ( p[1] * 3u ) ^ ( p[2] * 9u ) ^ ( p[3] + 918237461u );
	}
};
struct SPostProcessData
{
	typedef std::unordered_map<CVec4, std::vector<IPostProcess::SObject>, SVec4Hash> CColorHash;
	CColorHash postColorer;
	CColorHash occluded;
};
void RenderPostProcess( NGfx::CRenderContext *pRC, const SPostProcessData &data );

class CPostColorer : public IPostProcess
{
	OBJECT_NOCOPY_METHODS( CPostColorer );
	ZDATA
	CDGPtr<CFuncBase<CVec4>> pColor;
	ZEND int operator&( CStructureSaver &f )
	{
		f.Add( 2, &pColor );
		return 0;
	}

public:
	CPostColorer( CFuncBase<CVec4> *_p = 0 ) : pColor( _p ) {}
	virtual void Render( SPostProcessData *pDst, const std::vector<SObject> &render );
};

class _3DMOTOR_EXPORT COccludedColorer : public IPostProcess
{
	OBJECT_NOCOPY_METHODS( COccludedColorer );
	ZDATA
	CDGPtr<CFuncBase<CVec4>> pColor;
	ZEND int operator&( CStructureSaver &f )
	{
		f.Add( 2, &pColor );
		return 0;
	}

public:
	COccludedColorer( CFuncBase<CVec4> *_p = 0 ) : pColor( _p ) {}
	virtual void Render( SPostProcessData *pDst, const std::vector<SObject> &render );
};

}
