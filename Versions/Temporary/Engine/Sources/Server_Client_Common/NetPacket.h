#pragma once

class CNetPacket : public CObjectBase
{
public:	
	int nClientID;

	CNetPacket() { }
	CNetPacket( const int _nClientID ) : nClientID( _nClientID ) { }

	// every net packet should have operator&
	virtual int operator&( IBinSaver &f ) = 0;
};

// id of not parced packet stored as a raw memory stream
#define UNKNOWN_PACKET_TYPE_ID 0x30136400

