#pragma once

#include "libdb_export.h"

class LIBDB_EXPORT CLogger
{
	// Runtime defaults also cover construction before a log stream is installed.
	bool bCheckReferences = false;
	std::list<std::string> stkTrace;
	CDataStream *pStream = nullptr;
	CMemoryStream memoryStream;
	
	std::string GetStackTrace() const;
public:
	CLogger();
	~CLogger();
	
	void SetLogStream( CDataStream *_pStream );
	void PushStack( const std::string &szLevel );
	void PopStack();
	void WriteLog( const std::string & szLog, bool bAppendNL = true );
	void DumpEntireLog( CDataStream *pOutStream );
	void SetReferenceChecking( bool bCheck );
	bool ShouldCheckReferences() const { return bCheckReferences; }
	void Finalize();
};


