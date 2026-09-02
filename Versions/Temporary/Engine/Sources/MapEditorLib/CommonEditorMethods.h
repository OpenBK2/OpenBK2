#pragma once

#include <cstdint>

#include "MapEditorLib_export.h"

MAPEDITORLIB_EXPORT bool SetGetEditParameters( uint32_t pEditParameters, unsigned nCommandHandlerType, int nCmdID );
MAPEDITORLIB_EXPORT void CreateRefKey( std::string *pszKey, const struct SPropertyDesc *pPropertyDesc );



