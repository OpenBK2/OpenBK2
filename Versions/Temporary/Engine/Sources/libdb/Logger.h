#pragma once

class CLogger
{
	bool bCheckReferences;
	list<string> stkTrace;
	CDataStream *pStream;
	CMemoryStream memoryStream;
	
	string GetStackTrace() const;
public:
	CLogger();
	~CLogger();
	
	void SetLogStream( CDataStream *_pStream );
	void PushStack( const string &szLevel );
	void PopStack();
	void WriteLog( const string & szLog, bool bAppendNL = true );
	void DumpEntireLog( CDataStream *pOutStream );
	void SetReferenceChecking( bool bCheck );
	bool ShouldCheckReferences() const { return bCheckReferences; }
	void Finalize();
};


