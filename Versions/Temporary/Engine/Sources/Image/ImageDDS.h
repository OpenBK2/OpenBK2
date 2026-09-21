#pragma once

#include <cstdint>

#include "Image_export.h"

namespace NGfx
{
	enum EPixelFormat : int;
}

namespace NImage
{
bool RecognizeFormatDDS( CDataStream *pStream );
IMAGE_EXPORT bool LoadImageDDS( CArray2D<uint32_t> *pRes, CDataStream *pStream );
//! Compress to one of the DXT or ARGB formats and write a .dds.
//! Defined in ImageDDSWrite.cpp. Builds the container itself and compresses
//! through squish, so it needs neither D3DX nor a Direct3D device and builds
//! everywhere.
IMAGE_EXPORT bool ConvertAndSaveAsDDS( const std::string &szFileName, const CArray2D<uint32_t> &srcImage,
															 EImageType eImageType, NGfx::EPixelFormat nSubFormat, int nNumMipLevels,
															 bool bWrapX, bool bWrapY, float fMappingSize );
}

