#if !defined(__MAPINFO_EDITOR_DATA__OBJECT_INFO_COLLECTOR__)
#define __MAPINFO_EDITOR_DATA__OBJECT_INFO_COLLECTOR__
#pragma once

#include "MapInfoEditorData_Consts.h"
#include "MapInfoEditorData_Clipboard.h"
#include "MapInfoEditorData_Selection.h"
#include "MapInfoEditorData_ObjectInfo.h"
#include "..\MapEditorLib\Tools_IndexCollector.h"
#include "..\MapEditorLib\Tools_FreeIDCollector.h"
#include "EditorUpdatableWorld.h"

class CMapInfoEditor;

namespace NMapInfoEditor
{
	struct SObjectInfoCollector
	{
		typedef hash_map<UINT, CPtr<SObjectInfo> > CObjectInfoMap;
		typedef hash_map<string, UINT> CObjectTypeMap;
		typedef hash_map<UINT, UINT> CSceneIDMap;
		typedef hash_map<UINT, UINT> CLinkIDMap;
		//
		CObjectInfoMap objectInfoMap;												// Список SObjectInfo
		CLinkIDMap linkIDMap;																// Список LinkID->SObjectInfoID
		CSceneIDMap sceneIDMap;															// Список SceneID->SObjectInfoID
		//
		CIndexCollector<UINT> linkIDToIndexCollector;				// Список индексов объектов ( прямое отображение LinkID->Index )
		CIndexCollector<UINT> bridgeIDToIndexCollector;			// Список индексов мостов ( прямое отображение ID->Index )
		CIndexCollector<UINT> trenchIDToIndexCollector;			// Список индексов окопов ( прямое отображение ID->Index )
		CIndexCollector<UINT> spotIDToIndexCollector;				// Список индексов spots ( прямое отображение ID->Index )
		//
		CFreeIDCollector objectInfoIDCollector;							// для создания и идентификации новых SMapOnjectInfo
		CFreeIDCollector linkIDCollector;										// для создания и идентификации новых object link ID
		CFreeIDCollector bridgeIDCollector;									// для создания и идентификации новых bridge
		CFreeIDCollector trenchIDCollector;									// для создания и идентификации новых entrenchment
		CFreeIDCollector sceneIDCollector;									// для создания и идентификации новых scene object
		//
		CPtr<IManipulator> pPropertyManipulator;
		//
		SObjectSelection objectSelection;
		SObjectClipboard objectClipboard;
		//
		CTRect<float> mapSize;
		NDb::ESeason eSeason;
		NDb::EDayNight eDayNight;
		const class CMapInfoEditor *pMapInfoEditor;					// required for Spots
		//
		CObj<CEditorUpdatableWorld> pEditorUpdatableWorld;

