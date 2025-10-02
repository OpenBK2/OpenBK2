#include "StdAfx.h"
#include "..\3DLib\GGeometry.h"
#include "DBScene.h"
#include "GRenderModes.h"
#include "HeightFog.h"

namespace NGScene
{

const float F_INV_255 = 1.0f / 255;

class CHeightFogHolder : public CPtrFuncBase<CObjectInfo>
{
	OBJECT_BASIC_METHODS( CHeightFogHolder )
	//
	ZDATA
	CDGPtr<CPtrFuncBase<CObjectInfo> > pGeom;
	CDBPtr<NDb::SHeightFog> pHeightFog;
	SFBTransform place;
	CDGPtr<CFuncBase<SFBTransform> > pPlace;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pGeom); f.Add(3,&pHeightFog); f.Add(4,&place);f.Add(5,&pPlace); return 0; }
protected:
	CHeightFogHolder() {}
	bool NeedUpdate() { return pGeom.Refresh() | ( !pPlace || pPlace.Refresh() ); }
	void Recalc();
public:
	CHeightFogHolder( CPtrFuncBase<CObjectInfo> *_pGeom, const NDb::SHeightFog *_pHeightFog, const SFBTransform &_place ) :
			pGeom(_pGeom), pHeightFog(_pHeightFog), place(_place) {}
	CHeightFogHolder( CPtrFuncBase<CObjectInfo> *_pGeom, const NDb::SHeightFog *_pHeightFog, CFuncBase<SFBTransform> *_pPlace ) :
			pGeom(_pGeom), pHeightFog(_pHeightFog), pPlace(_pPlace) { Identity( &(place.forward) ); Identity( &(place.backward) ); }
};

inline void GetHeightFogCol( CVec3 *pCol, const CVec3 &vPos, const CVec3 &vFogCol, float fMinHeight, float fHeightCoeff )
{
	const float t = Clamp( ( vPos.z - fMinHeight ) * fHeightCoeff, 0.0f, 1.0f );
	const float it = 1.0f - t;
	pCol->Set( vFogCol.x * it + t, vFogCol.y * it + t, vFogCol.z * it + t );
}

void CHeightFogHolder::Recalc()
{
	CObjectInfo *pMesh = pGeom->GetValue();
	if ( !pMesh )
		return;

	if ( !pValue )
		pValue = new CObjectInfo( *pMesh );

	if ( pHeightFog )
	{
		const vector<CVec3> &poses = pMesh->GetPositions();
		const vector<WORD> &posInds = pMesh->GetPositionIndices();
		const vector<DWORD> &srcAttrs = pMesh->GetAttribute( GATTR_VERTEX_COLOR );
		vector<DWORD> attrs( srcAttrs );
		if ( attrs.size() != posInds.size() )
		{
			attrs.resize( posInds.size() );
			fill( attrs.begin(), attrs.end(), 0xffffffff );
		}

		const float fHeightCoeff = 1.0f / ( pHeightFog->fMaxHeight - pHeightFog->fMinHeight );
		CVec3 vCol, vPos;

		const SFBTransform &curPlace = pPlace ? pPlace->GetValue() : place;

		for ( int i = 0; i < attrs.size(); ++i )
		{
			curPlace.forward.RotateHVector( &vPos, poses[posInds[i]] );
			DWORD &nAttr = attrs[i];
			const float fR = float( ( nAttr >> 16 ) & 0xff ) * F_INV_255;
			const float fG = float( ( nAttr >> 8 ) & 0xff ) * F_INV_255;
			const float fB = float( nAttr & 0xff ) * F_INV_255;
			GetHeightFogCol( &vCol, vPos, pHeightFog->vFogColor, pHeightFog->fMinHeight, fHeightCoeff );
			const int cR = Clamp( Float2Int( vCol.x * fR * 255.0f ), 0, 255 );
			const int cG = Clamp( Float2Int( vCol.y * fG * 255.0f ), 0, 255 );
			const int cB = Clamp( Float2Int( vCol.z * fB * 255.0f ), 0, 255 );
			nAttr = ( nAttr & 0xff000000 ) | ( cR << 16 ) | ( cG << 8 ) | cB;
		}

		pValue->SetAttribute( GATTR_VERTEX_COLOR, attrs );
	}
}

CPtrFuncBase<CObjectInfo> *CreateHeightFogHolder( CPtrFuncBase<CObjectInfo> *pGeom, const NDb::SHeightFog *pHeightFog,
																								  const SFBTransform &place )
{
	if ( pGeom )
		return new CHeightFogHolder( pGeom, pHeightFog, place );
	return 0;
}

CPtrFuncBase<CObjectInfo> *CreateHeightFogHolder( CPtrFuncBase<CObjectInfo> *pGeom, const NDb::SHeightFog *pHeightFog,
																								  CFuncBase<SFBTransform> *pPlace )
{
	if ( pGeom )
		return new CHeightFogHolder( pGeom, pHeightFog, pPlace );
	return 0;
}

} // namespace NGScene

using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x1318BB41, CHeightFogHolder )

