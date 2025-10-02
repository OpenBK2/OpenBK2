#include "stdafx.h"

#include "Bridge.h"
#include "SerializeOwner.h"
//DEBUG{
//DEBUG}

/*int CFullEntrenchment::operator&( IBinSaver &saver )
{
	if ( !saver.IsReading() )
	{
		int nSize = entrenchParts.size();
		saver.Add( 1, &nSize );
		int cnt = 2;
		for ( list<CEntrenchmentPart*>::iterator iter = entrenchParts.begin(); iter != entrenchParts.end(); ++iter )
			SerializeOwner( cnt++, &(*iter), &saver );
	}
	else
	{
		int nSize;
		saver.Add( 1, &nSize );

		entrenchParts.clear();

		int cnt = 2;
		for ( int i = 0; i < nSize; ++i )
		{
			CEntrenchmentPart *pPart = 0;
			SerializeOwner( cnt++, &pPart, &saver );
			entrenchParts.push_back( pPart );
		}
	}

	return 0;
}*/

int CFullBridge::SSpanLock::operator&( IBinSaver &saver )
{
	
	saver.Add( 1, &formerTiles );
	SerializeOwner( 3, &pSpan, &saver );
	saver.Add( 4, &tiles );
	return 0;
}

int CFullBridge::operator&( IBinSaver &saver )
{
	int cnt = 0;
	saver.Add( ++cnt, &bGivingDamage );

	if ( !saver.IsReading() )
	{
		int nSize = spans.size();
		saver.Add( ++cnt, &nSize );
		for ( list<CBridgeSpan*>::iterator iter = spans.begin(); iter != spans.end(); ++iter )
			SerializeOwner( ++cnt, &(*iter), &saver );
		
		nSize = projectedSpans.size();
		saver.Add( ++cnt, &nSize );
		for ( list<CBridgeSpan*>::iterator iter = projectedSpans.begin(); iter != projectedSpans.end(); ++iter )
			SerializeOwner( ++cnt, &(*iter), &saver );

	}
	else
	{
		int nSize;
		saver.Add( ++cnt, &nSize );
		spans.clear();
		vector<CBridgeSpan*> spanseVector;
		spanseVector.resize( nSize );
		
		for ( vector<CBridgeSpan*>::iterator iter = spanseVector.begin(); iter != spanseVector.end(); ++iter )
			SerializeOwner( ++cnt, &(*iter), &saver );
		spans.insert( spans.end(), spanseVector.begin(), spanseVector.end() );

		saver.Add( ++cnt, &nSize );
		projectedSpans.clear();

		for ( int i = 0; i < nSize; ++i )
		{
			CBridgeSpan *pSpan = 0;
			SerializeOwner( ++cnt, &pSpan, &saver );
			projectedSpans.push_back( pSpan );
		}
	}

	saver.Add( ++cnt, &lockedSpans );
	saver.Add( ++cnt, &nSpans );
	saver.Add( ++cnt, (CLinkObject*)this );
	saver.Add( ++cnt, &bLockingBridge );
	return 0;
}

