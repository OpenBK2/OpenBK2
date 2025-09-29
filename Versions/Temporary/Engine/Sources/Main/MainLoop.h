#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ************************************************************************************************************************ //
// **
// ** main loop, interface and interface command
// **
// **
// **
// ************************************************************************************************************************ //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
interface IInterfaceBase : virtual public CObjectBase
{
	//virtual bool Init() = 0;
	virtual void OnGetFocus( bool bFocus ) = 0;
	//
	virtual void Step( bool bAppActive ) = 0;
	virtual bool ProcessEvent( const struct SGameMessage &msg ) = 0;
	// переход к этому интерфейсу из другого полноэкранного
	virtual void StartInterface() {}
	// call it after load form save file
	virtual void AfterLoad() {}
	virtual bool IsModal() { return true; }
};
interface IInterfaceCommand : public CObjectBase
{
	virtual void Exec() = 0;
	virtual void Configure( const char *pszConfig ) {  }
};

namespace NMainLoop
{
bool StepApp( bool bActive ); // return false on exit state
void ResetStack();
void Command( IInterfaceCommand *pCommand );
void Command( int nCommandID, const char *pszConfiguration );
const string& GetBaseDir();
void InitMainLoop();
void PushInterface( IInterfaceBase *pInterface );
void PopInterface();
IInterfaceBase *GetTopInterface();
IInterfaceBase *GetPrevInterface( IInterfaceBase *pCurrentInterface );
void SetInputEnabled( bool bEnabled );
void Serialize( IBinSaver &saver, interface IProgressHook *pHook = 0 );
void AfterLoad();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ************************************************************************************************************************ //
// **
// ** file inspector
// **
// **
// **
// ************************************************************************************************************************ //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
interface IFilesInspector : public CObjectBase
{
	enum { tidTypeID = 0x10075C03 };
	// add new entry
	virtual bool AddEntry( const string &szName, interface IFilesInspectorEntry *pEntry ) = 0;
	// remove entry
	virtual bool RemoveEntry( const string &szName ) = 0;
	// get entry
	virtual interface IFilesInspectorEntry* GetEntry( const string &szName ) = 0;
	// clear all entries
	virtual void Clear() = 0;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
interface IFilesInspectorEntry : public CObjectBase
{
	// inspect one stream name
	virtual void InspectStream( const string &szName ) = 0;
	// clear entry
	virtual void Clear() = 0;
};
