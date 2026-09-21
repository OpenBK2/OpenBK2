#include "stdafx.h"

#include "BuildDetails.h"

#include "Interface_Logger.h"
#include "Interface_MainFrame.h"
#include "Interface_UserData.h"

// Generated at build time by cmake/gitrevision.cmake (add_git_revision).
#include "GitRevision.h"

// The ABI the engine compiles against. Only the version macros are wanted here
// -- GrannyProductVersion, GrannyProductReleaseName and the GrannyVersionsMatch
// predicate -- but the header is what defines them, and it pulls in nothing of
// windows.h, so it sits beside wx without trouble.
#include "vendor/granny/include/granny.h"

#include <wx/platinfo.h>
#include <wx/string.h>
#include <wx/utils.h>
#include <wx/version.h>
#include <wx/app.h>
#include <wx/window.h>

// See BuildDetails.h for why this is one function and not two lists.
//
// Everything below was the About box's CollectDetails, moved here whole, with
// the two libraries the editor's file formats rest on added: Granny for .gr2
// and fastgltf for .gltf/.glb. They belong in a bug report for the same reason
// the wx version does -- a model that loads wrongly is a question about one of
// them -- and the log window already named one of them and nothing else.

namespace
{
#define STRINGIZE_INNER( x )	#x
#define STRINGIZE( x )				STRINGIZE_INNER( x )


	const char* ConfigurationName()
	{
#if defined( _FINALRELEASE )
		return "final release";
#elif defined( _DEBUG )
		return "debug";
#else
		return "release";
#endif
	}


	// Which compiler built this, for a bug report.
	//
	// _MSC_FULL_VER stood here alone, which made the line a statement that this
	// was built with MSVC as much as a version. It is not, off Windows, and the
	// answer is worth having there for the same reason it is worth having here.
	//
	// Clang is asked about before GCC on purpose: clang defines __GNUC__ as well,
	// to claim compatibility, so testing for GCC first would name it wrongly.
	wxString CompilerName()
	{
#if defined( __clang__ )
		return wxString::Format( "clang %d.%d.%d", __clang_major__, __clang_minor__,
														 __clang_patchlevel__ );
#elif defined( __GNUC__ )
		return wxString::Format( "gcc %d.%d.%d", __GNUC__, __GNUC_MINOR__,
														 __GNUC_PATCHLEVEL__ );
#elif defined( _MSC_FULL_VER )
		return wxString::Format( "MSVC %d", _MSC_FULL_VER );
#else
		return "unknown";
#endif
	}


	// The instruction set the compiler was told it may use (-DARCHITECTURE),
	// highest first.
	const char* InstructionSetName()
	{
#if defined( __AVX512F__ )
		return "AVX512";
#elif defined( __AVX2__ )
		return "AVX2";
#elif defined( __AVX__ )
		return "AVX";
#elif defined( _M_X64 ) || ( defined( _M_IX86_FP ) && ( _M_IX86_FP >= 2 ) )
		return "SSE2";
#else
		return "default";
#endif
	}


	// What answers the Granny calls, and which Granny it claims to be.
	//
	// The log used to say "Using granny2.dll of version 2.11.8.0", which is wrong
	// twice over. What answers the calls is libgr2, this tree's own
	// implementation (Versions/Temporary/Engine/Sources/vendor/libgr2), not RAD's
	// DLL; and off Windows there is no .dll to name at all. What is worth
	// reporting is the ABI, which is all either version was ever about: libgr2
	// answers 2.11.8.0 because the structure layouts it reproduces are the 2.11
	// ones.
	//
	// The header's own version is named only when the two disagree. They cannot,
	// as long as libgr2 is what loads -- it reads its four numbers from
	// granny211.h's -- so printing 2.11.8.0 twice on every start would say
	// nothing. A real granny2.dll found earlier on the search path is the case
	// this is for, and then the mismatch is the report.
	//
	// A line carrying this marker is the one thing in the block that is a
	// complaint rather than a fact, and Log below picks its level by looking for
	// it. Matching on the text rather than on a second predicate keeps the
	// decision next to what it is about: the line is an error exactly when it
	// says so.
	const char *const PSZ_MISMATCH = "*** MISMATCH";

	wxString GrannyVersion()
	{
		wxString strGranny;
		strGranny << "libgr2, compatible with Granny " << GrannyGetVersionString();
		if ( !GrannyVersionsMatch )
		{
			strGranny << "  " << PSZ_MISMATCH << ": compiled against "
								<< GrannyProductVersion " (" STRINGIZE( GrannyProductReleaseName ) ")";
		}
		return strGranny;
	}


