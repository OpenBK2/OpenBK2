#pragma once

#include <cstdint>

namespace NImage
{
	bool RecognizeFormatBMP( CDataStream *pStream );
	bool LoadImageBMP( CArray2D<uint32_t> *pRes, CDataStream *pStream );
};


