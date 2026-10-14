#pragma once

#include "NetAddress.h"
#include "NetDriver.h"

#include "Net_export.h"

namespace NNet
{
class NET_EXPORT CLinksManagerCommon : public ILinksManager
{
	SOCKET s;
	CNodeAddress broadcastAddr;

protected:
	virtual bool IsGoodAddress( const sockaddr &addr ) const
	{
		return true;
	}

public:
	CLinksManagerCommon();
	~CLinksManagerCommon() override;
	bool Init( const int nPort );

	bool MakeBroadcastAddr( class CNodeAddress *pRes, int nPort ) const override;
	bool IsLocalAddr( const CNodeAddress &test ) const override;
	bool Send( const CNodeAddress &dst, CMemoryStream &pkt ) const override;
	bool Recv( class CNodeAddress *pSrc, CMemoryStream *pPkt ) const override;
	bool GetSelfAddress( class CNodeAddressSet *pRes ) const override;
};
} // namespace NNet
