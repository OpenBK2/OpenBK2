#include "stdafx.h"
#include <fmt/format.h>
#include "Interface_CommandHandler.h"
#include "CommandHandlerDefines.h"
#include "ResourceDefines.h"

#include "CommonExporterMethods.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/InteractiveMaya.h"
#include "System/FileUtils.h"
#include "Misc/HPTimer.h"
#include "Misc/StrProc.h"
#include "Image/DDS.h"
#include "Image/Targa.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <cstdint>
#include <thread>

std::string GetGrannyExportSettingsFileName( const std::string &szTypeName )
{
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	SUserData::SMayaExportData::CGrannyExportSettingsMap::const_iterator pos = 
		pUserData->constUserData.mayaExportData.grannyExportSettings.find( szTypeName );
	std::string szSettingsFileName = pos == pUserData->constUserData.mayaExportData.grannyExportSettings.end() ? "" : pos->second;
	NStr::ReplaceAllChars( &szSettingsFileName, '\\', '/' );
	return szSettingsFileName;
}


bool CheckFilesUpdated( const std::string &szSrc, const std::string &szDst, bool bForced )
{
	if ( bForced ) 
		return false;

	NVFS::SFileStats src, dst;
	if ( !NVFS::GetWinFileStats( &src, szSrc ) || !NVFS::GetWinFileStats( &dst, szDst ) )
		return false;

	return src.mtime < dst.mtime;
}

void MakeDoubleSlash( std::string *pszPath )
{
  NI_ASSERT( pszPath != 0, "MakeDoubleSlash() pszPath == 0" );
	//	
	for ( std::string::iterator itChar = pszPath->begin(); itChar != pszPath->end(); ++itChar )
	if ( ( *itChar ) == '\\' )
	{
		int nCount = 0;
		while ( ( itChar != pszPath->end() ) && ( ( *itChar ) == '\\' ) )
		{	
			++itChar;
			++nCount;
		}
		if ( ( nCount & 0x1 ) > 0 )
		{
			itChar = pszPath->insert( itChar, '\\' );
		}
	}
}


void MEStartScript( std::string *pszScriptText, bool bGUIMode )
{
	NI_ASSERT( pszScriptText != 0, "MEStartScript() pszScriptText == 0" );
	//
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	// Формируем тело скрипта
	pszScriptText->clear();
}


void MEFinishScript(std::string *pszScriptText, bool bGUIMode )
{
  NI_ASSERT( pszScriptText != 0, "MEFinishScript() pszScriptText == 0" );
	//
	if ( bGUIMode )
	{
		( *pszScriptText ) += "quit -a;\r\n";
	}
	// Раздваиваем все слеши (их должно быть четное количество в последовательностях
	MakeDoubleSlash( pszScriptText );
}


