#pragma once

#include "DB.h"
#include "LightXML.h"
#include "Misc/HashFuncs.h"
#include "XmlSaver.h"

#include <cstdint>

namespace NXml
{
class CXmlReader;
class CXmlNode;
} // namespace NXml

class CXMLChunkSaver : public IXmlSaver
{
	OBJECT_NOCOPY_METHODS( CXMLChunkSaver );
	//
	CObj<NXml::CXmlReader> pXmlReader;
	const NXml::CXmlNode *pReadNode;

	std::vector<const NXml::CXmlNode *> readChunkLevels;
	int nCurChunkLevel;

	NLXML::CXMLDocument document;
	CDataStream *pDstStream;
	NLXML::CXMLMultiNode *pCurrNode;
	typedef std::list<NLXML::CXMLMultiNode *> CChunksList;
	CChunksList chunkLevels;
	bool bReading;
	std::string szCurrObjectPath;
	std::list<std::string> objectNamesStack;
	//
	typedef std::unordered_map<void *, CPtr<CXmlResource>> CObjectsHash;
	CObjectsHash objects;
	// The ID each stored object is written under, in its references and as its
	// __ServerPtr in SharedClasses. It used to be the object's address, cut to
	// 4 bytes, so a file differed from run to run and on x64 two objects could
	// collide. IDs are handed out 1, 2, 3... in the order objects are first
	// reached, as CStructureSaver does. A reader only matches references to
	// objects by them, so files written either way load either way.
	typedef std::unordered_map<void *, uintptr_t> CPObjectsHash;
	CPObjectsHash storedObjects;
	uintptr_t nNextObjectID = 1; // 0 is the null pointer
	std::list<CPtr<CXmlResource>> toStore;
	//
	void PushReadChunkLevel( const NXml::CXmlNode *pNode );
	void PopReadChunkLevel();

	bool StartChunk( const chunk_id idChunk, int nChunkNumber ) override;
	void FinishChunk() override;
	int CountChunks() override;

	bool DataChunk( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber ) override;
	bool DataChunk( const chunk_id idChunk, int *pnData, int nChunkNumber ) override;
	bool DataChunk( const chunk_id idChunk, float *pfData, int nChunkNumber ) override;
	bool DataChunk( const chunk_id idChunk, bool *pData, int nChunkNumber ) override;
	bool DataChunk( const chunk_id idChunk, boost::uuids::uuid *pgData, int nChunkNumber ) override;
	bool DataChunkDBID( CDBID *pDBID ) override;
	bool DataChunkFilePath( NFile::CFilePath *pFilePath ) override;
	bool DataChunkString( std::string &data ) override;
	bool DataChunkString( std::wstring &data ) override;
	//
	void ReportCurrentObject( const CDBID &dbid ) override;
	void PushCurrentObject( const CDBID &dbid ) override;
	void PopCurrentObject() override;

	const std::string &GetCurrObjectPath() const
	{
		return szCurrObjectPath;
	}

	//
	bool AddAttribute( const chunk_id attrName, bool *pData ) override;
	bool AddAttribute( const chunk_id attrName, int *pData ) override;
	bool AddAttribute( const chunk_id attrName, float *pData ) override;
	bool AddAttribute( const chunk_id attrName, std::string *pData ) override;
	bool AddAttribute( const chunk_id attrName, std::wstring *pData ) override;
	//
	void StoreObject( CObjectBase *pObject ) override;
	CObjectBase *LoadObject() override;
	//
	void Start( CDataStream *pStream, bool bRead );
	void Finish();

	CXMLChunkSaver() {}

public:
	CXMLChunkSaver( CDataStream *pStream, bool bRead )
	{
		NI_ASSERT( pStream != 0, "" );
		Start( pStream, bRead );
	}

	~CXMLChunkSaver() override
	{
		Finish();
	}

	//
	bool IsReading() const override
	{
		return bReading;
	}
};
