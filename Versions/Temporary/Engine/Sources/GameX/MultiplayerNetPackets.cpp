#include "stdafx.h"
#include "MultiplayerNetPackets.h"

#include "GameX_export.h"
REGISTER_SAVELOAD_CLASS( GAMEX, 101, CB2SlotInfoPacket );
REGISTER_SAVELOAD_CLASS( GAMEX, 102, CB2GameSpecificInfoPacket );
REGISTER_SAVELOAD_CLASS( GAMEX, 104, CB2GameRoomStartGamePacket );
REGISTER_SAVELOAD_CLASS( GAMEX, 105, CB2SuggestKickPacket );
REGISTER_SAVELOAD_CLASS( GAMEX, 106, CB2LagTimeUpdatePacket );
REGISTER_SAVELOAD_CLASS( GAMEX, 107, CB2DropPlayerAtSegmentPacket );
REGISTER_SAVELOAD_CLASS( GAMEX, 108, CB2UserPausePacket );

