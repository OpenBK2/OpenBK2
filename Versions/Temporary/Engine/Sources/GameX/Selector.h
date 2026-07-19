#pragma once

#include "B2_M1_World/MapObj.h"
#include "Selector.h"

typedef	std::list< CPtr<CMOSelectable> > CMapObjectsList;
typedef std::vector< std::pair<int, int> > CUnitGroups;
typedef std::vector< CPtr<CMOSelectable> > CMapObjectsVector;
struct SShootAreas;

enum ESelectionState
{
	ssNone,
	ssUnits,
	ssEnemy,
	ssBuilding,
};

struct SSelectionGroup 
{
private:
	CMapObjectsList objList;
	int	nID;
	CMapObjectsList objListHidden;
public:
	//
	void Add( CMOSelectable *pMO )
	{
		if ( !pMO )
			return;
		NI_ASSERT( find( objList.begin(), objList.end(), pMO ) == objList.end(), "Duplicate object" );
		objList.push_back( pMO );
	}
	void Remove( CMOSelectable *pMO )
	{
		objList.remove( pMO );
		objListHidden.remove( pMO );
	}
	void Hide( CMOSelectable *pMO )
	{
		if ( !pMO )
			return;
		CMapObjectsList::iterator it = find( objList.begin(), objList.end(), pMO );
		if ( it == objList.end() )
			return;
		objList.erase( it );
		objListHidden.remove( pMO ); // avoid duplicate
		objListHidden.push_back( pMO );
	}
	void UnHide( CMOSelectable *pMO )
	{
		if ( !pMO )
			return;
		CMapObjectsList::iterator it = find( objListHidden.begin(), objListHidden.end(), pMO );
		if ( it == objListHidden.end() )
			return;
		objListHidden.erase( it );
		objList.remove( pMO ); // avoid duplicate
		objList.push_back( pMO );
	}
	void Clear()
	{
		objList.clear();
		objListHidden.clear();
	}
	bool IsEmpty() const { return objList.empty(); }
	int GetID() const { return nID; }
	void SetID( int _nID ) { nID = _nID; }
	const CMapObjectsList& GetList() const { return objList; }
	bool HasObject( CMOSelectable *pMO ) const
	{
		CMapObjectsList::const_iterator it = find( objList.begin(), objList.end(), pMO );
		if ( it != objList.end() )
			return true;
		CMapObjectsList::const_iterator it2 = find( objListHidden.begin(), objListHidden.end(), pMO );
		if ( it2 != objListHidden.end() )
			return true;
		return false;
	}

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &objList );
		saver.Add( 2, &nID );
		saver.Add( 3, &objListHidden );
		return 0;
	}
};

class CSelector : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CSelector );

private:
	static CUserActions MakeCommonActions();
	static CUserActions MakeMultiClassActions();
	static bool IsAllUnitsCommand( EActionCommand eCommand );

	static void GetActions( CUserActions *pActions, const std::vector< CPtr<CMOSelectable> > &objects, EActionsType eActions );
	static void GetPossibleActions( CUserActions *pActions, const std::vector< CPtr<CMOSelectable> > &objects );
	static void GetEnabledActions( CUserActions *pActions, const std::vector< CPtr<CMOSelectable> > &objects, EActionsType eActions );
	static void GetDisabledActions( CUserActions *pActions, const std::vector< CPtr<CMOSelectable> > &objects, EActionsType eActions );
	static int GetAbilityTier( const std::vector< CPtr<CMOSelectable> > &objects, NDb::EUserAction eAction );
	static const NDb::SUnitSpecialAblityDesc* GetAbilityDesc( const std::vector< CPtr<CMOSelectable> > &objects, NDb::EUserAction eAction );
