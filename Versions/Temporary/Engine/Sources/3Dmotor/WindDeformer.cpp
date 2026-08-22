#include "stdafx.h"
#include "3DLib/GGeometry.h"
#include "DBScene.h"
#include "GRenderModes.h"
#include "WindDeformer.h"
#include "GfxBuffers.h"

#include <algorithm>
#include <cstdint>

namespace NGScene
{

const float F_INV_255 = 1.0f / 255;

class CWindDeformer : public CPtrFuncBase<CObjectInfo>
{
	OBJECT_BASIC_METHODS( CWindDeformer )
	//
	ZDATA
	CDGPtr<CPtrFuncBase<CObjectInfo> > pGeom;
	SFBTransform place;
	CDGPtr<CFuncBase<SFBTransform> > pPlace;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pGeom); f.Add(3,&place); f.Add(4,&pPlace); return 0; }
protected:
	CWindDeformer() {}
	bool NeedUpdate() { return pGeom.Refresh(); }
	void Recalc();
public:
	CWindDeformer( CPtrFuncBase<CObjectInfo> *_pGeom, const SFBTransform &_place ) :
		pGeom(_pGeom), place(_place) {}
	CWindDeformer( CPtrFuncBase<CObjectInfo> *_pGeom, CFuncBase<SFBTransform> *_pPlace ) :
		pGeom(_pGeom), pPlace(_pPlace) { Identity( &(place.forward) ); Identity( &(place.backward) ); }
	CPtrFuncBase<CObjectInfo> *GetOriginalGeometry() const { return pGeom; }
};

void CWindDeformer::Recalc()
{
	CObjectInfo *pMesh = pGeom->GetValue();
	if ( !pMesh )
		return;

	if ( !pValue )
		pValue = new CObjectInfo( *pMesh );

	std::vector<NGScene::SUVInfo> &v = pValue->verts;
	const std::vector<uint16_t> &posInds = pValue->GetPositionIndices();
	std::vector<CVec3> &c = pValue->positions;

	float x, y, angle;

	const SFBTransform &curPlace = pPlace ? pPlace->GetValue() : place;

	float x11 = curPlace.forward._14;
	float x12 = curPlace.forward._24;
	float x13 = curPlace.forward._34;

	float r = (rand()%RAND_MAX)/(float)RAND_MAX;

	angle = (x11 + x12 + r * 3.0f ) * 0.2f;

	int s = (std::min)( v.size(), posInds.size() );

	for( int j=0; j < s; ++j)
	{
		x =cos(angle + (c[  posInds[j] ].x + c[  posInds[j] ].y) * 0.2f );
		y =sin(angle + (c[  posInds[j] ].x + c[  posInds[j] ].y) * 0.2f );

		float factor = c[  posInds[j] ].z * 0.01f;
		v[j].texLM.nU = x * NGfx::N_VEC_FULL_TEX_SIZE * factor;
		v[j].texLM.nV = y * NGfx::N_VEC_FULL_TEX_SIZE * factor;
	}
}

CPtrFuncBase<CObjectInfo> *CreateDeformerHolder( CPtrFuncBase<CObjectInfo> *pGeom, 
	const SFBTransform &place )
{
	if ( pGeom )
		return new CWindDeformer( pGeom, place );
	return 0;
}

CPtrFuncBase<CObjectInfo> *CreateDeformerHolder( CPtrFuncBase<CObjectInfo> *pGeom, CFuncBase<SFBTransform> *pPlace )
{
	if ( pGeom )
		return new CWindDeformer( pGeom, pPlace );
	return 0;
}

CObjectBase *GetWindDeformerSource( CObjectBase *p )
{
	if ( !p )
		return 0;
	if ( CDynamicCast<CWindDeformer> pWind = p )
		return pWind->GetOriginalGeometry();
	return 0;
}

}

using namespace NGScene;
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x1358BB78, CWindDeformer )
