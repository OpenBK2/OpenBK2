#ifndef __INTERFACE_CAMPAIGN_SELECTION_MENU_H__
#define __INTERFACE_CAMPAIGN_SELECTION_MENU_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "InterfaceScreenBase.h"

namespace NDb
{
	struct SCampaign;
	struct SDifficultyLevel;
}

void StartCampaign( const NDb::SCampaign *pCampaignDB, int nDifficulty, bool bCustom );

class CInterfaceCampaignSelectionMenu : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceCampaignSelectionMenu );
private:
	enum EPlay
	{
		PT_CAMPAIGN = 0,
		PT_OUTRO = 1,
		PT_COUNT = 2,
	};

	struct SCampaign
	{
		ZDATA
		CPtr<IWindow> pWnd;
		CPtr<IButton> pBtn;
		CPtr<IWindow> pPictureWnd;
		CPtr<ITextView> pNameView;
		CPtr<IScrollableContainer> pDescCont;
		CPtr<ITextView> pDescView;
		string szBtnName;
		int nDBCampaign;
		CPtr<IWindow> pBackgroundWnd;
		string szOutro;
		EPlay ePlay;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWnd); f.Add(3,&pBtn); f.Add(4,&pPictureWnd); f.Add(5,&pNameView); f.Add(6,&pDescCont); f.Add(7,&pDescView); f.Add(8,&szBtnName); f.Add(9,&nDBCampaign); f.Add(10,&pBackgroundWnd); f.Add(11,&szOutro); f.Add(12,&ePlay); return 0; }
	};
private:
	ZDATA_(CInterfaceScreenBase)
	CPtr<IWindow> pMain;
	vector< CPtr<IWindow> > campaignWnds;
	vector<SCampaign> campaigns;
	int nSelected;
	ZSKIP //vector< CPtr<IButton> > playButtons;
	EPlay ePlay;
	CPtr<IComboBox> pDifficulty;
	CPtr<IButton> pPlayBtn;
	CPtr<IButton> pPlayOutroBtn;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pMain); f.Add(3,&campaignWnds); f.Add(4,&campaigns); f.Add(5,&nSelected); f.Add(7,&ePlay); f.Add(8,&pDifficulty); f.Add(9,&pPlayBtn); f.Add(10,&pPlayOutroBtn); return 0; }
private:
	bool OnSelectCampaign( const string &szSender );
	bool OnBack();
	bool OnPlay();
	bool OnPlayOutro();

	void SelectCampaign( int nIndex, bool bFirstTime );
	void MakeInterior();
	void AddCampaignWindow( int nWndIndex, int nCampaignIndex );
	void AddDifficultyLevel( const NDb::SDifficultyLevel *pDifficultyLevel );
public:
	CInterfaceCampaignSelectionMenu();

	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;
	//}

	bool Init();

	void OnGetFocus( bool bFocus );
	bool StepLocal( bool bAppActive );

	bool IsModal();
};

#ifndef _MP_DEMO
class CICCampaignSelectionMenu : public CInterfaceCommandBase<CInterfaceCampaignSelectionMenu>
{
	OBJECT_BASIC_METHODS( CICCampaignSelectionMenu );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};
#endif // _MP_DEMO

#endif //__INTERFACE_CAMPAIGN_SELECTION_MENU_H__
