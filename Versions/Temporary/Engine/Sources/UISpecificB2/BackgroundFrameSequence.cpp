#include "StdAfx.h"
#include "../ui/commandparam.h"
#include "../ui/dbuserinterface.h"
#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../system/time.h"
#include "BackgroundFrameSequence.h"
#include "../UI/UIVisitor.h"
#include "../3DMotor/RectLayout.h"
#include "../UI/UIComponents.h"
#include "../Main/GameTimer.h"
#include "../Misc/Win32Random.h"

// CBackgroundFrameSequence

CBackgroundFrameSequence::CBackgroundFrameSequence() :
	timeStart( 0 )
{
}

void CBackgroundFrameSequence::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	CBackground::InitByDesc( _pDesc );
	pStats = checked_cast<const NDb::SBackgroundFrameSequence*>( _pDesc );
	pTexture = pStats->pTexture;
	if ( pStats )
	{
		if ( pStats->bRandomStartFrame && pStats->nFrameCount > 0 )
			nFrame = NWin32Random::Random( 0, pStats->nFrameCount - 1 );
		else
			nFrame = 0;
		nTime = pStats->nTime;
		if ( pStats->nRandomAddTime > 0 )
			nTime += NWin32Random::Random( 0, pStats->nRandomAddTime - 1 );
	}
	NI_ASSERT( !pStats || (pStats->nFrameCount <= pStats->nFrameCountX * pStats->nFrameCountY), "Too low frames at texture" );
#ifndef _FINALRELEASE
	if ( CHECK_UI_TEXTURES_INSTANT_LOAD != 0 )
	{
		CheckInstantLoadTexture( pTexture );
	}
#endif
}

void CBackgroundFrameSequence::Visit( IUIVisitor * pVisitor )
{
	if ( pos.IsEmpty() || !pStats || !pTexture )
		return;

	if ( nTime > 0 && pStats->nFrameCount > 0 )
	{
		NTimer::STime time = Singleton<IGameTimer>()->GetAbsTime();

		if ( timeStart == 0 )
			timeStart = time;

		int nDeltaTime = time - timeStart;
		if ( nDeltaTime >= nTime )
		{
			nFrame = (nFrame + nDeltaTime / nTime) % pStats->nFrameCount;
			timeStart = time - nDeltaTime % nTime;
		}
	}
	
	int nX = 0;
	int nY = 0;
	if ( pStats->nFrameCountX > 0 )
	{
		nX = nFrame % pStats->nFrameCountX;
		nY = nFrame / pStats->nFrameCountX;
	}
	
	CVec2 vVirtSize;
	vVirtSize.x = Min( (float)( pos.Width() ), (float)( pStats->vFrameSize.x ) );
	vVirtSize.y = Min( (float)( pos.Height() ), (float)( pStats->vFrameSize.y ) );
	NGfx::SPixel8888 sColor = FadeColor( pStats->nColor, GetFadeValue() );

	CRectLayout rects( pos.x1, pos.y1, vVirtSize.x, vVirtSize.y, 
		nX * pStats->vFrameSize.x, nY * pStats->vFrameSize.y, 
		pStats->vFrameSize.x, pStats->vFrameSize.y, FadeColor( sColor.dwColor, GetFadeValue() ) );
	VirtualToScreen( &rects );

	pVisitor->VisitUIRect( pTexture, 3, rects );
}

void CBackgroundFrameSequence::SetTexture( const struct NDb::STexture *pDesc )
{
	pTexture = pDesc;
#ifndef _FINALRELEASE
	if ( CHECK_UI_TEXTURES_INSTANT_LOAD != 0 )
	{
		CheckInstantLoadTexture( pTexture );
	}
#endif
}

REGISTER_SAVELOAD_CLASS( 0x171C1B40, CBackgroundFrameSequence )


