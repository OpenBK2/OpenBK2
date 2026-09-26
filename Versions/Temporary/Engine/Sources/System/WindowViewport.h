#pragma once

#include <algorithm>
#include <cstdint>

namespace NWinFrame
{
// Share the integer presentation rectangle with input so rounding cannot move
// click targets. Game resolution remains independent of desktop resolution.
struct SWindowViewport
{
	int x = 0, y = 0, width = 0, height = 0;
	int renderWidth = 0, renderHeight = 0;

	static SWindowViewport Fit( int renderWidth, int renderHeight, int windowWidth, int windowHeight )
	{
		SWindowViewport result;
		if ( renderWidth <= 0 || renderHeight <= 0 || windowWidth <= 0 || windowHeight <= 0 )
			return result;
		result.renderWidth = renderWidth;
		result.renderHeight = renderHeight;
		result.width = windowWidth;
		result.height = windowHeight;
		if ( int64_t( windowWidth ) * renderHeight > int64_t( windowHeight ) * renderWidth )
			result.width = (std::max)( 1, int( int64_t( windowHeight ) * renderWidth / renderHeight ) );
		else
			result.height = (std::max)( 1, int( int64_t( windowWidth ) * renderHeight / renderWidth ) );
		result.x = ( windowWidth - result.width ) / 2;
		result.y = ( windowHeight - result.height ) / 2;
		return result;
	}

	void ClientToGame( float *px, float *py ) const
	{
		if ( width <= 0 || height <= 0 )
			return;
		*px = std::clamp( ( *px - x ) * renderWidth / width, 0.0f, float( renderWidth - 1 ) );
		*py = std::clamp( ( *py - y ) * renderHeight / height, 0.0f, float( renderHeight - 1 ) );
	}

	void GameToClient( float *px, float *py ) const
	{
		if ( renderWidth <= 0 || renderHeight <= 0 )
			return;
		*px = x + *px * width / renderWidth;
		*py = y + *py * height / renderHeight;
	}
};
}
