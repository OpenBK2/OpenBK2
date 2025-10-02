#include "StdAfx.h"

#include "3Dmotor_export.h"

#include "GfxBuffers.h"
#include "GTexture.h"
#include "..\System\Commands.h"
#include "DBScene.h"
#include "..\Image\DDS.h"

NGfx::EPixelFormat GetPixelFormat( const SDDSHeader &hdr )
{
	if ( hdr.ddspf.dwFlags & DDS_FOURCC ) 
	{
		switch ( hdr.ddspf.dwFourCC ) 
		{
			case MAKEFOURCC('D','X','T','1'):
				return NGfx::CF_DXT1;
			case MAKEFOURCC('D','X','T','2'):
				return NGfx::CF_DXT2;
			case MAKEFOURCC('D','X','T','3'):
				return NGfx::CF_DXT3;
			case MAKEFOURCC('D','X','T','4'):
				return NGfx::CF_DXT4;
			case MAKEFOURCC('D','X','T','5'):
				return NGfx::CF_DXT5;
		}
		ASSERT( false && "Unknown DXT format" );
	}
	else if ( (hdr.ddspf.dwFlags & DDS_RGB) == DDS_RGB ) 
	{
		if ( (hdr.ddspf.dwRGBBitCount == 32) && (hdr.ddspf.dwGBitMask == 0x0000ff00) ) 
			return NGfx::CF_A8R8G8B8;
		else if ( hdr.ddspf.dwRGBBitCount == 16 )
		{
			switch ( hdr.ddspf.dwGBitMask ) 
			{
				case 0x000003e0:
					return NGfx::CF_A1R5G5B5;
				case 0x000000f0:
					return NGfx::CF_A4R4G4B4;
				case 0x000007e0:
					return NGfx::CF_R5G6B5;
			}
		}
		ASSERT( false && "Unsupported ARGB format" );
	}
	return NGfx::CF_A8R8G8B8;
}

bool bDXTModeOn = true;
static int GetRealTextureID( const NDb::STexture *pTex )
{
	int nID = pTex->GetRecordID();
	if ( bDXTModeOn )
		return nID;
	if ( pTex->bIsDXT )
		return nID | 0x01000000;
	return nID;
}