bool MERunScript( const std::string &rszScriptText, const std::string &rszFileNamePostfix, bool bNeedExport, bool bGUIMode )
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();

	// Записываем тело скрипта в файл
	const std::string &szUserDataScriptFileName = GetOption( &SUserData::SMayaExportData::szScriptFileName );
	const std::string &szUserDataLogFileName = GetOption( &SUserData::SMayaExportData::szLogFileName );

	std::string szScriptFileName;
	std::string szLogFileName;
	if ( rszFileNamePostfix.empty() )
	{
		szScriptFileName = szUserDataScriptFileName;
		szLogFileName = szUserDataLogFileName;
	}
	else
	{
		//szScriptFileName
		{
			const int nPointPos = szUserDataScriptFileName.rfind( '.' );
			if ( nPointPos != std::string::npos )
			{
				szScriptFileName = szUserDataScriptFileName.substr( 0, nPointPos );
			}
			else
			{
				szScriptFileName = szUserDataScriptFileName;
			}
			szScriptFileName += rszFileNamePostfix;
			if ( nPointPos != std::string::npos )
			{
				szScriptFileName += szUserDataScriptFileName.substr( nPointPos );
			}
		}
		//szLogFileName
		{
			const int nPointPos = szUserDataLogFileName.rfind( '.' );
			if ( nPointPos != std::string::npos )
			{
				szLogFileName = szUserDataLogFileName.substr( 0, nPointPos );
			}
			else
			{
				szLogFileName = szUserDataLogFileName;
			}
			szLogFileName += rszFileNamePostfix;
			if ( nPointPos != std::string::npos )
			{
				szLogFileName += szUserDataLogFileName.substr( nPointPos );
			}
		}
	}
	szScriptFileName = pUserData->constUserData.szStartFolder + szScriptFileName;
	szLogFileName = pUserData->constUserData.szStartFolder + szLogFileName;
	//
	bool bResult = true;
	{
		HANDLE hFile = ::CreateFile( szScriptFileName.c_str(),
																 GENERIC_WRITE,
																 FILE_SHARE_READ,
																 0,
																 CREATE_ALWAYS,
																 FILE_ATTRIBUTE_NORMAL,
																 0 );
		if ( hFile != INVALID_HANDLE_VALUE )
		{
			uint32_t dwBytesWritten = 0;
			bResult = WriteFile( hFile, rszScriptText.c_str(), rszScriptText.size(), &dwBytesWritten, 0 );
			if ( bResult )
			{
				bResult = ( dwBytesWritten == rszScriptText.size() );
			}
			::CloseHandle( hFile );
			hFile = 0;
		}
		else
		{
			bResult = false;
		}
	}
	if ( !bResult )
	{
		return false;
	}

	uint32_t dwResult = ERROR_SUCCESS;
	if ( bNeedExport )
	{
		// Запускаем Maya
		const std::string &szUserDataToolFileName = "maya.exe";//GetOption( &SUserData::SMayaExportData::szToolFileName );
		const std::string szGUIMode = bGUIMode ? " " : " -batch ";
		const std::string szCommandLine = fmt::format( "{}{}-script \"{}\" -log \"{}\"", 
																				szUserDataToolFileName.c_str(),
																				szGUIMode.c_str(),
																				szScriptFileName.c_str(),
																				szLogFileName.c_str() );
		//
		STARTUPINFO startinfo;
		PROCESS_INFORMATION procinfo;
		memset( &startinfo, 0, sizeof( STARTUPINFO ) );
		memset( &procinfo, 0, sizeof( PROCESS_INFORMATION ) );
		startinfo.cb = sizeof( startinfo );
		bResult = ::CreateProcess( 0, const_cast<char*>( szCommandLine.c_str() ), 0, 0, false, 0, 0, 0, &startinfo, &procinfo );
		if ( bResult )
		{
			const uint32_t dwWaitObject = ::WaitForSingleObject( procinfo.hProcess, INFINITE );
			::CloseHandle( procinfo.hProcess );
			::CloseHandle( procinfo.hThread );
		}
	}						
	return bResult;
}


// Получить размер картинки
bool GetDDSImageSize( const std::string &szImageFileName, CTPoint<int> *pSize )
{
	CFileStream stream( szImageFileName, CFileStream::WIN_READ_ONLY );
	if ( stream.IsOk() ) 
	{
		SDDSFileHeader hdr;
		stream.Read( &hdr, sizeof( hdr ) );
		pSize->x = hdr.header.dwWidth;
		pSize->y = hdr.header.dwHeight;
	}
	else
	{
		pSize->x = 0;
		pSize->y = 0;
	}
	//
	return true;
}


// Получить размер картинки
bool GetTGAImageSize( const std::string &szImageFileName, CTPoint<int> *pSize )
{
	CFileStream stream( NVFS::GetMainVFS(), szImageFileName );
	if ( stream.IsOk() ) 
	{
		NImage::STGAFileHeader hdr;
		NImage::LoadTGAHeader( &hdr, &stream );
		pSize->x = hdr.imagespec.wImageWidth;
		pSize->y = hdr.imagespec.wImageHeight;
	}
	else
	{
		pSize->x = 0;
		pSize->y = 0;
	}
	//
	return true;
}


