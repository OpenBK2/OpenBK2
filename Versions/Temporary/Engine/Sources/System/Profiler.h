#pragma once

#include <cstdint>

namespace NProfiler
{
class CProfiler
{
	std::string szFile;
	int nLine;
	uint32_t dwStartTime;
public:
	CProfiler( const char* pszFile, const int nLine );
	~CProfiler();
};

void DumpStats();
}

#define PROFILE_BLOCK NProfiler::CProfiler profiler( __FILE__, __LINE__ );

