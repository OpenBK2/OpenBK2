#include "stdafx.h"

#include "SecuredSettings.h"

#include <cstdint>

namespace NSettings
{

void ClearSecret( std::string *pszSecret )
{
	// Volatile writes are retained even when the string is about to be destroyed.
	volatile char *p = pszSecret->empty() ? nullptr : &( *pszSecret )[0];
	for ( size_t i = 0; i < pszSecret->size(); ++i )
	{
		p[i] = 0;
	}
	pszSecret->clear();
}

SCredentials::~SCredentials()
{
	Clear();
}

void SCredentials::Clear()
{
	ClearSecret( &szLogin );
	ClearSecret( &szPassword );
}

ESecretResult ReadCredentials( ISecuredSettings &store, const std::string &szKey, SCredentials *pCredentials )
{
	pCredentials->Clear();
	std::string secret;
	const auto result = store.Read( szKey, &secret );
	if ( result != ESecretResult::OK )
	{
		ClearSecret( &secret );
		return result;
	}
	// A versioned length prefix preserves arbitrary UTF-8 without delimiters.
	if ( secret.size() < 5 || secret[0] != 1 )
	{
		ClearSecret( &secret );
		return ESecretResult::INVALID_DATA;
	}
	uint32_t nLength = 0;
	for ( unsigned i = 0; i < 4; ++i )
	{
		nLength |= static_cast<uint32_t>( static_cast<unsigned char>( secret[i + 1] ) ) << ( i * 8 );
	}
	if ( nLength > secret.size() - 5 )
	{
		ClearSecret( &secret );
		return ESecretResult::INVALID_DATA;
	}
	pCredentials->szLogin.assign( secret, 5, nLength );
	pCredentials->szPassword.assign( secret, 5 + nLength, std::string::npos );
	ClearSecret( &secret );
	return ESecretResult::OK;
}

ESecretResult WriteCredentials( ISecuredSettings &store, const std::string &szKey, const SCredentials &credentials )
{
	// Credential Manager has a small blob limit; keep the shared format bounded.
	if ( credentials.szLogin.size() + credentials.szPassword.size() > 2500 )
	{
		return ESecretResult::INVALID_DATA;
	}
	std::string secret;
	secret.reserve( 5 + credentials.szLogin.size() + credentials.szPassword.size() );
	secret += '\1';
	const auto nLength = static_cast<uint32_t>( credentials.szLogin.size() );
	for ( unsigned i = 0; i < 4; ++i )
	{
		secret += static_cast<char>( ( nLength >> ( i * 8 ) ) & 0xff );
	}
	secret += credentials.szLogin;
	secret += credentials.szPassword;
	const auto result = store.Write( szKey, secret );
	ClearSecret( &secret );
	return result;
}

} // namespace NSettings