		SObjectInfoCollector();
		//
		void Clear();
		void SetMapInfoEditor( const CMapInfoEditor *_pMapInfoEditor );
		//
		// вспомогательные методы
		UINT GetLinkIDByObjectIndex( int nObjectIndex, IManipulator *pManipulator, bool bObject );
		// методы общей загрузки карты
		bool PostLoad( IEditorScene *pEditorScene, IManipulator *pManipulator );

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// методы добавления и удаления элементов
		template<class TObjectInfo>
		TObjectInfo* Insert( TObjectInfo *pCreatedObjectInfo, UINT *pnObjectInfoID )
		{
			TObjectInfo *pObjectInfo = pCreatedObjectInfo;
			// создаем если необходимо
			if ( pObjectInfo == 0 )
			{
				pObjectInfo = new TObjectInfo();
			}
			// заполняем необходимую информацию
			pObjectInfo->pObjectInfoCollector = this;
			pObjectInfo->nObjectInfoID = objectInfoIDCollector.LockID();
			// заносим к массив
			objectInfoMap[pObjectInfo->nObjectInfoID] = pObjectInfo;
			if ( pnObjectInfoID )
			{
				( *pnObjectInfoID ) = pObjectInfo->nObjectInfoID;
			}
			return pObjectInfo;
		}
		SObjectInfo* Insert( SObjectInfo *pCreatedObjectInfo, UINT *pnObjectInfoID )
		{
			if ( pCreatedObjectInfo == 0 )
			{
				return 0;
			}
			// заполняем необходимую информацию
			pCreatedObjectInfo->pObjectInfoCollector = this;
			pCreatedObjectInfo->nObjectInfoID = objectInfoIDCollector.LockID();
			// заносим к массив
			objectInfoMap[pCreatedObjectInfo->nObjectInfoID] = pCreatedObjectInfo;
			if ( pnObjectInfoID )
			{
				( *pnObjectInfoID ) = pCreatedObjectInfo->nObjectInfoID;
			}
			return pCreatedObjectInfo;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		void Remove( UINT nObjectInfoID,
								 bool bUpdateScene, IEditorScene *pEditorScene,
								 bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator )
		{
			CObjectInfoMap::iterator posObject = objectInfoMap.find( nObjectInfoID );
			if ( posObject != objectInfoMap.end() )
			{
				if ( posObject->second != 0 )
				{
					posObject->second->Remove( bUpdateScene, pEditorScene, bUpdateDB, pObjectController, pManipulator );
				}
				objectInfoIDCollector.FreeID( posObject->first );
				objectInfoMap.erase( posObject );
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		const NDb::SMapObjectInfo* SObjectInfoCollector::GetObjectStatusBarParams( int nSceneID );

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		void Pick( list<int> *pEditorSceneIDList, const CTPoint<int> &rMousePoint ) const;
		void Pick( list<int> *pEditorSceneIDList, const CTRect<int> &rFrame ) const;
		UINT Pick( UINT nSceneID ) const;
		//
		SObjectInfo* GetObjectInfo( const UINT nObjectInfoID );
		SObjectInfo* GetObjectInfoByLinkID( const UINT nLinkID );
		SObjectInfo* GetObjectInfoBySceneID( const UINT nSceneID );
		//
		bool UpdateDB( UINT nObjectInfoID, bool bUpdateLinkedObjects, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		//
		void Draw( UINT nObjectInfoID, CSceneDrawTool *pEditorSceneDrawTool );
		//
		bool Move( UINT nObjectInfoID,
							 const SObjectEditInfo *pObjectEditInfo, const CVec3 &rvNewPosition,
							 bool bMoveLinkedObjects,
							 bool bUpdateScene, IEditorScene *pEditorScene,
							 bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		bool Rotate( UINT nObjectInfoID,
								 const SObjectEditInfo *pObjectEditInfo, float fNewDirection,
								 bool bRotateLinkedObjects,
								 bool bUpdateScene, IEditorScene *pEditorScene,
								 bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		//		
		bool CheckLinkCapability( UINT nObjectInfoID, UINT nLinkToSceneID );
		bool InsertLink( UINT nObjectInfoID, UINT nLinkToSceneID, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		bool RemoveLinks( UINT nObjectInfoID, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		bool RemoveLinkTo( UINT nObjectInfoID, CObjectBaseController *pObjectController, IManipulator *pManipulator );

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// work with selection
		void GetLinkedObjectInfoIDList( CObjectInfoIDSet *pLinkedObjectInfoIDSet, const CObjectInfoIDSet &rObjectInfoIDSet, CObjectInfoIDSet *pAlreadyInUseObjectInfoIDSet );
		void AddLinkedObjectsToSelection();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		template<class Type>
		void CreateSelection( const Type &rObjectSceneIDList )
		{
			ClearSelection();
			InsertToSelection( rObjectSceneIDList, false );
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		template<class Type>
		void InsertToSelection( const Type &rObjectSceneIDList, bool bAutoRemove )
		{
			// проверим, что некоторые элементы необходимо добавить
			CObjectInfoIDSet newObjectInfoIDSet;
			CObjectInfoIDSet existingObjectInfoIDSet;
			for ( Type::const_iterator itObjectSceneID = rObjectSceneIDList.begin(); itObjectSceneID != rObjectSceneIDList.end(); ++itObjectSceneID )
			{
				const UINT nObjectInfoID = Pick( *itObjectSceneID );
				if ( objectSelection.objectSelectionPartMap.find( nObjectInfoID ) == objectSelection.objectSelectionPartMap.end() )
				{
					InsertHashSetElement( &newObjectInfoIDSet, nObjectInfoID );
				}
				else
				{
					InsertHashSetElement( &existingObjectInfoIDSet, nObjectInfoID );
				}
			}
			if ( !newObjectInfoIDSet.empty() )
			{
				// установим абсолютные координаты
				objectSelection.MakeAbsolute();
				//
				// добавим элементы
				for ( CObjectInfoIDSet::const_iterator itNewObjectInfoID = newObjectInfoIDSet.begin(); itNewObjectInfoID != newObjectInfoIDSet.end(); ++itNewObjectInfoID )
				{
					const UINT nObjectInfoID = itNewObjectInfoID->first;
					if ( SObjectInfo *pObjectInfo = GetObjectInfo( nObjectInfoID ) )
					{
						SObjectSelectionPart objectSelectionPart;
						objectSelectionPart.vPosition = pObjectInfo->vPosition;
						objectSelectionPart.fDirection = pObjectInfo->fDirection;
						// заносим элемент в структуру данных объекта
						objectSelection.objectSelectionPartMap[nObjectInfoID] = objectSelectionPart;
					}
				}
				// установим относительные координаты
				objectSelection.MakeRelative();
			}
			else if ( bAutoRemove )
			{
				// установим абсолютные координаты
				objectSelection.MakeAbsolute();
				// удалим элементы
				for ( CObjectInfoIDSet::const_iterator itExistingObjectInfoID = existingObjectInfoIDSet.begin(); itExistingObjectInfoID != existingObjectInfoIDSet.end(); ++itExistingObjectInfoID )
				{
					const UINT nObjectInfoID = itExistingObjectInfoID->first;
					CObjectSelectionPartMap::iterator posObjectSelectionPart = objectSelection.objectSelectionPartMap.find( nObjectInfoID );
					if ( posObjectSelectionPart != objectSelection.objectSelectionPartMap.end() )
					{
						objectSelection.objectSelectionPartMap.erase( posObjectSelectionPart );
					}
				}
				// установим относительные координаты
				objectSelection.MakeRelative();
			}
			AddLinkedObjectsToSelection();
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		template<class Type>
		void RemoveFromSelection( const Type &rObjectSceneIDList )
		{
			// установим абсолютные координаты
			objectSelection.MakeAbsolute();
			// удалим элементы
			for ( Type::const_iterator itObjectSceneID = rObjectSceneIDList.begin(); itObjectSceneID != rObjectSceneIDList.end(); ++itObjectSceneID )
			{
				const UINT nObjectInfoID = Pick( *itObjectSceneID );
				CObjectSelectionPartMap::iterator posObjectSelectionPart = objectSelection.objectSelectionPartMap.find( nObjectInfoID );
				if ( posObjectSelectionPart != objectSelection.objectSelectionPartMap.end() )
				{
					objectSelection.objectSelectionPartMap.erase( posObjectSelectionPart );
				}
			}
			// установим относительные координаты
			objectSelection.MakeRelative();
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		template<class Type>
		void CreateSelection( const Type &rvPos0, const Type &rvPos1 )
		{
			ClearSelection();
			InsertToSelection( rvPos0, rvPos1, false );
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		template<class Type>
		void InsertToSelection( const Type &rvPos0, const Type &rvPos1, bool bAutoRemove )
		{
			CTRect<float> selectionRect( rvPos0.x, rvPos0.y, rvPos1.x, rvPos1.y );
			selectionRect.Normalize();

			// проверим, что некоторые элементы необходимо добавить
			CObjectInfoIDSet newObjectInfoIDSet;
			CObjectInfoIDSet existingObjectInfoIDSet;
			for( CObjectInfoMap::iterator itObject = objectInfoMap.begin(); itObject != objectInfoMap.end(); ++itObject )
			{
				bool bInside = false;
				for ( SObjectInfo::CSceneElementMap::const_iterator itSceneElement = itObject->second->sceneElementMap.begin(); itSceneElement != itObject->second->sceneElementMap.end(); ++itSceneElement )
				{
					CVec3 vObjectScenePosition = itSceneElement->second.GetPosition( itObject->second->vPosition );
					bInside = selectionRect.IsInside( vObjectScenePosition.x, vObjectScenePosition.y );
					if ( bInside )
					{
						break;
					}
				}
				if ( bInside )
				{
					const UINT nObjectInfoID = itObject->first;
					if ( objectSelection.objectSelectionPartMap.find( nObjectInfoID ) == objectSelection.objectSelectionPartMap.end() )
					{
						InsertHashSetElement( &newObjectInfoIDSet, nObjectInfoID );
					}
					else
					{
						InsertHashSetElement( &existingObjectInfoIDSet, nObjectInfoID );
					}
				}
			}
			//
			if ( !newObjectInfoIDSet.empty() )
			{
				// установим абсолютные координаты
				objectSelection.MakeAbsolute();
				//
				// добавим элементы
				for ( CObjectInfoIDSet::const_iterator itNewObjectInfoID = newObjectInfoIDSet.begin(); itNewObjectInfoID != newObjectInfoIDSet.end(); ++itNewObjectInfoID )
				{
					const UINT nObjectInfoID = itNewObjectInfoID->first;
					if ( SObjectInfo *pObjectInfo = GetObjectInfo( nObjectInfoID ) )
					{
						SObjectSelectionPart objectSelectionPart;
						objectSelectionPart.vPosition = pObjectInfo->vPosition;
						objectSelectionPart.fDirection = pObjectInfo->fDirection;
						// заносим элемент в структуру данных объекта
						objectSelection.objectSelectionPartMap[nObjectInfoID] = objectSelectionPart;
					}
				}
				// установим относительные координаты
				objectSelection.MakeRelative();
			}
			else if ( bAutoRemove )
			{
				// установим абсолютные координаты
				objectSelection.MakeAbsolute();
				// удалим элементы
				for ( CObjectInfoIDSet::const_iterator itExistingObjectInfoID = existingObjectInfoIDSet.begin(); itExistingObjectInfoID != existingObjectInfoIDSet.end(); ++itExistingObjectInfoID )
				{
					const UINT nObjectInfoID = itExistingObjectInfoID->first;
					CObjectSelectionPartMap::iterator posObjectSelectionPart = objectSelection.objectSelectionPartMap.find( nObjectInfoID );
					if ( posObjectSelectionPart != objectSelection.objectSelectionPartMap.end() )
					{
						objectSelection.objectSelectionPartMap.erase( posObjectSelectionPart );
					}
				}
				// установим относительные координаты
				objectSelection.MakeRelative();
			}
			AddLinkedObjectsToSelection();
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		template<class Type>
		void RemoveFromSelection( const Type &rvPos0, const Type &rvPos1 )
		{
			CTRect<float> selectionRect( rvPos0.x, rvPos0.y, rvPos1.x, rvPos1.y );
			selectionRect.Normalize();
			// установим абсолютные координаты
			objectSelection.MakeAbsolute();
			// удалим элементы
			for( CObjectInfoMap::iterator itObject = objectInfoMap.begin(); itObject != objectInfoMap.end(); ++itObject )
			{
				bool bInside = false;
				for ( SObjectInfo::CSceneElementMap::const_iterator itSceneElement = itObject->second.sceneElementMap.begin(); itSceneElement != itObject->second.sceneElementMap.end(); ++itSceneElement )
				{
					CVec3 vObjectScenePosition = itSceneElement->second.GetPosition( itObject->second.vPosition );
					bInside = selectionRect.IsInside( vObjectScenePosition.x, vObjectScenePosition.y );
					if ( bInside )
					{
						break;
					}
				}
				if ( bInside )
				{
					const UINT nObjectInfoID = itObject->first;
					CObjectSelectionPartMap::iterator posObjectSelectionPart = objectSelection.objectSelectionPartMap.find( nObjectInfoID );
					if ( posObjectSelectionPart != objectSelection.objectSelectionPartMap.end() )
					{
						objectSelection.objectSelectionPartMap.erase( posObjectSelectionPart );
					}
				}
			}
			// установим относительные координаты
			objectSelection.MakeRelative();
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		template<class Type>
		bool IsInSelection( const Type &rObjectSceneIDList ) const
		{
			for ( Type::const_iterator itObjectSceneID = rObjectSceneIDList.begin(); itObjectSceneID != rObjectSceneIDList.end(); ++itObjectSceneID )
			{
				const UINT nObjectInfoID = Pick( *itObjectSceneID );
				CObjectSelectionPartMap::const_iterator posObjectSelectionPart = objectSelection.objectSelectionPartMap.find( nObjectInfoID );
				if ( posObjectSelectionPart == objectSelection.objectSelectionPartMap.end() )
				{
					return false;
				}
			}
			return true;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline bool IsInSelection( UINT nSceneID ) const
		{
			const UINT nObjectInfoID = Pick( nSceneID );
			CObjectSelectionPartMap::const_iterator posObjectSelectionPart = objectSelection.objectSelectionPartMap.find( nObjectInfoID );
			if ( posObjectSelectionPart == objectSelection.objectSelectionPartMap.end() )
			{
				return false;
			}
			return true;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline bool IsInSelection( int nSceneID ) const
		{
			const UINT nObjectInfoID = Pick( nSceneID );
			CObjectSelectionPartMap::const_iterator posObjectSelectionPart = objectSelection.objectSelectionPartMap.find( nObjectInfoID );
			if ( posObjectSelectionPart == objectSelection.objectSelectionPartMap.end() )
			{
				return false;
			}
			return true;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		const CVec3& GetSelectionPosition() { return objectSelection.vPosition; }
		const float GetSelectionDirection() { return objectSelection.fDirection; }
		const SObjectSelection::ESelectionType GetSelectionType() { return objectSelection.eSelectionType; }
		void SetSelectionType(SObjectSelection::ESelectionType eSelectionType ) { objectSelection.eSelectionType = eSelectionType; }
		const SObjectSelection::ESelectionType PickSelection( const CVec3 &rvTerrainPos );
		void BackupSelectionPosition() { objectSelection.BackupPosition(); }
		void RollbackSelectionPosition( const SObjectEditInfo *pObjectEditInfo,
																		IEditorScene *pEditorScene );
		inline bool IsSelectionEmpty() { return objectSelection.IsEmpty(); }
		inline void ClearSelection() { objectSelection.Clear(); }
		inline void ResetPickSelection() { objectSelection.ResetPickSelection(); }

		void RemoveSelection( IEditorScene *pEditorScene, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		bool UpdateDBSelection( bool bUpdateLinkedObjects, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		void DrawSelection( CSceneDrawTool *pEditorSceneDrawTool );
		void ClearShootAreas();
		void DrawShootAreas( CSceneDrawTool *pEditorSceneDrawTool );

		//
		bool MoveSelection( const SObjectEditInfo *pObjectEditInfo, const CVec3 &rvNewPosition, bool bExactPosition, bool bIgnoreDifference, bool bSetToZero,
												bool bUpdateScene, IEditorScene *pEditorScene,
												bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		bool RotateSelection(	const SObjectEditInfo *pObjectEditInfo, const float fNewDirection, bool bExactDirection, bool bIgnoreDifference,
													bool bUpdateScene, IEditorScene *pEditorScene,
													bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		//
		bool CheckSelectionLinkCapability( UINT nLinkToSceneID );
		bool InsertSelectionLink( UINT nLinkToSceneID, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		bool RemoveSelectionLinks( CObjectBaseController *pObjectController, IManipulator *pManipulator );
		bool RemoveSelectionLinkTo( CObjectBaseController *pObjectController, IManipulator *pManipulator );
		//
		void ShowSelectionPropertyManipulator( IManipulator *pManipulator, const SObjectSet &rObjectSet );
		void HideSelectionPropertyManipulator();
		//
		void UpdateObjectByController( int nObjectIndex, UINT nFlags, IEditorScene *pEditorScene, IManipulator *pManipulator );
		void UpdateSpotByController( int nSpotIndex, UINT nFlags, IEditorScene *pEditorScene, IManipulator *pManipulator );
		void PostLoadByController( const CIndicesList &rIndices, IEditorScene *pEditorScene, IManipulator *pManipulator, bool bObject );
		void UpdateSelectionByController();
		//
		int CopyClipboard();
		bool PasteClipboard( const SObjectEditInfo *pObjectEditInfo, const CVec3 &rvPastePosition, bool bExactPosition, bool bIgnoreDifference, bool bSetToZero,
												 bool bUpdateScene, IEditorScene *pEditorScene,
												 bool bUpdateDB, CObjectBaseController *pObjectController, IManipulator *pManipulator );
		//
		void ShowClipboard( const SObjectEditInfo *pObjectEditInfo, const CVec3 &rvPastePosition, bool bExactPosition, bool bIgnoreDifference, bool bSetToZero,
												IEditorScene *pEditorScene, IManipulator *pManipulator );
		void MoveClipboard( const SObjectEditInfo *pObjectEditInfo, const CVec3 &rvPastePosition, bool bExactPosition, bool bIgnoreDifference, bool bSetToZero,
												IEditorScene *pEditorScene, IManipulator *pManipulator );
		void HideClipboard( IEditorScene *pEditorScene, IManipulator *pManipulator );
		//
		inline bool IsClipboardEmpty() { return objectClipboard.IsEmpty(); }
		void ClearClipboard();
		//
		void Trace();
	};
};

#endif // !defined(__MAPINFO_EDITOR_DATA__OBJECT_INFO_COLLECTOR__)

