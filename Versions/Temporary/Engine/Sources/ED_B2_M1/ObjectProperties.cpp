#include "stdafx.h"

#include "ObjectProperties.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/Interface_Controller.h"
#include "MapEditorLib/Interface_View.h"
#include "MapEditorLib/MultiManipulator.h"
#include "libdb/ResourceManager.h"

#include <cstdint>
#include <string>

namespace NObjectProperties
{
	void Show( const SObjectSet &rObjectSet )
	{
		IResourceManager *pResourceManager = Singleton<IResourceManager>();
		CPtr<IManipulator> pObjectManipulator = 0;
		{
			CMultiManipulator *pMultiManipulator = new CMultiManipulator();
			for ( CObjectNameSet::const_iterator itObjectName = rObjectSet.objectNameSet.begin(); itObjectName != rObjectSet.objectNameSet.end(); ++itObjectName )
			{
				pMultiManipulator->InsertManipulator( itObjectName->first, pResourceManager->CreateObjectManipulator( rObjectSet.szObjectTypeName, itObjectName->first ), false, false );
			}
			pObjectManipulator = pMultiManipulator;
		}
		IView *pView = 0;
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_GET_VIEW, reinterpret_cast<uintptr_t>( &pView ) );
		if ( pView != 0 )
		{
			pView->SetViewManipulator( pObjectManipulator, rObjectSet, std::string() );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_VIEW, ID_VIEW_SHOW_PROPERTY_BROWSER, 1 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_CREATE_TREE, 0 );
		}
	}
}
