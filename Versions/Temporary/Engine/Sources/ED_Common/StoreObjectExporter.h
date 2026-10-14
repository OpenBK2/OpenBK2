#pragma once

#include "MapEditorLib/Interface_Exporter.h"
#include "MapEditorLib/Tools_HashSet.h"

class CStoreObjectExporter : public IExporter
{
	SObjectSet objectSet;

public:
	inline const SObjectSet &GetObjectSet() const
	{
		return objectSet;
	}

	// IExporter
	bool StartExport( const std::string &rszObjectTypeName, bool bForce ) override
	{
		objectSet.szObjectTypeName = rszObjectTypeName;
		objectSet.objectNameSet.clear();
		return true;
	}

	EXPORT_RESULT ExportObject( IManipulator *pManipulator,
	                            const std::string &rszObjectTypeName,
	                            const std::string &rszObjectName,
	                            bool bForce,
	                            EXPORT_TYPE exportType ) override
	{
		if ( ( exportType != ET_BEFORE_REF ) && ( exportType != ET_NO_REF ) )
		{
			return ER_SUCCESS;
		}
		// Добавляем элемент в набор
		InsertHashSetElement( &( objectSet.objectNameSet ), CDBID( rszObjectName ) );
		return ER_SUCCESS;
	}

	//
	bool StartCheck( const std::string &rszObjectTypeName, bool bExport ) override
	{
		return true;
	}

	void FinishCheck( const std::string &rszObjectTypeName, bool bExport ) override {}

	EXPORT_RESULT CheckObject( IManipulator *pManipulator,
	                           const std::string &rszObjectTypeName,
	                           const std::string &rszObjectName,
	                           bool bExport,
	                           EXPORT_TYPE exportType ) override
	{
		return ER_SUCCESS;
	}
};
