#pragma once

#include "Net/LinksManager.h"

namespace NNet
{

class CGameLinksManager : public CLinksManagerCommon
{
	OBJECT_NOCOPY_METHODS( CGameLinksManager );

	std::list<sockaddr> bannedAddresses;
protected:
	virtual bool IsGoodAddress( const sockaddr &addr ) const;
public:
	void BanAddress( const sockaddr &sockAddr );
};

}


