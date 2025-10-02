#include "StdAfx.h"
#include "..\ui\commandparam.h"
#include "..\ui\dbuserinterface.h"
#include "..\ui\ui.h"
#include "..\input\gamemessage.h"
#include "..\ui\uifactory.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "EffectorB2Move.h"
#include "DBUISpecificB2.h"

void CEffectorB2Move::Configure( const NDb::SUIStateBase *_pCmd, interface IScreen *pScreen, SWindowContext *_pContext, const string &szAnimatedWindow ) 
{ 
	const NDb::SUISB2Move *pCmd( checked_cast<const NDb::SUISB2Move*>( _pCmd ) );
	NI_VERIFY( pCmd, "no command", return );
	SWindowContextB2Move *pContext = dynamic_cast<SWindowContextB2Move*>( _pContext );

	pElement = dynamic_cast<IWindow*>( pScreen->GetElement( pCmd->szElementToMove, true ) );
	NI_ASSERT( pElement != 0, StrFmt( "no element \"%s\"", pCmd->szElementToMove.c_str() ) );
	if ( !pElement )
	{
		bFinished = true;
		return;
	}

	if ( pCmd->bBorder )
		eState = ES_BORDER_IN;
	else
		eState = (pContext && pContext->bGoOut) ? ES_GO_OUT : ES_GO_IN;
	bGoOut = (eState == ES_GO_OUT);

	bForward = true;

	vMoveOffset = pCmd->vOffset;
	CVec2 vAccelCoeff = pCmd->vAccelCoeff;
	if ( eState == ES_GO_IN )
	{
		vAccelCoeff.x = (vAccelCoeff.x <= FP_EPSILON) ? 0.0f : 1.0f / vAccelCoeff.x;
		vAccelCoeff.y = (vAccelCoeff.y <= FP_EPSILON) ? 0.0f : 1.0f / vAccelCoeff.y;
	}
	fMoveTime = pCmd->fMoveTime;

	vMoveOffset2 = pCmd->vOffsetBounce;
	CVec2 vAccelCoeffBounceIn = pCmd->vAccelCoeffBounce;
	CVec2 vAccelCoeffBounceOut(
		(vAccelCoeffBounceIn.x <= FP_EPSILON) ? 0.0f : 1.0f / vAccelCoeffBounceIn.x,
		(vAccelCoeffBounceIn.y <= FP_EPSILON) ? 0.0f : 1.0f / vAccelCoeffBounceIn.y );
	fMoveTime2 = pCmd->fMoveTimeBounce;
	
	fWaitTime = max( 0.0f, max( pCmd->fMaxMoveTime, (pContext ? pContext->fMaxMoveTime : 0.0f) ) - fMoveTime );

	int x, y, w, h;
	pElement->GetPlacement( &x, &y, &w, &h );

	CTRect<float> rcElement = pElement->GetWindowRect();

	if ( IWindow *pParent = pElement->GetParentWindow() )
	{
		CTRect<float> rcParent = pParent->GetWindowRect();

		if ( vMoveOffset.x > 0 )
			vMoveOffset.x = rcParent.Width() - rcElement.x1;
		else if ( vMoveOffset.x < 0 )
			vMoveOffset.x = -rcElement.x2;

		if ( vMoveOffset.y > 0 )
			vMoveOffset.y = rcParent.Height() - rcElement.y1;
		else if ( vMoveOffset.y < 0 )
			vMoveOffset.y = -rcElement.y2;
	}

	vInitialPos = CVec2( x, y );
	vInitialSize = CVec2( w, h );

	CalcSpeedAccel( &vSpeed, &vAccel, vMoveOffset, vAccelCoeff, fMoveTime );
	CalcSpeedAccel( &vSpeedBounceOut, &vAccelBounceOut, vMoveOffset2, vAccelCoeffBounceOut, fMoveTime2 );
	CalcSpeedAccel( &vSpeedBounceIn, &vAccelBounceIn, vMoveOffset2, vAccelCoeffBounceIn, fMoveTime2 );

	vOuterPos = vInitialPos + GetDelta( vSpeed, vAccel, fMoveTime );
	vBouncePos = vInitialPos + GetDelta( vSpeedBounceIn, vAccelBounceIn, fMoveTime2 );
	
	vBounceSize = vInitialSize - 2.0f * GetDelta( vSpeedBounceIn, vAccelBounceIn, fMoveTime2 );

	bFinished = false;
	fElapsedTime = 0;
}

void CEffectorB2Move::CalcSpeedAccel( CVec2 *pSpeed, CVec2 *pAccel, const CVec2 &vMoveOffset, const CVec2 &vAccelCoeff, float fTime ) const
{
	if ( fTime < FP_EPSILON )
	{
		pSpeed->x = 0.0f;
		pSpeed->y = 0.0f;

		pAccel->x = 0.0f;
		pAccel->y = 0.0f;
	}
	else
	{
		pSpeed->x = 2.0f * vMoveOffset.x / ((vAccelCoeff.x + 1.0f) * fTime);
		pSpeed->y = 2.0f * vMoveOffset.y / ((vAccelCoeff.y + 1.0f) * fTime);

		pAccel->x = pSpeed->x * (vAccelCoeff.x - 1.0f) / fTime;
		pAccel->y = pSpeed->y * (vAccelCoeff.y - 1.0f) / fTime;
	}
}

