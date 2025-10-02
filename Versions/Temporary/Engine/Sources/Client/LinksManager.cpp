#include "stdafx.h"

#include "LinksManager.h"

namespace NNet
{

bool CGameLinksManager::IsGoodAddress( const sockaddr &addr ) const
{
	bool bGood = true;
	list<sockaddr>::const_iterator iter = bannedAddresses.begin();
	while ( iter != bannedAddresses.end() && bGood )
	{
		const sockaddr &bannedAddr = *iter;
		bGood = memcmp( &addr, &bannedAddr, sizeof(addr) ) != 0;

		++iter;
	}

	return bGood;
}

void CGameLinksManager::BanAddress( const sockaddr &sockAddr )
{
	bannedAddresses.push_back( sockAddr );
}

}

