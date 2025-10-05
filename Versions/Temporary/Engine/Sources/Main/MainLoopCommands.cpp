#include "stdafx.h"
#include "MainLoopCommands.h"
#include "Profiles.h"
#include "System/WinVFS.h"

class CICExitGame : public IInterfaceCommand
{
	OBJECT_BASIC_METHODS( CICExitGame );
public:
	void Exec()
	{
		std::wstring wszCommand = NGlobal::GetVar( "main_loop_on_exit_command", L"" );
		if ( !wszCommand.empty() )
		{
			NGlobal::ProcessCommand( wszCommand );
		}
		else
		{
			NMainLoop::ResetStack();
			NMainLoop::Command( 0 );
		}
	}
};

class CICFinalExitGame : public IInterfaceCommand
{
	OBJECT_BASIC_METHODS( CICFinalExitGame );
public:
	void Exec()
	{
		NMainLoop::ResetStack();
		NMainLoop::Command( 0 );
	}
};

IBinSaver *CreateSaveLoadSaver( CDataStream *pStream, ESaverMode mode )
{
	std::vector<SBinSaverExternalObject> external;
	std::vector<int> singletonIDs;
	NSingleton::GetAllSingletonIDs( &singletonIDs );
	for ( int k = 0; k < singletonIDs.size(); ++k )
	{
		int nID = singletonIDs[k];
		CObjectBase *p = NSingleton::Singleton( nID );
		external.push_back( SBinSaverExternalObject( nID, p ) );
	}
	return CreateBinSaver( pStream, mode, external );
}

IBinSaver *CreateSaveSaverWithCheckers( CDataStream *pStream, std::vector< CPtr<IDebugSaveCheckObj> > &checkers )
{
	std::vector<SBinSaverExternalObject> external;
	std::vector<int> singletonIDs;
	NSingleton::GetAllSingletonIDs( &singletonIDs );
	for ( int k = 0; k < singletonIDs.size(); ++k )
	{
		int nID = singletonIDs[k];
		CObjectBase *p = NSingleton::Singleton( nID );
		external.push_back( SBinSaverExternalObject( nID, p ) );
	}

	return CreateBinSaverWithCheckers( pStream, external, checkers );
}



static std::string GetSavePathName( const std::string &szName )
{
	return NProfile::GetCurrentProfileDir() + "Saves\\" + szName + ".sav";
}

class CICSave : public CICSaveBase
{
	OBJECT_NOCOPY_METHODS( CICSave );
public:
	CICSave() {}
	CICSave( const std::string &szFileName ) : CICSaveBase( szFileName ) {}

#ifndef _FINALRELEASE
	void OnProgress( EStage eStage )
	{
		switch ( eStage )
		{
			case STG_START:
			{
				const std::string szReport = StrFmt( "Saving at \"%s\"...", GetSavePathName( GetFileName() ).c_str() );
				WriteToPipe( PIPE_CHAT, szReport.c_str(), 0xffff0000 );
			}
			break;

			case STG_AFTER_SAVE_DONE:
			{
				WriteToPipe( PIPE_CHAT, "Save done.", 0xffff0000 );
			}
			break;
		}
	}
#endif //_FINALRELEASE
	//
	int operator&( IBinSaver &saver ) { saver.Add( 1, static_cast<CICSaveBase*>(this) ); return 0; }
};

class CICLoad : public CICLoadBase
{
	OBJECT_NOCOPY_METHODS( CICLoad );
public:
	CICLoad() {}
	CICLoad( const std::string &szFileName ) : CICLoadBase( szFileName ) {}

#ifndef _FINALRELEASE
	void OnProgress( EStage eStage )
	{
		switch ( eStage )
		{
			case STG_START:
				WriteToPipe( PIPE_CHAT, "Loading...", 0xffff0000 );
			break;

			case STG_AFTER_LOAD_DONE:
				WriteToPipe( PIPE_CHAT, "Load done.", 0xffff0000 );
			break;
		}
	}
#endif //_FINALRELEASE
};

class CICCloseInterface : public IInterfaceCommand
{
	OBJECT_BASIC_METHODS( CICCloseInterface );
public:
	void Exec()
	{
		NMainLoop::PopInterface();
	}
};

// CICLoadBase

CICLoadBase::CICLoadBase()
{
}

CICLoadBase::CICLoadBase( const std::string &_szFileName ) :
	szFileName(_szFileName)
{
}

void CICLoadBase::Configure( const char *pszConfig )
{
	szFileName = pszConfig;
}

void CICLoadBase::Exec()
{
	const std::string szPathName = GetSavePathName( szFileName );
	if ( !NVFS::DoesWinFileExist( szPathName ) )
		return;
	OnProgress( STG_START );
	{
		NMainLoop::ResetStack();

		CFileStream stream( szPathName, CFileStream::WIN_READ_ONLY );
		CPtr<IBinSaver> pSaver = CreateSaveLoadSaver( &stream, SAVER_MODE_READ );
		if ( pSaver == 0 ) 
			return;
		NMainLoop::Serialize( *pSaver );
		OnProgress( STG_SERIALIZE_DONE );
	}

	NMainLoop::AfterLoad();
	OnProgress( STG_AFTER_LOAD_DONE );
}

// CICSaveBase

CICSaveBase::CICSaveBase()
{
}

CICSaveBase::CICSaveBase( const std::string &_szFileName ) :
	szFileName( _szFileName )
{
}

void CICSaveBase::Configure( const char *pszConfig )
{
	szFileName = pszConfig;
}

const std::string CICSaveBase::GetPathName()
{
	return GetSavePathName( szFileName );
}

void CICSaveBase::Exec()
{
	if ( szFileName.empty() )
		return;
	OnProgress( STG_START );
	// save
	{
		CFileStream stream( GetSavePathName( szFileName ), CFileStream::WIN_CREATE );
		CPtr<IBinSaver> pSaver = CreateSaveLoadSaver( &stream, SAVER_MODE_WRITE );
		if ( pSaver == 0 ) 
			return;
		NMainLoop::Serialize( *pSaver );
	}

	OnProgress( STG_AFTER_SAVE_DONE );
}

IInterfaceCommand *CreateICExitGame()
{
	return new CICExitGame;
}
IInterfaceCommand *CreateICLoad( const std::string &szName )
{
	return new CICLoad( szName );
}
IInterfaceCommand *CreateICSave( const std::string &szName )
{
	return new CICSave( szName );
}
IInterfaceCommand *CreateICCloseInterface()
{
	return new CICCloseInterface();
}
IInterfaceCommand *CreateICFinalExitGame()
{
	return new CICFinalExitGame();
}

static void CmdExit( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	NMainLoop::Command( CreateICExitGame() );
}

static void CmdFinalExit( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	NMainLoop::Command( CreateICFinalExitGame() );
}

START_REGISTER(MainLoop)
REGISTER_CMD( "exit", CmdExit );
REGISTER_CMD( "final_exit", CmdFinalExit );
FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( 0x10075C09, CICExitGame );
REGISTER_SAVELOAD_CLASS( 0x10075C0A, CICSave );
REGISTER_SAVELOAD_CLASS( 0x10075C0B, CICLoad );
REGISTER_SAVELOAD_CLASS( 0x170B7380, CICCloseInterface );
REGISTER_SAVELOAD_CLASS( 0x17274380, CICFinalExitGame );

