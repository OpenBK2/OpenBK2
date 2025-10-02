#pragma once



namespace NImage
{
	void Copy( CArray2D<DWORD> *pDestination, const CArray2D<DWORD> &rSource, const CTPoint<int> &rStartPoint );
	//
	void Load2Bitmap( CBitmap *pBitmap, const CArray2D<DWORD> &rImage );
	void Load2Bitmap( CBitmap *pBitmap, const CArray2D<DWORD> &rImage, const CTPoint<int> &rSize );
};

