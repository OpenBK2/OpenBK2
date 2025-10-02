#pragma once

namespace NAnimation
{
	struct SAnimHandle;
	unsigned int GetMarkTimes( vector<float> *pResult, const SAnimHandle &animHandle, const string &szTrackName, const string &szMarkName );
}

