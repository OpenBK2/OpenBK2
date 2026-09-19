#include "stdafx.h"

#include "BuildDataView.h"

#include "MapEditorLib/Interface_View.h"

// The build data dialog's rule for OK, which the dialog (BuildDataViewWx.cpp)
// asks.

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
}
