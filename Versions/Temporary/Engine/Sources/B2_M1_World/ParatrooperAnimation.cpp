#include "StdAfx.h"
#include "ParatrooperAnimation.h"
#include "MOUnitInfantry.h"
#include "../3Dmotor/GAnimation.hpp"
#include "../Stats_B2_M1/DBAnimB2.h"

bool CParatrooperAnimationProcess::Update( const NTimer::STime &time )
{
	if ( time >= timeToChange )
	{
		pInfantry->AIUpdateAnimationChanged( pUnitAnim, timeToChange );
		if ( NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator(nParachuteID) )
		{
			if ( !pParaAnim )
				return false;
			pAnimator->ClearAllAnimations();
			AddAnimation( pParaAnim, timeToChange, pAnimator, true );
		}
		return false;
	}
	return true;
}

bool CParachuteFinishProcess::Update( const NTimer::STime &time )
{
	if ( time >= timeToChange )
	{
		pInfantry->HideParachute();
		const NDb::SModel *pModel = pInfantry->GetModelDesc();
		Scene()->ChangeModel( pInfantry->GetID(), pModel );
		pInfantry->AIUpdateAnimationChanged( pIdleAnim, timeToChange );
		pInfantry->SetupWeapon( eSeason );
		Scene()->ShowObject( pInfantry->GetID(), pInfantry->IsVisible() );
		return false;
	}
	return true;
}

REGISTER_SAVELOAD_CLASS( 0x1015CD00, CParatrooperAnimationProcess );
REGISTER_SAVELOAD_CLASS( 0x1015E400, CParachuteFinishProcess );

