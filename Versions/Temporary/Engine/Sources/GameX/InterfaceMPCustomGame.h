
#pragma once

#include "InterfaceMPBase.h"


class CInterfaceMPCustomGame : public CInterfaceMPScreenBase, 
	public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceMPCustomGame );

	//--------------
	struct SMPGameInfo
	{
		ZDATA
		int					nServerID;
		std::string			szSessionName;
		std::string			szMapName;
		int					nPlayers;
		int					nPlayersMax;
		int					nPing;
		bool				bPwdReq;
		int					nSizeX;
		int					nSizeY;
		int					nGameType;
		int					nTechLevel;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nServerID); f.Add(3,&szSessionName); f.Add(4,&szMapName); f.Add(5,&nPlayers); f.Add(6,&nPlayersMax); f.Add(7,&nPing); f.Add(8,&bPwdReq); f.Add(9,&nSizeX); f.Add(10,&nSizeY); f.Add(11,&nGameType); f.Add(12,&nTechLevel); return 0; }
	};
	//--------------
	class CCustomGameListData : public CObjectBase
	{
		OBJECT_NOCOPY_METHODS( CCustomGameListData )
	public:
		ZDATA
		SMPGameInfo info;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&info); return 0; }

		CCustomGameListData() {}
		CCustomGameListData( SMPGameInfo &_info ) : info ( _info ) { };
	};
	//--------------
	class CItemCustomGameListViewer : public IDataViewer
	{
		OBJECT_NOCOPY_METHODS(CItemCustomGameListViewer)
	public:
		void MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const;
	};
	//--------------
	struct SFilterData
	{
		int nMapSize;
		int nPlayers;
		int nGameType;
		int nTechLevel;
		SFilterData() : nMapSize(0), nPlayers(0), nGameType(0), nTechLevel(0) {}
	};

	struct SGameEntry
	{
		CPtr<IListControlItem> pItem;
		SMPGameInfo info;
	};
	typedef std::unordered_map<int/*serverID*/, SGameEntry > CMPGameListItems;

	CPtr<IWindow> pMain;
	CMPGameListItems items;
	int nSelectedID;
	CPtr<IListControl> pList;
	CPtr<IEditLine> pSessionInput;
	CPtr<IWindow> pFilters;
	CPtr<IComboBox> pMapSizeBox;
	CPtr<IComboBox> pGameTypeBox;
	CPtr<IComboBox> pTechLevel;
	CPtr<IComboBox> pNumPlayersBox;
	SFilterData filter;
	CPtr<IWindow> pPasswordPopup;
	CPtr<IEditLine> pPasswordEdit;
	std::wstring wszPassword;
	CPtr<IWindow> pGettingListPopup;
	CPtr<IButton> pRefresh;
private:
	void RegisterObservers();

	void SetControls();
	//void UpdateInterior();
	void TryToJoinGame();
	void RebuildGameList();
	bool IsAllowedByFilter( const SMPGameInfo &game );
	bool OnUpdateGameList( SMPUIGameListMessage *pMsg );

	//{
	bool OnBackReaction( const std::string &szSender );
	
	bool OnJoinGameReaction( const std::string &szSender );

	bool OnFiltersReaction( const std::string &szSender );
	bool OnLoadGameReaction( const std::string &szSender );
	bool OnCreateGameReaction( const std::string &szSender );

	bool OnRefreshReaction( const std::string &szSender );
	bool OnSelectGameReaction( const std::string &szSender );
	bool OnCancelFilters( const std::string &szSender );
	bool OnOkFilters( const std::string &szSender );

	bool OnPasswordOk( const std::string &szSender );
	bool OnPasswordCancel( const std::string &szSender );
	//}
protected:
	~CInterfaceMPCustomGame();
public:
	CInterfaceMPCustomGame();

	bool Init();
	void OnGetFocus( bool bFocus );
	bool ProcessEvent( const SGameMessage &msg );
	void AfterLoad();

	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction );
	int Check( const std::string &szCheckName ) const;
	//}
	void SetFiltersData();
};

class CICMPCustomGame : public CInterfaceCommandBase<CInterfaceMPCustomGame>
{
	OBJECT_BASIC_METHODS( CICMPCustomGame );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};


