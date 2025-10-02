#ifndef __INTERFACE_HELP_H__
#define __INTERFACE_HELP_H__

#pragma ONCE

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
	bool OnCloseReaction( const string &szSender );
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
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;
	//}

	bool StepLocal( bool bAppActive );

	void MakeInterior( const wstring &wszHeader, const wstring &wszDesc );
};

class CICHelp : public CInterfaceCommandBase<CInterfaceHelp>
{
	OBJECT_BASIC_METHODS( CICHelp );

	ZDATA_(CInterfaceCommandBase<CInterfaceHelp>)
	string szInterfaceType;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceCommandBase<CInterfaceHelp>*)this); f.Add(2,&szInterfaceType); return 0; }
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};

#endif //__INTERFACE_HELP_H__

