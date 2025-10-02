#pragma once

#include "InterfaceScreenBase.h"

namespace NDb
{
	struct SMapInfo;
}

class CInterfaceMissionBriefing : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceMissionBriefing );
	
	struct SObjective
	{
		ZDATA
		CPtr<IWindow> pWnd;
		CPtr<ITextView> pNameView;
		vector<CVec2> mapPositions;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWnd); f.Add(3,&pNameView); f.Add(4,&mapPositions); return 0; }
	};

	ZDATA_(CInterfaceScreenBase)
	CPtr<IWindow> pMainWnd;
	CPtr<IWindow> pTopPanel;
	CPtr<IWindow> pBottomPanel;
	CPtr<IWindow> pMissionDescPanel;
	CPtr<IWindow> pMinimapPanel;
	CPtr<IWindow> pObjectivesListPanel;
	
	CPtr<IScrollableContainer> pMissionDescCont;
	CPtr<ITextView> pMissionDescView;
	CPtr<IMiniMap> pMinimap;
	CPtr<IScrollableContainer> pObjectivesListCont;
	ZSKIP //CPtr<IWindow> pObjectivesListItemTemplate;
	CDBPtr<NDb::SMapInfo> pMapInfo;
	ZSKIP //vector<SObjective> objectives;
	ZSKIP //int nSelectedObjective;
	CPtr<ITextView> pHeaderView;
	CPtr<ITextView> pObjectivesView;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pMainWnd); f.Add(3,&pTopPanel); f.Add(4,&pBottomPanel); f.Add(5,&pMissionDescPanel); f.Add(6,&pMinimapPanel); f.Add(7,&pObjectivesListPanel); f.Add(8,&pMissionDescCont); f.Add(9,&pMissionDescView); f.Add(10,&pMinimap); f.Add(11,&pObjectivesListCont); f.Add(13,&pMapInfo); f.Add(16,&pHeaderView); f.Add(17,&pObjectivesView); return 0; }
private:
	void MakeInterior();
	void MissionStart();
	
	bool OnBack();
	bool OnPlay();
public:
	CInterfaceMissionBriefing();

	bool Init();

	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;
	//}
};

class CICMissionBriefing : public CInterfaceCommandBase<CInterfaceMissionBriefing>
{
	OBJECT_BASIC_METHODS( CICMissionBriefing );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};

