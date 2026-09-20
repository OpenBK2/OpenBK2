#include "stdafx.h"

#include "DialogState.h"

#include "Interface_UserData.h"
#include "Tools_Resources.h"

#include "System/FilePath.h"

// Where every dialog keeps its placement and parameters. Was defined in
// ResizeDialog.h, with CResizeDialog, the MFC dialog base that first wrote
// these files. The separators are JoinPath's now, in GetStatePath: written
// out as backslashes this named one file that does not exist off Windows,
// and a dialog whose state cannot be read just opens with its defaults, so
// nothing reported it.
#define RESIZE_DIALOG_OPTIONS_FOLDER "ResizeDialogStyles"

// The chunk names and ids are exactly what CResizeDialog::SOptions used, because
// they are the on-disk format of every Editor/ResizeDialogStyles/*.xml that
// already exists. See the note in DialogState.h about "IntParameterss".

int SDialogState::operator&( IBinSaver &bs )
{
	bs.Add( 1, &rect );
	bs.Add( 2, &nParameters );
	bs.Add( 3, &szParameters );
	bs.Add( 4, &fParameters );
	return 0;
}


int SDialogState::operator&( IXmlSaver &xs )
{
	xs.Add( "Rect", &rect );
	xs.Add( "IntParameterss", &nParameters );
	xs.Add( "StringParameters", &szParameters );
	xs.Add( "FloatParameters", &fParameters );
	return 0;
}


namespace
{
	// The path CResizeDialog builds, in one place so the two callers cannot
	// disagree about it.
	bool GetStatePath( const std::string &rszDialogName, std::string *pszPath )
	{
		if ( rszDialogName.empty() )
		{
			return false;
		}
		IUserDataContainer *pContainer = Singleton<IUserDataContainer>();
		if ( pContainer == 0 || pContainer->Get() == 0 )
		{
			// Before the user data singleton exists there is nowhere to read
			// from, which is not an error: a dialog opening that early just gets
			// its defaults.
			return false;
		}
		( *pszPath ) = NFile::JoinPath( pContainer->Get()->constUserData.szStartFolder,
																		"Editor", RESIZE_DIALOG_OPTIONS_FOLDER, rszDialogName );
		return true;
	}
}


namespace NDialogState
{
	bool Load( const std::string &rszDialogName, SDialogState *pState )
	{
		std::string szPath;
		if ( pState == 0 || !GetStatePath( rszDialogName, &szPath ) )
		{
			return false;
		}
		return LoadXMLResource( szPath, ".xml", rszDialogName, *pState );
	}


	bool Save( const std::string &rszDialogName, SDialogState *pState )
	{
		std::string szPath;
		if ( pState == 0 || !GetStatePath( rszDialogName, &szPath ) )
		{
			return false;
		}
		return SaveXMLResource( szPath, ".xml", rszDialogName, *pState );
	}
}
