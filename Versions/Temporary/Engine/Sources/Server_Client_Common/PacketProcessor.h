#pragma once

#include "Server_Client_Common_export.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "NetPacket.h"
#include "ChatPackets.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define REGISTER_PACKET_PROCESSOR( FuncName )	Register( this, FuncName );
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
interface IPacketProcessorFun : public CObjectBase
{
	virtual bool Process( CNetPacket *pRawPacket ) = 0;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename TObj, typename TPacket>
class CPacketProcessorFun : public IPacketProcessorFun
{
	OBJECT_NOCOPY_METHODS( CPacketProcessorFun );

	typedef bool (TObj::*ProcessFun)( TPacket *pPacket );
	ProcessFun pfnProcessFun;
	TObj *pObj;
public:
	CPacketProcessorFun() { }
	CPacketProcessorFun( TObj *_pObj, ProcessFun _pfnProcessFun )
		: pObj( _pObj ), pfnProcessFun( _pfnProcessFun ) { }

	virtual bool Process( CNetPacket *pRawPacket )
	{
		if ( pRawPacket )
		{
			TPacket *pPacket = CastToUserObject( pRawPacket, (TPacket*)0 );
			if ( pPacket )
				return (pObj->*pfnProcessFun)( pPacket );
		}

		return false;
	}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SERVER_CLIENT_COMMON_EXPORT CPacketProcessorBase
{
	hash_map<int, CPtr<IPacketProcessorFun> > packetProcessorFuns;
	list< CPtr<CNetPacket> > packets;
protected:
	template<typename TObj, typename TPacket>
	void Register( TObj *pObj, bool (TObj::*pfnProcessFun)( TPacket *pPacket ) )
	{
		const int nTypeID = NObjectFactory::GetObjectTypeID( typeid( TPacket ) );
		NI_ASSERT( nTypeID > 0, "Unregistered save/load class" );
		NI_ASSERT( packetProcessorFuns.find( nTypeID ) == packetProcessorFuns.end(),
			StrFmt( "Packet %d already registered", nTypeID ) );

		packetProcessorFuns[nTypeID] = new CPacketProcessorFun<TObj, TPacket>( pObj, pfnProcessFun );
	}

	void PushPacket( CNetPacket *pPacket );
public:
	virtual bool ProcessPacket( CNetPacket *pPacket );
	CNetPacket* GetPacket();

	virtual bool Segment() = 0;
	virtual void ReloadConfig() {}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CPacketProcessor : virtual public CObjectBase, public CPacketProcessorBase
{
public:
	virtual bool CanBePaused() { return false; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
