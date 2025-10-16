#include "stdafx.h"

#include <thread>
#include <chrono>

#include <boost/predef.h>

#if BOOST_OS_WINDOWS
#include <wtypes.h>
#include <winbase.h>
#endif

#include "MainLoopInternal.h"

#include "Misc/StrProc.h"
#include "System/FileUtils.h"

#include "Input/GameMessage.h"
#include "System/BasicShare.h"
#include "MainLoopCommands.h"
#include "System/GResource.h"
#include "libdb/Db.h"

#include "port/time.h"

#include <fmt/format.h>

namespace NVFS
{
	LIBDB_EXPORT void VFSSegmentProfiler();
}

// messages
static void MsgCommandExit( const SGameMessage &msg )
{
	NMainLoop::Command( CreateICExitGame() );
}
static void MsgCommandSave( const SGameMessage &msg )
{
	NMainLoop::Command( CreateICSave( "quick" ) );
}
static void MsgCommandLoad( const SGameMessage &msg )
{
	NMainLoop::Command( CreateICLoad( "quick" ) );
}
static void MsgCommandCloseInterface( const SGameMessage &msg )
{
	NMainLoop::Command( CreateICCloseInterface() );
}
static bool ProcessMainLoopCmds( const SGameMessage &msg )
{
	static NInput::CGMORegContainer registers;
	if ( registers.IsEmpty() )
	{
		registers.AddObserver( "exit", MsgCommandExit );
		registers.AddObserver( "save", MsgCommandSave );
		registers.AddObserver( "load", MsgCommandLoad );
		registers.AddObserver( "close_interface", MsgCommandCloseInterface );
	}
	return registers.ProcessEvent( msg, 0 );
}