void CutExtension( std::string *pFileName, const char *pszExt )
{
	const int nPos = pFileName->rfind( '.' );
	if ( nPos == std::string::npos ) 
		return;
	//
	if ( pFileName->compare(nPos + 1, -1, pszExt) == 0 )
		pFileName->resize( nPos );
}



void NormalizeFilePath( std::string *pszPath )
{
	for ( std::string::iterator i = pszPath->begin(); i != pszPath->end(); ++i )
	{
		if ( *i == '\\' )
		{
			*i = '/';
		}
	}
}


bool BuildSrcFilePath( std::string *pszFilePath, const std::string &szRefValue )
{
	if ( szRefValue.size() )
	{
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		*pszFilePath = pUserData->constUserData.szExportSourceFolder + szRefValue;
		NormalizeFilePath( pszFilePath );
	}
	return true;
}


bool BuildSrcFilePath( std::string *pszFilePath, IManipulator *pManipulator, const std::string &szRefFieldDBPath )
{
	if ( CManipulatorManager::GetValue( pszFilePath, pManipulator, szRefFieldDBPath ) )
	{
		if ( !pszFilePath->empty() )
		{
			SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
			pszFilePath->insert( 0, pUserData->constUserData.szExportSourceFolder );
			NormalizeFilePath( pszFilePath );
			return true;
		}
	}
	else
	{
		std::string szType;
		pManipulator->GetType( std::string(), &szType );
		NI_ASSERT( false, fmt::format("Field {} not found in object of type {}", szRefFieldDBPath.c_str(), szType.c_str()) );
	}

	return false;
}


std::string BuildDestFilePath( IManipulator* pManipulator, const std::string &szDestFolder )
{
	CVariant varUID;
	bool bResult = CManipulatorManager::GetValue( &varUID, pManipulator, "uid" );
	if ( !bResult )
	{
		return szDestFolder + std::to_string(  pManipulator->GetID( "" ) );
	}
	GUID uid;
	memcpy( &uid, varUID.GetPtr(), sizeof( uid ) );
	return szDestFolder + NBinResources::GUIDToString( uid );
}


void GetMayaInstallPath( std::string *szPath, const std::string &szMayaVersion )
{
	uint32_t type;
	const int nMaxSize = 512;
	int nSize = nMaxSize;
	uint8_t buffer[nMaxSize];
	int32_t error = ::RegQueryValueEx( HKEY_LOCAL_MACHINE,
			fmt::format("SOFTWARE\\Alias|Wavefront\\Maya\\{}\\Setup\\InstallPath\\MAYA_INSTALL_LOCATION", szMayaVersion.c_str()).c_str(),
			0, &type,
			buffer, (LPDWORD)&nSize
			);
	if ( error == ERROR_SUCCESS && type == REG_SZ )
	{
		*szPath = reinterpret_cast<char*>(buffer);
	}
}


