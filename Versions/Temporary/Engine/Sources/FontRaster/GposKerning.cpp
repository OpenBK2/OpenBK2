#include "GposKerning.h"

#include <algorithm>
#include <utility>

namespace NFontRaster
{

namespace
{

// Big-endian reads that fail rather than run past the table
bool ReadU16( const std::vector<uint8_t> &table, const uint32_t nOffset, uint16_t *pnValue )
{
	if ( static_cast<uint64_t>( nOffset ) + 2 > table.size() )
		return false;
	*pnValue = static_cast<uint16_t>( ( table[nOffset] << 8 ) | table[nOffset + 1] );
	return true;
}

bool ReadU32( const std::vector<uint8_t> &table, const uint32_t nOffset, uint32_t *pnValue )
{
	uint16_t nHigh = 0, nLow = 0;
	if ( !ReadU16( table, nOffset, &nHigh ) || !ReadU16( table, nOffset + 2, &nLow ) )
		return false;
	*pnValue = ( static_cast<uint32_t>( nHigh ) << 16 ) | nLow;
	return true;
}

int CountBits( uint16_t nValue )
{
	int nCount = 0;
	for ( ; nValue != 0; nValue &= nValue - 1 )
		++nCount;
	return nCount;
}

// The glyph's index in a Coverage table, or -1 when it is not covered
int CoverageIndex( const std::vector<uint8_t> &table, const uint32_t nCoverage, const uint16_t nGlyph )
{
	uint16_t nFormat = 0, nCount = 0;
	if ( !ReadU16( table, nCoverage, &nFormat ) || !ReadU16( table, nCoverage + 2, &nCount ) )
		return -1;
	// both formats are sorted by glyph, so a binary search either way
	int nLow = 0, nHigh = static_cast<int>( nCount ) - 1;
	while ( nLow <= nHigh )
	{
		const int nMiddle = ( nLow + nHigh ) / 2;
		if ( nFormat == 1 )
		{
			uint16_t nListed = 0;
			if ( !ReadU16( table, nCoverage + 4 + nMiddle * 2, &nListed ) )
				return -1;
			if ( nListed == nGlyph )
				return nMiddle;
			if ( nListed < nGlyph )
				nLow = nMiddle + 1;
			else
				nHigh = nMiddle - 1;
		}
		else if ( nFormat == 2 )
		{
			uint16_t nStart = 0, nEnd = 0, nStartIndex = 0;
			const uint32_t nRange = nCoverage + 4 + nMiddle * 6;
			if ( !ReadU16( table, nRange, &nStart ) || !ReadU16( table, nRange + 2, &nEnd ) || !ReadU16( table, nRange + 4, &nStartIndex ) )
				return -1;
			if ( nGlyph < nStart )
				nHigh = nMiddle - 1;
			else if ( nGlyph > nEnd )
				nLow = nMiddle + 1;
			else
				return nStartIndex + ( nGlyph - nStart );
		}
		else
		{
			return -1;
		}
	}
	return -1;
}

// The glyph's class in a ClassDef table; a glyph it does not list is class 0
int ClassOf( const std::vector<uint8_t> &table, const uint32_t nClassDef, const uint16_t nGlyph )
{
	uint16_t nFormat = 0;
	if ( !ReadU16( table, nClassDef, &nFormat ) )
		return 0;
	if ( nFormat == 1 )
	{
		uint16_t nStart = 0, nCount = 0, nClass = 0;
		if ( !ReadU16( table, nClassDef + 2, &nStart ) || !ReadU16( table, nClassDef + 4, &nCount ) )
			return 0;
		if ( nGlyph < nStart || nGlyph >= nStart + nCount )
			return 0;
		return ReadU16( table, nClassDef + 6 + ( nGlyph - nStart ) * 2, &nClass ) ? nClass : 0;
	}
	if ( nFormat == 2 )
	{
		uint16_t nCount = 0;
		if ( !ReadU16( table, nClassDef + 2, &nCount ) )
			return 0;
		int nLow = 0, nHigh = static_cast<int>( nCount ) - 1;
		while ( nLow <= nHigh )
		{
			const int nMiddle = ( nLow + nHigh ) / 2;
			uint16_t nStart = 0, nEnd = 0, nClass = 0;
			const uint32_t nRange = nClassDef + 4 + nMiddle * 6;
			if ( !ReadU16( table, nRange, &nStart ) || !ReadU16( table, nRange + 2, &nEnd ) || !ReadU16( table, nRange + 4, &nClass ) )
				return 0;
			if ( nGlyph < nStart )
				nHigh = nMiddle - 1;
			else if ( nGlyph > nEnd )
				nLow = nMiddle + 1;
			else
				return nClass;
		}
	}
	return 0;
}

}

CGposKerning::CGposKerning( std::vector<uint8_t> _table ) : table( std::move( _table ) )
{
	uint16_t nMajor = 0, nFeatureList = 0, nLookupList = 0;
	if ( !ReadU16( table, 0, &nMajor ) || nMajor != 1 || !ReadU16( table, 6, &nFeatureList ) || !ReadU16( table, 8, &nLookupList ) )
		return;
	// every lookup any "kern" feature refers to, whatever the script: the
	// engine does no script itemisation, and fonts give every script the same
	// kerning lookups in practice
	std::vector<uint16_t> kernLookups;
	uint16_t nFeatures = 0;
	if ( !ReadU16( table, nFeatureList, &nFeatures ) )
		return;
	for ( int i = 0; i < nFeatures; ++i )
	{
		const uint32_t nRecord = nFeatureList + 2 + i * 6;
		if ( static_cast<uint64_t>( nRecord ) + 6 > table.size() )
			return;
		if ( table[nRecord] != 'k' || table[nRecord + 1] != 'e' || table[nRecord + 2] != 'r' || table[nRecord + 3] != 'n' )
			continue;
		uint16_t nFeatureOffset = 0, nCount = 0;
		if ( !ReadU16( table, nRecord + 4, &nFeatureOffset ) )
			return;
		const uint32_t nFeature = nFeatureList + nFeatureOffset;
		if ( !ReadU16( table, nFeature + 2, &nCount ) )
			continue;
		for ( int j = 0; j < nCount; ++j )
		{
			uint16_t nIndex = 0;
			if ( ReadU16( table, nFeature + 4 + j * 2, &nIndex ) )
				kernLookups.push_back( nIndex );
		}
	}
	// lookups apply in the order the lookup list gives them
	std::sort( kernLookups.begin(), kernLookups.end() );
	kernLookups.erase( std::unique( kernLookups.begin(), kernLookups.end() ), kernLookups.end() );
	uint16_t nLookupCount = 0;
	if ( !ReadU16( table, nLookupList, &nLookupCount ) )
		return;
	for ( const uint16_t nIndex : kernLookups )
	{
		uint16_t nLookupOffset = 0, nType = 0, nSubtables = 0;
		if ( nIndex >= nLookupCount || !ReadU16( table, nLookupList + 2 + nIndex * 2, &nLookupOffset ) )
			continue;
		const uint32_t nLookup = nLookupList + nLookupOffset;
		if ( !ReadU16( table, nLookup, &nType ) || !ReadU16( table, nLookup + 4, &nSubtables ) )
			continue;
		std::vector<uint32_t> subtables;
		for ( int k = 0; k < nSubtables; ++k )
		{
			uint16_t nSubtableOffset = 0;
			if ( !ReadU16( table, nLookup + 6 + k * 2, &nSubtableOffset ) )
				break;
			uint32_t nSubtable = nLookup + nSubtableOffset;
			if ( nType == 9 )
			{
				// an extension: the real lookup type, and a 32-bit offset to the
				// subtable from the extension's own start
				uint16_t nExtensionType = 0;
				uint32_t nExtensionOffset = 0;
				if ( !ReadU16( table, nSubtable + 2, &nExtensionType ) || nExtensionType != 2 ||
					!ReadU32( table, nSubtable + 4, &nExtensionOffset ) )
					continue;
				nSubtable += nExtensionOffset;
			}
			else if ( nType != 2 )
			{
				continue;
			}
			uint16_t nFormat = 0;
			if ( ReadU16( table, nSubtable, &nFormat ) && ( nFormat == 1 || nFormat == 2 ) )
				subtables.push_back( nSubtable );
		}
		if ( !subtables.empty() )
			lookups.push_back( std::move( subtables ) );
	}
}

int CGposKerning::FindInSubtable( const uint32_t nSubtable, const uint16_t nLeft, const uint16_t nRight, bool *pbFound ) const
{
	*pbFound = false;
	uint16_t nFormat = 0, nCoverage = 0, nFormat1 = 0, nFormat2 = 0;
	if ( !ReadU16( table, nSubtable, &nFormat ) || !ReadU16( table, nSubtable + 2, &nCoverage ) ||
		!ReadU16( table, nSubtable + 4, &nFormat1 ) || !ReadU16( table, nSubtable + 6, &nFormat2 ) )
		return 0;
	const int nCovered = CoverageIndex( table, nSubtable + nCoverage, nLeft );
	if ( nCovered < 0 )
		return 0;
	// A value record holds 2 bytes per bit its format sets, in bit order, and
	// the advance is bit 2, after the two placements
	const int nSize1 = CountBits( nFormat1 & 0xFF ) * 2;
	const int nSize2 = CountBits( nFormat2 & 0xFF ) * 2;
	const int nAdvanceAt = ( nFormat1 & 0x0004 ) != 0 ? CountBits( nFormat1 & 0x0003 ) * 2 : -1;
	uint32_t nValueRecord = 0;
	if ( nFormat == 1 )
	{
		// a set of pairs per covered first glyph, sorted by the second
		uint16_t nSets = 0, nSetOffset = 0, nPairs = 0;
		if ( !ReadU16( table, nSubtable + 8, &nSets ) || nCovered >= nSets ||
			!ReadU16( table, nSubtable + 10 + nCovered * 2, &nSetOffset ) )
			return 0;
		const uint32_t nSet = nSubtable + nSetOffset;
		if ( !ReadU16( table, nSet, &nPairs ) )
			return 0;
		const int nRecordSize = 2 + nSize1 + nSize2;
		int nLow = 0, nHigh = static_cast<int>( nPairs ) - 1;
		while ( nLow <= nHigh && nValueRecord == 0 )
		{
			const int nMiddle = ( nLow + nHigh ) / 2;
			uint16_t nSecond = 0;
			if ( !ReadU16( table, nSet + 2 + nMiddle * nRecordSize, &nSecond ) )
				return 0;
			if ( nSecond < nRight )
				nLow = nMiddle + 1;
			else if ( nSecond > nRight )
				nHigh = nMiddle - 1;
			else
				nValueRecord = nSet + 2 + nMiddle * nRecordSize + 2;
		}
		if ( nValueRecord == 0 )
			return 0;
	}
	else
	{
		// a matrix of values by the classes of the two glyphs; a covered first
		// glyph always finds an entry, class 0 being every unlisted glyph
		uint16_t nClassDef1 = 0, nClassDef2 = 0, nClasses1 = 0, nClasses2 = 0;
		if ( !ReadU16( table, nSubtable + 8, &nClassDef1 ) || !ReadU16( table, nSubtable + 10, &nClassDef2 ) ||
			!ReadU16( table, nSubtable + 12, &nClasses1 ) || !ReadU16( table, nSubtable + 14, &nClasses2 ) )
			return 0;
		const int nClass1 = ClassOf( table, nSubtable + nClassDef1, nLeft );
		const int nClass2 = ClassOf( table, nSubtable + nClassDef2, nRight );
		if ( nClass1 >= nClasses1 || nClass2 >= nClasses2 )
			return 0;
		nValueRecord = nSubtable + 16 + ( nClass1 * nClasses2 + nClass2 ) * ( nSize1 + nSize2 );
	}
	*pbFound = true;
	uint16_t nAdvance = 0;
	if ( nAdvanceAt < 0 || !ReadU16( table, nValueRecord + nAdvanceAt, &nAdvance ) )
		return 0;
	return static_cast<int16_t>( nAdvance );
}

int CGposKerning::GetAdjustment( const uint16_t nLeft, const uint16_t nRight ) const
{
	int nTotal = 0;
	for ( const std::vector<uint32_t> &subtables : lookups )
	{
		for ( const uint32_t nSubtable : subtables )
		{
			bool bFound = false;
			const int nValue = FindInSubtable( nSubtable, nLeft, nRight, &bFound );
			if ( bFound )
			{
				nTotal += nValue;
				break;
			}
		}
	}
	return nTotal;
}

}
