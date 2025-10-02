#ifndef __GSCENEVOLUME_H_
#define __GSCENEVOLUME_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GRenderCore.h"
namespace NDb
{
	struct SMaterial;
}

namespace NGScene
{
class IHZBuffer : public CObjectBase
{
public:
	virtual bool IsVisible( const SSphere &s, CTransformStack *pTS ) const = 0;
};

void GeneratePartList( IRender *pRender, const CVec3 &vCenter, float fRadius, 
	list<SRenderPartSet> *pRes, IRender::EDepthType eType, const SGroupSelect &mask );

typedef hash_map<CPtr<CObjectBase>,CPartFlags,SPtrHash> CIgnorePartsHash;

void MakeShadowVolumes( IRender *pRender, CTransformStack *pTS, const CVec3 &vCenter, 
	float fRadius, vector<STriangle> *pTris, 
	vector<CVec3> *pVertices, IRender::EDepthType eType, const SGroupSelect &mask,
	float *pHullRadius,
	CIgnorePartsHash *pIgnore = 0 );

void MakeInvisibleElementsListFast( IRender *pRender, CTransformStack *pTS, 
	const SGroupSelect &mask, const CVec2 &screenSize, CIgnorePartsHash *pIgnore, 
	CObj<IHZBuffer> *pHZBuffer );
void MakeInvisibleElementsList( IRender *pRender, CTransformStack *pTS, 
	const SGroupSelect &mask, const CVec2 &screenSize, CIgnorePartsHash *pIgnore, 
	CObj<IHZBuffer> *pHZBuffer );

} // namespace

#endif


