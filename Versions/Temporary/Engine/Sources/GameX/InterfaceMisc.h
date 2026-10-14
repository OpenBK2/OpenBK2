
#pragma once
#include "InterfaceScreenBase.h"

class CInterfaceMessageBox : public CInterfaceScreenBase
{
	OBJECT_NOCOPY_METHODS( CInterfaceMessageBox );

public:
	class CReactions : public IProgrammedReactionsAndChecks
	{
		OBJECT_NOCOPY_METHODS( CReactions );

		ZDATA_( IProgrammedReactionsAndChecks )
		CPtr<IWindow> pScreen;

		ZEND int operator&( IBinSaver &f ) override
		{
			f.Add( 1, (IProgrammedReactionsAndChecks *)this );
			f.Add( 2, &pScreen );
			return 0;
		}

	public:
		CReactions() {}

		~CReactions() override {}

		CReactions( IWindow *_pScreen )
			: pScreen( _pScreen )
		{
		}

		bool Execute( const std::string &szSender, const std::string &szReaction ) override;
		int Check( const std::string &szCheckName ) const override;
	};

private:
	ZDATA_( CInterfaceScreenBase )
	CObj<CReactions> pReactions;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CInterfaceScreenBase *)this );
		f.Add( 2, &pReactions );
		return 0;
	}

protected:
	void MsgOk( const SGameMessage &msg );
	void MsgCancel( const SGameMessage &msg );
	void MsgYes( const SGameMessage &msg );
	void MsgNo( const SGameMessage &msg );

	void ResizeTextView( ITextView *pTextView, const std::wstring &szText, int nMinX );

public:
	CInterfaceMessageBox();
	~CInterfaceMessageBox() override;

	bool Init() override;

	void SetParams( const std::string &szName, const std::wstring &szText );

	void OnGetFocus( bool bFocus ) override;

	bool ProcessEvent( const SGameMessage &msg ) override;

	bool StepLocal( bool bAppActive ) override;
};

class CICMessageBox : public CInterfaceCommandBase<CInterfaceMessageBox>
{
	OBJECT_BASIC_METHODS( CICMessageBox );

	ZDATA_( CInterfaceCommandBase<CInterfaceMessageBox> )
	std::string szMainWindowName;
	std::wstring szText;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CInterfaceCommandBase<CInterfaceMessageBox> *)this );
		f.Add( 2, &szMainWindowName );
		f.Add( 3, &szText );
		return 0;
	}

private:
	void PreCreate() override;
	void PostCreate( IInterface *pInterface ) override;

public:
	void Configure( const char *pszConfig ) override;

public:
	static std::string MakeConfigString( const std::string &MessageBoxType, const std::wstring &szText );
};

class CICPreviousMenu : public IInterfaceCommand
{
	OBJECT_BASIC_METHODS( CICPreviousMenu );

	ZDATA_( IInterfaceCommand )
	std::string szResult; // message for send

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (IInterfaceCommand *)this );
		f.Add( 2, &szResult );
		return 0;
	}

public:
	void Exec() override;
	void Configure( const char *pszConfig ) override; // message for send
};

// clears all interfaces that are on top of provided interface
class CICRemoveInterfacesUpTo : public IInterfaceCommand
{
	OBJECT_BASIC_METHODS( CICRemoveInterfacesUpTo );

	ZDATA_( IInterfaceCommand )
	std::string szInterfaceID;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (IInterfaceCommand *)this );
		f.Add( 2, &szInterfaceID );
		return 0;
	}

public:
	void Exec() override;
	void Configure( const char *pszConfig ) override;
};

// use carefully, mainly for manual console commands
class CICClearInterfaces : public IInterfaceCommand
{
	OBJECT_BASIC_METHODS( CICClearInterfaces );

	ZDATA_( IInterfaceCommand )

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (IInterfaceCommand *)this );
		return 0;
	}

public:
	void Exec() override;
};

class CICSuppressEnableFocus : public IInterfaceCommand
{
	OBJECT_BASIC_METHODS( CICSuppressEnableFocus );

	ZDATA_( IInterfaceCommand )

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (IInterfaceCommand *)this );
		return 0;
	}

public:
	void Exec() override;
};

class CICHideUnfocusedScreen : public IInterfaceCommand
{
	OBJECT_BASIC_METHODS( CICHideUnfocusedScreen );

	ZDATA_( IInterfaceCommand )

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (IInterfaceCommand *)this );
		return 0;
	}

public:
	void Exec() override;
};
