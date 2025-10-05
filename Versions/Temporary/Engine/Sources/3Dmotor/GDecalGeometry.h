#pragma once
#include "System/Dg.h"
#include "DiscretePos.h"
#include "3DLib/GGeometry.h"
namespace NGScene
{
class CObjectInfo;
const float F_DEPTH_WINDOW = 0.2f;

class CDecalGeometry: public CPtrFuncBase<CObjectInfo>
{
	OBJECT_BASIC_METHODS(CDecalGeometry);
	ZDATA
	CDGPtr<CPtrFuncBase<CObjectInfo> > pSource;
	CVec3 vOrigin, vNormal;
	CVec2 vSize;
	float fRotation;
	SDiscretePos srcPos;
	CVec2 vShift;
	float fNormalEdge, fDepthMargin;
public:
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pSource); f.Add(3,&vOrigin); f.Add(4,&vNormal); f.Add(5,&vSize); f.Add(6,&fRotation); f.Add(7,&srcPos); f.Add(8,&vShift); f.Add(9,&fNormalEdge); f.Add(10,&fDepthMargin); return 0; }
protected:
	virtual bool NeedUpdate() { return pSource.Refresh(); }
	virtual void Recalc();
public:
	CDecalGeometry() {}
	// srcPos transforms source geometry to space where _vOrigin, _vNormal are specified
	// rotation is in radians
	CDecalGeometry( CPtrFuncBase<CObjectInfo> *p, const SDiscretePos &_srcPos, 
		const CVec3 &_vOrigin, const CVec3 &_vNormal, const CVec2 &_vSize, float _fRotation, const CVec2 &_vShift = VNULL2,
		float fNormalEdge = -0.01f, float fDepthMargin = 1e10f );
};

class IPart;
class CPerPolyDecal : public CPtrFuncBase<CObjectInfo>
{
	ZDATA
	CPtr<IPart> pPart;
	CObjectInfo::SData data;
	std::vector<CVec3> srcPositions;
public:
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pPart); f.Add(3,&data); f.Add(4,&srcPositions); return 0; }
protected:
	virtual void Recalc();
	virtual void Recalc( CObjectInfo::SData *pRes, const CObjectInfo &info, const std::vector<CVec3> &positions ) = 0;
public:
	CPerPolyDecal() {}
	CPerPolyDecal( IPart *pPart, const std::vector<CVec3> &_srcPositions );
};

class CExplosionDecalGeometry : public CPerPolyDecal
{
	OBJECT_BASIC_METHODS(CExplosionDecalGeometry);
	ZDATA_(CPerPolyDecal)
	CVec3 vOrigin;
	float fSize, fRotation;
public:
	ZEND int operator&( CStructureSaver &f ) { f.Add(1,(CPerPolyDecal*)this); f.Add(2,&vOrigin); f.Add(3,&fSize); f.Add(4,&fRotation); return 0; }
protected:
	virtual void Recalc( CObjectInfo::SData *pRes, const CObjectInfo &info, const std::vector<CVec3> &positions );
public:
	CExplosionDecalGeometry() {}
	CExplosionDecalGeometry( IPart *_pPart, const std::vector<CVec3> &positions, const CVec3 &_vOrigin, float _fSize, float _fRotation )
		: CPerPolyDecal(_pPart, positions), vOrigin(_vOrigin), fSize(_fSize), fRotation(_fRotation) {}
};

class CPerPolyDecalGeometry: public CPerPolyDecal
{
	OBJECT_BASIC_METHODS(CPerPolyDecalGeometry);
	ZDATA_(CPerPolyDecal)
	CVec3 vOrigin, vNormal;
	CVec2 vSize;
	float fRotation;
	CVec2 vShift;
public:
	ZEND int operator&( CStructureSaver &f ) { f.Add(1,(CPerPolyDecal*)this); f.Add(2,&vOrigin); f.Add(3,&vNormal); f.Add(4,&vSize); f.Add(5,&fRotation); f.Add(6,&vShift); return 0; }
protected:
	virtual void Recalc( CObjectInfo::SData *pRes, const CObjectInfo &info, const std::vector<CVec3> &positions );
public:
	CPerPolyDecalGeometry() {}
	CPerPolyDecalGeometry( IPart *_pPart, const std::vector<CVec3> &positions,
		const CVec3 &_vOrigin, const CVec3 &_vNormal, const CVec2 &_vSize, float _fRotation, const CVec2 &_vShift = VNULL2 )
		: CPerPolyDecal(_pPart, positions), vOrigin(_vOrigin), vNormal(_vNormal), vSize(_vSize), fRotation(_fRotation), vShift(_vShift) {}
};

}

