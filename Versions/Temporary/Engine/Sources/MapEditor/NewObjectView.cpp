#include "stdafx.h"

#include <fmt/printf.h>

#include "NewObjectView.h"
#include "ResourceDefines.h"

#include "MapEditorLib/Interface_FolderCallback.h"
#include "Misc/StrProc.h"

// "Create New <type> Object": the two rules that are not about controls, and
// the title, which the dialog (NewObjectViewWx.cpp) uses.

namespace
{
	// "_<type>", which is what the Add Type button puts on the end of a name.
	std::string TypePostfix( const SBuildDataParams *pBuildDataParams )
	{
		return std::string( "_" ) + pBuildDataParams->szObjectTypeName;
	}


	std::string Lower( const std::string &rszText )
	{
		std::string szLower = rszText;
		NStr::ToLower( &szLower );
		return szLower;
	}
}


namespace NNewObject
{
	void ApplyTypePostfix( SBuildDataParams *pBuildDataParams, bool bAddType )
	{
		if ( pBuildDataParams == 0 )
		{
			return;
		}
		const std::string szPostfix = TypePostfix( pBuildDataParams );
		const int nPostfixSize = szPostfix.size();
		const int nNameSize = pBuildDataParams->szObjectName.size();

		bool bPostfixExists = false;
		if ( nPostfixSize <= nNameSize )
		{
			bPostfixExists = ( Lower( pBuildDataParams->szObjectName ).compare(
													 nNameSize - nPostfixSize, nPostfixSize, Lower( szPostfix ) ) == 0 );
		}
		if ( bAddType && !bPostfixExists )
		{
			pBuildDataParams->szObjectName += szPostfix;
		}
		else if ( !bAddType && bPostfixExists )
		{
			pBuildDataParams->szObjectName =
				pBuildDataParams->szObjectName.substr( 0, nNameSize - nPostfixSize );
		}
	}


	bool CanAccept( SBuildDataParams *pBuildDataParams, const std::string &rszTypedName )
	{
		if ( pBuildDataParams == 0 )
		{
			return false;
		}
		// The second condition is the one worth reading twice: with Add Type on
		// and nothing typed, the name is exactly "_<type>", which is not a name.
		std::string szObjectName;
		pBuildDataParams->GetObjectName( &szObjectName );
		return ( !rszTypedName.empty() ) &&
					 ( Lower( pBuildDataParams->szObjectName ) != Lower( TypePostfix( pBuildDataParams ) ) ) &&
					 ( Singleton<IFolderCallback>()->IsUniqueName( pBuildDataParams->szObjectTypeName,
																												szObjectName ) );
	}


	std::string Title( const SBuildDataParams *pBuildDataParams )
	{
		if ( pBuildDataParams == 0 )
		{
			return std::string();
		}
		CString strFormat;
		strFormat.LoadString( IDS_PC_BD_DIALOG_TITLE );
		return fmt::sprintf( strFormat.GetString(), pBuildDataParams->szObjectTypeName.c_str() );
	}
}
