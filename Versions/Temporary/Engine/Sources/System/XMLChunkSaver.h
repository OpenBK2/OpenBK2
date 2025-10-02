#pragma once

#include "LightXML.h"
#include "Db.h"
#include "../Misc/HashFuncs.h"
#include "XmlSaver.h"

namespace NXml
{
	class CXmlReader;
	class CXmlNode;
}
using namespace NLXML;

class CXMLChunkSaver : public IXmlSaver
{
	OBJECT_NOCOPY_METHODS( CXMLChunkSaver );
	//
	CObj<NXml::CXmlReader> pXmlReader;
	const NXml::CXmlNode *pReadNode;

	vector<const NXml::CXmlNode*> readChunkLevels;
	int nCurChunkLevel;

	CXMLDocument document;
	CDataStream *pDstStream;
	CXMLMultiNode *pCurrNode;
	typedef list<CXMLMultiNode*> CChunksList;
	CChunksList chunkLevels;
	bool bReading;
	string szCurrObjectPath;
	list<string> objectNamesStack;
	//
	typedef hash_map<void*,CPtr<CXmlResource>,SDefaultPtrHash> CObjectsHash;
	CObjectsHash objects;
	typedef hash_map<void*,bool,SDefaultPtrHash> CPObjectsHash;
	CPObjectsHash storedObjects;
	list< CPtr<CXmlResource> > toStore;
	//
	void PushReadChunkLevel( const NXml::CXmlNode *pNode );
	void PopReadChunkLevel();
	
	bool StartChunk( const chunk_id idChunk, int nChunkNumber );
	void FinishChunk();
	int CountChunks();

	bool DataChunk( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber );
	bool DataChunk( const chunk_id idChunk, int *pnData, int nChunkNumber );
	bool DataChunk( const chunk_id idChunk, float *pfData, int nChunkNumber );
	bool DataChunk( const chunk_id idChunk, bool *pData, int nChunkNumber );
	bool DataChunk( const chunk_id idChunk, GUID *pgData, int nChunkNumber );
	bool DataChunkDBID( CDBID *pDBID );
	bool DataChunkFilePath( NFile::CFilePath *pFilePath );
	bool DataChunkString( string &data );
	bool DataChunkString( wstring &data );
	//
	void ReportCurrentObject( const CDBID &dbid );
	void PushCurrentObject( const CDBID &dbid );
	void PopCurrentObject();
	const string &GetCurrObjectPath() const { return szCurrObjectPath; }
	//
	bool AddAttribute( const chunk_id attrName, bool *pData );
	bool AddAttribute( const chunk_id attrName, int *pData );
	bool AddAttribute( const chunk_id attrName, float *pData );
	bool AddAttribute( const chunk_id attrName, string *pData );
	bool AddAttribute( const chunk_id attrName, wstring *pData );
	//
	void StoreObject( CObjectBase *pObject );
	CObjectBase* LoadObject();
	//
	void Start( CDataStream *pStream, bool bRead );
	void Finish();

	CXMLChunkSaver() {  }
public:
	CXMLChunkSaver( CDataStream *pStream, bool bRead ) 
	{ 
		NI_ASSERT( pStream != 0, "" );
		Start( pStream, bRead );
	}
	~CXMLChunkSaver() { Finish(); }
	//
	bool IsReading() const { return bReading; }
};


