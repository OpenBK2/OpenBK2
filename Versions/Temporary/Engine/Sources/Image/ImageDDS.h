#pragma once

struct IDirect3DDevice9;
namespace NGfx
{
	enum EPixelFormat;
}

namespace NImage
{
bool RecognizeFormatDDS( CDataStream *pStream );
bool LoadImageDDS( CArray2D<DWORD> *pRes, CDataStream *pStream );
//! convert to DDS using DX compression function
void ConvertAndSaveAsDDSWithDX( IDirect3DDevice9 * pDevice, const std::string &szFileName, const CArray2D<DWORD> &srcImage,
															 EImageType eImageType, NGfx::EPixelFormat nSubFormat, int nNumMipLevels, 
															 bool bWrapX, bool bWrapY, float fMappingSize );
}

