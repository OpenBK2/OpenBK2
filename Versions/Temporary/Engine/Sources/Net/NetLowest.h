#pragma once

#include "NetAddress.h"
class CMemoryStream;

namespace NNet
{

const int N_MAX_HOST_HOMES = 4;
class CNodeAddressSet
{
	unsigned short nPort;
	int ips[N_MAX_HOST_HOMES];
public:
	void Clear() { Zero(*this); }
	bool GetAddress( int n, CNodeAddress *pRes ) const;

	friend class CLinksManagerCommon;
};

}
