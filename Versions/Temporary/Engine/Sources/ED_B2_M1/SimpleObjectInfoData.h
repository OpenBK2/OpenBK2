#pragma once

#include "MapInfoEditorData.h"

namespace NMapInfoEditor
{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SSimpleObjectInfo : public SObjectInfo
{
	OBJECT_BASIC_METHODS( SSimpleObjectInfo );

public:
	static const NDb::SModel *GetModel( const NDb::SHPObjectRPGStats *pHPObjectRPGStats, const NDb::ESeason eSeason );
	static const NDb::SModel *
	ChooseModelWithHP( const NDb::SHPObjectRPGStats *pHPObjectRPGStats, const float fHP, const NDb::ESeason eSeason );
	static bool NeedMakeOrientation( const std::string &rszRPGStatsTypeName, const CDBID &rRPGStatsDBID );

	//
	// SObjectInfo
	SObjectInfo *CallDuplicate() const override
	{
		return Duplicate();
	}

	EObjectInfoType GetObjectInfoType() override
	{
		return OIT_SIMPLE_OBJECT;
	}

	bool NeedMakeOrientation() override;
	bool KeepZeroHeight() override;

	bool KeepCommonHeight() override
	{
		return false;
	}

	bool NeedProcessEditParameters() override
	{
		return true;
	}

	//
	bool Load( const SObjectLoadInfo *pObjectLoadInfo, IEditorScene *pScene, IManipulator *pManipulator ) override;
	bool Create( const SObjectCreateInfo *pObjectCreateInfo,
	             IEditorScene *pScene,
	             CObjectBaseController *pObjectController,
	             IManipulator *pManipulator ) override;

	void FillMaskManipulator( class CMaskManipulator *pMaskManipulator ) override {}

	void GetMask( std::string *pszMask ) override {}
};
}; // namespace NMapInfoEditor
