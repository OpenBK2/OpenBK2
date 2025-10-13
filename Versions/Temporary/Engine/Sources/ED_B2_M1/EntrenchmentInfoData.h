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
		list<SEntrenchmentSegInfo> segmentsInfo;
	};

	typedef list<int> CSegmentLinkIDList;
	typedef list<CSegmentLinkIDList> CSegmentLinkIDListList;
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SEntrenchmentInfo : public SObjectInfo
	{
		UINT nEntrenchmentID;
		CSegmentLinkIDListList segmentLinkIDListList;				// LinkID List List (used in advanced clipoard)

		OBJECT_BASIC_METHODS( SEntrenchmentInfo );

	public:
		SEntrenchmentInfo() : nEntrenchmentID( INVALID_NODE_ID ) {}

		// SObjectInfo
		SObjectInfo* CallDuplicate() const { return Duplicate(); }
		EObjectInfoType GetObjectInfoType() { return OIT_ENTRENCHMENT; }
		bool NeedMakeOrientation() { return true; }
		bool KeepZeroHeight() { return true; }
		bool KeepCommonHeight() { return false; }
		bool NeedProcessEditParameters() { return false; }
		//
		static const NDb::SModel* GetModel( const NDb::SEntrenchmentRPGStats *pEntrenchmentRPGStats, NDb::EEntrenchSegmType type, NDb::ESeason eSeason );
		
		const NDb::SEntrenchmentRPGStats::SEntrenchSegmentRPGStats* GetSegmentInfo( const NDb::SEntrenchmentRPGStats *pEntrenchmentRPGStats, int nFrameIndex );

		bool Load( const SObjectLoadInfo* pObjectLoadInfo, IEditorScene *pEditorScene, IManipulator *pManipulator );
		bool Create( const SObjectCreateInfo* pObjectCreateInfo, IEditorScene *pEditorScene, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		void FillMaskManipulator( class CMaskManipulator *pMaskManipulator ) {}
		void GetMask( string *pszMask ) {}

		static bool GetEntrenchmentSegmentPositionOnTerrain( CVec3 *pPos,
																												 CQuat *pQuat,
																												 uint32_t *pdwNormal,
																												 CVec3 *pScale,
																												 const float fSegmentHalfLen,
																												 const CVec3 &rvCenterPos,
																												 const float fXYDirection );

		void Remove( bool bUpdateScene, IEditorScene *pEditorScene,
								 bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		bool RemoveFromDB( CObjectBaseController *pObjectController, IManipulator *pManipulator );
		virtual void CreateSceneObjects( IEditorScene *pEditorScene, IManipulator *pManipulator, bool bUpdateParentStructure );
		static bool GetEntrenchmentSegInfoByMapObjElement( SEntrenchmentSegInfo *pSegInfo, const SObjectInfo *pMOI, 
		const NDb::SEntrenchmentRPGStats *pEntrenchmentStats, const SMapInfoElement *pElement );
		bool PasteSelf( CLinkIDMap *pNew2OldLinkIDMap, CLinkIDMap *pOld2NewLinkIDMap, IEditorScene *pEditorScene, CObjectBaseController *pObjectController, IManipulator *pManipulator );
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}//namespace


