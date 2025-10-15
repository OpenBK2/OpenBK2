#include "stdafx.h"

#include "dbsceneconsts.h"
#include "AttachedObj.hpp"
#include "SceneHoldQueue.h"
#include "3Dmotor/AIVisitor.h"
#include "3Dmotor/GScene.h"

#include "Sound/SoundScene.h"
#include "Sound/DBSound.h"


BASIC_REGISTER_CLASS( SCENEB2, IAttachedObject );

//*******************************************************************
//*										 IAttachedObject														 *
//*******************************************************************

IAttachedObject* IAttachedObject::CreateStaticObj( const NDb::SModel *pModel, CFuncBase<SFBTransform> *pTransform, const int nBoneIndex, CFuncBase<SBound> *pBound, bool bUseLOD )
{
	return new CStaticAttachedObj( pModel, pTransform, nBoneIndex, pBound, bUseLOD );
}

IAttachedObject* IAttachedObject::CreateAnimatedObj( const NDb::SModel *pModel, CFuncBase<SFBTransform> *pTransform,
																										 CFuncBase<STime> *pTime, const int nBoneIndex, CFuncBase<SBound> *pBound )
{
	if ( pModel->pSkeleton )
	{
		NAnimation::SGrannySkeletonHandle handle;
		handle.pSkeleton = pModel->pSkeleton;
		handle.nModelInFile = 0;
		CPtr<NAnimation::ISkeletonAnimator> pAnimator = NAnimation::CreateSkeletonAnimator( handle, pTime );

		if ( pAnimator )
			return new CAnimatedAttachedObj( pModel, pTransform, pAnimator, nBoneIndex, pBound );
	}
	
	return 0;
}

IAttachedObject* IAttachedObject::CreateEffect( const NDb::SEffect *pEffect, CFuncBase<SFBTransform> *pTransform,
																							  const NTimer::STime nStartTime, const int nBoneIndex, bool bVertical )
{
	if ( bVertical )
		return new CAttachedEffect( pEffect, new CAlwaysVerticalTransform( pTransform, bVertical ), nStartTime, nBoneIndex );
	else
		return new CAttachedEffect( pEffect, pTransform, nStartTime, nBoneIndex );
}

IAttachedObject* IAttachedObject::CreateAnimatedLightEffect( const NDb::SAttachedLightEffect *pEffect,
																														 CFuncBase<SFBTransform> *pTransform, const NTimer::STime nStartTime,
																														 const int nBoneIndex, const bool bInEditor )
{
	return new CAttachedLightEffect( pEffect, pTransform, nStartTime, nBoneIndex, bInEditor );
}

IAttachedObject* IAttachedObject::CreateStaticLightEffect( const NDb::SAttachedLightEffect *pEffect, const int nObjectID,
																													 const NTimer::STime nStartTime, const bool bInEditor )
{
	return new CAttachedStaticLightEffect( pEffect, nObjectID, nStartTime, bInEditor );
}

void CAttachedObject::Destroy( const NTimer::STime time )
{
	pHolder = 0;
}
void CAttachedObject::Clear( const NTimer::STime time )
{
	pHolder = 0;
}

//*******************************************************************
//*								 Attached objects implementation								 *
//*******************************************************************

// CStaticAttachedObj

void CStaticAttachedObj::Visit( IAIVisitor *pAIVisitor )
{
	if ( pModel && pModel->pGeometry->pAIGeometry ) 
	{
		RefreshTransform();		
		pAIVisitor->AddHull( pModel->pGeometry->pAIGeometry, GetTransform()->GetValue().forward, 0, 0, 1 );
	}
}

const NDb::SModel* CStaticAttachedObj::GetModel() const
{ 
	return pModel; 
}

void CStaticAttachedObj::ReCreate( NGScene::IGameView *pGScene, CCSTime *pTimer )
{
	if ( pModel != 0 )
	{
		NGScene::IGameView::SMeshInfo meshInfo;
		pGScene->CreateMeshInfo( pModel, &meshInfo, false );
		SetHolder( pGScene->CreateMesh( meshInfo, GetTransform(), GetBounder(), 0, bUseLOD ? NGScene::SFullRoomInfo(0, NGScene::N_MASK_LOD_HIGH) : 0 ) );

		if ( pModel->pGeometry->pAIGeometry )
		{
			GetSrcBind().Link( Scene()->GetSyncSrc(), this );
			IScene *pScene = Scene();
			CSyncSrc<IVisObj> *pSyncSrc = pScene->GetSyncSrc();
			GetSrcBind().Update();
		}
	}
}

