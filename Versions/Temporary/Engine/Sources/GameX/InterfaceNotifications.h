#pragma once

#include "B2_M1_World/Notifications.h"
#include "UISpecificB2/UISpecificB2.h"
#include "Misc/HashFuncs.h"

class CMapObj;
struct IAILogic;
namespace NDb
{
	struct SMissionObjective;
}

class CVisualNotifications : public IVisualNotifications
{
	OBJECT_NOCOPY_METHODS( CVisualNotifications );
	
	typedef std::unordered_map< NDb::ENotificationType, CDBPtr<NDb::SNotification>, SEnumHash > CEntries;
	
	struct SNotification
	{
		ZDATA
		int nID;
		NDb::EMinimapFigureType eFigure;
		CVec2 vPos;
		float fSize;
		float fAngle;
		NGfx::SPixel8888 color;
		float fTimeRemain;
		float fSpeed;
		float fRotationSpeed;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nID); f.Add(3,&eFigure); f.Add(4,&vPos); f.Add(5,&fSize); f.Add(6,&fAngle); f.Add(7,&color); f.Add(8,&fTimeRemain); f.Add(9,&fSpeed); f.Add(10,&fRotationSpeed); return 0; }
	};
	
	struct SKeyPoint
	{
		ZDATA
		CVec2 vPos;
		bool bSelected;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&vPos); f.Add(3,&bSelected); return 0; }
	};
	
public:	
	struct SEvent : public CObjectBase
	{
		OBJECT_BASIC_METHODS( SEvent );
	public:
		ZDATA
		CPtr<IWindow> pItemWnd;
		CDBPtr<NDb::SNotificationEvent> pDBEvent;
		float fVisibleTime;
		std::string szName;
		std::vector<CVec2> positions;
		int nID;
		std::vector< CPtr<CMapObj> > objects;
		std::wstring wszText;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pItemWnd); f.Add(3,&pDBEvent); f.Add(4,&fVisibleTime); f.Add(5,&szName); f.Add(6,&positions); f.Add(7,&nID); f.Add(8,&objects); f.Add(9,&wszText); return 0; }
	};
private:
	struct SMapPointer
	{
		ZDATA
		int nUniqueID;
		CDBPtr<NDb::SModel> pModel;
		CVec3 vPos;
		bool bPlaced;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nUniqueID); f.Add(3,&pModel); f.Add(4,&vPos); f.Add(5,&bPlaced); return 0; }
	public:
		~SMapPointer();
		
		void Init( int nUniqueID, const CVec3 &vPos );
		void Remove();
		void Move( const CVec3 &vPos );
	};
	typedef std::vector<SMapPointer> SMapPointers;

	ZDATA
	CPtr< IMiniMap > pMiniMap;
	CVec2 vMapSize;
	
	int nSelectedID;
	bool bShowObjectives;
	bool bNewObjective;
	float fNewObjectiveTime;
	std::list< SNotification >	notifications;
	CEntries entries;
	std::unordered_map<int, CPtr<CMapObj> > keyObjects;
	ZSKIP //list<int> newObjectives;
	ZSKIP //bool bIsObjectiveNotifyActive;
	ZSKIP//SObjective newObjectiveInfo;
	ZSKIP//bool bNewReinforcement;
	ZSKIP//float fNewReinforcementTime;
	int nSelectedKeyObject;
	ZSKIP//float fNewReinforcementStep;
	
	CPtr<IWindow> pParent;
	CPtr<IWindow> pItemTemplateWnd;

	std::list< CObj<SEvent> > events;
	int nFreeEvent;
	int nMaxEventCount;
	int nEventItemHeight;
	int nEventBottom;
	
	std::vector< CDBPtr<NDb::SNotificationEvent> > dbEvents;
	bool bEventTimerStopped;
	
	CVec3 vLastCameraPos;
	
	int nLastFreeID;
	std::vector<int> freeIDs;
	std::unordered_map<int,SMapPointers> pointers;
	CPtr<IAILogic> pAI;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pMiniMap); f.Add(3,&vMapSize); f.Add(4,&nSelectedID); f.Add(5,&bShowObjectives); f.Add(6,&bNewObjective); f.Add(7,&fNewObjectiveTime); f.Add(8,&notifications); f.Add(9,&entries); f.Add(10,&keyObjects); f.Add(16,&nSelectedKeyObject); f.Add(18,&pParent); f.Add(19,&pItemTemplateWnd); f.Add(20,&events); f.Add(21,&nFreeEvent); f.Add(22,&nMaxEventCount); f.Add(23,&nEventItemHeight); f.Add(24,&nEventBottom); f.Add(25,&dbEvents); f.Add(26,&bEventTimerStopped); f.Add(27,&vLastCameraPos); f.Add(28,&nLastFreeID); f.Add(29,&freeIDs); f.Add(30,&pointers); f.Add(31,&pAI); return 0; }

	NTimer::STime nAbsTime;
	std::vector < NTimer::STime > textMessageTimes;
	CVec2 vMPMarkerPos;
	bool bShowMPMarker;
	NTimer::STime timeMarkerExpire;
