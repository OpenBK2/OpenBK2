#pragma once
#include "GAnimation.hpp"
#include "vendor/granny/include/granny.h"

#include <memory>

namespace NDb
{
	struct SSkeleton;
}

namespace NAnimation
{

// Granny hands out raw C handles with matching Granny*Free* calls. Wrapping them in unique_ptr
// makes the release automatic, so a partially constructed animator (Create() bails out whenever
// the skeleton resource is missing) can no longer reach a manual free of an unassigned handle.
struct SGrannyModelInstanceDeleter
{
	void operator()( granny_model_instance *pInstance ) const { GrannyFreeModelInstance( pInstance ); }
};
struct SGrannyLocalPoseDeleter
{
	void operator()( granny_local_pose *pPose ) const { GrannyFreeLocalPose( pPose ); }
};
struct SGrannyWorldPoseDeleter
{
	void operator()( granny_world_pose *pPose ) const { GrannyFreeWorldPose( pPose ); }
};

using TGrannyModelInstancePtr = std::unique_ptr<granny_model_instance, SGrannyModelInstanceDeleter>;
using TGrannyLocalPosePtr = std::unique_ptr<granny_local_pose, SGrannyLocalPoseDeleter>;
using TGrannyWorldPosePtr = std::unique_ptr<granny_world_pose, SGrannyWorldPoseDeleter>;

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
	std::vector<SBoneTimePose> positions;
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
		// AddAnimation() default-constructs a holder and assigns the fields it knows about, so
		// anything left out here would be serialized as an indeterminate value. tFadeDuration in
		// particular reaches save games and replays, which have to be byte-identical between peers.
		ZDATA
		STime tStartTime = 0;
		ZSKIP
		ZSKIP //bool bLoop;
		float fSpeed = 1.f;
		float fWeight = 1.f;
		bool bFadeIn = false, bFadeOut = false;
		STime tFadeDuration = 0;
		SAnimHandle hAnimation;
		ZSKIP
		ZSKIP
		STime tEndTime = STime( -1 );
		int nLoopCount = 0;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&tStartTime); f.Add(5,&fSpeed); f.Add(6,&fWeight); f.Add(7,&bFadeIn); f.Add(8,&bFadeOut); f.Add(9,&tFadeDuration); f.Add(10,&hAnimation); f.Add(13,&tEndTime); f.Add(14,&nLoopCount); return 0; }
		CDGPtr<CPtrFuncBase<CGrannyFileInfo> > pAnimFileLoader;
		granny_control *pControl = nullptr;
		std::vector<STrackChannelBinding> scalarTracks;
		granny_text_track *pAnnotationTrack = nullptr;
	};

	struct SScalarChannel
	{
		float fValue;
		float fWeight;
		std::string szName;
		bool bBinded;
	};
	struct SChannelByName
	{
		const std::string &szName;
		SChannelByName( const std::string &_szName ) : szName(_szName) {}
		bool operator()( const SScalarChannel &channel )
		{
			return (channel.szName == szName);
		}
	};
	std::vector<SScalarChannel> scalarChannels;

	OBJECT_NOCOPY_METHODS(CSkeletonAnimator);
	// Every member carries a default initializer: Create() returns early when the skeleton
	// resource cannot be loaded, so no constructor may rely on it to assign these.
	SGrannySkeletonHandle skeletonH;
	CDGPtr< CFuncBase<STime> > pTime;
	CDGPtr< CFuncBase<SFBTransform> > pGlobalTransform;
	granny_skeleton *pSkeleton = nullptr;
	granny_model model = {};
	TGrannyModelInstancePtr pModelInstance;
	TGrannyLocalPosePtr pGrannyPose;
	TGrannyWorldPosePtr pGlobalPose;
	int nBones = 0;
	CObj<IAnimMutator> pSpecMutator;
	std::vector<SSimpleBoneMutator> boneMutators;
	bool bBoneMutatorsEnabled = false;
	std::vector<SAnimationHolder> animHolders;
	SAnimID nAnimWithMovement = -1;
	float fGlobalMovementSpeed = 0.f;               // in meters per second.
	float fTransitHalfDuration = 0.f;               // in seconds
	CDGPtr<CPtrFuncBase<CGrannyFileInfo> > pSkeletonFileLoader;
	bool bGlobalPoseValid = false;
	bool bSmthChanged = true;
	bool bJustLoaded = false;

	void RecoverAnimHolder( SAnimID animID );
	bool AddAnimationInternal( SAnimationHolder *pH );
	void AddScalarTracks( SAnimationHolder *pH, granny_track_group *pTrackGroup );
	void AddAnnotationTrack( SAnimationHolder *pH, granny_track_group *pTrackGroup );

	void RefreshWorldPose();
	//void SetSpeedFactor( const STime &tCurrent, float fSpeed, SAnimID animID );
	void CheckJustLoaded();
	void Create( const SSkeletonHandle &_skeletonH, CFuncBase<STime> *_pTime );
	void CopyPoseFromGranny();
	void CopyPoseToGranny();
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
	CSkeletonAnimator( const SSkeletonHandle &modelH, CFuncBase<STime> *_pTime );
	~CSkeletonAnimator();

	int operator&( CStructureSaver &f );

	// IGetBone
	virtual int GetBoneIndex( const char *pszName );
	virtual void GetBoneNames( std::vector<std::string> *pBoneNames );

	// IChannelAnimator -- arbitrary scalar channel animation support
	virtual int GetChannelCount();
	virtual int GetChannelIndex( const std::string &szName );
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
	virtual unsigned int GetMarkTimes( std::vector<float> *pResult, const SAnimID animID, const std::string &szMarkName );
	virtual unsigned int EnumMarks( std::vector<std::string> *pResult, const SAnimID animID );
	virtual void SetSpeedFactor( const SAnimID animID, float fSpeed );
	virtual void SetLocalTime( const SAnimID animID, const STime tTime );
	virtual void SetEndTime( const SAnimID animID, const STime tEndTime );
	virtual void SetLoopCount( const SAnimID animID, const int nLoopCount );
	virtual void SetGlobalAnimTransit( const STime tDuration );

	virtual void SetGlobalPosition( const SHMatrix &pos );
	virtual void SetGlobalTransform( CFuncBase<SFBTransform> *pTransform );
	virtual void SetGlobMoveAnimation( const SAnimID animID, const float fMovementSpeed );

	virtual void SetBoneMutator( const char *pszBoneName, const STime &tStart, 
		const std::vector<SDesiredBoneMove> &boneMutation );
	virtual void SetBoneMutator( const int nBoneIndex, const STime &tStart, 
		const std::vector<SDesiredBoneMove> &boneMutation );
	virtual void SetSpecialMutator( IAnimMutator *pMutator );

	virtual bool GetBonePosition( const char *pszBoneName, CVec3 *pResTranslation );
	virtual bool GetBonePosition( int nBoneIndex, CVec3 *pResTranslation );
	virtual bool GetBonePosition( const char *pszBoneName, SHMatrix *pRes );
	virtual bool GetBonePosition( int nBoneIndex, SHMatrix *pRes );
	virtual bool GetLocalBonePosition( const char *pszBoneName, SHMatrix *pLocalPos );
	
	virtual CFuncBase<SFBTransform>* CreateTransform( const std::string &szBoneName );
	virtual CFuncBase<SFBTransform>* CreateTransform( int nBoneIndex );

	bool HasSkeleton() const { return pSkeleton != 0; }
};

} // namespace


