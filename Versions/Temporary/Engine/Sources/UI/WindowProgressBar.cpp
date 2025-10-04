#include "stdafx.h"
#include "windowprogressbar.h"


REGISTER_SAVELOAD_CLASS(0x11075B88, CWindowProgressBar)
REGISTER_SAVELOAD_CLASS(0x170A53C0, CWindowMultiTextureProgressBar)

int CWindowProgressBar::operator&( IBinSaver &saver ) 
{ 
	saver.Add( 1, static_cast<CWindow*>( this ) );
	saver.Add( 2, &pForward );
	saver.Add( 3, &pBackward );
	saver.Add( 4, &pInstance );
	saver.Add( 5, &pShared );
	saver.Add( 6, &fStepSize );
	saver.Add( 7, &bShowFirstElement );
	return 0; 
}

void CWindowProgressBar::Reposition( const CTRect<float> &parentRect )
{
	CWindow::Reposition( parentRect );
	SetPosition( pInstance->fProgress );
}

void CWindowProgressBar::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindowProgressBar *pDesc( checked_cast<const NDb::SWindowProgressBar*>( _pDesc ) );
	pInstance = pDesc->Duplicate();

	CWindow::InitByDesc( _pDesc );
	pShared = checked_cast_ptr<const NDb::SWindowProgressBarShared *>( pDesc->pShared );
	pForward = CUIFactory::MakeWindowPart( pShared->pForward );
	pBackward = CUIFactory::MakeWindowPart( pShared->pBackward );
	pGlow = CUIFactory::MakeWindowPart( pShared->pGlow );
	fStepSize = pShared->fStepSize;
	bShowFirstElement = false;
}

void CWindowProgressBar::Visit( struct IUIVisitor *pVisitor )
{
	CWindow::Visit( pVisitor );
	if ( pForward )
		pForward->Visit( pVisitor );
	if ( pBackward )
		pBackward->Visit( pVisitor );
	VisitText(pVisitor);
	if ( pGlow && GetPosition() > 0.f && GetPosition() < 1.f)
		pGlow->Visit( pVisitor );
}

void CWindowProgressBar::SetPosition( const float fPos ) 
{ 
	pInstance->fProgress = Clamp( fPos, 0.0f, 1.0f ); 
	CTRect<float> rect;
	FillWindowRect( &rect );

	// resize backgrounds
	float fProgressSize = rect.Width() * pInstance->fProgress;
	if ( fStepSize > 0.0f )
	{
		if ( bShowFirstElement )
		{
			fProgressSize = (rect.Width() - fStepSize) * pInstance->fProgress;
			fProgressSize = fStepSize * (floorf( fProgressSize / fStepSize ) + 1.0f);
		}
		else
			fProgressSize = fStepSize * floorf( fProgressSize / fStepSize );
		fProgressSize = Clamp( fProgressSize, 0.0f, rect.Width() );
	}

	const CVec2 vForwardPos( rect.x1, rect.y1 );
	const CVec2 vForwardSize ( fProgressSize, rect.Height() );

	const CVec2 vBackwardPos( vForwardPos.x + vForwardSize.x, vForwardPos.y );
	const CVec2 vBackwardSize( rect.Width() - vForwardSize.x, rect.Height() );

	const CVec2 vGlowPos( vForwardPos.x + vForwardSize.x - pShared->vGlowSize.x / 2.f, rect.GetCenter().y - pShared->vGlowSize.y / 2.f );

	if ( pForward )
		pForward->SetPos( vForwardPos, vForwardSize );

	if ( pBackward )
		pBackward->SetPos( vBackwardPos, vBackwardSize );

	if ( pGlow )
		pGlow->SetPos( vGlowPos, pShared->vGlowSize );
}

void CWindowProgressBar::ShowFirstElement( bool bShow )
{
	bShowFirstElement = bShow;
	SetPosition( pInstance->fProgress );
}

void CWindowProgressBar::SetForward( const NDb::SBackground *_pForward )
{
	pForward = CUIFactory::MakeWindowPart( _pForward );
}

// CWindowMultiTextureProgressBar

int CWindowMultiTextureProgressBar::operator&( IBinSaver &saver ) 
{ 
	saver.Add( 1, static_cast<CWindow*>( this ) );
	saver.Add( 2, &parts );
	saver.Add( 4, &pInstance );
	saver.Add( 5, &pShared );
	return 0; 
}

void CWindowMultiTextureProgressBar::Reposition( const CTRect<float> &parentRect )
{
	CWindow::Reposition( parentRect );
}

void CWindowMultiTextureProgressBar::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindowMultiTextureProgressBar *pDesc( checked_cast<const NDb::SWindowMultiTextureProgressBar*>( _pDesc ) );
	pInstance = pDesc->Duplicate();

	CWindow::InitByDesc( _pDesc );
	pShared = checked_cast_ptr<const NDb::SWindowMultiTextureProgressBarShared *>( pDesc->pShared );
	parts.reserve( pShared->states.size() );
	for ( vector< NDb::SMultiTextureProgressBarSharedState >::const_iterator it = pShared->states.begin(); 
		it != pShared->states.end(); ++it )
	{
		const NDb::SMultiTextureProgressBarSharedState &state = *it;
		parts.push_back( CUIFactory::MakeWindowPart( state.pBackground ) );
	}
}

void CWindowMultiTextureProgressBar::Visit( struct IUIVisitor *pVisitor )
{
	CWindow::Visit( pVisitor );
	
	CTRect<float> rect;
	FillWindowRect( &rect );

	if ( pInstance->bSolid )
	{
		NI_ASSERT( !pInstance->progresses.empty(), "Not enough values for multi texture progress bar" );
		int nStateIndex = FindStateIndex( pInstance->progresses.front() );
		
		const CVec2 vPos( rect.x1, rect.y1 );
		const CVec2 vSize ( rect.Width() * pInstance->progresses.front(), rect.Height() );
		parts[nStateIndex]->SetPos( vPos, vSize );
		parts[nStateIndex]->Visit( pVisitor );
	}
	else
	{
		for ( int i = 0; i < pInstance->progresses.size(); ++i )
		{
			float fProgress = pInstance->progresses[i];
			int nStateIndex = FindStateIndex( fProgress );
			
			const CVec2 vSize( pInstance->fPieceSize, rect.Height() );
			const CVec2 vPos( rect.x1 + vSize.x * i, rect.y1 );
			parts[nStateIndex]->SetPos( vPos, vSize );
			parts[nStateIndex]->Visit( pVisitor );
		}
	}
}

bool CWindowMultiTextureProgressBar::IsSolid() const
{
	return pInstance->bSolid;
}

void CWindowMultiTextureProgressBar::GetPositions( vector<float> *pPositions ) const
{
	*pPositions = pInstance->progresses;
}

void CWindowMultiTextureProgressBar::SetPositions( const vector<float> &positions, bool bSolid )
{
	NI_ASSERT( !positions.empty(), "Not enough values for multi texture progress bar" );
	pInstance->progresses = positions;
	pInstance->bSolid = bSolid;
}

int CWindowMultiTextureProgressBar::FindStateIndex( float fProgress )
{
	int nIndex = 0;
	for ( vector< NDb::SMultiTextureProgressBarSharedState >::const_iterator it = pShared->states.begin(); 
		it != pShared->states.end(); ++it, ++nIndex )
	{
		const NDb::SMultiTextureProgressBarSharedState &state = *it;
		if ( fProgress <= state.fValue )
			break;
	}
	return min( nIndex, parts.size() - 1 );
}


