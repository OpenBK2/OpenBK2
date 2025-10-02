#include "StdAfx.h"
#include "..\ui\commandparam.h"
#include "..\ui\dbuserinterface.h"
#include "..\ui\ui.h"
#include "..\input\gamemessage.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\3dmotor\rectlayout.h"
#include "..\system\time.h"
#include "WindowFrameSequence.h"
#include "../UI/UIVisitor.h"
#include "../Main/GameTimer.h"
#include "../UI/Background.h"
#include "../Misc/Win32Random.h"

// CWindowFrameSequence

CWindowFrameSequence::CWindowFrameSequence() :
	timeStart( 0 )
{
}

CWindowFrameSequence::~CWindowFrameSequence()
{
}

void CWindowFrameSequence::Visit( interface IUIVisitor *pVisitor )
{
	CTRect<float> rc;
	FillWindowRect( &rc );
	VirtualToScreen( rc, &rc );
	CClipStore s( pVisitor, rc );

	CWindow::Visit( pVisitor );

	if ( pShared )
	{
		if ( timeStart == 0 )
			timeStart = Singleton<IGameTimer>()->GetAbsTime();

		if ( bRun && nTime > 0 && pShared->nFrameCount > 0 )
		{
			NTimer::STime time = Singleton<IGameTimer>()->GetAbsTime();
			int nDeltaTime = time - timeStart;
			if ( nDeltaTime >= nTime )
			{
				nFrame = (nFrame + nDeltaTime / nTime) % pShared->nFrameCount;
				timeStart = time - nDeltaTime % nTime;
			}
		}
		
		int nX = 0;
		int nY = 0;
		if ( pShared->nFrameCountX > 0 )
		{
			nX = nFrame % pShared->nFrameCountX;
			nY = nFrame / pShared->nFrameCountX;
		}
		
		CTRect<float> rect = GetWindowRect();

		CRectLayout rects( rect.x1, rect.y1, rect.Width(), rect.Height(), 
			nX * pShared->vFrameSize.x, nY * pShared->vFrameSize.y, pShared->vFrameSize.x, pShared->vFrameSize.y,
			FadeColor( 0xFFFFFFFF, GetTotalFadeValue() ) );
		VirtualToScreen( &rects );

		pVisitor->VisitUIRect( pShared->pTexture, 3, rects );
	}
}

void CWindowFrameSequence::InitByDesc( const struct NDb::SUIDesc *pDesc )
{
	pInstance = checked_cast<const NDb::SWindowFrameSequence*>( pDesc )->Duplicate();
	pShared = checked_cast_ptr<const NDb::SWindowFrameSequenceShared*>( pInstance->pShared );
	
	if ( pShared )
	{
		if ( pShared->bRandomStartFrame && pShared->nFrameCount > 0 )
			nFrame = NWin32Random::Random( 0, pShared->nFrameCount - 1 );
		else
			nFrame = 0;
		nTime = pShared->nTime;
		if ( pShared->nRandomAddTime > 0 )
			nTime += NWin32Random::Random( 0, pShared->nRandomAddTime - 1 );
	}
	bRun = false;
	
	CWindow::InitByDesc( pDesc );
}

void CWindowFrameSequence::Run( bool _bRun )
{
	if ( bRun == _bRun )
		return;
		
	bRun = _bRun;
	if ( bRun )
		timeStart = Singleton<IGameTimer>()->GetAbsTime();
}

void CWindowFrameSequence::Reset()
{
	nFrame = 0;
	timeStart = Singleton<IGameTimer>()->GetAbsTime();
}

REGISTER_SAVELOAD_CLASS(0x1717A442, CWindowFrameSequence)

