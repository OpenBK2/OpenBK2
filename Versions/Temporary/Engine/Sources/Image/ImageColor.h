#if !defined(__COMMON_TOOLS__COLORS__)
#define __COMMON_TOOLS__COLORS__
#pragma once


template<class TColor>
COLORREF GetBGRColorFromARGBColor( TColor nColor ){ return RGB( 0xFF & ( ( nColor & 0xFF0000 ) >> 16 ), 0xFF & ( ( nColor & 0xFF00 ) >> 8 ), nColor & 0xFF ); }


template<class TColor>
void UpdateARGBColorFromBGRColor( COLORREF color, TColor *pColor )
{
	if ( pColor )
	{
		( *pColor ) = ( ( *pColor ) & 0xFF000000 ) + ( ( ( color & 0xFF ) << 16 ) & 0xFF0000 ) + ( color & 0xFF00 ) + ( ( ( color & 0xFF0000 ) >> 16 ) & 0xFF );
	}
}


template<class TColor>
void MakeBGRColor( COLORREF *pnColor, TColor nRed, TColor nGreen, TColor nBlue )
{ 
	( *pnColor ) = ( ( ( nBlue & 0xFF ) << 16 ) & 0x00FF0000 ) + 
								 ( ( ( nGreen & 0xFF ) << 8 ) & 0x0000FF00 ) + 
								 ( ( nRed & 0xFF ) & 0x000000FF );
}
template<class TColor>
COLORREF MakeBGRColor( TColor nRed, TColor nGreen, TColor nBlue )
{ 
	return ( ( ( nBlue & 0xFF ) << 16 ) & 0x00FF0000 ) + 
				 ( ( ( nGreen & 0xFF ) << 8 ) & 0x0000FF00 ) + 
				 ( ( nRed & 0xFF ) & 0x000000FF );
}


inline DWORD GetARGBColorFromVec4( const CVec4 &vColor )
{
	return (
		( ( int( vColor.a * 255 ) << 24 ) & 0xFF000000 ) +
		( ( int( vColor.r * 255 ) << 16 ) & 0x00FF0000 ) +
		( ( int( vColor.g * 255 ) <<  8 ) & 0x0000FF00 ) +
		( ( int( vColor.b * 255 )       ) & 0x000000FF )
		);
}


inline COLORREF GetBGRFromVec4( const CVec4 &vColor )
{
	return (
		( ( int( vColor.b * 255 ) << 16 ) & 0x00FF0000 ) +
		( ( int( vColor.g * 255 ) <<  8 ) & 0x0000FF00 ) +
		( ( int( vColor.r * 255 )       ) & 0x000000FF )
		);
}


template<class TColor>
void GetVec4FromBGRColor( CVec4 *pvColor, TColor nColor )
{
	if ( pvColor )
	{
		pvColor->a = 0;
		pvColor->r = ( (float)GetRedFromBGRColor( nColor )   ) / 0xFF;
		pvColor->g = ( (float)GetGreenFromBGRColor( nColor ) ) / 0xFF;
		pvColor->b = ( (float)GetBlueFromBGRColor( nColor )  ) / 0xFF;
	}
}






template<class TColor>
void MakeARGBColor( TColor *pnColor, TColor nAlpha, TColor nRed, TColor nGreen, TColor nBlue )
{ 
	( *pnColor ) = ( ( ( nAlpha & 0xFF ) << 24 ) & 0xFF000000 ) + 
								 ( ( ( nRed & 0xFF ) << 16 ) & 0x00FF0000 ) + 
								 ( ( ( nGreen & 0xFF ) << 8 ) & 0x0000FF00 ) + 
								 ( ( nBlue & 0xFF ) & 0x000000FF );
}
template<class TColor>
void UpdateColorARGBColor( TColor *pnColor, TColor nRed, TColor nGreen, TColor nBlue )
{ 
	( *pnColor ) = ( ( *pnColor ) & 0xFF000000 ) + 
								 ( ( ( nRed & 0xFF ) << 16 ) & 0x00FF0000 ) + 
								 ( ( ( nGreen & 0xFF ) << 8 ) & 0x0000FF00 ) + 
								 ( ( nBlue & 0xFF ) & 0x000000FF );
}
template<class TColor>
void UpdateAlphaARGBColor( TColor *pnColor, TColor nAlpha ) { ( *pnColor ) = ( ( ( nAlpha & 0xFF ) << 24 ) + ( ( *pnColor ) & 0x00FFFFFF ) ); }
template<class TColor>
void UpdateRedARGBColor( TColor *pnColor, TColor nRed ) { ( *pnColor ) = ( ( ( nRed & 0xFF ) << 16 ) + ( ( *pnColor ) & 0xFF00FFFF ) ); }
template<class TColor>
void UpdateGreenARGBColor( TColor *pnColor, TColor nGreen ) { ( *pnColor ) = ( ( ( nGreen & 0xFF ) << 8 ) + ( ( *pnColor ) & 0xFFFF00FF ) ); }
template<class TColor>
void UpdateBlueARGBColor( TColor *pnColor, TColor nBlue ) { ( *pnColor ) = ( ( nBlue & 0xFF ) + ( ( *pnColor ) & 0xFFFFFF00 ) ); }


