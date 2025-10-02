#include "StdAfx.h"
#include "ObjectRecordIDAllocator.h"
#include "../Misc/HPTimer.h"
#include "../Misc/StrProc.h"
#include "../System/FileUtils.h"
#include "../System/Commands.h"

namespace NDb
{
namespace NObjectIDAllocator
{

static string s_szObjectIDFolderName;
void SetObjectRecordIDsFolderName( const string &szFolderName )
{
	s_szObjectIDFolderName = szFolderName;
}

class CFileLockHolder
{
	HANDLE lock;

public:
	CFileLockHolder( const string &szFileLockName )
	{
		do
		{
			::Sleep(100);
			lock = ::CreateFile(szFileLockName.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, 0);
			// FIXME: проверять GetLastError на sharing error, а то при просто некорректном имени файла всё повиснет
		}
		while( lock == INVALID_HANDLE_VALUE );
	}
	//
	~CFileLockHolder()
	{
		::CloseHandle( lock );
	}

};

int AllocateNewObjectID( const string &szClassTypeName )
{
	if ( s_szObjectIDFolderName.empty() )
		return -1;
	string szFolderName = s_szObjectIDFolderName;
	if ( szFolderName[szFolderName.size() - 1] != '\\' && szFolderName[szFolderName.size() - 1] != '/' )
		szFolderName += '\\';
	// create lock
	CFileLockHolder fileLockHolder( szFolderName + "ObjectIDs.lock" );
	// allocate new object ID
	const string szFileName = szFolderName + "ObjectIDs.ini";
	char buffer[1024];
	GetPrivateProfileString( "ClassTypeIDs", szClassTypeName.c_str(), "1000000", buffer, 1024, szFileName.c_str() );
	const int nObjectRecordID = buffer[0] == 0 ? 1000000 : NStr::ToInt( buffer );
	WritePrivateProfileString( "ClassTypeIDs", szClassTypeName.c_str(), 
		                         StrFmt("%d", nObjectRecordID + 1), szFileName.c_str() );
	return nObjectRecordID;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
}
}

