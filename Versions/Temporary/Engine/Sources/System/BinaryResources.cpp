#include "stdafx.h"

#include "BinaryResources.h"
#include "VFSOperations.h"
#include "Misc/StrProc.h"

namespace NBinResources
{

bool IsEmptyGUID( const GUID &uid )
{
	const GUID uidNull = { 0, 0, 0, {0,0,0,0,0,0,0,0} };
	return IsEqualGUID( uid, uidNull );
}

string GUIDToString( const GUID &uid )
{
	return StrFmt( "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", uid.Data1, uid.Data2, 
		uid.Data3, uid.Data4[0], uid.Data4[1], uid.Data4[2], uid.Data4[3], uid.Data4[4], 
		uid.Data4[5], uid.Data4[6], uid.Data4[7] );
}

string GetBinaryFileName( const string &rszDirPrefix, const int nRecordID, const GUID &uid )
{
	string szDirPrefix = rszDirPrefix;
	NStr::TrimRight( szDirPrefix, '\\' );
	if ( IsEmptyGUID( uid ) )
	{
		DebugTrace( "Empty resource GUID: %s\\%d", szDirPrefix.c_str(), nRecordID );
		return StrFmt( "%s\\%d", szDirPrefix.c_str(), nRecordID );
	}
	return StrFmt( "%s\\%s", szDirPrefix.c_str(), GUIDToString( uid ).c_str() );
}

string GetExistentBinaryFileName( const string &rszDirPrefix, const int nRecordID, const GUID &uid )
{
	string szName = GetBinaryFileName( rszDirPrefix, nRecordID, uid );

	if ( NVFS::GetMainVFS()->DoesFileExist( szName ) )
		return szName;

	string szDirPrefix = rszDirPrefix;
	NStr::TrimRight( szDirPrefix, '\\' );
	return StrFmt( "%s\\%d", szDirPrefix.c_str(), nRecordID );
}

}

