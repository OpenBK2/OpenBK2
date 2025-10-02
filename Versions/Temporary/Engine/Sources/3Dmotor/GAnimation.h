#pragma once
#include "GAnimation.hpp"
#include "../vendor/Granny/include/granny.h"

namespace NDb
{
	struct SSkeleton;
}

namespace NAnimation
{

struct SSimpleBoneMutator
{
private:
	struct SBoneTimePose
	{
		CQuat rot;
		CVec3 pos;
		STime tEnd;
	};
	ZDATA
	vector<SBoneTimePose> positions;
	bool bEnabled;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&positions); f.Add(3,&bEnabled); return 0; }

public:
	SSimpleBoneMutator() : bEnabled(false) {}

	void GetAtTime( const STime &t, CQuat *qRot, CVec3 *vPos );
	void AddBoneTimePose( const STime &tEnd, 
		const CQuat &finalRot, const CVec3 &finalPos );
	void Clear();
	void Enable( bool bEnable )
	{
		bEnabled = bEnable;
	}
	bool IsEnabled() const { return bEnabled; }
	STime GetEnd() const;
};

class CSkeletonAnimator : public ISkeletonAnimator, public IGetBone
{
	struct STrackChannelBinding
	{
		granny_vector_track *pTrack;
		int nChannelIndex;
		STrackChannelBinding( granny_vector_track *_pTrack, int _nChannelIndex )
			: pTrack(_pTrack), nChannelIndex(_nChannelIndex)
		{}
	};

	struct SAnimationHolder
	{
		ZDATA
		STime tStartTime;
		ZSKIP
		ZSKIP //bool bLoop;
		float fSpeed;
		float fWeight;
		bool bFadeIn, bFadeOut;
		STime tFadeDuration;
		SAnimHandle hAnimation;
		ZSKIP
		ZSKIP
		STime tEndTime;
		int nLoopCount;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&tStartTime); f.Add(5,&fSpeed); f.Add(6,&fWeight); f.Add(7,&bFadeIn); f.Add(8,&bFadeOut); f.Add(9,&tFadeDuration); f.Add(10,&hAnimation); f.Add(13,&tEndTime); f.Add(14,&nLoopCount); return 0; }
		CDGPtr<CPtrFuncBase<CGrannyFileInfo> > pAnimFileLoader;
		granny_control *pControl;
		vector<STrackChannelBinding> scalarTracks;
		granny_text_track *pAnnotationTrack;
	};

	struct SScalarChannel
	{
		float fValue;
		float fWeight;
		string szName;
		bool bBinded;
	};
	struct SChannelByName
	{
		const string &szName;
		SChannelByName( const string &_szName ) : szName(_szName) {}
		bool operator()( const SScalarChannel &channel )
		{
			return (channel.szName == szName);
		}
	};
	vector<SScalarChannel> scalarChannels;

	OBJECT_NOCOPY_METHODS(CSkeletonAnimator);
	SGrannySkeletonHandle skeletonH;
	CDGPtr< CFuncBase<STime> > pTime;
	CDGPtr< CFuncBase<SFBTransform> > pGlobalTransform;
	granny_skeleton *pSkeleton;
	granny_model model;
	granny_model_instance *pModelInstance;
	granny_world_pose *pGlobalPose;
	int nBones;
	CObj<IAnimMutator> pSpecMutator;
	vector<SSimpleBoneMutator> boneMutators;
	bool bBoneMutatorsEnabled;
	vector<SAnimationHolder> animHolders;
	SAnimID nAnimWithMovement;
	float fGlobalMovementSpeed;                     // in meters per second.
	float fTransitHalfDuration;                     // in seconds
	CDGPtr<CPtrFuncBase<CGrannyFileInfo> > pSkeletonFileLoader;
	bool bGlobalPoseValid;
	bool bSmthChanged;
	bool bJustLoaded;

	void RecoverAnimHolder( SAnimID animID );
	bool AddAnimationInternal( SAnimationHolder *pH );
	void AddScalarTracks( SAnimationHolder *pH, granny_track_group *pTrackGroup );
	void AddAnnotationTrack( SAnimationHolder *pH, granny_track_group *pTrackGroup );

	void RefreshWorldPose();
	//void SetSpeedFactor( const STime &tCurrent, float fSpeed, SAnimID animID );
	void CheckJustLoaded();
	void Create( const SGrannySkeletonHandle &_skeletonH, CFuncBase<STime> *_pTime );
	void ClearAnimVector();
	void FreezeAllMutators();
	bool DoesWantToUpdate( const STime &t ) const;
	void Touch()
	{
		CheckJustLoaded();
		bSmthChanged = true;
	}
	void ApplyGlobalMovementCorrection();
	void RecalcScalarChannels();
	void SetGlobalPositionInternal( const SHMatrix &pos );

