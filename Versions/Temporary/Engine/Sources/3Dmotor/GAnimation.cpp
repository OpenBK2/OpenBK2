#include "stdafx.h"
#include "System/BasicShare.h"
#include "System/BinaryResources.h"
#include "GAnimFormat.h"
#include "vendor/granny/include/granny.h"
#include "GAnimation.h"
#include "GAnimUtils.h"
#include "GltfAnimation.h"
#include "DBScene.h"

#include <fmt/format.h>

namespace NAnimation
{
CBasicShare<CDBPtr<NDb::SAnimBase>, CGrannyAnimationLoader, SDBPtrHash> shareAnimations(104);


// SSimpleBoneMutator

void SSimpleBoneMutator::AddBoneTimePose( const STime &tEnd, 
		const CQuat &finalRot, const CVec3 &finalPos )
{
	SBoneTimePose pos;
	pos.pos = finalPos;
	pos.rot = finalRot;
	pos.tEnd = tEnd;
	positions.push_back( pos );
	Enable( true );
}

void SSimpleBoneMutator::Clear()
{
	positions.clear();
	Enable( false );
}

void SSimpleBoneMutator::GetAtTime( const STime &t, CQuat *qRot, CVec3 *vPos )
{
	ASSERT( IsEnabled() );
	if ( positions.empty() )
	{
		*qRot = QNULL;
		*vPos = VNULL3;
		return;
	}
	if ( t <= positions[0].tEnd )
	{
		*qRot = positions[0].rot;
		*vPos = positions[0].pos;
		return;
	}
	for ( int i = 1; i < positions.size(); ++i )
	{
		if ( t <= positions[i].tEnd )
		{
			float fFactor = ( t - positions[i-1].tEnd ) / (float)( positions[i].tEnd - positions[i-1].tEnd );
			qRot->Interpolate( positions[i-1].rot, positions[i].rot, fFactor );
			vPos->Interpolate( positions[i-1].pos, positions[i].pos, fFactor );
			return;
		}
	}
	int nMaxIndex = positions.size() - 1;
	*qRot = positions[ nMaxIndex ].rot;
	*vPos = positions[ nMaxIndex ].pos;	
}

STime SSimpleBoneMutator::GetEnd() const
{
	return positions.back().tEnd;
}


// CSkeletonAnimator

CSkeletonAnimator::CSkeletonAnimator()
{
	// Every other member has a default initializer in the header. An identity placement is the
	// one value a zero-initialized granny_model does not already carry.
	GrannyMakeIdentity( &model.InitialPlacement );
}

// Delegates to the default constructor so the two construction paths cannot drift apart. This
// matters because Create() gives up without assigning anything when the skeleton resource is
// missing, and CreateSkeletonAnimator() then destroys the half-built object.
CSkeletonAnimator::CSkeletonAnimator( const SSkeletonHandle &_skeletonH, CFuncBase<STime> *_pTime ) :
	CSkeletonAnimator()
{
	Create( _skeletonH, _pTime );

	// initializing mutators
	boneMutators.resize( nBones );
	for ( int i = 0; i < nBones; ++i )
		boneMutators[i].Enable( false );

	SHMatrix id;
	Identity( &id );
	SetGlobalPositionInternal( id );
}

void CSkeletonAnimator::Create( const SSkeletonHandle &_skeletonH, CFuncBase<STime> *_pTime )
{
	pTime = _pTime;
	bJustLoaded = false;
	skeletonH = _skeletonH;
	pSkeleton = 0;

	pSkeletonFileLoader = GetSkeletonFileInfo( _skeletonH.pSkeleton );
	if ( !pSkeletonFileLoader )
	{
		if ( _skeletonH.pSkeleton )
		{
			NI_ASSERT( 0, fmt::format("Can't create skeleton \"{}\"", _skeletonH.pSkeleton->GetDBID().ToString() ) );
		}
		else
		{
			NI_ASSERT( 0, "Can't create skeleton" );
		}
		return;
	}

	pSkeletonFileLoader.Refresh();
	pSkeleton = GetSkeleton( pSkeletonFileLoader->GetValue(), _skeletonH.nModelInFile );
	if ( !pSkeleton )
	{
		if ( _skeletonH.pSkeleton )
		{
			NI_ASSERT( 0, fmt::format("Can't create skeleton \"{}\"", _skeletonH.pSkeleton->GetDBID().ToString() ) );
		}
		else
		{
			NI_ASSERT( 0, "Can't create skeleton" );
		}
		return;
	}

	model.Name = GetModelNameOfSkeleton( pSkeletonFileLoader->GetValue(), _skeletonH.nModelInFile ); 
	model.Skeleton = pSkeleton;
	GrannyMakeIdentity( &model.InitialPlacement );
	model.MeshBindingCount = 0;
	model.MeshBindings = 0;
	pModelInstance.reset( GrannyInstantiateModel( &model ) );
	nBones = pSkeleton->BoneCount;
	value.localPose.resize( nBones );
	value.worldPose.resize( nBones );
	value.compositePose.resize( nBones );

	pGlobalPose.reset();
	bGlobalPoseValid = false;
	bSmthChanged = true;
}

void CSkeletonAnimator::ClearAnimVector()
{
	for ( int i = 0; i < animHolders.size(); ++i )
	{
		if ( animHolders[i].pControl )
			GrannyFreeControl( animHolders[i].pControl );
	}
	animHolders.clear();

	// scalar channels
	// В поддержку каналов, сделанных "на вырост", scalarChannels не должны очищаться.
	//scalarChannels.clear();

	// global movement
	nAnimWithMovement = -1;
	fGlobalMovementSpeed = 0.f;
}

CSkeletonAnimator::~CSkeletonAnimator()
{
	// ClearAnimVector() has to run first: the granny controls it frees are attached to
	// pModelInstance, which is released afterwards when the unique_ptr members are destroyed.
	CheckJustLoaded();
	ClearAnimVector();
}

// internal method to set position (without marking internal state as changed)
void CSkeletonAnimator::SetGlobalPositionInternal( const SHMatrix &mGlobal ) 
{ 
	value.poseGlobal[0] = mGlobal._11; value.poseGlobal[4] = mGlobal._12; value.poseGlobal[8] = mGlobal._13; value.poseGlobal[12] = mGlobal._14;
	value.poseGlobal[1] = mGlobal._21; value.poseGlobal[5] = mGlobal._22; value.poseGlobal[9] = mGlobal._23; value.poseGlobal[13] = mGlobal._24;
	value.poseGlobal[2] = mGlobal._31; value.poseGlobal[6] = mGlobal._32; value.poseGlobal[10] = mGlobal._33; value.poseGlobal[14] = mGlobal._34;
	value.poseGlobal[3] = mGlobal._41; value.poseGlobal[7] = mGlobal._42; value.poseGlobal[11] = mGlobal._43; value.poseGlobal[15] = mGlobal._44;
}

void CSkeletonAnimator::CopyPoseFromGranny()
{
	value.localPose.resize( nBones );
	for ( int i = 0; i < nBones; ++i )
	{
		const granny_transform *pSource = GrannyGetLocalPoseTransform( pGrannyPose.get(), i );
		SBoneTransform &target = value.localPose[i];
		if ( !pSource )
			continue;
		target.Flags = pSource->Flags;
		memcpy( target.Position, pSource->Position, sizeof(target.Position) );
		memcpy( target.Orientation, pSource->Orientation, sizeof(target.Orientation) );
		memcpy( target.ScaleShear, pSource->ScaleShear, sizeof(target.ScaleShear) );
	}
}

void CSkeletonAnimator::CopyPoseToGranny()
{
	for ( int i = 0; i < nBones; ++i )
	{
		granny_transform *pTarget = GrannyGetLocalPoseTransform( pGrannyPose.get(), i );
		const SBoneTransform *pSource = value.GetBone( i );
		if ( !pTarget || !pSource )
			continue;
		pTarget->Flags = pSource->Flags;
		memcpy( pTarget->Position, pSource->Position, sizeof(pTarget->Position) );
		memcpy( pTarget->Orientation, pSource->Orientation, sizeof(pTarget->Orientation) );
		memcpy( pTarget->ScaleShear, pSource->ScaleShear, sizeof(pTarget->ScaleShear) );
	}
}

void CSkeletonAnimator::SetGlobalPosition( const SHMatrix &mGlobal ) 
{ 
	SetGlobalPositionInternal( mGlobal );
	bSmthChanged = true;
}

void CSkeletonAnimator::SetGlobalTransform( CFuncBase<SFBTransform> *pTransform )
{
	pGlobalTransform = pTransform;
	bSmthChanged = true;
}

void CSkeletonAnimator::SetSpecialMutator( IAnimMutator *pMutator )
{
	pSpecMutator = pMutator;
	bSmthChanged = true;
}


void CSkeletonAnimator::RecalcScalarChannels()
{
	//1)
	//foreach active anim control
	//	get control's speed?, effective weight, local clock
	//	foreach scalar track of control
	//		evaluate track using control's speed and clock
	//		multiply track's result value by control's weight
	//		add resulting weighted value to value sum under corresponding channel number
	//		add control's weight to weight sum under corresponding channel number
	//foreach channel
	//	divide summated value by summated weight
	for ( int i = 0; i < scalarChannels.size(); ++i )
	{
		scalarChannels[i].fValue = 0.f;
		scalarChannels[i].fWeight = 0.f;
	}
	for ( int id = 0; id < animHolders.size(); ++id )
	{
		const SAnimationHolder &holder = animHolders[id];
		if ( !GrannyControlIsComplete( holder.pControl ) )
		{
			const float fLocalTime = GrannyGetControlClampedLocalClock( holder.pControl );
			const float fWeight = GrannyGetControlEffectiveWeight( holder.pControl );

			float fValue = 0.f;
			for( int trackId = 0; trackId < holder.scalarTracks.size(); ++trackId )
			{
				const STrackChannelBinding &channelBinding = holder.scalarTracks[trackId];
				SScalarChannel &channel = scalarChannels[channelBinding.nChannelIndex];

				GrannyEvaluateCurveAtT( 1, false, false, &channelBinding.pTrack->ValueCurve, false, fLocalTime, 0, &fValue, nullptr );
				channel.fValue += fValue * fWeight;
				channel.fWeight += fWeight;
			}
		}
	}
	for ( int i = 0; i < scalarChannels.size(); ++i )
	{
		if ( scalarChannels[i].fWeight > FP_EPSILON )
		{
			scalarChannels[i].fValue = scalarChannels[i].fValue / scalarChannels[i].fWeight;
		}
		else
		{
			scalarChannels[i].fValue = 0.f;
		}
	}

	//2)
	//foreach channel
	//	foreach affecting track
	//		get control's speed?, effective weight, local clock
	//		evaluate track using control's speed and clock
	//		multiply track's result value by control's weight
	//		add resulting weighted value to value sum
	//		add control's weight to weight sum
	//	divide summated value by summated weight
}


bool CSkeletonAnimator::NeedUpdate()
{
	CheckJustLoaded();
	bool bNewTime = pTime.Refresh(), bNewGP = false;
	if ( pGlobalTransform )
		bNewGP = pGlobalTransform.Refresh();
	if ( !pGrannyPose )
		return true;
	if ( GrannyGetLocalPoseBoneCount( pGrannyPose.get() ) != nBones )
		return true;
	if ( !bNewTime && !bNewGP )
		return false;
	if ( bSmthChanged )
	{
		bSmthChanged = false;
		return true;
	}
	else
	{
		return bNewGP || DoesWantToUpdate( pTime->GetValue() );
	}
}

void CSkeletonAnimator::Recalc()
{
	CheckJustLoaded();
	STime time = pTime->GetValue();

	if ( pGlobalTransform )
	{
		pGlobalTransform.Refresh();
		SetGlobalPositionInternal( pGlobalTransform->GetValue().forward );
	}

	if ( !pGrannyPose )
		pGrannyPose.reset( GrannyNewLocalPose( nBones ) );

	bGlobalPoseValid = false;

	GrannySetModelClock( pModelInstance.get(), 0.001f * time );
	for ( int id = 0; id < animHolders.size(); ++id )
	{
		if ( GrannyControlIsComplete( animHolders[id].pControl ) )
			GrannySetControlActive( animHolders[id].pControl, false );
	}
	GrannySampleModelAnimations( pModelInstance.get(), 0, nBones, pGrannyPose.get() );

	if ( nAnimWithMovement != -1 )
	{
		const SAnimationHolder &h = animHolders[nAnimWithMovement];
		if ( !GrannyControlIsComplete( h.pControl ) )
		{
			if ( h.tStartTime <= time )
				ApplyGlobalMovementCorrection();
		}
		else
			nAnimWithMovement = -1;
	}

	RecalcScalarChannels();

	if ( bBoneMutatorsEnabled )
	{
		for ( int i = 0; i < nBones; ++i )
		{
			if ( boneMutators[i].IsEnabled() )
			{
				CQuat qRot;
				CVec3 vPos;
				boneMutators[i].GetAtTime( time, &qRot, &vPos );
				granny_transform *pBoneTransform = GrannyGetLocalPoseTransform( pGrannyPose.get(), i );
				if ( pBoneTransform ) 
				{
					granny_transform tr;
					GrannyMakeIdentity( &tr );
					memcpy( tr.Position, &vPos, 3 * sizeof( float ) );
					memcpy( tr.Orientation, &qRot, 4 * sizeof( float ) );
					GrannyPostMultiplyBy( pBoneTransform, &tr );
					pBoneTransform->Flags |= GrannyHasPosition;
					pBoneTransform->Flags |= GrannyHasOrientation;
				}
			}
		}
	}

	// Special mutators now operate on the same engine pose for every format.
	CopyPoseFromGranny();
	if ( IsValid( pSpecMutator ) )
	{
		pSpecMutator->MutateSkeletonPose( &value );
		CopyPoseToGranny();
	}
	else
		pSpecMutator = 0;

	// Publish world and skinning matrices as part of the neutral pose value.
	RefreshWorldPose();
}

void CSkeletonAnimator::ApplyGlobalMovementCorrection()
{
	SAnimationHolder &h = animHolders[ nAnimWithMovement ]; 
	float fLocalTime = GrannyGetControlClampedLocalClock( h.pControl );
	float fEffectiveWeight = GrannyGetControlEffectiveWeight( h.pControl );
	float fSpeed = GrannyGetControlSpeed( h.pControl );

	CVec3 hipMoveCorrection;
	const float y = (fGlobalMovementSpeed * fLocalTime) * fEffectiveWeight / fSpeed;
	hipMoveCorrection.x = value.poseGlobal[4] * y;
	hipMoveCorrection.y = value.poseGlobal[5] * y;
	hipMoveCorrection.z = value.poseGlobal[6] * y;

	value.poseGlobal[12] += hipMoveCorrection.x;
	value.poseGlobal[13] += hipMoveCorrection.y;
	value.poseGlobal[14] += hipMoveCorrection.z;
}

void CSkeletonAnimator::AddScalarTracks( SAnimationHolder *pH, granny_track_group *pTrackGroup )
{
	for ( int i = 0; i < pTrackGroup->VectorTrackCount; ++i )
	{
		granny_vector_track &scalarTrack = pTrackGroup->VectorTracks[i];

		// согласно схеме оптимизации запросов GetChannelIndex всегда возвращает валидный индекс
		int nChannelIndex = GetChannelIndex( scalarTrack.Name );
		ASSERT( nChannelIndex != INVALID_CHANNEL_ID );
		scalarChannels[nChannelIndex].bBinded = true;

		pH->scalarTracks.push_back( STrackChannelBinding(&scalarTrack, nChannelIndex) );
	}
}

void CSkeletonAnimator::AddAnnotationTrack( SAnimationHolder *pH, granny_track_group *pTrackGroup )
{
	// FIXME: отбирать трэк по имени
	if ( pTrackGroup->TextTrackCount > 0 )
	{
		granny_text_track &textTrack = pTrackGroup->TextTracks[0];
		pH->pAnnotationTrack = &textTrack;
	}
}

bool CSkeletonAnimator::AddAnimationInternal( CSkeletonAnimator::SAnimationHolder *pH )
{
	SAnimationHolder &newHolder = *pH;
	if ( newHolder.hAnimation.pAnimFile &&
		!newHolder.hAnimation.pAnimFile->GetModelFileRef().empty() )
		return false;

	newHolder.pAnimFileLoader = shareAnimations.Get( newHolder.hAnimation.pAnimFile );
	newHolder.pAnimFileLoader.Refresh();
	ASSERT( newHolder.pAnimFileLoader->GetValue() );
	granny_animation *pAnimation = 0;
	if ( newHolder.pAnimFileLoader->GetValue() )
	{
		granny_file_info *pFileInfo = newHolder.pAnimFileLoader->GetValue()->GetData();
		if ( pFileInfo
				&& newHolder.hAnimation.nAnimNumber >= 0
				&& newHolder.hAnimation.nAnimNumber < pFileInfo->AnimationCount )
		{
			pAnimation = pFileInfo->Animations[ newHolder.hAnimation.nAnimNumber ];
		}
	}
	if ( !pAnimation )
	{
		if ( newHolder.hAnimation.pAnimFile )
			DebugTrace("GAnimation: failed to load animation resource %s", newHolder.hAnimation.pAnimFile->GetDBID().ToString().c_str() );
		return false;
	}

	//newHolder.pControl = GrannyPlayControlledAnimation( newHolder.tStartTime * 0.001f, pAnimation, pModelInstance );
/**/
	{
		granny_controlled_animation_builder *pBuilder = GrannyBeginControlledAnimation(newHolder.tStartTime / 1000.0f, pAnimation);

		const int nTrackGroupIndex = 0;
		GrannySetTrackGroupTarget(pBuilder, nTrackGroupIndex, pModelInstance.get());
		GrannySetTrackGroupAccumulation(pBuilder, nTrackGroupIndex, GrannyNoAccumulation);

		granny_track_mask *pModelMask = GrannyNewTrackMask( 1.0, pSkeleton->BoneCount );
		// Set the track mask to 1 everywhere the granny_track_group has animated
		// data, and 0 everywhere it is constant or has no data whatsoever.
		granny_real32 fIdentityValue = 1.0f;
		granny_real32 fConstantValue = 1.0f;
		granny_real32 fAnimatedValue = 1.0f;
		GrannySetSkeletonTrackMaskFromTrackGroup( pModelMask,
				pSkeleton, pAnimation->TrackGroups[nTrackGroupIndex],
				fIdentityValue, fConstantValue, fAnimatedValue
				);
		GrannySetTrackGroupModelMask(pBuilder, nTrackGroupIndex, pModelMask);

		newHolder.pControl = GrannyEndControlledAnimation(pBuilder);
	}
/**/

	if ( !newHolder.pControl )
		return false;

	if ( fTransitHalfDuration > 0.f )
	{
		const float fHalfTransit = fTransitHalfDuration;
		const float fStartTime = newHolder.tStartTime * 0.001f;
		const float fEndTime = newHolder.tEndTime * 0.001f;

		GrannySetControlEaseIn( newHolder.pControl, true );
		GrannySetControlEaseInCurve( newHolder.pControl, fStartTime - fHalfTransit, fStartTime + fHalfTransit,
				0.f, 0.f, 1.f, newHolder.fWeight
				);
		if ( newHolder.tEndTime != -1 )
		{
			GrannySetControlEaseOut( newHolder.pControl, true );
			GrannySetControlEaseOutCurve( newHolder.pControl, fEndTime - fHalfTransit, fEndTime + fHalfTransit,
					newHolder.fWeight, 1.f, 0.f, 0.f
					);
		}
	}

	GrannySetControlSpeed( newHolder.pControl, newHolder.fSpeed );
	GrannySetControlLoopCount( newHolder.pControl, newHolder.nLoopCount );
	GrannySetControlForceClampedLooping( newHolder.pControl, true );

	if ( newHolder.tEndTime != -1 )
		GrannyCompleteControlAt( newHolder.pControl, newHolder.tEndTime * 0.001f );
	else
		GrannyFreeControlOnceUnused( newHolder.pControl );

	{
		const int nTrackGroupIndex = 0;
		granny_track_group *pTrackGroup = pAnimation->TrackGroups[nTrackGroupIndex];
		AddScalarTracks( &newHolder, pTrackGroup );
		AddAnnotationTrack( &newHolder, pTrackGroup );
	}

	return true;
}

CSkeletonAnimator::SAnimID CSkeletonAnimator::AddAnimation(
		STime tStartTime, const SAnimHandle &h, 
		bool bLoop, float fSpeed, float fWeight, STime tEndTime )
{
	Touch();

	SAnimationHolder newHolder;
	newHolder.hAnimation = h;
	newHolder.tStartTime = tStartTime;
	newHolder.nLoopCount = (bLoop ? 0 : 1);
	newHolder.fSpeed = fSpeed;
	newHolder.fWeight = fWeight;
	newHolder.bFadeIn = newHolder.bFadeOut = false;
	newHolder.tEndTime = tEndTime;
	newHolder.pAnnotationTrack = 0;

	if ( AddAnimationInternal( &newHolder ) )
	{
		animHolders.push_back( newHolder );
		return animHolders.size() - 1;
	}
	else
		return -1;
}

void CSkeletonAnimator::SetGlobMoveAnimation( const SAnimID animID, const float fMovementSpeed )
{
	NI_ASSERT( (animID >= 0 && animID < animHolders.size()), fmt::format("Invalid anim index \"{}\"", animID) );
	if ( animID >= 0 && animID < animHolders.size() )
	{
		NI_ASSERT( nAnimWithMovement == -1, "Only one globmove animation at a time supported" );
		if ( nAnimWithMovement == -1 )
		{
			nAnimWithMovement = animID;
			fGlobalMovementSpeed = fMovementSpeed * 1000.f;
		}
	}
}

void CSkeletonAnimator::SetGlobalAnimTransit( const STime tDuration )
{
	fTransitHalfDuration = tDuration * 0.001f / 2;
}

void CSkeletonAnimator::FadeIn( const STime &tDuration, SAnimID id )
{
	Touch();
	if ( id < 0 || id >= animHolders.size() )
	{
		ASSERT(0);
		return;
	}
	SAnimationHolder &h = animHolders[id]; 
	ASSERT( !h.bFadeIn && !h.bFadeOut );
	h.bFadeIn = true;
	h.tFadeDuration = tDuration;
	GrannyEaseControlIn( h.pControl, tDuration / 1000.0f, false );
}

void CSkeletonAnimator::FadeOut( const STime &tDuration, SAnimID id )
{
	Touch();
	if ( id < 0 || id >= animHolders.size() )
	{
		ASSERT(0);
		return;
	}
	SAnimationHolder &h = animHolders[id]; 
	//ASSERT( !h.bFadeIn && !h.bFadeOut );
	h.bFadeOut = true;
	h.tFadeDuration = tDuration;
	GrannyEaseControlOut( animHolders[id].pControl, tDuration / 1000.0f );
}

void CSkeletonAnimator::FadeOutAllAnimations( const STime &tDuration )
{
	Touch();
	for ( int id = 0; id < animHolders.size(); ++id )
		FadeOut( tDuration, id );
}

/*void CSkeletonAnimator::SetSpeedFactor( const STime &tCurrent, float fSpeed, SAnimID animID )
{
	Touch();
	ASSERT( fSpeed > 0 );
	if ( animID < 0 || animID >= animHolders.size() )
	{
		ASSERT(0);
		return;
	}
	SAnimationHolder &h = animHolders[ animID ];
	float fOldSpeed = h.fSpeed;
	h.fSpeed = fSpeed;
	GrannySetControlSpeed( h.pControl, h.fSpeed );
}*/

void CSkeletonAnimator::SetSpeedFactorForAllAnimations( const STime &tCurrent, float fSpeed )
{
	Touch();
	for ( int id = 0; id < animHolders.size(); ++id )
		SetSpeedFactor( id, fSpeed );
}


float CSkeletonAnimator::GetDuration( const SAnimID animID )
{
	if ( animID < 0 || animID >= animHolders.size() )
	{
		ASSERT(0);
		return 0.f;
	}
	SAnimationHolder &h = animHolders[ animID ];
	// control speed accounted internally in GrannyGetControlDuration
	return GrannyGetControlDuration( h.pControl );
}


unsigned int CSkeletonAnimator::GetMarkTimes( std::vector<float> *pResult, const SAnimID animID, const std::string &szMarkName )
{
	ASSERT( pResult );
	pResult->clear();

	if ( animID < 0 || animID >= animHolders.size() )
	{
		ASSERT(0);
		return 0;
	}
	SAnimationHolder &h = animHolders[ animID ];
	if ( h.pAnnotationTrack )
	{
		const granny_text_track *pTrack = h.pAnnotationTrack;
		for ( int i = 0; i < pTrack->EntryCount; ++i )
		{
			const granny_text_track_entry &entry = pTrack->Entries[i];
			if ( szMarkName == entry.Text )
			{
				float fSpeed = GrannyGetControlSpeed( h.pControl );
				float fPointTime = entry.TimeStamp / fSpeed;
				pResult->push_back( fPointTime );
			}
		}
	}

	return pResult->size();
}


unsigned int CSkeletonAnimator::EnumMarks( std::vector<std::string> *pResult, const SAnimID animID )
{
	ASSERT( pResult );
	pResult->clear();

	if ( animID < 0 || animID >= animHolders.size() )
	{
		ASSERT( animID >= 0 && animID < animHolders.size() );
		return 0;
	}
	SAnimationHolder &h = animHolders[ animID ];
	if ( h.pAnnotationTrack )
	{
		std::vector<std::string> &marks = *pResult;

		const granny_text_track *pTrack = h.pAnnotationTrack;
		for ( int i = 0; i < pTrack->EntryCount; ++i )
		{
			const granny_text_track_entry &entry = pTrack->Entries[i];
			std::vector<std::string>::iterator markIt = find( marks.begin(), marks.end(), entry.Text );
			if ( markIt == marks.end() )
			{
				marks.push_back( entry.Text );
			}
		}
	}

	return pResult->size();
}


void CSkeletonAnimator::SetSpeedFactor( const SAnimID animID, float fSpeed )
{
	if ( animID < 0 || animID >= animHolders.size() )
	{
		ASSERT(0);
		return;
	}
	SAnimationHolder &h = animHolders[ animID ];
	h.fSpeed = fSpeed;
	GrannySetControlSpeed( h.pControl, fSpeed );
}

void CSkeletonAnimator::SetLocalTime( const SAnimID animID, const STime tTime )
{
	if ( animID < 0 || animID >= animHolders.size() )
	{
		ASSERT(0);
		return;
	}
	SAnimationHolder &h = animHolders[ animID ];
	float fSpeed = GrannyGetControlSpeed( h.pControl );
	float fTime = tTime * 0.001f * fSpeed;
	GrannySetControlRawLocalClock( h.pControl, fTime );
}

void CSkeletonAnimator::SetEndTime( const SAnimID animID, const STime tEndTime )
{
	if ( animID < 0 || animID >= animHolders.size() )
	{
		ASSERT(0);
		return;
	}
	SAnimationHolder &h = animHolders[ animID ];
	h.tEndTime = tEndTime;
	if ( tEndTime != -1 )
		GrannyCompleteControlAt( h.pControl, tEndTime / 1000.0f );
	else
		GrannyFreeControlOnceUnused( h.pControl );
}

void CSkeletonAnimator::SetLoopCount( const SAnimID animID, const int nLoopCount )
{
	if ( animID < 0 || animID >= animHolders.size() )
	{
		ASSERT(0);
		return;
	}
	SAnimationHolder &h = animHolders[ animID ];
	h.nLoopCount = nLoopCount;
	GrannySetControlLoopCount( h.pControl, nLoopCount );
}

int CSkeletonAnimator::GetBoneIndex( const char *pszName )
{
	CheckJustLoaded();
	int nBoneIndex = -1;
	if( GrannyFindBoneByName( pSkeleton, pszName, &nBoneIndex ) )
		return nBoneIndex;
	else
		return -1;
}

void CSkeletonAnimator::GetBoneNames( std::vector<std::string> *pBoneNames )
{
	if ( pSkeleton )
	{
		pBoneNames->reserve( pSkeleton->BoneCount );
		for ( int i = 0; i < pSkeleton->BoneCount; ++i )
			pBoneNames->push_back( pSkeleton->Bones[i].Name );
	}
}

void CSkeletonAnimator::SetBoneMutator( const char *pszBoneName, const STime &tStart, 
	const std::vector<SDesiredBoneMove> &boneMutation )
{
	Touch();
	int nBoneIndex;
	if ( !GrannyFindBoneByName( pSkeleton, pszBoneName, &nBoneIndex ) )
		return;
	SetBoneMutator( nBoneIndex, tStart, boneMutation );
}

void CSkeletonAnimator::SetBoneMutator( const int nBoneIndex, const STime &tStart, 
																			 const std::vector<SDesiredBoneMove> &boneMutation )
{
	Touch();
	if ( nBoneIndex < 0 || nBoneIndex >= nBones ) 
	{
		ASSERT(0); 
		return;
	}
	bBoneMutatorsEnabled = true;
	CQuat currRot; CVec3 currPos;
	if ( boneMutators[ nBoneIndex ].IsEnabled() )
		boneMutators[ nBoneIndex ].GetAtTime( tStart, &currRot, &currPos );
	else
	{
		currRot = QNULL; 
		currPos = VNULL3;
	}
	boneMutators[ nBoneIndex ].Clear();
	boneMutators[ nBoneIndex ].AddBoneTimePose( tStart, currRot, currPos );
	STime tEnd = tStart;
	for ( int i = 0; i < boneMutation.size(); ++i )
	{
		tEnd += boneMutation[ i ].tDuration;
		boneMutators[ nBoneIndex ].AddBoneTimePose( tEnd, boneMutation[ i ].finalRot, boneMutation[ i ].finalPos );
	}
}

void CSkeletonAnimator::RefreshWorldPose()
{
	if ( bGlobalPoseValid )
		return;
	bGlobalPoseValid = true;
	if ( !pGlobalPose )
		pGlobalPose.reset( GrannyNewWorldPose( nBones ) );
	GrannyBuildWorldPose( pSkeleton, 0, nBones, pGrannyPose.get(), value.poseGlobal, pGlobalPose.get() );
	value.worldPose.resize( nBones );
	value.compositePose.resize( nBones );
	for ( int i = 0; i < nBones; ++i )
	{
		const granny_real32 *pWorld = GrannyGetWorldPose4x4( pGlobalPose.get(), i );
		const granny_real32 *pComposite = GrannyGetWorldPoseComposite4x4( pGlobalPose.get(), i );
		if ( pWorld )
		{
			SHMatrix &m = value.worldPose[i];
			m._11 = pWorld[0]; m._12 = pWorld[4]; m._13 = pWorld[8]; m._14 = pWorld[12];
			m._21 = pWorld[1]; m._22 = pWorld[5]; m._23 = pWorld[9]; m._24 = pWorld[13];
			m._31 = pWorld[2]; m._32 = pWorld[6]; m._33 = pWorld[10]; m._34 = pWorld[14];
			m._41 = pWorld[3]; m._42 = pWorld[7]; m._43 = pWorld[11]; m._44 = pWorld[15];
		}
		if ( pComposite )
		{
			SHMatrix &m = value.compositePose[i];
			m._11 = pComposite[0]; m._12 = pComposite[4]; m._13 = pComposite[8]; m._14 = pComposite[12];
			m._21 = pComposite[1]; m._22 = pComposite[5]; m._23 = pComposite[9]; m._24 = pComposite[13];
			m._31 = pComposite[2]; m._32 = pComposite[6]; m._33 = pComposite[10]; m._34 = pComposite[14];
			m._41 = pComposite[3]; m._42 = pComposite[7]; m._43 = pComposite[11]; m._44 = pComposite[15];
		}
	}
}

bool CSkeletonAnimator::GetBonePosition( int nBoneIndex, SHMatrix *pRes )
{
	CheckJustLoaded();
	if ( nBoneIndex < 0 || nBoneIndex >= nBones )
		return false;
	RefreshWorldPose();
	if ( nBoneIndex >= static_cast<int>(value.worldPose.size()) )
		return false;
	*pRes = value.worldPose[nBoneIndex];
	return true;
}

bool CSkeletonAnimator::GetBonePosition( int nBoneIndex, CVec3 *pResTranslation )
{
	CheckJustLoaded();
	if ( nBoneIndex < 0 || nBoneIndex >= nBones )
		return false;
	RefreshWorldPose();
	if ( nBoneIndex >= static_cast<int>(value.worldPose.size()) )
		return false;
	pResTranslation->x = value.worldPose[nBoneIndex]._14;
	pResTranslation->y = value.worldPose[nBoneIndex]._24;
	pResTranslation->z = value.worldPose[nBoneIndex]._34;
	return true;
}

bool CSkeletonAnimator::GetLocalBonePosition( const char *pszBoneName, SHMatrix *pLocalPos )
{
	CheckJustLoaded();
	const int nBoneIndex = GetBoneIndex( pszBoneName );
	if ( nBoneIndex < 0 )
		return false;

	RefreshWorldPose();
	const SBoneTransform *pBoneTransform = value.GetBone( nBoneIndex );
	if ( !pBoneTransform )
		return false;

	CVec3 vPos;
	CQuat qRot;
	memcpy( &vPos, &(pBoneTransform->Position), 3 * sizeof( float ) ); 
	memcpy( &qRot, &(pBoneTransform->Orientation), 4 * sizeof( float ) );

	pLocalPos->Set( vPos, qRot );

	return true;
}

bool CSkeletonAnimator::GetBonePosition( const char *pszBoneName, CVec3 *pResTranslation )
{
	CheckJustLoaded();
	int nBoneIndex = GetBoneIndex( pszBoneName );
	return GetBonePosition( nBoneIndex, pResTranslation );
}

bool CSkeletonAnimator::GetBonePosition( const char *pszBoneName, SHMatrix *pRes )
{
	CheckJustLoaded();
	int nBoneIndex = GetBoneIndex( pszBoneName );
	return GetBonePosition( nBoneIndex, pRes );
}

int CSkeletonAnimator::GetChannelCount()
{
	return scalarChannels.size();
}

int CSkeletonAnimator::GetChannelIndex( const std::string &szName )
{
	CheckJustLoaded();

	std::vector<SScalarChannel>::iterator channelIt = find_if( scalarChannels.begin(), scalarChannels.end(), SChannelByName(szName) );
	if ( channelIt != scalarChannels.end() )
	{
		const int nChannelIndex = distance( scalarChannels.begin(), channelIt );
		return nChannelIndex;
	}

	// Такого канала ещё нет.
	// Чтобы избавиться от повторных запросов и повторных поисков по символьному имени,
	// создаём неактивный scalarChannel "на вырост".
	int nChannelIndex = scalarChannels.size();
	SScalarChannel &channel = scalarChannels.emplace_back();
	channel.szName = szName;
	channel.bBinded = false;

	return nChannelIndex;
;
}

float CSkeletonAnimator::GetChannelValue( int nChannelIndex )
{
	CheckJustLoaded();
	if ( nChannelIndex >= 0 && nChannelIndex < scalarChannels.size() )
	{
		return scalarChannels[nChannelIndex].fValue;
	}

	return 0.f;
}

void CSkeletonAnimator::RecoverAnimHolder( SAnimID animID )
{
	SAnimationHolder &h = animHolders[ animID ];
	AddAnimationInternal( &h );
	if ( h.bFadeIn )
		FadeIn( h.tFadeDuration, animID );
	else if ( h.bFadeOut ) 
		FadeOut( h.tFadeDuration, animID );
}

CFuncBase<SFBTransform>* CSkeletonAnimator::CreateTransform( const std::string &szBoneName )
{
	CheckJustLoaded();
	int nBoneIndex = GetBoneIndex( szBoneName.c_str() );
	if ( nBoneIndex < 0 )
		return 0;
	return CreateTransform( nBoneIndex );
}

CFuncBase<SFBTransform>* CSkeletonAnimator::CreateTransform( int nBoneIndex )
{
	CheckJustLoaded();
	return new CAddBoneFilter( this, skeletonH, nBoneIndex );
}

void CSkeletonAnimator::ClearAllAnimations()
{
	Touch();
	ClearAnimVector();
	FreezeAllMutators();
}

int CSkeletonAnimator::operator&( CStructureSaver &f )
{
	if ( f.IsReading() )
		Clear();
	f.Add( 2, &pTime );
	f.Add( 3, &skeletonH );
	f.Add( 4, &nBones );
	f.Add( 5, &pSpecMutator );
	f.Add( 6, &boneMutators );
	f.Add( 7, &bBoneMutatorsEnabled );
	f.Add( 8, &animHolders );
	for ( int i = 0; i < 16; ++i )
		f.Add( 9, &value.poseGlobal[i], i + 1 );
	f.Add( 10, &bSmthChanged );
	f.Add( 11, &pGlobalTransform );
	f.Add( 12, &nAnimWithMovement );
	f.Add( 13, &fGlobalMovementSpeed );
	f.Add( 14, &fTransitHalfDuration );
	if ( f.IsReading() ) 
		bJustLoaded = true;
	return 0;
}

void CSkeletonAnimator::CheckJustLoaded()
{
	if ( bJustLoaded )
	{
		Create( skeletonH, pTime );
		for ( int i = 0; i < animHolders.size(); ++i )
			RecoverAnimHolder( i ); 
		bJustLoaded = false;
	}
}

void CSkeletonAnimator::FreezeAllMutators()
{
	if ( !bBoneMutatorsEnabled )
		return;
	ASSERT( IsValid( pTime ) );
	STime tStart = pTime->GetValue();
	for ( int i = 0; i < boneMutators.size(); ++i )
	{
		if ( boneMutators[i].IsEnabled() )
			SetBoneMutator( i, tStart, std::vector<SDesiredBoneMove>() );
	}
}

bool CSkeletonAnimator::DoesWantToUpdate( const STime &t ) const
{
	for ( int i = 0; i < animHolders.size(); ++i )
	{
		//if ( animHolders[i].tWhenEnables > t )
		//	continue;
		if ( animHolders[i].fWeight <= 0 )
			continue;
		if ( animHolders[i].nLoopCount == 0 )
			return true;
		if ( GrannyGetControlDurationLeft( animHolders[i].pControl ) <= 0 )
			continue;
		return true;
	}
	if ( IsValid( pSpecMutator ) && pSpecMutator->NeedUpdate() )
		return true;
	if ( !bBoneMutatorsEnabled )
		return false;
	for ( int i = 0; i < boneMutators.size(); ++i )
	{
		if ( boneMutators[i].IsEnabled() && boneMutators[i].GetEnd() > t )
			return true;
	}
	return false;
}


ISkeletonAnimator *CreateSkeletonAnimator(
	const SSkeletonHandle &modelH, CFuncBase<STime> *_pTime )
{
	if ( modelH.pSkeleton && !modelH.pSkeleton->szModelFileRef.empty() )
	{
		CGltfSkeletonAnimator *pAnimator = new CGltfSkeletonAnimator( modelH, _pTime );
		if ( !pAnimator->HasSkeleton() )
		{
			delete pAnimator;
			return 0;
		}
		return pAnimator;
	}
	CSkeletonAnimator *pAnimator = 0;

	// try & catch were removed
	pAnimator = new CSkeletonAnimator( modelH, _pTime );
	if ( !pAnimator->HasSkeleton() )
	{
		delete pAnimator;
		return 0;
	}
	return pAnimator;
//	return new CSkeletonAnimator( modelH, _pTime );
}

} // namespace
using namespace NAnimation;
BASIC_REGISTER_CLASS( _3DMOTOR, IAnimMutator )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x01321150, CSkeletonAnimator )
BASIC_REGISTER_CLASS( _3DMOTOR, ISkeletonAnimator )
