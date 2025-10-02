
#pragma once

class CMemoryConsumeCheck
{
	typedef hash_map<int /*nConsumeID*/, MEMORYSTATUS > CRememberedStatus;
	CRememberedStatus rememberedStatus;
	
	typedef hash_map<int /*nConsumeID*/, double /*memory used*/> CMemoryUsed;
	CMemoryUsed memoryUsed;
	CMemoryUsed memoryUsedSummed;

	NTimer::STime printTime;


	void PrintCounters()
	{
		IConsoleBuffer *pConsoleBuffer = Singleton<IConsoleBuffer>();
		pConsoleBuffer->WriteASCII( 11, "Memory consuming:", 0, true );

		for ( CMemoryUsed::iterator it = memoryUsed.begin(); it != memoryUsed.end(); ++it )
		{
			memoryUsedSummed[it->first] += it->second;
			pConsoleBuffer->WriteASCII( 11, StrFmt( "%d = %f Kbytes/sec, Total = %f", it->first, it->second / 20 / 1024, memoryUsedSummed[it->first] / 20 / 1024 ), 0, true );
			it->second = 0;
		}	
		pConsoleBuffer->WriteASCII( 11, "=================================================", 0, true );
	}

public:

	CMemoryConsumeCheck::CMemoryConsumeCheck()
	: printTime ( 0 )
	{
	}


	void Check( const int nConsumeID, const bool bStart )
	{
		if ( bStart )
		{
			rememberedStatus[nConsumeID].dwLength = sizeof( rememberedStatus[nConsumeID] );
			GlobalMemoryStatus( &rememberedStatus[nConsumeID] );
		}
		else
		{
			MEMORYSTATUS status;
			status.dwLength = sizeof( status );
			GlobalMemoryStatus( &status );
			
			if ( memoryUsed.find( nConsumeID ) == memoryUsed.end() )
				memoryUsed[nConsumeID] = 0;
			
			if ( rememberedStatus.find( nConsumeID ) == rememberedStatus.end() )
				rememberedStatus[nConsumeID] = status;
			
			const double fFormerFree = rememberedStatus[nConsumeID].dwAvailPhys;
			const double fNewFree = status.dwAvailPhys;
			memoryUsed[nConsumeID] +=  fFormerFree - fNewFree;

			const NTimer::STime currentTime = curTime;
			if ( currentTime  > printTime )
			{
				printTime = currentTime + 20000;
				PrintCounters();
			}
		}
	}
};


