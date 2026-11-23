#pragma once

#include "NetAddress.h"

#include "Net_export.h"

class CMemoryStream;

namespace NNet
{

const int N_MAX_HOST_HOMES = 4;

// The client exchanges the socket's local endpoints before peer login.
class NET_EXPORT CNodeAddressSet
{
	unsigned short nPort;
	int ips[N_MAX_HOST_HOMES];

public:
	void Clear()
	{
		Zero( *this );
	}

	bool GetAddress( int n, CNodeAddress *pRes ) const;

	friend class CLinksManagerCommon;
};

} // namespace NNet