// CAnimatedAttachedObj

void CAnimatedAttachedObj::Visit( IAIVisitor *pAIVisitor )
{
	if ( pModel && pModel->pGeometry->pAIGeometry ) 
	{
		RefreshTransform();
		pAIVisitor->AddHull( pModel->pGeometry->pAIGeometry, GetTransform()->GetValue().forward, 0, 0, 1 );
	}
}

const NDb::SModel* CAnimatedAttachedObj::GetModel() const
{ 
	return pModel; 
}

void CAnimatedAttachedObj::ReCreate( NGScene::IGameView *pGScene, CCSTime *pTimer )
{
	if ( pModel != 0 && pAnimator != 0 )
	{
		pAnimator->SetGlobalTransform( GetTransform() );
		NGScene::IGameView::SMeshInfo meshInfo;
		pGScene->CreateMeshInfo( pModel, &meshInfo, true );
		SetHolder( pGScene->CreateMesh( meshInfo, GetTransform(), GetBounder(), NGScene::CMeshAnimStuff( pModel, pAnimator ), NGScene::SFullRoomInfo(0, NGScene::N_MASK_LOD_HIGH) ) );

		if ( pModel->pGeometry->pAIGeometry )
		{
			IScene *pScene = Scene();
			CSyncSrc<IVisObj> *pSyncSrc = pScene->GetSyncSrc();
			GetSrcBind().Link( pSyncSrc, this );
			GetSrcBind().Update();
		}
	}
}

// CAttachedEffect

void CAttachedEffect::ReCreate( NGScene::IGameView *pGScene, CCSTime *pTimer )
{
	if ( pEffect != 0 )
		SetHolder( pGScene->CreateParticles( pEffect, nStartTime, pTimer, GetTransform() ) );
}

void CAttachedEffect::Destroy( const NTimer::STime time )
{
	Clear( time );
	pEffect = 0;

	// CAttachedObject::Destroy( time );	// Do not destroy holder
}

void CAttachedEffect::Clear( const NTimer::STime time )
{
	if ( GetHolder() != 0 )
	{
		NGScene::StopParticlesGeneration( GetHolder(), time );
		NGScene::StopDynamicLighting( GetHolder() );
		SetToSceneHoldQueue( this, true );
	}
}

CAttachedEffect::~CAttachedEffect()
{
//	if ( pEffect != 0 && IsValid( GetHolder() ) )
//		Destroy( Singleton<IGameTimer>()->GetGameTime() );
}

// CAlwaysVerticalTransform

CAlwaysVerticalTransform::CAlwaysVerticalTransform( CFuncBase<SFBTransform> *_pBaseTransform, const bool _bKeepVertical ) 
: pBaseTransform(_pBaseTransform), bKeepVertical(_bKeepVertical)
{ 
}

void CAlwaysVerticalTransform::Recalc()
{
	pBaseTransform.Refresh();
	if ( bKeepVertical )
	{
		const SHMatrix &mf = pBaseTransform->GetValue().forward;
		value.forward.Set(	1, 0, 0, mf.xw,
			0, 1, 0, mf.yw,
			0, 0, 1, mf.zw,
			0, 0, 0, 1 );
		const SHMatrix &mb = pBaseTransform->GetValue().backward;
		value.backward.Set(	1, 0, 0, mb.xw,
			0, 1, 0, mb.yw,
			0, 0, 1, mb.zw,
			0, 0, 0, 1 );
	}
	else
	{
		value.forward = pBaseTransform->GetValue().forward;
		value.backward = pBaseTransform->GetValue().backward;
	}
}

// CAttachedLightEffectTransform

CAttachedLightEffectTransform::CAttachedLightEffectTransform( CFuncBase<SFBTransform> *_pBaseTransform, const SHMatrix &_mMultiplier ) 
: pBaseTransform(_pBaseTransform), mMultiplier(_mMultiplier)
{ 
	pvTranslatePart = new CCCVec3();
}

