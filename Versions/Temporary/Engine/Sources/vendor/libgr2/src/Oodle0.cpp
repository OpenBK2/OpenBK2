// Oodle0 decompression.
//
// PROVENANCE. A derived work, not an independent implementation. The algorithm
// comes from blendergranny's io_scene_gr2/gr2/decompress/oodle0.py, which is MIT
// licensed, Copyright (c) 2026 ciupix and contributors, and which carries an
// explicit clean-room policy in its docs/PROVENANCE.md: behaviour studied from
// private references, code written fresh, notes kept at the algorithm level. That
// policy is the reason this is the version to derive from rather than
// granny-ro-js's port of it, whose comments cite a leaked RAD source path.
//
//   https://github.com/ciupix/blendergranny  (io_scene_gr2/gr2/decompress/oodle0.py)
//
// Its Oodle0 was checked against the real DLL over 15,742 files with identical
// SHA-256 on every one, so it is a reference worth being faithful to.
//
// WHAT CHANGED from that reference, and why:
//
//  - Python integers are unbounded and C++ ones are not, so every place the
//    original relies on that is written out: the range coder's products need 64
//    bits, and the model's total bins are explicitly 16-bit lanes packed two to a
//    32-bit add, which is what the arithmetic in the reference is modelling.
//  - Every read and write is bounds checked and failure is a return value rather
//    than an exception, because this runs on whatever a .pak contains.
//  - The bit reader takes its buffer's length, so running off the end is a
//    refusal rather than reading zeros forever.
//
// HOW IT WORKS. Three layers.
//
// A VARIABLE BIT READER pulls little-endian 32-bit words and hands out bits from
// the low end.
//
// A RANGE CODER sits on that, with a 31-bit low, high and code. It is the
// unusual part: the code register is filled with bit-reversed input, whole
// nibbles at a time when it renormalises by eight, which is a detail no amount of
// reasoning recovers and which has to be copied exactly.
//
// An ADAPTIVE MODEL sits on that. Symbols are counted as they appear, and a
// symbol the model has never seen is coded as an escape at position zero followed
// by the raw value, which is why every read here is "ask the model, and if it
// escapes, read the value and teach it". Counts are kept in sixteen cumulative
// bins so the coder's scale is one lookup, and halved when they grow past 16,384
// so recent statistics dominate.
//
// The LZ LAYER above chooses per symbol between a literal and a back reference.
// A length of zero means a literal. Lengths come from one of 65 models selected
// by the previous length, so runs of similar lengths cost almost nothing, and the
// four highest codes mean 128, 192, 256 and 512. An offset is split at two bits,
// low part and high part, each from its own model.

#include "Oodle0.h"

#include "Trace.h"

#include <cstring>
#include <vector>

namespace NGr2
{

namespace
{

constexpr uint32_t HEADER_SIZE = 36;
constexpr uint32_t STAGE_COUNT = 3;
constexpr uint32_t OFFSET_SPLIT_SHIFT = 2;
constexpr uint32_t LOW_OFFSET_MASK = ( 1u << OFFSET_SPLIT_SHIFT ) - 1;
constexpr uint32_t MAX_LENGTHS = 64;
constexpr uint32_t MASK31 = 0x7fffffffu;

//! The four longest length codes, in place of a count.
constexpr uint32_t LONG_LENGTHS[4] = { MAX_LENGTHS * 2, MAX_LENGTHS * 3, MAX_LENGTHS * 4,
                                       MAX_LENGTHS * 8 };

//! Counts are halved once their total reaches this.
constexpr uint32_t RESCALE_TOTAL = 16384;

//! Reverse the low nBits of a value. The code register is filled with these.
uint32_t BitReverse( uint32_t nValue, uint32_t nBits )
{
	uint32_t nResult = 0;
	for ( uint32_t i = 0; i < nBits; ++i )
	{
		nResult = ( nResult << 1 ) | ( ( nValue >> i ) & 1u );
	}
	return nResult;
}

//! Bits out of a little-endian word stream, low end first.
class CVarBits
{
public:
	CVarBits( const uint8_t *pData, uint32_t nSize, uint32_t nAt )
		: m_pData( pData )
		, m_nSize( nSize )
		, m_nAt( nAt )
	{
	}

