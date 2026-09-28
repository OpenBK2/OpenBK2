#pragma once

#include "LightXML.h"
#include "DB.h"
#include "Misc/HashFuncs.h"
#include "XmlSaver.h"

#include <cstdint>

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

	std::vector<const NXml::CXmlNode*> readChunkLevels;
	int nCurChunkLevel;

	CXMLDocument document;
	CDataStream *pDstStream;
	CXMLMultiNode *pCurrNode;
	typedef std::list<CXMLMultiNode*> CChunksList;
	CChunksList chunkLevels;
	bool bReading;
	std::string szCurrObjectPath;
	std::list<std::string> objectNamesStack;
	//
	typedef std::unordered_map<void*,CPtr<CXmlResource>> CObjectsHash;
	CObjectsHash objects;
	// The ID each stored object is written under, in its references and as its
	// __ServerPtr in SharedClasses. It used to be the object's address, cut to
	// 4 bytes, so a file differed from run to run and on x64 two objects could
	// collide. IDs are handed out 1, 2, 3... in the order objects are first
	// reached, as CStructureSaver does. A reader only matches references to
	// objects by them, so files written either way load either way.
	typedef std::unordered_map<void*,uintptr_t> CPObjectsHash;
	CPObjectsHash storedObjects;
	uintptr_t nNextObjectID = 1;			// 0 is the null pointer
	std::list< CPtr<CXmlResource> > toStore;
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
	bool DataChunk( const chunk_id idChunk, boost::uuids::uuid *pgData, int nChunkNumber );
	bool DataChunkDBID( CDBID *pDBID );
	bool DataChunkFilePath( NFile::CFilePath *pFilePath );
	bool DataChunkString( std::string &data );
	bool DataChunkString( std::wstring &data );
	//
	void ReportCurrentObject( const CDBID &dbid );
	void PushCurrentObject( const CDBID &dbid );
	void PopCurrentObject();
	const std::string &GetCurrObjectPath() const { return szCurrObjectPath; }
	//
	bool AddAttribute( const chunk_id attrName, bool *pData );
	bool AddAttribute( const chunk_id attrName, int *pData );
	bool AddAttribute( const chunk_id attrName, float *pData );
	bool AddAttribute( const chunk_id attrName, std::string *pData );
	bool AddAttribute( const chunk_id attrName, std::wstring *pData );
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