namespace NMainLoop
{
typedef std::list< CObj<IInterfaceCommand> > CInterfaceCommandsList;
typedef std::list< CObj<IInterfaceBase> > CInterfacesList;
static CInterfaceCommandsList icmds;					// interface commands
static CInterfacesList interfaces;						// interfaces stack
static std::string szBaseDir;								// base dir of the main loop
static bool bInputEnabled = true;

const std::string& GetBaseDir()
{
	return szBaseDir;
}

void InitMainLoop()
{
	char buffer[1024];
	GetCurrentDirectory( 1024, buffer );
	//
	const std::string szLogFileName = std::string(buffer) + "\\log.txt";
	const std::string szErrorFileName = std::string(buffer) + "\\error.txt";
	DeleteFile( szErrorFileName.c_str() );
	DeleteFile( szLogFileName.c_str() );
	//
	std::string szTemp = buffer;
	NI_ASSERT( !szTemp.empty(), "Can't get current directory" );
	if ( szTemp[szTemp.size() - 1] != '\\' )
		szTemp += '\\';
	szTemp += "..\\";
	NFile::GetFullName( &szBaseDir, szTemp );
	if ( szBaseDir[szBaseDir.size() - 1] != '\\' ) 
		szBaseDir += '\\';
	NStr::ToLower( &szBaseDir );
}

void ResetStack()
{
	while ( !interfaces.empty() )
	{
		IInterfaceBase *p = interfaces.back();
		p->OnGetFocus( false );
		interfaces.pop_back();
	}
	interfaces.clear();
}

void __declspec(dllexport) SFLB4_PushInterface( IInterfaceBase *pInterface )
{
	if ( !interfaces.empty() ) 
		interfaces.back()->OnGetFocus( false );
	interfaces.push_back( pInterface );
	pInterface->OnGetFocus( true );
}
void PushInterface( IInterfaceBase *pInterface )
{
	SFLB4_PushInterface( pInterface );
}

void PopInterface()
{
	if ( !interfaces.empty() ) 
	{
		interfaces.back()->OnGetFocus( false );
		interfaces.pop_back();
	}
	if ( !interfaces.empty() ) 
		interfaces.back()->OnGetFocus( true );
}

void Command( IInterfaceCommand *pCommand )
{
	icmds.push_back( pCommand );
}

void Command( int nCommandID, const char *pszConfiguration )
{
	if ( IInterfaceCommand *pCmd = MakeObject<IInterfaceCommand>( nCommandID ) )
	{
		pCmd->Configure( pszConfiguration );
		Command( pCmd );
	}
}

IInterfaceBase *GetTopInterface()
{
	return !interfaces.empty() ? interfaces.back() : 0;
}

IInterfaceBase *GetPrevInterface( IInterfaceBase *pCurrentInterface )
{
	if( !pCurrentInterface )
		return interfaces.back();

	for( CInterfacesList::iterator it = interfaces.begin(); it!=interfaces.end(); ++it )
	{
		if( *it == pCurrentInterface )
		{
			if( it != interfaces.begin() )
				return *(--it);
			else
				return 0;
		}
	}
	return 0;
}

void SetInputEnabled( bool bEnabled )
{
	bInputEnabled = bEnabled;
}

void Serialize( IBinSaver &saver, struct IProgressHook *pHook )
{
	SerializeShared( &saver );
	NSingleton::Serialize( 1, saver );
	//saver.Add( 2, &icmds );
	saver.Add( 3, &interfaces );
	saver.Add( 4, &bInputEnabled );
	//
	NSystem::Serialize( 11, saver );
}

void AfterLoad()
{
	// interfaces after load
	for ( CInterfacesList::iterator it = interfaces.begin(); it != interfaces.end(); ++it )
		(*it)->AfterLoad();
	// UNTIL MODAL INTERFACES (ONLY LAST INTERFACE HAS FOCUS
	if ( !interfaces.empty() )
		(interfaces.back())->OnGetFocus( true );
}

static bool ProcessInterfaceCmds()
{
	while ( !icmds.empty() )
	{
		CObj<IInterfaceCommand> pCmd = icmds.front();
		icmds.pop_front();
		if ( !IsValid(pCmd) )
		{
			ResetStack();
			return false;
		}
		pCmd->Exec();
	}
	return true;
}

static bool ProcessEvent( const SGameMessage &msg )
{
	if ( !interfaces.empty() )
	{
		CInterfacesList::iterator it = interfaces.end(); 
		while ( it != interfaces.begin() )
		{
			--it;
			if ( (*it)->ProcessEvent( msg ) || ( msg.mMessage.cType != NInput::CT_UNKNOWN && (*it)->IsModal() ) )
				return true;
		}
	}
	return false;
}

static void LimitInterfaceFrameRate( bool bActive )
{
	static const int nMaxInterfaceFPS = 720;
	static LARGE_INTEGER frequency;
	static LARGE_INTEGER timeLastFrame;
	static bool bHaveLastFrame = false;

	IInterfaceBase *pInterface = GetTopInterface();
	if ( !bActive || !pInterface || !pInterface->ShouldLimitFrameRate() )
	{
		bHaveLastFrame = false;
		return;
	}

	if ( frequency.QuadPart == 0 )
	{
		if ( !QueryPerformanceFrequency( &frequency ) || frequency.QuadPart == 0 )
			return;
	}

	LARGE_INTEGER timeNow;
	QueryPerformanceCounter( &timeNow );

	if ( !bHaveLastFrame )
	{
		timeLastFrame = timeNow;
		bHaveLastFrame = true;
		return;
	}

	const LONGLONG nFrameTicks = ( frequency.QuadPart + nMaxInterfaceFPS - 1 ) / nMaxInterfaceFPS;
	LONGLONG nElapsedTicks = timeNow.QuadPart - timeLastFrame.QuadPart;

	while ( nElapsedTicks < nFrameTicks )
	{
		const LONGLONG nTicksLeft = nFrameTicks - nElapsedTicks;
		const uint32_t nSleepMS = uint32_t( nTicksLeft * 1000 / frequency.QuadPart );
		if ( nSleepMS > 1 )
			std::this_thread::sleep_for( std::chrono::milliseconds( nSleepMS - 1 ) );
		else
			std::this_thread::yield();

		QueryPerformanceCounter( &timeNow );
		nElapsedTicks = timeNow.QuadPart - timeLastFrame.QuadPart;
	}

	timeLastFrame = timeNow;
}

bool StepApp( bool bActive )
{
	NDb::SegmentProfiler();
	NVFS::VFSSegmentProfiler();

	//	bAppActive = bActive;
	// execute interface (overlord) commands
	if ( !ProcessInterfaceCmds() )
		return false;
	// check for empty interfaces stack
	if ( interfaces.empty() )
	{
		//NI_ASSERT( !interfaces.empty(), "Can't perform execution more: empty interfaces stack... leaving..." );
		return false;
	}
	NI_ASSERT( IsValid(interfaces.back()), fmt::format("Invalid Interface of class \"{}\"", typeid(*interfaces.back()).name()) );
	// update game timer
	Singleton<IGameTimer>()->Update( GetCurrentTimeMilliseconds() );
	// process messages
	NInput::PumpMessages( bActive );

	// step only top interface
	if ( !interfaces.empty() ) 
	{
		//if ( bActive )
		//	NGfx::CheckBackBufferSize();
		//bAppIsActive = bActive;
		//NGfx::SetGamma( bActive );//bSetGamma );
		if ( !ProcessInterfaceCmds() )
			return false;
		ASSERT( IsValid( interfaces.back() ) );

		if ( true )//bInput )
		{
			// commands processing
			SGameMessage event;
			while ( NInput::GetEvent( &event ) )
			{
				if ( bInputEnabled )
				{
					//if ( !ProcessEvent( event ) )
					ProcessEvent( event );
					ProcessMainLoopCmds( event );
				}
				if ( !ProcessInterfaceCmds() )
					return false;
			}
			if ( event.mMessage.cType == NInput::CT_TIME || bInputEnabled )
				ProcessEvent( event );
			if ( !ProcessInterfaceCmds() )
				return false;

			//currentTime = event.mMessage.tTime;
		}
		//
		if ( bActive )
			NGScene::LoadPrecached();
		//
		if ( !interfaces.empty() )
		{
			IInterfaceBase *pInterface = interfaces.back();
			pInterface->Step( bActive );
			if ( !ProcessInterfaceCmds() )
				return false;
		}
	}
	LimitInterfaceFrameRate( bActive );
	return true;
}
}

static void CmdLoad( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.empty() )
		NMainLoop::Command( CreateICLoad( "quick" ) );
	else
		NMainLoop::Command( CreateICLoad( NStr::ToMBCS( paramsSet[0] ) ) );
}
static void CmdSave( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.empty() )
		NMainLoop::Command( CreateICSave( "quick" ) );
	else
		NMainLoop::Command( CreateICSave( NStr::ToMBCS( paramsSet[0] ) ) );
}
START_REGISTER( MainLoopInternal )
	REGISTER_CMD( "load", CmdLoad );
	REGISTER_CMD( "save", CmdSave );
FINISH_REGISTER