	uint32_t Get( uint32_t nBits );
	uint32_t Get1();

	bool Failed() const { return m_bFailed; }

private:
	//! The next word, zero padded past the end, which the format relies on.
	//!
	//! Reading past the end is normal at the tail and is not an error by itself.
	//! Reading far past it is, and that is what the limit catches.
	uint32_t ReadWord();

	const uint8_t *m_pData = nullptr;
	uint32_t m_nSize = 0;
	uint32_t m_nAt = 0;
	uint32_t m_nBits = 0;
	uint32_t m_nBitLen = 0;
	bool m_bFailed = false;
};

uint32_t CVarBits::ReadWord()
{
	// Eight words of slack, which is more than any renormalisation can consume
	// after the last real byte.
	if ( m_nAt > m_nSize + 32 )
	{
		m_bFailed = true;
		return 0;
	}

	uint8_t bytes[4] = { 0, 0, 0, 0 };
	for ( uint32_t i = 0; i < 4; ++i )
	{
		if ( m_nAt + i < m_nSize )
		{
			bytes[i] = m_pData[m_nAt + i];
		}
	}
	m_nAt += 4;

	uint32_t nWord = 0;
	memcpy( &nWord, bytes, sizeof( nWord ) );
	return nWord;
}

uint32_t CVarBits::Get( uint32_t nBits )
{
	if ( nBits == 0 || nBits > 31 )
	{
		return 0;
	}

	const uint64_t nMask = ( uint64_t( 1 ) << nBits ) - 1;
	if ( m_nBitLen >= nBits )
	{
		const uint32_t nValue = static_cast<uint32_t>( m_nBits & nMask );
		m_nBits >>= nBits;
		m_nBitLen -= nBits;
		return nValue;
	}

	const uint32_t nWord = ReadWord();
	const uint32_t nValue = static_cast<uint32_t>(
		( uint64_t( m_nBits ) | ( uint64_t( nWord ) << m_nBitLen ) ) & nMask );
	m_nBits = nWord >> ( nBits - m_nBitLen );
	m_nBitLen = m_nBitLen + 32 - nBits;
	return nValue;
}

uint32_t CVarBits::Get1()
{
	if ( m_nBitLen != 0 )
	{
		const uint32_t nValue = m_nBits & 1u;
		m_nBits >>= 1;
		--m_nBitLen;
		return nValue;
	}

	const uint32_t nWord = ReadWord();
	m_nBits = nWord >> 1;
	m_nBitLen = 31;
	return nWord & 1u;
}

//! The range coder.
class CArithBits
{
public:
	CArithBits( const uint8_t *pData, uint32_t nSize, uint32_t nAt )
		: m_Bits( pData, nSize, nAt )
	{
		m_nCode = BitReverse( m_Bits.Get( 31 ), 31 );
	}

	//! Where the code sits within a range of nScale, before narrowing.
	uint32_t GetCount( uint32_t nScale ) const;

	//! Narrow to [nStart, nStart + nCount) of nScale, and refill.
	void Remove( uint32_t nStart, uint32_t nCount, uint32_t nScale );

	//! One exact value out of nScale, which is how a raw escape is read.
	uint32_t GetValue( uint32_t nScale );

