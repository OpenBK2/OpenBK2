#include "stdafx.h"

#include "SplashScreen.h"

#include <boost/predef.h>

#if !BOOST_OS_WINDOWS
#include "SdlVideo.h"

#include <SDL3/SDL.h>
#endif


namespace NSplash
{
#if BOOST_OS_WINDOWS
static const char *pszSplashScreenWindowClass = "SplashScreen_WindowClass_DEFAULT";

class CSplashScreen : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CSplashScreen );
	//
	class CBitmap
	{
		CTPoint<int> size;
		BITMAP bitmapInfo;
		HBITMAP hBitmap;
		HPALETTE hPalette;
		//
		void Clear();
	public:
		CBitmap();
		~CBitmap();

		bool Load( const std::string &rszImageFileName );
		bool Draw( HDC *pHDC );
		int GetSizeX() { return bitmapInfo.bmWidth; }
		int GetSizeY() { return bitmapInfo.bmHeight; }
	};

	std::string szImageFileName;
	CBitmap bitmap;	
	HWND hWnd;
public:
	CSplashScreen();
	~CSplashScreen();

	bool Create( const std::string &rszImageFileName, bool bTopMost );
	bool IsWindow();
	bool ShowWindow( int nCmdShow );
	bool UpdateWindow();
	void Destroy();
	//
	bool Draw( HDC *pHDC );
};

LRESULT CALLBACK SplashScreenWndProc( HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam )
{
	CSplashScreen* pSplashScreen = reinterpret_cast<CSplashScreen*>( ::GetWindowLongPtr( hWnd, GWLP_USERDATA ) );
	switch ( uMsg ) 
	{
	case WM_SYSCOMMAND:
		return 1;

	case WM_PAINT:
		if ( GetUpdateRect( hWnd, 0, false ) != 0 ) 
		{
			PAINTSTRUCT ps;
			HDC hDC = ::BeginPaint( hWnd, &ps );

			if ( pSplashScreen )
				pSplashScreen->Draw( &hDC );

			::EndPaint( hWnd, &ps );
			::ValidateRect( hWnd, 0 );
		}
		break;

	default:
		return DefWindowProc( hWnd, uMsg, wParam, lParam );
	}
	return 0;
}

CSplashScreen::CBitmap::CBitmap()
: hBitmap( 0 ), hPalette( 0 )
{
	bitmapInfo.bmHeight = 0;
	bitmapInfo.bmWidth = 0;
}

CSplashScreen::CBitmap::~CBitmap()
{
	Clear();
}

void CSplashScreen::CBitmap::Clear()
{
	if ( hBitmap )
	{
		::DeleteObject( hBitmap );
		hBitmap = 0;
	}

	if ( hPalette )
	{
		::DeleteObject( hPalette );
		hPalette = 0;
	}

	bitmapInfo.bmHeight = 0;
	bitmapInfo.bmWidth = 0;
}

bool CSplashScreen::CBitmap::Load( const std::string &rszImageFileName )
{
	Clear();
	//Use LoadImage() to get the image loaded into a DIBSection
	hBitmap = (HBITMAP)::LoadImage( 0,
		rszImageFileName.c_str(),
		IMAGE_BITMAP,
		0,
		0,
		LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE );
	if ( hBitmap == 0 )
		return false;

	//Get the color depth of the DIBSection
	::GetObject( hBitmap, sizeof( BITMAP ), &bitmapInfo );

	//If the DIBSection is 256 color or less, it has a color table
	if ( ( bitmapInfo.bmBitsPixel * bitmapInfo.bmPlanes ) <= 8 )
	{
		// Create a memory DC and select the DIBSection into it
		HDC hMemDC = ::CreateCompatibleDC( 0 );
		HBITMAP hOldBitmap = (HBITMAP)::SelectObject( hMemDC, hBitmap );
		//Get the DIBSection's color table
		RGBQUAD rgb[256];
		::GetDIBColorTable( hMemDC, 0, 256, rgb );
		// Create a palette from the color tabl
		LPLOGPALETTE pLogPal = (LOGPALETTE *)malloc( sizeof( LOGPALETTE ) + ( 256 * sizeof( PALETTEENTRY ) ) );
		pLogPal->palVersion = 0x300;
		pLogPal->palNumEntries = 256;
		for ( int i = 0; i < 256 ; ++i )
		{
			pLogPal->palPalEntry[i].peRed = rgb[i].rgbRed;
			pLogPal->palPalEntry[i].peGreen = rgb[i].rgbGreen;
			pLogPal->palPalEntry[i].peBlue = rgb[i].rgbBlue;
			pLogPal->palPalEntry[i].peFlags = 0;
		}
		hPalette = ::CreatePalette( pLogPal );

		free( pLogPal );
		::SelectObject( hMemDC, hOldBitmap );
		::DeleteDC( hMemDC );
	}
	else
	{
		// It has no color table, so use a halftone palette
		HDC hRefDC = ::GetDC( 0 );
		hPalette = ::CreateHalftonePalette( hRefDC );
		::ReleaseDC( 0, hRefDC );
	}
	return true;
}