bool StartupMayaProcess( CInteractiveMaya *pMayaProcess )
{
	if ( !pMayaProcess->IsStarted() )
	{
		const int nMayaResponseTimeout = GetOption( &SUserData::SMayaExportData::nMayaResponseTimeout );
		if ( nMayaResponseTimeout != 0 )
		{
			pMayaProcess->SetResponseTimeout( nMayaResponseTimeout );
		}

		if ( pMayaProcess->Start() )
		{
			const std::string &szMayaScriptPath = GetOption( &SUserData::SMayaExportData::szMayaScriptPath );
			const std::string &szStartupScript = GetOption( &SUserData::SMayaExportData::szStartupScript );
			int nMayaExecutionQuota = GetOption( &SUserData::SMayaExportData::nMayaExecutionQuota );

			std::string szScript;
			if ( !szMayaScriptPath.empty() )
			{
				std::string szLocalMayaScriptPath( szMayaScriptPath );
				NormalizeFilePath( &szLocalMayaScriptPath );
				szScript += fmt::format( "putenv \"MAYA_SCRIPT_PATH\" (\"{};\" + `getenv \"MAYA_SCRIPT_PATH\"`)",
						szLocalMayaScriptPath.c_str()
						);
				szScript += ";\n";
			}
			if ( !szStartupScript.empty() )
			{
				szScript += szStartupScript;
				szScript += ";\n";
			}

			if ( szScript.size() )
			{
				ILogger *pLogger = NLog::GetLogger();
				pLogger->Log( LT_NORMAL, "\nPerform startup configuration specified by UserData.xml...\n" );
				if ( !pMayaProcess->TransactCommand( szScript, "0" ) )
				{
					pLogger->Log( LT_ERROR, "Expected result is \"0\". Startup procedure failed.\n" );
					return false;
				}
			}

			// setting execution quota at this point (after startup executions)
			// allows us apply quota limits to user commands only
			if ( nMayaExecutionQuota > 0 )
			{
				pMayaProcess->SetExecutionQuota( nMayaExecutionQuota );
			}
		}
		else
		{
			return false;
		}
	}
	return true;
}


bool WaitForFile( const std::string &szFileName, const double fMaxWaitTime, bool bReportAsError )
{
	NHPTimer::STime timeCurr, timeOriginal;
	NHPTimer::GetTime( &timeOriginal );
	do 
	{
		timeCurr = timeOriginal;
		// check file existance
		if ( !NFile::DoesFileExist( szFileName ) )
		{
			std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
			continue;
		}
		// check file size
		{
			CFileStream stream( NVFS::GetMainVFS(), szFileName );
			if ( stream.IsOk() && stream.GetSize() != 0 )
				return true;
		}
		std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
	} while ( NHPTimer::GetTimePassed( &timeCurr ) < fMaxWaitTime * 0.001 );
	// error - stream doesn't exist
	ILogger *pLogger = NLog::GetLogger();
	const ELogOutputType eLogType = bReportAsError ? LT_ERROR : LT_IMPORTANT;
	pLogger->Log( eLogType, fmt::format("Can't access file - possibly it still not exist\n") );
	pLogger->Log( eLogType, fmt::format("\tFile name: {}\n", szFileName.c_str()) );
	return false;
}

bool ExecuteMayaScript( const std::string &szScript )
{
	CInteractiveMaya *pMayaProcess = CInteractiveMaya::Get();
	ILogger *pLogger = NLog::GetLogger();
	//
	if ( StartupMayaProcess( pMayaProcess ) )
	{
		if ( pMayaProcess->TransactCommand( szScript, "0" ) == false )
		{
			pLogger->Log( LT_ERROR, "Export from Maya failed\n" );
			return false;
		}
	}
	return true;
}


bool GetSelectedObjects( SObjectSet *pObjectSet, const std::string &szObjectTypeName )
{
	NI_ASSERT( pObjectSet != 0, "GetSelectedObjects(): SObjectSet == 0" );
	//
	bool bResult = Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<uint32_t>( pObjectSet ) );
	bResult = bResult && ( pObjectSet->szObjectTypeName == szObjectTypeName );
	bResult = bResult && ( !pObjectSet->objectNameSet.empty() );
	//
	if ( !bResult )
	{
		pObjectSet->szObjectTypeName.clear();
		pObjectSet->objectNameSet.clear();
	}
	//
	return bResult;
}

void MoveTempFileToDestination( const std::string &szTempFileFullName, const std::string &szDstFileFullName )
{
	const bool bAddToRCS = NFile::DoesFileExist( szDstFileFullName ) == false;
	NFile::CopyFile( szTempFileFullName, szDstFileFullName );
	DeleteFile( szTempFileFullName.c_str() );
}

// basement storage  


