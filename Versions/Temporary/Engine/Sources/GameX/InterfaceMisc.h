
#pragma once
#include "InterfaceScreenBase.h"

class CInterfaceMessageBox : public CInterfaceScreenBase
{
	OBJECT_NOCOPY_METHODS( CInterfaceMessageBox );
public:

	class CReactions : public IProgrammedReactionsAndChecks
	{
		OBJECT_NOCOPY_METHODS(CReactions);
		
		ZDATA_(IProgrammedReactionsAndChecks)
		CPtr<IWindow> pScreen;
		ZEND int operator&( IBinSaver &f ) { f.Add(1,(IProgrammedReactionsAndChecks*)this); f.Add(2,&pScreen); return 0; }
	public:
		CReactions() {  }
		~CReactions() 
		{  
		}
		CReactions( IWindow *_pScreen ) : pScreen( _pScreen ) {   }
		virtual bool Execute( const string &szSender, const string &szReaction );
		virtual int Check( const string &szCheckName ) const;
	};
	
private:
	ZDATA_(CInterfaceScreenBase)
	CObj<CReactions> pReactions;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pReactions); return 0; }
protected:
	void MsgOk( const SGameMessage &msg );
	void MsgCancel( const SGameMessage &msg );
	void MsgYes( const SGameMessage &msg );
	void MsgNo( const SGameMessage &msg );
	
	void ResizeTextView( ITextView *pTextView, const wstring &szText, int nMinX );
public:
	CInterfaceMessageBox();
	~CInterfaceMessageBox();

	bool Init();
	
	void SetParams( const string &szName, const wstring &szText );

	void OnGetFocus( bool bFocus );

	bool ProcessEvent( const SGameMessage &msg );

	bool StepLocal( bool bAppActive );
};

class CICMessageBox : public CInterfaceCommandBase<CInterfaceMessageBox>
{
	OBJECT_BASIC_METHODS( CICMessageBox );
	
	ZDATA_(CInterfaceCommandBase<CInterfaceMessageBox>)
	string szMainWindowName;
	wstring szText;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceCommandBase<CInterfaceMessageBox>*)this); f.Add(2,&szMainWindowName); f.Add(3,&szText); return 0; }
private:
	void PreCreate( );
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
public:
	static string MakeConfigString( const string &MessageBoxType, const wstring &szText );
};

class CICPreviousMenu : public IInterfaceCommand
{
	OBJECT_BASIC_METHODS( CICPreviousMenu );
	
	ZDATA_(IInterfaceCommand)
	string szResult; // message for send
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(IInterfaceCommand*)this); f.Add(2,&szResult); return 0; }
public:
	void Exec();
	void Configure( const char *pszConfig ); // message for send
};

// clears all interfaces that are on top of provided interface
class CICRemoveInterfacesUpTo : public IInterfaceCommand
{
	OBJECT_BASIC_METHODS( CICRemoveInterfacesUpTo );

	ZDATA_(IInterfaceCommand)
	string szInterfaceID;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(IInterfaceCommand*)this); f.Add(2,&szInterfaceID); return 0; }
public:
	void Exec();
	void Configure( const char *pszConfig );
};

// use carefully, mainly for manual console commands
class CICClearInterfaces : public IInterfaceCommand
{
	OBJECT_BASIC_METHODS( CICClearInterfaces );
	
	ZDATA_(IInterfaceCommand)
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(IInterfaceCommand*)this); return 0; }
public:
	void Exec();
};

class CICSuppressEnableFocus : public IInterfaceCommand
{
	OBJECT_BASIC_METHODS( CICSuppressEnableFocus );
	
	ZDATA_(IInterfaceCommand)
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(IInterfaceCommand*)this); return 0; }
public:
	void Exec();
};

class CICHideUnfocusedScreen : public IInterfaceCommand
{
	OBJECT_BASIC_METHODS( CICHideUnfocusedScreen );
	
	ZDATA_(IInterfaceCommand)
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(IInterfaceCommand*)this); return 0; }
public:
	void Exec();
};


