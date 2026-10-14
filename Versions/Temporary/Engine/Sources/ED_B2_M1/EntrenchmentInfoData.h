#pragma once

#include "MapInfoEditorData.h"

#include <cstdint>

namespace NMapInfoEditor
{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SEntrenchmentSegInfo
{
	NDb::EEntrenchSegmType eSegType;
	CVec3 vPos0;
	CVec3 vPos1;
	CVec3 vPosCenter;
	float fDirAngle;
	CVec3 vAABBSize;
	CVec2 vAABBCenter;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SEntrenchmentCreateInfo : public SObjectCreateInfo
{
	std::list<SEntrenchmentSegInfo> segmentsInfo;
};

typedef std::list<int> CSegmentLinkIDList;
typedef std::list<CSegmentLinkIDList> CSegmentLinkIDListList;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SEntrenchmentInfo : public SObjectInfo
{
	unsigned nEntrenchmentID;
	CSegmentLinkIDListList segmentLinkIDListList; // LinkID List List (used in advanced clipoard)

	OBJECT_BASIC_METHODS( SEntrenchmentInfo );

public:
	SEntrenchmentInfo()
		: nEntrenchmentID( INVALID_NODE_ID )
	{
	}

	// SObjectInfo
	SObjectInfo *CallDuplicate() const override
	{
		return Duplicate();
	}

	EObjectInfoType GetObjectInfoType() override
	{
		return OIT_ENTRENCHMENT;
	}

	bool NeedMakeOrientation() override
	{
		return true;
	}

	bool KeepZeroHeight() override
	{
		return true;
	}

	bool KeepCommonHeight() override
	{
		return false;
	}

	bool NeedProcessEditParameters() override
	{
		return false;
	}

	//
	static const NDb::SModel *GetModel( const NDb::SEntrenchmentRPGStats *pEntrenchmentRPGStats,
	                                    NDb::EEntrenchSegmType type,
	                                    NDb::ESeason eSeason );

	const NDb::SEntrenchmentRPGStats::SEntrenchSegmentRPGStats *
	GetSegmentInfo( const NDb::SEntrenchmentRPGStats *pEntrenchmentRPGStats, int nFrameIndex );

	bool Load( const SObjectLoadInfo *pObjectLoadInfo, IEditorScene *pEditorScene, IManipulator *pManipulator ) override;
	bool Create( const SObjectCreateInfo *pObjectCreateInfo,
	             IEditorScene *pEditorScene,
	             CObjectBaseController *pObjectController,
	             IManipulator *pManipulator ) override;

	void FillMaskManipulator( class CMaskManipulator *pMaskManipulator ) override {}

	void GetMask( std::string *pszMask ) override {}

	static bool GetEntrenchmentSegmentPositionOnTerrain( CVec3 *pPos,
	                                                     CQuat *pQuat,
	                                                     uint32_t *pdwNormal,
	                                                     CVec3 *pScale,
	                                                     const float fSegmentHalfLen,
	                                                     const CVec3 &rvCenterPos,
	                                                     const float fXYDirection );

	void Remove( bool bUpdateScene,
	             IEditorScene *pEditorScene,
	             bool bUpdateDB,
	             CObjectBaseController *pObjectController,
	             IManipulator *pManipulator ) override;
	bool RemoveFromDB( CObjectBaseController *pObjectController, IManipulator *pManipulator ) override;
	void
	CreateSceneObjects( IEditorScene *pEditorScene, IManipulator *pManipulator, bool bUpdateParentStructure ) override;
	static bool GetEntrenchmentSegInfoByMapObjElement( SEntrenchmentSegInfo *pSegInfo,
	                                                   const SObjectInfo *pMOI,
	                                                   const NDb::SEntrenchmentRPGStats *pEntrenchmentStats,
	                                                   const SMapInfoElement *pElement );
	bool PasteSelf( CLinkIDMap *pNew2OldLinkIDMap,
	                CLinkIDMap *pOld2NewLinkIDMap,
	                IEditorScene *pEditorScene,
	                CObjectBaseController *pObjectController,
	                IManipulator *pManipulator ) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace NMapInfoEditor