	bool Failed() const { return m_Bits.Failed(); }

private:
	CVarBits m_Bits;
	uint32_t m_nHigh = MASK31;
	uint32_t m_nLow = 0;
	uint32_t m_nCode = 0;
};

uint32_t CArithBits::GetCount( uint32_t nScale ) const
{
	if ( nScale == 0 )
	{
		return 0;
	}
	// 64-bit because the product is up to 2^31 times the scale. This is the one
	// line where Python's unbounded integers were doing real work.
	const uint64_t nRange = uint64_t( m_nHigh - m_nLow ) + 1;
	const uint64_t nOffset = uint64_t( m_nCode - m_nLow ) + 1;
	return static_cast<uint32_t>( ( nOffset * nScale - 1 ) / nRange );
}

void CArithBits::Remove( uint32_t nStart, uint32_t nCount, uint32_t nScale )
{
	if ( nScale == 0 )
	{
		return;
	}

	uint32_t nHigh = m_nHigh;
	uint32_t nLow = m_nLow;
	uint32_t nCode = m_nCode;

	const uint64_t nWidth = uint64_t( nHigh - nLow ) + 1;
	nHigh = static_cast<uint32_t>( nLow + ( nWidth * ( uint64_t( nStart ) + nCount ) ) / nScale
	                               - 1 );
	nLow = static_cast<uint32_t>( nLow + ( nWidth * nStart ) / nScale );

	// Renormalise. The nibble-reversed refills are the part to copy rather than
	// reason about: the encoder wrote them that way.
	if ( ( ( nHigh ^ nLow ) & 0x40000000u ) == 0 )
	{
		while ( ( ( nHigh ^ nLow ) & 0x7f800000u ) == 0 )
		{
			nLow <<= 8;
			nHigh = ( nHigh << 8 ) | 0xffu;
			const uint32_t nByte = m_Bits.Get( 8 );
			nCode = ( nCode << 8 ) | ( BitReverse( nByte & 0x0fu, 4 ) << 4 )
			        | BitReverse( nByte >> 4, 4 );
		}
		if ( ( ( nHigh ^ nLow ) & 0x78000000u ) == 0 )
		{
			nLow <<= 4;
			nHigh = ( nHigh << 4 ) | 0x0fu;
			nCode = ( nCode << 4 ) | BitReverse( m_Bits.Get( 4 ), 4 );
		}
		while ( ( ( nHigh ^ nLow ) & 0x40000000u ) == 0 )
		{
			nLow <<= 1;
			nHigh = ( nHigh << 1 ) | 1u;
			nCode = ( nCode << 1 ) | m_Bits.Get1();
		}
	}

	// Underflow: the two straddle the midpoint without agreeing on the bit above.
	while ( ( nLow & 0x20000000u ) != 0 && ( nHigh & 0x20000000u ) == 0 )
	{
		nCode ^= 0x20000000u;
		nLow = ( nLow & 0x1fffffffu ) << 1;
		nHigh = ( nHigh << 1 ) | 0x40000001u;
		nCode = ( nCode << 1 ) | m_Bits.Get1();
	}

	m_nHigh = nHigh & MASK31;
	m_nLow = nLow & MASK31;
	m_nCode = nCode & MASK31;
}

uint32_t CArithBits::GetValue( uint32_t nScale )
{
	uint32_t nValue = GetCount( nScale );
	if ( nScale != 0 && nValue >= nScale )
	{
		nValue = nScale - 1;
	}
	Remove( nValue, 1, nScale );
	return nValue;
}

//! Where a model's counts should be split across sixteen cumulative bins.
struct SBinning
{
	uint32_t nShift = 15;
	uint32_t nLastBinStart = 0;
};

SBinning BestShift( uint32_t nValue )
{
	SBinning out;
	if ( nValue < 6 )
	{
		return out;
	}

	uint32_t nBestMax = 0xffffffffu;
	uint32_t nBestBin = 0;
	for ( uint32_t i = 0; i < 16; ++i )
	{
		const uint32_t nSize = 1u << i;
		uint32_t nBins = ( nValue + nSize - 1 ) / nSize;
		if ( nBins > 16 )
		{
			nBins = 16;
		}
		uint32_t nLast = nValue - nSize * ( nBins - 1 );
		if ( nLast < nSize )
		{
			nLast = nSize;
		}
		if ( nLast < nBestMax )
		{
			nBestBin = i;
			nBestMax = nLast;
		}
		if ( nSize > nValue )
		{
			break;
		}
	}

	out.nShift = nBestBin;
	out.nLastBinStart = 15u * ( 1u << nBestBin );
	return out;
}

//! What a model returned: a known value, or an escape and the slot to fill.
struct SSymbol
{
	uint32_t nValue = 0;
	uint32_t nEscapeIndex = 0;
	bool bEscape = false;
	bool bFailed = false;
};

//! An adaptive symbol model with an escape at position zero.
class CArithModel
{
public:
	explicit CArithModel( uint32_t nUniqueValues );

	SSymbol Decompress( CArithBits &bits );
	void SetEscaped( uint32_t nIndex, uint32_t nValue )
	{
		m_Values[nIndex] = static_cast<uint16_t>( nValue );
	}

private:
	//! Which entry a coder count falls in, and where that entry starts.
	bool FindPos( uint32_t nCount, uint32_t *pnPos, uint32_t *pnStart ) const;

