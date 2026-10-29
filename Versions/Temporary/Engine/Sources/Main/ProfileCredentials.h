#pragma once

#include "System/SecuredSettings.h"

#include "Main_export.h"

#include <future>
#include <memory>

namespace NProfile
{

enum class ECredentialOperation
{
	READ,
	WRITE,
	REMOVE
};

struct SCredentialReply
{
	NSettings::ESecretResult result = NSettings::ESecretResult::UNAVAILABLE;
	NSettings::SCredentials credentials;
};

// A request captures its profile before starting the worker. It never accesses
// console variables or UI objects from the worker thread.
class MAIN_EXPORT CCredentialRequest
{
	std::shared_ptr<NSettings::ISecuredSettings> pStore;
	std::future<SCredentialReply> future;

public:
	CCredentialRequest( ECredentialOperation operation, const NSettings::SCredentials &credentials = {} );
	~CCredentialRequest();
	bool IsReady() const;
	SCredentialReply TakeResult();
};

MAIN_EXPORT NSettings::ESecretResult ForgetProfileCredentials( const std::string &szProfileDir );

} // namespace NProfile
