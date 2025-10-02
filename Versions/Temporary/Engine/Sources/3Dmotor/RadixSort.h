#pragma once

enum ERadixFetch
{
	RF_NORMAL,
	RF_BACK,
	RF_FLOAT
};
inline void RadixSortStage( int shift, const int *pDepths, vector<int> *pRes, ERadixFetch fetch )
{
	int lists[256];
	memset( lists, -1, sizeof(lists) );
	int nCount = pRes->size();
	vector<int> temp( nCount );
	for ( int k = 0; k < nCount; ++k )
	{
		int nIdx = (*pRes)[k];
		int n = ( pDepths[ nIdx ] >> shift ) & 0xff; // determine pile to which this element is to be stored
		temp[nIdx] = lists[n];
		lists[n] = nIdx;
	}
	int *pOut = &(*pRes)[0], *pFinal = pOut + nCount - 1;
	switch ( fetch )
	{
	case RF_NORMAL:
		for ( int i = 0; i < 256; i++ )
		{
			for ( int nIdx = lists[i]; nIdx >= 0; nIdx = temp[nIdx] )
				*pOut++ = nIdx;
		}
		ASSERT( pOut == pFinal + 1 );
		break;
	case RF_BACK:
		for ( int i = 255; i >= 0; i-- )
		{
			for ( int nIdx = lists[i]; nIdx >= 0; nIdx = temp[nIdx] )
				*pOut++ = nIdx;
		}
		ASSERT( pOut == pFinal + 1 );
		break;
	case RF_FLOAT:
		for ( int i = 255; i >= 128; i-- )
		{
			for ( int nIdx = lists[i]; nIdx >= 0; nIdx = temp[nIdx] )
				*pOut++ = nIdx;
		}
		for ( int i = 127; i >= 0; i-- )
		{
			for ( int nIdx = lists[i]; nIdx >= 0; nIdx = temp[nIdx] )
				*pFinal-- = nIdx;
		}
		break;
	}
}
// lowest first
inline void DoRadixSort( const float *pSrc, int nSize, vector<int> *pRes )
{
	pRes->resize( nSize );
	if ( nSize <= 0 )
		return;
	for ( int k = 0; k < pRes->size(); ++k )
		(*pRes)[k] = k;
	//RadixSortStage( 0, pSort, pSort1, nElem, 1 );
	RadixSortStage( 8, (const int*)pSrc, pRes, RF_BACK );
	RadixSortStage( 16, (const int*)pSrc, pRes, RF_NORMAL );
	RadixSortStage( 24, (const int*)pSrc, pRes, RF_FLOAT );
}

inline void DoRadixSort( const unsigned int *pSrc, int nSize, vector<int> *pRes )
{
	pRes->resize( nSize );
	if ( nSize <= 0 )
		return;
	for ( int k = 0; k < pRes->size(); ++k )
		(*pRes)[k] = k;
	RadixSortStage( 0, (const int*)pSrc, pRes, RF_BACK );
	RadixSortStage( 8, (const int*)pSrc, pRes, RF_NORMAL );
	RadixSortStage( 16, (const int*)pSrc, pRes, RF_BACK );
	RadixSortStage( 24, (const int*)pSrc, pRes, RF_NORMAL );
}

