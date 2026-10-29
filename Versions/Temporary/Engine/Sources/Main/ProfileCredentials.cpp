#include "stdafx.h"

#include "ProfileCredentials.h"
#include "Profiles.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace NProfile
{
namespace
{
	std::mutex credentialIdentityMutex;

	NSettings::ESecretResult GetCredentialKey( const std::string &szDirectory, bool bCreate, std::string *pszKey )
	{
		// POSIX file locks do not exclude another thread in the same process.
		std::lock_guard<std::mutex> localGuard( credentialIdentityMutex );
		try
		{
			const auto directory = std::filesystem::u8path( szDirectory );
			const auto path = directory / ".credentials-id";
			// This lock protects identity creation across two game processes. The file
			// contains a random identifier only, never a login or password.
			const auto lockPath = directory / ".credentials-lock";
			{
				std::ofstream file( lockPath, std::ios::app );
				if ( !file )
				{
					return NSettings::ESecretResult::UNAVAILABLE;
				}
			}
			boost::interprocess::file_lock lock( lockPath.u8string().c_str() );
			boost::interprocess::scoped_lock<boost::interprocess::file_lock> guard( lock );
			std::string id;
			if ( std::filesystem::exists( path ) )
			{
				std::ifstream file( path );
				std::getline( file, id );
				if ( id.size() != 36 )
				{
					return NSettings::ESecretResult::INVALID_DATA;
				}
				id = boost::uuids::to_string( boost::uuids::string_generator()( id ) );
			}
			else if ( bCreate )
			{
				id = boost::uuids::to_string( boost::uuids::random_generator()() );
				const auto temporary = directory / ( ".credentials-id-" + id );
				std::ofstream file( temporary, std::ios::binary );
				file << id << '\n';
				file.close();
				if ( !file )
				{
					std::error_code ignored;
					std::filesystem::remove( temporary, ignored );
					return NSettings::ESecretResult::UNAVAILABLE;
				}
				std::filesystem::rename( temporary, path );
			}
			else
			{
				return NSettings::ESecretResult::NOT_FOUND;
			}
			*pszKey = id + "/NivalNet";
			return NSettings::ESecretResult::OK;
		}
		catch ( const std::exception & )
		{
			return NSettings::ESecretResult::UNAVAILABLE;
		}
	}

	SCredentialReply RunCredentialRequest( const std::string &directory,
	                                       const std::shared_ptr<NSettings::ISecuredSettings> &store,
	                                       ECredentialOperation operation,
	                                       const NSettings::SCredentials &credentials )
	{
		SCredentialReply reply;
		try
		{
			std::string key;
			reply.result = GetCredentialKey( directory, operation == ECredentialOperation::WRITE, &key );
			if ( reply.result == NSettings::ESecretResult::NOT_FOUND && operation == ECredentialOperation::REMOVE )
			{
				reply.result = NSettings::ESecretResult::OK;
				return reply;
			}
			if ( reply.result != NSettings::ESecretResult::OK )
			{
				return reply;
			}
			switch ( operation )
			{
				case ECredentialOperation::READ:
					reply.result = NSettings::ReadCredentials( *store, key, &reply.credentials );
					break;
				case ECredentialOperation::WRITE:
					reply.result = NSettings::WriteCredentials( *store, key, credentials );
					break;
				case ECredentialOperation::REMOVE:
					reply.result = store->Remove( key );
					break;
			}
		}
		catch ( const std::exception & )
		{
			reply.result = NSettings::ESecretResult::UNAVAILABLE;
		}
		return reply;
	}

} // namespace

CCredentialRequest::CCredentialRequest( ECredentialOperation operation, const NSettings::SCredentials &credentials )
	: pStore( NSettings::CreateSecuredSettings() )
{
	// Capture the directory on the UI thread before the user can switch profiles.
	future =
			std::async( std::launch::async, RunCredentialRequest, GetCurrentProfileDir(), pStore, operation, credentials );
}

CCredentialRequest::~CCredentialRequest()
{
	pStore->Cancel();
	// future's destructor joins the worker before the store or module goes away.
}

bool CCredentialRequest::IsReady() const
{
	return future.valid() && future.wait_for( std::chrono::seconds( 0 ) ) == std::future_status::ready;
}

SCredentialReply CCredentialRequest::TakeResult()
{
	return future.get();
}

NSettings::ESecretResult ForgetProfileCredentials( const std::string &szProfileDir )
{
	std::string key;
	const auto result = GetCredentialKey( szProfileDir, false, &key );
	if ( result == NSettings::ESecretResult::NOT_FOUND )
	{
		return NSettings::ESecretResult::OK;
	}
	if ( result != NSettings::ESecretResult::OK )
	{
		return result;
	}
	auto store = NSettings::CreateSecuredSettings();
	return store->Remove( key );
}

} // namespace NProfile
