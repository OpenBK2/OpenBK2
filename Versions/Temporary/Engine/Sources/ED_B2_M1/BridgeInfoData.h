#pragma once

#include "MapInfoEditorData.h"

namespace NMapInfoEditor
{
	typedef list<CVec3> CBridgeCenterPointList;

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
			TYPE_TERMINATOR	= 0,
			TYPE_SPAN				= 1,
		};

		enum EDirection
		{
			DIRECTION_X				= 0,
			DIRECTION_Y				= 1,
			DIRECTION_UNKNOWN	= 2,
			DIRECTION_FREE		= 3,
		};

		unsigned nBridgeID;

		OBJECT_BASIC_METHODS( SBridgeInfo );
		
	public:
		//	
		SBridgeInfo() : nBridgeID( INVALID_NODE_ID ) {}
		static int CreateCenterPoints( CBridgeCenterPointList *pResultList,
																	 CVec3 *pvDirection,
																	 const CVec3 &rvStart,
																	 const CVec3 &rvEnd,
																	 EDirection direction,
																	 float fEndSize,
																	 float fCenterSize,
																	 bool bFixStartPoint );
		static const NDb::SModel* GetModel( const NDb::SBridgeRPGStats *pBridgeRPGStats, EType type, NDb::ESeason eSeason );

		// SObjectInfo
		SObjectInfo* CallDuplicate() const { return Duplicate(); }
		EObjectInfoType GetObjectInfoType() { return OIT_BRIDGE; }
		bool NeedMakeOrientation() { return false; }
		bool KeepZeroHeight() { return false; }
		bool KeepCommonHeight() { return true; }
		bool NeedProcessEditParameters() { return true; }
		//
		bool Load( const SObjectLoadInfo* pObjectLoadInfo, IEditorScene *pEditorScene, IManipulator *pManipulator );
		bool Create( const SObjectCreateInfo* pObjectCreateInfo, IEditorScene *pEditorScene, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		void FillMaskManipulator( class CMaskManipulator *pMaskManipulator ) {}
		void GetMask( string *pszMask ) {}
		bool RemoveFromDB( CObjectBaseController *pObjectController, IManipulator *pManipulator );
		void Remove( bool bUpdateScene, IEditorScene *pEditorScene,
								 bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		bool PasteSelf( CLinkIDMap *pNew2OldLinkIDMap, CLinkIDMap *pOld2NewLinkIDMap, IEditorScene *pEditorScene, CObjectBaseController *pObjectController, IManipulator *pManipulator );
	};
};


