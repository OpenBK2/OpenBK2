#include "stdafx.h"

#include "UITextureFilter.h"

#include <algorithm>
#include <cmath>

namespace NGScene
{
namespace
{
	struct SColorAverage
	{
		float fR = 0, fG = 0, fB = 0, fA = 0;

		void Add( const NGfx::SPixel8888 &pixel, float fWeight )
		{
			const float fCoverage = pixel.a * fWeight;
			fR += pixel.r * fCoverage;
			fG += pixel.g * fCoverage;
			fB += pixel.b * fCoverage;
			fA += fCoverage;
		}

		NGfx::SPixel8888 Get() const
		{
			if ( fA <= 0 )
			{
				return NGfx::SPixel8888( 0, 0, 0, 0 );
			}
			return NGfx::SPixel8888( Clamp( int( fR / fA + 0.5f ), 0, 255 ),
			                         Clamp( int( fG / fA + 0.5f ), 0, 255 ),
			                         Clamp( int( fB / fA + 0.5f ), 0, 255 ),
			                         Clamp( int( fA + 0.5f ), 0, 255 ) );
		}
	};

	void SampleLevel( SColorAverage *pColor, const CArray2D<NGfx::SPixel8888> &image, float fU, float fV, float fWeight )
	{
		const float fX = Clamp( fU - 0.5f, 0.0f, float( image.GetSizeX() - 1 ) );
		const float fY = Clamp( fV - 0.5f, 0.0f, float( image.GetSizeY() - 1 ) );
		const int x = int( fX ), y = int( fY );
		const int x1 = (std::min)( x + 1, image.GetSizeX() - 1 );
		const int y1 = (std::min)( y + 1, image.GetSizeY() - 1 );
		const float fDX = fX - x, fDY = fY - y;
		pColor->Add( image[y][x], fWeight * ( 1 - fDX ) * ( 1 - fDY ) );
		pColor->Add( image[y][x1], fWeight * fDX * ( 1 - fDY ) );
		pColor->Add( image[y1][x], fWeight * ( 1 - fDX ) * fDY );
		pColor->Add( image[y1][x1], fWeight * fDX * fDY );
	}
} // namespace

void BuildUITextureMips( std::vector<CArray2D<NGfx::SPixel8888>> *pMips, const CArray2D<NGfx::SPixel8888> &image )
{
	// Build separately so the source may be the caller's existing base mip.
	std::vector<CArray2D<NGfx::SPixel8888>> result;
	if ( image.GetSizeX() <= 0 || image.GetSizeY() <= 0 )
	{
		pMips->clear();
		return;
	}
	int nWidth = GetNextPow2( image.GetSizeX() ), nHeight = GetNextPow2( image.GetSizeY() );
	result.emplace_back( nWidth, nHeight );
	for ( int y = 0; y < nHeight; ++y )
	{
		for ( int x = 0; x < nWidth; ++x )
		{
			result[0][y][x] = image[(std::min)( y, image.GetSizeY() - 1 )][(std::min)( x, image.GetSizeX() - 1 )];
		}
	}
	while ( nWidth > 1 || nHeight > 1 )
	{
		nWidth = (std::max)( 1, nWidth / 2 );
		nHeight = (std::max)( 1, nHeight / 2 );
		CArray2D<NGfx::SPixel8888> next( nWidth, nHeight );
		const auto &source = result.back();
		for ( int y = 0; y < nHeight; ++y )
		{
			for ( int x = 0; x < nWidth; ++x )
			{
				SColorAverage color;
				for ( int dy = 0; dy < 2; ++dy )
				{
					for ( int dx = 0; dx < 2; ++dx )
					{
						color.Add( source[(std::min)( y * 2 + dy, source.GetSizeY() - 1 )]
						                 [(std::min)( x * 2 + dx, source.GetSizeX() - 1 )],
						           0.25f );
					}
				}
				next[y][x] = color.Get();
			}
		}
		result.push_back( std::move( next ) );
	}
	pMips->swap( result );
}

NGfx::SPixel8888 SampleUITexture( const std::vector<CArray2D<NGfx::SPixel8888>> &mips, float fU, float fV, float fLOD )
{
	if ( mips.empty() )
	{
		return NGfx::SPixel8888( 0, 0, 0, 0 );
	}
	fLOD = Clamp( fLOD, 0.0f, float( mips.size() - 1 ) );
	const int nLow = int( fLOD ), nHigh = (std::min)( nLow + 1, int( mips.size() - 1 ) );
	const float fBlend = fLOD - nLow;
	SColorAverage color;
	for ( int i = 0; i < 2; ++i )
	{
		const auto &level = mips[i == 0 ? nLow : nHigh];
		SampleLevel( &color,
		             level,
		             fU * level.GetSizeX() / mips[0].GetSizeX(),
		             fV * level.GetSizeY() / mips[0].GetSizeY(),
		             i == 0 ? 1 - fBlend : fBlend );
	}
	return color.Get();
}
} // namespace NGScene
