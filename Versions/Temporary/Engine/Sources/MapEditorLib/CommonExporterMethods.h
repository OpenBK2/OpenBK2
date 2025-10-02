#if !defined(__COMMON_EXPORTER_METHODS__)
#define __COMMON_EXPORTER_METHODS__
#pragma once

#include "Interface_UserData.h"


template<class CArray2DType>
void Trace2DByteArray( const CArray2DType &rArray, const string &rszAdditionalMessage )
{
	CTPoint<int> size( rArray.GetSizeX(), rArray.GetSizeY() );
	DebugTrace( "size:[%dx%d]%s", size.x, size.y, rszAdditionalMessage.c_str() );
	for ( int y = size.y - 1; y >= 0; --y )
	{
		string szTrace = StrFmt( "%02d |", y );
		for ( int x = 0; x < size.x; ++x )
		{
			szTrace += StrFmt( " %02x", rArray[y][x] );
		}
		DebugTrace( szTrace.c_str() );
	}
	string szTrace = ( "   -" );
	for ( int x = 0; x < size.x; ++x )
	{
		szTrace += "---";
	}
	DebugTrace( szTrace.c_str() );
	szTrace = ( "    " );
	for ( int x = 0; x < size.x; ++x )
	{
		szTrace += StrFmt( " %02d", x );
	}
	DebugTrace( szTrace.c_str() );
}


template<class CArray2DType>
void Trace2DFloatArray( const CArray2DType &rArray, const string &rszAdditionalMessage )
{
	CTPoint<int> size( rArray.GetSizeX(), rArray.GetSizeY() );
	DebugTrace( "size:[%dx%d]%s", size.x, size.y, rszAdditionalMessage.c_str() );
	for ( int y = size.y - 1; y >= 0; --y )
	{
		string szTrace = StrFmt( "%02d |", y );
		for ( int x = 0; x < size.x; ++x )
		{
			szTrace += StrFmt( " %06.03f", rArray[y][x] );
		}
		DebugTrace( szTrace.c_str() );
	}
	string szTrace = ( "   -" );
	for ( int x = 0; x < size.x; ++x )
	{
		szTrace += "-------";
	}
	DebugTrace( szTrace.c_str() );
	szTrace = ( "    " );
	for ( int x = 0; x < size.x; ++x )
	{
		szTrace += StrFmt( "     %02d", x );
	}
	DebugTrace( szTrace.c_str() );
}


// Берёт опцию из mayaExportData секции UserData.
// Производится lookup сначала в пользовательском наборе установок, затем в общем
// таким образом локальные установки пользователя имеют приоритет.
//
template<class T> const T & GetOption( const T SUserData::SMayaExportData::* pField )
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	return (pUserData->mayaExportData.*pField != T()
		? pUserData->mayaExportData.*pField
		: pUserData->constUserData.mayaExportData.*pField
		);
}


// get granny export settings file name for Maya export
string GetGrannyExportSettingsFileName( const string &szTypeName );

// cut extension from file name
void CutExtension( string *pFileName, const char *pszExt );
void MakeDoubleSlash( string *pszPath );
// convert \ to / to make normal (exceptable in maya scripts) file path
void NormalizeFilePath( string *pszPath );
// construct full source file path from reference value
bool BuildSrcFilePath( string *pszFilePath, const string &szRefValue );
// construct full source file path from reference field
bool BuildSrcFilePath( string *pszFilePath, struct IManipulator *pManipulator, const string &szRefFieldDBPath );
// construct full destination file path from nObjectID or "uid" field (if present)
string BuildDestFilePath( IManipulator* pManipulator, const string &szDestFolder );

// return true if bForced == true or if szSrc file is newer then szDst file
bool CheckFilesUpdated( const string &szSrc, const string &szDst, bool bForced );

// Interactive Maya support routines
// retrieves Maya install path from registry
void GetMayaInstallPath( string & szPath, const string &szMayaVersion );
// launches interactive maya instance and executes startup script
bool StartupMayaProcess( class CInteractiveMaya *pMayaProcess );
// Wait for file to be accessible
bool WaitForFile( const string &szFileName, const double fMaxWaitTime /* = 10000 */, bool bReportAsError = true );
// execute single script with interactive Maya
bool ExecuteMayaScript( const string &szScript );

// Non-interactive Maya Export (granny or particles)
// Сформировать первые строки скрипта экспорта результатов работы Maya в данные игры
void MEStartScript( string *pszScriptText, bool bGUIMode );
// Завершить формирование скрипта экспорта результатов работы Maya в данные игры
void MEFinishScript( string *pszScriptText, bool bGUIMode );
// Выполнить скрипт Maya
// bNeedExport - есть необходимость выполнять скрит ( если false - только сохранить скрипт на диск )
bool MERunScript( const string &rszScriptText, const string &rszFileNamePostfix, bool bNeedExport, bool bGUIMode );

// Получить размер картинки
bool GetDDSImageSize( const string &szImageFileName, CTPoint<int> *pSize  );
// Получить размер картинки
bool GetTGAImageSize( const string &szImageFileName, CTPoint<int> *pSize  );

bool GetSelectedObjects( SObjectSet *pObjectSet, const string &szObjectTypeName );

template<class TObjectHookFunctional>
bool ForEachObject( const SObjectSet &rObjectSet, TObjectHookFunctional objectHookFunctional )
{
	// CRAP{ PLAIN_TEXT
	ILogger *pLogger = NLog::GetLogger();
	for ( CObjectNameSet::const_iterator itObject = rObjectSet.objectNameSet.begin(); itObject != rObjectSet.objectNameSet.end(); ++itObject ) 
	{
		try
		{
			if ( objectHookFunctional( rObjectSet.szObjectTypeName, itObject->first ) == false )
			{
				pLogger->Log( LT_ERROR, "ForEachObject() processing failed\n" );
				pLogger->Log( LT_ERROR, StrFmt("\tType: %s, ID: %d\n", rObjectSet.szObjectTypeName.c_str(), itObject->first ) );
			}
		}
		catch ( ... ) 
		{
			pLogger->Log( LT_ERROR, "ForEachObject processing general fail\n" );
			pLogger->Log( LT_ERROR, StrFmt("\tType: %s, ID: %d\n", rObjectSet.szObjectTypeName.c_str(), itObject->first ) );
		}
	}
	return true;
	// CRAP} PLAIN_TEXT
}

//! move temp file, made during export, to real destination and register it in RCS
void MoveTempFileToDestination( const string &szTempFileFullName, const string &szDstFileFullName );

#endif // !defined(__COMMON_EXPORTER_METHODS__)
 
