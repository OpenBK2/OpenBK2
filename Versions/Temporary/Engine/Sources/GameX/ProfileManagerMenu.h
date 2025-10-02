#ifndef __PROFILE_MANAGER_MENU_H__
#define __PROFILE_MANAGER_MENU_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "InterfaceScreenBase.h"

class CInterfaceProfileManager : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceProfileManager );
private:
	enum EState
	{
		EST_DEFAULT,
		EST_DELETE_QUESTION,
	};
	
	struct SProfileEntry
	{
		ZDATA
		wstring					wszName;
		CPtr< IWindow >	pWindow;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszName); f.Add(3,&pWindow); return 0; }
	};
	typedef	vector< SProfileEntry > CProfileList;
	
	ZDATA_(CInterfaceScreenBase)
	CProfileList profiles;
	int nSelectedItem;

	CPtr< IWindow > pMain;
	CPtr< IScrollableContainer > pProfileListTemplate;
	CPtr< IScrollableContainer > pProfileList;
	CPtr< IWindow > pItemTemplate;
	CPtr< IEditLine > pNameEditLine;
	
	EState eState;
	CPtr<IWindow> pCenterPanel;
	CPtr<IButton> pCreateBtn;
	CPtr<IButton> pSelectBtn;
	CPtr<IButton> pDeleteBtn;
	bool bNoUpdateSelection;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&profiles); f.Add(3,&nSelectedItem); f.Add(4,&pMain); f.Add(5,&pProfileListTemplate); f.Add(6,&pProfileList); f.Add(7,&pItemTemplate); f.Add(8,&pNameEditLine); f.Add(9,&eState); f.Add(10,&pCenterPanel); f.Add(11,&pCreateBtn); f.Add(12,&pSelectBtn); f.Add(13,&pDeleteBtn); f.Add(14,&bNoUpdateSelection); return 0; }
protected:
	void MsgOk( const SGameMessage &msg );
	void MsgCancel( const SGameMessage &msg );
	void MsgBack( const string &szSender );
	void MsgSelect( const string &szSender );
	void MsgCreate( const string &szSender );
	void MsgDelete( const string &szSender );
	void MsgSelectionChange( const string &szSender );
	bool OnEditChanged();
	
	int FindProfileIndex() const;
	void UpdateButtons();

	void FillScreenList();
	bool IsSameProfileName( const wstring &wszName1, const wstring &wszName2 ) const;
public:
	CInterfaceProfileManager();

	bool Init();

	void OnGetFocus( bool bFocus );

	bool ProcessEvent( const SGameMessage &msg );

	bool StepLocal( bool bAppActive );

	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;
	//}
};

class CICProfileManager : public CInterfaceCommandBase<CInterfaceProfileManager>
{
	OBJECT_BASIC_METHODS( CICProfileManager );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};


#endif //__PROFILE_MANAGER_MENU_H__