	//! Add to the cumulative bins.
	//!
	//! nAmount carries two 16-bit lanes and is added as one 32-bit value across
	//! pairs of bins, which is what makes a single add update two running totals.
	void IncrementTotals( uint32_t nValue, uint32_t nAmount );
	void AddTotalPair( uint32_t nPairIndex, uint32_t nAmount );
	void QuickIncrement( uint32_t nValue, uint32_t nAmount );
	void DecrementCounts( uint32_t nValue, uint32_t nAmount );
	void Rescale();

	uint32_t BinOf( uint32_t nIndex ) const
	{
		return nIndex < m_Binning.nLastBinStart ? ( nIndex >> m_Binning.nShift ) : 15u;
	}

	uint32_t m_nUniqueValues = 0;
	uint16_t m_Totals[16] = {};
	std::vector<uint16_t> m_Counts;
	std::vector<uint16_t> m_Values;
	uint32_t m_nNumber = 0;
	SBinning m_Binning;
};

CArithModel::CArithModel( uint32_t nUniqueValues )
	: m_nUniqueValues( nUniqueValues )
{
	// Rounded up with room to spare, which is what the encoder assumes.
	const uint32_t nCount = ( nUniqueValues + 5 ) & ~3u;
	m_Counts.assign( nCount, 0 );
	m_Values.assign( nCount, 0 );
	m_Binning = BestShift( nUniqueValues + 1 );
	// The escape starts with a weight of three, in both lanes.
	QuickIncrement( 0, 0x00030003u );
}

void CArithModel::AddTotalPair( uint32_t nPairIndex, uint32_t nAmount )
{
	const uint32_t nIndex = nPairIndex * 2;
	const uint32_t nOld =
		uint32_t( m_Totals[nIndex] ) | ( uint32_t( m_Totals[nIndex + 1] ) << 16 );
	const uint32_t nNew = nOld + nAmount;
	m_Totals[nIndex] = static_cast<uint16_t>( nNew & 0xffffu );
	m_Totals[nIndex + 1] = static_cast<uint16_t>( nNew >> 16 );
}

void CArithModel::IncrementTotals( uint32_t nValue, uint32_t nAmount )
{
	const uint16_t nLow = static_cast<uint16_t>( nAmount & 0xffffu );
	if ( nValue >= m_Binning.nLastBinStart )
	{
		m_Totals[15] = static_cast<uint16_t>( m_Totals[15] + nLow );
		return;
	}

	uint32_t nBin = nValue >> m_Binning.nShift;
	if ( ( nBin & 1u ) != 0 )
	{
		m_Totals[nBin] = static_cast<uint16_t>( m_Totals[nBin] + nLow );
		++nBin;
	}
	for ( uint32_t nPair = nBin >> 1; nPair < 8; ++nPair )
	{
		AddTotalPair( nPair, nAmount );
	}
}

void CArithModel::QuickIncrement( uint32_t nValue, uint32_t nAmount )
{
	IncrementTotals( nValue, nAmount );
	m_Counts[nValue] = static_cast<uint16_t>( m_Counts[nValue] + ( nAmount & 0xffffu ) );
}

void CArithModel::DecrementCounts( uint32_t nValue, uint32_t nAmount )
{
	const uint32_t nNegative = 0u - nAmount;
	QuickIncrement( nValue, ( ( nNegative - 1 ) << 16 ) | ( nNegative & 0xffffu ) );
}

bool CArithModel::FindPos( uint32_t nCount, uint32_t *pnPos, uint32_t *pnStart ) const
{
	uint32_t nStart = 0;
	for ( uint32_t i = 0; i < m_Counts.size(); ++i )
	{
		if ( nCount < nStart + m_Counts[i] )
		{
			*pnPos = i;
			*pnStart = nStart;
			return true;
		}
		nStart += m_Counts[i];
	}
	return false;
}

void CArithModel::Rescale()
{
	m_Binning = BestShift( m_nNumber + 1 );

	uint32_t Bins[16] = {};
	m_Counts[0] = static_cast<uint16_t>( m_Counts[0] >> 1 );
	Bins[BinOf( 0 )] += m_Counts[0];

	uint32_t nMaxCount = 0;
	uint32_t nMaxPos = 0;
	uint32_t nIndex = 1;
	bool bDone = false;
	while ( nIndex <= m_nNumber && !bDone )
	{
		// Drop everything that halved away, filling the gap from the end.
		while ( m_Counts[nIndex] <= 1 )
		{
			if ( nIndex < m_nNumber )
			{
				m_Counts[nIndex] = m_Counts[m_nNumber];
				m_Values[nIndex] = m_Values[m_nNumber];
				m_Counts[m_nNumber] = 0;
				--m_nNumber;
			}
			else
			{
				m_Counts[nIndex] = 0;
				--m_nNumber;
				bDone = true;
				break;
			}
		}
		if ( bDone )
		{
			break;
		}

		m_Counts[nIndex] = static_cast<uint16_t>( m_Counts[nIndex] >> 1 );
		if ( m_Counts[nIndex] > nMaxCount )
		{
			nMaxCount = m_Counts[nIndex];
			nMaxPos = nIndex;
		}
		Bins[BinOf( nIndex )] += m_Counts[nIndex];
		++nIndex;
	}

	if ( nMaxCount != 0 )
	{
		// The heaviest symbol moves to the front of the last bin, where the
		// search reaches it soonest.
		uint32_t nSwapPos = m_nNumber < m_Binning.nLastBinStart
		                        ? ( ( m_nNumber >> m_Binning.nShift ) << m_Binning.nShift )
		                        : m_Binning.nLastBinStart;
		if ( nSwapPos == 0 )
		{
			nSwapPos = 1;
		}
		if ( nMaxPos != nSwapPos )
		{
			const uint16_t nOldCount = m_Counts[nSwapPos];
			m_Counts[nSwapPos] = m_Counts[nMaxPos];
			Bins[BinOf( nSwapPos )] += uint32_t( m_Counts[nSwapPos] ) - nOldCount;
			Bins[BinOf( nMaxPos )] += uint32_t( nOldCount ) - m_Counts[nSwapPos];
			m_Counts[nMaxPos] = nOldCount;
			const uint16_t nSwapValue = m_Values[nSwapPos];
			m_Values[nSwapPos] = m_Values[nMaxPos];
			m_Values[nMaxPos] = nSwapValue;
		}
	}

	// The escape has to stay reachable while there are values still unseen.
	if ( m_nNumber != m_nUniqueValues && m_Counts[0] == 0 )
	{
		m_Counts[0] = static_cast<uint16_t>( m_Counts[0] + 2 );
		Bins[BinOf( 0 )] += 2;
	}

	uint32_t nRunning = 0;
	for ( uint32_t i = 0; i < 16; ++i )
	{
		nRunning += Bins[i];
		m_Totals[i] = static_cast<uint16_t>( nRunning );
	}
}

SSymbol CArithModel::Decompress( CArithBits &bits )
{
	SSymbol out;

	if ( m_Totals[15] >= RESCALE_TOTAL )
	{
		Rescale();
	}

	const uint32_t nScale = m_Totals[15];
	const uint32_t nCount = bits.GetCount( nScale );

	uint32_t nPos = 0;
	uint32_t nStart = 0;
	if ( !FindPos( nCount, &nPos, &nStart ) )
	{
		out.bFailed = true;
		return out;
	}

	const uint16_t nOldCount = m_Counts[nPos];
	// The totals are bumped before the range narrows, so the scale Remove sees is
	// the total from before this symbol. Order matters and is not obvious.
	IncrementTotals( nPos, 0x00010001u );
	bits.Remove( nStart, nOldCount, m_Totals[15] - 1u );
	m_Counts[nPos] = static_cast<uint16_t>( m_Counts[nPos] + 1 );

	if ( nPos != 0 )
	{
		out.nValue = m_Values[nPos];
		return out;
	}

	// An escape: a value this model has not seen. The caller reads it raw and
	// gives it back through SetEscaped.
	++m_nNumber;
	if ( m_nNumber >= m_Counts.size() )
	{
		out.bFailed = true;
		return out;
	}
	QuickIncrement( m_nNumber, 0x00020002u );
	if ( m_nNumber == m_nUniqueValues )
	{
		// Every value is now known, so the escape can never be needed again.
		DecrementCounts( 0, m_Counts[0] );
	}

	out.bEscape = true;
	out.nEscapeIndex = m_nNumber;
	return out;
}

//! The twelve bytes of parameters in front of each of the three stages.
struct SParameters
{
	uint32_t nMaxByteValue = 0;
	uint32_t nMaxOffset = 0;
	uint32_t nUniqueByteValues = 0;
	uint32_t nUniqueOffsets = 0;
	uint8_t nUniqueLengths[4] = {};

