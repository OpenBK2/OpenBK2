#pragma once

namespace NDBWatcherClient
{

interface IDBWatcherClient : public CObjectBase
{
	enum { tidTypeID = 0x30228BC0 };
	enum EResult
	{
		COMPLETE = 0,
		FAILED = 1,
		SERVICE_NOT_READY = 2
	};
	virtual EResult GetReferencingObjects( const string &szName, vector<CDBID> *pReferencingObjs ) = 0;
};

void RegisterSingleton();

}

