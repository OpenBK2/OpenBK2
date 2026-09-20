#include "stdafx.h"
#include <fmt/format.h>

#include "BasicDataExtractor.h"
#include "libdb/ResourceManager.h"
#include "Interface_UserData.h"
#include "Tools_Image.h"
#include "Misc/2Darray.h"
#include "Image/Image.h"
#include "Image/ImageDDS.h"
#include "Image/ImageScale.h"
#include "System/FilePath.h"
#include "System/FileUtils.h"
#include "System/VFSOperations.h"
#include "Tools_Resources.h"

#include <cstdint>

bool CBasicDataExtractor::LoadImagesFromSource( CArray2D<uint32_t> *pSmallImage,
																							  CArray2D<uint32_t> *pNormalImage,
																								const std::string &szFileName,
																								ELoadImageMethod eMethod )
{
	if ( szFileName.empty() || ( szFileName == " " ) )
	{
		return false;
	}

	SFileStreamHolder streamHolder;
	OpenStreamHolder( &streamHolder, szFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		CArray2D<uint32_t> imageSource;
		NImage::LoadImageDDS( &imageSource, streamHolder.pStream );
		if ( !imageSource.IsEmpty() ) 
		{
			pSmallImage->SetSizes( SMALL_IMAGE_SIZE_X, SMALL_IMAGE_SIZE_Y );
			pNormalImage->SetSizes( NORMAL_IMAGE_SIZE_X, NORMAL_IMAGE_SIZE_Y );
			//
			switch ( eMethod )
			{
			case LOAD_IMAGE_COPY:
				NImage::Copy( pSmallImage, imageSource, CTPoint<int>( 0, 0 ) );
				NImage::Copy( pNormalImage, imageSource, CTPoint<int>( 0, 0 ) );
				break;
			case LOAD_IMAGE_SCALE:
				NImage::Scale( pSmallImage, imageSource, NImage::IMAGE_SCALE_METHOD_LANCZOS3 );
				NImage::Scale( pNormalImage, imageSource, NImage::IMAGE_SCALE_METHOD_LANCZOS3 );
				break;
			}
			return true;
		}
	}
	return false;
}


namespace
{
	// Where one object's two icons are kept between sessions.
	//
	// The object name is a database name, and those separate with a backslash:
	// "Units\Technics\GB\Aviation\lysander\MechUnitRPGStats.xdb". Here it is
	// being used as a path, one directory per part, which is what the backslashes
	// in the format string were for - they are directory separators on Windows
	// and ordinary characters everywhere else, so off Windows the whole cache
	// went into single files whose names contained backslashes, under whatever
	// the VFS took the absolute start folder to be relative to.
	//
	// So: the name folded to this tree's separator, and the path built with
	// JoinPath. The layout on disk is the one Windows has always written.
	std::string CacheFileName( const SUserData *pUserData, const std::string &rszObjectTypeName,
														 const std::string &rszObjectName )
	{
		std::string szObjectPath = rszObjectName;
		NFile::NormalizePath( &szObjectPath );
		return NFile::JoinPath( pUserData->constUserData.szStartFolder, "Editor", "IconCache",
														rszObjectTypeName, szObjectPath );
	}
}


bool CBasicDataExtractor::LoadImagesFromCache( CArray2D<uint32_t> *pNormalImage,
																	 CArray2D<uint32_t> *pSmallImage,
																							 const std::string &rszObjectTypeName,
																							 const std::string &rszObjectName )
{
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	const std::string szCacheFileName = CacheFileName( pUserData, rszObjectTypeName, rszObjectName );
	//
	if ( NFile::DoesFileExist( szCacheFileName ) )
	{
		SFileStreamHolder streamHolder;
		OpenStreamHolder( &streamHolder, szCacheFileName );
		if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
		{
			// just load icons from cache
			if ( CPtr<IBinSaver> pSaver = CreateBinSaver( streamHolder.pStream, SAVER_MODE_READ ) )
			{
				CArray2D<uint32_t> imageSmall;
				CArray2D<uint32_t> imageNormal;
				//
				pSaver->Add( 1, &imageSmall );
				pSaver->Add( 2, &imageNormal );
				//
				if ( ( imageSmall.GetSizeX() == SMALL_IMAGE_SIZE_X ) && ( imageSmall.GetSizeY() == SMALL_IMAGE_SIZE_Y ) &&
						 ( imageNormal.GetSizeX() == NORMAL_IMAGE_SIZE_X ) && ( imageNormal.GetSizeY() == NORMAL_IMAGE_SIZE_Y ) ) 
				{
					( *pNormalImage ) = imageNormal;
					( *pSmallImage ) = imageSmall;
					return true;
				}
			}
		}
	}
	return false;
}


void CBasicDataExtractor::SaveImagesToCache( CArray2D<uint32_t> &rImageSmall,
																						 CArray2D<uint32_t> &rImageNormal,
																						 const std::string &rszObjectTypeName,
																						 const std::string &rszObjectName )
{
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	const std::string szCacheFileName = CacheFileName( pUserData, rszObjectTypeName, rszObjectName );
	//
	SFileStreamHolder streamHolder;
	CreateStreamHolder( &streamHolder, szCacheFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IBinSaver> pSaver = CreateBinSaver( streamHolder.pStream, SAVER_MODE_WRITE ) )
		{
			pSaver->Add( 1, &rImageSmall );
			pSaver->Add( 2, &rImageNormal );
		}
	}
}


bool CBasicDataExtractor::GetLabel( std::string *pszLabel, const std::string &rszObjectTypeName, const std::string &rszObjectName, IManipulator *pObjectManipulator )
{
	int nPos = rszObjectName.rfind( PATH_SEPARATOR_CHAR );
	if ( nPos != std::string::npos )
	{
		( *pszLabel ) = rszObjectName.substr( nPos + 1 ).c_str(); 
		return true;
	}
	return false;
}


unsigned CBasicDataExtractor::GetObjectData( CArray2D<uint32_t> *pNormalImage,
																		 CArray2D<uint32_t> *pSmallImage,
																		 std::string *pszLabel,
																				 const std::string &rszObjectTypeName,
																				 const std::string &rszObjectName,
																				 const std::string &rszDataExtractorType )
{
	unsigned nResult = 0;
	CPtr<IManipulator> pObjectManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( rszObjectTypeName, rszObjectName );
	if ( pObjectManipulator == 0 )
	{
		return 0;
	}
	if ( GetLabel( pszLabel, rszObjectTypeName, rszObjectName, pObjectManipulator ) )
	{
		nResult |= OCDE_LABEL;
	}
	if ( LoadImagesFromCache( pNormalImage, pSmallImage, rszObjectTypeName, rszObjectName ) )
	{
		nResult |= OCDE_SMALL_BITMAP | OCDE_NORMAL_BITMAP;
	}
	else
	{
		CArray2D<uint32_t> smallImage, normalImage;
		if ( GetImages( &smallImage, &normalImage, rszObjectTypeName, rszObjectName, pObjectManipulator ) )
		{
			SaveImagesToCache( smallImage, normalImage, rszObjectTypeName, rszObjectName );
			( *pSmallImage ) = smallImage;
			( *pNormalImage ) = normalImage;
			nResult |= OCDE_SMALL_BITMAP | OCDE_NORMAL_BITMAP;
		}
	}
	return nResult;
}


