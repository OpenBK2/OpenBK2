#include "stdafx.h"
#include <fmt/format.h>

#include "Misc/2Darray.h"
#include "TextureExporter.h"
#include "MapEditorLib/ExporterFactory.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/Interface_MOD.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/CommonExporterMethods.h"
#include "System/FileUtils.h"
#include "Misc/Win32Helper.h"
#include "Misc/StrProc.h"
#include "Image/DDS.h"
#include "Image/Targa.h"
#include "Image/Image.h"
#include "Image/ImageDDS.h"
#include "Image/ImageTGA.h"
#include "Image/ImagePSD.h"
#include "3Dmotor/GPixelFormat.h"
#include "3Dmotor/GView.h"
#include "System/FilePath.h"
#include "System/FileUtils.h"
#include "System/VFSOperations.h"

#include <cstdint>
#include <filesystem>

#include "3Dmotor_export.h"

namespace NGfx
{
  _3DMOTOR_EXPORT EXTERNVAR NWin32Helper::com_ptr<IDirect3DDevice9> pDevice;
}

namespace
{
std::string TextureSourcePath( IManipulator *pManipulator, const std::string &source )
{
	if ( source.empty() )
		return source;
	// SrcName may be absolute, relative to the configured source directory,
	// or beside its XDB in the mounted game/mod data.
	const auto path = std::filesystem::u8path(source);
	if ( path.is_absolute() )
		return source;
	const std::string local = NFile::JoinPath(NFile::GetFilePath(NDb::GetFileName(pManipulator->GetDBID())), source);
	for ( const std::string &candidate : {
		local, source, NFile::JoinPath(Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder, source) } )
	{
		CFileStream stream( NVFS::GetMainVFS(), candidate );
		if ( stream.IsOk() && stream.CanRead() )
			return candidate;
	}
	return source;
}
}

REGISTER_EXPORTER_IN_DLL( Texture, CTextureExporter )


