#pragma once

class CCommands;

#include "port/socket.h"

#include <thread>

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
	// Detached rather than held: there is no portable way to interrupt a
	// thread parked in a blocking accept or recv, so it ends with the process.
	// The HANDLE this replaces was never closed either.
	std::thread readingThread;
public:
	CTerminal() {}
	CTerminal( CCommands *_pCommands, const int _nPort );
	~CTerminal();
	void Segment();
	void OutString( const std::string &szString );

	void MTSegment();
	void ReadToCache();
};


