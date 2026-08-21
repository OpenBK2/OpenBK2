#include "stdafx.h"

#include <thread>
#include "SWTexture.h"
#include "DBScene.h"
#include "Image/DDS.h"
#include "Image/GUnpackDXT.h"

#include <cstdint>

NGfx::EPixelFormat GetPixelFormat( const SDDSHeader &hdr );
namespace NGScene
{

// CFileTexture

template <class TPixel>
void LoadTextureData( std::vector<CArray2D<TPixel> > *pMips, int _nMips, int _nSizeX, int _nSizeY, CDataStream *pFile )
{
	std::vector<CArray2D<TPixel> > &mips = *pMips;
	//int nXSize = _nSizeX;
	//int nYSize = _nSizeY;
	
	ASSERT( _nMips > 0 );
	_nMips = (std::max)( _nMips, 1 );
	mips.resize( _nMips );
	for ( int nMip = 0; nMip < _nMips; ++nMip )
	{
		int nXSize = (_nSizeX >> nMip) / TPixel::XSize;
		int nYSize = (_nSizeY >> nMip) / TPixel::YSize;
		ASSERT( nXSize > 0 && nYSize > 0 );
		mips[nMip].SetSizes( nXSize, nYSize );
		for ( int y = 0; y < nYSize; ++y )
			pFile->Read( &mips[nMip][y][0], nXSize * sizeof(TPixel) );
		//nXSize >>= 1;
		//nYSize >>= 1;
	}
}

static void CreateChecker( CSWTextureData *pTexture )
{
	NGfx::SPixel8888 colors[2];
	colors[0] = NGfx::SPixel8888(0,0,0,255);
	colors[1] = NGfx::SPixel8888(255,255,255,255);
	const int nSize = 128;

	pTexture->mips.resize( 1 );
	pTexture->mips[0].SetSizes( nSize, nSize );

	for ( int y = 0; y < nSize; ++y )
		for ( int x = 0; x < nSize; ++x )
			pTexture->mips[0][y][x] = colors[ ( ( x & 4) == 0 ) & ( ( y & 4) == 0 ) ];
}

bool CSWTexture::IsReady()
{ 
	Touch();
	if ( bIsReady )
		return true;
	if ( IsValid(pRequest) )
	{
		if ( !pRequest->IsReady() )
			return false;
		bIsReady = true;
		LoadTexture();
		pRequest = 0;
		ReleaseFileRequestHolder();
		return true;
	}
	const NDb::STexture *pTex = GetKey().tKey;
	if ( !pTex )
		return true;
	pRequest = CreateFileRequiest( pTex->szDestName.c_str(), 0 ); //CreateFileRequiest( "Textures", GetKey() );
	AddFileRequest( pRequest );
	return false; 
}

template<class TPixel>
void LoadDxt( CSWTextureData *pValue, int nDxt, int nSizeX, int nSizeY, int nMips, CDataStream *pStream )
{
	std::vector<CArray2D<TPixel> > dxt;
	LoadTextureData<TPixel>( &dxt, nMips, nSizeX, nSizeY, pStream ); 
	pValue->mips.resize( dxt.size() );
	for ( int k = 0; k < dxt.size(); ++k )
	{
		const CArray2D<TPixel> &src = dxt[k];
		CArray2D<NGfx::SPixel8888> &dst = pValue->mips[k];
		int nSizeX = src.GetSizeX() * TPixel::XSize;
		int nSizeY = src.GetSizeY() * TPixel::YSize;
		dst.SetSizes( nSizeX, nSizeY );
		NImage::UnpackDXT( nDxt, nSizeX, nSizeY, &src[0][0], (CArray2D<uint32_t>*)&dst );
	}
}

void FixMips( SDDSHeader *pRes );
void CSWTexture::LoadTexture()
{
	pValue = new CSWTextureData;
	if ( !pRequest )
	{
		CreateChecker( pValue );
		return;
	}

	CFileRequest &file = *pRequest;
	SDDSFileHeader hdr;
	file->Seek( 0 );
	file->Read( &hdr, sizeof(hdr) );
	FixMips( &hdr.header );

	int nMips = hdr.header.dwMipMapCount;
	int nSizeX = hdr.header.dwWidth;
	int nSizeY = hdr.header.dwHeight;
	// fill with data
	switch ( GetPixelFormat( hdr.header ) )
	{
		case NGfx::CF_DXT1: LoadDxt<NGfx::SPixelDXT1>( pValue, 1, nSizeX, nSizeY, nMips, file.GetStream() ); break;
		case NGfx::CF_DXT2: LoadDxt<NGfx::SPixelDXT2>( pValue, 2, nSizeX, nSizeY, nMips, file.GetStream() ); break;
		case NGfx::CF_DXT3: LoadDxt<NGfx::SPixelDXT3>( pValue, 3, nSizeX, nSizeY, nMips, file.GetStream() ); break;
		case NGfx::CF_DXT4: LoadDxt<NGfx::SPixelDXT4>( pValue, 4, nSizeX, nSizeY, nMips, file.GetStream() ); break;
		case NGfx::CF_DXT5: LoadDxt<NGfx::SPixelDXT5>( pValue, 5, nSizeX, nSizeY, nMips, file.GetStream() ); break;
		/*
			case NGfx::CF_R5G6B5:		LoadTextureData<NGfx::SPixel565>( pValue, hdr.nSizeX, hdr.nSizeY, hdr.nNumMipLevels, file.GetStream() ); break;
			case NGfx::CF_A1R5G5B5: LoadTextureData<NGfx::SPixel1555>( pValue, hdr.nSizeX, hdr.nSizeY, hdr.nNumMipLevels, file.GetStream() ); break;
			case NGfx::CF_A4R4G4B4: LoadTextureData<NGfx::SPixel4444>( pValue, hdr.nSizeX, hdr.nSizeY, hdr.nNumMipLevels, file.GetStream() ); break;
			case NGfx::CF_NORMALES: LoadTextureData<NGfx::SPixel8888>( pValue, hdr.nSizeX, hdr.nSizeY, hdr.nNumMipLevels, file.GetStream() ); break;
			*/
		case NGfx::CF_A8R8G8B8: LoadTextureData<NGfx::SPixel8888>( &pValue->mips, nMips, nSizeX, nSizeY, file.GetStream() ); break;
		default: ASSERT( 0 ); CreateChecker( pValue ); break;
	}
}

void CSWTexture::Recalc()
{
	//ASSERT(0);
	while ( !IsReady() )
		std::this_thread::yield();
}

// CSWTextureData

void CSWTextureData::PrepareBump()
{
	if ( bumpMips.size() == mips.size() )
		return;
	bumpMips.resize( mips.size() );
	for ( int k = 0; k < mips.size(); ++k )
	{
		bumpMips[k].SetSizes( mips[k].GetSizeX(), mips[k].GetSizeY() );
		for ( int y = 0; y < mips[k].GetSizeY(); ++y )
		{
			for ( int x = 0; x < mips[k].GetSizeX(); ++x )
			{
				NGfx::SPixel8888 src = mips[k][y][x];
				SBumpPixel &res = bumpMips[k][y][x];
				if ( src.b == 128 )
				{
					res.fDU = res.fDV = 0;
				}
				else
				{
					float fInv = 1.0f / ( src.b - 128 );
					res.fDU = ( ((int)src.r) - 128 ) * fInv;
					res.fDV = ( ((int)src.g) - 128 ) * fInv;
				}
			}
		}
	}
}

// CBilinearTexture

void CBilinearTexture::Recalc()
{
	pValue = new CSWTextureData;
	pValue->mips.resize( 1 );
	pValue->mips[0].SetSizes( nXSize, nYSize );
	ASSERT( pic.GetSizeX() > 1 && pic.GetSizeY() > 1 );
	float fdU = 0, fdV = 0;
	int nPicXSize = pic.GetSizeX();
	int nPicYSize = pic.GetSizeY();
	if ( nXSize > 1 )
		fdU = ( (float)nPicXSize - 1.01f ) / ( nXSize - 1 );
	if ( nYSize > 1 )
		fdV = ( (float)nPicYSize - 1.01f ) / ( nYSize - 1 );
	int nUPos, nDU = Float2Int( fdU * 0x8000 ), nVPos, nDV = Float2Int( fdV * 0x8000 );

	nVPos = 0;
	for ( int y = 0; y < nYSize; ++y )
	{
		nUPos = 0;
		NGfx::SPixel8888 *pDst = &pValue->mips[0][y][0];

		float fVPos = y * fdV;
		int v0 = static_cast<int>(fVPos);
		int v1 = (std::min)(v0 + 1,  nPicYSize - 1);
		float fy = fVPos - v0;

		for ( int x = 0; x < nXSize; ++x )
		{

			float fUPos = x * fdU;
			int u0 = static_cast<int>(fUPos);
			int u1 = (std::min)(u0 + 1, nPicXSize - 1);
			float fx = fUPos - u0;

			const NGfx::SPixel8888 & p00 = pic[v0][u0];
			const NGfx::SPixel8888 & p10 = pic[v1][u0];
			const NGfx::SPixel8888 & p01 = pic[v0][u1];
			const NGfx::SPixel8888 & p11 = pic[v1][u1];

			auto lerp = [](uint8_t a, uint8_t b, float t) {
				return static_cast<uint8_t>(a * (1 - t) + b * t + 0.5f);
			};

			NGfx::SPixel8888 & dst = pValue->mips[0][y][x];

			uint8_t r0 = lerp(p00.r, p10.r, fx);
			uint8_t g0 = lerp(p00.g, p10.g, fx);
			uint8_t b0 = lerp(p00.b, p10.b, fx);
			uint8_t a0 = lerp(p00.a, p10.a, fx);

			uint8_t r1 = lerp(p01.r, p11.r, fx);
			uint8_t g1 = lerp(p01.g, p11.g, fx);
			uint8_t b1 = lerp(p01.b, p11.b, fx);
			uint8_t a1 = lerp(p01.a, p11.a, fx);

			dst.r = lerp(r0, r1, fy);
			dst.g = lerp(g0, g1, fy);
			dst.b = lerp(b0, b1, fy);
			dst.a = lerp(a0, a1, fy);

			pDst++;
			nUPos += nDU;
		}
		nVPos += nDV;
	}
}

} // namespace
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0xF0821150, CSWTexture )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x007c1140, CBilinearTexture )

