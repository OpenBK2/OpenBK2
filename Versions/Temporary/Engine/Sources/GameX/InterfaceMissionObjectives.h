#ifndef __INTERFACE_MISSION_OBJECTIVES_H__
#define __INTERFACE_MISSION_OBJECTIVES_H__

#pragma ONCE

#include "InterfaceScreenBase.h"

class CInterfaceMissionObjectives : public CInterfaceScreenBase
{
	OBJECT_NOCOPY_METHODS( CInterfaceMissionObjectives );
public:
	class CReactions : public IProgrammedReactionsAndChecks
	{
		OBJECT_NOCOPY_METHODS(CReactions);
		CPtr<IWindow> pScreen;
		CPtr<CInterfaceMissionObjectives> pInterface;
	public:
		CReactions() {  }
		~CReactions() 
		{  
		}
		CReactions( IWindow *_pScreen, CInterfaceMissionObjectives *_pInterface ) : 
			pScreen( _pScreen ), pInterface( _pInterface )
		{
		}
		virtual bool Execute( const string &szSender, const string &szReaction );
		virtual int Check( const string &szCheckName ) const;

		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &pScreen );
			saver.Add( 2, &pInterface );
			return 0;
		}
	};
private:
	struct SObjective
	{
		ZDATA
		CPtr<IWindow> pWnd;
		int nID;
		CPtr<IButton> pButton;
		bool bAutoSelect;
		CPtr<ITextView> pText;
		CPtr<IButton> pStatus;
		wstring wszHeader;
		wstring wszDescBrief;
		wstring wszDescFull;
		int nButtonIndex;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWnd); f.Add(3,&nID); f.Add(4,&pButton); f.Add(5,&bAutoSelect); f.Add(6,&pText); f.Add(7,&pStatus); f.Add(8,&wszHeader); f.Add(9,&wszDescBrief); f.Add(10,&wszDescFull); f.Add(11,&nButtonIndex); return 0; }
		
		SObjective() : nID( -1 ), nButtonIndex( 0 ) {}
	};
	
	CObj<CReactions> pReactions;
	
	CPtr<IWindow> pMainWnd;
	CPtr<IWindow> pDescPanel;

	vector<SObjective> objectives;
	bool bIsModal;
	
	int nPrevSelectionID;
	
	bool bCameraBack;
	CPtr<IScrollableContainer> pDescScrollableWnd;
	CPtr<ITextView> pDescName;
	CPtr<ITextView> pDescBrief;
	CPtr<ITextView> pDescFull;
	CPtr<IButton> pDescStatus;
	CPtr<IWindow> pObjectivesPanel;
	CPtr<IWindow> pNextObjective;
	CPtr<IWindow> pObjectiveItem;
	CPtr<ITextView> pHeader;
	wstring wszObjectivesSummary;
	wstring wszMissionName;
	wstring wszMissionBriefing;
	wstring wszObjectivesSummaryTooltip;
	float fMainInitialTop;
	float fMainInitialHeight;
	float fListInitialHeight;
	float fItemHeight;
protected:
	void MsgBack( const SGameMessage &msg );
	void MsgSetModality( const SGameMessage &msg );
	void MsgMissionObjectivesChanged( const SGameMessage &msg );
	
	void UpdateKnownObjectives();
	void MakeObjectives();
	void UpdateObjectives( int nSelected );
	int GetButtonIndex( enum EMissionObjectiveState eState );
	~CInterfaceMissionObjectives();
public:
	CInterfaceMissionObjectives();

	bool Init();

	bool StepLocal( bool bAppActive );

	int operator&( IBinSaver &saver );

	bool IsModal();
	
	void SetStartID( int nID );
};

class CICMissionObjectives : public CInterfaceCommandBase<CInterfaceMissionObjectives>
{
	OBJECT_BASIC_METHODS( CICMissionObjectives );
	
	ZDATA_(CInterfaceCommandBase<CInterfaceMissionObjectives>)
	int nID;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceCommandBase<CInterfaceMissionObjectives>*)this); f.Add(2,&nID); return 0; }
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};

#endif //__INTERFACE_MISSION_OBJECTIVES_H__

