#include "stdafx.h"
#include "MapEditorLib/ToolkitChoice.h"

#include "BuildDataView.h"
#include "PC_BuildDataDialog.h"

#include "MapEditorLib/Interface_View.h"
#include "MapEditorLib/MfcWidget.h"

#include <cstdlib>

// The build data dialog as it has always been, CPCBuildDataDialog, behind the
// boundary; the dispatcher; and the rule for OK, which the wx dialog uses too.

namespace
{
	// The same flag every migrated dialog follows, so a session runs either the
	// MFC set or the wx set.
	bool UseWx()
	{
		return NToolkit::UseWxViews();
	}
}


namespace NBuildData
{
	//CRAP{ PLAIN_TEXT
	bool CanAccept( SBuildDataParams *pBuildDataParams, IBuildDataCallback *pBuildDataCallback,
									IView *pView, std::string *pszErrorMessage )
	{
		std::string szErrorMessage;
		bool bEnableOKButton = ( pBuildDataParams != 0 );
		// The name has to be given, and no object of the type may have it yet.
		if ( bEnableOKButton && ( pBuildDataParams->nFlags & BDF_CHECK_FILE_NAME ) )
		{
			std::string szObjectName;
			pBuildDataParams->GetObjectName( &szObjectName );
			if ( pBuildDataParams->szObjectName.empty() )
			{
				bEnableOKButton = false;
				szErrorMessage = "Object name is invalid. Name can't be empty.";
			}
			// The dialog asked the callback without looking; without one there is
			// nobody to say the name is taken.
			else if ( ( pBuildDataCallback != 0 ) &&
								!pBuildDataCallback->IsUniqueObjectName( pBuildDataParams->szObjectTypeName, szObjectName ) )
			{
				bEnableOKButton = false;
				szErrorMessage = "Object name is invalid. Object already exists!";
			}
		}
		// The fields, as the builder judges them.
		if ( bEnableOKButton && ( pBuildDataParams->nFlags & BDF_CHECK_PROPERTIES ) )
		{
			if ( ( pBuildDataCallback != 0 ) && ( pView != 0 ) &&
					 !pBuildDataCallback->IsValidBuildData( pView->GetViewManipulator(), &szErrorMessage, pView ) )
			{
				bEnableOKButton = false;
			}
		}
		if ( pszErrorMessage != 0 )
		{
			( *pszErrorMessage ) = szErrorMessage;
		}
		return bEnableOKButton;
	}
	//CRAP} PLAIN_TEXT


	// What CBuilderContainer::FillBuildData did with the dialog, moved here.
	bool RunMfc( IWidget *pParent, IManipulator *pManipulator, const SObjectSet &rObjectSet,
							 const std::string &rszTemporaryLabel, SBuildDataParams *pBuildDataParams,
							 IBuildDataCallback *pBuildDataCallback )
	{
		CPCBuildDataDialog buildDataDialog( ToCWnd( pParent ) );
		buildDataDialog.SetBuildDataParams( pBuildDataParams );
		buildDataDialog.SetTemporaryLabel( rszTemporaryLabel );
		buildDataDialog.SetBuildDataCallback( pBuildDataCallback );
		buildDataDialog.GetView()->SetViewManipulator( pManipulator, rObjectSet, rszTemporaryLabel );
		const bool bResult = ( buildDataDialog.DoModal() == IDOK );
		buildDataDialog.GetView()->RemoveViewManipulator();
		return bResult;
	}


	bool Run( IWidget *pParent, IManipulator *pManipulator, const SObjectSet &rObjectSet,
						const std::string &rszTemporaryLabel, SBuildDataParams *pBuildDataParams,
						IBuildDataCallback *pBuildDataCallback )
	{
		if ( UseWx() )
		{
			return RunWx( pParent, pManipulator, rObjectSet, rszTemporaryLabel, pBuildDataParams, pBuildDataCallback );
		}
		return RunMfc( pParent, pManipulator, rObjectSet, rszTemporaryLabel, pBuildDataParams, pBuildDataCallback );
	}
}
