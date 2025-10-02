#include "stdafx.h"
#include "mapeditorlib/resourcedefines.h"

#include "PC_BaseDialog.h"

bool CPCBaseDialog::HandleCommand( UINT nCommandID, DWORD dwData )
{
	switch( nCommandID )
	{
		case ID_PC_DIALOG_GET_VIEW:
			if ( dwData != 0 )
			{
				IView **ppView = reinterpret_cast<IView**>( dwData );
				( *ppView ) = GetView();
				return true;
			}
			return false;
		case ID_PC_DIALOG_GET_COMMAND_HANDLER:
			if ( dwData != 0 )
			{
				ICommandHandler **ppCommandHandler = reinterpret_cast<ICommandHandler**>( dwData );
				( *ppCommandHandler ) = GetCommandHandler();
				return true;
			}
			return false;
		case ID_PC_DIALOG_CREATE_TREE:
			CreateTree();
			return true;
		case ID_PC_DIALOG_UPDATE_VALUES:
			UpdateValues();
		default:
			return false;
	}
}


bool CPCBaseDialog::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CPCBaseDialog::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CPCBaseDialog::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_PC_DIALOG_GET_VIEW:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_PC_DIALOG_GET_COMMAND_HANDLER:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_PC_DIALOG_CREATE_TREE:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_PC_DIALOG_UPDATE_VALUES:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		default:
			return false;
	}
}

// basement storage  


