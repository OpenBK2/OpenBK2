#pragma once

#include "interface_view.h"
#include "Interface_Builder.h" //CTableSet;
#include "Tools_Resources.h"
#include "ControlSelection.h"
#include "System/XmlSaver.h"

#include <cstdint>

struct SUserData
{
	enum ESerializeType
	{
		ST_LOAD	= 0,						
		ST_SAVE = 1,
	};

	enum ESettingsType
	{
		EDITOR_SETTINGS	= 0,						
		EXPORTER_SETINGS = 1,
		BUILDER_SETTINGS = 2,
		PRIVATE_SETTINGS = 3,
	};

	enum ENormalizePathType
	{
		NPT_UNKNOWN							= 0,
		NPT_EXPORT_SOURCE				= 1,
		NPT_EXPORT_DESTINATION	= 2,
		NPT_DATA_STORAGE				= 3,
		NPT_START								= 4,
		NPT_COUNT								= 5,
	};
	//
	struct SMayaExportData
	{
		typedef std::unordered_map<std::string, std::string> CGrannyExportSettingsMap;

		CGrannyExportSettingsMap grannyExportSettings;
		//// TODO{ remove it
		std::string szAIGeomSettingsFileName;
		std::string szGeomSettingsFileName;
		std::string szGeomAttributesSettingsFileName;
		std::string szAnimSettingsFileName;
		std::string szSkeletonSettingsFileName;
		std::string szMaterialsSettingsFileName;
		//// TODO}
		// interactive maya response timeout. 0 means no timeout
		int nMayaResponseTimeout;
		// interactive maya will restart every nMayaExecutionQuota query executions. 0 means no restarts ever
		int nMayaExecutionQuota;
		std::string szMayaExportPath;
		std::string szMayaVersionPath;
		std::string szMayaScriptPath;
		std::string szStartupScript;
		std::string szOldPluginFileName;
		std::string szOldPluginName;
		std::string szToolFileName;
		std::string szLogFileName;
		std::string szScriptFileName;
		std::string szOldPluginParticleFixYZOption;

		SMayaExportData()
			: nMayaResponseTimeout(0), nMayaExecutionQuota(0)
		{}

		// serializing...
		int operator&( IXmlSaver &xs )
		{
			xs.Add( "GrannyExportSettings", &grannyExportSettings );
			// TODO{ remove it
			xs.Add( "AIGeomSettingsFileName", &szAIGeomSettingsFileName );
			xs.Add( "GeomSettingsFileName", &szGeomSettingsFileName );
			xs.Add( "GeomAttributesSettingsFileName", &szGeomAttributesSettingsFileName );
			xs.Add( "AnimSettingsFileName", &szAnimSettingsFileName );
			xs.Add( "SkeletonSettingsFileName", &szSkeletonSettingsFileName );
			xs.Add( "MaterialsSettingsFileName", &szMaterialsSettingsFileName );
			// TODO}
			xs.Add( "MayaResponseTimeout", &nMayaResponseTimeout );
			xs.Add( "MayaExecutionQuota", &nMayaExecutionQuota );
			xs.Add( "MayaExportPath", &szMayaExportPath );
			xs.Add( "MayaVersionPath", &szMayaVersionPath );
			xs.Add( "MayaScriptPath", &szMayaScriptPath );
			xs.Add( "StartupScript", &szStartupScript );
			xs.Add( "OldPluginFileName", &szOldPluginFileName );
			xs.Add( "OldPluginName", &szOldPluginName );
			xs.Add( "ToolFileName", &szToolFileName );
			xs.Add( "LogFileName", &szLogFileName );
			xs.Add( "ScriptFileName", &szScriptFileName );
			xs.Add( "OldPluginParticleFixYZOption", &szOldPluginParticleFixYZOption );
			return 0;
		}
	};
	//
	struct SObjectTypeData
	{
		//typedef hash_map<string, string> CBrowsePathMap;
		// CRAP{ HASH_SET
		typedef std::unordered_map<std::string, uint32_t> CExpandedPropertySet;
		typedef std::unordered_map<std::string, uint32_t> CExpandedObjectSet;
		// CRAP} HASH_SET
		std::string szCurrentObject;										// объект выделенный в таблице
		std::string szCurrentProperty;									// свойство выделенное у объекта
		//CBrowsePathMap browsePathMap;						// пути для диалогов OpenFile и т.д.
		CExpandedPropertySet expandedPropertySet;	// открытые ветки в Property Control
		CExpandedObjectSet expandedObjectSet;			// открытые ветки в дереве обьектов
		//string szEditorSettingsFileName;					// имя файла с настройками редактора, если не задан генерируется по умолчанию
		//string szExporterSettingsFileName;				// имя файла с настройками экспортера, если не задан генерируется по умолчанию
		//string szBuilderSettingsFileName;					// имя файла с настройками компоновщика, если не задан генерируется по умолчанию

