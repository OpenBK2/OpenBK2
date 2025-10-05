
#pragma once

#include "InterfaceScreenBase.h"

class CInterfaceHelp : public CInterfaceScreenBase, 
	public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceHelp );

	ZDATA_(CInterfaceScreenBase)
	CPtr<IWindow> pMain;
	CPtr<ITextView> pHeader;
	CPtr<IScrollableContainer> pDescCont;
	CPtr<ITextView> pDesc;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pMain); f.Add(3,&pHeader); f.Add(4,&pDescCont); f.Add(5,&pDesc); return 0; }
private:
	void RegisterObservers();

	//{
	bool OnCloseReaction( const std::string &szSender );
	//}
protected:
	~CInterfaceHelp();
public:
	CInterfaceHelp();

	bool Init();
	void OnGetFocus( bool bFocus );
	bool ProcessEvent( const SGameMessage &msg );
	void AfterLoad();

	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction );
	int Check( const std::string &szCheckName ) const;
	//}

	bool StepLocal( bool bAppActive );

	void MakeInterior( const std::wstring &wszHeader, const std::wstring &wszDesc );
};

class CICHelp : public CInterfaceCommandBase<CInterfaceHelp>
{
	OBJECT_BASIC_METHODS( CICHelp );

	ZDATA_(CInterfaceCommandBase<CInterfaceHelp>)
	std::string szInterfaceType;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceCommandBase<CInterfaceHelp>*)this); f.Add(2,&szInterfaceType); return 0; }
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};


