#pragma once

#include "Server_Client_Common_export.h"

typedef int TPACKET_ID;

// Shared by network clients and regression tests; Windows requires an export.
SERVER_CLIENT_COMMON_EXPORT IBinSaver* CreateNetSaver( class CMemoryStream *pStream, ESaverMode mode );


