#include "stdafx.h"

#include "Terminal.h"
#include "Server_Client_Common/Commands.h"

#include <chrono>
#include <cstdint>

#include <cstdarg>
#include <mutex>
#include <thread>

const int INPUT_BUFFER_SIZE = 255;

static std::mutex csClientSocketReading;
static std::mutex csClientSocketWriting;
CObj<CTerminal> pTheTerminal;

// An ordinary function now, since std::thread takes one. It also no longer
// has to be spelled in DWORD and LPVOID to match LPTHREAD_START_ROUTINE, which
// is what the tree's move to the cstdint types had quietly broken.
static void TheTerminalThreadProc( CTerminal *pTerminal )
{
	for ( ;; )
	{
		std::this_thread::sleep_for( std::chrono::milliseconds( 30 ) );
		pTerminal->MTSegment();
	}
}

CTerminal::CTerminal( CCommands *_pCommands, const int _nPort ) : pCommands( _pCommands ), nPort( _nPort ), bClientIsOK( false )
{
	pTheTerminal = this;

	// Winsock alone needs starting before a socket exists. LinksManager does
	// this for the game's own sockets in a static constructor, but that lives
	// in another module and is not guaranteed to have run, and asking twice is
	// harmless: the library counts.
#if BOOST_OS_WINDOWS
	WSADATA wsaData;
	WSAStartup( MAKEWORD( 1, 1 ), &wsaData );
#endif

	listeningSocket = socket( AF_INET,	// Go over TCP/IP
		SOCK_STREAM,	// This is a stream-oriented socket
		IPPROTO_TCP );	// Use TCP rather than UDP

	if ( listeningSocket == INVALID_SOCKET )
	{
		WriteMSG( "Terminal: cannot create the socket, the remote console will not be available\n" );
		return;
	}

	// Use a sockaddr_in struct to fill in address information
	sockaddr_in serverInfo;
	memset( &serverInfo, 0, sizeof( serverInfo ) );

	serverInfo.sin_family = AF_INET;
	serverInfo.sin_addr.s_addr = INADDR_ANY;	// Since this socket is listening for connections,
	// any local address will do
	serverInfo.sin_port = htons( nPort );		// Convert integer 8888 to network-byte order
	// and insert into the port field

	// sizeof( serverInfo ), not sizeof( sockaddr ): they are the same size for
	// AF_INET, but the length belongs to the structure being passed.
	if ( bind( listeningSocket, reinterpret_cast<sockaddr *>( &serverInfo ), sizeof( serverInfo ) ) < 0 )
	{
		WriteMSG( "Terminal: cannot bind port %d, the remote console will not be available\n", nPort );
		closesocket( listeningSocket );
		listeningSocket = INVALID_SOCKET;
		return;
	}

	if ( listen( listeningSocket, 1 ) < 0 )
	{
		WriteMSG( "Terminal: cannot listen on port %d, the remote console will not be available\n", nPort );
		closesocket( listeningSocket );
		listeningSocket = INVALID_SOCKET;
		return;
	}
	readingThread = std::thread( &TheTerminalThreadProc, this );
	readingThread.detach();
}

void CTerminal::Segment()
{
	if ( !readCache.empty() )
	{
		std::string szError;
		std::string szLineEntered;
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
			if ( newSocket != INVALID_SOCKET )
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

void CTerminal::OutString( const std::string &szString )
{
	std::lock_guard lock( csClientSocketWriting );
	writeCache.push_back( szString );
	if ( bClientIsOK ) 
	{
		while ( !writeCache.empty() && bClientIsOK )
		{
			const std::string &szOutString = writeCache.front();
			int nSent = send( acceptedSocket, szOutString.c_str(), szOutString.size(), 0 );
			if ( nSent < 0 )
			{
				return;
			}
			writeCache.pop_front();
		}
	}
}

void CTerminal::ReadToCache()
{
	static std::vector<char> buffer( INPUT_BUFFER_SIZE + 1 );
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
	if ( listeningSocket != INVALID_SOCKET )
	{
		closesocket( listeningSocket );
	}
#if BOOST_OS_WINDOWS
	WSACleanup();
#endif
}

// Everything the server has to say goes through here.
//
// It used to say it only down the terminal socket, and only once CTerminal
// existed, which is the last thing CGameServer's constructor builds. So every
// message about starting up was discarded before it could be read: the port it
// bound, the database it connected to, and "Cannot bind socket to the port"
// when it did not. Running the server in a console produced no output at all,
// ever, and the only way to see anything was to connect to the terminal port
// afterwards and wait for something new to happen.
//
// So stdout first and unconditionally, then the terminal if there is one. The
// remote console keeps working exactly as it did; it is no longer the only
// reader.
void WriteMSG( const char* pszFormat, ... )
{
	char buff[1024];

	va_list va;
	va_start( va, pszFormat );
	vsprintf( buff, pszFormat, va );
	va_end( va );

	fputs( buff, stdout );
	fflush( stdout );

	if ( pTheTerminal )
	{
		pTheTerminal->OutString( buff );
	}
}


