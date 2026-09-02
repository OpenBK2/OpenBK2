#include "stdafx.h"

#include "BasicDataExtractor.h"
#include "libdb/ResourceManager.h"
#include "Interface_UserData.h"
#include "Tools_Image.h"
#include "Misc/2Darray.h"
#include "Image/Image.h"
#include "Image/ImageDDS.h"
#include "Image/ImageScale.h"
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


bool CBasicDataExtractor::LoadImagesFromCache( class CBitmap *pNormalBitmap,
																							 class CBitmap *pSmallBitmap,
																							 const std::string &rszObjectTypeName,
																							 const std::string &rszObjectName )
{
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	const std::string szCacheFileName = pUserData->constUserData.szStartFolder + StrFmt( "Editor\\IconCache\\%s\\%s", rszObjectTypeName.c_str(), rszObjectName.c_str() );
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
					NImage::Load2Bitmap( pNormalBitmap, imageNormal );
					NImage::Load2Bitmap( pSmallBitmap, imageSmall );
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
	const std::string szCacheFileName = pUserData->constUserData.szStartFolder + StrFmt( "Editor\\IconCache\\%s\\%s", rszObjectTypeName.c_str(), rszObjectName.c_str() );
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


bool CBasicDataExtractor::GetLabel( CString *pstrLabel, const std::string &rszObjectTypeName, const std::string &rszObjectName, IManipulator *pObjectManipulator )
{
	int nPos = rszObjectName.rfind( PATH_SEPARATOR_CHAR );
	if ( nPos != std::string::npos )
	{
		( *pstrLabel ) = rszObjectName.substr( nPos + 1 ).c_str(); 
		return true;
	}
	return false;
}


unsigned CBasicDataExtractor::GetObjectData( class CBitmap *pNormalBitmap,
																				 class CBitmap *pSmallBitmap,
																				 CString *pstrLabel,
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
	if ( GetLabel( pstrLabel, rszObjectTypeName, rszObjectName, pObjectManipulator ) )
	{
		nResult |= OCDE_LABEL;
	}
	if ( LoadImagesFromCache( pNormalBitmap, pSmallBitmap, rszObjectTypeName, rszObjectName ) )
	{
		nResult |= OCDE_SMALL_BITMAP | OCDE_NORMAL_BITMAP;
	}
	else
	{
		CArray2D<uint32_t> smallImage, normalImage;
		if ( GetImages( &smallImage, &normalImage, rszObjectTypeName, rszObjectName, pObjectManipulator ) )
		{
			SaveImagesToCache( smallImage, normalImage, rszObjectTypeName, rszObjectName );
			NImage::Load2Bitmap( pSmallBitmap, smallImage );
			NImage::Load2Bitmap( pNormalBitmap, normalImage );
			nResult |= OCDE_SMALL_BITMAP | OCDE_NORMAL_BITMAP;
		}
	}
	return nResult;
}


