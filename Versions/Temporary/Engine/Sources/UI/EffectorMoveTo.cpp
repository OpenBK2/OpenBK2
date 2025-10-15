// EffectorMoveTo.cpp: implementation of the CEffectorMoveTo class.
//


#include "stdafx.h"
#include "EffectorMoveTo.h"

#include "Window.h"

// Construction/Destruction

REGISTER_SAVELOAD_CLASS(UI, 0x11075C01, CEffectorMoveTo)

int CEffectorMoveTo::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pElement );
	saver.Add( 2, &bFinished );
	saver.Add( 3, &vMoveOffset );
	saver.Add( 4, &fMoveTime );												// points per second
	saver.Add( 5, &vSpeed );														// speed
	saver.Add( 6, &fElapsedTime );											// time elapsed so far
	saver.Add( 7, &vMoveFrom );
	saver.Add( 8, &bForward );

	return 0;
}

void CEffectorMoveTo::Configure( const NDb::SUIStateBase *_pCmd, struct IScreen *pScreen, SWindowContext *pContext, const std::string &szAnimatedWindow )
{ 
	const NDb::SUISMoveTo *pCmd( checked_cast<const NDb::SUISMoveTo*>( _pCmd ) );
	CParam<CVec2> vOffset( pCmd->vOffset );
	CParam<float> moveTime( pCmd->fMoveTime );
	CParam<std::string> elementToMove( pCmd->szElementToMove );
	if ( pContext )
	{
		vOffset.Merge( pContext->vOffset );
		moveTime.Merge( pContext->fMoveTime );
		elementToMove.Merge( pContext->szElementToMove );
	}
	else if ( !szAnimatedWindow.empty() )
	{
		CParam<std::string> szAnimated( szAnimatedWindow );
		elementToMove.Merge( szAnimated );
	}
	NI_ASSERT( vOffset.IsValid(), "OFFSET is invalid" );
	NI_ASSERT( moveTime.IsValid(), "MOVE TIME is invalid" );
	NI_ASSERT( elementToMove.IsValid(), "szElementToMove is invalid" );

	vMoveOffset = vOffset.Get();
	fMoveTime = moveTime.Get();
	bForward = true;

	pElement = dynamic_cast<CWindow*>( pScreen->GetElement( elementToMove.Get(), true ) );
	NI_ASSERT( pElement != 0, StrFmt( "no element \"%s\"", elementToMove.Get().c_str() ) );
	if ( pElement )
	{
		if ( fMoveTime == 0 ) 
		{
			bFinished = true;
			return;
		}
		bFinished = false;
		int x, y;
		pElement->GetPlacement( &x, &y, 0, 0 );
		vMoveFrom = CVec2( x, y );
		vSpeed = vMoveOffset;
		const float fSpeed = fabs( vSpeed ) / fMoveTime;
		if ( fSpeed == 0.0f )
		{
			bFinished = true;
			return;
		}
		Normalize( &vSpeed );
		vSpeed *= fSpeed;
		fElapsedTime = 0;
	}
}

void CEffectorMoveTo::Reverse()
{
	std::pair<CVec2,int> res( GetCur() );
	bForward = !bForward;
			
	const CVec2 vTmp( vMoveFrom );
	vMoveFrom = vMoveOffset + vMoveFrom;
	vMoveOffset = -vMoveOffset;
	
	fElapsedTime = fabs(res.first - vMoveFrom) / fabs( vSpeed );

	//fMoveTime = fElapsedTime;
	//fElapsedTime = 0;
	vSpeed.Negate();
	bFinished = false;
}

const std::pair<CVec2,int> CEffectorMoveTo::GetCur() const
{
	std::pair<CVec2,int> res;
	if ( fElapsedTime >= fMoveTime )
	{
		res.first = vMoveOffset + vMoveFrom;
		res.second = fElapsedTime - fMoveTime;
	}
	else
	{
		res.first = vMoveFrom + vSpeed * fElapsedTime;
		res.second = fElapsedTime;
	}
	return res;
}

const int CEffectorMoveTo::Segment( const int timeDiff, struct IScreen *pScreen, const bool bFastForward )
{
	const float fFormerElapsedTime = fElapsedTime;
	fElapsedTime += timeDiff;
	const std::pair<CVec2,int> res( GetCur() );

	if ( res.first == vMoveOffset + vMoveFrom )
	{
		bFinished = true;
		fElapsedTime = fMoveTime;
	}
	pElement->SetPlacement( res.first.x, res.first.y, 0, 0, EWPF_POS_X|EWPF_POS_Y );

	// return consumed time
	return res.second - fFormerElapsedTime;
}
