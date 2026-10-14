#pragma once
#include "GSkeleton.h"
#include "RectLayout.h"
#include "System/time.hpp"

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
DEFINE_DG_CONSTANT_NODE( CCWString, std::wstring );
DEFINE_DG_CONSTANT_NODE( CCRectLayout, CRectLayout );

//! Создаёт CCTRect
inline CCTRect *CreateRect( int x, int y, int width, int height )
{
	CCTRect *pRect = new CCTRect( CTRect<int>( x, y, x + width, y + height ) );
	return pRect;
}

// hierarchy forming node
class CMSRNode : public CFuncBase<SFBTransform>
{
	OBJECT_BASIC_METHODS( CMSRNode );

protected:
	bool NeedUpdate() override
	{
		return pAncestor.Refresh() | pPos.Refresh();
	}

	void Recalc() override;

public:
	CDGPtr<CFuncBase<SFBTransform>> pAncestor;
	CDGPtr<CFuncBase<SFBTransform>> pPos;

	//
	CMSRNode() {}

	int operator&( CStructureSaver &f ) override;
};

// hierarchy forming node
class CMNode : public CFuncBase<SFBTransform>
{
	OBJECT_BASIC_METHODS( CMNode );
	ZDATA
	CDGPtr<CFuncBase<SFBTransform>> pAncestor;
	CDGPtr<CFuncBase<CVec3>> pMove;

	ZEND int operator&( CStructureSaver &f ) override
	{
		f.Add( 2, &pAncestor );
		f.Add( 3, &pMove );
		return 0;
	}

protected:
	bool NeedUpdate() override
	{
		return pAncestor.Refresh() | pMove.Refresh();
	}

	void Recalc() override;

public:
	CMNode() {}

	CMNode( CFuncBase<SFBTransform> *_pA, CFuncBase<CVec3> *_pM )
		: pAncestor( _pA )
		, pMove( _pM )
	{
	}
};

// scale node
class CScaleNode : public CFuncBase<CTRect<int>>
{
	OBJECT_BASIC_METHODS( CScaleNode );

protected:
	bool NeedUpdate() override
	{
		return pSize.Refresh();
	}

	void Recalc() override;

public:
	CVec2 vScreenRect;
	CDGPtr<CFuncBase<CTRect<int>>> pSize;

	//
	CScaleNode() {}

	int operator&( CStructureSaver &f ) override;
};

// parameters 2 MSR converter
class CMSRConvert : public CFuncBase<SFBTransform>
{
	OBJECT_BASIC_METHODS( CMSRConvert );

protected:
	bool NeedUpdate() override
	{
		return pMove.Refresh() | pRotate.Refresh() | pScale.Refresh();
	}

	void Recalc() override;

public:
	CDGPtr<CFuncBase<CVec3>> pMove;
	CDGPtr<CFuncBase<CVec3>> pRotate;
	CDGPtr<CFuncBase<CVec3>> pScale;
	//
	int operator&( CStructureSaver &f ) override;
};

class CExtractTranslation : public CFuncBase<CVec3>
{
	OBJECT_BASIC_METHODS( CExtractTranslation );
	ZDATA
	CDGPtr<CFuncBase<SFBTransform>> pTrans;

	ZEND int operator&( CStructureSaver &f ) override
	{
		f.Add( 2, &pTrans );
		return 0;
	}

protected:
	bool NeedUpdate() override
	{
		return pTrans.Refresh();
	}

	void Recalc() override
	{
		value = pTrans->GetValue().forward.GetTranslation();
	}

public:
	CExtractTranslation( CFuncBase<SFBTransform> *_p = 0 )
		: pTrans( _p )
	{
	}
};

class CVec3Convert : public CFuncBase<CVec3>
{
	OBJECT_BASIC_METHODS( CVec3Convert );

protected:
	bool NeedUpdate() override
	{
		return pX.Refresh() | pY.Refresh() | pZ.Refresh();
	}

	void Recalc() override;

public:
	CDGPtr<CFuncBase<float>> pX;
	CDGPtr<CFuncBase<float>> pY;
	CDGPtr<CFuncBase<float>> pZ;
	//
	int operator&( CStructureSaver &f ) override;
};

// testing dynamics node only
class CSinus : public CFuncBase<float>
{
	OBJECT_BASIC_METHODS( CSinus );

protected:
	bool NeedUpdate() override
	{
		return pTime.Refresh();
	}

	void Recalc() override;

public:
	float fpMn, fpFreq, fpAdd;
	CDGPtr<CFuncBase<STime>> pTime;

	//
	CSinus()
	{
		fpMn = 1;
		fpFreq = 1;
		fpAdd = 0;
	}

	int operator&( CStructureSaver &f ) override;
};

} // namespace NGScene
