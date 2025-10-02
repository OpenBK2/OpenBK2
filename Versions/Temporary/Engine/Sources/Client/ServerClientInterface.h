#pragma once

namespace NNet
{
	struct SNetDriverConsts;
}

interface IServerClient : public CObjectBase
{
	virtual class CNetPacket* GetPacket() = 0;
	virtual void SendPacket( class CNetPacket *pPacket ) = 0;
	virtual void SendGamePacket( class CNetPacket *pPacket, bool bBroadcast ) = 0;
	virtual void Segment() = 0;

	static IServerClient* Create( const char* pServerIPAddress, const int nNetGameVersion, const int nServerPort, const int nTimeOut );
};

