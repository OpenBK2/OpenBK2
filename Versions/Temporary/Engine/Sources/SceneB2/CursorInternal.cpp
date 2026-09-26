#include "stdafx.h"

#include "CursorInternal.h"
#include "System/VFSOperations.h"
#include "System/WinFrame.h"
#include "System/WinCursor.h"

#include <fmt/format.h>

BASIC_REGISTER_CLASS( SCENEB2, ICursor );

CCursor::CCursor()
: nCurrMode( 0 ), hCurrCursor( 0 ), nCurrStep( 0 ), nStepCount( 0 ),
  bAcquired( false ), bShow( true ), bCanShow( true )
{
	Show( false );
}

CCursor::~CCursor()
{
	ClearModes();
}

void CCursor::ClearModes()
{
	for ( CModesMap::iterator it = modes.begin(); it != modes.end(); ++it )
	{
		if ( it->second != 0 )
			NWinCursor::DestroyCursor( it->second );
	}
	modes.clear();
	// hCurrCursor points into what was just freed, so it goes with them
	hCurrCursor = 0;
	nCurrStep = 0;
	nStepCount = 0;
}

void CCursor::RegisterMode( const int nMode, const std::string &szFileName )
{
//	NI_ASSERT( modes.find(nMode) == modes.end(), StrFmt("Cursor mode %d already registered", nMode) );
	if ( szFileName.empty() || szFileName == " " )
		return;

	//
	NWinCursor::TCursor hCursor = NWinCursor::LoadCursor( szFileName );
	NI_ASSERT( hCursor != 0, fmt::format("Can't load cursor \"{}\" from file", szFileName) );
	if ( hCursor != 0 ) 
	{
		modes[nMode] = hCursor;
		modesFiles[nMode] = szFileName;
	}
}

bool CCursor::SetMode( const int nMode )
{
	if ( !modes.empty() )
	{
		nCurrMode = nMode;
		CModesMap::const_iterator posCursor = modes.find( nMode );
		if ( posCursor == modes.end() )
			posCursor = modes.find( 0 );
	//	hCurrCursor = modes[eMode];
		// Only a change of cursor restarts the animation, which is what Win32
		// does when a new animated cursor is set. Testing the cursor rather than
		// the mode matters because the callers set a mode every frame, and
		// restarting on each of those would leave the animation on frame one
		// forever.
		if ( hCurrCursor != posCursor->second )
		{
			hCurrCursor = posCursor->second;
			RestartAnimation();
		}
		OnSetCursor();

		return true;
	}
	else
		return false;
}

void CCursor::RestartAnimation()
{
	nCurrStep = 0;
	nStepCount = NWinCursor::GetStepCount( hCurrCursor );
	timeNextStep = std::chrono::steady_clock::now() +
		std::chrono::milliseconds( NWinCursor::GetStepDelay( hCurrCursor, nCurrStep ) );
}

void CCursor::Step()
{
	// One step means a still cursor, and it is what every cursor reports on
	// Windows, so this is where that platform leaves.
	if ( nStepCount <= 1 || !bShow || !bCanShow )
	{
		return;
	}

	const std::chrono::steady_clock::time_point timeNow = std::chrono::steady_clock::now();
	if ( timeNow < timeNextStep )
	{
		return;
	}
	// Driven by absolute time rather than by counting frames, so the animation
	// runs at the rate the file asks for whatever the frame rate is, and a hitch
	// skips the steps it slept through instead of leaving the animation
	// permanently behind the clock. The loop terminates because a step that
	// exists is at least one jiffy long.
	while ( timeNow >= timeNextStep )
	{
		nCurrStep = ( nCurrStep + 1 ) % nStepCount;
		timeNextStep += std::chrono::milliseconds( NWinCursor::GetStepDelay( hCurrCursor, nCurrStep ) );
	}
	OnSetCursor();
}

void CCursor::OnSetCursor()
{
	if ( bCanShow )
		NWinFrame::SetCursor( bShow ? NWinCursor::GetStepFrame( hCurrCursor, nCurrStep ) : 0 );
}

void CCursor::Show( const bool _bShow )
{
	bShow = _bShow;
	if ( bShow ) 
		SetMode( nCurrMode );
	else
		NWinFrame::SetCursor( 0 );
}

void CCursor::CanShow( const bool _bCanShow )
{
	bCanShow = _bCanShow;
	if ( !bCanShow )
		Show( false );
//	else
//		OnSetCursor();
}

void CCursor::SetBounds( const int x1, const int y1, const int x2, const int y2 )
{
	rcClip.Set( x1, y1, x2, y2 );
	AcquireLocal();
}

void CCursor::AcquireLocal()
{
	// WinFrame transforms logical bounds and reapplies them after activation.
	NWinFrame::SetMouseBounds( rcClip.left, rcClip.top, rcClip.right, rcClip.bottom, bAcquired );
}

void CCursor::Acquire( const bool bAcquire )
{
	bAcquired = bAcquire;
	AcquireLocal();
}

void CCursor::SetPos( const int nX, const int nY )
{
	NWinFrame::SetMousePos( float( nX ), float( nY ) );
}

const CVec2 CCursor::GetPos() const
{
	float x = 0, y = 0;
	NWinFrame::GetMousePos( &x, &y );
	return CVec2( x, y );
}

int CCursor::operator&( IBinSaver &saver )
{
	saver.Add( 1, &nCurrMode );
	saver.Add( 2, &rcClip );
	saver.Add( 3, &bAcquired );
	saver.Add( 4, &bShow );
	saver.Add( 5, &modesFiles );
	saver.Add( 6, &bCanShow );
	//
	if ( saver.IsReading() )
	{
		// ClearModes rather than a bare clear(): the cursors registered before
		// the load are about to be registered again, and off Windows each one is
		// now a whole animation's worth of SDL cursors to leak.
		ClearModes();
		std::unordered_map<int, std::string> modesFilesCopy = modesFiles;
		modesFiles.clear();
		for ( std::unordered_map<int, std::string>::iterator iter = modesFilesCopy.begin(); iter != modesFilesCopy.end(); ++iter )
			RegisterMode( iter->first, iter->second );

		Show( bShow );
	}
	//
	return 0;
}

ICursor *CreateCursor()
{
	return new CCursor();
}

REGISTER_SAVELOAD_CLASS( SCENEB2, 0x1007AC00, CCursor )


