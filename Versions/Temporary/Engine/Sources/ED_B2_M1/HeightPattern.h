#if !defined(__HEIGHT_PATTERN__)
#define __HEIGHT_PATTERN__
#pragma once

#include "..\misc\2DArray.h"
#include "..\MapEditorLib\Tools_Gradient.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SHeightPattern
{
	CTPoint<int> pos;
	float fRatio;
	CArray2D<float> heights;
	bool bOdd;
	//
	SHeightPattern() : pos( 0, 0 ), fRatio( 1.0f ), bOdd( false ) {}
	SHeightPattern( const CTPoint<int> &rPos, float _fRatio, const CArray2D<float> &rHeights, bool _bOdd )
		: pos( rPos ), fRatio( _fRatio ), heights( rHeights ), bOdd( _bOdd ) {}
	SHeightPattern( int nPosX, int nPosY, float _fRatio, const CArray2D<float> &rHeights, bool _bOdd )
		: pos( nPosX, nPosY ), fRatio( _fRatio ), heights( rHeights ), bOdd( _bOdd ) {}
	SHeightPattern( const SHeightPattern &rPattern )
		: pos( rPattern.pos ), fRatio( rPattern.fRatio ), heights( rPattern.heights ), bOdd ( rPattern.bOdd ) {}
	SHeightPattern& operator=( const SHeightPattern &rPattern )
	{
		if( &rPattern != this )
		{
			pos = rPattern.pos;
			fRatio = rPattern.fRatio;
			heights = rPattern.heights;
			bOdd = rPattern.bOdd;
		}
		return *this;
	}
	//
	bool CreateByGradient( float fValue, int nGridLines, const SGradient &rGradient );
	bool CreateByValue( float fValue, int nGridLines, bool bEllipse );
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//функционал создающий паттерн по градиенту
struct SCreateHeightPatternByGradientFunctional
{
	SHeightPattern *pHeightPattern;
	const SGradient *pGradient;
	float fRatio;
	float bSquareInterpolated;
	//
	SCreateHeightPatternByGradientFunctional( SHeightPattern *_pHeightPattern, const SGradient *_pGradient, float _fRatio, bool _bSquareInterpolated )
		: pHeightPattern( _pHeightPattern ), pGradient( _pGradient ), fRatio( _fRatio ), bSquareInterpolated( _bSquareInterpolated )
	{
		NI_ASSERT( pHeightPattern != 0, "Wrong parameter: pHeightPattern == 0" );
		NI_ASSERT( pGradient != 0, "Wrong parameter: pGradient == 0" );
	}
	//
	bool operator()( int nXIndex, int nYIndex, float fDistance )
	{ 
		pHeightPattern->heights[nYIndex][nXIndex] = ( *pGradient )( fDistance * ( pGradient->range.max - pGradient->range.min ), bSquareInterpolated ) * fRatio;
		return true;
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SCreateHeightPatternByValueFunctional
{
	SHeightPattern *pHeightPattern;
	float fValue;
	//
	SCreateHeightPatternByValueFunctional( SHeightPattern *_pHeightPattern, float _fValue )
		: pHeightPattern( _pHeightPattern ), fValue( _fValue )
	{
		NI_ASSERT( pHeightPattern != 0, "Wrong parameter: pHeightPattern == 0" );
	}
	//
	bool operator()( int nXIndex, int nYIndex, float fDistance )
	{ 
		pHeightPattern->heights[nYIndex][nXIndex] = fValue;
		return true;
	}
};
	
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//функционал создающий паттерн заравнивания
struct SCreateHeightPatternToLevelFunctional
{
	SHeightPattern *pHeightPattern;
	float fAverageHeight;
	float fLevelRatio;

	SCreateHeightPatternToLevelFunctional( SHeightPattern *_pHeightPattern, float _fAverageHeight, float _fLevelRatio )
		: fAverageHeight( _fAverageHeight ), fLevelRatio( _fLevelRatio ), pHeightPattern( _pHeightPattern )
	{
		NI_ASSERT( pHeightPattern != 0, "Wrong parameter: pHeightPattern == 0" );
	}

	bool operator()( int nXIndex, int nYIndex, float fValue )
	{ 
		if ( fValue == 1.0f )
		{
			float fDelta = ( fAverageHeight - GetHeight( nXIndex, nYIndex ) ) * fLevelRatio;
			pHeightPattern->heights[nYIndex - pHeightPattern->pos.y][nXIndex - pHeightPattern->pos.x] = fDelta;
		}
		return true;
	}
	//
	virtual float GetHeight( int nXIndex, int nYIndex ) = 0;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//функционал создающий паттерн заравнивания, все высоты приводятся к положительным величинам
struct SCreateHeightPatternToLevelOverZeroFunctional
{
	SHeightPattern *pHeightPattern;
	float fAverageHeight;
	float fLevelRatio;

	SCreateHeightPatternToLevelOverZeroFunctional( SHeightPattern *_pHeightPattern, float _fAverageHeight, float _fLevelRatio )
		: fAverageHeight( _fAverageHeight ), fLevelRatio( _fLevelRatio ), pHeightPattern( _pHeightPattern )
	{
		NI_ASSERT( pHeightPattern != 0, "Wrong parameter: pHeightPattern == 0" );
		if ( fAverageHeight < 0 )
		{
			fAverageHeight = 0.0f;
		}
	}

	bool operator()( int nXIndex, int nYIndex, float fValue )
	{ 
		if ( fValue == 1.0f )
		{
			const float fHeight = GetHeight( nXIndex, nYIndex );
			if ( fHeight >= 0 )
			{
				const float fDelta = ( fAverageHeight - fHeight ) * fLevelRatio;
				pHeightPattern->heights[nYIndex - pHeightPattern->pos.y][nXIndex - pHeightPattern->pos.x] = fDelta;
			}
			else
			{
				pHeightPattern->heights[nYIndex - pHeightPattern->pos.y][nXIndex - pHeightPattern->pos.x] = fHeight * ( -1.0f );
			}
		}
		return true;
	}
	//
	virtual float GetHeight( int nXIndex, int nYIndex ) = 0;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//функционал создающий паттерн в высотами
struct SCreateHeightPatternFunctional
{
	SHeightPattern *pHeightPattern;

	SCreateHeightPatternFunctional( SHeightPattern *_pHeightPattern )
		: pHeightPattern( _pHeightPattern )
	{
		NI_ASSERT( pHeightPattern != 0, "Wrong parameter: pHeightPattern == 0" );
	}

	bool operator()( int nXIndex, int nYIndex, float fValue )
	{ 
		if ( fValue == 1.0f )
		{
			pHeightPattern->heights[nYIndex - pHeightPattern->pos.y][nXIndex - pHeightPattern->pos.x] = GetHeight( nXIndex, nYIndex );
		}
		return true;
	}
	//
	virtual float GetHeight( int nXIndex, int nYIndex ) = 0;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//функционал определяющий среднее значение высоты
struct SCalculateAverageHeightFunctional
{
	float fTotalHeight;
	int nPointCount;
	//	
	SCalculateAverageHeightFunctional() : fTotalHeight( 0.0f ), nPointCount( 0 ) {}
	//
	bool operator()( int nXIndex, int nYIndex, float fValue )
	{ 
		if ( fValue != 0.0f )
		{
			fTotalHeight += GetHeight( nYIndex, nXIndex );
			++nPointCount;
		}
		return true;
	}
	//
	float GetAverageHeight() { return ( ( nPointCount != 0 ) ? ( fTotalHeight / nPointCount ) : 0.0f ); }
	//
	virtual float GetHeight( int nXIndex, int nYIndex ) = 0;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//применение функционалов для всех элементов SHeightPattern.heights
template<class TYPE>
bool ApplyHightPatterns( const CTRect<int> &rRect,
												 const std::vector<SHeightPattern> &rPatterns,
												 TYPE &rApplyFunctional,								//функционал
												 bool isIgnoreInvalidIndices = false )	//пропускать обьекты за краями карты
{
	for ( int nPatternIndex = 0; nPatternIndex < rPatterns.size(); ++nPatternIndex )
	{
		CTRect<int> indices( rPatterns[nPatternIndex].pos.x,
												 rPatterns[nPatternIndex].pos.y,
												 rPatterns[nPatternIndex].pos.x + rPatterns[nPatternIndex].heights.GetSizeX(),
												 rPatterns[nPatternIndex].pos.y + rPatterns[nPatternIndex].heights.GetSizeY() );
		const int nResult = ValidateRect( rRect, &indices );
		//нет ни одного вертекса
		if ( nResult < 0 )
		{
			if ( isIgnoreInvalidIndices )
			{
				//скипаем обьект, переходим к следующему
				continue;
			}
			else
			{
				//возвращаем ошибку
				return false;
			}
		}
		//некоторые вертексы лишние
		if ( ( nResult == 0 ) && !isIgnoreInvalidIndices )
		{
			//возвращаем ошибку
			return false;
		}
		//пробегаем по тайлам
		for ( int nXIndex = indices.minx; nXIndex < indices.maxx; ++nXIndex )
		{
			for ( int nYIndex = indices.miny; nYIndex < indices.maxy; ++nYIndex )
			{
				if ( !rApplyFunctional( nXIndex, nYIndex, rPatterns[nPatternIndex].heights[nYIndex - rPatterns[nPatternIndex].pos.y][nXIndex - rPatterns[nPatternIndex].pos.x] * rPatterns[nPatternIndex].fRatio ) )
				{
					return false;
				}
			}
		}
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//применение функционалов для всех элементов SHeightPattern.heights
template<class TYPE>
bool ApplyHeightPattern( const CTRect<int> &rRect,
												 const SHeightPattern &rPattern,
												 TYPE &rApplyFunctional,
												 bool isIgnoreInvalidIndices = false )
{
	CTRect<int> indices( rPattern.pos.x,
											 rPattern.pos.y,
											 rPattern.pos.x + rPattern.heights.GetSizeX(),
											 rPattern.pos.y + rPattern.heights.GetSizeY() );
	const int nResult = ValidateRect( rRect, &indices );
	//нет ни одного вертекса
	if ( nResult < 0 )
	{
		if ( isIgnoreInvalidIndices )
		{
			//скипаем обьект
			return true;
		}
		else
		{
			//возвращаем ошибку
			return false;
		}
	}
	//некоторые вертексы лишние
	if ( ( nResult == 0 ) && !isIgnoreInvalidIndices )
	{
		//возвращаем ошибку
		return false;
	}
	//пробегаем по тайлам
	for ( int nXIndex = indices.minx; nXIndex < indices.maxx; ++nXIndex )
	{
		for ( int nYIndex = indices.miny; nYIndex < indices.maxy; ++nYIndex )
		{
			if ( !rApplyFunctional( nXIndex, nYIndex, rPattern.heights[nYIndex - rPattern.pos.y][nXIndex - rPattern.pos.x] * rPattern.fRatio ) )
			{
				return false;
			}
		}
	}
	//	
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Специальнй случай - применение функционала в цепочке точек (не нужно создавать несколько функционалов)
template<class TYPE>
bool ApplyHeightPatternInChain( const CTRect<int> &rRect,
																SHeightPattern *pHeightPattern,
																std::vector<CTPoint<int> > &rPointChain,
																TYPE &rApplyFunctional,
																bool isIgnoreInvalidIndices = false,
																std::vector<CTRect<int> > *pIgnoreRects = 0 )
{
	NI_ASSERT_T( pHeightPattern != 0,
							 NStr::Format( "Wrong parameter: %x\n", pHeightPattern ) );

	for ( int nPointIndex = 0; nPointIndex < rPointChain.size(); ++nPointIndex )
	{
		pHeightPattern->pos = rPointChain[nPointIndex];
		CTRect<int> indices( pHeightPattern->pos.x,
												 pHeightPattern->pos.y,
												 pHeightPattern->pos.x + pHeightPattern->heights.GetSizeX(),
												 pHeightPattern->pos.y + pHeightPattern->heights.GetSizeY() );
		const int nResult = ValidateRect( rRect, &indices );
		//нет ни одного вертекса
		if ( nResult < 0 )
		{
			if ( isIgnoreInvalidIndices )
			{
				//скипаем обьект, переходим к следующему
				continue;
			}
			else
			{
				//возвращаем ошибку
				return false;
			}
		}
		//некоторые вертексы лишние
		if ( ( nResult == 0 ) && !isIgnoreInvalidIndices )
		{
			//возвращаем ошибку
			return false;
		}
		//пробегаем по тайлам
		for ( int nXIndex = indices.minx; nXIndex < indices.maxx; ++nXIndex )
		{
			for ( int nYIndex = indices.miny; nYIndex < indices.maxy; ++nYIndex )
			{
				bool bOutsideIgnoreRects = true;				
				if ( pIgnoreRects )
				{
					for ( int nRectIndex = 0; nRectIndex < pIgnoreRects->size(); ++nRectIndex )
					{
						const CTRect<int> &rIgrnoreRect = ( *pIgnoreRects )[nRectIndex];
						if ( IsValidPoint( rIgrnoreRect, nXIndex, nYIndex ) )
						{
							bOutsideIgnoreRects = false;
							break;	
						}
					}
				}
				if ( bOutsideIgnoreRects )
				{
					if ( !rApplyFunctional( nXIndex, nYIndex, pHeightPattern->heights[nYIndex - pHeightPattern->pos.y][nXIndex - pHeightPattern->pos.x] * pHeightPattern->fRatio ) )
					{
						return false;
					}
				}
			}
		}
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//применение функционалов внутри эллипса для полея heights
template<class TYPE>
static bool ApplyInRadius( const CTRect<int> &rRect, TYPE &rApplyFunctional )
{
	NI_ASSERT( ( rRect.Width() > 0 ) && ( rRect.Height() > 0 ),
						 StrFmt( "Invalid sizes: (%d, %d)\n", rRect.Width(), rRect.Height() ) );
	if ( ( rRect.Width() <= 0 ) || ( rRect.Height() <= 0 ) )
	{
		return false;
	}
	//
	float fHalfAxisX = ( rRect.Width() - 1.0f ) / 2.0f;
	float fHalfAxisY = ( rRect.Height() - 1.0f ) / 2.0f;
	//
	for ( int nXIndex = 0; nXIndex < rRect.Width(); ++nXIndex )
	{
		for ( int nYIndex = 0; nYIndex < rRect.Height(); ++nYIndex )
		{
			const float fPosX = ( nXIndex - fHalfAxisX ) / fHalfAxisX;
			const float fPosY = ( nYIndex - fHalfAxisY ) / fHalfAxisY;
			const float fDistance = fabs( fPosX, fPosY );
			if ( fDistance <= 1.0f )
			{
				if ( !rApplyFunctional( nXIndex + rRect.minx, nYIndex + rRect.miny, fDistance ) )
				{
					return false;
				}
			}
		}
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif //#if !defined(__HEIGHT_PATTERN__)
