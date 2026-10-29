#include "System/SecuredSettings.h"

#include <cstdlib>
#include <map>
#include <string>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <gtest/gtest.h>

namespace
{

class CMemoryStore : public NSettings::ISecuredSettings
{
public:
	std::map<std::string, std::string> entries;
	NSettings::ESecretResult failure = NSettings::ESecretResult::OK;

	NSettings::ESecretResult Read( const std::string &key, std::string *secret ) override
	{
		if ( failure != NSettings::ESecretResult::OK )
		{
			return failure;
		}
		const auto found = entries.find( key );
		if ( found == entries.end() )
		{
			return NSettings::ESecretResult::NOT_FOUND;
		}
		*secret = found->second;
		return NSettings::ESecretResult::OK;
	}

	NSettings::ESecretResult Write( const std::string &key, const std::string &secret ) override
	{
		if ( failure == NSettings::ESecretResult::OK )
		{
			entries[key] = secret;
		}
		return failure;
	}

	NSettings::ESecretResult Remove( const std::string &key ) override
	{
		entries.erase( key );
		return NSettings::ESecretResult::OK;
	}
};

TEST( SecuredSettings, BothLoginAndPasswordAreInsideTheSecretAndProfilesStaySeparate )
{
	CMemoryStore store;
	NSettings::SCredentials credentials;
	credentials.szLogin = u8"user \u0107\u03bb";
	credentials.szPassword = std::string( "password\0with\nbytes", 19 );
	ASSERT_EQ( NSettings::ESecretResult::OK,
	           NSettings::WriteCredentials( store, "opaque-profile/NivalNet", credentials ) );
	EXPECT_EQ( "opaque-profile/NivalNet", store.entries.begin()->first );
	NSettings::SCredentials loaded;
	ASSERT_EQ( NSettings::ESecretResult::OK, NSettings::ReadCredentials( store, "opaque-profile/NivalNet", &loaded ) );
	EXPECT_EQ( credentials.szLogin, loaded.szLogin );
	EXPECT_EQ( credentials.szPassword, loaded.szPassword );
	EXPECT_EQ( NSettings::ESecretResult::NOT_FOUND,
	           NSettings::ReadCredentials( store, "different-profile/NivalNet", &loaded ) );
	EXPECT_TRUE( loaded.szPassword.empty() );
	EXPECT_TRUE( loaded.szLogin.empty() );
}

TEST( SecuredSettings, LockedOrUnavailableStoreDoesNotProduceAFallbackSecret )
{
	CMemoryStore store;
	store.failure = NSettings::ESecretResult::UNAVAILABLE;
	NSettings::SCredentials credentials;
	credentials.szLogin = "login";
	credentials.szPassword = "password";
	EXPECT_EQ( NSettings::ESecretResult::UNAVAILABLE, NSettings::WriteCredentials( store, "profile", credentials ) );
	EXPECT_TRUE( store.entries.empty() );
	EXPECT_EQ( NSettings::ESecretResult::UNAVAILABLE, NSettings::ReadCredentials( store, "profile", &credentials ) );
	EXPECT_TRUE( credentials.szPassword.empty() );
}

TEST( SecuredSettings, MalformedCredentialPayloadIsRejected )
{
	CMemoryStore store;
	NSettings::SCredentials credentials;
	for ( const auto &payload : { std::string( "broken" ), std::string( "\1\xff\xff\xff\xff", 5 ), std::string() } )
	{
		store.entries["profile"] = payload;
		EXPECT_EQ( NSettings::ESecretResult::INVALID_DATA, NSettings::ReadCredentials( store, "profile", &credentials ) );
		EXPECT_TRUE( credentials.szLogin.empty() );
		EXPECT_TRUE( credentials.szPassword.empty() );
	}
}

TEST( SecuredSettings, NativeStoreRoundTrip )
{
	// Explicit opt-in prevents CI from prompting for or modifying a real keyring.
	if ( !std::getenv( "OPENBK2_TEST_NATIVE_CREDENTIALS" ) )
	{
		GTEST_SKIP() << "Set OPENBK2_TEST_NATIVE_CREDENTIALS=1 to exercise a unique temporary OS credential.";
	}
	auto store = NSettings::CreateSecuredSettings();
	const auto key = "test/" + boost::uuids::to_string( boost::uuids::random_generator()() );

	struct CCleanup
	{
		NSettings::ISecuredSettings &store;
		std::string key;

		~CCleanup()
		{
			store.Remove( key );
		}
	} cleanup{ *store, key };

	NSettings::SCredentials credentials;
	credentials.szLogin = u8"OpenBK2 test \u0107";
	credentials.szPassword = "temporary test password";
	ASSERT_EQ( NSettings::ESecretResult::OK, NSettings::WriteCredentials( *store, key, credentials ) );
	NSettings::SCredentials loaded;
	ASSERT_EQ( NSettings::ESecretResult::OK, NSettings::ReadCredentials( *store, key, &loaded ) );
	EXPECT_EQ( credentials.szLogin, loaded.szLogin );
	EXPECT_EQ( credentials.szPassword, loaded.szPassword );
	credentials.szPassword = "updated password";
	ASSERT_EQ( NSettings::ESecretResult::OK, NSettings::WriteCredentials( *store, key, credentials ) );
	ASSERT_EQ( NSettings::ESecretResult::OK, NSettings::ReadCredentials( *store, key, &loaded ) );
	EXPECT_EQ( credentials.szPassword, loaded.szPassword );
	ASSERT_EQ( NSettings::ESecretResult::OK, store->Remove( key ) );
	EXPECT_EQ( NSettings::ESecretResult::NOT_FOUND, NSettings::ReadCredentials( *store, key, &loaded ) );
}

} // namespace