namespace NGScene
{
_3DMOTOR_EXPORT int nTextureUseMip = 0;
bool bLowRAM = false;

STextureKey GetKey( const NDb::STexture *pTex )
{
	int nFlags = 0;
	switch ( pTex->eAddrType )
	{
	case NDb::STexture::WRAP: nFlags = STextureKey::TK_WRAP_X | STextureKey::TK_WRAP_Y; break;
	case NDb::STexture::WRAP_X: nFlags = STextureKey::TK_WRAP_X; break;
	case NDb::STexture::WRAP_Y: nFlags = STextureKey::TK_WRAP_Y; break;
	case NDb::STexture::CLAMP: nFlags = 0; break;
	default: ASSERT(0); break;
	}
	return STextureKey( pTex, nFlags );
}

// CTextureLoader

template <class TPixel, class TStream>
void LoadTextureData( NGfx::CTexture *pTexture, int _nX, int _nY, int _nSizeX, int _nSizeY, 
	int _nLevels, TStream *pFile, int nCutMip, const TPixel *p = 0 )
{
	CDynamicCast<NGfx::I2DBuffer> pTexBuffer( pTexture );
	int nLevels = Min( _nLevels, nCutMip + pTexBuffer->GetNumMipLevels() );
	for ( int nLevel = 0; nLevel < nLevels; ++nLevel )
	{
		int nX = (_nX >> nLevel) / TPixel::XSize;
		int nY = (_nY >> nLevel) / TPixel::YSize;
		int nSizeX = (_nSizeX >> nLevel) / TPixel::XSize;
		int nSizeY = (_nSizeY >> nLevel) / TPixel::YSize;
		if ( nLevel - nCutMip < 0 )
		{
			pFile->Seek( pFile->GetPosition() + nSizeY * nSizeX * sizeof(TPixel) );
			continue;
		}
		NGfx::CTextureLock<TPixel> lock( pTexture, nLevel - nCutMip, NGfx::INPLACE );
		ASSERT( lock.GetSizeX() >= nX + nSizeX );
		ASSERT( lock.GetSizeY() >= nY + nSizeY );
		for ( int y = nY; y < nY + nSizeY; ++y )
			pFile->Read( &(lock[y][nX]), nSizeX * sizeof(TPixel) );
	}
}

static NGfx::EFace loadFace;
template <class TPixel, class TStream>
void LoadTextureData( NGfx::CCubeTexture *pTexture, int _nX, int _nY, int _nSizeX, int _nSizeY, 
	int _nLevels, TStream *pFile, int nCutMip, const TPixel *p = 0 )
{
	CDynamicCast<NGfx::ICubeBuffer> pTexBuffer( pTexture );
	int nLevels = Min( _nLevels, nCutMip + pTexBuffer->GetNumMipLevels() );
	for ( int nLevel = 0; nLevel < nLevels; ++nLevel )
	{
		int nX = (_nX >> nLevel) / TPixel::XSize;
		int nY = (_nY >> nLevel) / TPixel::YSize;
		int nSizeX = (_nSizeX >> nLevel) / TPixel::XSize;
		int nSizeY = (_nSizeY >> nLevel) / TPixel::YSize;
		if ( nLevel - nCutMip < 0 )
		{
			pFile->Seek( pFile->GetPosition() + nSizeY * nSizeX * sizeof(TPixel) );
			continue;
		}
		NGfx::CTextureLock<TPixel> lock( pTexture, loadFace, nLevel - nCutMip, NGfx::INPLACE );
		ASSERT( lock.GetSizeX() >= nX + nSizeX );
		ASSERT( lock.GetSizeY() >= nY + nSizeY );
		for ( int y = nY; y < nY + nSizeY; ++y )
			pFile->Read( &(lock[y][nX]), nSizeX * sizeof(TPixel) );
	}
}

template<class TRes, class TStream>
static bool RealLoadTexture( TRes *pTexture, TStream *pStream, const SDDSHeader &hdr,
	int nX, int nY, int nSizeX, int nSizeY, int nLevels, int nCutMip )
{
	nLevels = Min( nLevels, int(hdr.dwMipMapCount) );
	switch ( GetPixelFormat(hdr) )
	{
		case NGfx::CF_DXT1:			LoadTextureData<NGfx::SPixelDXT1>( pTexture, nX, nY, nSizeX, nSizeY, nLevels, pStream, nCutMip ); break;
		case NGfx::CF_DXT2:			LoadTextureData<NGfx::SPixelDXT2>( pTexture, nX, nY, nSizeX, nSizeY, nLevels, pStream, nCutMip ); break;
		case NGfx::CF_DXT3:			LoadTextureData<NGfx::SPixelDXT3>( pTexture, nX, nY, nSizeX, nSizeY, nLevels, pStream, nCutMip ); break;
		case NGfx::CF_DXT4:			LoadTextureData<NGfx::SPixelDXT4>( pTexture, nX, nY, nSizeX, nSizeY, nLevels, pStream, nCutMip ); break;
		case NGfx::CF_DXT5:			LoadTextureData<NGfx::SPixelDXT5>( pTexture, nX, nY, nSizeX, nSizeY, nLevels, pStream, nCutMip ); break;
		case NGfx::CF_R5G6B5:		LoadTextureData<NGfx::SPixel565> ( pTexture, nX, nY, nSizeX, nSizeY, nLevels, pStream, nCutMip ); break;
		case NGfx::CF_A1R5G5B5: LoadTextureData<NGfx::SPixel1555>( pTexture, nX, nY, nSizeX, nSizeY, nLevels, pStream, nCutMip ); break;
		case NGfx::CF_A4R4G4B4: LoadTextureData<NGfx::SPixel4444>( pTexture, nX, nY, nSizeX, nSizeY, nLevels, pStream, nCutMip ); break;
		case NGfx::CF_A8R8G8B8: LoadTextureData<NGfx::SPixel8888>( pTexture, nX, nY, nSizeX, nSizeY, nLevels, pStream, nCutMip ); break;
		default: return false;
	}
	return true;
}

static NGfx::EPixelFormat SelectFormat( const vector<NGfx::SPixel8888> &data )
{
	vector<int> counts( 256, 0 );
	for ( int k = 0; k < data.size(); ++k )
		++counts[ data[k].a ];
	if ( counts[255] == data.size() )
		return NGfx::CF_R5G6B5;
	if ( ( counts[255] + counts[0] ) == data.size() )
		return NGfx::CF_A1R5G5B5;
	return NGfx::CF_A4R4G4B4;
}

static NGfx::SPixel565 DoConvert( const NGfx::SPixel8888 &src, NGfx::SPixel565 *p = 0 ) { return NGfx::SPixel565( src.r >> 3, src.g >> 2, src.b >> 3 ); }
static NGfx::SPixel1555 DoConvert( const NGfx::SPixel8888 &src, NGfx::SPixel1555 *p = 0 ) { return NGfx::SPixel1555( src.r >> 3, src.g >> 3, src.b >> 3, src.a >> 7 ); }
static NGfx::SPixel4444 DoConvert( const NGfx::SPixel8888 &src, NGfx::SPixel4444 *p = 0 ) { return NGfx::SPixel4444( src.r >> 4, src.g >> 4, src.b >> 4, src.a >> 4 ); }

template<class T>
static void ConvertTo16Bit( const vector<NGfx::SPixel8888> &data, NGfx::CTexture *pTexture, int nLevel, int nSizeX, int nSizeY, NGfx::EPixelFormat format )
{
	NGfx::CTextureLock<T> lock( pTexture, nLevel, NGfx::INPLACE );
	ASSERT( lock.GetSizeX() >= nSizeX );
	ASSERT( lock.GetSizeY() >= nSizeY );
	for ( int y = 0; y < nSizeY; ++y )
	{
		int nBase = y * nSizeX;
		for ( int x = 0; x < nSizeX; ++x )
			lock[y][x] = DoConvert( data[nBase + x], (T*)0 );
	}
}

static NGfx::CTexture* LoadConvertTo16Bit( CDataStream *pStream, const SDDSHeader &hdr, NGfx::ETextureUsage eUsage, NGfx::EWrap eWrap, int nCutMips,
	int nSizeX, int nSizeY, int nLevels )
{
	for ( int k = 0; k < nCutMips; ++k )
	{
		if ( nLevels == 1  )
			break;
		pStream->Seek( pStream->GetPosition() + nSizeX * nSizeY * 4 );
		nSizeX >>= 1;
		nSizeY >>= 1;
		--nLevels;
	}
	NGfx::EPixelFormat format = NGfx::CF_A8R8G8B8; // keep compiler happy
	NGfx::I2DBuffer *pTexBuffer = 0;
	CObj<NGfx::CTexture> pTexture;
	for ( int k = 0; k < nLevels; ++k )
	{
		if ( pTexBuffer && k >= pTexBuffer->GetNumMipLevels() )
			break;
		vector<NGfx::SPixel8888> data;
		data.resize( nSizeX * nSizeY );
		pStream->Read( &data[0], data.size() * sizeof(data[0]) );
		if ( k == 0 )
		{
			if ( eUsage == NGfx::TEXTURE_2D || eUsage == NGfx::TRANSPARENT_TEXTURE )
				format = NGfx::CF_A4R4G4B4;
			else
				format = SelectFormat( data );
			pTexture = NGfx::MakeTexture( nSizeX, nSizeY, nLevels, format, eUsage, eWrap );
			pTexBuffer = dynamic_cast<NGfx::I2DBuffer*>( pTexture.GetBarePtr() );
		}
		switch ( format )
		{
		case NGfx::CF_R5G6B5: ConvertTo16Bit<NGfx::SPixel565>( data, pTexture, k, nSizeX, nSizeY, format ); break;
		case NGfx::CF_A1R5G5B5: ConvertTo16Bit<NGfx::SPixel1555>( data, pTexture, k, nSizeX, nSizeY, format ); break;
		case NGfx::CF_A4R4G4B4: ConvertTo16Bit<NGfx::SPixel4444>( data, pTexture, k, nSizeX, nSizeY, format ); break;
		default: ASSERT(0); break;
		}
		nSizeX >>= 1;
		nSizeY >>= 1;
	}
	return pTexture.Extract();
}

// CFileTexture

void CFileTexture::CreateChecker()
{
	NGfx::SPixel8888 colors[2];
	colors[0] = NGfx::SPixel8888(0,0,0,255);
	colors[1] = NGfx::SPixel8888(255,255,255,255);
	const int nSize = 128;
	pValue = NGfx::MakeTexture( nSize, nSize, 1, NGfx::SPixel8888::ID, NGfx::REGULAR, NGfx::CLAMP );
	NGfx::CTextureLock<NGfx::SPixel8888> lock( pValue, 0, NGfx::INPLACE );
	for ( int y = 0; y < nSize; ++y )
	{
		for ( int x = 0; x < nSize; ++x )
			lock[y][x] = colors[ ( (x&4) == 0 ) & ( (y&4) == 0 ) ];
	}
}

static NGfx::CTexture* MakeTexture( const SDDSHeader &hdr, NGfx::ETextureUsage eUsage, NGfx::EWrap eWrap, int nCutMips )
{
	NGfx::EPixelFormat ePixelFormat = GetPixelFormat( hdr );
	if ( eUsage == NDb::STexture::TEXTURE_2D && ePixelFormat != NGfx::CF_A8R8G8B8 )
		eUsage = NGfx::REGULAR;
	return NGfx::MakeTexture( hdr.dwWidth >> nCutMips, hdr.dwHeight >> nCutMips, hdr.dwMipMapCount - nCutMips, 
		ePixelFormat, eUsage, eWrap );
}

void FixMips( SDDSHeader *pRes )
{
	if ( pRes->dwMipMapCount == 0 ) 
		pRes->dwMipMapCount = 1;
	NGfx::EPixelFormat format = GetPixelFormat( *pRes );
	int nMaxPossibleLevels = GetMSB( Min(pRes->dwWidth, pRes->dwHeight) ) - ( (format >= NGfx::CF_DXT1) && (format <= NGfx::CF_DXT5) ? 2 : 0 );
	pRes->dwMipMapCount = Min( (int)pRes->dwMipMapCount, (int)nMaxPossibleLevels );
}

void CFileTexture::Recalc()
{
	if ( IsValid(pRequest) && !pRequest->IsReady() && IsValid( pValue ) )
		return;

	const NDb::STexture *pTex = GetKey().tKey.pTexture;
	if ( !pTex )
	{
		ASSERT(0);
		CreateChecker();
		return;
	}
	NGfx::ETextureUsage eUsage;
	int nCutMips = nTextureUseMip;
	switch ( pTex->eType )
	{
		case NDb::STexture::REGULAR: eUsage = NGfx::REGULAR; break;
		case NDb::STexture::TEXTURE_2D: eUsage = NGfx::TEXTURE_2D; nCutMips = 0; break;
		default: ASSERT(0); eUsage = NGfx::REGULAR; break;
	}
	int nKeyFlags = GetKey().tKey.nFlags;
	if ( nKeyFlags & STextureKey::TK_TRANSPARENT )
		eUsage = NGfx::TRANSPARENT_TEXTURE;
	NGfx::EWrap eWrap = NGfx::CLAMP;
	switch ( nKeyFlags & (STextureKey::TK_WRAP_X|STextureKey::TK_WRAP_Y ) )
	{
	case STextureKey::TK_WRAP_X: eWrap = NGfx::WRAP_X; break;
	case STextureKey::TK_WRAP_Y: eWrap = NGfx::WRAP_Y; break;
	case STextureKey::TK_WRAP_Y|STextureKey::TK_WRAP_X: eWrap = NGfx::WRAP; break;
	case 0: eWrap = NGfx::CLAMP; break;
	default: ASSERT(0); break;
	}
	if ( !IsValid(pRequest) )
	{
		pRequest = CreateFileRequiest( pTex->szDestName.c_str(), 0 );//"Textures", GetRealTextureID( pTex ) );
		if ( pTex->eType == NDb::STexture::TEXTURE_2D || pTex->bInstantLoad )
			pRequest->Read();
		else
			AddFileRequest( pRequest );
	}

	if ( !pRequest->IsReady() )
	{
		bIsFakeTexture = true;
		bool bHasRead = false;
		if ( !bLowRAM && !NGfx::Is16BitTextures() )
		{
			SDDSFileHeader hdr;
			CResourceFileOpener file( "LRTextures", GetRealTextureID( pTex ) );
			if ( file.IsOk() )
			{
				file.GetFileStream().Read( &hdr, sizeof(hdr) );
				FixMips( &hdr.header );
				pValue = MakeTexture( hdr.header, eUsage, eWrap, 0 );
				if ( RealLoadTexture( pValue.GetPtr(), &file.GetFileStream(), hdr.header, 0, 0, hdr.header.dwWidth, hdr.header.dwHeight, hdr.header.dwMipMapCount, 0 ) )
					bHasRead = true;
			}
		}
		if ( !bHasRead )
		{
			if ( NGfx::Is16BitTextures() )
			{
				pValue = NGfx::MakeTexture( 1, 1, 1, NGfx::SPixel4444::ID, eUsage, eWrap );
				NGfx::CTextureLock<NGfx::SPixel4444> lock( pValue, 0, NGfx::INPLACE );
				lock[0][0] = DoConvert( NGfx::SPixel8888( pTex->nAverageColor ), (NGfx::SPixel4444*)0 );
			}
			else
			{
				pValue = NGfx::MakeTexture( 1, 1, 1, NGfx::SPixel8888::ID, eUsage, eWrap );
				NGfx::CTextureLock<NGfx::SPixel8888> lock( pValue, 0, NGfx::INPLACE );
				lock[0][0].dwColor = pTex->nAverageColor;
			}
		}
		return;
	}
	bIsFakeTexture = false;
	SDDSFileHeader hdr;
	CFileRequest &file = *pRequest;
	if ( file->GetSize() > 0 )
	{
		file->Seek(0);
		file->Read( &hdr, sizeof(hdr) );
		FixMips( &hdr.header );

		nCutMips = Min( nCutMips, int(hdr.header.dwMipMapCount) - 1 );
		if ( GetPixelFormat( hdr.header ) == NGfx::CF_A8R8G8B8 && NGfx::Is16BitTextures() )
		{
			pValue = LoadConvertTo16Bit( file.GetStream(), hdr.header, eUsage, eWrap, nCutMips, hdr.header.dwWidth, hdr.header.dwHeight, hdr.header.dwMipMapCount );
			if ( !pValue )
			{
				NI_ASSERT( pValue != 0, StrFmt("Can't load texture %s from stream \"%s\"", pTex->GetDBID().ToString().c_str(), pTex->szDestName.c_str()) );
				CreateChecker();
			}
		}
		else
		{
			pValue = MakeTexture( hdr.header, eUsage, eWrap, nCutMips );
			if ( !RealLoadTexture( pValue.GetPtr(), file.GetStream(), hdr.header, 0, 0, hdr.header.dwWidth, hdr.header.dwHeight, hdr.header.dwMipMapCount, nCutMips ) )
			{
				NI_ASSERT( false, StrFmt("Can't load texture %s from stream \"%s\"", pTex->GetDBID().ToString().c_str(), pTex->szDestName.c_str()) );
				CreateChecker();
			}
		}
	}
	else
	{
//		NI_ASSERT( false, StrFmt("Can't load texture %d from stream \"%s\" - empty stream", GetKey().tKey.nID, pTex->szDestName.c_str()) );
		CreateChecker();
	}
	pRequest = 0;
	ReleaseFileRequestHolder();
}

bool CFileTexture::NeedUpdate()
{
	bool bRes = TParent::NeedUpdate();
	return bIsFakeTexture || bRes;
}

// CFileCubeTexture

static string GetID( const NDb::STexture *p ) { if (p) return p->szDestName; return ""; }
void CFileCubeTexture::Recalc()
{
	string szTextureIDs[6];// = {0,0,0,0,0,0};
	const NDb::SCubeTexture *pTex = GetKey().tKey;
	ASSERT( pTex );
	if ( pTex )
	{
		szTextureIDs[0] = GetID( pTex->pPositiveX );
		szTextureIDs[1] = GetID( pTex->pPositiveY );
		szTextureIDs[2] = GetID( pTex->pPositiveZ );
		szTextureIDs[3] = GetID( pTex->pNegativeX );
		szTextureIDs[4] = GetID( pTex->pNegativeY );
		szTextureIDs[5] = GetID( pTex->pNegativeZ );
	}
	ASSERT( NGfx::POSITIVE_X == 0 );
	CObj<CFileRequest> pRequest;

	SDDSFileHeader hdrMain;
	{
		pRequest = CreateFileRequiest( szTextureIDs[0].c_str(), 0 );//"Textures", GetRealTextureID( NDb::Get<NDb::STexture>( nTextureIDs[0] ) ) );
		pRequest->Read();
		pRequest->GetStream()->Read( &hdrMain, sizeof(hdrMain) );
	}
	FixMips( &hdrMain.header );
	//nSize = key.GetTextureSize();
	pValue = NGfx::MakeCubeTexture( hdrMain.header.dwWidth, hdrMain.header.dwMipMapCount, GetPixelFormat(hdrMain.header), NGfx::REGULAR );
	for ( int i=0; i<6; ++i )
	{
		if ( szTextureIDs[i] == "" )
			continue;
		loadFace = (NGfx::EFace)i;
		SDDSFileHeader hdr;
		pRequest = CreateFileRequiest( szTextureIDs[i].c_str(), 0 );//"Textures", GetRealTextureID( NDb::Get<NDb::STexture>( nTextureIDs[i] ) ) );
		pRequest->Read();
		CFileRequest &file = *pRequest;
		file->Read( &hdr, sizeof(hdr) );
		FixMips( &hdr.header );

		if ( hdr.header.dwWidth != hdrMain.header.dwWidth || hdr.header.dwHeight != hdrMain.header.dwHeight ||
					hdr.header.dwMipMapCount != hdrMain.header.dwMipMapCount || GetPixelFormat(hdr.header) != GetPixelFormat(hdrMain.header) )
		{
			ASSERT(0);
			CreateChecker();
			return;
		}
		if ( !RealLoadTexture( pValue.GetPtr(), file.GetStream(), hdr.header, 0, 0, hdr.header.dwWidth, hdr.header.dwHeight, hdr.header.dwMipMapCount, 0 ) )
		{
			ASSERT(0);
			CreateChecker();
			return;
		}
	}
}

void CFileCubeTexture::CreateChecker()
{
	NGfx::SPixel8888 colors[2];
	colors[0] = NGfx::SPixel8888(0,0,0,255);
	colors[1] = NGfx::SPixel8888(255,255,255,255);
	const int nSize = 128;
	pValue = NGfx::MakeCubeTexture( nSize, 1, NGfx::SPixel8888::ID, NGfx::REGULAR );
	for ( int k = 0; k < 6; ++k )
	{
		NGfx::CTextureLock<NGfx::SPixel8888> lock( pValue, (NGfx::EFace)k, 0, NGfx::INPLACE );
		for ( int y = 0; y < nSize; ++y )
		{
			for ( int x = 0; x < nSize; ++x )
				lock[y][x] = colors[ ( (x&4) == 0 ) & ( (y&4) == 0 ) ];
		}
	}
}

// CColorTexture

void CColorTexture::Recalc()
{
	const int N_SIZE = 4;
	pValue = NGfx::MakeTexture( N_SIZE, N_SIZE, 1, NGfx::SPixel8888::ID, NGfx::REGULAR, NGfx::CLAMP );
	NGfx::CTextureLock<NGfx::SPixel8888> lock( pValue, 0, NGfx::INPLACE );
	for ( int y = 0; y < lock.GetSizeY(); ++y )
	{
		for ( int x = 0; x < lock.GetSizeX(); ++x )
			lock[y][x].dwColor = NGfx::GetDWORDColor( vColor );
	}
}


START_REGISTER(GTexture)
	//REGISTER_VAR_EX( "gfx_texture_usedxt", NGlobal::VarBoolHandler, &bDXTModeOn, 1, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_texture_mip", NGlobal::VarIntHandler, &nTextureUseMip, 0, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_low_ram", NGlobal::VarBoolHandler, &bLowRAM, 0, STORAGE_NONE )
FINISH_REGISTER

} // namespace
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x00821150, CFileTexture )
//REGISTER_SAVELOAD_CLASS( 0x116A1130, CFileTextureComplex )
REGISTER_SAVELOAD_CLASS( 0x01412121, CFileCubeTexture )
REGISTER_SAVELOAD_CLASS( 0x00682200, CColorTexture )

