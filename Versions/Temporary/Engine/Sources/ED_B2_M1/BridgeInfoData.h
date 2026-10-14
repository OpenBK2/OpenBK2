#pragma once

#include "MapInfoEditorData.h"

namespace NMapInfoEditor
{
typedef std::list<CVec3> CBridgeCenterPointList;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SBridgeCreateInfo : public SObjectCreateInfo
{
	CBridgeCenterPointList centerPointList;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SBridgeInfo : public SObjectInfo
{
	enum EType
	{
		TYPE_TERMINATOR = 0,
		TYPE_SPAN = 1,
	};

	enum EDirection
	{
		DIRECTION_X = 0,
		DIRECTION_Y = 1,
		DIRECTION_UNKNOWN = 2,
		DIRECTION_FREE = 3,
	};

	unsigned nBridgeID;

	OBJECT_BASIC_METHODS( SBridgeInfo );

public:
	//
	SBridgeInfo()
		: nBridgeID( INVALID_NODE_ID )
	{
	}

	static int CreateCenterPoints( CBridgeCenterPointList *pResultList,
	                               CVec3 *pvDirection,
	                               const CVec3 &rvStart,
	                               const CVec3 &rvEnd,
	                               EDirection direction,
	                               float fEndSize,
	                               float fCenterSize,
	                               bool bFixStartPoint );
	static const NDb::SModel *GetModel( const NDb::SBridgeRPGStats *pBridgeRPGStats, EType type, NDb::ESeason eSeason );

	// SObjectInfo
	SObjectInfo *CallDuplicate() const override
	{
		return Duplicate();
	}

	EObjectInfoType GetObjectInfoType() override
	{
		return OIT_BRIDGE;
	}

	bool NeedMakeOrientation() override
	{
		return false;
	}

	bool KeepZeroHeight() override
	{
		return false;
	}

	bool KeepCommonHeight() override
	{
		return true;
	}

	bool NeedProcessEditParameters() override
	{
		return true;
	}

	//
	bool Load( const SObjectLoadInfo *pObjectLoadInfo, IEditorScene *pEditorScene, IManipulator *pManipulator ) override;
	bool Create( const SObjectCreateInfo *pObjectCreateInfo,
	             IEditorScene *pEditorScene,
	             CObjectBaseController *pObjectController,
	             IManipulator *pManipulator ) override;

	void FillMaskManipulator( class CMaskManipulator *pMaskManipulator ) override {}

	void GetMask( std::string *pszMask ) override {}

	bool RemoveFromDB( CObjectBaseController *pObjectController, IManipulator *pManipulator ) override;
	void Remove( bool bUpdateScene,
	             IEditorScene *pEditorScene,
	             bool bUpdateDB,
	             CObjectBaseController *pObjectController,
	             IManipulator *pManipulator ) override;
	bool PasteSelf( CLinkIDMap *pNew2OldLinkIDMap,
	                CLinkIDMap *pOld2NewLinkIDMap,
	                IEditorScene *pEditorScene,
	                CObjectBaseController *pObjectController,
	                IManipulator *pManipulator ) override;
};
}; // namespace NMapInfoEditor
