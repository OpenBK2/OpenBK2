#pragma once

#include <cstdint>
#include <vector>

namespace NNet
{

// One IPv4 address of this machine, as the network interfaces have it.
//
// The links manager used to find these by resolving its own host name, which
// on Windows gives the adapters' addresses but on Debian, and other systems
// whose /etc/hosts maps the host name to 127.0.1.1, gives loopback: broadcasts
// then never left the machine. Asking the interfaces is right on both.
//
// Plain integers rather than socket types, so that this header does not care
// which socket header its includer has: NetInterfaces.cpp needs winsock2.h on
// Windows, and the rest of Net is built on winsock.h, which cannot share a
// translation unit with it.
struct SInterfaceAddress
{
	uint32_t dwAddress; // network byte order, as in in_addr::s_addr
	uint32_t dwMask;    // network byte order
	bool bBroadcast;    // the interface carries broadcasts; a point-to-point link such as a VPN tunnel does not
};

// The IPv4 addresses of the interfaces that are up and connected, loopback
// excluded. Asked afresh each call, so a network that comes up while the game
// runs is found.
void GetInterfaceAddresses( std::vector<SInterfaceAddress> *pResult );

} // namespace NNet
