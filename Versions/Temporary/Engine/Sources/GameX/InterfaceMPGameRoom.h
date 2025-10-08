
#pragma once

#include "InterfaceMPBase.h"
#include "GameRoomData.h"
#include "ChatControl.h"



class CInterfaceMPGameRoom : public CInterfaceMPScreenBase, 
	public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceMPGameRoom );

	struct SUISlot
	{
		SMPSlot info;
		CPtr<IWindow> pPlayerName;
		CPtr<ITextView> pPlayerNameText;
		CPtr<IComboBox> pPlayerCombo;
		CPtr<IComboBox> pCountry;
		CPtr<IButton> pTeam;
		CPtr<IComboBox> pColour;
		CPtr<IButton> pAccept;
		CPtr<IButton> pPing;
		CPtr<IWindow> pControlDisabler;
	};

	typedef std::vector<SUISlot> CUISlots;

	ZDATA_(CInterfaceMPScreenBase)
		// { Info for the game 
	CPtr<ITextView> pSessionName;
	CPtr<ITextView> pTechLevel;
	CPtr<ITextView> pNumPlayers;	
	CPtr<ITextView>	pMapName;		
	CPtr<IWindow> pMinimap;
	CPtr<ITextView> pTimeLimit;
	CPtr<ITextView> pCaptureTime;
	CPtr<ITextView> pGameSpeed;
	CPtr<IButton> pUnitExperience;	
	// } Info for the game 

	CPtr<IWindow> pMain;

	// { List of players
	CUISlots slots;
	int nOwnSlot;
	CPtr<IScrollableContainer> pList;
	CPtr<IWindow> pSlotTemplate;
	// } List of players
		
	CPtr<IButton> pButtonBeginGame;		

	CPtr<IEditLine> pChatInput;
	CPtr<IScrollableContainer> pChatOutput;
	CPtr<CChatControlWrapper> pChatWrapper;

	CPtr<IWindow> pWaitMessage;
	bool bConnected;

	SB2GameSpecificData gameDesc;
	bool bHost;
	int nRequestedSlotChangeIndex;
	bool bRequestedSlotChangeClosed;
	CPtr<IWindow> pAdvancedPopup;
	CPtr<ITextView> pCantStartAccept;
	CPtr<ITextView> pCantStartSide;
	CPtr<ITextView> pCantStartColour;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceMPScreenBase*)this); f.Add(2,&pSessionName); f.Add(3,&pTechLevel); f.Add(4,&pNumPlayers); f.Add(5,&pMapName); f.Add(6,&pMinimap); f.Add(7,&pTimeLimit); f.Add(8,&pCaptureTime); f.Add(9,&pGameSpeed); f.Add(10,&pUnitExperience); f.Add(11,&pMain); f.Add(12,&slots); f.Add(13,&nOwnSlot); f.Add(14,&pList); f.Add(15,&pSlotTemplate); f.Add(16,&pButtonBeginGame); f.Add(17,&pChatInput); f.Add(18,&pChatOutput); f.Add(19,&pWaitMessage); f.Add(20,&bConnected); f.Add(21,&gameDesc); f.Add(22,&bHost); return 0; }
private:
	void RegisterObservers();

	void InitControls();
	void UpdateMapName();
	void SetControls();
	void UpdateInterior();
	void CheckStartConditions();
	void ReturnToGamesList();
	void OpenCloseEmptySlot( const int nSlot, const bool bClosed );

	void SendUpdateSlot( const int nSlot );

	void MsgOk( const SGameMessage &msg );
	void MsgCancel( const SGameMessage &msg );
	//{
	bool OnBackReaction();
	bool OnBeginGameReaction();

	bool OnAcceptGameReaction();	
	bool OnRejectGameReaction();	
	bool OnChangeSideReaction();
	bool OnChangeTeamReaction();
	bool OnChangeColorReaction();
	bool OnPlayerCombo( const std::string &szSender );
	bool OnInterruptReaction();
	bool OnSessionEnter();
	bool OnShowAdvanced( const bool bShow );

	bool OnChatMessage( SMPUIChatMessage *pMsg );
	bool OnGameRoomInitMessage( const SMPUIGameRoomInitMessage *pMsg );
	bool OnUpdateSlotMessage( SMPUIUpdateSlotMessage *pMsg );
	//}
	bool OnChatEnter();
	bool OnChatEscape();
	bool OnChangeStatus();
	void SetTimeLimit( int nTimeLimit );

	IListControlItem* FindChangedSideTeam( int &nNewSide, int &nNewTeam );
	bool CheckNewColor( int nClientID, int nColor );
	int GetAvailableColor();
	void TryInitThisPlayerSideAndColor();
	
protected:
	~CInterfaceMPGameRoom();
public:
	CInterfaceMPGameRoom();

	bool Init();
	void OnGetFocus( bool bFocus );
	bool ProcessEvent( const SGameMessage &msg );
	void AfterLoad();
	void FillMapData();

	void SetOwner( bool _bCreator );
	

	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction );
	int SelectionIndexToNationIndex(int nSelectionIndex);
	int NationIndexToSelectionIndex(int nCountryIndex);
	int Check( const std::string &szCheckName ) const;
	//}
};

#ifndef _SINGLE_DEMO
class CICMPGameRoom : public CInterfaceCommandBase<CInterfaceMPGameRoom>
{
	OBJECT_BASIC_METHODS( CICMPGameRoom );
	//
	ZDATA
	std::string str;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&str); return 0; }
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};
#endif // _SINGLE_DEMO


