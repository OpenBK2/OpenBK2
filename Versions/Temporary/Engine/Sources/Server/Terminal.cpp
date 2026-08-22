#include "stdafx.h"

#include "Terminal.h"
#include "Server_Client_Common/Commands.h"
#include "Misc/Win32Helper.h"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>

const int INPUT_BUFFER_SIZE = 255;

static std::mutex csClientSocketReading;
static std::mutex csClientSocketWriting;
CObj<CTerminal> pTheTerminal;

static uint32_t WINAPI TheTerminalThreadProc( LPVOID lpParameter )
{
	CTerminal* pTerminal = reinterpret_cast<CTerminal*>(lpParameter);
	while (1)
	{
		std::this_thread::sleep_for( std::chrono::milliseconds( 30 ) );
		pTerminal->MTSegment();
	}
	return 0;
}

CTerminal::CTerminal( CCommands *_pCommands, const int _nPort ) : pCommands( _pCommands ), nPort( _nPort ), bClientIsOK( false )
{
	pTheTerminal = this;
	uint16_t sockVersion;
	WSADATA wsaData;

	sockVersion = MAKEWORD(1, 1);			// We'd like Winsock version 1.1

	// We begin by initializing Winsock
	WSAStartup( sockVersion, &wsaData );

	listeningSocket = socket(AF_INET,		// Go over TCP/IP
		SOCK_STREAM,   	// This is a stream-oriented socket
		IPPROTO_TCP );		// Use TCP rather than UDP

	if ( listeningSocket == INVALID_SOCKET ) 
	{
		NI_ASSERT( false, "Cannot create terminal socket!" );
		WSACleanup();				// Shutdown Winsock
		return;			// Return an error value
	}

	// Use a SOCKADDR_IN struct to fill in address information
	SOCKADDR_IN serverInfo;

	serverInfo.sin_family = AF_INET;
	serverInfo.sin_addr.s_addr = INADDR_ANY;	// Since this socket is listening for connections,
	// any local address will do
	serverInfo.sin_port = htons( nPort );		// Convert integer 8888 to network-byte order
	// and insert into the port field

	if  ( bind( listeningSocket, (LPSOCKADDR)&serverInfo, sizeof(struct sockaddr) ) == SOCKET_ERROR )
	{
		NI_ASSERT( false, "Cannot bind to terminal socket!" );
		WSACleanup();				// Shutdown Winsock
		return;			// Return an error value
	}

	if  ( listen( listeningSocket, 1 ) == SOCKET_ERROR )
	{
		NI_ASSERT( false, "Cannot listen on terminal socket!" );
		WSACleanup();				// Shutdown Winsock
		return;			// Return an error value
	}
	uint32_t dwThreadId;

	hReadingThread = CreateThread( 0, 1024*1024, TheTerminalThreadProc, reinterpret_cast<LPVOID>(this), 0, &dwThreadId );
}

void CTerminal::Segment()
{
	if ( !readCache.empty() )
	{
		string szError;
		string szLineEntered;
		{
			std::lock_guard lock( csClientSocketReading );
			szLineEntered = readCache.front();
			readCache.pop_front();
		}
		pCommands->LineEntered( szLineEntered, &szError );
		WriteMSG( "%s", szError.c_str() );
	}
}

void CTerminal::MTSegment()
{
	if ( !bClientIsOK )
	{
		SOCKET newSocket = accept( listeningSocket, 0, 0 );
		{
			if ( newSocket != SOCKET_ERROR )
			{
				{
					std::lock_guard lock( csClientSocketWriting );
					acceptedSocket = newSocket;
					bClientIsOK = true;
				}
				OutString( "\nConnected to Nival.Net\n" );
				while ( bClientIsOK )
				{
					std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
					ReadToCache();
				}
			}
		}
	}
}

void CTerminal::OutString( const string &szString )
{
	std::lock_guard lock( csClientSocketWriting );
	writeCache.push_back( szString );
	if ( bClientIsOK ) 
	{
		while ( !writeCache.empty() && bClientIsOK )
		{
			const string &szOutString = writeCache.front();
			int nSent = send( acceptedSocket, szOutString.c_str(), szOutString.size(), 0 );
			if ( nSent == SOCKET_ERROR )
			{
				return;
			}
			writeCache.pop_front();
		}
	}
}

void CTerminal::ReadToCache()
{
	static vector<char> buffer( INPUT_BUFFER_SIZE + 1 );
	buffer[ INPUT_BUFFER_SIZE ] = 0;
	int nBytesReceived = 0;
	while ( bClientIsOK )	
	{
		nBytesReceived = recv( acceptedSocket, &buffer[0], INPUT_BUFFER_SIZE, 0 );
		if ( nBytesReceived < 1 || nBytesReceived > INPUT_BUFFER_SIZE )
		{
			bClientIsOK = false;
			break;
		}
		buffer[ nBytesReceived ] = 0;
		{
			std::lock_guard lock( csClientSocketReading );
			readCache.push_back( &buffer[0] );
		}
	}
}

CTerminal::~CTerminal()
{
	WSACleanup();
}

void WriteMSG( const char* pszFormat, ... )
{
	if ( !pTheTerminal ) return;
	static char buff[1024];

	va_list va;
	va_start( va, pszFormat );
	vsprintf( buff, pszFormat, va );
	va_end( va );
#ifndef USE_REMOTE_CONSOLE
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, buff );
#else
	pTheTerminal->OutString( buff );
#endif
}


