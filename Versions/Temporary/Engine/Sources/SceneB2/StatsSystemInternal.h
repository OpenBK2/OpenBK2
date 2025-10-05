#pragma once

#include "statsystem.h"

class CStatsSystem :	public IStatSystem
{
	OBJECT_NOCOPY_METHODS( CStatsSystem )

public:
	void UpdateEntry( const std::string &szName, const std::string &szEntry, const DWORD dwColor = 0xffffffff );
	int operator&( IBinSaver &saver );
};


