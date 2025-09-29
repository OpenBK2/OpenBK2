#if !defined(__STORE_OBJECT_EXPORTER__)
#define __STORE_OBJECT_EXPORTER__
#pragma once

#include "..\MapEditorLib\Interface_Exporter.h"
#include "..\MapEditorLib\Tools_HashSet.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CStoreObjectExporter : public IExporter
{
	SObjectSet objectSet;

public:
	inline const SObjectSet& GetObjectSet() const { return objectSet; }
	
	// IExporter
	virtual bool StartExport( const string &rszObjectTypeName, bool bForce )
	{
		objectSet.szObjectTypeName = rszObjectTypeName;
		objectSet.objectNameSet.clear();
		return true;
	}
	virtual EXPORT_RESULT ExportObject( IManipulator* pManipulator,
																			const string &rszObjectTypeName,
																			const string &rszObjectName,
																			bool bForce,
																			EXPORT_TYPE exportType )
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
	virtual bool StartCheck( const string &rszObjectTypeName, bool bExport ) { return true; }
	virtual void FinishCheck( const string &rszObjectTypeName, bool bExport ) {}
	virtual EXPORT_RESULT CheckObject( IManipulator* pManipulator,
																		 const string &rszObjectTypeName,
																		 const string &rszObjectName,
																		 bool bExport,
																		 EXPORT_TYPE exportType ) { return ER_SUCCESS; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // !defined(__STORE_OBJECT_EXPORTER__)