EXPORT_RESULT CTextureExporter::ExportObject( IManipulator* pManipulator,
																							const std::string &rszObjectTypeName,
																							const std::string &rszObjectName,
																							bool bForce,
																							EXPORT_TYPE exportType )
{
	NI_ASSERT( pManipulator != 0, "CTextureExporter::ExportObject() pManipulator == 0 )" );
	if ( ( exportType != ET_BEFORE_REF ) && ( exportType != ET_NO_REF ) )
	{
		return ER_SUCCESS;
	}
	//
	// Manually supplied DDS resources do not need a source image.
	bool bStandardExport = true;
	CManipulatorManager::GetValue( &bStandardExport, pManipulator, "StandardExport" );
	if ( !bStandardExport )
		return ER_SUCCESS;
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	ILogger *pLogger = NLog::GetLogger();
	//
	//
	std::string szSourceValue;
	CManipulatorManager::GetValue( &szSourceValue, pManipulator, "SrcName" );
	const std::string szSource = TextureSourcePath( pManipulator, szSourceValue );
	//
	if ( szSourceValue.empty() || !NVFS::GetMainVFS()->DoesFileExist(szSource) )
	{
		NLog::Log( LT_ERROR, "Source texture empty or source file doesn't exist!\n" );
		NLog::Log( LT_ERROR, "\tObject name: %s\n", rszObjectName.c_str() );
		return ER_FAIL;
	}
	//
	std::string szRealDestination = NFile::CutFileExt( rszObjectName, "xdb" ) + ".dds";
	NFile::NormalizePath( &szRealDestination );
	const std::string dbDestination = szRealDestination;
	szRealDestination = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + szRealDestination;
	NFile::NormalizePath( &szRealDestination );
	std::string szDestination; // Allocate the temporary file only after input validation.
	// check for source and destination times if not forced mode
	if ( CheckFilesUpdated(szSource, szRealDestination, bForce) )
	{
		CManipulatorManager::SetValue( dbDestination, pManipulator, "DestName" );
		return ER_SUCCESS;
	}
	//
	std::string szType;
	std::string szAddrType;
	std::string szFormat;
	std::string szUsageType;
	int nMips = 0;
	float fGain = 0.0f;
	float fMSize = 0.0f;
	bool bFlipY = false;
	CManipulatorManager::GetValue( &szUsageType, pManipulator, "Type" );
	CManipulatorManager::GetValue( &szType, pManipulator, "ConversionType" );
	CManipulatorManager::GetValue( &szAddrType, pManipulator, "AddrType" );
	CManipulatorManager::GetValue( &szFormat, pManipulator, "Format" );
	CManipulatorManager::GetValue( &nMips, pManipulator, "NMips" );
	CManipulatorManager::GetValue( &fGain, pManipulator, "BumpGain" );
	CManipulatorManager::GetValue( &fMSize, pManipulator, "MappingSize" );
	CManipulatorManager::GetValue( &bFlipY, pManipulator, "FlipY" );
	fMSize = fMSize / ( fGain > FP_EPSILON ? fGain : FP_EPSILON );
	//
	bool bResult = false;
	{
		NImage::EImageType eImageType = NImage::IMAGE_TYPE_PICTURE;
		bool bWrapX = true, bWrapY = true;
		NGfx::EPixelFormat ePixelFormat = NGfx::CF_A8R8G8B8;
		// process params
		if ( szType == "CONVERT_ORDINARY" )
			eImageType = NImage::IMAGE_TYPE_PICTURE;
		else if ( szType == "CONVERT_ORDINARY_FASTMIP" )
			eImageType = NImage::IMAGE_TYPE_PICTURE_FASTMIP;
		else if ( szType == "CONVERT_LINEAR_PICTURE" )
			eImageType = NImage::IMAGE_TYPE_PICTURE;
		else if ( szType == "CONVERT_TRANSPARENT" )
			eImageType = NImage::IMAGE_TYPE_TRANSPARENT;
		else if ( szType == "CONVERT_TRANSPARENT_ADD" )
			eImageType = NImage::IMAGE_TYPE_TRANSPARENT_ADD;
		else if ( szType == "CONVERT_BUMP" )
			eImageType = NImage::IMAGE_TYPE_BUMP;
		//
		if ( szAddrType == "CLAMP" )
		{
			bWrapX = false;
			bWrapY = false;
		}
		else if ( szAddrType == "WRAP_Y" )
		{
			bWrapX = false;
			bWrapY = true;
		}
		else if ( szAddrType == "WRAP_X" )
		{
			bWrapX = true;
			bWrapY = false;
		}
		else
		{
			bWrapX = true;
			bWrapY = true;
		}
		//
		if ( szFormat == "TF_565" )
			ePixelFormat = NGfx::CF_R5G6B5;
		if ( szFormat == "TF_1555" )
			ePixelFormat = NGfx::CF_A1R5G5B5;
		if ( szFormat == "TF_4444" )
			ePixelFormat = NGfx::CF_A4R4G4B4;
		if ( szFormat == "TF_8888" )
			ePixelFormat = NGfx::CF_A8R8G8B8;
		else if ( szFormat == "TF_DXT1" ) 
			ePixelFormat = NGfx::CF_DXT1;
		else if ( szFormat == "TF_DXT2" ) 
			ePixelFormat = NGfx::CF_DXT2;
		else if ( szFormat == "TF_DXT3" ) 
			ePixelFormat = NGfx::CF_DXT3;
		else if ( szFormat == "TF_DXT4" ) 
			ePixelFormat = NGfx::CF_DXT4;
		else if ( szFormat == "TF_DXT5" ) 
			ePixelFormat = NGfx::CF_DXT5;

		{
			CArray2D<uint32_t> image;
			CFileStream stream( NVFS::GetMainVFS(), szSource );
			if ( stream.IsOk() ) 
			{
				// load TGA or PSD file
				if ( NImage::RecognizeFormatTGA( &stream ) == true )
				{
					if ( !NImage::LoadTGAImage(image, &stream) )
					{
						NLog::Log(LT_ERROR, "Cannot decode TGA texture: %s\n", szSource.c_str());
						return ER_FAIL;
					}
				}
				else
				{
					int nRes = NImage::RecognizeFormatPSD( &stream );
					if (  nRes == 1 )
						NImage::LoadImagePSD( image, &stream );
					else
					{
						if (nRes == 2)
						{
							NLog::Log( LT_ERROR, "Source PSD texture must be in a valid PSD-format: RGB, 8-bit per channel!\n" );
							NLog::Log( LT_ERROR, "\tObject name: %s\n", rszObjectName.c_str() );
							NLog::Log( LT_ERROR, "\tFile name: %s\n", szSource.c_str() );
							return ER_FAIL;
						}
						else
						{
							NLog::Log( LT_ERROR, "Source texture must be TGA or PSD file!\n" );
							NLog::Log( LT_ERROR, "\tObject name: %s\n", rszObjectName.c_str() );
							NLog::Log( LT_ERROR, "\tFile name: %s\n", szSource.c_str() );
							return ER_FAIL;
						}
					}
				}
				//
				if ( image.GetSizeX() <= 0 || image.GetSizeY() <= 0 || !stream.IsOk() )
				{
					NLog::Log(LT_ERROR, "Empty or invalid source image: %s\n", szSource.c_str());
					return ER_FAIL;
				}
				if ( szUsageType == "TEXTURE_2D" )
				{
					// 2D interface texture must have number of mip levels = 1
					if ( nMips != 1 )
					{
						NLog::Log( LT_IMPORTANT, "'TEXTURE_2D' must have only one mip level. Setting it.\n" );
						NLog::Log( LT_IMPORTANT, "\tObject name: %s\n", rszObjectName.c_str() );
						NLog::Log( LT_IMPORTANT, "\tSource image: %s\n", szSource.c_str() );
						NLog::Log( LT_IMPORTANT, "\tNum mip levels: %d\n", nMips );
						nMips = 1;
						CManipulatorManager::SetValue( nMips, pManipulator, "NMips" );
					}
				}
				else
				{
					// non-2d texture must be power of 2
					if ( image.GetSizeX() != GetNextPow2(image.GetSizeX()) ||image.GetSizeY() != GetNextPow2(image.GetSizeY()) )
					{
						const std::string szError = fmt::format( "Source image \"{}\" must be power two size ({} : {})\n", 
							                             szSource.c_str(), image.GetSizeX(), image.GetSizeY() );
						NLog::Log( LT_ERROR, "Source image size must be power of 2\n" );
						NLog::Log( LT_ERROR, "\tObject name: %s\n", rszObjectName.c_str() );
						NLog::Log( LT_ERROR, "\tSource image: %s\n", szSource.c_str() );
						if ( image.GetSizeX() != GetNextPow2(image.GetSizeX()) )
							NLog::Log( LT_ERROR, "\tWidth: %d\n", image.GetSizeX() );
						if ( image.GetSizeY() != GetNextPow2(image.GetSizeY()) )
							NLog::Log( LT_ERROR, "\tHeight: %d\n", image.GetSizeY() );
						return ER_FAIL;
					}
				}

				if ( bFlipY ) 
					NImage::FlipY( image );
				szDestination = NFile::GetTempFileName();
				bResult = NImage::ConvertAndSaveAsDDSWithDX( NGfx::pDevice, szDestination, image, eImageType, ePixelFormat, nMips, bWrapX, bWrapY, fMSize );
			}
			else
			{
				NLog::Log( LT_ERROR, "Can't open source image: %s\n", szSource.c_str() );
				NLog::Log( LT_ERROR, "\tObject name: %s\n", rszObjectName.c_str() );
				return ER_FAIL;
			}
		}
	}
	//
	if ( bResult && CheckDestination(szDestination, rszObjectName) != false )
	{
		// copy file from temporary location to real destination
		// Assign DestName only once a complete DDS is installed successfully.
		NFile::CreatePath( NFile::GetFilePath(szRealDestination) );
		if ( !::MoveFileExA(szDestination.c_str(), szRealDestination.c_str(),
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) )
		{
			NLog::Log( LT_ERROR, "Cannot write DDS: %s (Windows error %lu)\n", szRealDestination.c_str(), GetLastError() );
			::DeleteFileA( szDestination.c_str() );
			return ER_FAIL;
		}
		CManipulatorManager::SetValue( dbDestination, pManipulator, "DestName" );
		szDestination = szRealDestination;
		// 
		CTPoint<int> imageSize;
		if ( GetDDSImageSize( szDestination, &imageSize ) )
		{
			pManipulator->SetValue( "Width", imageSize.x );
			pManipulator->SetValue( "Height", imageSize.y );
			// set IsDXT
			if ( (szFormat == "TF_DXT1") || (szFormat == "TF_DXT2") || (szFormat == "TF_DXT3") || (szFormat == "TF_DXT4") || (szFormat == "TF_DXT5") ) 
				pManipulator->SetValue( "IsDXT", true );
			else
				pManipulator->SetValue( "IsDXT", false );
		}

		// reload all textures to apply changes
		NGScene::ReloadTexture( 0 );
		//
		NLog::Log( LT_NORMAL, "Texture exported successfully\n" );
		NLog::Log( LT_NORMAL, "\tObject name: %s\n", rszObjectName.c_str() );
		NLog::Log( LT_NORMAL, "\tSource file name: %s\n", szSourceValue.c_str() );

		return ER_SUCCESS;
	}
	else
		DeleteFile( szDestination.c_str() );
	//
	// 
	NLog::Log( LT_ERROR, "Texture export failed (most probable reason - source was not loaded correctly)\n" );
	NLog::Log( LT_ERROR, "\tObject name: %s\n", rszObjectName.c_str() );
	return ER_FAIL;
}


