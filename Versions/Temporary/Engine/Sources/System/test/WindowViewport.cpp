#include "../WindowViewport.h"

#include <gtest/gtest.h>

using NWinFrame::SWindowViewport;

TEST( WindowViewport, UpscalesFourByThreeWithPillarboxing )
{
	const auto viewport = SWindowViewport::Fit( 1024, 768, 1920, 1080 );
	EXPECT_EQ( viewport.x, 240 );
	EXPECT_EQ( viewport.y, 0 );
	EXPECT_EQ( viewport.width, 1440 );
	EXPECT_EQ( viewport.height, 1080 );
}

TEST( WindowViewport, DownscalesWithoutCropping )
{
	const auto viewport = SWindowViewport::Fit( 3840, 2160, 1920, 1080 );
	EXPECT_EQ( viewport.x, 0 );
	EXPECT_EQ( viewport.y, 0 );
	EXPECT_EQ( viewport.width, 1920 );
	EXPECT_EQ( viewport.height, 1080 );
}

TEST( WindowViewport, WiderGameUsesLetterboxing )
{
	const auto viewport = SWindowViewport::Fit( 1920, 1080, 1280, 1024 );
	EXPECT_EQ( viewport.x, 0 );
	EXPECT_EQ( viewport.y, 152 );
	EXPECT_EQ( viewport.width, 1280 );
	EXPECT_EQ( viewport.height, 720 );
}

TEST( WindowViewport, PortraitDisplayAndUltrawideDisplay )
{
	auto viewport = SWindowViewport::Fit( 768, 1024, 1080, 1920 );
	EXPECT_EQ( viewport.x, 0 );
	EXPECT_EQ( viewport.y, 240 );
	EXPECT_EQ( viewport.width, 1080 );
	EXPECT_EQ( viewport.height, 1440 );
	viewport = SWindowViewport::Fit( 1920, 1080, 3440, 1440 );
	EXPECT_EQ( viewport.x, 440 );
	EXPECT_EQ( viewport.y, 0 );
	EXPECT_EQ( viewport.width, 2560 );
	EXPECT_EQ( viewport.height, 1440 );
}

TEST( WindowViewport, MouseHitsImageCenterAndClampsAtBars )
{
	const auto viewport = SWindowViewport::Fit( 1024, 768, 1920, 1080 );
	float x = 960, y = 540;
	viewport.ClientToGame( &x, &y );
	EXPECT_FLOAT_EQ( x, 512 );
	EXPECT_FLOAT_EQ( y, 384 );
	x = 100;
	y = -20;
	viewport.ClientToGame( &x, &y );
	EXPECT_FLOAT_EQ( x, 0 );
	EXPECT_FLOAT_EQ( y, 0 );
	x = 1900;
	y = 1080;
	viewport.ClientToGame( &x, &y );
	EXPECT_FLOAT_EQ( x, 1023 );
	EXPECT_FLOAT_EQ( y, 767 );
}

TEST( WindowViewport, WarpAndPollingUseTheSameTransform )
{
	for ( const auto viewport : {
		SWindowViewport::Fit( 1024, 768, 1920, 1080 ),
		SWindowViewport::Fit( 3840, 2160, 1920, 1080 ),
		SWindowViewport::Fit( 1920, 1080, 1280, 1024 ),
		SWindowViewport::Fit( 1024, 768, 1365, 767 ) } )
	{
		for ( const float fraction : { 0.0f, 0.25f, 0.5f, 0.75f } )
		{
			const float originalX = fraction * viewport.renderWidth;
			const float originalY = fraction * viewport.renderHeight;
			float x = originalX, y = originalY;
			viewport.GameToClient( &x, &y );
			viewport.ClientToGame( &x, &y );
			EXPECT_NEAR( x, originalX, 0.001f );
			EXPECT_NEAR( y, originalY, 0.001f );
		}
	}
}

TEST( WindowViewport, OddSizesStayCenteredAndInsideTheWindow )
{
	const auto viewport = SWindowViewport::Fit( 1024, 768, 1365, 767 );
	EXPECT_EQ( viewport.width, 1022 );
	EXPECT_EQ( viewport.height, 767 );
	EXPECT_EQ( viewport.x, 171 );
	EXPECT_EQ( viewport.y, 0 );
	EXPECT_LE( viewport.x + viewport.width, 1365 );
}

TEST( WindowViewport, NativeResolutionHasIdentityCoordinates )
{
	const auto viewport = SWindowViewport::Fit( 1920, 1080, 1920, 1080 );
	float x = 640, y = 360;
	viewport.ClientToGame( &x, &y );
	EXPECT_FLOAT_EQ( x, 640 );
	EXPECT_FLOAT_EQ( y, 360 );
	viewport.GameToClient( &x, &y );
	EXPECT_FLOAT_EQ( x, 640 );
	EXPECT_FLOAT_EQ( y, 360 );
}

TEST( WindowViewport, MinimizedAndUninitializedSizesAreSafe )
{
	for ( const auto viewport : {
		SWindowViewport::Fit( 1024, 768, 0, 0 ),
		SWindowViewport::Fit( 0, 0, 1920, 1080 ),
		SWindowViewport::Fit( -1, 768, 1920, 1080 ) } )
	{
		EXPECT_EQ( viewport.width, 0 );
		EXPECT_EQ( viewport.height, 0 );
		float x = 5, y = 6;
		viewport.ClientToGame( &x, &y );
		viewport.GameToClient( &x, &y );
		EXPECT_FLOAT_EQ( x, 5 );
		EXPECT_FLOAT_EQ( y, 6 );
	}
}