	//! How many distinct lengths the model for a given previous length expects.
	uint32_t LengthUnique( uint32_t nIndex ) const
	{
		uint32_t nGroup = nIndex / ( MAX_LENGTHS / 4 );
		if ( nGroup > 3 )
		{
			nGroup = 3;
		}
		return nUniqueLengths[3 - nGroup];
	}
};

SParameters ReadParameters( const uint8_t *pBytes )
{
	uint32_t nWords[3] = {};
	memcpy( nWords, pBytes, sizeof( nWords ) );

	SParameters out;
	// The same packing Oodle1 uses for its first word: nine bits then twenty
	// three. The second word is split the same way here, where Oodle1 splits the
	// upper part again.
	out.nMaxByteValue = nWords[0] & 0x1ffu;
	out.nMaxOffset = nWords[0] >> 9;
	out.nUniqueByteValues = nWords[1] & 0x1ffu;
	out.nUniqueOffsets = nWords[1] >> 9;
	memcpy( out.nUniqueLengths, &nWords[2], sizeof( out.nUniqueLengths ) );
	return out;
}

//! Every model one stage needs.
class CStage
{
public:
	explicit CStage( const SParameters &params );

	//! Decode into pOut until nStop bytes of output exist in total.
	//!
	//! \param nWritten how much of the output is already filled, in and out.
	bool Decode( CArithBits &bits, uint8_t *pOut, uint32_t nSize, uint32_t nStop,
	             uint32_t *pnWritten );

private:
	//! Ask a model, and read the value raw if the model escapes.
	static bool Draw( CArithModel &model, CArithBits &bits, uint32_t nEscapeScale,
	                  uint32_t *pnValue );

