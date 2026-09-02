#pragma once

#include "Interface_Exporter.h"
#include "Tools_UniqueList.h"


class CExporterBase : public IExporter
{
	typedef std::unordered_map<string, string> CExportObjectNameMap;
	typedef std::unordered_map<string, string> CExportObjectTypeNameMap;
	typedef CUniqueList<list<string>, string> CObjectTypeNameList;
	typedef std::unordered_map<string, EXPORT_RESULT> CResultMap;
	//
	struct SInvalidLink
	{
		string szObjectTypeName;
		string szObjectName;
		string szPropertyName;
	};
	typedef list<SInvalidLink> CInvalidLinkList;
	//
	// типы подвергнувшиеся StartExport ( их необходимо будет пропустить через FinishExport )
	CObjectTypeNameList objectTypeNameList;
	// результаты вызовов StartExport
	CResultMap startExportResultMap;
	// обьекты уже проконверченные ( для разрешения циклических ссылок, формат: ObjectTypeName:ObjectName )
	// результаты вызовов ExportObject
	CResultMap exportObjectResultMap;
	// Список незаполненных ссылок
	CInvalidLinkList invalidLinkList;	
	//
	bool GetObjectTypeNameSet( IManipulator* pManipulator,
														 const string &rszObjectTypeName,
														 const string &rszObjectName,
														 CObjectTypeNameList *pObjectTypeNameList,
														 CExportObjectTypeNameMap *pExportObjectTypeNameMap,
														 CExportObjectNameMap *pExportObjectNameMap,
														 CInvalidLinkList* pInvalidLinkList );
	//
	void InnerStartExport( const CObjectTypeNameList &rNewObjectTypeNameList, bool bExport, bool bForce );
	void InnerFinishExport( bool bExport, bool bForce );
	EXPORT_RESULT InnerExportObject( IManipulator* pManipulator,
																	 const string &rszObjectTypeName,
																	 const string &rszObjectName,
																	 bool bExport,
																	 bool bForce );
	EXPORT_RESULT HierarchyExportObject( IManipulator* pManipulator,
																			 const string &rszObjectTypeName,
																			 const string &rszObjectName,
																			 bool bExport,
																			 bool bForce );
public:
	EXPORT_RESULT GetStartExportResult( const string &rszObjectTypeName );
	void SetStartExportResult( const string &rszObjectTypeName, EXPORT_RESULT eResult );
	//
	EXPORT_RESULT GetExportObjectResult( const string &rszObjectRefName );
	void SetExportObjectResult( const string &rszObjectRefName, EXPORT_RESULT eResult );
	//
	bool StartExport( const string &rszObjectTypeName, bool bForce );
	void FinishExport( const string &rszObjectTypeName, bool bForce );
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
	//
	bool StartCheck( const string &rszObjectTypeName, bool bExport );
	void FinishCheck( const string &rszObjectTypeName, bool bExport );
	EXPORT_RESULT CheckObject( IManipulator* pManipulator,
														 const string &rszObjectTypeName,
														 const string &rszObjectName,
														 bool bExport,
														 EXPORT_TYPE exportType );
};



