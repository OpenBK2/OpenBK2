#pragma once
#include "libdb_export.h"

namespace NDBWatcherClient
{

struct IDBWatcherClient : public CObjectBase
{
	enum { tidTypeID = 0x30228BC0 };
	enum EResult
	{
		COMPLETE = 0,
		FAILED = 1,
		SERVICE_NOT_READY = 2
	};
	virtual EResult GetReferencingObjects( const std::string &szName, std::vector<CDBID> *pReferencingObjs ) = 0;
};

LIBDB_EXPORT void RegisterSingleton();

}

