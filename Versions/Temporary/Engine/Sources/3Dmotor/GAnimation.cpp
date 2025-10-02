#include "StdAfx.h"
#include "../System/BasicShare.h"
#include "../System/BinaryResources.h"
#include "GAnimFormat.h"
#include "../vendor/Granny/include/granny.h"
#include "GAnimation.h"
#include "GAnimUtils.h"
#include "DBScene.h"


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

CSkeletonAnimator::CSkeletonAnimator() : bJustLoaded(false)
{
	// global movement
	nAnimWithMovement = -1;
	fGlobalMovementSpeed = 0.f;

	fTransitHalfDuration = 0.f;

	pSkeleton = 0;
	model.Name = 0;
	model.Skeleton = pSkeleton;
	GrannyMakeIdentity( &model.InitialPlacement );
	model.MeshBindingCount = 0;
	model.MeshBindings = 0;
	pModelInstance = 0;
	nBones = 0;

	pGlobalPose = 0;
	bGlobalPoseValid = false;
	bSmthChanged = true;

	bBoneMutatorsEnabled = false;

}

CSkeletonAnimator::CSkeletonAnimator( const SGrannySkeletonHandle &_skeletonH, CFuncBase<STime> *_pTime )
{
	// global movement
	nAnimWithMovement = -1;
	fGlobalMovementSpeed = 0.f;

	fTransitHalfDuration = 0.f;

	Create( _skeletonH, _pTime );

	// initializing mutators
	bBoneMutatorsEnabled = false;
	boneMutators.resize( nBones );
	for ( int i = 0; i < nBones; ++i )
		boneMutators[i].Enable( false );

	SHMatrix id;
	Identity( &id );
	SetGlobalPositionInternal( id );
}

void CSkeletonAnimator::Create( const SGrannySkeletonHandle &_skeletonH, CFuncBase<STime> *_pTime )
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
			NI_ASSERT( 0, StrFmt("Can't create skeleton \"%s\"", _skeletonH.pSkeleton->GetDBID().ToString().c_str() ) );
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
			NI_ASSERT( 0, StrFmt("Can't create skeleton \"%s\"", _skeletonH.pSkeleton->GetDBID().ToString().c_str() ) );
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
	pModelInstance = GrannyInstantiateModel( &model );
	nBones = pSkeleton->BoneCount;

	pGlobalPose = 0;
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
	CheckJustLoaded();
	ClearAnimVector();
	if ( pModelInstance )
		GrannyFreeModelInstance( pModelInstance );
	if ( value.pPose )
	{
		GrannyFreeLocalPose( value.pPose );
		value.pPose = 0;
	}
	if ( pGlobalPose )
		GrannyFreeWorldPose( pGlobalPose );
}

