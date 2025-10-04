#include "stdafx.h"
#include "windowscrollablecontainer.h"

REGISTER_SAVELOAD_CLASS(0x170AF301, CWindowScrollableContainer)

int CWindowScrollableContainer::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CWindowScrollableContainerBase*>( this ) );
	saver.Add( 2, &pInstance );
	return 0;
}

void CWindowScrollableContainer::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindowScrollableContainer *pDesc( checked_cast<const NDb::SWindowScrollableContainer*>( _pDesc ) );
	pInstance = pDesc->Duplicate();
	CWindowScrollableContainerBase::InitByDesc( _pDesc );
}

void CWindowScrollableContainer::Select( IWindow *pElement )
{
	CWindowScrollableContainerBase::Select( pElement );
	//Play reaction
	if ( GetSelectedItem() ) {
		RunAnimationAndCommands( pInstance->onSelection, "", true, true );
	}
}

