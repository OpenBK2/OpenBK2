#pragma once

namespace NXMLExport
{

class CXmlExporter
{
protected:
	struct SObjStackEntry
	{
		string szClassTypeName;
		string szObjectName;
		string szFieldName;
	};
private:
	typedef list<SObjStackEntry> CObjectsStack;
	CObjectsStack objectsStack;
	typedef hash_map<string, bool> CExportedObjectsMap;
	CExportedObjectsMap exportedObjects;
	//
	void ExportObjectToXML( FILE *file, const string &szTypeName, const int nClassTypeID, 
													const string &szObjectName, const int nObjectID, const string &szFieldName );
protected:
	const SObjStackEntry *GetFrontObject() { return objectsStack.empty() ? 0 : &( objectsStack.front() ); }
	//
	virtual string MakePathName( const string &szObjectName, const string &szClassTypeName, const string &szFieldName ) = 0;
	virtual void StartExport( const string &szObjectName, const string &szClassTypeName, const string &szFieldName );
	virtual void FinishExport();
public:
	virtual bool ExportObject( const string &szObjectName, const string &szClassTypeName, const string &szFieldName );
};

void DumpAllObjects();
CXmlExporter *GetExporter();

}

