#include "stdafx.h"

#include "DBWatcherClient.h"

// The XDBWatcher client where there is no COM to reach it with.
//
// DBWatcherClient.cpp is built by MSVC only: it reaches the service's type
// library with `#import "../XDBWatcherClient/XDBWatcherClient.tlb"`, which is
// an MSVC extension, and the CMakeLists beside this file explains what GCC
// makes of that. What the CMakeLists also said, until today, was that
// excluding the file left nothing undefined because "the only caller of
// RegisterSingleton() is MapEditor, which is Windows-only anyway". MapEditor
// builds on Linux now, so that stopped being true and this is what it needs.
//
// Registering nothing is the right behaviour rather than a placeholder for it.
// Every user of the client asks for it through
// Singleton<IDBWatcherClient>() and tests the result:
//
//   if ( NDBWatcherClient::IDBWatcherClient *pClient = Singleton<...>() )
//
// so a singleton that was never registered means "no watcher", which is
// exactly the state of affairs. The three users -- EditorDatabase, RenameNode
// and Wrapper/ResourceManagerInternal -- all fall back to doing the work
// themselves, since the service is an optimisation: it answers "what refers to
// this object" without walking the database.
//
// **TODO, and it is a question before it is a task.** XDBWatcher.exe is not in
// this tree and not on any machine this port has been run on, so the client
// has had nothing to connect to for the whole of it: the constructor's
// ConnectWatcher throws, the catch sets bFailed, and every call answers
// SERVICE_NOT_READY. Before anyone writes a portable client, the thing to
// settle is whether the service survives the port at all. If it does, the
// protocol is COM today and would need to be something else; if it does not,
// this stub and DBWatcherClient.cpp and the interface can all go, and the
// three users keep the paths they already take when it is absent.
namespace NDBWatcherClient
{

void RegisterSingleton()
{
	// Nothing. See above: the absence of the singleton is the answer.
}

}
