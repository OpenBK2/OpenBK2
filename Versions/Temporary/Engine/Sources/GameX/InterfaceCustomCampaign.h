#pragma once

#include "InterfaceScreenBase.h"

namespace NDb
{
	struct SCampaign;
}

class CInterfaceCustomCampaign : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceCustomCampaign );

	struct SCampaign
	{
		ZDATA
		CDBPtr<NDb::SCampaign> pCampaignDB;
		CPtr<IWindow> pWnd;
		CPtr<IWindow> pFlagWnd;
		CPtr<ITextView> pNameView;
		wstring wszName;
		wstring wszDesc;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pCampaignDB); f.Add(3,&pWnd); f.Add(4,&pFlagWnd); f.Add(5,&pNameView); f.Add(6,&wszName); f.Add(7,&wszDesc); return 0; }
	};
	
	ZDATA_(CInterfaceScreenBase)
	CPtr<IWindow> pMainWnd;
	CPtr<IWindow> pTopPanel;
	CPtr<IWindow> pBottomPanel;
	CPtr<IWindow> pCampaignsListPanel;
	CPtr<IWindow> pCampaignDescPanel;
	
	CPtr<IScrollableContainer> pCampaignsListCont;
	CPtr<IWindow> pCampaignsListItemTemplate;
	CPtr<IScrollableContainer> pCampaignDescCont;
	CPtr<ITextView> pCampaignDescView;
	CPtr<IComboBox> pDifficultyComboBox;
	CPtr<IButton> pPlayBtn;
	vector<SCampaign> campaigns;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pMainWnd); f.Add(3,&pTopPanel); f.Add(4,&pBottomPanel); f.Add(5,&pCampaignsListPanel); f.Add(6,&pCampaignDescPanel); f.Add(7,&pCampaignsListCont); f.Add(8,&pCampaignsListItemTemplate); f.Add(9,&pCampaignDescCont); f.Add(10,&pCampaignDescView); f.Add(11,&pDifficultyComboBox); f.Add(12,&pPlayBtn); f.Add(13,&campaigns); return 0; }
private:
	void MakeInterior();
	void AddCampaign( const NDb::SCampaign *pCampaign );
	const SCampaign* FindSelected() const;
	
	bool OnBack();
	bool OnPlay();
	bool OnSelect();
	bool OnDblClick();

	void CampaignStart( const NDb::SCampaign *pCampaignDB );
	void UpdateSelection( bool bFirstTime );
	void UpdateDifficulty( const SCampaign *pCampaign, bool bFirstTime );
public:
	CInterfaceCustomCampaign();

	bool Init();

	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;

public:
	bool StepLocal( bool bAppActive );
	//}
};

class CICCustomCampaign : public CInterfaceCommandBase<CInterfaceCustomCampaign>
{
	OBJECT_BASIC_METHODS( CICCustomCampaign );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};