protected:
	CSkeletonAnimator();

	virtual bool NeedUpdate();
	virtual void Recalc();

public:
	CSkeletonAnimator( const SGrannySkeletonHandle &modelH, CFuncBase<STime> *_pTime );
	~CSkeletonAnimator();

	int operator&( CStructureSaver &f );

	// IGetBone
	virtual int GetBoneIndex( const char *pszName );
	virtual void GetBoneNames( vector<string> *pBoneNames );

	// IChannelAnimator -- arbitrary scalar channel animation support
	virtual int GetChannelCount();
	virtual int GetChannelIndex( const string &szName );
	virtual float GetChannelValue( int nChannelIndex );

	// ISkeletonAnimator
	virtual SAnimID AddAnimation( STime tStartTime, const SAnimHandle &h, 
		bool bLoop, float fSpeed = 1.0f, float fWeight = 1.0f, STime tEndTime = -1 );

	virtual void ClearAllAnimations();
	virtual void FadeIn( const STime &tDuration, SAnimID whatFadeIn );
	virtual void FadeOut( const STime &tDuration, SAnimID whatFadeOut );
	virtual void FadeOutAllAnimations( const STime &tDuration );
	virtual void SetSpeedFactorForAllAnimations( const STime &tCurrent, float fSpeed );
	virtual float GetDuration( const SAnimID animID );
	virtual unsigned int GetMarkTimes( vector<float> *pResult, const SAnimID animID, const string &szMarkName );
	virtual unsigned int EnumMarks( vector<string> *pResult, const SAnimID animID );
	virtual void SetSpeedFactor( const SAnimID animID, float fSpeed );
	virtual void SetLocalTime( const SAnimID animID, const STime tTime );
	virtual void SetEndTime( const SAnimID animID, const STime tEndTime );
	virtual void SetLoopCount( const SAnimID animID, const int nLoopCount );
	virtual void SetGlobalAnimTransit( const STime tDuration );

	virtual void SetGlobalPosition( const SHMatrix &pos );
	virtual void SetGlobalTransform( CFuncBase<SFBTransform> *pTransform );
	virtual void SetGlobMoveAnimation( const SAnimID animID, const float fMovementSpeed );

	virtual void SetBoneMutator( const char *pszBoneName, const STime &tStart, 
		const vector<SDesiredBoneMove> &boneMutation );
	virtual void SetBoneMutator( const int nBoneIndex, const STime &tStart, 
		const vector<SDesiredBoneMove> &boneMutation );
	virtual void SetSpecialMutator( IAnimMutator *pMutator );

	virtual bool GetBonePosition( const char *pszBoneName, CVec3 *pResTranslation );
	virtual bool GetBonePosition( int nBoneIndex, CVec3 *pResTranslation );
	virtual bool GetBonePosition( const char *pszBoneName, SHMatrix *pRes );
	virtual bool GetBonePosition( int nBoneIndex, SHMatrix *pRes );
	virtual bool GetLocalBonePosition( const char *pszBoneName, SHMatrix *pLocalPos );
	
	virtual CFuncBase<SFBTransform>* CreateTransform( const string &szBoneName );
	virtual CFuncBase<SFBTransform>* CreateTransform( int nBoneIndex );

	bool HasSkeleton() const { return pSkeleton != 0; }
};

} // namespace


