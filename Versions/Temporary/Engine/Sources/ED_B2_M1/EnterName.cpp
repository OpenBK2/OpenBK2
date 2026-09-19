#include "stdafx.h"

#include "EnterName.h"

// The name prompt's memory: the dialog (EnterNameWx.cpp) takes its starting
// text, and this keeps the last name accepted, as CEnterNameDialog once kept it
// itself.

namespace
{
	// The last name accepted, which the next prompt starts with -- including an
	// empty one, as CEnterNameDialog::OnOK always stored it. Per process, as the
	// static it replaces was.
	std::string szLastName;
}


namespace NEnterName
{
	bool Run( IWidget *pParent, const std::string &rszCaption, const std::string &rszLabel, std::string *pszName )
	{
		if ( pszName == 0 )
		{
			return false;
		}
		std::string szName = szLastName;
		if ( !RunWx( pParent, rszCaption, rszLabel, &szName ) )
		{
			return false;
		}
		szLastName = szName;
		( *pszName ) = szName;
		return true;
	}
}
