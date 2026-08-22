#pragma once

#include "UI/CommandParam.h"
#include "UI/DBUserInterface.h"

#include <cstdint>

namespace NDebugInfo
{
	void UpdateEntry( const std::string &szName, const std::string &szValue, const uint32_t dwColor );
}

