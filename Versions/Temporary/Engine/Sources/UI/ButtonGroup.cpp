#include "StdAfx.h"
#include ".\buttongroup.h"
#include "WindowMSButton.h"

REGISTER_SAVELOAD_CLASS( 0x11075AC0, CButtonGroup )

IWindow * CButtonGroup::ChooseDefault()
{
	IWindow *pDefault = 0;
	int nMinY = -1;
	int nMinX = -1;
	for ( CButtons::iterator it = buttons.begin(); it != buttons.end(); ++it )
	{
		int nX, nY;
		(*it)->GetPlacement( &nX, &nY, 0, 0 );
		if ( nMinX == -1 || nMinY == -1 || nX < nMinX || nY < nMinY )
		{
			pDefault = *it;
			nMinX = nX;
			nMinY = nY;
		}
	}
	return pDefault;
}

void CButtonGroup::Add( IWindow * pButton ) 
{ 
	NI_ASSERT( dynamic_cast<CWindowMSButton*>( pButton ) != 0, "not button passed" );
	buttons.insert( dynamic_cast<CWindowMSButton*>( pButton ) ); 
}

void CButtonGroup::SetActive( IWindow *_pButton )
{
	/*IButton *pButton = dynamic_cast<IButton*>( _pButton);
	if ( pButton )
	{
	//for ( CButtons::iterator it = buttons.begin(); it != buttons.end(); ++it )
	//( *it )->SetState( *it == pPressed ? 1 : 0 );
	if ( )
	}*/
	TrySwitchState( _pButton );
	dynamic_cast_ptr<IButton*>( pPressed )->SetState( 1 );
}

bool CButtonGroup::TrySwitchState( IWindow *pButton )
{
	if ( dynamic_cast<IButton*>( pButton )->GetState() == 1 )
	{
		// attempt to depress currently pressed button
		return false;
	}
	// depress pressed
	if ( pPressed )
		dynamic_cast_ptr<IButton*>( pPressed )->SetState( 0 );
	// remember new pressed
	pPressed = pButton;
	return true;
}

void CButtonGroup::Init()
{
	// somehow choose button to be pressed, press it.
	// depress all others.
	pPressed = ChooseDefault();
	for ( CButtons::iterator it = buttons.begin(); it != buttons.end(); ++it )
		( *it )->SetState( *it == pPressed ? 1 : 0 );
}

IWindow* CButtonGroup::GetPressed() const
{
	return pPressed;
}

IWindow* CButtonGroup::GetButton( int i )
{	
	int k = 0;
	for ( CButtons::iterator it = buttons.begin(); it != buttons.end(); ++it  )
	{
		++k;
		if ( k == i )
			return *it;
	}
	return 0;
}

void CButtonGroup::Remove_All()
{
	buttons.clear();
	pPressed = 0;
}


