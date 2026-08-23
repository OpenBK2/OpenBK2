#include "stdafx.h"

#include "FileUtils.h"
#include "FilePath.h"
#include "Misc/StrProc.h"

#if BOOST_OS_WINDOWS
#include <objbase.h>
#include <direct.h>
#include <tchar.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <system_error>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace NFile
{


// ************************************************************************************************************************ //
// **
// ** class CFileIterator functions
// **
// **
// **
// ************************************************************************************************************************ //

// FindFirstFile's mask semantics, as the callers here use them. "*" and "*.*" both
// mean everything, including a name with no dot at all, which a literal reading of
// "*.*" would exclude. '?' and a '*' elsewhere in the pattern work too, though no
// caller writes one today.
//
// Case-insensitive, as FindFirstFile was: a "*.sav" search has always found Foo.SAV,
// and on a case-sensitive filesystem a plain suffix compare would stop finding it.
static bool MatchesMask( const std::string &szName, const std::string &szMask )
{
	if ( szMask.empty() || szMask == "*" || szMask == "*.*" )
		return true;
	const char *pszName = szName.c_str();
	const char *pszMask = szMask.c_str();
	const char *pszStar = 0;
	const char *pszRetry = 0;
	while ( *pszName != 0 )
	{
		if ( *pszMask == '?' || NStr::ASCII_tolower( *pszMask ) == NStr::ASCII_tolower( *pszName ) )
		{
			++pszMask;
			++pszName;
		}
		else if ( *pszMask == '*' )
		{
			// remember where to resume if the rest of the pattern turns out not to fit
			pszStar = pszMask++;
			pszRetry = pszName;
		}
		else if ( pszStar != 0 )
		{
			pszMask = pszStar + 1;
			pszName = ++pszRetry;
		}
		else
		{
			return false;
		}
	}
	while ( *pszMask == '*' )
	{
		++pszMask;
	}
	return *pszMask == 0;
}

void CFileIterator::Open( const std::string &szFullMask )
{
	const size_t nPos = szFullMask.find_last_of( "/" "\\" );
	const std::string szDir = ( nPos == std::string::npos ) ? std::string() : szFullMask.substr( 0, nPos + 1 );
	szMask = ( nPos == std::string::npos ) ? szFullMask : szFullMask.substr( nPos + 1 );

	// Absolute, because this has always handed back absolute names and callers open
	// files by them. lexically_normal rather than canonical: the directory need not
	// exist, and that case has to end as an empty iteration rather than an error.
	std::error_code ec;
	const std::filesystem::path dir =
		std::filesystem::absolute( szDir.empty() ? std::filesystem::path( "." ) : std::filesystem::path( szDir ), ec )
			.lexically_normal()
			.make_preferred();
	if ( ec )
		return;
	it = std::filesystem::directory_iterator( dir, ec );
	if ( ec )
	{
		it = std::filesystem::directory_iterator();
		return;
	}
	SkipToMatch();
}

void CFileIterator::SkipToMatch()
{
	const std::filesystem::directory_iterator itEnd;
	std::error_code ec;
	while ( it != itEnd && !MatchesMask( it->path().filename().string(), szMask ) )
	{
		it.increment( ec );
		if ( ec )
		{
			it = itEnd;
			return;
		}
	}
}

const CFileIterator& CFileIterator::Next()
{
	if ( IsEnd() )
		return *this;
	std::error_code ec;
	it.increment( ec );
	if ( ec )
		it = std::filesystem::directory_iterator();
	else
		SkipToMatch();
	return *this;
}

// ************************************************************************************************************************ //
//                                         external file utilites
// ************************************************************************************************************************ //

// Adding the write permission is what clearing FILE_ATTRIBUTE_READONLY was: the
// Windows standard library maps that attribute onto these bits. Adding it where it
// is already set costs nothing, so the read-only test that the attribute query used
// to answer is no longer needed, and neither are GetAttribs and IsReadOnly.
static void MakeWritable( const std::string &szName )
{
	std::error_code ec;
	std::filesystem::permissions( szName, std::filesystem::perms::owner_write,
		std::filesystem::perm_options::add, ec );
}

class CDeleteFiles
{
	bool bDeleteRO;
	bool bDeleteDir;
public:
	CDeleteFiles( bool _bDeleteRO, bool _bDeleteDir ) : bDeleteRO( _bDeleteRO ), bDeleteDir( _bDeleteDir ) {  }
	//
	void operator()( const CFileIterator &it )
	{
		if ( !it.IsDirectory() )
		{
			if ( bDeleteRO )
				MakeWritable( it.GetFullName() );
			DeleteFile( it.GetFullName().c_str() );
		}
		else if ( bDeleteDir )
		{
			if ( bDeleteRO )
				MakeWritable( it.GetFullName() );
			RemoveDirectory( it.GetFullName().c_str() );
		}
	}
};

void DeleteFiles( const char *pszStartDir, const char *pszMask, bool bRecursive )
{
	EnumerateFiles( pszStartDir, pszMask, CDeleteFiles(true, false), bRecursive );
}

void DeleteDirectory( const std::string &szDir )
{
	EnumerateFiles( szDir, "*.*", CDeleteFiles(true, true), true );
	RemoveDirectory( szDir.c_str() );
}

// The two flags this carried are gone with GetDirectoryDirs, which was the only
// caller that ever asked for directories.
class CDirFileEnum
{
	std::list<std::string> *pNames;			// store names here
public:
	CDirFileEnum( std::list<std::string> *_pNames ) : pNames( _pNames ) {  }
	void operator()( const CFileIterator &it )
	{
		if ( !it.IsDirectory() )
			pNames->push_back( it.GetFullName() );
	}
};
void GetDirectoryFiles( const char *pszDirName, const char *pszMask, std::list<std::string> *pNames, bool bRecurse )
{
	EnumerateFiles( pszDirName, pszMask, CDirFileEnum(pNames), bRecurse );
}

bool DoesFileExist( const std::string &szFileName )
{
	//return _access( szFileName.c_str(), 0 ) != -1;
	HANDLE hFile = ::CreateFile( szFileName.c_str(), GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0 );
	if ( hFile == INVALID_HANDLE_VALUE )
		return false;
	::CloseHandle( hFile );
	return true;
}

bool DoesFolderExist( const std::string &szFolderName )
{
	//return _access( szFileName.c_str(), 0 ) != -1;
	HANDLE hFile = ::CreateFile( szFolderName.c_str(), GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
	if ( hFile == INVALID_HANDLE_VALUE )
		return false;
	::CloseHandle( hFile );
	return true;
}

// A whitelist, so what it accepts is a name every target platform can hold. This
// is the game's own policy for names it creates, not a reading of any one
// operating system's rules.
bool IsValidFileName( const std::string &szFileName )
{
	// only the name is checked, any directory part in front of it is not
	const std::string::size_type nSlash = szFileName.find_last_of( "/" "\\" );
	const std::string::size_type nStart = ( nSlash == std::string::npos ) ? 0 : nSlash + 1;
	static const char szValidCharSet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-_0123456789 ";
	// _strspnp answered with a pointer to the first character outside the set, and
	// a null one when there was none. find_first_not_of answers the same question.
	return szFileName.find_first_not_of( szValidCharSet, nStart ) == std::string::npos;
}

static const char *pszWrongNames[] = {
	"con", "prn", "aux", "clock$", "nul",
	"com1", "com2", "com3", "com4", "com5", "com6", "com7", "com8", "com9",
	"lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9",
	0
};

// Whether the game will accept this as a name to create, which is deliberately
// stricter than any single platform requires. Linux would take almost all of
// what is rejected here, but a profile or save directory is copied between
// machines, and a name made on one has to still open on the others; the device
// names above are in the list for that reason rather than because this is
// Windows.
bool IsValidDirName( const std::string &_szName )
{
	std::string szName = _szName;
	if ( szName.empty() || szName.size() > 250 )
		return false;
	NStr::ToLower( &szName );
	for ( int i = 0; i < szName.size(); ++i )
	{
		int c = (unsigned char) szName[i];
		if ( c < 33 || c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' || c == '|' || c == '*' || c == '?' )
			return false;
	}
	for ( const char **p = pszWrongNames; *p; ++p )
	{
		if ( szName == *p )
			return false;
	}
	return true;
}

bool CopyFile( const std::string &szSrcName, const std::string &szDstName )
{
	CreatePath( GetFilePath( szDstName ) );
	// overwrite_existing because ::CopyFile was called with bFailIfExists false, so
	// an existing destination was always replaced rather than reported.
	std::error_code ec;
	return std::filesystem::copy_file( szSrcName, szDstName,
		std::filesystem::copy_options::overwrite_existing, ec );
}

// What GetFullPathName did: resolve against the working directory and fold away '.'
// and '..', without requiring the path to exist. absolute does the first and
// lexically_normal the second, and neither touches the filesystem, so a name that is
// not there yet still resolves. Callers depend on that, they build output paths with
// it. A trailing separator survives, which the log path in NetLogger relies on.
//
// canonical would be the wrong tool twice over: it insists the path exists, and it
// resolves symlinks, which GetFullPathName never did.
static std::string MakeFullPathName( const std::string &szPath )
{
	std::error_code ec;
	const std::filesystem::path full =
		std::filesystem::absolute( szPath, ec ).lexically_normal().make_preferred();
	return ec ? szPath : full.string();
}

std::string GetFullName( const std::string &szPath )
{
	return MakeFullPathName( szPath );
}

void GetFullName( std::string *pResult, const std::string &szPath )
{
	*pResult = MakeFullPathName( szPath );
}

std::string GetTempPath()
{
	std::error_code ec;
	std::filesystem::path temp = std::filesystem::temp_directory_path( ec );
	if ( ec )
	{
		// the working directory, as before, when there is no temp directory to find
		temp = ".";
	}
	std::string szRes = temp.make_preferred().string();
	// Callers append a file name straight onto this, so the separator belongs
	// here. temp_directory_path is not specified to include one.
	if ( szRes.empty() || !IsFolderSeparator( szRes[szRes.size() - 1] ) )
	{
		szRes += char( std::filesystem::path::preferred_separator );
	}
	return szRes;
}
std::string GetTempFileName()
{
	const auto guid = boost::uuids::random_generator()();
	return GetTempPath() + boost::uuids::to_string(guid);
}

std::string GetCurrDir()
{
	// The error_code overload, as in NFile::CreatePath: the throwing one would raise
	// at callers that have only ever tested for an empty string, and this does fail
	// in practice, on a working directory deleted out from under the process.
	//
	// The fixed 1024 byte buffer goes with it, and a bug with it: GetCurrentDirectory
	// writes nothing and returns the size it wanted when the path does not fit, which
	// is not zero, so a path over 1024 characters returned uninitialised stack.
	std::error_code ec;
	const std::filesystem::path currDir = std::filesystem::current_path( ec );
	return ec ? std::string() : currDir.string();
}

std::string GetNormalizedCurrDir()
{
	std::string szCurrDir = GetCurrDir();
	NFile::NormalizePath( &szCurrDir );
	NFile::AppendSlash( &szCurrDir, '/' );
	return szCurrDir;
}

void SetCurrDir( const std::string &szDir )
{
	// SetCurrentDirectory returned a BOOL this never read, and the signature leaves
	// nowhere to report one, so a failure stays as quiet as it already was.
	std::error_code ec;
	std::filesystem::current_path( szDir, ec );
}

}; // namespace NFile ends


