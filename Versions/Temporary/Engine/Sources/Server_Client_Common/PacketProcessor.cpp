#include "stdafx.h"

#include "PacketProcessor.h"

BASIC_REGISTER_CLASS( IPacketProcessorFun );

bool CPacketProcessorBase::ProcessPacket( CNetPacket *pPacket )
{
	const int nTypeID = NObjectFactory::GetObjectTypeID( pPacket );
	hash_map<int, CPtr<IPacketProcessorFun> >::iterator iter = packetProcessorFuns.find( nTypeID );
	if ( iter != packetProcessorFuns.end() )
	{
		IPacketProcessorFun *pProcessor = iter->second;
		return pProcessor->Process( pPacket );
	}

	return false;
}

void CPacketProcessorBase::PushPacket( CNetPacket *pPacket )
{
	packets.push_back( pPacket );
}

CNetPacket* CPacketProcessorBase::GetPacket()
{
	if ( packets.empty() )
		return 0;
	else
	{
		CPtr<CNetPacket> pPacket = packets.front();
		packets.pop_front();

		return pPacket.Extract();
	}
}

