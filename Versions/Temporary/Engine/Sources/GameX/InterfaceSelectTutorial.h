#pragma once

#include "InterfaceScreenBase.h"

namespace NDb
{
	struct SMapInfo;
}

class CInterfaceSelectTutorial : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceSelectTutorial );
private:
	struct SMapInfo
	{
		ZDATA
		CDBPtr<NDb::SMapInfo> pMapInfo;
		wstring wszDifficulty;
		CPtr<IWindow> pWnd;
		CPtr<ITextView> pNameView;
		CPtr<ITextView> pDifficultyView;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pMapInfo); f.Add(3,&wszDifficulty); f.Add(4,&pWnd); f.Add(5,&pNameView); f.Add(6,&pDifficultyView); return 0; }
	};
	
	ZDATA_(CInterfaceScreenBase)
	CPtr<IWindow> pMainWnd;
	CPtr<IButton> pPlayBtn;
	CPtr<IWindow> pMissionsPanel;
	CPtr<IWindow> pMinimapPanel;
	CPtr<IWindow> pMissionDescPanel;
	CPtr<IScrollableContainer> pMissionsCont;
	CPtr<IWindow> pMissionsItemWnd;
	CPtr<IMiniMap> pMinimap;
	CPtr<ITextView> pMissionNameView;
	CPtr<ITextView> pMissionSeasonView;
	CPtr<IScrollableContainer> pMissionDescCont;
	CPtr<ITextView> pMissionDescView;
	vector<SMapInfo> maps;
	CPtr<IWindow> pMissionDescItemBlock;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pMainWnd); f.Add(3,&pPlayBtn); f.Add(4,&pMissionsPanel); f.Add(5,&pMinimapPanel); f.Add(6,&pMissionDescPanel); f.Add(7,&pMissionsCont); f.Add(8,&pMissionsItemWnd); f.Add(9,&pMinimap); f.Add(10,&pMissionNameView); f.Add(11,&pMissionSeasonView); f.Add(12,&pMissionDescCont); f.Add(13,&pMissionDescView); f.Add(14,&maps); f.Add(15,&pMissionDescItemBlock); return 0; }
private:
	void MakeInterior();
	
	bool OnPlay();
	bool OnBack();
	bool OnSelect();
	
	void Select( int nIndex );
public:
	CInterfaceSelectTutorial();

	bool Init();
	
	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;

public:
	bool StepLocal( bool bAppActive );
	//}
};

class CICSelectTutorial : public CInterfaceCommandBase<CInterfaceSelectTutorial>
{
	OBJECT_BASIC_METHODS( CICSelectTutorial );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};


