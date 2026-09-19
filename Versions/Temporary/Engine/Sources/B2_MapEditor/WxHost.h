#pragma once

// The editor's entry point: wx's.
//
// This used to start wx by hand from inside MFC's CWinApp -- wxEntryStart in
// InitInstance, wx's loop from Run, wxEntryCleanup in ExitInstance -- because
// MFC owned WinMain (AfxWinMain) and its module initialisation, and a WinMain
// of our own would have dropped MFC's AfxInitialize. Nothing in the editor
// links MFC any more, so wx owns WinMain now (wxIMPLEMENT_APP, in WxHost.cpp),
// and runs the editor application from its wxApp: CEditorApp::Initialize from
// OnInit, CEditorApp::Shutdown from OnExit, which is the order CWxHostedApp
// kept -- wx up before the editor, down after it.
//
// No wx here, and none in main.cpp: a translation unit that includes wx's
// headers loses windows.h's A/W macros, and main.cpp is full of code that
// relies on them. WxHost.cpp is the one that sees wx.

#include <string>

class CEditorApp;

namespace NWxHost
{
	// The one editor application object, which main.cpp makes.
	CEditorApp& GetEditorApp();
}