bool CSplashScreen::CBitmap::Draw( HDC *pHDC )
{
	if ( (hBitmap == 0) || (hPalette == 0) )
		return false;

	HDC hMemDC = ::CreateCompatibleDC( *pHDC );
	HBITMAP hOldBitmap = (HBITMAP)::SelectObject( hMemDC, hBitmap );
	HPALETTE hOldPalette = ::SelectPalette( *pHDC, hPalette, false );
	::RealizePalette( *pHDC );

	::BitBlt( *pHDC, 0, 0, bitmapInfo.bmWidth, bitmapInfo.bmHeight, hMemDC, 0, 0, SRCCOPY );

	::SelectObject( hMemDC, hOldBitmap );
	::SelectPalette( *pHDC, hOldPalette, FALSE );

	DeleteDC ( hMemDC );

	return true;
}

CSplashScreen::CSplashScreen() : hWnd( 0 )
{
}

CSplashScreen::~CSplashScreen()
{ 
	if ( ::IsWindow(hWnd) )
		Destroy();	
}

bool CSplashScreen::Draw( HDC *pHDC )
{
	return bitmap.Draw( pHDC );
}

bool CSplashScreen::Create( const std::string &_szImageFileName, bool bTopMost )
{
	szImageFileName = _szImageFileName;

	if ( ::IsWindow(hWnd) )
		Destroy();	

	if ( !bitmap.Load( szImageFileName ) )
		return false;

	WNDCLASSEX wcex;

	wcex.cbSize					= sizeof( WNDCLASSEX ); 
	wcex.style					= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc		= (WNDPROC)SplashScreenWndProc;
	wcex.cbClsExtra			= 0;
	wcex.cbWndExtra			= 0;
	wcex.hInstance			= ::GetModuleHandle( 0 );
	wcex.hIcon					= 0;
	wcex.hCursor				= ::LoadCursor( 0, IDC_ARROW );
	wcex.hbrBackground	= (HBRUSH)( COLOR_WINDOW + 1 );
	wcex.lpszMenuName		= 0;
	wcex.lpszClassName	= pszSplashScreenWindowClass;
	wcex.hIconSm				= 0;
	if ( !::RegisterClassEx( &wcex ) ) 
		return false;

	hWnd = CreateWindowEx( WS_EX_TOOLWINDOW  | ( bTopMost ? WS_EX_TOPMOST :  0 ),
		pszSplashScreenWindowClass,
		"",
		WS_POPUP,
		0,
		0,
		0,
		0,
		0,
		0,
		::GetModuleHandle( 0 ),
		0 );

	if ( ::IsWindow(hWnd) )
	{
		const CTPoint<int> bitmapSize( bitmap.GetSizeX(), bitmap.GetSizeY() );
		const int nScreenSizeX = GetSystemMetrics( SM_CXSCREEN );
		const int nScreenSizeY = GetSystemMetrics( SM_CYSCREEN );
		const int nX = ( nScreenSizeX - bitmapSize.x ) / 2;
		const int nY = ( nScreenSizeY - bitmapSize.y ) / 2;
		::MoveWindow( hWnd, nX, nY, bitmapSize.x, bitmapSize.y, false );
		::SetWindowLongPtr( hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( this ) );
		::UpdateWindow( hWnd ); 
	}
	return ::IsWindow( hWnd );
}

bool CSplashScreen::ShowWindow( int nCmdShow )
{
	if ( ::IsWindow(hWnd) )
	{
		::ShowWindow( hWnd, nCmdShow );
		::UpdateWindow( hWnd );
		return true;
	}
	return false;
}

bool CSplashScreen::IsWindow()
{
	return ::IsWindow( hWnd );
}

bool CSplashScreen::UpdateWindow()
{
	if ( ::IsWindow(hWnd) )
	{
		::UpdateWindow( hWnd );
		return true;
	}
	return false;
}

