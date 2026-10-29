#include "stdafx.h"

#include "SecuredSettings.h"

#include <Security/Security.h>

namespace NSettings
{
namespace
{

	CFMutableDictionaryRef MakeQuery( const std::string &szKey )
	{
		CFMutableDictionaryRef query =
				CFDictionaryCreateMutable( nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
		CFStringRef account = CFStringCreateWithBytes(
				nullptr, reinterpret_cast<const UInt8 *>( szKey.data() ), szKey.size(), kCFStringEncodingUTF8, false );
		if ( !account )
		{
			CFRelease( query );
			return nullptr;
		}
		CFDictionarySetValue( query, kSecClass, kSecClassGenericPassword );
		CFDictionarySetValue( query, kSecAttrService, CFSTR( "com.OpenBK2Team.OpenBK2" ) );
		CFDictionarySetValue( query, kSecAttrAccount, account );
		// A locked keychain yields a session-only login instead of a modal prompt
		// which cannot be cancelled when the game's login screen is closed.
		CFDictionarySetValue( query, kSecUseAuthenticationUI, kSecUseAuthenticationUIFail );
		CFRelease( account );
		return query;
	}

	class CMacSecuredSettings : public ISecuredSettings
	{
	public:
		ESecretResult Read( const std::string &szKey, std::string *pszSecret ) override
		{
			ClearSecret( pszSecret );
			CFMutableDictionaryRef query = MakeQuery( szKey );
			if ( !query )
			{
				return ESecretResult::INVALID_DATA;
			}
			CFDictionarySetValue( query, kSecReturnData, kCFBooleanTrue );
			CFTypeRef value = nullptr;
			const OSStatus status = SecItemCopyMatching( query, &value );
			CFRelease( query );
			if ( status == errSecSuccess && value && CFGetTypeID( value ) == CFDataGetTypeID() )
			{
				const auto data = static_cast<CFDataRef>( value );
				pszSecret->assign( reinterpret_cast<const char *>( CFDataGetBytePtr( data ) ), CFDataGetLength( data ) );
				CFRelease( value );
				return ESecretResult::OK;
			}
			if ( value )
			{
				CFRelease( value );
			}
			return status == errSecItemNotFound ? ESecretResult::NOT_FOUND : ESecretResult::UNAVAILABLE;
		}

		ESecretResult Write( const std::string &szKey, const std::string &szSecret ) override
		{
			CFMutableDictionaryRef query = MakeQuery( szKey );
			if ( !query )
			{
				return ESecretResult::INVALID_DATA;
			}
			CFDataRef data = CFDataCreate( nullptr, reinterpret_cast<const UInt8 *>( szSecret.data() ), szSecret.size() );
			CFMutableDictionaryRef values =
					CFDictionaryCreateMutable( nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
			CFDictionarySetValue( values, kSecValueData, data );
			OSStatus status = SecItemUpdate( query, values );
			if ( status == errSecItemNotFound )
			{
				CFDictionarySetValue( query, kSecValueData, data );
				status = SecItemAdd( query, nullptr );
			}
			CFRelease( values );
			CFRelease( data );
			CFRelease( query );
			return status == errSecSuccess ? ESecretResult::OK : ESecretResult::UNAVAILABLE;
		}

		ESecretResult Remove( const std::string &szKey ) override
		{
			CFMutableDictionaryRef query = MakeQuery( szKey );
			if ( !query )
			{
				return ESecretResult::INVALID_DATA;
			}
			const OSStatus status = SecItemDelete( query );
			CFRelease( query );
			return status == errSecSuccess || status == errSecItemNotFound ? ESecretResult::OK : ESecretResult::UNAVAILABLE;
		}
	};

} // namespace

std::unique_ptr<ISecuredSettings> CreateSecuredSettings()
{
	return std::make_unique<CMacSecuredSettings>();
}

} // namespace NSettings
