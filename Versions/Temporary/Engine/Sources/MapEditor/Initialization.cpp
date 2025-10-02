#include "StdAfx.h"

bool IsRunningOnLocalDrive()
{
#if defined(_DO_ASSERT_SLOW) && !defined(_FINALRELEASE)
	// check for remote drive - not allowed to run from!
	char buffer[2048];
	GetModuleFileName( 0, buffer, 2048 );
	string szModuleDir = buffer;
	szModuleDir.erase( szModuleDir.find( '\\' ) );
	if ( szModuleDir.empty() )
		return true;
	if ( szModuleDir[szModuleDir.size() - 1] != '\\' )
		szModuleDir += '\\';
	//
	if ( GetDriveType(szModuleDir.c_str()) == DRIVE_REMOTE )
	{
		::MessageBox( 0, "Program can't be launched from the remote drive!", "ERROR", MB_OK | MB_ICONEXCLAMATION );
		return false;
	}

#endif // use ctrl + }
	return true;
}


