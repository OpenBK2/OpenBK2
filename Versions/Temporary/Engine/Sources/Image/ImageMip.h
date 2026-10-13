#pragma once

namespace NImage
{
bool GenerateMipLevel( CArray2D<CVec4> *pDst, const CArray2D<CVec4> &src, bool bWrapX, bool bWrapY );
bool GenerateMipLevelPoint( CArray2D<CVec4> *pDst, const CArray2D<CVec4> &src );
void GenerateNormals( CArray2D<CVec4> *pData, const CVec4 &conv, float fMappingSize, bool bWrapX, bool bWrapY );
}