private:
	struct SObjectsSort
	{
		bool operator()( const CMapObj *pMapObject1, const CMapObj *pMapObject2 ) const 
		{ 
			CUserActions actions1, actions2;
			pMapObject1->GetPossibleActions( &actions1 );
			pMapObject2->GetPossibleActions( &actions2 );
			if ( actions1 == actions2 )
			{
				if ( pMapObject1->GetTypeID() == pMapObject2->GetTypeID() )
				{
					if ( pMapObject1->GetStats() == pMapObject2->GetStats() )
						return pMapObject1->GetID() > pMapObject2->GetID();
					else
						return pMapObject1->GetStats()->GetDBID().ToString() > pMapObject2->GetStats()->GetDBID().ToString();
				}
				else
					return pMapObject1->GetTypeID() > pMapObject2->GetTypeID();
			}
			else
				return actions1 > actions2;
		}
	};

	struct SSlot
	{
		ZDATA
		ZSKIP //CPtr<CMOSelectable> pSO;
		ZSKIP //int nIndex;
		ZSKIP //int nCount;
		std::vector< CPtr<CMOSelectable> > objects;
		ZEND int operator&( IBinSaver &f ) { f.Add(5,&objects); return 0; }
	};
	typedef std::vector< SSlot > CSlotVector;

	struct SSameSlotCompare
	{
		CPtr<CMOSelectable> pSO;
		
		SSameSlotCompare( CMOSelectable *_pSO ) { pSO = _pSO; }
		bool operator()( const SSlot &slot ) const
		{
			return IsSameType( pSO, slot.objects[0] );
		}
	};

	struct SSlotsSort
	{
		bool operator()( const SSlot &slot1, const SSlot &slot2 ) const 
		{ 
			CUserActions actions1, actions2;
			GetActions( &actions1, slot1.objects, ACTIONS_BY );
			GetActions( &actions2, slot2.objects, ACTIONS_BY );
			if ( actions1 == actions2 )
			{
				CMOSelectable *pSO1 = slot1.objects[0];
				CMOSelectable *pSO2 = slot2.objects[0];
				if ( pSO1->GetTypeID() == pSO2->GetTypeID() )
				{
					if ( pSO1->GetStats() == pSO2->GetStats() )
						return pSO1->GetID() > pSO2->GetID();
					else
						return pSO1->GetStats()->GetDBID().ToString() > pSO2->GetStats()->GetDBID().ToString();
				}
				else
					return pSO1->GetTypeID() > pSO2->GetTypeID();
			}
			else
				return actions1 > actions2;
		}
	};
	
	//
	CMapObjectsVector				objList;
	CMapObjectsList					newObjList;
	std::vector<SSelectionGroup>	groups;
	ESelectionState					eSelectionState;
	CUnitGroups							vAbilityGroups;
	int											nCurrentAbilityGroup;
	CUserActions						uaCurrentAbility;
//	CUserActions						uaAllUnitActions;
	const CUserActions			uaAllCommonActions;
	const CUserActions			uaMultiClassActions;
	int											nMaxUnitSlots;
	int											nMaxUnitPerSlot;
	//CPtr<CMOSelectable>			pSuperActive;
	bool										bUnloadMode;
	bool										bSupportSingleSelection;
	CPtr<CMapObj>						pPreselectedObject;
	CSlotVector							slots;
	//SSlot										superActive;
	std::vector< CPtr<CMOSelectable> > superActives;
	// optimisation
	bool bNeedUpdateSelectedUnits;
	bool bNeedUpdatePreselectedObject;
	bool bNeedUpdateAbilityIcons;
	bool bNeedUpdateActionButtons;
	bool bNeedUpdateSelectionInterior;
private:
	//
	void SetSelectionGroup( const int nIndex );
	void AddObj( CMOSelectable *pMO );
	void CalcSlots();
	void CalcSlots( CSlotVector *pSlots, const CMapObjectsVector &objects ) const;
	bool CanAddObject( CMOSelectable *pSO ) const;
	int CalcAbilityGroups();
	void UpdateAbilityIcons( const std::vector< CPtr<CMOSelectable> > &objects ) const;
	
	// уведомим об изменениях самой селекции
	void SendUpdateSelection();
	// уведомим об изменениях внутри селекции
	void SendUpdateSelectionInterior();

	void UpdateUnitsIcons( bool bPreselection );
	void SendUpdateActionButtons();

	bool SelectPrivate( CMapObj *pMO, bool bSelect, bool bDeath );

	void GetActionsPrivate( CUserActions *pActions, bool bEnabledOnly );
	
	void AddObjectToGroup( CMOSelectable *pMO, int nGroup );
	void RemoveObjectFromGroup( CMOSelectable *pMO, int nGroup );
	int FindObjectGroup( CMOSelectable *pMO ) const;
	void UpdateSelectedUnits();