template<class TColor>
TColor MakeARGBColor( TColor nAlpha, TColor nRed, TColor nGreen, TColor nBlue )
{ 
	return ( ( ( nAlpha & 0xFF ) << 24 ) & 0xFF000000 ) + 
				 ( ( ( nRed & 0xFF ) << 16 ) & 0x00FF0000 ) + 
				 ( ( ( nGreen & 0xFF ) << 8 ) & 0x0000FF00 ) + 
				 ( ( nBlue & 0xFF ) & 0x000000FF );
}
template<class TColor>
TColor UpdateColorARGBColor( TColor nColor, TColor nRed, TColor nGreen, TColor nBlue )
{ 
	return ( nColor & 0xFF000000 ) + 
				 ( ( ( nRed & 0xFF ) << 16 ) & 0x00FF0000 ) + 
				 ( ( ( nGreen & 0xFF ) << 8 ) & 0x0000FF00 ) + 
				 ( ( nBlue & 0xFF ) & 0x000000FF );
}
template<class TColor>
TColor UpdateAlphaARGBColor( TColor nColor, TColor nAlpha ) { return ( ( ( nAlpha & 0xFF ) << 24 ) + ( nColor & 0x00FFFFFF ) ); }
template<class TColor>
TColor UpdateRedARGBColor( TColor nColor, TColor nRed ) { return ( ( ( nRed & 0xFF ) << 16 ) + ( nColor & 0xFF00FFFF ) ); }
template<class TColor>
TColor UpdateGreenARGBColor( TColor nColor, TColor nGreen ) { return ( ( ( nGreen & 0xFF ) << 8 ) + ( nColor & 0xFFFF00FF ) ); }
template<class TColor>
TColor UpdateBlueARGBColor( TColor nColor, TColor nBlue ) { return ( ( nBlue & 0xFF ) + ( nColor & 0xFFFFFF00 ) ); }


template<class TColor>
DWORD GetAlphaFromARGBColor( TColor nColor ) { return ( 0xFF & ( ( nColor & 0xFF000000 ) >> 24 ) ); }
template<class TColor>
DWORD GetRedFromARGBColor( TColor nColor ) { return ( 0xFF & ( ( nColor & 0x00FF0000 ) >> 16 ) ); }
template<class TColor>
DWORD GetGreenFromARGBColor( TColor nColor ) { return ( 0xFF & ( ( nColor & 0x0000FF00 ) >> 8 ) ); }
template<class TColor>
DWORD GetBlueFromARGBColor( TColor nColor ) { return ( nColor & 0x000000FF ); }


inline DWORD GetRedFromBGRColor( COLORREF color ) { return ( color & 0x0000FF ); }
inline DWORD GetGreenFromBGRColor( COLORREF color ) { return ( 0xFF & ( ( color & 0x00FF00 ) >> 8 ) ); }
inline DWORD GetBlueFromBGRColor( COLORREF color ) { return ( 0xFF & ( ( color & 0xFF0000 ) >> 16 ) ); }


template<class TColor>
void GetVec3FromARGBColor( CVec3 *pvColor, TColor nColor )
{
	if ( pvColor )
	{
		pvColor->r = ( GetRedFromARGBColor( nColor )		* 1.0f ) / 0xFF;
		pvColor->g = ( GetGreenFromARGBColor( nColor )	* 1.0f ) / 0xFF;
		pvColor->b = ( GetBlueFromARGBColor( nColor )		* 1.0f ) / 0xFF;
	}
}


template<class TColor>
void GetVec4FromARGBColor( CVec4 *pvColor, TColor nColor )
{
	if ( pvColor )
	{
		pvColor->a = ( GetAlphaFromARGBColor( nColor )	* 1.0f ) / 0xFF;
		pvColor->r = ( GetRedFromARGBColor( nColor )		* 1.0f ) / 0xFF;
		pvColor->g = ( GetGreenFromARGBColor( nColor )	* 1.0f ) / 0xFF;
		pvColor->b = ( GetBlueFromARGBColor( nColor )		* 1.0f ) / 0xFF;
	}
}
template<class TColor>
TColor GetARGBColorGradient( TColor zeroColor, TColor fullColor, float fGradient )
{
	const int nZeroAlpha = GetAlphaFromARGBColor( zeroColor );
	const int nZeroRed = GetRedFromARGBColor( zeroColor );
	const int nZeroGreen = GetGreenFromARGBColor( zeroColor );
	const int nZeroBlue = GetBlueFromARGBColor( zeroColor );
	//
	const int nFullAlpha = GetAlphaFromARGBColor( fullColor );
	const int nFullRed = GetRedFromARGBColor( fullColor );
	const int nFullGreen = GetGreenFromARGBColor( fullColor );
	const int nFullBlue = GetBlueFromARGBColor( fullColor );
	//
	return MakeARGBColor( (TColor)(int)( nZeroAlpha + ( nFullAlpha - nZeroAlpha ) * fGradient ),
												(TColor)(int)( nZeroRed + ( nFullRed - nZeroRed ) * fGradient ),
												(TColor)(int)( nZeroGreen + ( nFullGreen - nZeroGreen ) * fGradient ),
												(TColor)(int)( nZeroBlue + ( nFullBlue - nZeroBlue ) * fGradient ) );
}

#endif // #if !defined(__COMMON_TOOLS__COLORS__)
