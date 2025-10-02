#if !defined(__MAPINFO_EDITOR_DATA__SIMPLE_OBJECT_INFO__)
#define __MAPINFO_EDITOR_DATA__SIMPLE_OBJECT_INFO__
#pragma once

#include "MapInfoEditorData.h"

namespace NMapInfoEditor
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SSimpleObjectInfo : public SObjectInfo
	{
		OBJECT_BASIC_METHODS( SSimpleObjectInfo );
		
	public:
		static const NDb::SModel* GetModel( const NDb::SHPObjectRPGStats *pHPObjectRPGStats, const NDb::ESeason eSeason );
		static const NDb::SModel* ChooseModelWithHP( const NDb::SHPObjectRPGStats *pHPObjectRPGStats, const float fHP, const NDb::ESeason eSeason );
		static bool NeedMakeOrientation( const string &rszRPGStatsTypeName, const CDBID &rRPGStatsDBID );
		//
		// SObjectInfo
		SObjectInfo* CallDuplicate() const { return Duplicate(); }
		EObjectInfoType GetObjectInfoType() { return OIT_SIMPLE_OBJECT; }
		virtual bool NeedMakeOrientation();
		virtual bool KeepZeroHeight();
		virtual bool KeepCommonHeight() { return false; }
		virtual bool NeedProcessEditParameters() { return true; }
		//
		virtual bool Load( const SObjectLoadInfo* pObjectLoadInfo, IEditorScene *pScene, IManipulator *pManipulator );
		virtual bool Create( const SObjectCreateInfo* pObjectCreateInfo, IEditorScene *pScene, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		virtual void FillMaskManipulator( class CMaskManipulator *pMaskManipulator ) {}
		virtual void GetMask( string *pszMask ) {}
	};
};

#endif // !defined(__MAPINFO_EDITOR_DATA__SIMPLE_OBJECT_INFO__)

