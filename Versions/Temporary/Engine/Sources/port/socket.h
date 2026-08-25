#pragma once

#include <boost/predef.h>

// Sockets, spelled the way this codebase already spells them.
//
// Net is Winsock by spelling and BSD sockets by substance: one non-blocking UDP
// socket, socket, bind, sendto, recvfrom, setsockopt, gethostbyname. There is no
// WSAAsyncSelect, no overlapped I/O, no completion port, not even a select. So
// the difference between the platforms is names and two argument types, and this
// header carries POSIX over to the Winsock spelling rather than rewriting the
// call sites.
//
// Deliberately not Boost.Asio. Asio would replace the I/O model rather than the
// spelling, and this is the transport under a lockstep simulation, where a change
// in when a datagram is sent or read relative to a tick is the kind of thing that
// shows up later as an ASYNC and is hard to attribute. The surface does not
// justify the risk.
//
// Keyed on the target OS rather than on the compiler, for the reason
// port/stdcall.h spells out.
#if BOOST_OS_WINDOWS
#include <winsock.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

//! The four octets of an IPv4 address, in the order a dotted quad writes them.
//!
//! Winsock reaches these through in_addr's S_un.S_un_b.s_b1 to s_b4, which POSIX
//! does not have: there in_addr holds s_addr and nothing else. s_addr is in
//! network order on both, which is the order the octets are printed in, so the
//! bytes of it are the answer on either platform. Winsock's own s_addr is a
//! compatibility macro for S_un.S_addr, so that spelling needs no help here.
inline unsigned char *AddressOctets( in_addr *pAddress )
{
	return reinterpret_cast<unsigned char *>( &pAddress->s_addr );
}

inline const unsigned char *AddressOctets( const in_addr *pAddress )
{
	return reinterpret_cast<const unsigned char *>( &pAddress->s_addr );
}

#if BOOST_OS_WINDOWS

//! The length in and out of getsockname and recvfrom. Winsock says int.
typedef int socket_length_t;

#else

//! Winsock's handle type. A descriptor everywhere else, so signed, and -1 is the
//! failure that INVALID_SOCKET names.
typedef int SOCKET;
#define INVALID_SOCKET ( -1 )

//! POSIX says socklen_t here, which is a distinct type from int even where it is
//! the same width, so the call sites name this rather than either directly.
typedef socklen_t socket_length_t;

//! A socket is a descriptor, so it closes like one.
inline int closesocket( SOCKET s )
{
	return ::close( s );
}

//! FIONBIO is the one command this codebase issues.
inline int ioctlsocket( SOCKET s, unsigned long nCommand, unsigned long *pnArgument )
{
	if ( nCommand == FIONBIO )
	{
		// The caller holds the flag in the unsigned long that Winsock's
		// signature asks for, and this ioctl reads an int. Narrow it here rather
		// than hand over four bytes of an eight byte object and rely on which
		// four the machine happens to put first.
		int nFlag = *pnArgument != 0 ? 1 : 0;
		return ::ioctl( s, FIONBIO, &nFlag );
	}
	return ::ioctl( s, nCommand, pnArgument );
}

#endif
