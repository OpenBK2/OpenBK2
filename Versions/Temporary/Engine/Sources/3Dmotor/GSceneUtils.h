#ifndef __GSCENEUTILS_H_
#define __GSCENEUTILS_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "RectLayout.h"
#include "../System/Time.hpp"
#include "GSkeleton.h"

namespace NGScene
{

struct SRenderStats
{
	int nVertices, nTris, nDIPs;
	int nSceneTris, nParticles, nLitParticles;
	float fFrameTime;
	bool bStaticGeometryThrashing, b2DTexturesThrashing, bTransparentThrashing;
	bool bLightmapThrashing;
	bool bDynamicGeometryTrashing;
	int nScenePointLights;
	bool bPointLightShadowThrashing;
};

DEFINE_DG_CONSTANT_NODE( CCMSR, SHMatrix );
DEFINE_DG_CONSTANT_NODE( CCFBTransform, SFBTransform );
DEFINE_DG_CONSTANT_NODE( CCSphere, SSphere );
DEFINE_DG_CONSTANT_NODE( CCVec2, CVec2 );
DEFINE_DG_CONSTANT_NODE( CCVec3, CVec3 );
DEFINE_DG_CONSTANT_NODE( CCVec4, CVec4 );
DEFINE_DG_CONSTANT_NODE( CCInt, int );
DEFINE_DG_CONSTANT_NODE( CCFloat, float );
DEFINE_DG_CONSTANT_NODE( CCTRect, CTRect<int> );
DEFINE_DG_CONSTANT_NODE( CCTPoint, CTPoint<int> );
DEFINE_DG_CONSTANT_NODE( CCWString, wstring );
DEFINE_DG_CONSTANT_NODE( CCRectLayout, CRectLayout );
#ifdef STUPID_VISUAL_ASSIST
class CCMSR;
class CCFBTransform;
class CCSphere;
class CCVec2;
class CCVec3;
class CCVec4;
class CCInt;
class CCFloat;
class CCTRect;
class CCTPoint;
class CCWString;
class CCRectLayout;
#endif

//! Создаёт CCTRect
inline CCTRect* CreateRect( int x, int y, int width, int height ) 
{
	CCTRect *pRect = new CCTRect( CTRect<int>( x, y, x + width, y + height ) );
	return pRect;
}

// hierarchy forming node
class CMSRNode: public CFuncBase<SFBTransform>
{
	OBJECT_BASIC_METHODS(CMSRNode);
protected:
	virtual bool NeedUpdate() { return pAncestor.Refresh() | pPos.Refresh(); }
	virtual void Recalc();
public:
	CDGPtr< CFuncBase<SFBTransform> > pAncestor;
	CDGPtr< CFuncBase<SFBTransform> > pPos;
	//
	CMSRNode() {}
	int operator&( CStructureSaver &f );
};

// hierarchy forming node
class CMNode : public CFuncBase<SFBTransform>
{
	OBJECT_BASIC_METHODS(CMNode);
	ZDATA
	CDGPtr< CFuncBase<SFBTransform> > pAncestor;
	CDGPtr< CFuncBase<CVec3> > pMove;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pAncestor); f.Add(3,&pMove); return 0; }
protected:
	virtual bool NeedUpdate() { return pAncestor.Refresh() | pMove.Refresh(); }
	virtual void Recalc();
public:
	CMNode() {}
	CMNode( CFuncBase<SFBTransform> *_pA, CFuncBase<CVec3> *_pM ) : pAncestor(_pA), pMove(_pM) {}
};

// scale node
class CScaleNode: public CFuncBase< CTRect<int> >
{
	OBJECT_BASIC_METHODS(CScaleNode);
protected:
	virtual bool NeedUpdate() { return pSize.Refresh(); }
	virtual void Recalc();
public:
	CVec2 vScreenRect;
	CDGPtr< CFuncBase< CTRect<int> > > pSize;
	//
	CScaleNode() {}
	int operator&( CStructureSaver &f );
};

// parameters 2 MSR converter
class CMSRConvert: public CFuncBase<SFBTransform>
{
	OBJECT_BASIC_METHODS(CMSRConvert);
protected:
	virtual bool NeedUpdate() { return pMove.Refresh() | pRotate.Refresh() | pScale.Refresh(); }
	virtual void Recalc();
public:
	CDGPtr< CFuncBase<CVec3> > pMove;
	CDGPtr< CFuncBase<CVec3> > pRotate;
	CDGPtr< CFuncBase<CVec3> > pScale;
	//
	int operator&( CStructureSaver &f );
};

class CExtractTranslation : public CFuncBase<CVec3>
{
	OBJECT_BASIC_METHODS(CExtractTranslation);
	ZDATA
	CDGPtr< CFuncBase<SFBTransform> > pTrans;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pTrans); return 0; }
protected:
	virtual bool NeedUpdate() { return pTrans.Refresh(); }
	virtual void Recalc() { value = pTrans->GetValue().forward.GetTranslation(); }
public:
	CExtractTranslation( CFuncBase<SFBTransform> *_p = 0 ) : pTrans(_p) {}
};

class CVec3Convert: public CFuncBase<CVec3>
{
	OBJECT_BASIC_METHODS(CVec3Convert);
protected:
	virtual bool NeedUpdate() { return pX.Refresh() | pY.Refresh() | pZ.Refresh(); }
	virtual void Recalc();
public:
	CDGPtr< CFuncBase<float> > pX;
	CDGPtr< CFuncBase<float> > pY;
	CDGPtr< CFuncBase<float> > pZ;
	//
	int operator&( CStructureSaver &f );
};

// testing dynamics node only
class CSinus: public CFuncBase<float>
{
	OBJECT_BASIC_METHODS(CSinus);
protected:
	virtual bool NeedUpdate() { return pTime.Refresh(); }
	virtual void Recalc();
public:
	float fpMn, fpFreq, fpAdd;
	CDGPtr< CFuncBase<STime> > pTime;
	//
	CSinus() { fpMn = 1; fpFreq = 1; fpAdd = 0; }
	int operator&( CStructureSaver &f );
};

}

#endif

