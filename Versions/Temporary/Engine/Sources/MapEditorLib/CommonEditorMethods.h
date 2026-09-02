#pragma once

#include <cstdint>

bool SetGetEditParameters( uint32_t pEditParameters, unsigned nCommandHandlerType, int nCmdID );
void CreateRefKey( std::string *pszKey, const struct SPropertyDesc *pPropertyDesc );