	// fastgltf carries no version of its own in its headers, so the pin comes
	// from the one place that decides it: cmake/fastgltf.cmake passes the tag it
	// fetches as FASTGLTF_VERSION_STR. A build wired up without it says so
	// rather than inventing a number.
	const char* GltfVersion()
	{
#if defined( FASTGLTF_VERSION_STR )
		return "fastgltf " FASTGLTF_VERSION_STR;
#else
		return "fastgltf, version unknown";
#endif
	}


	// The window the display scale is read from. The About box used its own
	// dialog, which is centred on the frame and so always on the same display;
	// the frame answers for both callers and needs no argument.
	wxWindow* ScaleWindow()
	{
		return wxTheApp ? wxTheApp->GetTopWindow() : nullptr;
	}
}


namespace NBuildDetails
{
	std::string Collect()
	{
		const SUserData *const pUserData = Singleton<IUserDataContainer>()->Get();
		wxString strDetails;
		strDetails << "Version       : " << wxString::FromUTF8( pUserData->constUserData.szVersion.c_str() ) << "\n";
		strDetails << "Revision      : " << GIT_REVISION_STR << " (" << GIT_BRANCH_STR << ", " << GIT_COMMIT_DATE_STR << ")\n";
		// CI numbers its builds and stamps their time; a local build is 0 and
		// has no stamp (cmake/versioninfo.cmake), so the compile time stands in.
		strDetails << "Build         : " << REVISION_NUMBER_STR;
		if ( ( BUILD_DATE_TIME_STR[0] != 0 ) && ( std::string( BUILD_DATE_TIME_STR ) != "1970-01-01 00:00:00" ) )
		{
			strDetails << ", built " << BUILD_DATE_TIME_STR;
		}
		else
		{
			strDetails << ", local build, compiled " << __DATE__ << " " << __TIME__;
		}
		strDetails << "\n";
		strDetails << "Configuration : " << ConfigurationName() << ", " << ( sizeof( void* ) * 8 ) << "-bit, " << InstructionSetName() << "\n";
		strDetails << "Compiler      : " << CompilerName() << "\n";
		strDetails << "MOD           : " << ( pUserData->szOpenedMODFolder.empty() ? wxString( "none" ) : wxString::FromUTF8( pUserData->szOpenedMODFolder.c_str() ) ) << "\n";
		strDetails << "wx (compiled) : " << wxVERSION_STRING << ", debug level " << wxDEBUG_LEVEL << "\n";
		// The DLL actually loaded, which is what can differ from the line above.
		strDetails << "wx (running)  : " << wxGetLibraryVersionInfo().GetVersionString() << "\n";
		strDetails << "wx port       : " << wxPlatformInfo::Get().GetPortIdName() << ", Unicode " << wxUSE_UNICODE << ", sizeof(wxChar) " << static_cast<int>( sizeof( wxChar ) ) << "\n";
		// The two model libraries, beside wx because they are the same kind of
		// fact: which implementation read the file that looks wrong.
		strDetails << "Granny (.gr2) : " << GrannyVersion() << "\n";
		strDetails << "glTF          : " << GltfVersion() << "\n";
		strDetails << "OS            : " << wxGetOsDescription() << "\n";
#if BOOST_OS_WINDOWS
		// The one GetACP left in the editor, and the one place it earns its keep:
		// the narrow strings are UTF-8 only where the manifest's code page took
		// (Windows 10 1903 and later), so a value other than 65001 here is the
		// symptom to look for. Windows-only because there is no such thing to
		// report elsewhere: the conversions say UTF-8 outright and never ask the
		// locale.
		strDetails << "ANSI code page: " << static_cast<unsigned>( ::GetACP() ) << "\n";
#endif
		// Before the frame exists there is no display to ask about, and the line
		// is left out rather than guessed at.
		if ( const wxWindow *const pWindow = ScaleWindow() )
		{
			strDetails << "Display scale : " << wxString::Format( "%.2f", pWindow->GetDPIScaleFactor() ) << "\n";
		}
		return std::string( strDetails.utf8_str() );
	}


	void Log()
	{
		ILogger *const pLogger = NLog::GetLogger();
		if ( pLogger == nullptr )
		{
			return;
		}
		pLogger->Log( LT_IMPORTANT, "Map editor build details:\n" );
		// Line by line, because the log window is a list of entries with a level
		// each, not a text buffer: one call per line is what lets the Granny line
		// stand out when it is a complaint.
		const std::string szDetails = Collect();
		std::string::size_type nStart = 0;
		while ( nStart < szDetails.size() )
		{
			const std::string::size_type nEnd = szDetails.find( '\n', nStart );
			const std::string::size_type nNext = ( nEnd == std::string::npos ) ? szDetails.size() : nEnd + 1;
			const std::string szLine = szDetails.substr( nStart, nNext - nStart );
			const bool bComplaint = ( szLine.find( PSZ_MISMATCH ) != std::string::npos );
			pLogger->Log( bComplaint ? LT_ERROR : LT_NORMAL, szLine );
			nStart = nNext;
		}
	}
}