void CAttachedLightEffectTransform::Recalc()
{
	pBaseTransform.Refresh();
	value.forward = pBaseTransform->GetValue().forward;
	value.forward = value.forward * mMultiplier;
	value.backward.HomogeneousInverse( value.forward );
	////
	pvTranslatePart->Set( value.forward.GetTrans3() );
}

// CConstantOffsetTransform

CConstantOffsetTransform::CConstantOffsetTransform( const int _nTargetID, const std::string &_szBoneName, CFuncBase<SFBTransform> *_pBaseTransform )
	: pBaseTransform( _pBaseTransform ),
	nTargetID( _nTargetID ),
	szBoneName( _szBoneName ),
	bNeedCalcMatrix( true ),
	pParentTransform( 0 )
{
	Identity( &mMultiplier );
}

CConstantOffsetTransform::CConstantOffsetTransform( const int _nTargetID, const std::string &_szBoneName,
																									  CFuncBase<SFBTransform> *_pBaseTransform, CFuncBase<SFBTransform> *_pParentTransform )
	: pBaseTransform( _pBaseTransform ),
	nTargetID( _nTargetID ),
	szBoneName( _szBoneName ),
	bNeedCalcMatrix( true )
{
	pParentTransform = dynamic_cast<CConstantOffsetTransform*>( _pParentTransform );
	Identity( &mMultiplier );
}

void CConstantOffsetTransform::Recalc()
{
	pBaseTransform.Refresh();

	if ( bNeedCalcMatrix )
	{
		NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( nTargetID, szBoneName );
		if ( pAnimator && pAnimator->GetLocalBonePosition( szBoneName.c_str(), &mMultiplier ) )
			bNeedCalcMatrix = false;
		if ( pParentTransform )
		{
			std::string szParentBone = pParentTransform->szBoneName;
			NAnimation::ISkeletonAnimator *pParentAnimator = Scene()->GetAnimator( nTargetID, szParentBone );
			SHMatrix mParent;

			if ( pParentAnimator && pParentAnimator->GetLocalBonePosition( szParentBone.c_str(), &mParent ) )
				mMultiplier = mMultiplier * mParent;

			pParentTransform = 0;
		}
	}

	value.backward = pBaseTransform->GetValue().backward;
	value.backward = mMultiplier * value.backward;
	value.forward.HomogeneousInverse( value.backward );
}

// CCenterOffsetTransform

CCenterOffsetTransform::CCenterOffsetTransform( const int _nTargetID, CFuncBase<SFBTransform> *_pBaseTransform,
	const SHMatrix &_mMultiplier ) :
	pBaseTransform( _pBaseTransform ),
	nTargetID( _nTargetID ),
	mMultiplier( _mMultiplier )
{
	mMultiplierInv.HomogeneousInverse( mMultiplier );
}

void CCenterOffsetTransform::Recalc()
{
	pBaseTransform.Refresh();

	value.forward = pBaseTransform->GetValue().forward;
	value.forward = value.forward * mMultiplier;
	value.forward.Set( value.forward.GetTrans3(), QNULL );
	value.backward = pBaseTransform->GetValue().backward;
	value.backward = mMultiplierInv * value.backward;
	value.backward.Set( value.backward.GetTrans3(), QNULL );
}

// CAttachedLightEffectPos

void CAttachedLightEffectPos::Recalc()
{
	pTransform.Refresh();
	const SHMatrix &forward = pTransform->GetValue().forward * mAddTransform;
	value = forward.GetTrans3(); // + CVec3( forward._13, forward._23, forward._33 ) * 2;
}

// CAttachedLightEffectPos

void CAttachedLightEffectDir::Recalc()
{
	pTransform.Refresh();
	const SHMatrix &forward = pTransform->GetValue().forward;
	value = CVec3( forward._13, forward._23, forward._33 );
}

// CAttachedLightEffect

