#ifndef __MULTIPLAYER_NET_PACKETS_H__
#define __MULTIPLAYER_NET_PACKETS_H__

#pragma ONCE

#include "../Server_Client_Common/NetPacket.h"
#include "MPInterfaceData.h"


class CB2SlotInfoPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CB2SlotInfoPacket );
public:
	ZDATA
	int nSlot;
	SMPSlot info;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nSlot); f.Add(3,&info); return 0; }

	CB2SlotInfoPacket() {}
	CB2SlotInfoPacket( int nClientID, int _nSlot, const SMPSlot &_info ) : CNetPacket( nClientID ), nSlot(_nSlot) { info = _info; }
};

class CB2GameSpecificInfoPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CB2GameSpecificInfoPacket );
public:
	ZDATA
	SB2GameSpecificData info;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&info); return 0; }

	CB2GameSpecificInfoPacket() {}
};

class CB2GameRoomStartGamePacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CB2GameRoomStartGamePacket );
public:
	struct SShortSlotInfo
	{
		ZDATA
		BYTE nCountry;
		BYTE nTeam;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nCountry); f.Add(3,&nTeam); return 0; }
	};
	ZDATA
	vector<SShortSlotInfo> slots;
	vector<BYTE> slotRehash;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&slots); f.Add(3,&slotRehash); return 0; }

	CB2GameRoomStartGamePacket() {}
	CB2GameRoomStartGamePacket( int nClientID ) :	CNetPacket( nClientID ) {}
};

class CB2SuggestKickPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CB2SuggestKickPacket );
public:
	ZDATA
	BYTE nSlotToKick;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nSlotToKick); return 0; }

	CB2SuggestKickPacket() {}
	CB2SuggestKickPacket( int nClientID, int nSlot ) : CNetPacket( nClientID ), nSlotToKick( nSlot ) {}
};

class CB2LagTimeUpdatePacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CB2LagTimeUpdatePacket );
public:
	ZDATA
	int nPlayer;
	int nTimeLeft;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nPlayer); f.Add(3,&nTimeLeft); return 0; }

	CB2LagTimeUpdatePacket() {}
	CB2LagTimeUpdatePacket( int nClientID, int _nPlayer, int _nTimeLeft ) : CNetPacket( nClientID ), nPlayer( _nPlayer ), nTimeLeft( _nTimeLeft ) {}
};

#endif //__MULTIPLAYER_NET_PACKETS_H__