		// serializing...
		int operator&( IXmlSaver &xs )
		{
			xs.Add( "CurrentObject", &szCurrentObject );
			xs.Add( "CurrentProperty", &szCurrentProperty );
			//xs.Add( "BrowsePathMap", &browsePathMap );
			xs.Add( "ExpandedPropertySet", &expandedPropertySet );
			xs.Add( "ExpandedObjectSet", &expandedObjectSet );
			//xs.Add( "EditorSettingsFileName", &szEditorSettingsFileName );
			//xs.Add( "ExporterSettingsFileName", &szExporterSettingsFileName );
			//xs.Add( "BuilderSettingsFileName", &szBuilderSettingsFileName );
			return 0;
		}
	};
	typedef std::list<std::string> CRecentList;
	typedef std::vector<COLORREF> CColorList;
	typedef std::unordered_map<std::string, SObjectTypeData> CObjectTypeDataMap;
	typedef std::unordered_map<std::string, std::string> CBuildDataTypeNameMap;
	typedef std::unordered_map<std::string, std::string> CObjectDBTypeMap;
	typedef std::unordered_map<std::string, std::string> CFilePathMap;
	typedef std::unordered_map<std::string, std::string> CRefPathMap;

	// данные не изменемые из программы и не сохраняющиеся в файл
	struct SConstUserData
	{
		struct SPropertyControlData
		{
			std::string szLUAKeyWordsFileName;
			// serializing...
			int operator&( IXmlSaver &xs )
			{
				xs.Add( "LUAKeyWordsFileName", &szLUAKeyWordsFileName );
				return 0;
			}
		};

		struct SObjectTypeData
		{
			std::string szEditorSettingsFileName;				// имя файла с настройками редактора, если не задан генерируется по умолчанию
			std::string szExporterSettingsFileName;			// имя файла с настройками экспортера, если не задан генерируется по умолчанию
			std::string szBuilderSettingsFileName;				// имя файла с настройками компоновщика, если не задан генерируется по умолчанию

			// serializing...
			int operator&( IXmlSaver &xs )
			{
				xs.Add( "EditorSettingsFileName", &szEditorSettingsFileName );
				xs.Add( "ExporterSettingsFileName", &szExporterSettingsFileName );
				xs.Add( "BuilderSettingsFileName", &szBuilderSettingsFileName );
				return 0;
			}
		};
		typedef std::unordered_map<std::string, SObjectTypeData> CObjectTypeDataMap;

		std::string szVersion;
		std::string szApplicationTitle;
		// поля необходимые для работы Property Control
		SPropertyControlData propertyControlData;
		// maya export settings (pathes, scripts, etc.)
		SMayaExportData mayaExportData;
		// Object types' specific configuration files
		CObjectTypeDataMap objectTypeData;
		// Имя типа объекта, используемое в особом режиме редактора, как основной тип редактируемых объектов
		std::string szMainObjectType;
		// Object GUID property
		std::string szGUIDName;
		// Common Folders
		std::string szExportSourceFolder;					// Исходники для binary resources
		std::string szExportDestinationFolder;			// Текущее место складывания binary resource при экспорте
		std::string szDataStorageFolder;						// Основной каталог resources "..\\Data", (для конфигурационных файлов)
		std::string szStartFolder;									// Стартовый каталог программы
		std::string szObjectRecordIDsFolder;				// folder with object record IDs .ini file