// internal method to set position (without marking internal state as changed)
void CSkeletonAnimator::SetGlobalPositionInternal( const SHMatrix &mGlobal ) 
{ 
	value.poseGlobal[0] = mGlobal._11; value.poseGlobal[4] = mGlobal._12; value.poseGlobal[8] = mGlobal._13; value.poseGlobal[12] = mGlobal._14;
	value.poseGlobal[1] = mGlobal._21; value.poseGlobal[5] = mGlobal._22; value.poseGlobal[9] = mGlobal._23; value.poseGlobal[13] = mGlobal._24;
	value.poseGlobal[2] = mGlobal._31; value.poseGlobal[6] = mGlobal._32; value.poseGlobal[10] = mGlobal._33; value.poseGlobal[14] = mGlobal._34;
	value.poseGlobal[3] = mGlobal._41; value.poseGlobal[7] = mGlobal._42; value.poseGlobal[11] = mGlobal._43; value.poseGlobal[15] = mGlobal._44;
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

				GrannyEvaluateCurveAtT( 1, false, &channelBinding.pTrack->ValueCurve, fLocalTime, &fValue );
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
	if ( !value.pPose )
		return true;
	if ( GrannyGetLocalPoseBoneCount( value.pPose ) != nBones )
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

	if ( !value.pPose )
		value.pPose = GrannyNewLocalPose( nBones );

	bGlobalPoseValid = false;

	GrannySetModelClock( pModelInstance, 0.001f * time );
	for ( int id = 0; id < animHolders.size(); ++id )
	{
		if ( GrannyControlIsComplete( animHolders[id].pControl ) )
			GrannySetControlActive( animHolders[id].pControl, false );
	}
	GrannySampleModelAnimations( pModelInstance, 0, nBones, value.pPose );

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
				granny_transform *pBoneTransform = GrannyGetLocalPoseTransform( value.pPose, i );
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

	if ( IsValid( pSpecMutator ) )
		pSpecMutator->MutateSkeletonPose( value.pPose );
	else
		pSpecMutator = 0;
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
		GrannySetTrackGroupTarget(pBuilder, nTrackGroupIndex, pModelInstance);
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
	NI_ASSERT( (animID >= 0 && animID < animHolders.size()), StrFmt("Invalid anim index \"%d\"", animID) );
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


unsigned int CSkeletonAnimator::GetMarkTimes( vector<float> *pResult, const SAnimID animID, const string &szMarkName )
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


unsigned int CSkeletonAnimator::EnumMarks( vector<string> *pResult, const SAnimID animID )
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
		vector<string> &marks = *pResult;

		const granny_text_track *pTrack = h.pAnnotationTrack;
		for ( int i = 0; i < pTrack->EntryCount; ++i )
		{
			const granny_text_track_entry &entry = pTrack->Entries[i];
			vector<string>::iterator markIt = find( marks.begin(), marks.end(), entry.Text );
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

void CSkeletonAnimator::GetBoneNames( vector<string> *pBoneNames )
{
	if ( pSkeleton )
	{
		pBoneNames->reserve( pSkeleton->BoneCount );
		for ( int i = 0; i < pSkeleton->BoneCount; ++i )
			pBoneNames->push_back( pSkeleton->Bones[i].Name );
	}
}

void CSkeletonAnimator::SetBoneMutator( const char *pszBoneName, const STime &tStart, 
	const vector<SDesiredBoneMove> &boneMutation )
{
	Touch();
	int nBoneIndex;
	if ( !GrannyFindBoneByName( pSkeleton, pszBoneName, &nBoneIndex ) )
		return;
	SetBoneMutator( nBoneIndex, tStart, boneMutation );
}

void CSkeletonAnimator::SetBoneMutator( const int nBoneIndex, const STime &tStart, 
																			 const vector<SDesiredBoneMove> &boneMutation )
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
		pGlobalPose = GrannyNewWorldPose( nBones );
	GrannyBuildWorldPose( pSkeleton, 0, nBones, value.pPose, value.poseGlobal, pGlobalPose );
}

bool CSkeletonAnimator::GetBonePosition( int nBoneIndex, SHMatrix *pRes )
{
	CheckJustLoaded();
	if ( nBoneIndex < 0 || nBoneIndex >= nBones )
		return false;
	RefreshWorldPose();
	granny_real32 *pMatrix = GrannyGetWorldPose4x4( pGlobalPose, nBoneIndex );
	if ( !pMatrix )
		return false;
	pRes->_11 = pMatrix[0]; pRes->_12 = pMatrix[4]; pRes->_13 = pMatrix[8]; pRes->_14 = pMatrix[12]; 
	pRes->_21 = pMatrix[1]; pRes->_22 = pMatrix[5]; pRes->_23 = pMatrix[9]; pRes->_24 = pMatrix[13]; 
	pRes->_31 = pMatrix[2]; pRes->_32 = pMatrix[6]; pRes->_33 = pMatrix[10]; pRes->_34 = pMatrix[14]; 
	pRes->_41 = pMatrix[3]; pRes->_42 = pMatrix[7]; pRes->_43 = pMatrix[11]; pRes->_44 = pMatrix[15]; 
	return true;
}

bool CSkeletonAnimator::GetBonePosition( int nBoneIndex, CVec3 *pResTranslation )
{
	CheckJustLoaded();
	if ( nBoneIndex < 0 || nBoneIndex >= nBones )
		return false;
	RefreshWorldPose();
	granny_real32 *pMatrix = GrannyGetWorldPose4x4( pGlobalPose, nBoneIndex );
	if ( !pMatrix )
		return false;
	pResTranslation->x = pMatrix[12];
	pResTranslation->y = pMatrix[13];
	pResTranslation->z = pMatrix[14]; 
	return true;
}

bool CSkeletonAnimator::GetLocalBonePosition( const char *pszBoneName, SHMatrix *pLocalPos )
{
	CheckJustLoaded();
	const int nBoneIndex = GetBoneIndex( pszBoneName );
	if ( nBoneIndex < 0 )
		return false;

	RefreshWorldPose();
	granny_transform *pBoneTransform = GrannyGetLocalPoseTransform( value.pPose, nBoneIndex );
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

int CSkeletonAnimator::GetChannelIndex( const string &szName )
{
	CheckJustLoaded();

	vector<SScalarChannel>::iterator channelIt = find_if( scalarChannels.begin(), scalarChannels.end(), SChannelByName(szName) );
	if ( channelIt != scalarChannels.end() )
	{
		const int nChannelIndex = distance( scalarChannels.begin(), channelIt );
		return nChannelIndex;
	}

	// Такого канала ещё нет.
	// Чтобы избавиться от повторных запросов и повторных поисков по символьному имени,
	// создаём неактивный scalarChannel "на вырост".
	int nChannelIndex = scalarChannels.size();
	SScalarChannel &channel = scalarChannels.push_back();
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

CFuncBase<SFBTransform>* CSkeletonAnimator::CreateTransform( const string &szBoneName )
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
			SetBoneMutator( i, tStart, vector<SDesiredBoneMove>() );
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
	const SGrannySkeletonHandle &modelH, CFuncBase<STime> *_pTime )
{
	CSkeletonAnimator *pAnimator = 0;

	// try & catch were removed
	pAnimator = new CSkeletonAnimator( modelH, _pTime );
	if ( !pAnimator->HasSkeleton() )
		return 0;
	return pAnimator;
//	return new CSkeletonAnimator( modelH, _pTime );
}

} // namespace
using namespace NAnimation;
BASIC_REGISTER_CLASS( IAnimMutator )
REGISTER_SAVELOAD_CLASS( 0x01321150, CSkeletonAnimator )
BASIC_REGISTER_CLASS( ISkeletonAnimator )