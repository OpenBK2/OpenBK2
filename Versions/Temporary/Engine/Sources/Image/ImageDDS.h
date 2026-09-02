#pragma once

#include <cstdint>

#include "Image_export.h"

struct IDirect3DDevice9;
namespace NGfx
{
	enum EPixelFormat : int;
}

namespace NImage
{
bool RecognizeFormatDDS( CDataStream *pStream );
IMAGE_EXPORT bool LoadImageDDS( CArray2D<uint32_t> *pRes, CDataStream *pStream );
//! convert to DDS using DX compression function
//! defined in ImageDDSWrite.cpp, which is built on Windows only
IMAGE_EXPORT void ConvertAndSaveAsDDSWithDX( IDirect3DDevice9 * pDevice, const std::string &szFileName, const CArray2D<uint32_t> &srcImage,
															 EImageType eImageType, NGfx::EPixelFormat nSubFormat, int nNumMipLevels, 
															 bool bWrapX, bool bWrapY, float fMappingSize );
}

