#include "stdafx.h"

#include "SecuredSettings.h"

#include <wincred.h>
#include <windows.h>

namespace NSettings
{
namespace
{

	std::wstring CredentialTarget( const std::string &szKey )
	{
		const int nSize = MultiByteToWideChar(
				CP_UTF8, MB_ERR_INVALID_CHARS, szKey.data(), static_cast<int>( szKey.size() ), nullptr, 0 );
		if ( nSize <= 0 )
		{
			return {};
		}
		std::wstring key( nSize, L'\0' );
		MultiByteToWideChar(
				CP_UTF8, MB_ERR_INVALID_CHARS, szKey.data(), static_cast<int>( szKey.size() ), &key[0], nSize );
		return L"com.OpenBK2Team.OpenBK2/" + key;
	}

	class CWindowsSecuredSettings : public ISecuredSettings
	{
	public:
		ESecretResult Read( const std::string &szKey, std::string *pszSecret ) override
		{
			ClearSecret( pszSecret );
			const auto target = CredentialTarget( szKey );
			if ( target.empty() )
			{
				return ESecretResult::INVALID_DATA;
			}
			PCREDENTIALW pCredential = nullptr;
			if ( !CredReadW( target.c_str(), CRED_TYPE_GENERIC, 0, &pCredential ) )
			{
				return GetLastError() == ERROR_NOT_FOUND ? ESecretResult::NOT_FOUND : ESecretResult::UNAVAILABLE;
			}
			if ( pCredential->CredentialBlobSize )
			{
				pszSecret->assign( reinterpret_cast<const char *>( pCredential->CredentialBlob ),
				                   pCredential->CredentialBlobSize );
				SecureZeroMemory( pCredential->CredentialBlob, pCredential->CredentialBlobSize );
			}
			CredFree( pCredential );
			return ESecretResult::OK;
		}

		ESecretResult Write( const std::string &szKey, const std::string &szSecret ) override
		{
			auto target = CredentialTarget( szKey );
			if ( target.empty() || szSecret.size() > CRED_MAX_CREDENTIAL_BLOB_SIZE )
			{
				return ESecretResult::INVALID_DATA;
			}
			CREDENTIALW credential{};
			credential.Type = CRED_TYPE_GENERIC;
			credential.TargetName = &target[0];
			credential.CredentialBlobSize = static_cast<DWORD>( szSecret.size() );
			credential.CredentialBlob = reinterpret_cast<LPBYTE>( const_cast<char *>( szSecret.data() ) );
			// LOCAL_MACHINE means persistence for this user's logon on this computer.
			credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
			return CredWriteW( &credential, 0 ) ? ESecretResult::OK : ESecretResult::UNAVAILABLE;
		}

		ESecretResult Remove( const std::string &szKey ) override
		{
			const auto target = CredentialTarget( szKey );
			if ( target.empty() )
			{
				return ESecretResult::INVALID_DATA;
			}
			if ( CredDeleteW( target.c_str(), CRED_TYPE_GENERIC, 0 ) || GetLastError() == ERROR_NOT_FOUND )
			{
				return ESecretResult::OK;
			}
			return ESecretResult::UNAVAILABLE;
		}
	};

} // namespace

std::unique_ptr<ISecuredSettings> CreateSecuredSettings()
{
	return std::make_unique<CWindowsSecuredSettings>();
}

} // namespace NSettings
