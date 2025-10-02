#include "stdafx.h"
#include "DBWatcherClient.h"

#import "../XDBWatcherClient/XDBWatcherClient.tlb"

namespace NDBWatcherClient
{

class CDBWatcherClient : public IDBWatcherClient
{
	OBJECT_NOCOPY_METHODS( CDBWatcherClient )

	XDBWatcherClient::IDBWatcherClientPtr pClient;
	bool bFailed;
public:
	CDBWatcherClient()
	{
		bFailed = false;

#pragma warning( disable: 4530 )
		try
		{
			pClient = XDBWatcherClient::IDBWatcherClientPtr( __uuidof( XDBWatcherClient::CDBWatcherClient ) );
			pClient->ConnectWatcher();
		}
		catch (...)
		{
			bFailed = true;
		}
#pragma warning( default: 4530 )
	}

	virtual IDBWatcherClient::EResult GetReferencingObjects( const string &szName, vector<CDBID> *pReferencingObjs )
	{
		if ( bFailed )
			return IDBWatcherClient::EResult::FAILED;
		
#pragma warning( disable: 4530 )
		bstr_t fileName( szName.c_str() );
		try
		{
			long nRefFiles = 0;
			if ( !pClient->CollectRefFiles( fileName, &nRefFiles ) )
			{
				return IDBWatcherClient::EResult::SERVICE_NOT_READY;
			}

			for ( int i = 0; i < nRefFiles; ++i )
			{
				bstr_t refFileName = pClient->GetRef( i );			
				const char *pszRefName = refFileName;

				pReferencingObjs->push_back( CDBID( pszRefName ) );
			}
		}
		catch (...)
		{
			bFailed = true;
			return IDBWatcherClient::EResult::FAILED;
		}
#pragma warning( default: 4530 )

		return IDBWatcherClient::EResult::COMPLETE;
	}
};
void RegisterSingleton()
{
	NSingleton::RegisterSingleton( new CDBWatcherClient(), IDBWatcherClient::tidTypeID );
}

}


