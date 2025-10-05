#pragma once

#include "CommonClientState.h"

struct SCustomLobbyClientInfo
{
	ZDATA
		int nID;
		std::string szNick;
		ECommonClientState eState;
		bool bWant2ReceiveChat;
		int nGameID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nID); f.Add(3,&szNick); f.Add(4,&eState); f.Add(5,&bWant2ReceiveChat); f.Add(6,&nGameID); return 0; }
};