		// serializing...
		int operator&( IXmlSaver &xs )
		{
			xs.Add( "Version", &szVersion );
			xs.Add( "ApplicationTitle", &szApplicationTitle );
			xs.Add( "PropertyControlData", &propertyControlData );
			xs.Add( "MayaExport", &mayaExportData );
			xs.Add( "ObjectTypeConfigFiles", &objectTypeData );
			xs.Add( "MainObjectType", &szMainObjectType );
			xs.Add( "GUIDName", &szGUIDName );
			xs.Add( "ExportSourceFolder", &szExportSourceFolder );
			xs.Add( "ExportDestinationFolder", &szExportDestinationFolder );
			xs.Add( "ObjectRecordIDsFolder", &szObjectRecordIDsFolder );
//			xs.Add( "DataStorageFolder", &szDataStorageFolder );

			return 0;
		}
	};
	SConstUserData constUserData;
	//
	CRecentList recentList;										// resent list for next and previous objects
	CRecentList recentResourceList;						// resent list for next and previous objects
	CColorList colorList;											// custom colors in color dialog
	CTableSetMap tableSetMap;									// visible tables
	std::unordered_map<int, std::string> szCurrentTableMap;	// current table
	std::vector<int> tableHeaderWidthList;					// table header width values
	std::vector<int> tableLinkHeaderWidthList;			// тоже самое но для диалога ObjectLink
	// User Module (project-specific module)
	//vector<string> userModuleFileNamesList;	// list of project specific modules to load at editor start
	std::string szUserModuleFileName;							// project specific module to load at editor start
	//
	CObjectTypeDataMap objectTypeDataMap;
	CBuildDataTypeNameMap buildDataTypeNameMap;
	//Search Parameters
	std::string szLastSearchedText;
	CDBID dbidMainOptions;
	// Maya export
	SMayaExportData mayaExportData;
	//
	CObjectDBTypeMap buildingDBTypeMap;		// default ojects for building builder (copy fields to new object from here)
	CObjectDBTypeMap infantryDBTypeMap;		// default ojects for infantry builder (copy fields to new object from here)
	CObjectDBTypeMap mechUnitDBTypeMap;		// default ojects for mechUnit builder (copy fields to new object from here)
	//
	CFilePathMap filePathMap;							// пути для диалогов OpenFile и т.д.
	CRefPathMap refPathMap;								// пути для диалогов RefLink и т.д.
	//
	std::string szDebugParam;
	//
	std::list<int> gdbBrowserIDList;						// список окон с каталогами
	int nFocusedGDBBrowserID;							// текущее активное окно с каталогамми
	//
	std::string szOpenedMODFolder;
	//
	struct SPCSelectionData
	{
		CVariant value;
	};
	typedef CControlSelection<std::string, SPCSelectionData> CPCSelection;
	CPCSelection pcSelection;
	//
	bool bShowLogMessages;
	bool bShowLogWarnings;
	bool bShowLogErrors;
	//
	SUserData()
		: bShowLogMessages( true ),
			bShowLogWarnings( true ),
			bShowLogErrors( true )
	{}
	//
	int operator&( IXmlSaver &xs )
	{
		xs.Add( "RecentList", &recentList );
		xs.Add( "RecentResourceList", &recentResourceList);
		xs.Add( "ColorList", &colorList );
		xs.Add( "TableSetMap", &tableSetMap );
		xs.Add( "CurrentTableMap", &szCurrentTableMap );
		xs.Add( "TableHeaderWidthList", &tableHeaderWidthList );
		xs.Add( "TableLinkHeaderWidthList", &tableLinkHeaderWidthList );
		xs.Add( "ObjectTypeDataMap", &objectTypeDataMap );
		xs.Add( "BuildDataTypeNameMap", &buildDataTypeNameMap );
		xs.Add( "LastSearchedText", &szLastSearchedText );
		xs.Add( "MainOptionsDBID", &dbidMainOptions );
		xs.Add( "MayaExport", &mayaExportData );
		xs.Add( "BuildingDBTypeMap", &buildingDBTypeMap );
		xs.Add( "InfantryDBTypeMap", &infantryDBTypeMap );
		xs.Add( "MechUnitDBTypeMap", &mechUnitDBTypeMap );
		xs.Add( "FilePathMap", &filePathMap );
		xs.Add( "RefPathMap", &refPathMap );
		xs.Add( "DebugParam", &szDebugParam );
		xs.Add( "GDBBrowserIDList", &gdbBrowserIDList );
		xs.Add( "FocusedGDBBrowserID", &nFocusedGDBBrowserID );
		xs.Add( "ShowLogMessages", &bShowLogMessages );
		xs.Add( "ShowLogWarnings", &bShowLogWarnings );
		xs.Add( "ShowLogErrors", &bShowLogErrors );
		xs.Add( "OpenedMODFolder", &szOpenedMODFolder );
		//
		if ( xs.IsReading() )
		{
			if ( dbidMainOptions.IsEmpty() )
				dbidMainOptions = CDBID( "Editor/EditorOptions.xdb" );
		}
		//
		return 0;
	}

