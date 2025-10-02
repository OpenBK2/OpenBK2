#include "StdAfx.h"
#include "checksumsaver.h"

#include "CheckSums.h"
#include "System/CheckSumLog.h"



hash_map<int, CObjectBase*> & GetStorage()
{
	static hash_map<int, CObjectBase*> storage;
	return storage;
}

void AddObject( int nObject, CObjectBase * p )
{
	GetStorage()[nObject] = p;
}

void CheckAIObjectBase( int nObj )
{
	hash_map<int, CObjectBase*>::iterator pos = GetStorage().find( nObj );
	if ( pos != GetStorage().end() )
	{
		CCheckSumSaver saver;
		(pos->second)->operator&( saver );
	}
}

void CheckAIObjectBase()
{
	CCheckSumSaver saver;
	for ( hash_map<int, CObjectBase*>::iterator it = GetStorage().begin(); it != GetStorage().end(); ++it )
		(it->second)->operator&( saver );
}
/*

CAIObjectBase::~CAIObjectBase()
{
	GetStorage().erase( ___nObj );
}

CAIObjectBase::CAIObjectBase()
{
	static int nObject = 0;
	AddObject( nObject++, this );
	___nObj = nObject;
}*/

static uLong nLocalCheckSum;

IBinSaver *CreateCheckSumSaver( unsigned long *pCheckSum, ICheckSumLog * pLog, const DWORD segmentTime )
{
	return new CCheckSumSaver( pCheckSum, pLog, segmentTime );
}

CCheckSumSaver::CCheckSumSaver( uLong *_pCheckSum, ICheckSumLog * pLog, const NTimer::STime segmentTime  )
: checkSumData( _pCheckSum ), checkSumString( _pCheckSum ), 
 checkSumObjects( _pCheckSum ), nAddedObjects( 0 ), bDissalowStoreObjects( false ), nCount( 0 ),
 nSegment( segmentTime ), pCommandsHistory ( pLog )
{
}

CCheckSumSaver::~CCheckSumSaver()
{
	if ( IsReading() )
		return;
	
	int nObject = 0;
	bDissalowStoreObjects = true;
	for ( CAddOrder::iterator it = addOrder.begin(); it != addOrder.end(); ++it )
	{
		// save object data
		const bool bStartChunkResult = StartChunk( (chunk_id) 1, nObject );
		ASSERT( bStartChunkResult );
		// do not store object's ptr value - differ on different hosts
		// DataChunk( 0, &pObject, 4, 1 );
		//
		if ( StartChunk( 1, 1 ) )
		{
			(*(*it))&( *this );
			FinishChunk();
		}
		FinishChunk();
		++nObject;
	}
	bDissalowStoreObjects = false;
}

CCheckSumSaver::CCheckSumSaver()
: checkSumData( &nLocalCheckSum ), checkSumString( &nLocalCheckSum ), 
checkSumObjects( &nLocalCheckSum ), nAddedObjects( 0 ), bDissalowStoreObjects( false ), nCount( 0 ),
nSegment( 0 )
{
	nLocalCheckSum = adler32( 0L, Z_NULL, 0 );
}

void CCheckSumSaver::DataChunk( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber )
{
#ifdef _DEBUG
	unsigned int * _pData = (unsigned int *)pData;
	bool bFound = false;
	if ( nSize > 2 )
	{
		for ( int i = 0; i < nSize / 4; ++i )
		{
			if ( _pData[i] == 0xcdcdcdcd || _pData[i] == 0xadadadad || _pData[i] == 0xfdfdfdfd  )
			{
				bFound = true;
				break;
			}
		}
	}
	else if ( nSize == 2 )
	{
		WORD * _pData = (WORD *)pData;
		if ( _pData[0]== 0xcdcd || _pData[0] == 0xadad|| _pData[0] == 0xfdfd )
		{
			bFound = true;
		}
	}
	else if ( nSize == 1 )
	{
		BYTE * _pData = (BYTE *)pData;
		if ( _pData[0]== 0xcd || _pData[0] == 0xad|| _pData[0] == 0xfd )
		{
			bFound = true;
		}
	}
	NI_ASSERT( nSize == 0 || !bFound, StrFmt( "Unitialized data at chunk %d", idChunk ) );
#endif

	static int nFirstCount = NGlobal::GetVar( "start_count", -1 );
	if ( nCount > nFirstCount )
	{
		static const int nBreakCount = NGlobal::GetVar( "break_count", -1 );
		static const int nSegmentBreak = NGlobal::GetVar( "break_segment", -1 );

		if ( nBreakCount == nCount && nSegment == nSegmentBreak )
		{
			DebugTrace( "break_count/break_segment reached: segment %i, count %i, data[0] = %i", nSegment, nCount, *((int*)pData) );
			__debugbreak();
		}

		*checkSumData = adler32( *checkSumData, (const BYTE*)&idChunk, sizeof(idChunk) );
		*checkSumData = adler32( *checkSumData, (const BYTE*)pData, nSize );
		//*checkSumData = adler32( *checkSumData, (const BYTE*)&nSize, sizeof(nSize) );
		//*checkSumData = adler32( *checkSumData, (const BYTE*)&nChunkNumber, sizeof(nChunkNumber) );

		if ( pCommandsHistory )
		{
			if ( !pCommandsHistory->AddChecksumLog( nSegment, *checkSumData, nCount ) )
			{
				DebugTrace( "Segment %i, count %i, data[0] = %i", nSegment, nCount, *((int*)pData) );
			}
		}


	}
	++nCount;
}

void CCheckSumSaver::DataChunkString( string &data )
{
	//*checkSumString = adler32( *checkSumString, (const BYTE*)(data.c_str()), data.size() * sizeof( string::value_type ) );
}

void CCheckSumSaver::DataChunkString( wstring &data )
{
	//z*checkSumString = adler32( *checkSumString, (const BYTE*)(data.c_str()), data.size() * sizeof( wstring::value_type ) );
}

void CCheckSumSaver::StoreObject( CObjectBase *pObject )
{
	if ( pObject == 0 || bDissalowStoreObjects ) 
		return;

	CAddedObjects::iterator pos = addedObjects.find( pObject );
	if ( pos == addedObjects.end() )
	{
		addOrder.push_back( pObject );
		++nAddedObjects;
		addedObjects[pObject] = nAddedObjects;
		pos = addedObjects.find( pObject );
	}
	const int nObj = pos->second;
	//	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE,//CONSOLE_STREAM_DEBUG_STATUS, 
	//	StrFmt( "%s, %i", typeid(*pObject).name(), nObj ), 0, true );

	static const int nBreakCount = NGlobal::GetVar( "break_count", -1 );
	static const int nSegmentBreak = NGlobal::GetVar( "segment_break", -1 );
	if ( pCommandsHistory )
		pCommandsHistory->AddChecksumLog( nSegment, *checkSumData, nCount );
	if ( nBreakCount == nCount && nSegment == nSegmentBreak )
	{
		__debugbreak();
	}
	++nCount;

	*checkSumObjects = adler32( *checkSumObjects, (const BYTE*)&(nObj), sizeof(nObj) );
}


