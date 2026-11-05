#include "ED_B2_M1/stdafx.h"

#include "ED_B2_M1/IconCapture.h"

#include <cmath>

#include <gtest/gtest.h>

TEST( IconCapture, DefaultReproducesTheLegacyCrop )
{
	CArray2D<int> capture( 512, 512 );
	for ( int y = 0; y < capture.GetSizeY(); ++y )
	{
		for ( int x = 0; x < capture.GetSizeX(); ++x )
		{
			capture[y][x] = y * 512 + x;
		}
	}
	CArray2D<int> image;
	ASSERT_TRUE( NIconRendering::CropCapture( &image, capture, NIconRendering::SOptions() ) );
	ASSERT_EQ( 42, image.GetSizeX() );
	ASSERT_EQ( 42, image.GetSizeY() );
	EXPECT_EQ( capture[235][235], image[0][0] );
	EXPECT_EQ( capture[276][276], image[41][41] );
	EXPECT_NEAR( 54.0f, NIconRendering::GetCaptureFOV( 54.0f, NIconRendering::SOptions() ), 0.00001f );
}

TEST( IconCapture, RectangularCropPreservesRowsColumnsAndOddSizes )
{
	CArray2D<int> capture( 9, 7 );
	for ( int y = 0; y < capture.GetSizeY(); ++y )
	{
		for ( int x = 0; x < capture.GetSizeX(); ++x )
		{
			capture[y][x] = y * 100 + x;
		}
	}
	for ( const auto &options : { NIconRendering::SOptions{ 5, 2 }, NIconRendering::SOptions{ 2, 5 } } )
	{
		CArray2D<int> image;
		ASSERT_TRUE( NIconRendering::CropCapture( &image, capture, options ) );
		ASSERT_EQ( options.nWidth, image.GetSizeX() );
		ASSERT_EQ( options.nHeight, image.GetSizeY() );
		for ( int y = 0; y < image.GetSizeY(); ++y )
		{
			for ( int x = 0; x < image.GetSizeX(); ++x )
			{
				EXPECT_EQ( ( y + ( 7 - options.nHeight ) / 2 ) * 100 + x + ( 9 - options.nWidth ) / 2, image[y][x] );
			}
		}
	}
}

TEST( IconCapture, ResolutionChangesPreserveTheProjectedFraming )
{
	for ( const auto &options : { NIconRendering::SOptions{ 84, 84 },
	                              NIconRendering::SOptions{ 126, 84 },
	                              NIconRendering::SOptions{ 42, 126 },
	                              NIconRendering::SOptions{ 512, 512 },
	                              NIconRendering::SOptions{ 21, 21 } } )
	{
		// Perspective x is proportional to cot(FOV/2). The same world point
		// must occupy the same fraction of the shorter icon dimension.
		const float fOriginalProjection = 1.0f / std::tan( ToRadian( 54.0f ) * 0.5f ) / 42.0f;
		const float fFOV = NIconRendering::GetCaptureFOV( 54.0f, options );
		const float fProjection =
				1.0f / std::tan( ToRadian( fFOV ) * 0.5f ) / (std::min)( options.nWidth, options.nHeight );
		EXPECT_NEAR( fOriginalProjection, fProjection, 0.000001f );
	}
}

TEST( IconCapture, InvalidDimensionsAndShortReadbackAreRejected )
{
	CArray2D<int> capture( 64, 32 );
	CArray2D<int> image;
	for ( const auto &options : { NIconRendering::SOptions{ 0, 42 },
	                              NIconRendering::SOptions{ 42, -1 },
	                              NIconRendering::SOptions{ 513, 42 },
	                              NIconRendering::SOptions{ 42, 513 },
	                              NIconRendering::SOptions{ 65, 32 },
	                              NIconRendering::SOptions{ 64, 33 } } )
	{
		EXPECT_FALSE( NIconRendering::CropCapture( &image, capture, options ) );
	}
}
