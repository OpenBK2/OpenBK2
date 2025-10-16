#pragma once

#include <fmt/format.h>

#if defined(_DO_ASSERT_SLOW)

inline bool CheckFixedRange( const int nIndex, const int nSize, const char *pszName )
{
	NI_ASSERT( nIndex >= 0 && nIndex < nSize, fmt::format("Index ({}) must be in the range [0..{}) for \"{}\"", nIndex, nSize, pszName) );
	return nIndex >= 0 && nIndex < nSize;
}

template <class TContainer>
inline bool CheckRange( const TContainer &container, const int nIndex )
{
	NI_ASSERT( nIndex >= 0 && nIndex < container.size(), fmt::format("Index ({}) must be in the range [0..{})", nIndex, container.size()) );
	return nIndex >= 0 && nIndex < container.size();
}

#else

#define CheckRange( x, y ) true
#define CheckFixedRange( x, y, z ) true

#endif // use ctrl + }


