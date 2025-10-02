#ifndef __INTERFACE_MP_CREATE_CUSTOM_GAME_H__
#define __INTERFACE_MP_CREATE_CUSTOM_GAME_H__

#pragma ONCE

#include "InterfaceMPBase.h"




class CInterfaceMPCreateCustomGame : public CInterfaceMPScreenBase, 
	public IProgrammedReactionsAndChecks, public ISliderNotify
{
	OBJECT_NOCOPY_METHODS( CInterfaceMPCreateCustomGame );

	//--------------
	struct SMPMapInfo					//Here "game" is equivalent to "server"
	{
		ZDATA
		wstring wszMapName;
		wstring wszGameType;
		int nMapSizeX;
		int nMapSizeY;
		int nPlayersMax;
		CDBPtr<NDb::SMultiplayerMap> pMPMap;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszMapName); f.Add(3,&wszGameType); f.Add(4,&nMapSizeX); f.Add(5,&nMapSizeY); f.Add(6,&nPlayersMax); f.Add(7,&pMPMap); return 0; }
	};
	//--------------
	class CCustomGameListData : public CObjectBase
	{
		OBJECT_NOCOPY_METHODS( CCustomGameListData )
	public:
		ZDATA
			SMPMapInfo info;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&info); return 0; }

		CCustomGameListData() {}
		CCustomGameListData( SMPMapInfo &_info ) : info ( _info ) { };
	};
	//--------------
	class CItemCustomGameListViewer : public IDataViewer
	{
		OBJECT_NOCOPY_METHODS(CItemCustomGameListViewer)
	public:
		void MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const;
	};
	//--------------


	typedef vector<SMPMapInfo> CMPMaps;

	ZDATA_(CInterfaceMPScreenBase)
		CPtr<IWindow> pMain;
		CMPMaps maps;
		CDBPtr<NDb::SMultiplayerMap> pSelected;
		CPtr<IListControl> pList;
		CPtr<IEditLine> pSessionName;
		CPtr<IComboBox> pTechLevelBox;
		CPtr<IComboBox> pNumPlayersBox;

		CPtr<ITextView>	pMapName;
		CPtr<IWindow> pPicture;
		//CDBPtr<NDb::STexture> pBlankTexture;
		CPtr<IButton> pButtonCreateGame;
		CPtr<IEditLine> pTimeLimit;
		CPtr<IEditLine> pCaptureTime;
		CPtr<ISlider> pSliderGameSpeed;
		CPtr<ITextView> pGameSpeedText;
		CPtr<IButton> pButtonUnitExperience;
		CPtr<IButton> pButtonStartPosition;
		CPtr<IButton> pButtonFog;

		CPtr<IWindow> pAdvancedPopup;
		CPtr<IEditLine> pAdvancedPassword;
		CPtr<IWindow> pChecksumPWL;

	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceMPScreenBase*)this); f.Add(2,&pMain); f.Add(3,&maps); f.Add(4,&pSelected); f.Add(5,&pList); f.Add(6,&pSessionName); f.Add(7,&pTechLevelBox); f.Add(8,&pNumPlayersBox); f.Add(9,&pMapName); f.Add(10,&pPicture); f.Add(11,&pButtonCreateGame); f.Add(12,&pTimeLimit); f.Add(13,&pCaptureTime); f.Add(14,&pSliderGameSpeed); f.Add(15,&pGameSpeedText); f.Add(16,&pButtonUnitExperience); f.Add(17,&pButtonStartPosition); f.Add(18,&pButtonFog); f.Add(19,&pAdvancedPopup); f.Add(20,&pAdvancedPassword); return 0; }
private:
	void RegisterObservers();

	void InitControls();
	void UpdateMapName();
	void SetControls();
	void UpdateInterior();
	void SliderPosition( const float fPosition, CWindow *pWho );

	//{
	bool OnBackReaction( const string &szSender );
	bool OnCreateGameReaction( const string &szSender );
	bool OnSelectMapReaction();
	bool OnSessionEnter();
	bool OnShowAdvanced( const bool bShow );

	bool CheckEnableCreateButton();
	//}
protected:
	~CInterfaceMPCreateCustomGame();
public:
	CInterfaceMPCreateCustomGame();

	bool Init();
	void OnGetFocus( bool bFocus );
	bool ProcessEvent( const SGameMessage &msg );
	void AfterLoad();

	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;	
	void FillMapData();
	//}
};

class CICMPCreateCustomGame : public CInterfaceCommandBase<CInterfaceMPCreateCustomGame>
{
	OBJECT_BASIC_METHODS( CICMPCreateCustomGame );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};

#endif //__INTERFACE_MP_CREATE_CUSTOM_GAME_H__