CAttachedLightEffect::CAttachedLightEffect( const NDb::SAttachedLightEffect *_pEffect, CFuncBase<SFBTransform> *pTransform,
																					  const NTimer::STime _nStartTime, const int nBoneIndex, const bool _bInEditor )
	: CAttachedObject( pTransform, nBoneIndex, 0 ),
	nStartTime( _nStartTime ),
	nPointLightID( OBJECT_ID_GENERATE ),
	pFlareHolder( 0 ),
	nSoundID(-1),
	bInEditor(_bInEditor)
{
	lightEffect = *_pEffect;
	////
	RegeneratePositions();
	SetHolder( 0 );
}

void CAttachedLightEffect::OnSerialize( IBinSaver &saver )
{
	if ( saver.IsReading() ) 
	{
		bNeedRecalc = true;
	}
}

void CAttachedLightEffect::RegeneratePositions()
{
	const float fDummyConeSize = lightEffect.fConeSize ? lightEffect.fConeSize : 1.0f;
	const float fDummyConeLength = lightEffect.fConeLength ? lightEffect.fConeLength : 1.0f;
	SHMatrix mScale( fDummyConeSize, 0, 0, 0,
									 0, fDummyConeSize, 0, 0, 
									 0, 0, fDummyConeLength, 0,
									 0, 0, 0, 1 );
	pConeTransform = new CAttachedLightEffectTransform( GetTransform(), mScale );

	SHMatrix mIdentity;
	Identity( &mIdentity );

	pParticleTransform = new CAttachedLightEffectTransform( GetTransform(), mIdentity );	

	const SHMatrix mFlareOffset( lightEffect.vFlarePos, QNULL );
	pFlarePos = new CAttachedLightEffectPos( GetTransform(), mFlareOffset );
	pSoundPos = new CAttachedSoundPos( GetTransform() );

	bNeedRecalc = false;
}

void CAttachedLightEffect::ReCreate( NGScene::IGameView *pGScene, CCSTime *pTimer )
{
	const NDb::SSceneConsts *pSceneConsts = Scene()->GetSceneConsts();
	SHMatrix mTemp;
	if ( !pSceneConsts )
	{
		NI_ASSERT( false, "Scene Consts not loaded" );
		return;
	}

	if ( bNeedRecalc )
		RegeneratePositions();

	//																				Flare
	if ( !bInEditor && lightEffect.fFlareSize > 0.0f /*&& !pFlareHolder*/ )
	{
		const NDb::STexture *pTexture = ( lightEffect.pFlareTexture ) ? lightEffect.pFlareTexture 
			: pSceneConsts->lightFX.pFlareTexture;

		pFlareHolder = pGScene->AddFlare( pFlarePos, Scene()->GetAbsTimer(), 0,
				lightEffect.fFlareSize, pTexture, pSceneConsts->lightFX.fFlareAppearTime, 
				pSceneConsts->lightFX.fFlareAppearTime );
	}
	//																				Point light
	if ( !bInEditor && lightEffect.fPointLightSize > 0.0f )
	{
		// No point lights for animated objects?
	}
	//																				Light cone
	if ( !bInEditor && lightEffect.fConeSize > 0.0f )
	{
		const NDb::STexture *pTexture = lightEffect.pConeTexture;
		SHMatrix mDummy;
		Identity( &mDummy );
		if ( !pTexture )
			pTexture = pSceneConsts->lightFX.pLightConeTexture;
		////
		pDirFlareHolder = pGScene->AddDirFlare( pFlarePos/*new CAttachedLightEffectPos( GetTransform(), mDummy )*/, 
			new CAttachedLightEffectDir( GetTransform() ), CVec2( lightEffect.fConeLength, lightEffect.fConeSize ), pTexture, 0 );
	}
	//																				Additional (particle) effect
	if ( !bInEditor && lightEffect.pAdditionalEffect )
	{
		const NDb::SEffect *pEffect =  lightEffect.pAdditionalEffect->GetSceneEffect();
		if ( pEffect )
			SetHolder( pGScene->CreateParticles( pEffect, nStartTime, pTimer, pParticleTransform ) );
	}

	// Additional sound effect
	if ( !bInEditor && lightEffect.pAdditionalEffect && lightEffect.pAdditionalEffect->pSoundEffect )
	{
		AddSound( lightEffect.pAdditionalEffect->pSoundEffect );
	}
	
}

