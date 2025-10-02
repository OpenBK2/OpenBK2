#pragma once

#include "Main_export.h"


// ************************************************************************************************************************ //
// **
// ** main loop, interface and interface command
// **
// **
// **
// ************************************************************************************************************************ //

struct IInterfaceBase : virtual public CObjectBase
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
struct IInterfaceCommand : public CObjectBase
{
	virtual void Exec() = 0;
	virtual void Configure( const char *pszConfig ) {  }
};

namespace NMainLoop
{
MAIN_EXPORT bool StepApp( bool bActive ); // return false on exit state
MAIN_EXPORT void ResetStack();
MAIN_EXPORT void Command( IInterfaceCommand *pCommand );
MAIN_EXPORT void Command( int nCommandID, const char *pszConfiguration );
MAIN_EXPORT const string& GetBaseDir();
void InitMainLoop();
MAIN_EXPORT void PushInterface( IInterfaceBase *pInterface );
MAIN_EXPORT void PopInterface();
MAIN_EXPORT IInterfaceBase *GetTopInterface();
MAIN_EXPORT IInterfaceBase *GetPrevInterface( IInterfaceBase *pCurrentInterface );
void SetInputEnabled( bool bEnabled );
MAIN_EXPORT void Serialize( IBinSaver &saver, struct IProgressHook *pHook = 0 );
void AfterLoad();
}

// ************************************************************************************************************************ //
// **
// ** file inspector
// **
// **
// **
// ************************************************************************************************************************ //

struct IFilesInspector : public CObjectBase
{
	enum { tidTypeID = 0x10075C03 };
	// add new entry
	virtual bool AddEntry( const string &szName, struct IFilesInspectorEntry *pEntry ) = 0;
	// remove entry
	virtual bool RemoveEntry( const string &szName ) = 0;
	// get entry
	virtual struct IFilesInspectorEntry* GetEntry( const string &szName ) = 0;
	// clear all entries
	virtual void Clear() = 0;
};

struct IFilesInspectorEntry : public CObjectBase
{
	// inspect one stream name
	virtual void InspectStream( const string &szName ) = 0;
	// clear entry
	virtual void Clear() = 0;
};

