#pragma once

namespace NImage
{
	bool RecognizeFormatBMP( CDataStream *pStream );
	bool LoadImageBMP( CArray2D<DWORD> *pRes, CDataStream *pStream );
};


