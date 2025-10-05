#pragma once

namespace NTimer
{
	typedef DWORD STime;
}
namespace NDb
{
	enum ENotificationEventType;
}
class CMapObj;

enum EVisualNotification
{
	EVNT_CAPTURE_KEY_OBJECT,
	EVNT_LOSS_KEY_OBJECT,
	EVNT_KEY_OBJECT_STATE,

	EVNT_RECEIVE_OBJECTIVE,
	EVNT_COMPLETE_OBJECTIVE,
	EVNT_FAIL_OBJECTIVE,
	EVNT_REMOVE_OBJECTIVE,
	EVNT_SELECT_OBJECTIVE,
	
	EVNT_CHECK_OBJECTIVES,
	EVNT_SHOW_OBJECTIVES,
	EVNT_HIDE_OBJECTIVES,
	EVNT_CHECK_OBJECTIVE_NOTIFY,

	EVNT_ENEMY_ARTILLERY_SEEN,
	EVNT_ENEMY_AA_SEEN,
	
	EVNT_REINFORCEMENT_AVAILABLE,
	EVNT_REINFORCEMENT_AVAILABLE_REPEAT,
	EVNT_REINFORCEMENT_ARRIVED,

	EVNT_SHOW_KEY_POINTS_OBSOLETE,
	EVNT_HIDE_KEY_POINTS_OBSOLETE,
	EVNT_ADD_KEY_POINT_OBSOLETE,
	EVNT_CLEAR_KEY_POINTS_OBSOLETE,
	EVNT_SELECT_KEY_POINT,

	EVNT_REINFORCEMENT_AVAILABLE_CANCEL,

	EVNT_CAMERA_BACK,

	EVNT_UPDATE_OBJECTIVE, // moving objectives etc.
	EVNT_UNITS_GIVEN,
};

struct IVisualNotifications : public CObjectBase
{
	struct SObjective
	{
		ZDATA
		std::wstring wszHeader;
		std::wstring wszDesc;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszHeader); f.Add(3,&wszDesc); return 0; }
	};
	
	struct SEventParams
	{
		ZDATA
		int nID;
		NDb::ENotificationEventType eEventType;
		std::vector<CVec2> positions;
		std::vector<int> ids; // misc info
		std::vector< CPtr<CMapObj> > objects;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nID); f.Add(3,&eEventType); f.Add(4,&positions); f.Add(5,&ids); f.Add(6,&objects); return 0; }
	};

	virtual bool Notify( EVisualNotification eType, int nID, const CVec2 &vPos ) = 0;
	virtual bool Notify( EVisualNotification eType, class CMapObj *pMO ) { return false; }
	virtual void Step( const NTimer::STime nDeltaTime, bool bAppActive ) = 0;
	
	virtual void OnBtn( const std::string &szSender, bool bRightBtn ) {}
	
	virtual void OnEvent( const SEventParams &params ) {}
	virtual void OnRemoveEvent( NDb::ENotificationEventType eEventType, int nID ) {}

	virtual void PlaceMarker( const CVec2 &vPos ) {}
};


