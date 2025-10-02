#ifndef __IMAGE_BMP_H__
#define __IMAGE_BMP_H__

namespace NImage
{
	bool RecognizeFormatBMP( CDataStream *pStream );
	bool LoadImageBMP( CArray2D<DWORD> *pRes, CDataStream *pStream );
};

#endif // __IMAGE_BMP_H__