void CSplashScreen::Destroy()
{
	if ( ::IsWindow(hWnd) )
		DestroyWindow( hWnd );
	hWnd = 0;
	UnregisterClass( pszSplashScreenWindowClass, ::GetModuleHandle(0) );
}

CObjectBase *CreateSplashScreen( const std::string &_szImageFileName, bool bTopMost )
{
	CSplashScreen *pSplash = new CSplashScreen();
	pSplash->Create( _szImageFileName, bTopMost );
	pSplash->ShowWindow( SW_SHOW );
	if ( !pSplash->IsWindow() )
	{
		delete pSplash;
		pSplash = 0;
	}
	//
	return pSplash;
}

#else

namespace
{
// Every Win32 detail above that is not "show this image in the middle of the
// screen" is either 8bpp palette handling, which no display has needed this
// century, or the paint plumbing a window class requires. Neither has an SDL
// counterpart to find, so this is written to the same contract rather than
// translated call by call: the only entry point either version offers is
// CreateSplashScreen, and its only caller creates one and later releases it.
class CSplashScreen : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CSplashScreen );
	//
	SDL_Window *pWindow;
	SDL_Surface *pImage;
public:
	CSplashScreen(): pWindow( 0 ), pImage( 0 ) {}
	~CSplashScreen();

	bool Create( const std::string &rszImageFileName, bool bTopMost );
};

CSplashScreen::~CSplashScreen()
{
	if ( pWindow != 0 )
	{
		SDL_DestroyWindow( pWindow );
		pWindow = 0;
	}
	if ( pImage != 0 )
	{
		SDL_DestroySurface( pImage );
		pImage = 0;
	}
}

bool CSplashScreen::Create( const std::string &rszImageFileName, bool bTopMost )
{
	if ( !NSdl::EnsureVideo() )
	{
		return false;
	}
	// a filesystem path rather than a VFS one, which is what LoadImage with
	// LR_LOADFROMFILE took, and what main.cpp still passes
	pImage = SDL_LoadBMP( rszImageFileName.c_str() );
	if ( pImage == 0 )
	{
		csSystem << CC_RED << "Cannot load the splash image " << rszImageFileName.c_str()
		         << ": " << SDL_GetError() << endl;
		return false;
	}

	// Borderless for WS_POPUP, utility to stay out of the task bar the way
	// WS_EX_TOOLWINDOW did, on top only when the caller asked, and centred.
	//
	// Position goes in at creation rather than through SDL_SetWindowPosition
	// afterwards, which a window manager is free to apply late or not at all
	// once the window is mapped. It is still a request: Wayland gives clients no
	// way to place their own windows, and some X11 managers place small windows
	// themselves, so the centring is what the compositor allows.
	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetStringProperty( props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "" );
	SDL_SetNumberProperty( props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED );
	SDL_SetNumberProperty( props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED );
	SDL_SetNumberProperty( props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, pImage->w );
	SDL_SetNumberProperty( props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, pImage->h );
	SDL_SetBooleanProperty( props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, true );
	SDL_SetBooleanProperty( props, SDL_PROP_WINDOW_CREATE_UTILITY_BOOLEAN, true );
	SDL_SetBooleanProperty( props, SDL_PROP_WINDOW_CREATE_ALWAYS_ON_TOP_BOOLEAN, bTopMost );
	pWindow = SDL_CreateWindowWithProperties( props );
	SDL_DestroyProperties( props );
	if ( pWindow == 0 )
	{
		csSystem << CC_RED << "Cannot create the splash window: " << SDL_GetError() << endl;
		return false;
	}

	// Straight to the window surface: there is one static frame and no input,
	// so a renderer would buy nothing. This is the BitBlt the WM_PAINT handler
	// used to do, done once, since nothing here pumps a message loop and no
	// second paint is ever going to arrive.
	SDL_Surface *pWindowSurface = SDL_GetWindowSurface( pWindow );
	if ( pWindowSurface == 0 )
	{
		csSystem << CC_RED << "Cannot reach the splash window surface: " << SDL_GetError() << endl;
		return false;
	}
	SDL_BlitSurface( pImage, 0, pWindowSurface, 0 );
	SDL_UpdateWindowSurface( pWindow );
	// let the window reach the compositor before the caller goes back to loading
	SDL_PumpEvents();
	return true;
}
}

CObjectBase *CreateSplashScreen( const std::string &_szImageFileName, bool bTopMost )
{
	CSplashScreen *pSplash = new CSplashScreen();
	if ( !pSplash->Create( _szImageFileName, bTopMost ) )
	{
		delete pSplash;
		pSplash = 0;
	}
	//
	return pSplash;
}
#endif
}
