#include "stdafx.h"

#include "WxHost.h"

// The wxApp instance. wxIMPLEMENT_APP_NO_MAIN registers the factory that
// wxEntryStart uses; it writes no WinMain, which is the point -- MFC has one,
// and CWxHostedApp::Run runs wx's loop from it.
wxIMPLEMENT_APP_NO_MAIN( NWxHost::CWxHostApp );
