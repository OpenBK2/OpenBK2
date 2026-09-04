#pragma once

// Start the out-of-process crash handler.
//
// Both executables want this, and for the same reasons, so it lives here rather
// than inside either of them. The handler's file name and the two directory
// names are the contract between a process being watched and the handler
// watching it; two copies of that contract would eventually disagree about
// where dumps land, and the way that failure shows up is an empty reports
// directory rather than an error.
//
// It matters more for the editor than for the game. Every module in this tree
// builds /EHsc, under which catch ( ... ) does not catch a structured
// exception, so an access violation unwinds nothing, runs no handler and writes
// nothing: the process is simply gone. B2_MapEditor.exe had no crash handler at
// all, which is why a fault during database tree population left a truncated
// stingray trace and nothing else, and why reading that trace could not say
// whether the run had crashed or merely been closed. An MFC editor makes this
// worse than it sounds, because a fault inside a window procedure is reported
// as c000041d and the original access violation is what has to be captured.

#include <boost/predef.h>

#include <client/crash_report_database.h>
#include <client/crashpad_client.h>
#include <client/settings.h>

//! Start crashpad_handler and point it at this process.
//!
//! False when the handler could not be started, which is not a reason to refuse
//! to run: an unwatched editor is worth more than no editor, and the caller is
//! expected to carry on. The usual cause is that crashpad_handler.exe is not
//! beside the binary, since the path below is relative.
inline bool InitCrashpad()
{
	// base::FilePath holds a wstring on Windows and a string everywhere else,
	// which is what FILE_PATH_LITERAL is for: it adds the L only where one
	// belongs. The handler is a real executable, so its name carries the
	// platform suffix; the other two name directories and do not.
	//
	// All three stay relative, resolved against the working directory, which is
	// where the handler is installed beside the binary that starts it.
#if BOOST_OS_WINDOWS
	base::FilePath handler( FILE_PATH_LITERAL( "crashpad_handler.exe" ) );
#else
	base::FilePath handler( FILE_PATH_LITERAL( "crashpad_handler" ) );
#endif
	base::FilePath db( FILE_PATH_LITERAL( "crashpad_db" ) );
	base::FilePath metrics( FILE_PATH_LITERAL( "crashpad_metrics" ) );

	auto database = crashpad::CrashReportDatabase::Initialize( db );
	if ( !database )
	{
		return false;
	}
	database->GetSettings()->SetUploadsEnabled( false );

	crashpad::CrashpadClient client;
	return client.StartHandler( handler, db, metrics, "", "", {}, {}, true, false );
}
