#pragma once

namespace NDb
{
	struct SComplexEffect;
}
namespace NAnimation
{
	struct ISkeletonAnimator;
}

class CSmokeTrailEffect : public CObjectBase
{
	OBJECT_BASIC_METHODS( CSmokeTrailEffect );
	//
	ZDATA
	CDBPtr<NDb::SComplexEffect> pEffect;
	float fInterval;
	SHMatrix mLocalPos;
	float fTimeLastUpdate;
	CVec3 vLastVisPos;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pEffect); f.Add(3,&fInterval); f.Add(4,&mLocalPos); f.Add(5,&fTimeLastUpdate); f.Add(6,&vLastVisPos); return 0; }
	//
	void CreateEffect( const CVec3 &vPos, const CQuat &qRot, NTimer::STime time, bool bVisible );
public:
	CSmokeTrailEffect() {}
	CSmokeTrailEffect( const SHMatrix &_mLocalPos, float _fInterval, const NDb::SComplexEffect *_pEffect,
		                 const CVec3 &vPos, const CQuat &qRot, NTimer::STime currTime, bool bVisible );
	//
	void UpdatePlacement( const CVec3 &vPos, const CQuat &qRot, NTimer::STime currTime, bool bVisible );
};

void CalcRelativePos( SHMatrix *pmRelativePos, const SHMatrix &mPos, const std::string &szBoneName, NAnimation::ISkeletonAnimator *pAnimator );


