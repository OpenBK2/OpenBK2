#pragma once

#include "InterfaceScreenBase.h"

class CInterfaceLoadMod : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceLoadMod );

	struct SMod
	{
		ZDATA
		CPtr<IWindow> pWnd;
		CPtr<ITextView> pNameView;
		CPtr<IWindow> pCheckedWnd;
		wstring wszName;
		wstring wszDesc;
		NFile::CFilePath szFullFolderPath;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWnd); f.Add(3,&pNameView); f.Add(4,&pCheckedWnd); f.Add(5,&wszName); f.Add(6,&wszDesc); f.Add(7,&szFullFolderPath); return 0; }
	};
	
	ZDATA_(CInterfaceScreenBase)
	CPtr<IWindow> pMainWnd;
	CPtr<IWindow> pTopPanel;
	CPtr<IWindow> pBottomPanel;
	CPtr<IWindow> pModsListPanel;
	CPtr<IWindow> pModDescPanel;
	
	CPtr<IScrollableContainer> pModsListCont;
	CPtr<IWindow> pModsListItemTemplate;
	CPtr<IScrollableContainer> pModDescCont;
	CPtr<ITextView> pModDescView;
	CPtr<ITextView> pModNameView;
	CPtr<IButton> pDefaultBtn;
	CPtr<IButton> pAcceptBtn;
	vector<SMod> mods;
	wstring wszModDefaultName;
	wstring wszModDefaultDesc;
	CPtr<IWindow> pPrevCheckedWnd;
	int nInitialMod;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pMainWnd); f.Add(3,&pTopPanel); f.Add(4,&pBottomPanel); f.Add(5,&pModsListPanel); f.Add(6,&pModDescPanel); f.Add(7,&pModsListCont); f.Add(8,&pModsListItemTemplate); f.Add(9,&pModDescCont); f.Add(10,&pModDescView); f.Add(11,&pModNameView); f.Add(12,&pDefaultBtn); f.Add(13,&pAcceptBtn); f.Add(14,&mods); f.Add(15,&wszModDefaultName); f.Add(16,&wszModDefaultDesc); f.Add(17,&pPrevCheckedWnd); f.Add(18,&nInitialMod); return 0; }
private:
	void MakeInterior();
	void AddMod( const wstring &wszName, const wstring &wszDesc, const NFile::CFilePath &szFullFolderPath );
	const SMod* FindSelected() const;
	
	bool OnBack();
	bool OnDefault();
	bool OnAccept();
	bool OnSelect();
	bool OnDblClick();

	void SelectMod( const SMod *pMod );
	void UpdateSelection();
	bool IsChanged() const;
public:
	CInterfaceLoadMod();

	bool Init();

	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;

public:
	bool StepLocal( bool bAppActive );
	//}
};

class CICLoadMod : public CInterfaceCommandBase<CInterfaceLoadMod>
{
	OBJECT_BASIC_METHODS( CICLoadMod );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};

