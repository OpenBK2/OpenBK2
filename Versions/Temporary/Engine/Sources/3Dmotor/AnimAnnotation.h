#pragma once

namespace NAnimation
{
	struct SAnimHandle;
	unsigned int GetMarkTimes( std::vector<float> *pResult, const SAnimHandle &animHandle, const std::string &szTrackName, const std::string &szMarkName );
}

