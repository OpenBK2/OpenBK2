#pragma once

#include "Server_Client_Common_export.h"

// IBinSaver and ESaverMode, named below. They used to arrive through whatever
// prelude the includer happened to have, which made this header's include order
// load-bearing: clang-format sorts a block alphabetically, so a unit that did
// not include BinSaver.h first could not be made to compile by rearranging.
#include "System/BinSaver.h"

typedef int TPACKET_ID;

// Shared by network clients and regression tests; Windows requires an export.
SERVER_CLIENT_COMMON_EXPORT IBinSaver *CreateNetSaver( class CMemoryStream *pStream, ESaverMode mode );