void CAttachedLightEffect::AddSound( const NDb::SComplexSoundDesc *pSound )
{
	if ( nSoundID != -1 )
		RemoveSound();

	nSoundID = SoundScene()->AddSound( pSound, pSoundPos, SFX_MIX_SUBSTITUTE, pSound->bLooped ? SAM_LOOPED_NEED_ID : SAM_NEED_ID, 0, 2 );
}

void CAttachedLightEffect::RemoveSound()
{
	if ( nSoundID == -1 )
		return;

	SoundScene()->RemoveSound( nSoundID );
	nSoundID = -1;
}

void CAttachedLightEffect::Destroy( const NTimer::STime time )
{
	// Destroy point light
	if ( !bInEditor && lightEffect.fPointLightSize > 0.0f )
		Scene()->RemoveObject( nPointLightID );

	// Destroy flare
	if ( !bInEditor && lightEffect.fFlareSize > 0.0f )
		pFlareHolder = 0;

	// Destroy cone
	if ( !bInEditor && lightEffect.fConeSize > 0.0f )
		pDirFlareHolder = 0;

	// Destroy additional effect
	if ( !bInEditor && lightEffect.pAdditionalEffect && GetHolder() ) 
	{
		NGScene::StopParticlesGeneration( GetHolder(), time );
		NGScene::StopDynamicLighting( GetHolder() );
		SetToSceneHoldQueue( this, true );
	}

	// Destroy sound
	if ( !bInEditor && lightEffect.pAdditionalEffect && lightEffect.pAdditionalEffect->pSoundEffect ) 
	{
		RemoveSound();
	}

	//CAttachedObject::Destroy( time );				// Destroys light cone, if any
}

// CAttachedStaticLightEffect

CAttachedStaticLightEffect::CAttachedStaticLightEffect( const NDb::SAttachedLightEffect *_pEffect, const int _nObjectID,
																											  const NTimer::STime _nStartTime, const bool _bInEditor )
	: nStartTime( _nStartTime ),
	nPointLightID( OBJECT_ID_GENERATE ),
	nObjectUniqueID( _nObjectID ),
	nConeID( OBJECT_ID_GENERATE ),
	bInEditor( _bInEditor )
{
	lightEffect	= *_pEffect;

	if ( bInEditor && lightEffect.fConeSize == 0.0f )
	{		// Since only the cone is shown in editor, show a dummy cone
		lightEffect.fConeLength = 1.0f;
		lightEffect.fConeSize = 1.0f;
	}
	// Calculate matrices
	RegeneratePositions();
}

void CAttachedStaticLightEffect::OnSerialize( IBinSaver &saver )
{
	if ( saver.IsReading() ) 
	{
		bNeedRecalc = true;
	}
}

void CAttachedStaticLightEffect::RegeneratePositions()
{
	SHMatrix mLocal( lightEffect.vPos, lightEffect.qRot );
	const float fDummyConeSize = lightEffect.fConeSize ? lightEffect.fConeSize : 1.0f;
	const float fDummyConeLength = lightEffect.fConeLength ? lightEffect.fConeLength : 1.0f;
	SHMatrix mScale(	fDummyConeSize, 0, 0, 0,
		0, fDummyConeSize, 0, 0, 
		0, 0, fDummyConeLength, 0,
		0, 0, 0, 1 );

	SFBTransform globalTransform;
	Scene()->GetVisObjPlacement( nObjectUniqueID, &globalTransform );
	mConePlace = globalTransform.forward * mLocal;

	mLocal = mConePlace * SHMatrix( lightEffect.vPointLightPos, QNULL );
	vPointLightPos = mLocal.GetTrans3();

	mLocal = mConePlace * SHMatrix( lightEffect.vFlarePos, QNULL );
	pvFlarePlace = new CCCVec3;
	pvFlarePlace->Set( mLocal.GetTrans3() );

	mConePlace = mConePlace * mScale;

	globalTransform.forward = mConePlace;
	globalTransform.backward.HomogeneousInverse( globalTransform.forward );

	pConeTransform = new CCSFBTransform;
	pConeTransform->Set( globalTransform );

	bNeedRecalc = false;
}

