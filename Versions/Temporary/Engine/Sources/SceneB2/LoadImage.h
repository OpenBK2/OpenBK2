#pragma once

#include <cstdint>

template <class T>
class CArray2D;

bool LoadGrayTGAImage( CDataStream *pStream, CArray2D<uint8_t> &data );
bool SaveGrayTGAImage( CDataStream *pStream, CArray2D<uint8_t> &data );
