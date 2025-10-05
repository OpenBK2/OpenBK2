#include "stdafx.h"
#include "NetServerInfo.h"

namespace NNet
{

CServerInfoSupport::CServerInfoSupport( APPLICATION_ID _nApplicationID )
	: applicationID(_nApplicationID), fTime(0), bDoReply(false), fRequestDelay(0) 
{
}

void CServerInfoSupport::Step( float fDeltaTime )
{
	fRequestDelay -= fDeltaTime;
	fTime += fDeltaTime;
	// remove outdated information about servers
	for ( CServerInfoList::iterator i = servers.begin(); i != servers.end(); )
	{
		if ( i->fValidTimeLeft > 0 )
		{
			i->fValidTimeLeft -= fDeltaTime;
			if ( i->fValidTimeLeft <= 0 )
			{
				/*if ( book.HasAddr( (*i)->addr ) )
				{
					(*i)->fpPing = FP_INVALID_PING;
					++i;
				}
				else*/
					i = servers.erase( i );
			}
			else
				++i;
		}
		else
			++i;
	}
}

void CServerInfoSupport::ReplyServerInfoRequest( CBitStream &bits, CBitStream *pDstBits )
{
	float fReqTime;
	bits.Read( fReqTime );
	(*pDstBits).Write( fReqTime );
	(*pDstBits).Write( applicationID );
	short nSize = serverInfo.GetSize();
	(*pDstBits).Write( nSize );
	(*pDstBits).Write( serverInfo.GetBuffer(), serverInfo.GetSize() );
}

CServerInfoSupport::SServerInfo& CServerInfoSupport::GetInfo( const CNodeAddress &addr )
{
	for ( CServerInfoList::iterator i = servers.begin(); i != servers.end(); ++i )
	{
		if ( i->addr == addr )
			return *i;
	}
	SServerInfo b;
	b.addr = addr;
	servers.push_back( b );
	return servers.back();
}

void CServerInfoSupport::ProcessServerInfo( const CNodeAddress &addr, CBitStream &bits, float fServerListTimeout )
{
	APPLICATION_ID appID;
	float fReqSent;
	//
	bits.Read( fReqSent );
	bits.Read( appID );
	// check that application is correct
	if ( (appID&0xFFFFFF00) != (applicationID&0xFFFFFF00) )
		return;

	SServerInfo &info = GetInfo( addr );
	info.bWrongVersion = appID != applicationID;
	info.fPing = fTime - fReqSent;
	short nSize;
	bits.Read( nSize );
	info.info.SetSizeDiscard( nSize );
	bits.Read( info.info.GetBufferForWrite(), nSize );
	info.fValidTimeLeft = fServerListTimeout;
	/*if ( links.IsLocalAddr( addr ) )
	{
		if ( book.HasAddr( addr ) )
			pInfo->type = CCSServerInfo::ST_AddressBook;
		else
			pInfo->type = CCSServerInfo::ST_Local;
		pInfo->fpValidTimeLeft = FP_SERVER_LIST_TIMEOUT;
	}
	else
	{
		if ( book.HasAddr( addr ) )
		{
			pInfo->type = CCSServerInfo::ST_AddressBook;
			pInfo->fpValidTimeLeft = book.servers.size() * 0.5f + FP_SERVER_LIST_TIMEOUT;
		}
		else
		{
			if ( i != serverInfos.end() )
			{
				pInfo->type = CCSServerInfo::ST_Master;
				pInfo->fpValidTimeLeft = 1e38f;
			}
			else
				return;
		}
	}*/
	//
}

void CServerInfoSupport::WriteRequest( CBitStream *pBits )
{
	(*pBits).Write( fTime );
}

bool CServerInfoSupport::CanSendRequest( const CNodeAddress &broadcast, std::vector<CNodeAddress> *pDest )
{
	if ( fRequestDelay <= 0 )
	{
		fRequestDelay = 1;
		pDest->resize(0);
		pDest->push_back( broadcast );
		return true;
	}
	return false;
}

}