	const std::string GetPath( ENormalizePathType type )
	{
		switch ( type )
		{
			case NPT_EXPORT_SOURCE:
				return constUserData.szExportSourceFolder;
			case NPT_EXPORT_DESTINATION:
				return constUserData.szExportDestinationFolder;
			case NPT_DATA_STORAGE:
				return constUserData.szDataStorageFolder;
			case NPT_START:
				return constUserData.szStartFolder;
			default:
				break;
		}
		return std::string();
	}

	// Установить путь в соответствии с указанными свойствами
	// для однозначности: если указан начальный путь то наличие проверяется только по указанному пути
	// true		- имя существует в указанном каталоге ( поверка производится только при bExists = true )
	// false	- имя не существует в указанном каталоге ( поверка производится только при bExists = true )
	bool NormalizePath( std::string *pszPath, bool bFile, bool bExists, bool bReturnAbsolutePath, ENormalizePathType type, bool *pbAbsolutePath )
	{
		std::string szPathPrefix = GetPath( type );
		return ::NormalizePath( pszPath, bFile, bExists, bReturnAbsolutePath, szPathPrefix, pbAbsolutePath );
	}
	//
	template<class TSettings>
	bool SerializeSettings( TSettings &rSettings, const std::string &rszTypeName, ESettingsType settingsType, ESerializeType serializeType )
	{
		std::string szSettingsFileName;
		switch ( settingsType )
		{
			case EDITOR_SETTINGS:
			{
				szSettingsFileName = constUserData.objectTypeData[rszTypeName].szEditorSettingsFileName;
				if ( szSettingsFileName.empty() )
				{
					szSettingsFileName = StrFmt( "Editor\\%sEditor", rszTypeName.c_str() );
				}
				break;
			}
			case EXPORTER_SETINGS:
			{
				szSettingsFileName = constUserData.objectTypeData[rszTypeName].szExporterSettingsFileName;
				if ( szSettingsFileName.empty() )
				{
					szSettingsFileName = StrFmt( "Editor\\%sExporter", rszTypeName.c_str() );
				}
				break;
			}
			case BUILDER_SETTINGS:
			{
				szSettingsFileName = constUserData.objectTypeData[rszTypeName].szBuilderSettingsFileName;
				if ( szSettingsFileName.empty() )
				{
					szSettingsFileName = StrFmt( "Editor\\%sBuilder", rszTypeName.c_str() );
				}
				break;
			}
			case PRIVATE_SETTINGS:
			{
				szSettingsFileName = StrFmt( "Editor\\%s", rszTypeName.c_str() );
				break;
			}
			default:
				return false;
		}
		NormalizePath( &szSettingsFileName, true, false, true, NPT_START, 0 );
		if ( serializeType == ST_LOAD )
		{
			return LoadTypedSuperXMLResource( szSettingsFileName, ".xml", rSettings );
		}
		else if ( serializeType == ST_SAVE )
		{
			return SaveTypedSuperXMLResource( szSettingsFileName, ".xml", rSettings );
		}
		else
		{
			return false;
		}
	}
};


struct IUserDataContainer : public CObjectBase
{
	enum { tidTypeID = 0x1408AB40 };
	// Получить User Data
	virtual SUserData* Get() = 0;
	//
	virtual void Load() = 0;
	virtual void Save() = 0;
};



