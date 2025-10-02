#include "StdAfx.h"
#include "RectPacker.h"

namespace NRectPacker
{

struct SRectOrder
{
	const vector<SRect> &s;
	SRectOrder( const vector<SRect> &_s ) : s(_s) {}
	bool operator()( int a, int b )
	{
		return s[a].nYSize > s[b].nYSize || ( s[a].nYSize == s[b].nYSize && s[a].nXSize > s[b].nXSize );
	}
};

struct SStripe
{
	int nXShift, nYShift, nHeight;
	vector<int> rects;
};

struct SStripeBuilder
{
	vector<SRect> &res;
	vector<int> indices;
	vector<char> taken;
	vector<SStripe> stripes;
	int nWidth;
	int nTotalHeight;

	SStripeBuilder( vector<SRect> &_res )
		: res(_res), indices(_res.size())
	{
		for ( int k = 0; k < indices.size(); ++k )
			indices[k] = k;
		sort( indices.begin(), indices.end(), SRectOrder( res ) );
	}
	void FillStripe( SStripe *pRes, int nStartFrom )
	{
		ASSERT( pRes->rects.size() == 1 );
		int nTail = pRes->nXShift + res[ pRes->rects[0] ].nXSize;
		int nHeight = pRes->nHeight;
		for ( int i = nStartFrom; i < taken.size(); ++i )
		{
			if ( taken[i] )
				continue;
			const SRect &r = res[ indices[i] ];
			ASSERT( r.nYSize <= nHeight );
			if ( r.nYSize * 2 <= nHeight )
				break;
			if ( nTail + r.nXSize <= nWidth )
			{
				pRes->rects.push_back( indices[i] );
				taken[i] = 1;
				nTail += r.nXSize;
				if ( nTail == nWidth )
					return;
			}
		}
		// split height in 2 and try to fill the gap
		if ( nHeight == 1 )
			return;
		int nOriginalHeight = nHeight, nYShift = 0, nYStart = pRes->nYShift;
		for(;;)
		{
			nHeight /= 2;
			if ( nHeight == 0 )
				return;
			while ( BuildStripe( nTail, nYStart + nYShift, nHeight, nStartFrom ) )
			{
				nYShift += nHeight;
				if ( nYShift + nHeight > nOriginalHeight )
					return;
			}
		}
	}
	bool BuildStripe( int nXShift, int nYShift, int nHeight, int nStartFrom )
	{
		ASSERT( nYShift >= 0 );
		for ( int i = nStartFrom; i < taken.size(); ++i )
		{
			if ( taken[i] )
				continue;
			const SRect &r = res[ indices[i] ];
			if ( r.nYSize > nHeight )
				continue;
			if ( r.nYSize * 2 <= nHeight )
				break;
			ASSERT( r.nXSize <= nWidth );
			if ( nXShift + r.nXSize > nWidth )
				continue;
			SStripe &stripe = *stripes.insert( stripes.end() );
			stripe.nXShift = nXShift;
			stripe.nYShift = nYShift;
			stripe.nHeight = r.nYSize;
			stripe.rects.push_back( indices[i] );
			taken[i] = 1;
			FillStripe( &stripe, i + 1 );
			return true;
		}
		return false;
	}
	void Pack( int _nWidth )
	{
		nWidth = _nWidth;
		taken.resize(0);
		taken.resize( indices.size(), 0 );
		int nYShift = 0;
		for ( int i = taken.size() - 1; i >= 0; --i )
		{
			SRect &r = res[ indices[i] ];
			int nHeight = r.nYSize;
			if ( nHeight == 0 )
			{
				taken[i] = 1;
				r.nXShift = r.nYShift = 0;
			}
			else
				break;
		}
		for( int i = 0; i < taken.size(); ++i )
		{
			if ( taken[i] )
				continue;
			const SRect &r = res[ indices[i] ];
			int nHeight = r.nYSize;
			bool bTest = BuildStripe( 0, nYShift, nHeight, i );
			ASSERT( bTest );
			nYShift += nHeight;
		}
		nTotalHeight = nYShift;
		if ( nTotalHeight > nWidth )
		{
			// bad luck, use wider buffer
			stripes.clear();
			Pack( _nWidth * 2 );
		}
		else
		{
			// put rects on allocated places
			for ( int k = 0; k < stripes.size(); ++k )
			{
				const SStripe &s = stripes[k];
				int nXShift = s.nXShift;
				for ( int i = 0; i < s.rects.size(); ++i )
				{
					SRect &r = res[ s.rects[i] ];
					r.nXShift = nXShift;
					r.nYShift = s.nYShift;
					ASSERT( r.nYSize <= s.nHeight );
					ASSERT( r.nYSize * 2 >= s.nHeight );
					r.nYSize = s.nHeight;
					nXShift += r.nXSize;
					ASSERT( r.nXShift >= 0 && r.nXShift + r.nXSize <= nWidth );
					ASSERT( r.nYShift >= 0 && r.nYShift + r.nYSize <= nTotalHeight );
				}
			}
		}
	}
};

void PackRects( vector<SRect> *pRes, CTPoint<int> *pSize )
{
	/*for ( int k = 0; k < pRes->size(); ++k )
	{
		SRect &r = (*pRes)[k];
		r.nXShift = r.nYShift = 0;
		//r.nXSize = r.nYSize = 8;
	}
	pSize->x = 32;
	pSize->y = 32;
	return;*/
	int nArea = 0, nMaxWidth = 0;
	for ( int k = 0; k < pRes->size(); ++k )
	{
		const SRect &r = (*pRes)[k];
		nMaxWidth = Max( nMaxWidth, (int)r.nXSize );
		nArea += r.nXSize * r.nYSize;
	}
	int nXWidthLog2 = 0;
	while ( ( 1 << ( 2 * nXWidthLog2 ) ) < nArea )
		++nXWidthLog2;
	while ( ( 1 << nXWidthLog2 ) < nMaxWidth )
		++nXWidthLog2;
	int nWidth = 1 << nXWidthLog2;
	SStripeBuilder sb( *pRes );
	sb.Pack( nWidth );
	pSize->x = sb.nWidth;
	pSize->y = sb.nTotalHeight;
}

}
