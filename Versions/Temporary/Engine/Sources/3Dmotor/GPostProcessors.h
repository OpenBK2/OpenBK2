#ifndef __GPOSTPROCESSORS_H_
#define __GPOSTPROCESSORS_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "3Dmotor_export.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
#include "GScene.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NGScene
{
struct SVec4Hash
{
	int operator()( const CVec4 &a ) const { const int *p = (const int*)&a; return p[0] ^ ( p[1] * 3 ) ^ ( p[2] * 9 ) ^ ( p[3] + 918237461 ); }
};
struct SPostProcessData
{
	typedef hash_map<CVec4, vector<IPostProcess::SObject>, SVec4Hash > CColorHash;
	CColorHash postColorer;
	CColorHash occluded;
};
void RenderPostProcess( NGfx::CRenderContext *pRC, const SPostProcessData &data );
////////////////////////////////////////////////////////////////////////////////////////////////////
class CPostColorer : public IPostProcess
{
	OBJECT_NOCOPY_METHODS( CPostColorer );
	ZDATA
	CDGPtr<CFuncBase<CVec4> > pColor;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pColor); return 0; }
public:
	CPostColorer( CFuncBase<CVec4> *_p = 0 ) : pColor(_p) {}
	virtual void Render( SPostProcessData *pDst, const vector<SObject> &render );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class _3DMOTOR_EXPORT COccludedColorer : public IPostProcess
{
	OBJECT_NOCOPY_METHODS( COccludedColorer );
	ZDATA
	CDGPtr<CFuncBase<CVec4> > pColor;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pColor); return 0; }
public:
	COccludedColorer( CFuncBase<CVec4> *_p = 0 ) : pColor(_p) {}
	virtual void Render( SPostProcessData *pDst, const vector<SObject> &render );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
}
#endif
