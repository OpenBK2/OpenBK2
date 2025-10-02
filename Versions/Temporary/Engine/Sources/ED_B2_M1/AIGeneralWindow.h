#pragma once

#include "MapEditorLib/ResizeDialog.h"
#include "ResourceDefines.h"
#include "Stats_B2_M1/DBMapinfo.h"
#include "MapEditorLib/Interface_CommandHandler.h"

struct SAIGeneralPointsWindowData
{
	struct SAIPlayerInfo
	{
		struct SAIParcel : public NDb::SAIGeneralParcel
		{
			int nCurrentPoint;
			SAIParcel()
			{
				NDb::SAIGeneralParcel();
				Clear();
			}
			void Clear()
			{
				nCurrentPoint = -1;
			}
		};

		vector<int> mobileScriptIDs;
		vector<SAIParcel> parcels;
		int nCurrentID;
		int nCurrentParcel;
		//
		SAIPlayerInfo()
		{
			Clear();
		}
		void Clear()
		{
			mobileScriptIDs.clear();
			parcels.clear();
			nCurrentID = 0;
			nCurrentParcel = 0;
		}
	};
	//
	enum EAIGenPointsLastAction
	{
		AIGP_UNKNOWN,
		AIGP_NO_ACTIONS,
		AIGP_ID_ADD,
		AIGP_ID_DEL,
		AIGP_ID_JUMP,
		AIGP_PARCEL_ADD,
		AIGP_PARCEL_DEL,
		AIGP_PARCEL_JUMP,
		AIGP_PARCEL_EDIT,
		AIGP_PLAYER_JUMP
	};

	vector<SAIPlayerInfo> players;
	int nCurrentPlayer;
	EAIGenPointsLastAction eLastAction;

	SAIGeneralPointsWindowData()
	{
		Clear();
		nCurrentPlayer = 0;
	}
	void Clear()
	{
		players.clear();
	}
	//
	int CurrentPlayer() { return nCurrentPlayer; }
	int CurrentParcel() { return players[CurrentPlayer()].nCurrentParcel; }
	int CurrentID() { return players[CurrentPlayer()].nCurrentID; }
	int CurrentPoint() { return players[CurrentPlayer()].parcels[CurrentParcel()].nCurrentPoint; }
};


//
//
//		AI GENERAL POINTS WINDOW
//
//

class CAIGeneralPointsWindow : public CResizeDialog, public ICommandHandler
{
	bool bIsDataSetting;
	SAIGeneralPointsWindowData::EAIGenPointsLastAction eLastAction;
	// controls
	CListCtrl lcIDs;
	CListCtrl lcParcels;
	CComboBox	comboPlayer;
	CButton btnIDAdd;
	CButton btnIDDel;
	CButton btnParcelAdd;
	CButton btnParcelDel;

	// CResizeDialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CAIGeneralPointsWindowData )

	// CScriptAreaWindow
	void GetDialogData( SAIGeneralPointsWindowData *pData );
	void SetDialogData( const SAIGeneralPointsWindowData *pData );
	void SetLastAction( const SAIGeneralPointsWindowData::EAIGenPointsLastAction eAction ) { eLastAction = eAction; }

public:
	enum { IDD = IDD_TAB_MI_AIGENERAL };
	//
	CAIGeneralPointsWindow( CWnd* pParentWindow = 0 );
	~CAIGeneralPointsWindow();

	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();
	void OnOK() {}
	void OnCancel() {}

	// ICommandHandler
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
	virtual void NotifyHandler();
	void NotifyHandler( SAIGeneralPointsWindowData::EAIGenPointsLastAction eAction );

	DECLARE_MESSAGE_MAP()
	afx_msg void OnChangePlayerCombo();
	afx_msg void OnDestroy();
	afx_msg void OnLvnItemchangedListParcels( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnLvnKeydownAigenListParcels( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnBnClickedAddParcel();
	afx_msg void OnBnClickedDeleteParcel();
	afx_msg void OnLvnItemchangedListIDs( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnLvnKeydownAigenListIDs( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnBnClickedAddID();
	afx_msg void OnBnClickedDeleteID();
	afx_msg void OnNMDblclkAigenListParcels(NMHDR *pNMHDR, LRESULT *pResult);
	//
};


