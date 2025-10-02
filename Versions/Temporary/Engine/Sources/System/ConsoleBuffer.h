#pragma once

#include "System_export.h"


enum
{
	CONSOLE_STREAM_CONSOLE	= 2,					// feedback to console (just to display)
	
	CONSOLE_STREAM_DEBUG_WINDOW	= 10,
	// CONSOLE_STREAM_DEBUG_WINDOW +
	// +0, +1 - debug windows, scrollable texts
	// +2, +3, +4, +5 - single string windows.
};

struct IConsoleBuffer : public CObjectBase
{
	enum { tidTypeID = 0 };

	struct SConsoleLine
	{
		ZDATA
		int nStream;
		int nSequenceID;
		bool bPersistent;
		wstring szText;
		DWORD dwColor;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nStream); f.Add(3,&nSequenceID); f.Add(4,&bPersistent); f.Add(5,&szText); f.Add(6,&dwColor); return 0; }
	};
	// write string to console's stream
	virtual void Write( const int nStreamID, const wstring &szString, const DWORD color = 0xffffffff, const bool bPersistentMsg = false ) = 0;
	// write string to console's stream. doesn't support any locales - just for english text
	virtual void WriteASCII( const int nStreamID, const char *pszString, const DWORD color = 0xffffffff, const bool bPersistentMsg = false ) = 0;
	virtual void SetLogfile( const char *pszFilename ) = 0;
	virtual bool GetNextLine( SConsoleLine *pRes, int *pSequenceID ) = 0;
};

enum
{
	PIPE_WORLD_CMDS = 0,
	PIPE_SCRIPT_CMDS = 1,
	PIPE_GLOBE_CMDS = 21,
	PIPE_CONSOLE_CMDS	= 3,					// command, to parse in console
	PIPE_NET_CHAT	= 5,					// net chat (to send by network)
	PIPE_A7_MULTIPLAYER_CHECK = 7,
	PIPE_CHAT			= 4,					// chat string
};
// bPersistentMsg - if pipe dump to console is enabled this parameter will be forwarded to console
SYSTEM_EXPORT void WriteToPipe( int nPipe, const string &sz, DWORD dwColor = 0xffffffff, bool bPersistentMsg = false );
SYSTEM_EXPORT void WriteToPipe( int nPipe, const wstring &sz, DWORD dwColor = 0xffffffff, bool bPersistentMsg = false );
SYSTEM_EXPORT bool ReadFromPipe( int nPipe, string *pRes, DWORD *pDWColor );
SYSTEM_EXPORT bool ReadFromPipe( int nPipe, wstring *pRes, DWORD *pDWColor );
SYSTEM_EXPORT void SetupPipeDumpToConsole( int nSrcPipe, int nDstStream );

