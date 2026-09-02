#include "stdafx.h"
#include <fmt/format.h>

#include "FontExporter.h"

#include "MapEditorLib/ExporterFactory.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/Interface_MOD.h"
#include "Misc/StrProc.h"
#include "System/FilePath.h"
#include "System/FileUtils.h"
#include "MapEditorLib/CommonExporterMethods.h"

#include <cstdint>

REGISTER_EXPORTER_IN_DLL( Font, CFontExporter )


namespace
{
	const char FONTS_FOLDER[] = "bin/fonts/";
}


EXPORT_RESULT CFontExporter::ExportObject( IManipulator* pManipulator,
																					 const std::string &rszObjectTypeName,
																					 const std::string &rszObjectName,
																					 bool bForce,
																					 EXPORT_TYPE exportType )
{
	if ( ( exportType != ET_BEFORE_REF ) && ( exportType != ET_NO_REF ) )
		return ER_SUCCESS;
	//
	ILogger *pLogger = NLog::GetLogger();
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	int nWeight = 400, nHeight = 20;
	bool bItalic = false, bAntialiased = true;
	std::string szPitch, szCharset, szFaceName, szTextureName, szTextureFileName, szFontName;
	//
	bool bResult = true;
	bResult = bResult && CManipulatorManager::GetValue( &nWeight, pManipulator, "Thickness" );
	bResult = bResult && CManipulatorManager::GetValue( &nHeight, pManipulator, "Height" );
	bResult = bResult && CManipulatorManager::GetValue( &bItalic, pManipulator, "Italic" );
	bResult = bResult && CManipulatorManager::GetValue( &bAntialiased, pManipulator, "Antialiased" );
	bResult = bResult && CManipulatorManager::GetValue( &szPitch, pManipulator, "Pitch" );
	bResult = bResult && CManipulatorManager::GetValue( &szCharset, pManipulator, "Charset" );
	bResult = bResult && CManipulatorManager::GetValue( &szFaceName, pManipulator, "FaceName" );
	bResult = bResult && CManipulatorManager::GetValue( &szTextureName, pManipulator, "Texture" );
	bResult = bResult && CManipulatorManager::GetValue( &szFontName, pManipulator, "Name" );
	if ( bResult == false )
	{
		pLogger->Log( LT_ERROR, "Can't read parameters to generate font\n" );
		pLogger->Log( LT_ERROR, fmt::format("\tFont: {}\n", rszObjectName.c_str()) );
		return ER_FAIL;
	}
	//
	if ( szTextureName.empty() || szTextureName == " " )
	{
		szTextureName = "Fonts\\" + rszObjectName;
		// create new texture for this font
		if ( Singleton<IFolderCallback>()->InsertObject("Texture", szTextureName) == false )
		{
			pLogger->Log( LT_ERROR, "Can't create texture for font\n" );
			pLogger->Log( LT_ERROR, fmt::format("\tFont: {}\n", rszObjectName.c_str()) );
			pLogger->Log( LT_ERROR, fmt::format("\tTexture: {}\n", szTextureName.c_str()) );
			return ER_FAIL;
		}
		CPtr<IManipulator> pTexMan = Singleton<IResourceManager>()->CreateObjectManipulator( "Texture", szTextureName );
		if ( pTexMan == 0 )
		{
			pLogger->Log( LT_ERROR, "Can't create texture manipulator for font\n" );
			pLogger->Log( LT_ERROR, fmt::format("\tFont: {}\n", rszObjectName.c_str()) );
			pLogger->Log( LT_ERROR, fmt::format("\tTexture: {}\n", szTextureName.c_str()) );
			return ER_FAIL;
		}
		szTextureFileName = szTextureName + ".tga";
		CManipulatorManager::SetValue( szTextureFileName, pTexMan, "SrcName", false );
		CManipulatorManager::SetValue( "REGULAR", pTexMan, "Type", false );
		CManipulatorManager::SetValue( "CONVERT_ORDINARY", pTexMan, "ConversionType", false );
		CManipulatorManager::SetValue( "CLAMP", pTexMan, "AddrType", false );
		CManipulatorManager::SetValue( "TF_DXT3", pTexMan, "Format", false );
		CManipulatorManager::SetValue( 1, pTexMan, "NMips" );
		// set texture reference
		CManipulatorManager::SetValue( szTextureName, pManipulator, "Texture", true );
	}
	else
	{
		CPtr<IManipulator> pTexMan = Singleton<IResourceManager>()->CreateObjectManipulator( "Texture", szTextureName );
		CManipulatorManager::GetValue( &szTextureFileName, pTexMan, "SrcName" );
	}
	//
	if ( szTextureFileName.empty() || szTextureFileName == " " )
		return ER_FAIL;
	//
	const std::string szItalic = bItalic ? "-it" : "";
	const std::string szAA = bAntialiased ? "-aa" : "";

	NStr::ToLower( &szPitch );
	NStr::ToLower( &szCharset );

	std::string szBinFileName = BuildDestFilePath( pManipulator, Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + FONTS_FOLDER );
	std::string szPicFileName = pUserData->constUserData.szExportSourceFolder + szTextureFileName;
	std::string szCharsFileName = NFile::GetTempPath() + std::string( ".TEMP_FONT_FOLDER\\" ) + szFontName + std::string( ".txt" );
	//
	NStr::ReplaceAllChars( &szBinFileName, '/', '\\' );
	NStr::ReplaceAllChars( &szPicFileName, '/', '\\' );
	NStr::ReplaceAllChars( &szCharsFileName, '/', '\\' );
	//
	NFile::CreatePath( NFile::GetFilePath(szBinFileName) );
	NFile::CreatePath( NFile::GetFilePath(szPicFileName) );
	std::string szCommandLine;
	if ( NFile::DoesFileExist( szCharsFileName ) )
	{
		szCommandLine = fmt::format( "FontGen.exe -h{} -w{} {} {} -{} -{} \"{}\" \"{}\" \"{}\" \"{}\"", nHeight, nWeight, 
														szItalic.c_str(), szAA.c_str(), szPitch.c_str(), szCharset.c_str(), 
														szFaceName.c_str(), szBinFileName.c_str(), szPicFileName.c_str(), szCharsFileName.c_str() );
	}
	else
	{
		szCommandLine = fmt::format( "FontGen.exe -h{} -w{} {} {} -{} -{} \"{}\" \"{}\" \"{}\"", nHeight, nWeight, 
														szItalic.c_str(), szAA.c_str(), szPitch.c_str(), szCharset.c_str(), 
														szFaceName.c_str(), szBinFileName.c_str(), szPicFileName.c_str() );
	}
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
	}
	else
	{
		pLogger->Log( LT_ERROR, "Can't find font exporter: fontgen.exe\n" );
	}
	::CloseHandle( procinfo.hProcess );
	::CloseHandle( procinfo.hThread );

	return bResult ? ER_SUCCESS : ER_FAIL;
}


/*
const std::string szCommandLine = fmt::format( "{} -t{} -f{} -m{} -a{} -s{:f} {} \"{}\" \"{}\"", 
pUserData->szTEToolFileName.c_str(),
szType.c_str(),
szFormat.c_str(),
nMips,
szAddrType.c_str(),
fMSize,
bFlipY ? "-y" : "",
szSource.c_str(),
szDestination.c_str() );
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
}
::CloseHandle( procinfo.hProcess );
::CloseHandle( procinfo.hThread );

*/

