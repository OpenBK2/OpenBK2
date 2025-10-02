#pragma once


namespace NUITools
{
	void ApplyWindowAlign( NDb::EPositionAllign eAlign, float fParentPos, float fParentSize,
												 float fChildPos, 
												 float fLowerMargin, float fHigherMargin,
												 float *nSize,	/* in, out */
												 float *pScreenPos /* out */ );

	void ApplyTextureAllign( NDb::EPositionAllign eAllign,
													 float fWidth, float fTextureWidth,
													 float *pfMap1/*out*/, float *pfMap2/*out*/,
													 float *pfPos/* in, out*/ );

	void ApplyPlacement( const NDb::SWindowPlacement &placement, const CTRect<float> &parentRect, 
		CTRect<float> *pRect );
}

