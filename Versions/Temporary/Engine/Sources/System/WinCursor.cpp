#include "stdafx.h"

#include "FilePath.h"
#include "FileReaders.h"
#include "FileUtils.h"
#include "VFSOperations.h"
#include "WinCursor.h"
#include "VFSOperations.h"

namespace NWinCursor
{
HCURSOR LoadCursor( const string &szFileName )
{
	const string szTempFile( NFile::GetTempFileName() );	
	{
		CFileStream file( NVFS::GetMainVFS(), szFileName );
		if ( !file.IsOk() )
			return 0;

		CFileStream tempFile( szTempFile, CFileStream::WIN_CREATE );
		if ( !tempFile.IsOk() )
			return 0;

		file.ReadTo( &tempFile, file.GetSize() );
	}

	HCURSOR hCursor = ::LoadCursorFromFile( szTempFile.c_str() );
	::DeleteFile( szTempFile.c_str() );

	return hCursor;
}
}


