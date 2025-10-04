#pragma once

#include "System/Dg.h"
#include "System/time.hpp"

namespace NDb
{
	struct SParticleInstance;
}
namespace NGfx
{
	class CTexture;
}

namespace NGScene
{
class CParticleEffect;

class CParticlesInfo;
class IParticleFilter;
class CParticleAnimator: public CPtrFuncBase<CParticleEffect>
{
	OBJECT_BASIC_METHODS(CParticleAnimator);
protected:
	virtual bool NeedUpdate() { return pTime.Refresh() | pInfo.Refresh() | pPlacement.Refresh(); }
	virtual void Recalc();
private:	
	float GetRealTime( const STime &time );
	void CheckFirstTime();
	ZDATA
	STime stBeginTime;
	CDBPtr<NDb::SParticleInstance> pInstance;
	CDGPtr< CFuncBase<STime> > pTime;
	CDGPtr< CFuncBase<SFBTransform> > pPlacement;
	CDGPtr< CPtrFuncBase<CParticlesInfo> > pInfo;
public:
	vector<CObj<CPtrFuncBase<NGfx::CTexture> > > textureIDs;
private:
	CObj<IParticleFilter> pFilter;
	bool bLeaveParticlesWhereStarted;
	bool bPerParticleFog;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&stBeginTime); f.Add(3,&pInstance); f.Add(4,&pTime); f.Add(5,&pPlacement); f.Add(6,&pInfo); f.Add(7,&textureIDs); f.Add(8,&pFilter); f.Add(9,&bLeaveParticlesWhereStarted); f.Add(10,&bPerParticleFog); return 0; }
public:
	CParticleAnimator() {}
	CParticleAnimator( const NDb::SParticleInstance *_pInstance, STime t, IParticleFilter *_pFilter, 
		CFuncBase<STime> *_pTime, CPtrFuncBase<CParticlesInfo> *_pInfo, CFuncBase<SFBTransform> *_pPlacement,
		bool _bLeaveParticlesWhereStarted, bool _bPerParticleFog ) : pInstance(_pInstance), stBeginTime(t), pFilter(_pFilter), 
		pTime(_pTime), pInfo(_pInfo), pPlacement(_pPlacement),
		bLeaveParticlesWhereStarted(_bLeaveParticlesWhereStarted), bPerParticleFog(_bPerParticleFog) {}
	void StopParticlesGeneration( const STime &tStop ); 
};

}

