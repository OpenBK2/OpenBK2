#pragma once

#include <cstdint>

/**
#include "MapEditorLib/ResizeDialog.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "ResourceDefines.h"

//
//
//		ADVANCED CLIPBOARD WINDOW DATA
//
//

struct SAdvClipboardPasteSettings
{
	bool bPasteObjectsFencesEntrenchmentes;	// * objects + fences + entrenchments
	bool bPasteHeightsCrags;								// * terrain heights + crags
	bool bPasteBuildings;										// * buildings
	bool bPasteUnitsSquads;									// * mech units + squads
	bool bPasteTerrainTilesSpots;						// * terrain tiles + spots
	bool bPasteBridgeRoads;									// * bridges + roads
	bool bPasteRiversLakesIslands;					// * rivers + lakes + islands
	///
	SAdvClipboardPasteSettings()
	{
		bPasteObjectsFencesEntrenchmentes = true;
		bPasteHeightsCrags = true;
		bPasteBuildings = true;
		bPasteUnitsSquads = true;
		bPasteTerrainTilesSpots = true;
		bPasteBridgeRoads = true;
		bPasteRiversLakesIslands = true;
	}
	///
};

struct SAdvClipboardWindowData
{
	enum ELastAction
	{
		LA_NO_ACTIONS,
		LA_COPY,
		LA_PASTE,
		LA_SAVE_CLIP,
		LA_SEL_CLIP_CHANGE,
		LA_LOAD_CLIP
	};
	ELastAction eLastAction;
	std::vector<std::string> clipNames;
	int nSelectedClip;
	SAdvClipboardPasteSettings pasteSettings;
	///
	SAdvClipboardWindowData()
	{
		Clear();
	}
	////
	void Clear()
	{
		 eLastAction = LA_NO_ACTIONS;
		 clipNames.clear();
		 nSelectedClip = -1;
		 pasteSettings = SAdvClipboardPasteSettings();
	}
	///
};

//
//
//		ADVANCED CLIPBOARD WINDOW
//
//

class CAdvClipboardWindow : public CResizeDialog, public ICommandHandler
{
	SAdvClipboardWindowData::ELastAction eLastAction;
	CListCtrl lcPasteSettings;

	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CAdvClipboardWindow )

	void GetDialogData( SAdvClipboardWindowData *pData );
	void SetDialogData( const SAdvClipboardWindowData *pData );
	void SetLastAction( SAdvClipboardWindowData::ELastAction eAction ) { eLastAction = eAction; }

public:
	enum { IDD = IDD_TAB_MI_ADV_CLIPBOARD };

	CAdvClipboardWindow( CWnd *pParentWindow = 0 );
	virtual ~CAdvClipboardWindow();
	
	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();

	virtual void OnOK() {};
	virtual void OnCancel() {};

	//ICommandHandler
	virtual bool HandleCommand( unsigned nCommandID, uint32_t dwData );
	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
	void NotifyHandler();
	
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonCopy();
	afx_msg void OnBnClickedButtonPaste();
	afx_msg void OnBnClickedButtonSaveClip();
	afx_msg void OnLvnItemchangedListClips(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonLoadClip();
};

/**/

