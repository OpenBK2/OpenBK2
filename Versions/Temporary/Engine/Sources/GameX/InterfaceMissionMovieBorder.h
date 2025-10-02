#ifndef __INTERFACE_MISC_H__
#define __INTERFACE_MISC_H__

#pragma ONCE

#include "InterfaceScreenBase.h"

class CInterfaceMissionMovieBorder : public CInterfaceScreenBase, 
	public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceMissionMovieBorder );
private:
	void RegisterObservers();
protected:
	~CInterfaceMissionMovieBorder();
public:
	CInterfaceMissionMovieBorder();

	bool Init();
	void OnGetFocus( bool bFocus );
	bool StepLocal( bool bAppActive );
	bool ProcessEvent( const SGameMessage &msg );
	void AfterLoad();
	bool IsModal() { return false; }

	void MsgQuickSave( const SGameMessage &msg );
	void MsgQuickLoad( const SGameMessage &msg );

	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;
	//}
};

class CICMissionMovieBorder : public CInterfaceCommandBase<CInterfaceMissionMovieBorder>
{
	OBJECT_BASIC_METHODS( CICMissionMovieBorder );

	ZDATA_(CInterfaceCommandBase<CInterfaceMissionMovieBorder>)
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceCommandBase<CInterfaceMissionMovieBorder>*)this); return 0; }
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	CICMissionMovieBorder();
	
	void Configure( const char *pszConfig );
};

#endif //__INTERFACE_MISC_H__
