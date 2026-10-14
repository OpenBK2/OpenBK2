#pragma once

#include "InterfaceMPBase.h"

class CInterfaceMPLadderGame : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
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

		ZEND int operator&( IBinSaver &f ) override
		{
			f.Add( 2, &pMapDesc );
			f.Add( 3, &pSwitch );
			f.Add( 4, &bAllowed );
			f.Add( 5, &bSelected );
			return 0;
		}

		CLadderMapData() {}

		CLadderMapData( const NDb::SMultiplayerMap *_pDesc )
			: pMapDesc( _pDesc )
			, bAllowed( true )
			, bSelected( true )
		{
		}
	};

	//--------------
	class CItemLadderMapListViewer : public IDataViewer
	{
		OBJECT_NOCOPY_METHODS( CItemLadderMapListViewer )
	public:
		void MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const override;
	};

	//--------------

	typedef std::vector<CPtr<CLadderMapData>> CMPMaps;

	ZDATA_( CInterfaceScreenBase )
	CPtr<IWindow> pMain;
	CMPMaps maps;
	CPtr<IListControl> pList;
	CPtr<IComboBox> pGameTypeBox;
	CPtr<IComboBox> pSide;
	CPtr<IComboBox> pTechLevel;
	CPtr<ITextView> pMapName;
	CPtr<IWindow> pPicture;
	CPtr<IButton> pStartButton;
	CPtr<IWindow> pChecksumPWL;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CInterfaceScreenBase *)this );
		f.Add( 2, &pMain );
		f.Add( 3, &maps );
		f.Add( 4, &pList );
		f.Add( 5, &pGameTypeBox );
		f.Add( 6, &pSide );
		f.Add( 7, &pTechLevel );
		f.Add( 8, &pMapName );
		f.Add( 9, &pPicture );
		f.Add( 10, &pStartButton );
		return 0;
	}

private:
	void RegisterObservers();

	void InitControls();
	void UpdateMapName();
	void SetControls();
	void UpdateInterior();
	void CheckEnableStartButton();

	bool OnBackReaction( const std::string &szSender );
	bool OnStartGameReaction( const std::string &szSender );
	bool OnSelectMapReaction( const std::string &szSender );
	bool OnTeamSizeChanged();
	bool OnChangeMapStatusReaction();

protected:
	~CInterfaceMPLadderGame() override;

public:
	CInterfaceMPLadderGame();
	bool Init() override;
	void AfterLoad() override;
	bool StepLocal( bool bAppActive ) override;

	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction ) override;
	int Check( const std::string &szCheckName ) const override;
	void FillMapData();
	//}
};

class CICMPLadderGame : public CInterfaceCommandBase<CInterfaceMPLadderGame>
{
	OBJECT_BASIC_METHODS( CICMPLadderGame );
	//
	void PreCreate() override;
	void PostCreate( IInterface *pInterface ) override;

public:
	void Configure( const char *pszConfig ) override;
};
