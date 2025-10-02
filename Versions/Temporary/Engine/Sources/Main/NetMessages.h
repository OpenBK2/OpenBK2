#pragma once

enum ENetGameMessages
{
	// game creation phase
	//NGM_LOGIC_ID,
	//NGM_BROADCAST_PLAYER_INFO,
	//NGM_DIRECT_PLAYER_INFO,
	//NGM_PLAYER_LEFT, 
	//NGM_PLAYER_KICKED,
	//NGM_GAME_INFO,
	//NGM_GAME_STARTED,
	//NGM_ASK_FOR_NAME,
	//NGM_ANSWER_PLAYER_NAME,
	//NGM_PING,
	//NGM_GAME_IS_ALREADY_STARTED, // if new client tries to connect to already started game
	//NGM_GAME_SETTINGS_CHANGED,

	// game playing phase
	NGM_ID_SEGMENT,
	NGM_AI_CMD,
	//NGM_AI_LOGIC_CMD,
	//NGM_ID_PLAYER_INFO,
	//NGM_ID_LOGIC_ID,
	//NGM_ID_START_GAME,

	//// client commands
	//NGM_PAUSE,
	//NGM_GAME_SPEED,
	//NGM_DROP_PLAYER,
	//NGM_TIMEOUT,
	//
	//NGM_IAM_ALIVE,
	//NGM_LEFT_GAME,

	//// chat messages
	//NGM_CHAT_MESSAGE,
	//
	//// load map messages
	//NGM_SEND_ME_MAP,
	//NGM_TOTAL_PACKED_SIZE,
	//NGM_PACKED_FILE_INFO,
	//NGM_FILE_INFO,
	//NGM_PACKET,
	//NGM_FINISHED,
	//NGM_STREAM_FINISHED,
};


