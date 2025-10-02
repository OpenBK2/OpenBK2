#include "StdAfx.h"

#include "BasicDataExtractor.h"
#include "../libdb/ResourceManager.h"
#include "Interface_UserData.h"
#include "Tools_Image.h"
#include "../Misc/2DArray.h"
#include "../Image/Image.h"
#include "../Image/ImageDDS.h"
#include "../Image/ImageScale.h"
#include "../System/FileUtils.h"
#include "../System/VFSOperations.h"
#include "Tools_Resources.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


bool CBasicDataExtractor::LoadImagesFromSource( CArray2D<DWORD> *pSmallImage,
																							  CArray2D<DWORD> *pNormalImage, 
																								const string &szFileName,
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
		CArray2D<DWORD> imageSource;
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
																							 const string &rszObjectTypeName,
																							 const string &rszObjectName )
{
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	const string szCacheFileName = pUserData->constUserData.szStartFolder + StrFmt( "Editor\\IconCache\\%s\\%s", rszObjectTypeName.c_str(), rszObjectName.c_str() );
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
				CArray2D<DWORD> imageSmall;
				CArray2D<DWORD> imageNormal;
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


void CBasicDataExtractor::SaveImagesToCache( CArray2D<DWORD> &rImageSmall,
																						 CArray2D<DWORD> &rImageNormal,
																						 const string &rszObjectTypeName,
																						 const string &rszObjectName )
{
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	const string szCacheFileName = pUserData->constUserData.szStartFolder + StrFmt( "Editor\\IconCache\\%s\\%s", rszObjectTypeName.c_str(), rszObjectName.c_str() );
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


bool CBasicDataExtractor::GetLabel( CString *pstrLabel, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator )
{
	int nPos = rszObjectName.rfind( PATH_SEPARATOR_CHAR );
	if ( nPos != string::npos )
	{
		( *pstrLabel ) = rszObjectName.substr( nPos + 1 ).c_str(); 
		return true;
	}
	return false;
}


UINT CBasicDataExtractor::GetObjectData( class CBitmap *pNormalBitmap,
																				 class CBitmap *pSmallBitmap,
																				 CString *pstrLabel,
																				 const string &rszObjectTypeName,
																				 const string &rszObjectName,
																				 const string &rszDataExtractorType )
{
	UINT nResult = 0;
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
		CArray2D<DWORD> smallImage, normalImage;
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

