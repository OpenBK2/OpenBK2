
#pragma once

#include "ChatControl.h"
#include "GameRoomData.h"
#include "InterfaceMPBase.h"

#include "GameX_export.h"

#include <cstdint>
#include <string>
#include <vector>

class CInterfaceMPGameRoom : public CInterfaceMPScreenBase, public IProgrammedReactionsAndChecks
{
	friend class CManualPlacementTestAccess;
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

	ZDATA_( CInterfaceMPScreenBase )
	// { Info for the game
	CPtr<ITextView> pSessionName;
	CPtr<ITextView> pTechLevel;
	CPtr<ITextView> pNumPlayers;
	CPtr<ITextView> pMapName;
	CPtr<IMiniMap> pMinimap;
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
	bool bConnected = false;

	SB2GameSpecificData gameDesc;
	bool bHost = false;
	int nRequestedSlotChangeIndex;
	bool bRequestedSlotChangeClosed;
	CPtr<IWindow> pAdvancedPopup;
	CPtr<ITextView> pCantStartAccept;
	CPtr<ITextView> pCantStartSide;
	CPtr<ITextView> pCantStartColour;

	// Each marker belongs to its original lobby slot, including vacancies beyond the room capacity.
	struct SPlacementMarker
	{
		CPtr<IWindow> pFlag;
		CPtr<ITextView> pName;
		CPtr<CColorBackground> pPlayerColor;
		std::wstring wszBotName;
	};

	CPtr<IWindow> pPlacementPanel;
	CPtr<IButton> pPlacementToggle;
	float fPlacementToggleInsetY = -1.0f;
	std::vector<SPlacementMarker> placementMarkers;
	std::vector<uint8_t> manualPlacement;
	int nDraggedSlot = -1;
	CVec2 vDragOffset = VNULL2;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CInterfaceMPScreenBase *)this );
		f.Add( 2, &pSessionName );
		f.Add( 3, &pTechLevel );
		f.Add( 4, &pNumPlayers );
		f.Add( 5, &pMapName );
		f.Add( 6, &pMinimap );
		f.Add( 7, &pTimeLimit );
		f.Add( 8, &pCaptureTime );
		f.Add( 9, &pGameSpeed );
		f.Add( 10, &pUnitExperience );
		f.Add( 11, &pMain );
		f.Add( 12, &slots );
		f.Add( 13, &nOwnSlot );
		f.Add( 14, &pList );
		f.Add( 15, &pSlotTemplate );
		f.Add( 16, &pButtonBeginGame );
		f.Add( 17, &pChatInput );
		f.Add( 18, &pChatOutput );
		f.Add( 19, &pWaitMessage );
		f.Add( 20, &bConnected );
		f.Add( 21, &gameDesc );
		f.Add( 22, &bHost );
		return 0;
	}

private:
	void RegisterObservers();

	void InitControls();
	GAMEX_EXPORT void InitPlacementControls();
	void PositionPlacementToggle();
	void UpdatePlacementMarkers();
	void UpdatePlacementCountry( int nSlot );
	void PositionPlacementMarker( int nSlot, const CVec2 &vCenter );
	void RestorePlacementMarker( int nSlot );
	void CancelPlacementDrag();
	int FindPlacementMarker( const CVec2 &vPos, bool bOccupiedOnly ) const;
	bool OnPlacementPress( const SGameMessage &msg );
	bool OnPlacementMove( const SGameMessage &msg );
	bool OnPlacementRelease( const SGameMessage &msg );
	bool OnPlacementCancel( const SGameMessage &msg );
	bool OnManualPlacementMessage( const SMPUIManualPlacementMessage *pMsg );
	bool ToggleManualPlacement();
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

	IListControlItem *FindChangedSideTeam( int &nNewSide, int &nNewTeam );
	bool CheckNewColor( int nClientID, int nColor );
	int GetAvailableColor();
	void TryInitThisPlayerSideAndColor();

protected:
	bool StepLocal( bool bAppActive ) override;
	~CInterfaceMPGameRoom() override;

public:
	CInterfaceMPGameRoom();

	bool Init() override;
	void OnGetFocus( bool bFocus ) override;
	bool ProcessEvent( const SGameMessage &msg ) override;
	void AfterLoad() override;
	void FillMapData();

	void SetOwner( bool _bCreator );

	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction ) override;
	int SelectionIndexToNationIndex( int nSelectionIndex );
	int NationIndexToSelectionIndex( int nCountryIndex );
	int Check( const std::string &szCheckName ) const override;
	//}
};

#ifndef _SINGLE_DEMO
class CICMPGameRoom : public CInterfaceCommandBase<CInterfaceMPGameRoom>
{
	OBJECT_BASIC_METHODS( CICMPGameRoom );
	//
	ZDATA
	std::string str;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &str );
		return 0;
	}

	void PreCreate() override;
	void PostCreate( IInterface *pInterface ) override;

public:
	void Configure( const char *pszConfig ) override;
};
#endif // _SINGLE_DEMO
