#pragma once

#include "3Dmotor_export.h"

#include "GSkeleton.h"
#include "GChannelAnimator.h"
#include "../System/Time.hpp"


namespace NDb
{
	struct SSkeleton;
	struct SAnimBase;
}

namespace NAnimation
{

struct SAnimHandle
{
	ZDATA
	CDBPtr<NDb::SAnimBase> pAnimFile;
	int nAnimNumber;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pAnimFile); f.Add(3,&nAnimNumber); return 0; }
	SAnimHandle() : nAnimNumber(-1) {}
	SAnimHandle( const NDb::SAnimBase *pAnim, int _nAnimNumber ) : 
		pAnimFile( pAnim ), nAnimNumber( _nAnimNumber ) {}
};

interface IAnimMutator : public CObjectBase
{
	virtual bool NeedUpdate() { return true; }
	virtual void MutateSkeletonPose( granny_local_pose *pPose ) = 0;
};

interface ISkeletonAnimator : public CFuncBase<SGrannySkeletonPose>, public IChannelAnimator
{
	typedef int SAnimID; // Warning! SAnimID may become invalid after any recalc! Never remember SAnimID anywhere for long time

	// animation control:
	virtual SAnimID AddAnimation( STime tStartTime, const SAnimHandle &h, 
		bool bLoop, float fSpeed = 1.0f, float fWeight = 1.0f, STime tEndTime = -1 ) = 0;

	virtual void ClearAllAnimations() = 0;
	virtual void FadeIn( const STime &tDuration, SAnimID whatFadeIn ) = 0;
	virtual void FadeOut( const STime &tDuration, SAnimID whatFadeOut ) = 0;
	virtual void FadeOutAllAnimations( const STime &tDuration ) = 0;
	virtual void SetSpeedFactorForAllAnimations( const STime &tCurrent, float fSpeed ) = 0; // warning, doesn't affect mutators!
	virtual float GetDuration( const SAnimID animID ) = 0;
	virtual unsigned int GetMarkTimes( vector<float> *pResult, const SAnimID animID, const string &szMarkName ) = 0;
	virtual unsigned int EnumMarks( vector<string> *pResult, const SAnimID animID ) = 0;
	virtual void SetSpeedFactor( const SAnimID animID, float fSpeed ) = 0; // warning, doesn't affect mutators!
	virtual void SetLocalTime( const SAnimID animID, const STime tTime ) = 0;
	virtual void SetEndTime( const SAnimID animID, const STime tEndTime ) = 0;
	virtual void SetLoopCount( const SAnimID animID, const int nLoopCount ) = 0;
	virtual void SetGlobalAnimTransit( const STime tDuration ) = 0;

	// global movement
	virtual void SetGlobalPosition( const SHMatrix &pos ) = 0;
	virtual void SetGlobalTransform( CFuncBase<SFBTransform> *pTransform ) = 0;
	virtual void SetGlobMoveAnimation( const SAnimID animID, const float fMovementSpeed ) = 0;

	// locators
	virtual bool GetBonePosition( const char *pszBoneName, CVec3 *pResTranslation ) = 0;
	virtual bool GetBonePosition( int nBoneIndex, CVec3 *pResTranslation ) = 0;
	virtual bool GetBonePosition( const char *pszBoneName, SHMatrix *pRes ) = 0;
	virtual bool GetBonePosition( int nBoneIndex, SHMatrix *pRes ) = 0;
	virtual bool GetLocalBonePosition( const char *pszBoneName, SHMatrix *pLocalPos ) = 0;

	// mutators
	struct SDesiredBoneMove
	{
		const STime tDuration;
		const CQuat finalRot;
		const CVec3 finalPos;
		SDesiredBoneMove( const STime &_tDuration = 0, const CQuat &_finalRot = QNULL, const CVec3 &_finalPos = VNULL3 )
			: tDuration(_tDuration), finalRot(_finalRot), finalPos(_finalPos) {}
	};
	virtual void SetBoneMutator( const char *pszBoneName, const STime &tStart, 
		const vector<SDesiredBoneMove> &boneMutation ) = 0;
	virtual void SetBoneMutator( const int nBoneIndex, const STime &tStart, 
		const vector<SDesiredBoneMove> &boneMutation ) = 0;
	virtual void SetSpecialMutator( IAnimMutator *pMutator ) = 0;
	
	virtual CFuncBase<SFBTransform>* CreateTransform( const string &szBoneName ) = 0;
	virtual CFuncBase<SFBTransform>* CreateTransform( int nBoneIndex ) = 0;
};

_3DMOTOR_EXPORT ISkeletonAnimator *CreateSkeletonAnimator(
	const SGrannySkeletonHandle &skeleton, CFuncBase<STime> *_pTime );
} // namespace

