#include "StdAfx.h"
#include "Commands.h"
#include "ConsoleBufferInternal.h"
#include "..\Misc\StrProc.h"

#pragma warning( disable : 4530 )
#include <fstream>
using std::wofstream;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool bWriteLog = false;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Erase IML tags
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static wstring EraseTags( const wstring &szStr )
{
	wstring szRet;
	for ( int i = 0; i != szStr.npos && i < szStr.size(); )
	{
		int pos = szStr.find( '<', i );
		if ( pos != string::npos )
		{
			szRet += szStr.substr( i, pos - i );
			i = szStr.find( '>', pos );
			if ( i != string::npos )
				++i;
		}
		else
		{
			szRet += szStr.substr( i, szStr.size() );
			break;
		}
	}
	return szRet;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CConsoleBuffer
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CConsoleBuffer::WriteASCII( const int nStreamID, const char *pszString, const DWORD color, const bool bPersistentMsg )
{
	if ( pszString == 0 ) 
		return;
	Write( nStreamID, NStr::ToUnicode( pszString ), color, bPersistentMsg );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CConsoleBuffer::Write( const int nStreamID, const wstring &szString, const DWORD color, const bool bPersistentMsg )
{
	++nSlowCompress;
	if ( ( nSlowCompress & 1023 ) == 0 )
		CompressLines();
	int nID = 1;
	if ( !lines.empty() )
		nID = lines.back().nSequenceID + 1;
	SConsoleLine &l = *lines.insert( lines.end() );
	l.nSequenceID = nID;
	l.nStream = nStreamID;
	l.bPersistent = bPersistentMsg;
	l.szText = szString;
	l.dwColor= color;

	if ( bWriteLog )
	{
		static wofstream logFile;
		if ( !logFile.is_open() )
			logFile.open( "console.txt", std::ios_base::out | std::ios_base::trunc );
		if ( logFile.good() )
		{
			logFile << EraseTags( szString ).c_str() << L"\n";
			logFile.flush();
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CConsoleBuffer::GetNextLine( SConsoleLine *pRes, int *pSequenceID )
{
	int &nSeq = *pSequenceID;
	int nBest = -1;
	for ( int k = lines.size() - 1; k >= 0; --k )
	{
		if ( lines[k].nSequenceID > nSeq )
			nBest = k;
		else
			break;
	}
	if ( nBest == -1 )
		return false;
	*pRes = lines[ nBest ];
	nSeq = lines[ nBest ].nSequenceID;
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CConsoleBuffer::CompressLines()
{
	vector<bool> take;
	take.resize( lines.size(), false );
	for ( int k = 0; k < lines.size(); ++k )
	{
		if ( lines[k].bPersistent || k > lines.size() - N_MAX_CONSOLE_LINES )
			take[k] = true;
	}
	int nDst = 0;
	for ( int k = 0; k < lines.size(); ++k )
	{
		if ( take[k] )
			lines[ nDst++ ] = lines[k];
	}
	lines.resize( nDst );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CConsoleBuffer::DumpLog()
{
	bool bNeedDumpToFile = false;
	for ( int k = 0; k < lines.size(); ++k )
	{
		if ( lines[k].bPersistent )
		{
			bNeedDumpToFile = true;
			break;
		}
	}
	if ( bNeedDumpToFile == false )
		return;
	//
	FILE *file = szLogFileName.empty() ? 0 : fopen( szLogFileName.c_str(), "at" );
	if ( file )
	{
		for ( int k = 0; k < lines.size(); ++k )
		{
			if ( lines[k].bPersistent )
			{
				string sz = NStr::ToMBCS( lines[k].szText );
				fprintf( file, sz.c_str() );
			}
		}
		fclose( file );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// pipes support
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WriteToPipe( int nPipe, const string &sz, DWORD dwColor, bool bPersistentMsg )
{
	WriteToPipe( nPipe, NStr::ToUnicode( sz ), dwColor, bPersistentMsg );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void WriteToPipe( int nPipe, const wstring &sz, DWORD dwColor, bool bPersistentMsg )
{
	CConsoleBuffer *pBuffer = Singleton<CConsoleBuffer>();
	SPipeChannel &dst = pBuffer->GetPipeChannel( nPipe );
	SPipeMessage &msg = *dst.msgs.insert( dst.msgs.end() );
	msg.sz = sz;
	msg.dwColor = dwColor;
	for ( int k = 0; k < dst.copyToStreams.size(); ++k )
		pBuffer->Write( dst.copyToStreams[k], sz, dwColor, bPersistentMsg );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool GetPipeMessage( int nPipe, SPipeMessage *pRes )
{
	CConsoleBuffer *pBuffer = Singleton<CConsoleBuffer>();
	SPipeChannel &dst = pBuffer->GetPipeChannel( nPipe );
	if ( dst.msgs.empty() )
		return false;
	*pRes = dst.msgs.front();
	dst.msgs.pop_front();
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool ReadFromPipe( int nPipe, string *pRes, DWORD *pDWColor )
{
	*pRes = "";
	if ( pDWColor )
		*pDWColor = 0xffffffff;
	SPipeMessage msg;
	if ( !GetPipeMessage( nPipe, &msg ) )
		return false;
	*pRes = NStr::ToMBCS( msg.sz );
	if ( pDWColor )
		*pDWColor = msg.dwColor;
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool ReadFromPipe( int nPipe, wstring *pRes, DWORD *pDWColor )
{
	*pRes = L"";
	if ( pDWColor )
		*pDWColor = 0xffffffff;
	SPipeMessage msg;
	if ( !GetPipeMessage( nPipe, &msg ) )
		return false;
	*pRes = msg.sz;
	if ( pDWColor )
		*pDWColor = msg.dwColor;
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void SetupPipeDumpToConsole( int nSrcPipe, int nDstStream )
{
	CConsoleBuffer *pBuffer = Singleton<CConsoleBuffer>();
	SPipeChannel &dst = pBuffer->GetPipeChannel( nSrcPipe );
	dst.copyToStreams.push_back( nDstStream );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_SAVELOAD_CLASS( 0x300C8D40, CConsoleBuffer )
START_REGISTER(ConsoleBufferInternal)
	REGISTER_VAR_EX( "game_writelog", NGlobal::VarBoolHandler, &bWriteLog, false, STORAGE_NONE )
FINISH_REGISTER
