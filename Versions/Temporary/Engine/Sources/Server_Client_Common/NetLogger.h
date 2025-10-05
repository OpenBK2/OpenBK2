#pragma once

#include "Server_Client_Common_export.h"



class SERVER_CLIENT_COMMON_EXPORT CNetLogger : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CNetLogger );

	std::unordered_map<std::string, FILE*> logs;
public:
	enum { tidTypeID = 0x30134C00 };

	~CNetLogger();

	void OpenLogFile( const std::string &szNick );
	void CloseLogFile( const std::string &szNick );

	void Log( const std::string &szNick, const std::string &szLog );
};

SERVER_CLIENT_COMMON_EXPORT CNetLogger* GetNetLogger();
SERVER_CLIENT_COMMON_EXPORT const char* GetPacketInfo( class CNetPacket *pPacket );


