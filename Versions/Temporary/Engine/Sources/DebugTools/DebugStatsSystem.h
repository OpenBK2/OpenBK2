#pragma once

#include "UI/commandparam.h"
#include "UI/dbuserinterface.h"

#include <cstdint>

namespace NDebugInfo
{
	void UpdateEntry( const std::string &szName, const std::string &szValue, const uint32_t dwColor );
}