	uint32_t m_nMaxBytes = 0;
	uint32_t m_nMaxOffsets = 0;
	uint32_t m_nMaxOffsetLow = 0;
	uint32_t m_nBytesDecompressed = 0;
	uint32_t m_nLastLength = 0;

	CArithModel m_Bytes;
	CArithModel m_OffsetLow;
	CArithModel m_OffsetHigh;
	std::vector<CArithModel> m_Lengths;
};

CStage::CStage( const SParameters &params )
	: m_nMaxBytes( params.nMaxByteValue )
	, m_nMaxOffsets( params.nMaxOffset )
	, m_nMaxOffsetLow( params.nMaxOffset < LOW_OFFSET_MASK + 1 ? params.nMaxOffset
	                                                           : LOW_OFFSET_MASK + 1 )
	, m_Bytes( params.nUniqueByteValues )
	, m_OffsetLow( m_nMaxOffsetLow )
	, m_OffsetHigh( params.nUniqueOffsets )
{
	// One length model per previous length, which is what makes a run of equal
	// lengths nearly free.
	m_Lengths.reserve( MAX_LENGTHS + 1 );
	for ( uint32_t i = 0; i <= MAX_LENGTHS; ++i )
	{
		m_Lengths.emplace_back( params.LengthUnique( i ) );
	}
}

bool CStage::Draw( CArithModel &model, CArithBits &bits, uint32_t nEscapeScale,
                   uint32_t *pnValue )
{
	const SSymbol symbol = model.Decompress( bits );
	if ( symbol.bFailed || bits.Failed() )
	{
		return false;
	}
	if ( !symbol.bEscape )
	{
		*pnValue = symbol.nValue;
		return true;
	}

	*pnValue = bits.GetValue( nEscapeScale );
	model.SetEscaped( symbol.nEscapeIndex, *pnValue );
	return !bits.Failed();
}

bool CStage::Decode( CArithBits &bits, uint8_t *pOut, uint32_t nSize, uint32_t nStop,
                     uint32_t *pnWritten )
{
	while ( *pnWritten < nStop )
	{
		uint32_t nLengthSymbol = 0;
		if ( !Draw( m_Lengths[m_nLastLength], bits, MAX_LENGTHS + 1, &nLengthSymbol )
		     || nLengthSymbol > MAX_LENGTHS )
		{
			return false;
		}
		m_nLastLength = nLengthSymbol;

		if ( nLengthSymbol == 0 )
		{
			uint32_t nLiteral = 0;
			if ( !Draw( m_Bytes, bits, m_nMaxBytes, &nLiteral ) || nLiteral > 0xff
			     || *pnWritten >= nSize )
			{
				return false;
			}
			pOut[*pnWritten] = static_cast<uint8_t>( nLiteral );
			++*pnWritten;
			++m_nBytesDecompressed;
			continue;
		}

		const uint32_t nLength = nLengthSymbol >= MAX_LENGTHS - 3
		                             ? LONG_LENGTHS[nLengthSymbol - ( MAX_LENGTHS - 3 )]
		                             : nLengthSymbol + 1;

		uint32_t nLow = 0;
		if ( !Draw( m_OffsetLow, bits, m_nMaxOffsetLow, &nLow ) )
		{
			return false;
		}

		// The high part's range grows with what this stage has produced, so an
		// early match cannot name an offset that does not exist yet.
		const uint32_t nReach = m_nMaxOffsets < m_nBytesDecompressed ? m_nMaxOffsets
		                                                             : m_nBytesDecompressed;
		uint32_t nHigh = 0;
		if ( !Draw( m_OffsetHigh, bits, ( nReach >> OFFSET_SPLIT_SHIFT ) + 1, &nHigh ) )
		{
			return false;
		}

		const uint64_t nDistance =
			uint64_t( nLow ) + 1 + ( uint64_t( nHigh ) << OFFSET_SPLIT_SHIFT );
		if ( nDistance == 0 || nDistance > *pnWritten || nLength > nSize - *pnWritten )
		{
			// Before the start of the output, or past the end of it. The reference
			// checks the first and would run off the end on the second.
			return false;
		}

		// Byte at a time, because the source overlaps the destination whenever the
		// distance is shorter than the length, which is how a two byte offset
		// fills five hundred.
		const uint32_t nFrom = *pnWritten - static_cast<uint32_t>( nDistance );
		for ( uint32_t i = 0; i < nLength; ++i )
		{
			pOut[*pnWritten + i] = pOut[nFrom + i];
		}
		*pnWritten += nLength;
		m_nBytesDecompressed += nLength;
	}
	return true;
}

}

bool Oodle0Decompress( const uint8_t *pCompressed, uint32_t nCompressedSize, uint32_t nStop0,
                       uint32_t nStop1, uint8_t *pDecompressed, uint32_t nDecompressedSize )
{
	if ( pCompressed == nullptr || pDecompressed == nullptr )
	{
		return false;
	}
	if ( nCompressedSize < HEADER_SIZE )
	{
		return false;
	}

	// The stops are clamped rather than refused. first16Bit and first8Bit are
	// hints about where the 32, 16 and 8 bit data begin, and a file that puts
	// them out of order is still readable as long as the stages stay ordered.
	uint32_t nStops[STAGE_COUNT] = { nStop0, nStop1, nDecompressedSize };
	for ( uint32_t i = 0; i < STAGE_COUNT; ++i )
	{
		if ( nStops[i] > nDecompressedSize )
		{
			nStops[i] = nDecompressedSize;
		}
	}
	if ( nStops[1] < nStops[0] )
	{
		nStops[1] = nStops[0];
	}

	SParameters params[STAGE_COUNT];
	for ( uint32_t i = 0; i < STAGE_COUNT; ++i )
	{
		params[i] = ReadParameters( pCompressed + i * 12 );
	}

	CArithBits bits( pCompressed, nCompressedSize, HEADER_SIZE );

	uint32_t nWritten = 0;
	for ( uint32_t i = 0; i < STAGE_COUNT; ++i )
	{
		if ( nStops[i] <= nWritten )
		{
			// An empty stage, which the middle one always is in this game's data.
			continue;
		}
		CStage stage( params[i] );
		if ( !stage.Decode( bits, pDecompressed, nDecompressedSize, nStops[i], &nWritten ) )
		{
			return false;
		}
	}

	return nWritten == nDecompressedSize && !bits.Failed();
}

}
