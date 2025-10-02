#pragma once

#include "InterfaceScreenBase.h"

namespace NDb
{
	struct SMapInfo;
}

class CInterfaceCustomMissions : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceCustomMissions );

	struct SMission
	{
		ZDATA
		CDBPtr<NDb::SMapInfo> pMapInfo;
		CPtr<IWindow> pWnd;
		CPtr<IWindow> pFlagWnd;
		CPtr<ITextView> pNameView;
		CPtr<ITextView> pSizeView;
		wstring wszName;
		wstring wszSize;
		wstring wszSeason;
		wstring wszDesc;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pMapInfo); f.Add(3,&pWnd); f.Add(4,&pFlagWnd); f.Add(5,&pNameView); f.Add(6,&pSizeView); f.Add(7,&wszName); f.Add(8,&wszSize); f.Add(9,&wszSeason); f.Add(10,&wszDesc); return 0; }
	};
	
	ZDATA_(CInterfaceScreenBase)
	CPtr<IWindow> pMainWnd;
	CPtr<IWindow> pTopPanel;
	CPtr<IWindow> pBottomPanel;
	CPtr<IWindow> pMissionsListPanel;
	CPtr<IWindow> pMinimapPanel;
	CPtr<IWindow> pMissionDescPanel;
	
	CPtr<IWindow> pMissionsListItemTemplate;
	CPtr<IScrollableContainer> pMissionsListCont;
	CPtr<ITextView> pMissionDescView;
	CPtr<IScrollableContainer> pMissionDescCont;
	CPtr<IMiniMap> pMinimap;
	CPtr<IComboBox> pDifficultyComboBox;
	CPtr<IButton> pPlayBtn;
	vector<SMission> missions;
	CPtr<IWindow> pMissionDescItemHeader;
	CPtr<ITextView> pMissionDescItemNameView;
	CPtr<ITextView> pMissionDescItemSeasonView;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pMainWnd); f.Add(3,&pTopPanel); f.Add(4,&pBottomPanel); f.Add(5,&pMissionsListPanel); f.Add(6,&pMinimapPanel); f.Add(7,&pMissionDescPanel); f.Add(8,&pMissionsListItemTemplate); f.Add(9,&pMissionsListCont); f.Add(10,&pMissionDescView); f.Add(11,&pMissionDescCont); f.Add(12,&pMinimap); f.Add(13,&pDifficultyComboBox); f.Add(14,&pPlayBtn); f.Add(15,&missions); f.Add(16,&pMissionDescItemHeader); f.Add(17,&pMissionDescItemNameView); f.Add(18,&pMissionDescItemSeasonView); return 0; }
private:
	void MakeInterior();
	void AddMission( const NDb::SMapInfo *pMapInfo );
	const SMission* FindSelected() const;
	
	bool OnBack();
	bool OnPlay();
	bool OnSelect();
	bool OnDblClick();
	
	void MissionStart( const NDb::SMapInfo *pMapInfo );
	void UpdateSelection( bool bFirstTime );
	void UpdateDifficulty( const SMission *pMission, bool bFirstTime );
public:
	CInterfaceCustomMissions();

	bool Init();
	
	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;

public:
	bool StepLocal( bool bAppActive );

	//}
};

#if !defined(_SINGLE_DEMO) || defined(_MP_DEMO)
class CICCustomMissions : public CInterfaceCommandBase<CInterfaceCustomMissions>
{
	OBJECT_BASIC_METHODS( CICCustomMissions );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};
#endif // !defined(_SINGLE_DEMO) || defined(_MP_DEMO)