int GetDDSBPP( const SDDSHeader &hdr )
{
	if ( hdr.ddspf.dwFlags & DDS_FOURCC ) 
		return hdr.ddspf.dwFourCC == MAKEFOURCC('D','X','T','1') ? 4 : 8;
	else if ( (hdr.ddspf.dwFlags & DDS_RGB) == DDS_RGB ) 
		return hdr.ddspf.dwRGBBitCount;
	else
		return 8;
}


EXPORT_RESULT CTextureExporter::CheckObject( IManipulator* pManipulator,
																						 const std::string &rszObjectTypeName, 
																						 const std::string &rszObjectName,
																						 bool bExport,
																						 EXPORT_TYPE exportType )
{
  if ( bExport ) // skip checking after export
	{
		return ER_SUCCESS;
	}
	if ( ( exportType != ET_BEFORE_REF ) && ( exportType != ET_NO_REF ) )
	{
		return ER_SUCCESS;
	}
	//
	ILogger *pLogger = NLog::GetLogger();
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	
	std::string szSorceValue;
	CManipulatorManager::GetValue( &szSorceValue, pManipulator, "SrcName" );
	bool bStandardExport = true;
	CManipulatorManager::GetValue( &bStandardExport, pManipulator, "StandardExport" );
	const std::string szSrcFileName = TextureSourcePath( pManipulator, szSorceValue );
	// check texture file name
	if ( bStandardExport && !NFile::IsValidFileName( szSrcFileName.c_str() ) ) 
	{
		const std::string szError = fmt::format( "Texture \"{}\" has invalid source file name \"{}\"\n", rszObjectName.c_str(), szSrcFileName.c_str() );
		pLogger->Log( LT_ERROR, szError );
		return ER_FAIL;
	}
	// check source file (!)
	if ( bStandardExport && !NVFS::GetMainVFS()->DoesFileExist( szSrcFileName ) )
	{
		const std::string szError = fmt::format( "Texture \"{}\" source file \"{}\" doesn't exist!\n", rszObjectName.c_str(), szSrcFileName.c_str() );
		pLogger->Log( LT_ERROR, szError );
		return ER_FAIL;
	}
	// check destination
	std::string szDestValue;
	CManipulatorManager::GetValue( &szDestValue, pManipulator, "DestName" );
	const std::string szDstFileName = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + szDestValue;
	if ( CheckDestination( szDstFileName, rszObjectName ) == false )
		return ER_FAIL;
	//
	return ER_SUCCESS;
}