private:
	void InitPrivate();
	
	//{
	bool OnCaptureKeyObject( int nID, const CVec2 &vPos );
	bool OnLossKeyObject( int nID, const CVec2 &vPos );
	bool OnReceiveObjective( int nID, const CVec2 &vPos );
	bool OnCompleteObjective( int nID, const CVec2 &vPos );
	bool OnFailObjective( int nID, const CVec2 &vPos );
	bool OnRemoveObjective( int nID, const CVec2 &vPos );
	bool OnSelectObjective( int nID, const CVec2 &vPos );
	bool OnCheckObjectives( int nID, const CVec2 &vPos );
	bool OnShowObjectives( int nID, const CVec2 &vPos );
	bool OnHideObjectives( int nID, const CVec2 &vPos );

	bool OnArtillerySeen( int nID, const CVec2 &vPos );
	bool OnAASeen( int nID, const CVec2 &vPos );

	bool OnReinforcementAvailable( int nID, const CVec2 &vPos );
	bool OnReinforcementArrived( int nID, const CVec2 &vPos );
	bool OnUnitsGiven( int nID, const CVec2 &vPos );

	bool OnSelectKeyPoint( int nID );

	bool OnUpdateObjective( int nID );
	//}
	
	//{
	bool OnKeyObjectState( CMapObj *pMO );
	//}

	bool OnCameraBack();
	
	void AddNotification( int nID, const CVec2 &vPos, NDb::ENotificationType eType );
	void AddNotificationMain( int nID, const CVec2 &vPos, NDb::ENotificationType eType, 
		const std::wstring &wszCustomText );
	void AddNotificationMinimap( int nID, const CVec2 &vPos, NDb::ENotificationType eType );
	void AddObjectivePointers( int nID );
	void AddPointerModels( int nID, const std::vector<CVec3> &places );
	void RemoveObjectivePointers( int nID );
	void UpdateObjectivePointers( int nID );
	void AddObjectiveNotification( int nID, NDb::ENotificationType eType );
	void NewObjective( bool bChecked );
	void UpdateMarkers();
	
	void StepAbs( float fDeltaTime, bool bAppActive );

	void OnSerialize( IBinSaver &f );
	
	void InitEvents( IWindow *pParent );
	SEvent* CreateEventItem( const SEventParams &params );
	void CreateEventItemView( SEvent *pEvent );
	void UpdateEvents( float fDeltaTime );
	void RearrangeEvents();
	void EventLeftClick( SEvent *pEvent, bool *pErase );
	const NDb::SNotificationEvent* GetEvent( NDb::ENotificationEventType eEventType ) const;
	void AddEvent( const SEventParams &params );
	void MoveCamera( const CVec2 &vPos );
	const NDb::SMissionObjective* GetObjective( int nID );
	void RemoveEvent( NDb::ENotificationEventType eEventType, int nID );
	bool GetEventPos( CVec2 *pPos, const SEvent *pEvent ) const;
	void ObjectiveMove( int nID );
public:
	CVisualNotifications();
	CVisualNotifications( IWindow *pParent, IMiniMap *pMiniMap, const CVec2 &vMapSize, IAILogic *pAI );
	
	//{ IVisualNotifications
	bool Notify( EVisualNotification eType, int nID, const CVec2 &vPos );
	bool Notify( EVisualNotification eType, CMapObj *pMO );
	void Step( const NTimer::STime nDeltaGameTime, bool bAppActive );

	void OnBtn( const std::string &szSender, bool bRightBtn );

	void OnEvent( const SEventParams &params );
	void OnRemoveEvent( NDb::ENotificationEventType eEventType, int nID );

	void PlaceMarker( const CVec2 &vPos );
	//}
};


