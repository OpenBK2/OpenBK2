#include "StdAfx.h"
#include "GAnimLight.h"
#include "DBScene.h"
#include "GScene.h"

namespace NGScene
{

// CLightLoader

CFileRequest* CLightLoader::CreateRequest()
{
	return CreateFileRequiest( "Lights", SIntResKey( GetKey().tKey->uid, -1 ) );
}

void CLightLoader::RecalcValue( CFileRequest *p )
{
	pValue = new CAnimLightInfo;
	CObj<IBinSaver> pSaver = CreateBinSaver( p->GetStream(), SAVER_MODE_READ );
	pSaver->Add( 1, &pValue->fFrameRate );
	pSaver->Add( 2, &pValue->fTStart );
	pSaver->Add( 3, &pValue->fTEnd );
	pSaver->Add( 4, &pValue->pos );
	pSaver->Add( 5, &pValue->color );
	pSaver->Add( 6, &pValue->radius );
}

// CLightAnimator

void CLightAnimator::Recalc()
{
	if ( !IsValid( pValue ) )
		pValue = new CAnimLight;
	CAnimLight &value = *pValue;

	value.bEnd = true;
	value.bActive = false;

	STime time = pTime->GetValue();
	if ( time < stBeginTime )
	{
		value.bEnd = false;
		return;
	}
	float fTObject = (time - stBeginTime) * pInstance->fSpeed / 1000.f - pInstance->fOffset * pInstance->fSpeed;
	if ( fTObject < 0 )
	{
		value.bEnd = false;
		return;
	}
	CAnimLightInfo *pLight = pInfo->GetValue();
	if ( !pLight )
	{
		value.bEnd = false;
		return;
	}
	float fTAnim;
	if ( pInstance->fEndCycle == 0 )
	{
		fTAnim = fTObject;
		if ( fTAnim < pLight->fTEnd )
			value.bEnd = false;
	}
	else
	{
		int nCurCycle = int( fTObject / pInstance->fEndCycle ) + 1;
		if ( pInstance->nCycleCount && nCurCycle > pInstance->nCycleCount )
			nCurCycle = pInstance->nCycleCount;
		fTAnim = fTObject - (nCurCycle - 1) * pInstance->fEndCycle;
		if ( pInstance->nCycleCount == 0
			|| fTObject < pLight->fTEnd + (pInstance->nCycleCount - 1) * pInstance->fEndCycle )
			value.bEnd = false;
	}
	if ( fTAnim >= pLight->fTStart && fTAnim < pLight->fTEnd )
	{
		value.bActive = true;
		fTAnim *= pLight->fFrameRate;

		const SFBTransform &trans = pPlacement->GetValue();
		CVec3 local;
		pLight->pos.GetValue( fTAnim, &local );
		trans.forward.RotateHVector( &value.position, local );

		float fRadius;
		pLight->radius.GetValue( fTAnim, &fRadius );
		value.fRadius = fScale * fRadius;

		pLight->color.GetValue( fTAnim, &value.color );
	}
}

}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x11062160, CLightAnimator );
REGISTER_SAVELOAD_CLASS( 0x11062161, CLightLoader );
