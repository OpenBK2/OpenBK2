
#pragma once
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
		std::wstring					wszName;
		CPtr< IWindow >	pWindow;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszName); f.Add(3,&pWindow); return 0; }
	};
	typedef	std::vector< SProfileEntry > CProfileList;
	
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
	void MsgBack( const std::string &szSender );
	void MsgSelect( const std::string &szSender );
	void MsgCreate( const std::string &szSender );
	void MsgDelete( const std::string &szSender );
	void MsgSelectionChange( const std::string &szSender );
	bool OnEditChanged();
	
	int FindProfileIndex() const;
	void UpdateButtons();

	void FillScreenList();
	bool IsSameProfileName( const std::wstring &wszName1, const std::wstring &wszName2 ) const;
public:
	CInterfaceProfileManager();

	bool Init();

	void OnGetFocus( bool bFocus );

	bool ProcessEvent( const SGameMessage &msg );

	bool StepLocal( bool bAppActive );

	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction );
	int Check( const std::string &szCheckName ) const;
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



