#pragma once

#include "InterfaceMPBase.h"




class CInterfaceMPLadderGame : public CInterfaceScreenBase, 
	public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceMPLadderGame );

	class CLadderMapData : public CObjectBase
	{
		OBJECT_NOCOPY_METHODS( CLadderMapData )
	public:
		ZDATA
		CDBPtr<NDb::SMultiplayerMap> pMapDesc;
		CPtr<IButton> pSwitch;
		bool bAllowed;
		bool bSelected;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pMapDesc); f.Add(3,&pSwitch); f.Add(4,&bAllowed); f.Add(5,&bSelected); return 0; }

		CLadderMapData() {}
		CLadderMapData( const NDb::SMultiplayerMap *_pDesc ) : pMapDesc( _pDesc ), bAllowed( true ), bSelected( true ) { };
	};
	//--------------
	class CItemLadderMapListViewer : public IDataViewer
	{
		OBJECT_NOCOPY_METHODS(CItemLadderMapListViewer)
	public:
		void MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const;
	};
	//--------------


	typedef vector< CPtr<CLadderMapData> > CMPMaps;

	ZDATA_(CInterfaceScreenBase)
	CPtr<IWindow> pMain;
	CMPMaps maps;
	CPtr<IListControl> pList;
	CPtr<IComboBox> pGameTypeBox;
	CPtr<IComboBox> pSide;
	CPtr<IComboBox> pTechLevel;
	CPtr<ITextView>	pMapName;
	CPtr<IWindow> pPicture;
	CPtr<IButton> pStartButton;
	CPtr<IWindow> pChecksumPWL;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pMain); f.Add(3,&maps); f.Add(4,&pList); f.Add(5,&pGameTypeBox); f.Add(6,&pSide); f.Add(7,&pTechLevel); f.Add(8,&pMapName); f.Add(9,&pPicture); f.Add(10,&pStartButton); return 0; }
private:
	void RegisterObservers();

	void InitControls();
	void UpdateMapName();
	void SetControls();
	void UpdateInterior();
	void CheckEnableStartButton();

	bool OnBackReaction( const string &szSender );
	bool OnStartGameReaction( const string &szSender );
	bool OnSelectMapReaction( const string &szSender );
	bool OnTeamSizeChanged();
	bool OnChangeMapStatusReaction();
protected:
	~CInterfaceMPLadderGame();
public:
	CInterfaceMPLadderGame();
	bool Init();
	void AfterLoad();
	bool StepLocal( bool bAppActive );

	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;	
	void FillMapData();
	//}
};

class CICMPLadderGame : public CInterfaceCommandBase<CInterfaceMPLadderGame>
{
	OBJECT_BASIC_METHODS( CICMPLadderGame );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};


