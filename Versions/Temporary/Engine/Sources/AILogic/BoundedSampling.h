#pragma once

#include <cmath>

namespace NBoundedSampling
{
// Reject corrupt spacing rather than turning one shot or construction order into millions of samples.
constexpr int MAX_SAMPLES = 4096;

inline bool IsValidSpacing( const float fLength, const float fSpacing )
{
	return std::isfinite( fLength ) && fLength >= 0.0f && std::isfinite( fSpacing ) && fSpacing > 0.0f &&
	       double( fLength ) / double( fSpacing ) <= MAX_SAMPLES;
}

class CTrace
{
	float fLength;
	float fSpacing;
	float fNext = 0.0f;
	int nRemaining = MAX_SAMPLES;
	bool bValid;

public:
	CTrace( const float _fLength, const float _fSpacing )
		: fLength( _fLength )
		, fSpacing( _fSpacing )
		, bValid( IsValidSpacing( _fLength, _fSpacing ) )
	{
	}

	bool IsValid() const
	{
		return bValid;
	}

	bool GetNext( float *pDistance )
	{
		if ( !bValid || nRemaining == 0 || !( fNext < fLength ) )
		{
			return false;
		}
		*pDistance = fNext;
		--nRemaining;
		const float fAdvanced = fNext + fSpacing;
		if ( !( fAdvanced > fNext ) )
		{
			// Float addition must advance even if a loaded value has lost useful precision.
			nRemaining = 0;
		}
		fNext = fAdvanced;
		return true;
	}
};
} // namespace NBoundedSampling
