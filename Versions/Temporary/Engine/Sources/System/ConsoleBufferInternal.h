#pragma once

#include <cstdint>

struct SPipeMessage
{
	ZDATA
	std::wstring sz;
	uint32_t dwColor;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&sz); f.Add(3,&dwColor); return 0; }
};
struct SPipeChannel
{
	ZDATA
	std::vector<int> copyToStreams;
	std::list<SPipeMessage> msgs;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&copyToStreams); f.Add(3,&msgs); return 0; }
};

const int N_MAX_CONSOLE_LINES = 2000;
class CConsoleBuffer : public IConsoleBuffer
{
	OBJECT_NOCOPY_METHODS( CConsoleBuffer );
	//
	std::vector<SConsoleLine> lines;
	std::string szLogFileName;						// log file name
	std::unordered_map<int, SPipeChannel> pipes;
	int nSlowCompress;

	int CConsoleBuffer::operator&( IBinSaver &saver )
	{
		if ( saver.IsChecksum() )
			return 0;

		saver.Add( 7, &szLogFileName );
		saver.Add( 8, &pipes );
		saver.Add( 9, &lines );
		saver.Add( 10, &nSlowCompress );

		return 0;
	}

	~CConsoleBuffer() { DumpLog(); }
	void DumpLog();
	void CompressLines();
public:
	CConsoleBuffer() : szLogFileName("console_log.txt"), nSlowCompress(0) {}
	// write string to console's stream
	void Write( const int nStreamID, const std::wstring &szString, const uint32_t color = 0xffffffff, const bool bPersistentMsg = false );
	// write string to console's stream. doesn't support any locales - just for english text
	void WriteASCII( const int nStreamID, const char *pszString, const uint32_t color = 0xffffffff, const bool bPersistentMsg = false );
	void SetLogfile( const char *pszFilename ) { szLogFileName = pszFilename; }
	bool GetNextLine( SConsoleLine *pRes, int *pSequenceID );
	SPipeChannel &GetPipeChannel( int n ) { return pipes[n]; }
};

