#include "stdafx.h"

#include "SecuredSettings.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <libsecret/secret.h>

namespace NSettings
{
namespace
{

	// Neither the label nor the lookup attributes reveal the online username.
	const SecretSchema schema = {
		"com.OpenBK2Team.OpenBK2.Credentials",
		SECRET_SCHEMA_NONE,
		{ { "profile-service", SECRET_SCHEMA_ATTRIBUTE_STRING }, { nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING } } };

	class CLinuxSecuredSettings : public ISecuredSettings
	{
		GCancellable *pCancellation = g_cancellable_new();

		// A missing or unresponsive desktop service must not leave a login pending.
		class CDeadline
		{
			std::mutex mutex;
			std::condition_variable condition;
			bool bFinished = false;
			std::thread timer;

		public:
			explicit CDeadline( GCancellable *pCancellation )
				: timer(
							[this, pCancellation]()
							{
								std::unique_lock<std::mutex> lock( mutex );
								if ( !condition.wait_for( lock, std::chrono::seconds( 30 ), [this]() { return bFinished; } ) )
								{
									g_cancellable_cancel( pCancellation );
								}
							} )
			{
			}

			~CDeadline()
			{
				{
					std::lock_guard<std::mutex> lock( mutex );
					bFinished = true;
				}
				condition.notify_all();
				timer.join();
			}
		};

	public:
		~CLinuxSecuredSettings() override
		{
			g_object_unref( pCancellation );
		}

		void Cancel() override
		{
			g_cancellable_cancel( pCancellation );
		}

		ESecretResult Read( const std::string &szKey, std::string *pszSecret ) override
		{
			CDeadline deadline( pCancellation );
			ClearSecret( pszSecret );
			GError *pError = nullptr;
			SecretValue *pValue = secret_password_lookup_binary_sync(
					&schema, pCancellation, &pError, "profile-service", szKey.c_str(), nullptr );
			if ( pError )
			{
				g_error_free( pError );
				return ESecretResult::UNAVAILABLE;
			}
			if ( !pValue )
			{
				return ESecretResult::NOT_FOUND;
			}
			gsize nSize = 0;
			const char *pData = secret_value_get( pValue, &nSize );
			pszSecret->assign( pData, nSize );
			secret_value_unref( pValue );
			return ESecretResult::OK;
		}

		ESecretResult Write( const std::string &szKey, const std::string &szSecret ) override
		{
			CDeadline deadline( pCancellation );
			GError *pError = nullptr;
			SecretValue *pValue = secret_value_new( szSecret.data(), szSecret.size(), "application/octet-stream" );
			const gboolean bSaved = secret_password_store_binary_sync( &schema,
			                                                           SECRET_COLLECTION_DEFAULT,
			                                                           "OpenBK2 online login",
			                                                           pValue,
			                                                           pCancellation,
			                                                           &pError,
			                                                           "profile-service",
			                                                           szKey.c_str(),
			                                                           nullptr );
			secret_value_unref( pValue );
			if ( pError )
			{
				g_error_free( pError );
			}
			return bSaved ? ESecretResult::OK : ESecretResult::UNAVAILABLE;
		}

		ESecretResult Remove( const std::string &szKey ) override
		{
			CDeadline deadline( pCancellation );
			GError *pError = nullptr;
			SecretService *pService = secret_service_get_sync( SECRET_SERVICE_OPEN_SESSION, pCancellation, &pError );
			if ( !pService )
			{
				g_clear_error( &pError );
				return ESecretResult::UNAVAILABLE;
			}
			GHashTable *pAttributes = secret_attributes_build( &schema, "profile-service", szKey.c_str(), nullptr );
			GList *pItems =
					secret_service_search_sync( pService,
			                                &schema,
			                                pAttributes,
			                                static_cast<SecretSearchFlags>( SECRET_SEARCH_ALL | SECRET_SEARCH_UNLOCK ),
			                                pCancellation,
			                                &pError );
			bool bRemoved = pError == nullptr;
			for ( GList *p = pItems; p && bRemoved; p = p->next )
			{
				// password_clear only removes unlocked items; explicitly check every
				// matching item so a locked password is never reported as forgotten.
				SecretItem *pItem = SECRET_ITEM( p->data );
				bRemoved = !secret_item_get_locked( pItem ) && secret_item_delete_sync( pItem, pCancellation, &pError );
			}
			g_list_free_full( pItems, g_object_unref );
			g_hash_table_unref( pAttributes );
			g_object_unref( pService );
			g_clear_error( &pError );
			return bRemoved ? ESecretResult::OK : ESecretResult::UNAVAILABLE;
		}
	};

} // namespace

std::unique_ptr<ISecuredSettings> CreateSecuredSettings()
{
	return std::make_unique<CLinuxSecuredSettings>();
}

} // namespace NSettings
