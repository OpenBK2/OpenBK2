#pragma once


template <class T> class CArray2D;

bool LoadGrayTGAImage( CDataStream *pStream, CArray2D<BYTE> &data );
bool AppendGrayTGAImageAtBottom( CDataStream *pStream, CArray2D<BYTE> &data, int nPosY );
bool SaveGrayTGAImage( CDataStream *pStream, CArray2D<BYTE> &data );


