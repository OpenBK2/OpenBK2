#include "stdafx.h"
#include "InterfaceMPBase.h"
#include "GameXClassIDs.h"

#include "InterfaceMisc.h"

// CInterfaceMPScreenBase

CInterfaceMPScreenBase::CInterfaceMPScreenBase( const std::string &szInterfaceType, const std::string &szBindSection ) :
	CInterfaceScreenBase( szInterfaceType, szBindSection )
{
}

bool CInterfaceMPScreenBase::StepLocal( bool bAppActive )
{
	Singleton<IMPToUIManager>()->MPUISegment();
	
	bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );
	
	if ( bAppActive )
	{
		while ( CPtr< SMPUIMessage> pMsg = Singleton<IMPToUIManager>()->GetUIMessage() )
		{
			bool bResult = HandleMessage( pMsg );
			//NI_ASSERT( bResult, StrFmt( "Unhandled MP to UI message (code %d)", pMsg->eMessageType ) );
		}
	}

	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	//Singleton<IMPToUIManager>()->MPUISegment();
	
	return bResult;
}


