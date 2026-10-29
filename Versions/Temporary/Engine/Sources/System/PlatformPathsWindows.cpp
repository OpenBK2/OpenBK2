// Isolate the modern Known Folder API from the engine's legacy XP SDK target.
// This translation unit deliberately has no precompiled Windows headers.
#undef _WIN32_WINNT
#undef WINVER
#undef NTDDI_VERSION
#define _WIN32_WINNT 0x0600
#define WINVER 0x0600
#define NTDDI_VERSION 0x06000000

#include <filesystem>
#include <stdexcept>
#include <string>

#include <shlobj.h>

namespace NSettings
{

std::string GetWindowsApplicationDataDirectory()
{
	PWSTR pszPath = nullptr;
	const HRESULT result = SHGetKnownFolderPath( FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &pszPath );
	if ( FAILED( result ) )
	{
		CoTaskMemFree( pszPath );
		throw std::runtime_error( "Cannot locate the Windows Local AppData directory." );
	}
	const std::filesystem::path path( pszPath );
	CoTaskMemFree( pszPath );
	return ( path / "OpenBK2" ).u8string();
}

} // namespace NSettings
