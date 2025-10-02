#pragma once

#include "NetPacket.h"

/** information about game */
typedef unsigned char uchar;
struct SGameInfo : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( SGameInfo )
public:
	ZDATA
		int nID;																 
		string szName;
		string szMapName;
		uchar nMapSizeX;
		uchar nMapSizeY;
		uchar nPlayers;
		uchar nMaxPlayers;
		uchar nGameType;
		uchar nTechLevel;
		bool bCanConnect;
		bool bHasPassword;
		/** empty when this packet came from server */
		string szPassword;
//		/** some specific information */
//		CPtr<class CNetPacket> pSpecificInfo;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nID); f.Add(3,&szName); f.Add(4,&szMapName); f.Add(5,&nMapSizeX); f.Add(6,&nMapSizeY); f.Add(7,&nPlayers); f.Add(8,&nMaxPlayers); f.Add(9,&nGameType); f.Add(10,&nTechLevel); f.Add(11,&bCanConnect); f.Add(12,&bHasPassword); f.Add(13,&szPassword); return 0; }

	SGameInfo() : bCanConnect( true ), nPlayers( 0 ), nMaxPlayers( 0 ), bHasPassword( false ) { }
	SGameInfo( const int _nID )
		: nID( _nID ), szName( "" ), nPlayers( 0 ), nMaxPlayers( 0 ), bCanConnect( true ), bHasPassword( false ) { }
};


