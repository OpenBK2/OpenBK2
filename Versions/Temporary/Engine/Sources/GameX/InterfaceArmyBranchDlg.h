#pragma once

#include "InterfaceScreenBase.h"

class CInterfaceArmyBranchDlg : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceArmyBranchDlg );
	
	struct SVisSlot
	{
		ZDATA
		CPtr<IWindow> pWnd;
		CPtr<IButton> pBtn;
		CPtr<IWindow> pIconWnd;
		std::string szBtnName;
		CPtr<IWindow> pSelectionWnd;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWnd); f.Add(3,&pBtn); f.Add(4,&pIconWnd); f.Add(5,&szBtnName); f.Add(6,&pSelectionWnd); return 0; }
	};
public:
	struct SBranch
	{
		ZDATA
		std::wstring wszName;
		std::wstring wszDesc;
		CDBPtr<NDb::STexture> pIconTexture;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszName); f.Add(3,&wszDesc); f.Add(4,&pIconTexture); return 0; }
	};
private:
	ZDATA_(CInterfaceScreenBase)
	CPtr<IWindow> pMainWnd;
	CPtr<IButton> pLeftBtn;
	CPtr<IButton> pRightBtn;
	CPtr<ITextView> pBranchNameView;
	CPtr<IScrollableContainer> pDescCont;
	CPtr<ITextView> pDescView;
	std::vector<SVisSlot> visSlots;
	std::vector<SBranch> branches;
	int nSelectedBranch;
	int nLeftBranch;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pMainWnd); f.Add(3,&pLeftBtn); f.Add(4,&pRightBtn); f.Add(5,&pBranchNameView); f.Add(6,&pDescCont); f.Add(7,&pDescView); f.Add(8,&visSlots); f.Add(9,&branches); f.Add(10,&nSelectedBranch); f.Add(11,&nLeftBranch); return 0; }
private:
	void MakeInterior();
	void UpdateInterior();
	
	bool OnClose();
	bool OnLeft();
	bool OnRight();
	bool OnSelect( const std::string &szSender );
public:
	CInterfaceArmyBranchDlg();

	bool Init();
	void SetParams( const std::vector<CInterfaceArmyBranchDlg::SBranch> &branches );

	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction );
	int Check( const std::string &szCheckName ) const;
	//}
};

class CICArmyBranchDlg : public CInterfaceCommandBase<CInterfaceArmyBranchDlg>
{
	OBJECT_BASIC_METHODS( CICArmyBranchDlg );
	
	ZDATA_(CInterfaceCommandBase<CInterfaceArmyBranchDlg>)
	std::vector<CInterfaceArmyBranchDlg::SBranch> branches;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceCommandBase<CInterfaceArmyBranchDlg>*)this); f.Add(2,&branches); return 0; }
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	CICArmyBranchDlg() {}
	CICArmyBranchDlg( const std::vector<CInterfaceArmyBranchDlg::SBranch> &branches );

	void Configure( const char *pszConfig );
};


