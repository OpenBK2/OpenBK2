#include "stdafx.h"

#include "CommonPackets.h"
#include "CustomLobbyPackets.h"
#include "LadderLobbyPackets.h"
#include "GamePackets.h"
#include "LoginPackets.h"
#include "ZipPacket.h"

REGISTER_SAVELOAD_CLASS( 1, CLoginPacket );
REGISTER_SAVELOAD_CLASS( 2, CRegisterPacket );
REGISTER_SAVELOAD_CLASS( 3, CCheckConnectPacket );
REGISTER_SAVELOAD_CLASS( 4, CCheckConnectAnswerPacket );

REGISTER_SAVELOAD_CLASS( 5, CConnectGamePacket );
REGISTER_SAVELOAD_CLASS( 6, CAnswerConnectGame );
REGISTER_SAVELOAD_CLASS( 7, CNewGameConnectingClient );
REGISTER_SAVELOAD_CLASS( 8, CGameConnectingClientAccepted );
REGISTER_SAVELOAD_CLASS( 9, CConnectGameFailed );
REGISTER_SAVELOAD_CLASS( 10, CWant2Connect2Client );
REGISTER_SAVELOAD_CLASS( 11, CClientGameConnectInfo );
REGISTER_SAVELOAD_CLASS( 12, CClientWantToConnect );
REGISTER_SAVELOAD_CLASS( 13, CGameClientRemoved );
REGISTER_SAVELOAD_CLASS( 14, CIndentityPacket );
REGISTER_SAVELOAD_CLASS( 15, CNewGameClient );
REGISTER_SAVELOAD_CLASS( 16, CGameKilled );

REGISTER_SAVELOAD_CLASS( 17, CZipPacket );

REGISTER_SAVELOAD_CLASS( 18, CLeaveGamePacket );

REGISTER_SAVELOAD_CLASS( 19, CUpdateGameInfo );
REGISTER_SAVELOAD_CLASS( 20, CLobbyGamesPacket );
REGISTER_SAVELOAD_CLASS( 21, CCustomLobbyClientsPacket );

REGISTER_SAVELOAD_CLASS( 22, CNetNewClient );
REGISTER_SAVELOAD_CLASS( 23, CNetRemoveClient );
REGISTER_SAVELOAD_CLASS( 24, CConnectServerPacket );
REGISTER_SAVELOAD_CLASS( 25, CEnterLobbyPacket );
REGISTER_SAVELOAD_CLASS( 26, CLeaveLobbyPacket );

REGISTER_SAVELOAD_CLASS( 27, CChatPacket );
REGISTER_SAVELOAD_CLASS( 28, CChatAFKPacket );
REGISTER_SAVELOAD_CLASS( 29, CChatAFKResponsePacket );

REGISTER_SAVELOAD_CLASS( 30, CCommonClientStatePacket );
REGISTER_SAVELOAD_CLASS( 31, CMyIDPacket );
REGISTER_SAVELOAD_CLASS( 32, CGetLobbyClientsPacket );
REGISTER_SAVELOAD_CLASS( 33, CGameHeartBeatPacket );
REGISTER_SAVELOAD_CLASS( 34, CKillGamePacket )
REGISTER_SAVELOAD_CLASS( 35, CGetLobbyGamesPacket )
REGISTER_SAVELOAD_CLASS( 36, CCreateGamePacket );

REGISTER_SAVELOAD_CLASS( 37, CConnectedGameID );
REGISTER_SAVELOAD_CLASS( 38, CGameKickClient );
REGISTER_SAVELOAD_CLASS( 39, CGameClientWasKicked );
REGISTER_SAVELOAD_CLASS( 40, CDirectPacketToClient );
REGISTER_SAVELOAD_CLASS( 41, CTestDirectPacket );
REGISTER_SAVELOAD_CLASS( 42, CSpecificGameInfo );
REGISTER_SAVELOAD_CLASS( 43, CThroughServerConnectionPacket );
REGISTER_SAVELOAD_CLASS( 44, CGameClientDead );
REGISTER_SAVELOAD_CLASS( 45, CThroughServerGamePacket );

REGISTER_SAVELOAD_CLASS( 46, CGameTestBroadcastMsg );
REGISTER_SAVELOAD_CLASS( 47, CGameTestDirectMsg );

REGISTER_SAVELOAD_CLASS( 48, CPingPacket );

REGISTER_SAVELOAD_CLASS( 49, CLadderInfoPacket );
REGISTER_SAVELOAD_CLASS( 50, CLadderGameResultPacket );
REGISTER_SAVELOAD_CLASS( 51, CLadderInvitePacket ); 
REGISTER_SAVELOAD_CLASS( 52, CLadderStatisticsRequestPacket );
REGISTER_SAVELOAD_CLASS( 53, CLadderStatisticsPacket );

REGISTER_SAVELOAD_CLASS( 54, CChatChannelPacket );
REGISTER_SAVELOAD_CLASS( 55, CChatChannelClientsListPacket );
REGISTER_SAVELOAD_CLASS( 56, CChatClientListChangeNotifyPacket );
REGISTER_SAVELOAD_CLASS( 57, CChatChannelsListRequestPacket );
REGISTER_SAVELOAD_CLASS( 58, CChatChannelsListPacket );
REGISTER_SAVELOAD_CLASS( 59, CChatModifyIgnoreFriendListPacket );
REGISTER_SAVELOAD_CLASS( 60, CChatIgnoreFriendListPacket );
REGISTER_SAVELOAD_CLASS( 61, CChatGetIgnoreFriendListPacket );
REGISTER_SAVELOAD_CLASS( 62, CChatChannelByNickPacket );

REGISTER_SAVELOAD_CLASS( 63, CForgottenPasswordPacket );
REGISTER_SAVELOAD_CLASS( 64, CForgottenPasswordAnswerPacket );

REGISTER_SAVELOAD_CLASS( 65, CSystemBroadcastPacket );
REGISTER_SAVELOAD_CLASS( 66, CChatFriendNotifyPacket );

REGISTER_SAVELOAD_CLASS( 67, CGameStartLoadingPacket )
REGISTER_SAVELOAD_CLASS( 69, CLadderShortStatisticsPacket ) 
REGISTER_SAVELOAD_CLASS( 70, CLadderSurrenderPacket )
REGISTER_SAVELOAD_CLASS( 71, CLadderInvalidStatisticsPacket )

REGISTER_SAVELOAD_CLASS( 0x19245C01, SGameInfo );

