#include "Net/stdafx.h"

#include "Net/NetA4.h"
#include "Net/NetLowest.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

#if !BOOST_OS_WINDOWS
#include <ifaddrs.h>
#include <net/if.h>
#endif

// NetProbe: the LAN half of multiplayer, without the game.
//
// It drives the same CNetDriver and CLinksManager the game's LAN screen does
// (Client/LANClient.cpp, GameX/MPManagerModeLAN.cpp), compiled from the same
// sources, so a game list that stays empty can be reproduced and watched from
// two terminals instead of two full game instances. Every datagram the driver
// sends or receives is printed with its packet type, which is the part the
// game cannot show.
//
//   NetProbe info                       what the engine thinks the network is
//   NetProbe host  [--name S]           announce a game, accept joins, echo
//   NetProbe browse                     list games, the way the LAN screen does
//   NetProbe join  ADDR[:PORT]          log in to a host and exchange messages
//
// Options:
//   --port N        UDP port to bind; the game always binds 4201. 0 picks any
//                   free port, which lets browse and join share a machine with
//                   a host (the game cannot: its second instance fails to bind).
//   --game-port N   port requests are broadcast to and games are looked for on,
//                   default 4201
//   --broadcast A   send server-info requests to A instead of the address the
//                   engine computes, e.g. 192.168.1.255 or a host's own IP
//   --version N     application id, the game's NetGameVersion (default 1)
//   --name S        game name the host announces
//   --quiet         no per-packet trace
//   --all           trace NORMAL and NOP packets too (session traffic)

namespace
{
using namespace NNet;

const int N_LAN_PORT = 4201;  // LAN_PORT in GameX/MPManagerModeLAN.cpp
const int N_LAN_TIMEOUT = 15; // seconds, what SNetDriverConsts gets in the game

std::atomic<bool> bStopRequested( false );

extern "C" void OnStopSignal( int )
{
	bStopRequested = true;
}

const auto timeStart = std::chrono::steady_clock::now();
std::mutex logMutex;

// One line per event, stamped with seconds since start. The driver thread logs
// packets while the main thread logs messages, hence the lock.
void Log( const char *pszFormat, ... )
{
	std::lock_guard lock( logMutex );
	const double fTime = std::chrono::duration<double>( std::chrono::steady_clock::now() - timeStart ).count();
	std::printf( "[%8.3f] ", fTime );
	va_list va;
	va_start( va, pszFormat );
	std::vprintf( pszFormat, va );
	va_end( va );
	std::printf( "\n" );
	std::fflush( stdout );
}

std::string LastSocketError()
{
#if BOOST_OS_WINDOWS
	return "WSA error " + std::to_string( WSAGetLastError() );
#else
	return std::string( std::strerror( errno ) ) + " (errno " + std::to_string( errno ) + ")";
#endif
}

// Mirrors EPacket in NetA4.cpp, which is private to that file. The first byte
// of every datagram is one of these.
const char *PacketName( unsigned char cType )
{
	static const char *names[] = { "NORMAL",
	                               "LOGIN",
	                               "REQUEST_SERVER_INFO",
	                               "SERVER_INFO",
	                               "ACCEPTED",
	                               "REJECTED",
	                               "LOGOUT",
	                               "TRY_SHORTCUT",
	                               "NOP",
	                               "KICK" };
	return cType < sizeof( names ) / sizeof( names[0] ) ? names[cType] : "UNKNOWN";
}

// A links manager that forwards to the real one and prints what passes
// through it. Optionally replaces the broadcast address, which is the one
// input to server discovery the engine derives from the host name.
class CTraceLinks : public ILinksManager
{
	OBJECT_NOCOPY_METHODS( CTraceLinks );

	CPtr<ILinksManager> pLinks;
	CNodeAddress broadcastOverride;
	bool bHasBroadcastOverride = false;
	bool bTrace = true;
	bool bTraceAll = false;

	void Trace( const char *pszDir, const CNodeAddress &addr, const CMemoryStream &pkt ) const
	{
		if ( !bTrace || pkt.GetSize() == 0 )
		{
			return;
		}
		const unsigned char cType = pkt.GetBuffer()[0];
		if ( !bTraceAll && ( cType == 0 /* NORMAL */ || cType == 8 /* NOP */ ) )
		{
			return;
		}
		Log( "%s %-19s %s %s (%d bytes)",
		     pszDir,
		     PacketName( cType ),
		     pszDir[0] == 's' ? "->" : "<-",
		     addr.GetFastName().c_str(),
		     pkt.GetSize() );
	}

public:
	CTraceLinks() {}