public:
	static bool IsSameType( const CMapObj *pMO, const CMapObj *pMO2 );
public:
	CSelector();
	~CSelector()	{};
	void Segment();
	//
	int GetSelection() const	{ return objList.size(); }
	int GetSelection( std::vector<CMOSelectable*> *pBuffer ) const;
	// возвращает все объекты, имеющие собственные координаты
	// на экране (разбивая подразделения на юниты при необходимости)
	void GetSelectionMembers( std::vector<CMOSelectable*> *pBuffer ) const;
	//
	bool CanSelect( const CMapObj *pMO ) const { return (pMO && pMO->CanSelect()); }
  
	bool Select( CMapObj *pMO, bool bSelect );
	bool DeSelectDead( CMapObj *pMO );

	bool IsSelected( const CMapObj *pMO ) const;
	bool IsSelectedOrInSelectedContainer( const CMapObj *pMO ) const;
	bool IsPreselectedOrInPreselectedContainer( const CMapObj *pMO ) const;
	bool IsActive( const CMOSelectable *pMO ) const;
	bool IsSuperActive( const CMOSelectable *pMO ) const;

	bool IsUnloadMode() const { return bUnloadMode;	}

	// update selection by user actions (with mode switching)
	void DoneSelection( const bool bPreserveGroup, bool bKeepNumbers );
	// update selection by internal events
	void UpdateSelection( const bool bPreserveGroup, bool bKeepNumbers );
	bool IsEmpty() const { return objList.empty(); }
	
	void SetPreselection( CMapObj *pMO );

	void AssignSelectionToGroup( int nIndex );
	void AssignGroupToSelection( int nIndex, bool bAddToCurrent );
	void RemoveFromGroups( CMOSelectable *pMO );
	void HideFromGroups( CMOSelectable *pMO );
	void UnHideForGroups( CMOSelectable *pMO );
	void ReplaceSelectionGroup( CMOSelectable *pMOPattern, CMOSelectable *pMO );
	int FindSelectionGroup( CMOSelectable *pMO ) const;
	// возвращает все объекты в группе, имеющие собственные координаты
	// на экране (разбивая подразделения на юниты при необходимости)
	void GetGroupMembers( int nIndex, std::vector<CMOSelectable*> *pMembers );

	bool DoGroupCommand( class CCommandsSender *pCommandsSender, const struct SAIUnitCmd *pCommand, bool bPlaceInQueue );
	bool DoGroupCommandAutocast( class CCommandsSender *pCommandsSender, const struct SAIUnitCmd *pCommand, bool bPlaceInQueue );
	
	void SetShowAreas( EActionNotify eType, bool bOn );
	void GetAreas( SShootAreas *pAreas );

	void SelectSameSlots( const int nSlot );
	void SelectSameType( int nSlot );
	void SelectNextGroup();
	void SelectPrevGroup();
	void UnselectSlot( int nSlot );

	void Empty();
	// получить список action'ов, теоретически доступных для unit'ов в текущем выделении
	void GetActions( CUserActions *pActions );
	// получить список action'ов, недоступных для unit'ов в текущем выделении
	void GetDisabledActions( CUserActions *pActions );
	// получить список разрешенных действий (не совсем то же, что все действия без недоступных)
	void GetEnabledActions( CUserActions *pActions );
	void GetEnabledSuperActiveActions( CUserActions *pActions );
	void SetMaxUnits( const int nMaxUnitSlots, const int nMaxUnitPerSlot );
	int GetAbilityTier( NDb::EUserAction eAction ) const;
	const NDb::SUnitSpecialAblityDesc* GetAbilityDesc( NDb::EUserAction eAction ) const;

	void DoUpdateSelectedUnits();
	void DoUpdatePreselectedUnits();

	void DoUpdateSpecialAbility( CMapObj *pMO );
	void DoUpdateStats( CMapObj *pMO );

	ESelectionState GetSelectionState() const { return eSelectionState; }
	
	const CMOSelectable* GetFirstSlotUnit( int nSlot ) const;
	
	// убирает недопустимые действия для использования на указанном объекте 
	// (проверка на сухопутные, воздушные, морские юниты)
	void FilterActions( CUserActions *pActionsBy, CMapObj *pMO ) const;

	void AfterLoad();
	
	int operator&( IBinSaver &saver );
};