bool CTextureExporter::CheckDestination( const std::string &szFileName, const std::string &szObjectName ) const
{
	// check destination
	std::string szDestValue;
	CFileStream stream( szFileName, CFileStream::WIN_READ_ONLY );
	if ( !stream.IsOk() ) 
	{
		NLog::Log( LT_ERROR, "Texture doesn't exist (not exported yet?)\n" );
		NLog::Log( LT_ERROR, "\tTexture name:  \"%s\"\n", szObjectName.c_str() );
		return false;
	}
	//
	try
	{
		SDDSFileHeader fhdr;
		stream.Read( &fhdr, sizeof(fhdr) );
		if ( fhdr.dwSignature != SDDSFileHeader::SIGNATURE ) 
		{
			NLog::Log( LT_ERROR, "Invalid header in texture\n" );
			NLog::Log( LT_ERROR, "\tTexture name:  \"%s\"\n", szObjectName.c_str() );
			return false;
		}
		// check real size, width, height and correspondence with requested mip levels
		const int nTextureDataSize = stream.GetSize() - sizeof( fhdr );
		int nNumMipLevels = fhdr.header.dwMipMapCount;

		//BUGFIX{ : The DX DDS-saver sets nMips=0, but non-DX one sets nMips=1 
		if ( 0 == nNumMipLevels )
			++nNumMipLevels;
		//BUGFIX}

		if ( fhdr.header.dwSize != sizeof(SDDSHeader) || fhdr.header.ddspf.dwSize != sizeof(SDDSPixelFormat) ||
			fhdr.header.dwWidth == 0 || fhdr.header.dwHeight == 0 || nNumMipLevels > 32 )
			return false;
		// DXT stores a whole 4x4 block even for the small mip levels; rectangular
		// images also stop halving each dimension once it reaches one pixel.
		uint64_t width = fhdr.header.dwWidth, height = fhdr.header.dwHeight;
		uint64_t nTotalSize = 0;
		for ( int i = 0; i < nNumMipLevels; ++i )
		{
			if ( fhdr.header.ddspf.dwFlags & DDS_FOURCC )
				nTotalSize += ((width + 3) / 4) * ((height + 3) / 4) *
					(fhdr.header.ddspf.dwFourCC == MAKEFOURCC('D','X','T','1') ? 8 : 16);
			else
				nTotalSize += width * height * GetDDSBPP(fhdr.header) / 8;
			width = (std::max)(uint64_t(1), width / 2);
			height = (std::max)(uint64_t(1), height / 2);
		}
		if ( nTotalSize != nTextureDataSize ) 
		{
			NLog::Log( LT_ERROR, "Invalid result texture (size check)\n" );
			NLog::Log( LT_ERROR, "\tTexture name: \"%s\"\n", szObjectName.c_str() );
			return false;
		}
	}
	catch ( ... ) 
	{
		NLog::Log( LT_ERROR, "General error during texture check\n" );
		NLog::Log( LT_ERROR, "\tTexture name: \"%s\"\n", szObjectName.c_str() );
		return false;
	}
	//
	return true;
}