	CTraceLinks( ILinksManager *_pLinks, bool _bTrace, bool _bTraceAll )
		: pLinks( _pLinks )
		, bTrace( _bTrace )
		, bTraceAll( _bTraceAll )
	{
	}

	void OverrideBroadcast( const CNodeAddress &addr )
	{
		broadcastOverride = addr;
		bHasBroadcastOverride = true;
	}

	bool MakeBroadcastAddr( CNodeAddress *pRes, int nPort ) const override
	{
		if ( bHasBroadcastOverride )
		{
			*pRes = broadcastOverride;
			reinterpret_cast<sockaddr_in *>( pRes->GetSockAddr() )->sin_port = htons( nPort );
			return true;
		}
		return pLinks->MakeBroadcastAddr( pRes, nPort );
	}

	bool IsLocalAddr( const CNodeAddress &test ) const override
	{
		return pLinks->IsLocalAddr( test );
	}

	bool Send( const CNodeAddress &dst, CMemoryStream &pkt ) const override
	{
		Trace( "send", dst, pkt );
		const bool bResult = pLinks->Send( dst, pkt );
		if ( !bResult )
		{
			// The engine ignores this result, so a failing sendto is otherwise silent.
			Log( "     sendto %s FAILED: %s", dst.GetFastName().c_str(), LastSocketError().c_str() );
		}
		return bResult;
	}

	bool Recv( CNodeAddress *pSrc, CMemoryStream *pPkt ) const override
	{
		const bool bResult = pLinks->Recv( pSrc, pPkt );
		if ( bResult )
		{
			Trace( "recv", *pSrc, *pPkt );
		}
		return bResult;
	}

