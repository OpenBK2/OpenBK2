#pragma once

#include "GAnimation.h"
#include "GltfFormat.h"

namespace NAnimation
{

// glTF implementation of the same animator contract used by the GR2 backend.
// Animation tracks are rebound to the separately loaded skeleton by node name.
class CGltfSkeletonAnimator : public ISkeletonAnimator, public IGetBone
{
	struct SAnimationHolder
	{
		STime tStartTime;
		STime tEndTime;
		STime tFadeStart;
		STime tFadeDuration;
		float fSpeed;
		float fWeight;
		float fDuration;
		// Derived from the current GLB resource during binding, so these are not serialized.
		float fSourceStart;
		int nAnimationIndex;
		bool bFadeIn;
		bool bFadeOut;
		int nLoopCount;
		SAnimHandle hAnimation;
		NGltf::TGltfFilePtr pFile;

		SAnimationHolder();
		int operator&( IBinSaver &f );
	};

	OBJECT_NOCOPY_METHODS(CGltfSkeletonAnimator);

	SSkeletonHandle skeletonH;
	CDGPtr<CFuncBase<STime> > pTime;
	CDGPtr<CFuncBase<SFBTransform> > pGlobalTransform;
	CObj<IAnimMutator> pSpecMutator;
	NGltf::TGltfFilePtr pSkeletonFile;
	NGltf::SSkeletonDefinition skeleton;
	std::vector<SAnimationHolder> animations;
	std::vector<SSimpleBoneMutator> boneMutators;
	bool bBoneMutatorsEnabled;
	bool bSmthChanged;
	bool bJustLoaded;
	SAnimID nAnimWithMovement;
	float fGlobalMovementSpeed;
	STime tTransitDuration;

	void Create( const SSkeletonHandle &_skeletonH, CFuncBase<STime> *_pTime );
	bool BindAnimation( SAnimationHolder *pHolder );
	float GetEffectiveWeight( const SAnimationHolder &holder, STime time ) const;
	float GetLocalTime( const SAnimationHolder &holder, STime time, bool *pActive ) const;
	void SetGlobalPositionInternal( const SHMatrix &position );
	void ApplyAnimation( const SAnimationHolder &holder, float localTime, float weight );
	void BuildMatrices();
	void ApplyBoneMutators( STime time );
	void CheckJustLoaded();
	void Touch() { CheckJustLoaded(); bSmthChanged = true; }

protected:
	bool NeedUpdate() override;
	void Recalc() override;

public:
	CGltfSkeletonAnimator();
	CGltfSkeletonAnimator( const SSkeletonHandle &_skeletonH, CFuncBase<STime> *_pTime );
	~CGltfSkeletonAnimator() override {}

	bool HasSkeleton() const { return !skeleton.boneNames.empty(); }
	int operator&( CStructureSaver &f );

	int GetBoneIndex( const char *pszName ) override;
	void GetBoneNames( std::vector<std::string> *pBoneNames ) override;

	int GetChannelCount() override { return 0; }
	int GetChannelIndex( const std::string & ) override { return INVALID_CHANNEL_ID; }
	float GetChannelValue( int ) override { return 0.0f; }

	SAnimID AddAnimation( STime tStartTime, const SAnimHandle &h,
		bool bLoop, float fSpeed = 1.0f, float fWeight = 1.0f, STime tEndTime = -1 ) override;
	void ClearAllAnimations() override;
	void FadeIn( const STime &tDuration, SAnimID whatFadeIn ) override;
	void FadeOut( const STime &tDuration, SAnimID whatFadeOut ) override;
	void FadeOutAllAnimations( const STime &tDuration ) override;
	void SetSpeedFactorForAllAnimations( const STime &tCurrent, float fSpeed ) override;
	float GetDuration( const SAnimID animID ) override;
	unsigned int GetMarkTimes( std::vector<float> *pResult, const SAnimID,
		const std::string & ) override;
	unsigned int EnumMarks( std::vector<std::string> *pResult, const SAnimID ) override;
	void SetSpeedFactor( const SAnimID animID, float fSpeed ) override;
	void SetLocalTime( const SAnimID animID, const STime tTime ) override;
	void SetEndTime( const SAnimID animID, const STime tEndTime ) override;
	void SetLoopCount( const SAnimID animID, const int nLoopCount ) override;
	void SetGlobalAnimTransit( const STime tDuration ) override;

	void SetGlobalPosition( const SHMatrix &position ) override;
	void SetGlobalTransform( CFuncBase<SFBTransform> *pTransform ) override;
	void SetGlobMoveAnimation( const SAnimID animID, const float fMovementSpeed ) override;

	bool GetBonePosition( const char *pszBoneName, CVec3 *pResult ) override;
	bool GetBonePosition( int nBoneIndex, CVec3 *pResult ) override;
	bool GetBonePosition( const char *pszBoneName, SHMatrix *pResult ) override;
	bool GetBonePosition( int nBoneIndex, SHMatrix *pResult ) override;
	bool GetLocalBonePosition( const char *pszBoneName, SHMatrix *pResult ) override;

	void SetBoneMutator( const char *pszBoneName, const STime &tStart,
		const std::vector<SDesiredBoneMove> &boneMutation ) override;
	void SetBoneMutator( const int nBoneIndex, const STime &tStart,
		const std::vector<SDesiredBoneMove> &boneMutation ) override;
	void SetSpecialMutator( IAnimMutator *pMutator ) override;

	CFuncBase<SFBTransform> *CreateTransform( const std::string &szBoneName ) override;
	CFuncBase<SFBTransform> *CreateTransform( int nBoneIndex ) override;
};

}
