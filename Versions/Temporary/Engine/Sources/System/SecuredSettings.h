#pragma once

#include "System_export.h"

#include <memory>
#include <string>

namespace NSettings
{

enum class ESecretResult
{
	OK,
	NOT_FOUND,
	UNAVAILABLE,
	INVALID_DATA
};

// Keys contain only an opaque profile ID and a service ID. Usernames belong in
// the protected value too: some OS stores leave lookup attributes unencrypted.
class SYSTEM_EXPORT ISecuredSettings
{
public:
	virtual ~ISecuredSettings() = default;
	virtual ESecretResult Read( const std::string &szKey, std::string *pszSecret ) = 0;
	virtual ESecretResult Write( const std::string &szKey, const std::string &szSecret ) = 0;
	virtual ESecretResult Remove( const std::string &szKey ) = 0;

	// Called when a screen closes while the desktop keyring is prompting.
	virtual void Cancel() {}
};

SYSTEM_EXPORT std::unique_ptr<ISecuredSettings> CreateSecuredSettings();

struct SYSTEM_EXPORT SCredentials
{
	std::string szLogin;
	std::string szPassword;

	~SCredentials();
	void Clear();
};

SYSTEM_EXPORT void ClearSecret( std::string *pszSecret );
SYSTEM_EXPORT ESecretResult ReadCredentials( ISecuredSettings &store,
                                             const std::string &szKey,
                                             SCredentials *pCredentials );
SYSTEM_EXPORT ESecretResult WriteCredentials( ISecuredSettings &store,
                                              const std::string &szKey,
                                              const SCredentials &credentials );

} // namespace NSettings
