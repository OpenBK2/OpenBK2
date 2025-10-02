#pragma once

#include "NetDriver.h"
#include "NetServerInfo.h"
#include "NetLowest.h"

namespace NNet
{

// support for server info requests and server info tracking
// support for master server, address books, etc can be added here

class CServerInfoSupport
{
public:
	struct SServerInfo
	{
		CNodeAddress addr;
		float fValidTimeLeft;   // how much time left until server info expires//TimeSinceUpdate;
		float fPing;            // ping to server in seconds
		bool bWrongVersion;     // to catch different version
		CMemoryStream info;
	};
	typedef list<SServerInfo> CServerInfoList;
private:
	CServerInfoList servers;
	float fTime, fRequestDelay;
	APPLICATION_ID applicationID;
	bool bDoReply;
	CMemoryStream serverInfo;

	SServerInfo& GetInfo( const CNodeAddress &addr );
public:
	CServerInfoSupport( APPLICATION_ID _nApplicationID );
	//
	void Init( APPLICATION_ID _applicationID ) { applicationID = _applicationID; }
	//
	void Step( float fDeltaTime );
	CServerInfoList& GetServers() { return servers; }
	// requests reply	support
	bool DoReplyRequest() const { return bDoReply; }
	void StartReply( const CMemoryStream &info ) { bDoReply = true; serverInfo = info; }
	void StopReply() { bDoReply = false; }
	// packets forming
	void ReplyServerInfoRequest( CBitStream &bits, CBitStream *pDstBits );
	void ProcessServerInfo( const CNodeAddress &addr, CBitStream &bits, float fServerListTimeout );
	void WriteRequest( CBitStream *pBits );
	// server search
	bool CanSendRequest( const CNodeAddress &broadcast, vector<CNodeAddress> *pDest );
};

}
