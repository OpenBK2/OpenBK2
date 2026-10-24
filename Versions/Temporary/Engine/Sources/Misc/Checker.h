#pragma once

#include <fmt/format.h>

#if defined( _DO_ASSERT_SLOW )

inline bool CheckFixedRange( const int nIndex, const int nSize, const char *pszName )
{
	NI_ASSERT( nIndex >= 0 && nIndex < nSize,
	           fmt::format( "Index ({}) must be in the range [0..{}) for \"{}\"", nIndex, nSize, pszName ) );
	return nIndex >= 0 && nIndex < nSize;
}

template <class TContainer>
inline bool CheckRange( const TContainer &container, const int nIndex )
{
	NI_ASSERT( nIndex >= 0 && nIndex < container.size(),
	           fmt::format( "Index ({}) must be in the range [0..{})", nIndex, container.size() ) );
	return nIndex >= 0 && nIndex < container.size();
}

#else

// Both of these expanded to the literal true, so each of the six call sites in AILogic read as a
// range check and compiled to a statement whose value is discarded, which is the one spelling that
// cannot be distinguished from a check that passed. ( (void)0 ) says what the configuration does.
//
// Even with _DO_ASSERT_SLOW these only assert and return a bool that every call site drops, so the
// index reaches the container either way: an accessor that has to survive a bad index needs its own
// guard, not one of these.
#define CheckRange( x, y ) ( (void)0 )
#define CheckFixedRange( x, y, z ) ( (void)0 )

#endif // use ctrl + }
