#pragma once

class CCommands;
#include <WinSock2.h>

class CTerminal : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CTerminal )

	CPtr<CCommands> pCommands;
	int nPort;
	SOCKET listeningSocket;
	SOCKET acceptedSocket;
	bool bClientIsOK;
	std::list<std::string> readCache;
	std::list<std::string> writeCache;
	HANDLE hReadingThread;
public:
	CTerminal() {}
	CTerminal( CCommands *_pCommands, const int _nPort );
	~CTerminal();
	void Segment();
	void OutString( const std::string &szString );

	void MTSegment();
	void ReadToCache();
};


