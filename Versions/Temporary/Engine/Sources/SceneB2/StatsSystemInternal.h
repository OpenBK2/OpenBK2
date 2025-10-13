#pragma once

#include "statsystem.h"

#include <cstdint>

class CStatsSystem :	public IStatSystem
{
	OBJECT_NOCOPY_METHODS( CStatsSystem )

public:
	void UpdateEntry( const std::string &szName, const std::string &szEntry, const uint32_t dwColor = 0xffffffff );
	int operator&( IBinSaver &saver );
};


