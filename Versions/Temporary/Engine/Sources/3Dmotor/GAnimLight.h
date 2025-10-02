#ifndef __GAnimLight_H_
#define __GAnimLight_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "3Dmotor_export.h"


#include "..\System\Time.hpp"
#include "GParticleFormat.h"
namespace NDb
{
	struct SLightInstance;
	struct SAnimLight;
}

namespace NGScene
{

template <class TValue>
class TKeyVector
{
public:
	ZDATA
	vector< TKey<TValue> > keys;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&keys); return 0; }
	void GetValue( float fT, TValue *pRes ) const
	{
		int nKeys = keys.size();
		if ( nKeys == 1 )
		{
			*pRes = keys[0].value;
			return;
		}
		int s = 0, e = nKeys - 1;
		while( e - s > 1 )
		{
			int n = (s + e) / 2;
			if ( keys[n].nT <= fT )
				s = n;
			else
				e = n;
		}
		const TKey<TValue> &start = keys[s];
		const TKey<TValue> &end = keys[e];
		float fAlpha = (fT - start.nT) / (end.nT - start.nT);
		Interpolate( start.value, end.value, 1 - fAlpha, fAlpha, pRes );
	}
};

/*class CAnimLight: public CObjectBase
{
	OBJECT_BASIC_METHODS(CAnimLight);
public:
	CVec3 position;
	CVec3 color;
	float fRadius;
	bool bActive;
	bool bEnd;
};*/

class CAnimLightInfo: public CObjectBase
{
	OBJECT_BASIC_METHODS(CAnimLightInfo);
public:
	float fFrameRate;
	float fTStart;
	float fTEnd;
	TKeyVector<CVec3> pos;
	TKeyVector<CVec3> color;
	TKeyVector<float> radius;
};

class _3DMOTOR_EXPORT CLightLoader: public CLazyResourceLoader<CDBPtr<NDb::SAnimLight>, CAnimLightInfo>
{
	OBJECT_BASIC_METHODS(CLightLoader);
	virtual CFileRequest* CreateRequest();
	virtual void RecalcValue( CFileRequest *p );
};

class CAnimLight;
class _3DMOTOR_EXPORT CLightAnimator: public CPtrFuncBase<CAnimLight>
{
	OBJECT_BASIC_METHODS(CLightAnimator);
protected:
	virtual bool NeedUpdate() { return pTime.Refresh() | pInfo.Refresh() | pPlacement.Refresh(); }
	virtual void Recalc();
private:	
	ZDATA	
	float fScale;
	STime stBeginTime;
	CDBPtr<NDb::SLightInstance> pInstance;
public:
	CDGPtr< CFuncBase<STime> > pTime;
	CDGPtr< CFuncBase<SFBTransform> > pPlacement;
	CDGPtr< CPtrFuncBase<CAnimLightInfo> > pInfo;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&fScale); f.Add(3,&stBeginTime); f.Add(4,&pInstance); f.Add(5,&pTime); f.Add(6,&pPlacement); f.Add(7,&pInfo); return 0; }

	CLightAnimator() {}
	CLightAnimator( const NDb::SLightInstance *_pInstance, STime t, float _fScale ): pInstance(_pInstance), stBeginTime(t), fScale(_fScale) {}
};

}

#endif