void CAttachedStaticLightEffect::ReCreate( NGScene::IGameView *pGScene, CCSTime *pTimer )
{
	const NDb::SSceneConsts *pSceneConsts = Scene()->GetSceneConsts();

	if ( !pSceneConsts )
	{
		NI_ASSERT( false, "Scene Consts not loaded" );
		return;
	}

	if ( bNeedRecalc )
		RegeneratePositions();

	//																				Flare
	if ( !bInEditor && lightEffect.fFlareSize > 0.0f )
	{
		const NDb::STexture *pTexture = ( lightEffect.pFlareTexture ) ? lightEffect.pFlareTexture 
			: pSceneConsts->lightFX.pFlareTexture;

		pFlareHolder = pGScene->AddFlare( pvFlarePlace, Scene()->GetAbsTimer(), 0,
			lightEffect.fFlareSize, pTexture, 
			pSceneConsts->lightFX.fFlareAppearTime, pSceneConsts->lightFX.fFlareAppearTime );
	}
	//																				Point light
	if ( !bInEditor && lightEffect.fPointLightSize > 0.0f )
	{
		if ( nPointLightID != OBJECT_ID_GENERATE )
			Scene()->RemoveObject( nPointLightID );
		nPointLightID = Scene()->AddPointLight( nPointLightID, lightEffect.vColour, vPointLightPos, lightEffect.fPointLightSize );
	}
	//																				Light cone
	if ( !bInEditor && lightEffect.fConeSize > 0.0f )
	{
		SHMatrix mDummy;
		Identity( &mDummy );
		const NDb::STexture *pTexture = lightEffect.pConeTexture;
		if ( !pTexture )
			pTexture = pSceneConsts->lightFX.pLightConeTexture;
		////
		pDirFlareHolder = pGScene->AddDirFlare( new CAttachedLightEffectPos( pConeTransform, mDummy ), 
			new CAttachedLightEffectDir( pConeTransform ), CVec2( lightEffect.fConeLength, lightEffect.fConeSize ), pTexture, 0 );
	}
	//																				Particle effect
	if ( !bInEditor && lightEffect.pAdditionalEffect && lightEffect.pAdditionalEffect->pSceneEffect ) 
	{
		const NDb::SEffect *pEffect =  lightEffect.pAdditionalEffect->GetSceneEffect();
		if ( pEffect )
			SetHolder( pGScene->CreateParticles( pEffect, nStartTime, pTimer, pConeTransform ) );
	}

}

void CAttachedStaticLightEffect::Destroy( const NTimer::STime time )
{
	// Destroy point light
	if ( !bInEditor && lightEffect.fPointLightSize > 0.0f )
	{
		Scene()->RemoveObject( nPointLightID );
		nPointLightID = OBJECT_ID_GENERATE;
	}

	// Destroy flare
	if ( !bInEditor && lightEffect.fFlareSize > 0.0f )
		pFlareHolder = 0;

	// Destroy light cone
	if ( !bInEditor && lightEffect.fConeSize > 0.0f )
		pDirFlareHolder = 0;

	if ( !bInEditor && lightEffect.pAdditionalEffect ) 
	{
		NGScene::StopParticlesGeneration( GetHolder(), time );
		NGScene::StopDynamicLighting( GetHolder() );
		SetToSceneHoldQueue( this, true );
	}

	//CAttachedObject::Destroy( time );			
}

REGISTER_SAVELOAD_CLASS( SCENEB2, 0x3013D340, CStaticAttachedObj );
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x3013D341, CAnimatedAttachedObj );
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x3013D342, CAttachedEffect );
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x191462C0, CAttachedLightEffectTransform );
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x1914BC80, CAlwaysVerticalTransform );
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x19144300, CAttachedLightEffect );
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x19144C40, CAttachedStaticLightEffect );
REGISTER_SAVELOAD_CLASS( SCENEB2, 0xB4416170, CAttachedLightEffectPos );
REGISTER_SAVELOAD_CLASS( SCENEB2, 0xB4416171, CAttachedLightEffectDir );
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x3116CD01, CConstantOffsetTransform );
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x17243B80, CCenterOffsetTransform );


