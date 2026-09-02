#pragma once

#include "MapInfoEditorData.h"
#include "Misc/PlaneGeometry.h"

#include <cstdint>

namespace NMapInfoEditor
{
	//  Spot Square
	//
	//  v3                            v2
	//  |-----------------------------|
	//  |                             |
	//  |                             |
	//  |                             |
	//  |                             |
	//  |                             |
	//  |                             |
	//  |              *              |
	//  |              vPosition      |
	//  |                             |
	//  |                             |
	//  |                             |
	//  |                             |
	//  |                  vDirection |
	//  |---------------------------->|
	//  v0                            v1
	//
	typedef std::vector<CVec2> CSpotSquare;
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SSpotLoadInfo : public SObjectLoadInfo
	{
		CSpotSquare spotSquare;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SSpotCreateInfo : public SObjectCreateInfo
	{
		CSpotSquare spotSquare;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SSpotInfo : public SObjectInfo
	{
		OBJECT_BASIC_METHODS( SSpotInfo );

	public:
		std::string szRPGStatsTypeName;
		CDBID rpgStatsDBID;
		unsigned nLinkID;
		CSpotSquare spotSquare;
		CVec3 vAdditionalPosition;
		//
		void MakeAbsoluteSpotSquare()
		{
			NI_ASSERT( ( spotSquare.size() == 4 ), "MakeAbsoluteSpotSquare(), spotSquare.size() != 4" );
			const CVec2 vMassCenter = CVec2( vPosition.x + vAdditionalPosition.x, vPosition.y + vAdditionalPosition.y );
			for ( CSpotSquare::iterator itSpotPoint = spotSquare.begin(); itSpotPoint != spotSquare.end(); ++itSpotPoint )
			{
				( *itSpotPoint ) += vMassCenter;
			}
		}
		//
		void MakeRelativeSpotSquare( bool bDirection )
		{
			NI_ASSERT( ( spotSquare.size() == 4 ), "MakeAbsoluteSpotSquare(), spotSquare.size() != 4" );
			CVec2 vMassCenter = VNULL2;
			GetPolygonMassCenter( spotSquare, &vMassCenter );
			for ( CSpotSquare::iterator itSpotPoint = spotSquare.begin(); itSpotPoint != spotSquare.end(); ++itSpotPoint )
			{
				( *itSpotPoint ) -= vMassCenter;
			}
			vPosition = CVec3( vMassCenter.x - vAdditionalPosition.x, vMassCenter.y - vAdditionalPosition.y, 0.0f );
			if ( bDirection )
			{ 
				fDirection = GetPolarAngle( spotSquare[1] - spotSquare[0] );
			}
		}
		void ClearAdditionalPosition( bool bUpdateSceneElements ) { vAdditionalPosition = VNULL3; }
		//
		// если присутствует объект возвращает true
		bool Pick( const CVec3 &rvPos );
		bool Pick( const CSelectionSquare &rSelectionSquare );
		// рисует объект
		bool Draw( CSceneDrawTool *pSceneDrawTool );
		//
		// вспомогательные методы
		void UpdateSceneElements( bool bModel, bool bPosition, bool bDirection, float fAdditionalDirection ) {}
		//
		void FitToGrid( bool bUpdateSceneElements ) {}
		void RotateTo90Degree( bool bUpdateSceneElements ) {}
		void SetCommonHeight( bool bUpdateSceneElements ) {}
		void FixInvalidPos( bool bUpdateSceneElements );

		// абстрактные методы
		// настройщики
		void GetDrawSelectionParameters( uint32_t *pdwSceneObject, uint32_t *pdwObject, uint32_t *pdwObjectLink, uint32_t *pdwMainObject ) {}
		bool NeedMakeOrientation() { return false; }
		bool KeepZeroHeight() { return false; }
		bool KeepCommonHeight() { return false; }
		bool NeedProcessEditParameters() { return true; }
		// процедуры
		SObjectInfo* CallDuplicate() const { return Duplicate(); }
		EObjectInfoType GetObjectInfoType()  { return OIT_SPOT; }
		// загружаем объект из базы
		bool Load( const SObjectLoadInfo* pObjectLoadInfo, IEditorScene *pEditorScene, IManipulator *pManipulator );
		// создаем объект
		bool Create( const SObjectCreateInfo* pObjectCreateInfo, IEditorScene *pEditorScene, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		// заполнить свойствами MaskManipulator
		void InsertMaskManipulators( class CMultiManipulator *pPropertyManipulator, IManipulator *pManipulator ) {}
		void FillMaskManipulator( class CMaskManipulator *pMaskManipulator ) {}
		void GetMask( std::string *pszMask ) {}
		//
		// методы имеющие реализацию поумолчанию
		// заполняет данные объекта, которые можно загрузить когда все объекты загружены
		bool PostLoad( IEditorScene *pEditorScene, IManipulator *pManipulator ) { return true; }
		// изменяет положение объекта
		bool Move( const SObjectEditInfo *pObjectEditInfo, const CVec3 &rvNewPosition,
							 bool bMoveLinkedObjects,
							 bool bUpdateScene, IEditorScene *pEditorScene,
							 bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		// изменяет угол объекта
		bool Rotate( const SObjectEditInfo *pObjectEditInfo, float fNewDirection,
								 bool bRotateLinkedObjects,
								 bool bUpdateScene, IEditorScene *pEditorScene,
								 bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		// обновляет позиции в сцене
		bool UpdateScene( IEditorScene *pEditorScene );
		// обновляет позиции в базе данных
		bool UpdateDB( bool bUpdateLinkedObjects, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		bool RemoveFromDB( CObjectBaseController *pObjectController, IManipulator *pManipulator ) { return true; }
		void Remove( bool bUpdateScene, IEditorScene *pEditorScene,
								 bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		// проверить на возможность создания линка для этого объекта
		bool CheckLinkCapability( unsigned nSceneObjectIDLinkTo ) { return false; }
		// добавить линк на указанный объект
		bool InsertLink( bool bUpdateDB, unsigned nSceneObjectIDLinkTo, CObjectBaseController *pObjectController, IManipulator *pManipulator ) { return true; }
		// Удалить линки у объектов на этоот объкт, которые ссылаются на объект, после вызова этого метода на объект никто не ссылается
		bool RemoveLinks( bool bUpdateLinkedObjects, bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator ) { return true; }
		// Удалить линки у объекта, после вызова этого метода объект ни накого не ссылается
		bool RemoveLinkTo( bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator ) { return true; }
		//
		void UpdateByController( unsigned nSpotID, unsigned nFlags, IEditorScene *pEditorScene, IManipulator *pManipulator );
		//
		void CreateSceneObjects( IEditorScene *pEditorScene, IManipulator *pManipulator, bool bUpdateParentStructure );
		void RemoveFromScene( IEditorScene *pEditorScene, bool bUpdateParentStructure );
		void SetSceneObjectOpacity( IEditorScene *pEditorScene, const float fOpacity ) {}

		void CopySelf();
		void PasteLinkIDList( CLinkIDMap *pNew2OldLinkIDMap, CLinkIDMap *pOld2NewLinkIDMap );
		bool PasteSelf( CLinkIDMap *pNew2OldLinkIDMap, CLinkIDMap *pOld2NewLinkIDMap, IEditorScene *pEditorScene, CObjectBaseController *pObjectController, IManipulator *pManipulator );
	};
};