	bool GetSelfAddress( CNodeAddressSet *pRes ) const override
	{
		return pLinks->GetSelfAddress( pRes );
	}
};

struct SOptions
{
	std::string szCommand;
	std::string szTarget; // join's ADDR[:PORT]
	std::string szBroadcast;
	std::string szName = "NetProbe game";
	int nPort = N_LAN_PORT;
	int nGamePort = N_LAN_PORT;
	int nVersion = 1;
	bool bTrace = true;
	bool bTraceAll = false;
};

int Usage()
{
	std::printf( "usage: NetProbe info|host|browse|join [ADDR[:PORT]] [--port N] [--game-port N]\n"
	             "                [--broadcast ADDR] [--version N] [--name S] [--quiet] [--all]\n"
	             "See the comment at the top of Net/probe/NetProbe.cpp for what each does.\n" );
	return 2;
}

bool ParseOptions( int argc, char **argv, SOptions *pOptions )
{
	if ( argc < 2 )
	{
		return false;
	}
	pOptions->szCommand = argv[1];
	for ( int i = 2; i < argc; ++i )
	{
		const std::string szArg = argv[i];
		const bool bHasValue = i + 1 < argc;
		if ( szArg == "--port" && bHasValue )
		{
			pOptions->nPort = std::atoi( argv[++i] );
		}
		else if ( szArg == "--game-port" && bHasValue )
		{
			pOptions->nGamePort = std::atoi( argv[++i] );
		}
		else if ( szArg == "--broadcast" && bHasValue )
		{
			pOptions->szBroadcast = argv[++i];
		}
		else if ( szArg == "--version" && bHasValue )
		{
			pOptions->nVersion = std::atoi( argv[++i] );
		}
		else if ( szArg == "--name" && bHasValue )
		{
			pOptions->szName = argv[++i];
		}
		else if ( szArg == "--quiet" )
		{
			pOptions->bTrace = false;
		}
		else if ( szArg == "--all" )
		{
			pOptions->bTraceAll = true;
		}
		else if ( szArg[0] != '-' && pOptions->szTarget.empty() )
		{
			pOptions->szTarget = szArg;
		}
		else
		{
			return false;
		}
	}
	return true;
}

std::string Ipv4ToString( const in_addr &addr )
{
	const unsigned char *pOctets = AddressOctets( &addr );
	return std::to_string( pOctets[0] ) + "." + std::to_string( pOctets[1] ) + "." + std::to_string( pOctets[2] ) + "." +
	       std::to_string( pOctets[3] );
}

// Binds the way the game does, with the port given, and reports why it could not.
ILinksManager *CreateLinks( const SOptions &options )
{
	ILinksManager *pLinks = options.nPort > 0 ? CreateServerLinksManager( options.nPort ) : CreateClientLinksManager();
	if ( pLinks == 0 )
	{
		Log( "cannot open UDP port %d: %s", options.nPort, LastSocketError().c_str() );
		if ( options.nPort == N_LAN_PORT )
		{
			Log( "another program, or another game instance, already holds it. The game binds %d in every"
			     " instance, so a second instance on the same machine fails here too.",
			     N_LAN_PORT );
		}
	}
	return pLinks;
}

int RunInfo( const SOptions &options )
{
	char szHost[1024] = "";
	gethostname( szHost, sizeof( szHost ) - 1 );
	Log( "host name            %s", szHost );
	// The engine takes h_addr_list[0] of this lookup as "the" local address and
	// derives the broadcast address from its class (LinksManager.cpp).
	if ( const hostent *pHost = gethostbyname( szHost ) )
	{
		for ( int i = 0; pHost->h_addr_list[i] != 0; ++i )
		{
			Log( "  resolves to        %s%s",
			     Ipv4ToString( *reinterpret_cast<const in_addr *>( pHost->h_addr_list[i] ) ).c_str(),
			     i == 0 ? "   <- the engine uses this one" : "" );
		}
	}
	else
	{
		Log( "  does not resolve; the engine has no broadcast address at all" );
	}

	CPtr<ILinksManager> pLinks = CreateLinks( options );
	if ( !pLinks )
	{
		return 1;
	}
	Log( "bound UDP port       %d", options.nPort );

	CNodeAddress broadcast;
	pLinks->MakeBroadcastAddr( &broadcast, options.nGamePort );
	const std::string szBroadcast = broadcast.GetFastName();
	Log( "engine broadcast     %s", szBroadcast.c_str() );

	CNodeAddressSet self;
	if ( pLinks->GetSelfAddress( &self ) )
	{
		CNodeAddress addr;
		for ( int i = 0; self.GetAddress( i, &addr ); ++i )
		{
			Log( "engine self address  %s (sent to the host at login)", addr.GetFastName().c_str() );
		}
	}
	else
	{
		Log( "engine self address  none: GetSelfAddress failed, so logins cannot be sent" );
	}

#if !BOOST_OS_WINDOWS
	// What the machine actually has, to hold the two lines above against.
	bool bBroadcastMatches = false;
	ifaddrs *pList = 0;
	if ( getifaddrs( &pList ) == 0 )
	{
		for ( const ifaddrs *p = pList; p != 0; p = p->ifa_next )
		{
			if ( p->ifa_addr == 0 || p->ifa_addr->sa_family != AF_INET )
			{
				continue;
			}
			const std::string szAddr = Ipv4ToString( reinterpret_cast<const sockaddr_in *>( p->ifa_addr )->sin_addr );
			std::string szIfBroadcast = "-";
			if ( ( p->ifa_flags & IFF_BROADCAST ) && p->ifa_broadaddr != 0 )
			{
				szIfBroadcast = Ipv4ToString( reinterpret_cast<const sockaddr_in *>( p->ifa_broadaddr )->sin_addr );
				if ( szBroadcast.rfind( szIfBroadcast + ":", 0 ) == 0 )
				{
					bBroadcastMatches = true;
				}
			}
			Log( "interface %-10s %-15s broadcast %s%s",
			     p->ifa_name,
			     szAddr.c_str(),
			     szIfBroadcast.c_str(),
			     ( p->ifa_flags & IFF_UP ) ? "" : " (down)" );
		}
		freeifaddrs( pList );
	}
	if ( !bBroadcastMatches )
	{
		Log( "WARNING: the engine broadcasts to %s, which is no interface's broadcast address. Game lists"
		     " will only ever show games on this machine. Try: NetProbe browse --broadcast <one of the above>",
		     szBroadcast.c_str() );
	}
#endif
	return 0;
}

// Driver setup shared by host, browse and join: the game's LAN client does
// exactly this (CLANClient::Create), apart from the tracing wrapper.
IDriver *CreateDriver( const SOptions &options )
{
	CPtr<ILinksManager> pRealLinks = CreateLinks( options );
	if ( !pRealLinks )
	{
		return 0;
	}
	CPtr<CTraceLinks> pLinks = new CTraceLinks( pRealLinks, options.bTrace, options.bTraceAll );
	if ( !options.szBroadcast.empty() )
	{
		CNodeAddress addr;
		if ( !addr.SetInetName( options.szBroadcast.c_str(), options.nGamePort ) )
		{
			Log( "cannot resolve --broadcast %s", options.szBroadcast.c_str() );
			return 0;
		}
		pLinks->OverrideBroadcast( addr );
	}
	CNodeAddress broadcast;
	pLinks->MakeBroadcastAddr( &broadcast, options.nGamePort );
	Log( "bound UDP port %d, broadcasting to %s, application id %d",
	     options.nPort,
	     broadcast.GetFastName().c_str(),
	     options.nVersion );

	CPtr<IDriver> pDriver = CreateNetDriver( SNetDriverConsts( N_LAN_TIMEOUT ), true );
	pDriver->Init( options.nVersion, options.nGamePort, false, pLinks );
	return pDriver.Extract();
}

const char *MessageName( IDriver::EMessage eMsg )
{
	switch ( eMsg )
	{
		case IDriver::NEW_CLIENT:
			return "NEW_CLIENT";
		case IDriver::REMOVE_CLIENT:
			return "REMOVE_CLIENT";
		case IDriver::DIRECT:
			return "DIRECT";
		case IDriver::BROADCAST:
			return "BROADCAST";
		case IDriver::SERVER_DEAD:
			return "SERVER_DEAD";
		case IDriver::KICKED:
			return "KICKED";
	}
	return "?";
}

const char *RejectName( IDriver::EReject eReject )
{
	switch ( eReject )
	{
		case IDriver::NONE:
			return "NONE";
		case IDriver::TIMEOUT:
			return "TIMEOUT";
		case IDriver::BANNED:
			return "BANNED";
		case IDriver::WRONG_VERSION:
			return "WRONG_VERSION";
		case IDriver::MAXPLAYERS_REACHED:
			return "MAXPLAYERS_REACHED";
		case IDriver::PASSWORD_FAILED:
			return "PASSWORD_FAILED";
		case IDriver::ALREADY_CONNECTED:
			return "ALREADY_CONNECTED";
		case IDriver::FORBIDDEN:
			return "FORBIDDEN";
	}
	return "?";
}

void SendText( IDriver *pDriver, int nClientID, const std::string &szText )
{
	CMemoryStream pkt;
	pkt.Write( szText.data(), static_cast<int>( szText.size() ) );
	pDriver->SendDirect( nClientID, pkt );
}

// Drains the driver's message queue. A host answers every DIRECT message with
// an echo, so a join proves the data path in both directions.
void PumpMessages( IDriver *pDriver, bool bEcho )
{
	IDriver::EMessage eMsg;
	int nClientID = 0;
	CMemoryStream pkt;
	while ( pDriver->GetMessage( &eMsg, &nClientID, 0, &pkt ) )
	{
		const std::string szText( reinterpret_cast<const char *>( pkt.GetBuffer() ), pkt.GetSize() );
		Log( "message %-13s client %d%s%s", MessageName( eMsg ), nClientID, szText.empty() ? "" : ": ", szText.c_str() );
		if ( bEcho && eMsg == IDriver::DIRECT )
		{
			SendText( pDriver, nClientID, "echo: " + szText );
		}
	}
}

void Wait()
{
	std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
}

int RunHost( const SOptions &options )
{
	CPtr<IDriver> pDriver = CreateDriver( options );
	if ( !pDriver )
	{
		return 1;
	}
	// CMPManagerModeLAN::TryToCreateGame's order: StartGame moves the driver
	// from INACTIVE (browsing) to ACTIVE, without which a login is accepted but
	// the session never starts on this side.
	pDriver->StartGame();
	// What CMPManagerModeLAN::CompileAndSendLANServerInfo announces, less the
	// map: GetGameInfo hides a game whose player count or limit is zero.
	IDriver::SGameInfo info;
	info.wszServerName = std::wstring( options.szName.begin(), options.szName.end() );
	info.nHostPort = options.nGamePort;
	info.wszMapName = L"NetProbe";
	info.nCurPlayers = 1;
	info.nMaxPlayers = 2;
	info.eGameMode = IDriver::ESGM_WAIT;
	pDriver->StartGameInfoSend( info );
	pDriver->StartNewPlayerAccept();
	Log( "hosting \"%s\"; answering server-info requests and accepting joins. Ctrl-C stops.", options.szName.c_str() );

	while ( !bStopRequested )
	{
		PumpMessages( pDriver, true );
		Wait();
	}
	return 0;
}

int RunBrowse( const SOptions &options )
{
	CPtr<IDriver> pDriver = CreateDriver( options );
	if ( !pDriver )
	{
		return 1;
	}
	Log( "searching; the driver sends a request every second. Ctrl-C stops." );

	auto lastList = std::chrono::steady_clock::now();
	while ( !bStopRequested )
	{
		Wait();
		if ( std::chrono::steady_clock::now() - lastList < std::chrono::seconds( 3 ) )
		{
			continue;
		}
		lastList = std::chrono::steady_clock::now();
		// The same call, with the same skipping, that fills the LAN screen.
		int nGames = 0;
		CNodeAddress addr;
		bool bWrongVersion = false;
		float fPing = 0;
		IDriver::SGameInfo info;
		while ( pDriver->GetGameInfo( nGames, &addr, &bWrongVersion, &fPing, &info ) )
		{
			++nGames;
			Log( "game %d: %s  \"%ls\"  map \"%ls\"  players %d/%d  ping %.0f ms%s",
			     nGames,
			     addr.GetFastName().c_str(),
			     info.wszServerName.c_str(),
			     info.wszMapName.c_str(),
			     info.nCurPlayers,
			     info.nMaxPlayers,
			     fPing * 1000,
			     bWrongVersion ? "  WRONG VERSION" : "" );
		}
		if ( nGames == 0 )
		{
			Log( "no games yet" );
		}
	}
	return 0;
}

int RunJoin( const SOptions &options )
{
	if ( options.szTarget.empty() )
	{
		return Usage();
	}
	CNodeAddress target;
	if ( !target.SetInetName( options.szTarget.c_str(), options.nGamePort ) )
	{
		Log( "cannot resolve %s", options.szTarget.c_str() );
		return 1;
	}
	CPtr<IDriver> pDriver = CreateDriver( options );
	if ( !pDriver )
	{
		return 1;
	}
	Log( "logging in to %s", target.GetFastName().c_str() );
	pDriver->ConnectGame( target, CMemoryStream() );

	IDriver::EState eLast = IDriver::CONNECTING;
	auto lastSend = std::chrono::steady_clock::now();
	int nSent = 0;
	while ( !bStopRequested )
	{
		const IDriver::EState eState = pDriver->GetState();
		if ( eState != eLast )
		{
			eLast = eState;
			if ( eState == IDriver::ACTIVE )
			{
				Log( "connected, own client id %d", pDriver->GetSelfClientID() );
			}
			else if ( eState == IDriver::INACTIVE )
			{
				Log( "login failed or session ended, reason %s", RejectName( pDriver->GetRejectReason() ) );
				return 1;
			}
		}
		PumpMessages( pDriver, false );
		if ( eState == IDriver::ACTIVE && std::chrono::steady_clock::now() - lastSend > std::chrono::seconds( 2 ) )
		{
			lastSend = std::chrono::steady_clock::now();
			// The host has client id 0, as everywhere in the LAN code.
			SendText( pDriver, 0, "hello " + std::to_string( ++nSent ) );
		}
		Wait();
	}
	return 0;
}

} // namespace

int main( int argc, char **argv )
{
	SOptions options;
	if ( !ParseOptions( argc, argv, &options ) )
	{
		return Usage();
	}
	std::signal( SIGINT, OnStopSignal );
	std::signal( SIGTERM, OnStopSignal );

	if ( options.szCommand == "info" )
	{
		return RunInfo( options );
	}
	if ( options.szCommand == "host" )
	{
		return RunHost( options );
	}
	if ( options.szCommand == "browse" )
	{
		return RunBrowse( options );
	}
	if ( options.szCommand == "join" )
	{
		return RunJoin( options );
	}
	return Usage();
}
