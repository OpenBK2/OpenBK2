#include "stdafx.h"
#include <fmt/format.h>

#include "PointListView.h"
#include "PointListDialog.h"
#include "CommandHandlerDefines.h"
#include "ED_B2_M1Dll.h"

#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/Interface_Logger.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

// The building editor's point lists as they have always been -- CPointListDialog,
// a CResizeDialog over IDD_TAB_BLD_POINTS -- and, in front of both
// implementations, the dispatch that used to live inside that class.

namespace
{
	// CHID_POINTS_LIST_DIALOG, for every list at once. The states name the list
	// they mean in the data's nInstanceID, and this finds it: what
	// CPointListDialog::HandleCommand did over its static otherDialogs.
	class CPointListDispatch : public ICommandHandler
	{
	public:
		// Registration order, as the static list had it; only the first list with
		// a given instance ID is ever asked, and there is one of each.
		std::vector<NPointListView::IPointList*> lists;

		virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData )
		{
			SPointListDialogData *pData = reinterpret_cast<SPointListDialogData*>( dwData );
			// Not guarded in the original, which read nInstanceID through it
			// first thing. Guarded as CPaletteCommands guards every other palette.
			if ( pData == 0 )
			{
				return false;
			}
			for ( NPointListView::IPointList *pList : lists )
			{
				if ( static_cast<unsigned>( pData->nInstanceID ) != pList->GetInstanceID() )
				{
					continue;
				}
				switch ( nCommandID )
				{
					case ID_WINDOW_GET_DIALOG_DATA:
						pList->GetDialogData( pData );
						return true;
					//
					case ID_WINDOW_SET_DIALOG_DATA:
						pList->SetDialogData( pData );
						return true;
				}
			}
			return false;
		}

		virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
		{
			NI_ASSERT( pbEnable != 0, "CPointListDispatch::UpdateCommand(), pbEnable == 0" );
			NI_ASSERT( pbCheck != 0, "CPointListDispatch::UpdateCommand(), pbCheck == 0" );
			//
			switch ( nCommandID )
			{
			case ID_WINDOW_GET_DIALOG_DATA:
			case ID_WINDOW_SET_DIALOG_DATA:
				( *pbEnable ) = true;
				( *pbCheck ) = false;
				return true;
			default:
				return false;
			}
		}
	};

	// A function-local static, so it exists before the first list registers
	// whatever order the module's statics are built in.
	CPointListDispatch& Dispatch()
	{
		static CPointListDispatch dispatch;
		return dispatch;
	}
}


namespace NPointListView
{
	void Register( IPointList *pList )
	{
		Dispatch().lists.push_back( pList );
		// Every constructor used to Set the handler to itself, so the last list
		// built answered. It is the same one handler each time now.
		Singleton<ICommandHandlerContainer>()->Set( CHID_POINTS_LIST_DIALOG, &Dispatch() );
	}


	void Unregister( IPointList *pList )
	{
		std::vector<IPointList*> &rLists = Dispatch().lists;
		rLists.erase( std::remove( rLists.begin(), rLists.end(), pList ), rLists.end() );
		// The first list destroyed used to remove the registration while four
		// were still alive. It goes with the last one now; the lists are only
		// ever destroyed together, with the building editor's controls.
		if ( rLists.empty() )
		{
			Singleton<ICommandHandlerContainer>()->Remove( CHID_POINTS_LIST_DIALOG, &Dispatch() );
		}
	}


	void FollowSeason( IPointList *pSource, NDb::ESeason eSeason )
	{
		for ( IPointList *pList : Dispatch().lists )
		{
			if ( pList != pSource )
			{
				pList->FollowSeason( eSeason );
			}
		}
	}


	CWnd* CreateMfc( CDefault3DTabWindow *pTabWindow, unsigned nInstanceID, const std::string &rszLabel )
	{
		CPointListDialog *pDialog = pTabWindow->AddNewTab( new CPointListDialog( nInstanceID, CString( rszLabel.c_str() ) ) );
		if ( pDialog == 0 )
		{
			return 0;
		}
		// The template lives in this module's resources, so the handle is
		// swapped for the call and put back, as BuildingEditor did. A failure is
		// logged and the tab still added, also as it did.
		AfxSetResourceHandle( theEDB2M1Instance );
		const BOOL bResult = pDialog->Create( CPointListDialog::IDD, pTabWindow );
		AfxSetResourceHandle( AfxGetInstanceHandle() );
		if ( !bResult )
		{
			NLog::GetLogger()->Log( LT_ERROR, fmt::format( "Creation of CPointListDialog dialog failed" ) );
		}
		return pDialog;
	}


	CWnd* Create( CDefault3DTabWindow *pTabWindow, unsigned nInstanceID, const std::string &rszLabel )
	{
#ifdef OBK2_WITH_WX
		// The same flag every migrated piece follows, so a session runs either
		// the MFC set or the wx set.
		const char *pszUseWx = std::getenv( "OBK2_WX_DIALOGS" );
		if ( pszUseWx != 0 && pszUseWx[0] != '0' && pszUseWx[0] != '\0' )
		{
			return CreateWx( pTabWindow, nInstanceID, rszLabel );
		}
#endif
		return CreateMfc( pTabWindow, nInstanceID, rszLabel );
	}
}
