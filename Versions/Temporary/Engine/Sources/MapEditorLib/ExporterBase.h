#pragma once

#include "Interface_Exporter.h"
#include "Tools_UniqueList.h"


class CExporterBase : public IExporter
{
	typedef std::unordered_map<std::string, std::string> CExportObjectNameMap;
	typedef std::unordered_map<std::string, std::string> CExportObjectTypeNameMap;
	typedef CUniqueList<std::list<std::string>, std::string> CObjectTypeNameList;
	typedef std::unordered_map<std::string, EXPORT_RESULT> CResultMap;
	//
	struct SInvalidLink
	{
		std::string szObjectTypeName;
		std::string szObjectName;
		std::string szPropertyName;
	};
	typedef std::list<SInvalidLink> CInvalidLinkList;
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
														 const std::string &rszObjectTypeName,
														 const std::string &rszObjectName,
														 CObjectTypeNameList *pObjectTypeNameList,
														 CExportObjectTypeNameMap *pExportObjectTypeNameMap,
														 CExportObjectNameMap *pExportObjectNameMap,
														 CInvalidLinkList* pInvalidLinkList );
	//
	void InnerStartExport( const CObjectTypeNameList &rNewObjectTypeNameList, bool bExport, bool bForce );
	void InnerFinishExport( bool bExport, bool bForce );
	EXPORT_RESULT InnerExportObject( IManipulator* pManipulator,
																	 const std::string &rszObjectTypeName,
																	 const std::string &rszObjectName,
																	 bool bExport,
																	 bool bForce );
	EXPORT_RESULT HierarchyExportObject( IManipulator* pManipulator,
																			 const std::string &rszObjectTypeName,
																			 const std::string &rszObjectName,
																			 bool bExport,
																			 bool bForce );
public:
	EXPORT_RESULT GetStartExportResult( const std::string &rszObjectTypeName );
	void SetStartExportResult( const std::string &rszObjectTypeName, EXPORT_RESULT eResult );
	//
	EXPORT_RESULT GetExportObjectResult( const std::string &rszObjectRefName );
	void SetExportObjectResult( const std::string &rszObjectRefName, EXPORT_RESULT eResult );
	//
	bool StartExport( const std::string &rszObjectTypeName, bool bForce );
	void FinishExport( const std::string &rszObjectTypeName, bool bForce );
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
	//
	bool StartCheck( const std::string &rszObjectTypeName, bool bExport );
	void FinishCheck( const std::string &rszObjectTypeName, bool bExport );
	EXPORT_RESULT CheckObject( IManipulator* pManipulator,
														 const std::string &rszObjectTypeName,
														 const std::string &rszObjectName,
														 bool bExport,
														 EXPORT_TYPE exportType );
};