void CEffectorB2Move::Reverse()
{
	NI_ASSERT( 0, "Can't reverse EffectorB2Move" );
}

CVec2 CEffectorB2Move::GetDelta( const CVec2 &vSpeed, const CVec2 &vAccel, float fTime ) const
{
	return vSpeed * fTime + vAccel * (fTime * fTime * 0.5f);
}

const int CEffectorB2Move::Segment( const int timeDiff, interface IScreen *pScreen, const bool bFastForward )
{
	if ( bFinished )
		return 0;
	if ( eState == ES_NONE )
	{
		bFinished = true;
		return 0;
	}
		
	const float fFormerElapsedTime = fElapsedTime;
	fElapsedTime += timeDiff;

	CVec2 vCurrPos = vInitialPos;
	float fEffectTime = 0.0f;
	CVec2 vCurrSize = vInitialSize;

	switch ( eState )
	{
		case ES_GO_OUT:
		{
			if ( fElapsedTime >= fMoveTime + fWaitTime )
			{
				vCurrPos = vOuterPos;
				fEffectTime = fMoveTime + fWaitTime;
				eState = ES_NONE;
			}
			else
			{
				vCurrPos = vInitialPos + GetDelta( vSpeed, vAccel, max( 0.0f, fElapsedTime - fWaitTime ) );
				fEffectTime = fElapsedTime;
			}
			break;
		}

		case ES_GO_IN:
		{
			if ( fElapsedTime >= fMoveTime )
			{
				vCurrPos = vInitialPos;
				fEffectTime = fMoveTime;
				eState = ES_BOUNCE_OUT;
			}
			else
			{
				vCurrPos = vOuterPos - GetDelta( vSpeed, vAccel, fElapsedTime );
				fEffectTime = fElapsedTime;
			}
			break;
		}

		case ES_BOUNCE_OUT:
		{
			if ( fElapsedTime >= fMoveTime + fMoveTime2 )
			{
				vCurrPos = vBouncePos;
				fEffectTime = fMoveTime + fMoveTime2;
				eState = ES_BOUNCE_IN;
			}
			else
			{
				vCurrPos = vInitialPos + GetDelta( vSpeedBounceOut, vAccelBounceOut, fElapsedTime - fMoveTime );
				fEffectTime = fElapsedTime;
			}
			break;
		}

		case ES_BOUNCE_IN:
		{
			if ( fElapsedTime >= fMoveTime + fMoveTime2 * 2.0f )
			{
				vCurrPos = vInitialPos;
				fEffectTime = fMoveTime + fMoveTime2 * 2.0f;
				eState = ES_NONE;
			}
			else
			{
				vCurrPos = vBouncePos - GetDelta( vSpeedBounceIn, vAccelBounceIn, fElapsedTime - (fMoveTime + fMoveTime2) );
				fEffectTime = fElapsedTime;
			}
			break;
		}

		case ES_BORDER_IN:
		{
			if ( fElapsedTime >= fMoveTime2 )
			{
				vCurrPos = vBouncePos;
				vCurrSize = vBounceSize;
				fEffectTime = fMoveTime2;
				eState = ES_BORDER_OUT;
			}
			else
			{
				vCurrPos = vBouncePos + GetDelta( vSpeedBounceIn, vAccelBounceIn, fElapsedTime );
				vCurrSize = vInitialSize - 2 * GetDelta( vSpeedBounceIn, vAccelBounceIn, fElapsedTime );
				fEffectTime = fElapsedTime;
			}
			break;
		}

		case ES_BORDER_OUT:
		{
			if ( fElapsedTime >= fMoveTime2 * 2.0f )
			{
				vCurrPos = vInitialPos;
				vCurrSize = vInitialSize;
				fEffectTime = fMoveTime2 * 2.0f;
				eState = ES_NONE;
			}
			else
			{
				vCurrPos = vBouncePos - GetDelta( vSpeedBounceIn, vAccelBounceIn, fElapsedTime - fMoveTime2 );
				vCurrSize = vBounceSize + 2 * GetDelta( vSpeedBounceOut, vAccelBounceOut, fElapsedTime - fMoveTime2 );
				fEffectTime = fElapsedTime;
			}
			break;
		}
	};

	//pElement->SetPlacement( vCurrPos.x, vCurrPos.y, vCurrSize.x, vCurrSize.y, EWPF_ALL );
	if ( eState == ES_NONE )
	{
		if ( bGoOut )
		{
			// восстановим исходное положение для возможного повторного эффекта
			pElement->SetPlacement( vInitialPos.x, vInitialPos.y, vInitialSize.x, vInitialSize.y, EWPF_ALL );
			pElement->ShowWindow( false );
		}
		fElapsedTime = fEffectTime;
		return timeDiff; // consume all time for better visual effect
	}
	else
		pElement->ShowWindow( true );

	// return consumed time
	return fEffectTime - fFormerElapsedTime;
}

REGISTER_SAVELOAD_CLASS(0x171B2B80,SWindowContextB2Move)
REGISTER_SAVELOAD_CLASS(0x171B1C42,CEffectorB2Move)


