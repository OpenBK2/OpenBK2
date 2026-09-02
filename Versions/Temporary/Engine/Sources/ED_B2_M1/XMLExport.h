#pragma once

namespace NXMLExport
{

class CXmlExporter
{
protected:
	struct SObjStackEntry
	{
		std::string szClassTypeName;
		std::string szObjectName;
		std::string szFieldName;
	};
private:
	typedef std::list<SObjStackEntry> CObjectsStack;
	CObjectsStack objectsStack;
	typedef std::unordered_map<std::string, bool> CExportedObjectsMap;
	CExportedObjectsMap exportedObjects;
	//
	void ExportObjectToXML( FILE *file, const std::string &szTypeName, const int nClassTypeID, 
													const std::string &szObjectName, const int nObjectID, const std::string &szFieldName );
protected:
	const SObjStackEntry *GetFrontObject() { return objectsStack.empty() ? 0 : &( objectsStack.front() ); }
	//
	virtual std::string MakePathName( const std::string &szObjectName, const std::string &szClassTypeName, const std::string &szFieldName ) = 0;
	virtual void StartExport( const std::string &szObjectName, const std::string &szClassTypeName, const std::string &szFieldName );
	virtual void FinishExport();
public:
	virtual bool ExportObject( const std::string &szObjectName, const std::string &szClassTypeName, const std::string &szFieldName );
};

void DumpAllObjects();
CXmlExporter *GetExporter();

}

