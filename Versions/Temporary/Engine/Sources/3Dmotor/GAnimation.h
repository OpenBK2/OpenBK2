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
	void operator()( granny_model_instance *pInstance ) const
	{
		GrannyFreeModelInstance( pInstance );
	}
};

struct SGrannyLocalPoseDeleter
{
	void operator()( granny_local_pose *pPose ) const
	{
		GrannyFreeLocalPose( pPose );
	}
};

struct SGrannyWorldPoseDeleter
{
	void operator()( granny_world_pose *pPose ) const
	{
		GrannyFreeWorldPose( pPose );
	}
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
	ZEND int operator&( IBinSaver &f )
	{
		f.Add( 2, &positions );
		f.Add( 3, &bEnabled );
		return 0;
	}

public:
	SSimpleBoneMutator()
		: bEnabled( false )
	{
	}

	void GetAtTime( const STime &t, CQuat *qRot, CVec3 *vPos );
	void AddBoneTimePose( const STime &tEnd, const CQuat &finalRot, const CVec3 &finalPos );
	void Clear();

	void Enable( bool bEnable )
	{
		bEnabled = bEnable;
	}

	bool IsEnabled() const
	{
		return bEnabled;
	}

	STime GetEnd() const;
};

class CSkeletonAnimator : public ISkeletonAnimator, public IGetBone
{
	struct STrackChannelBinding
	{
		granny_vector_track *pTrack;
		int nChannelIndex;

		STrackChannelBinding( granny_vector_track *_pTrack, int _nChannelIndex )
			: pTrack( _pTrack )
			, nChannelIndex( _nChannelIndex )
		{
		}
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

		ZEND int operator&( IBinSaver &f )
		{
			f.Add( 2, &tStartTime );
			f.Add( 5, &fSpeed );
			f.Add( 6, &fWeight );
			f.Add( 7, &bFadeIn );
			f.Add( 8, &bFadeOut );
			f.Add( 9, &tFadeDuration );
			f.Add( 10, &hAnimation );
			f.Add( 13, &tEndTime );
			f.Add( 14, &nLoopCount );
			return 0;
		}

		CDGPtr<CPtrFuncBase<CGrannyFileInfo>> pAnimFileLoader;
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

		SChannelByName( const std::string &_szName )
			: szName( _szName )
		{
		}

		bool operator()( const SScalarChannel &channel )
		{
			return ( channel.szName == szName );
		}
	};

	std::vector<SScalarChannel> scalarChannels;

	OBJECT_NOCOPY_METHODS( CSkeletonAnimator );
	// Every member carries a default initializer: Create() returns early when the skeleton
	// resource cannot be loaded, so no constructor may rely on it to assign these.
	SGrannySkeletonHandle skeletonH;
	CDGPtr<CFuncBase<STime>> pTime;
	CDGPtr<CFuncBase<SFBTransform>> pGlobalTransform;
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
	float fGlobalMovementSpeed = 0.f; // in meters per second.
	float fTransitHalfDuration = 0.f; // in seconds
	CDGPtr<CPtrFuncBase<CGrannyFileInfo>> pSkeletonFileLoader;
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

	bool NeedUpdate() override;
	void Recalc() override;

public:
	CSkeletonAnimator( const SSkeletonHandle &modelH, CFuncBase<STime> *_pTime );
	~CSkeletonAnimator() override;

	int operator&( CStructureSaver &f ) override;

	// IGetBone
	int GetBoneIndex( const char *pszName ) override;
	void GetBoneNames( std::vector<std::string> *pBoneNames ) override;

	// IChannelAnimator -- arbitrary scalar channel animation support
	int GetChannelCount() override;
	int GetChannelIndex( const std::string &szName ) override;
	float GetChannelValue( int nChannelIndex ) override;

	// ISkeletonAnimator
	SAnimID AddAnimation( STime tStartTime,
	                      const SAnimHandle &h,
	                      bool bLoop,
	                      float fSpeed = 1.0f,
	                      float fWeight = 1.0f,
	                      STime tEndTime = -1 ) override;

	void ClearAllAnimations() override;
	void FadeIn( const STime &tDuration, SAnimID whatFadeIn ) override;
	void FadeOut( const STime &tDuration, SAnimID whatFadeOut ) override;
	void FadeOutAllAnimations( const STime &tDuration ) override;
	void SetSpeedFactorForAllAnimations( const STime &tCurrent, float fSpeed ) override;
	float GetDuration( const SAnimID animID ) override;
	unsigned int
	GetMarkTimes( std::vector<float> *pResult, const SAnimID animID, const std::string &szMarkName ) override;
	unsigned int EnumMarks( std::vector<std::string> *pResult, const SAnimID animID ) override;
	void SetSpeedFactor( const SAnimID animID, float fSpeed ) override;
	void SetLocalTime( const SAnimID animID, const STime tTime ) override;
	void SetEndTime( const SAnimID animID, const STime tEndTime ) override;
	void SetLoopCount( const SAnimID animID, const int nLoopCount ) override;
	void SetGlobalAnimTransit( const STime tDuration ) override;

	void SetGlobalPosition( const SHMatrix &pos ) override;
	void SetGlobalTransform( CFuncBase<SFBTransform> *pTransform ) override;
	void SetGlobMoveAnimation( const SAnimID animID, const float fMovementSpeed ) override;

	void SetBoneMutator( const char *pszBoneName,
	                     const STime &tStart,
	                     const std::vector<SDesiredBoneMove> &boneMutation ) override;
	void SetBoneMutator( const int nBoneIndex,
	                     const STime &tStart,
	                     const std::vector<SDesiredBoneMove> &boneMutation ) override;
	void SetSpecialMutator( IAnimMutator *pMutator ) override;

	bool GetBonePosition( const char *pszBoneName, CVec3 *pResTranslation ) override;
	bool GetBonePosition( int nBoneIndex, CVec3 *pResTranslation ) override;
	bool GetBonePosition( const char *pszBoneName, SHMatrix *pRes ) override;
	bool GetBonePosition( int nBoneIndex, SHMatrix *pRes ) override;
	bool GetLocalBonePosition( const char *pszBoneName, SHMatrix *pLocalPos ) override;

	CFuncBase<SFBTransform> *CreateTransform( const std::string &szBoneName ) override;
	CFuncBase<SFBTransform> *CreateTransform( int nBoneIndex ) override;

	bool HasSkeleton() const
	{
		return pSkeleton != 0;
	}
};

} // namespace NAnimation
