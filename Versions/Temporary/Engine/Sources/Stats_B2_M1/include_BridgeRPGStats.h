/*enum
{
	BRIDGE_SPAN_TYPE_BEGIN	= 0x00000001,
	BRIDGE_SPAN_TYPE_CENTER	= 0x00000002,
	BRIDGE_SPAN_TYPE_END		= 0x00000004,
};

int GetIndexLocal( int nIndex, const vector<int> &indices, const char *pszName, int *pRandomSeed = 0 ) const
{
	//NI_VERIFY( !indices.empty() && (nIndex < indices.size()), StrFmt("Can't find any \"%s\" segment for bridge", pszName), return -1 );
	if ( nIndex == -1 )
	{
		if ( pRandomSeed == 0 )
			return indices[ rand() % indices.size() ];
		else
		{
			*pRandomSeed %= indices.size();
			return indices[*pRandomSeed];
		}
	}
	else
		return indices[nIndex];
}
//
int GetRandomBeginIndex( int nIndex = -1, int nState = 0 )	const { return GetIndexLocal( nIndex, states[nState].begins, "begin", 0 ); }
int GetRandomLineIndex( int nIndex = -1, int nState = 0 )		const { return GetIndexLocal( nIndex, states[nState].lines, "line", 0 ); }
int GetRandomEndIndex( int nIndex = -1, int nState = 0 )		const { return GetIndexLocal( nIndex, states[nState].ends, "end", 0 ); }
//
int GetRandomBeginIndex( int nIndex, int nState, int *pRandomSeed )	const { return GetIndexLocal( nIndex, states[nState].begins, "begin", pRandomSeed ); }
int GetRandomLineIndex( int nIndex, int nState, int *pRandomSeed )  const { return GetIndexLocal( nIndex, states[nState].lines, "line", pRandomSeed ); }
int GetRandomEndIndex( int nIndex, int nState, int *pRandomSeed )   const { return GetIndexLocal( nIndex, states[nState].ends, "end", pRandomSeed ); }
//
bool IsBeginIndex( int nIndex ) const 
{ 
	return	( find( states[0].begins.begin(), states[0].begins.end(), nIndex ) != states[0].begins.end() ) ||
		( find( states[1].begins.begin(), states[1].begins.end(), nIndex ) != states[1].begins.end() ) ||
		( find( states[2].begins.begin(), states[2].begins.end(), nIndex ) != states[2].begins.end() );
}
bool IsEndIndex( int nIndex ) const 
{ 
	return	( find( states[0].ends.begin(), states[0].ends.end(), nIndex ) != states[0].ends.end() ) ||
		( find( states[1].ends.begin(), states[1].ends.end(), nIndex ) != states[1].ends.end() ) ||
		( find( states[2].ends.begin(), states[2].ends.end(), nIndex ) != states[2].ends.end() );
}
bool IsLineIndex( int nIndex ) const 
{ 
	return	( find( states[0].lines.begin(), states[0].lines.end(), nIndex ) != states[0].lines.end() ) ||
		( find( states[1].lines.begin(), states[1].lines.end(), nIndex ) != states[1].lines.end() ) ||
		( find( states[2].lines.begin(), states[2].lines.end(), nIndex ) != states[2].lines.end() );
}
//
const SSegmentRPGStats& GetSegmentStats( const int nIndex ) const
{
	NI_ASSERT( nIndex >= 0 && nIndex < segments.size(), StrFmt("Index %d for the segments of the \"%s\"must be in the range [0..%d]", nIndex, szKeyName.c_str(), segments.size()) );
	return segments[nIndex];
}
//
const SSpan& GetSpanStats( const int nIndex, const int nState = 0 ) const
{
	NI_ASSERT( nIndex >= 0 && nIndex < states[nState].spans.size(), StrFmt("Index %d for the spans of the \"%s\"must be in the range [0..%d]", nIndex, szKeyName.c_str(), states[nState].spans.size()) );
	return states[nState].spans[nIndex];
}
// в следующих четырёх функциях 'nIndex' обозначает не 'segment', а 'span', из которого надо выдернуть 'nSlab' segment и вернуть его данные*/
const SElementRPGStats& GetElementStats( const int nIndex ) const
{
	if ( nIndex == -1 || nIndex == 1 )
		return center;
	return end;
}
virtual const CVec2& GetOrigin( const int nIndex = -1 ) const
{
	return GetElementStats( nIndex ).vOrigin;
}
virtual const SByteArray2& GetPassability( const int nIndex = -1 ) const
{
	return GetElementStats( nIndex ).passability;
}
virtual const CVec2& GetVisOrigin( const int nIndex = -1 ) const
{
	return GetOrigin( nIndex );
}
virtual const SByteArray2& GetVisibility( const int nIndex = -1 ) const
{
	static SByteArray2 ar;
	return ar;
//	return GetElementStats( nIndex ).passability;
}
virtual const SPassProfile& GetPassProfile( const int nIndex = -1 ) const
{
	static SPassProfile emptyProfile;
	return emptyProfile;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
virtual const SPassProfile& GetVisProfile( const int nIndex = -1 ) const
{
	static SPassProfile emptyProfile;
	return emptyProfile;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
virtual void ToAIUnits( bool bInEditor )
{
	eGameType = SGVOGT_BRIDGE;
	SStaticObjectRPGStats::ToAIUnits( bInEditor );
	//FOR_EACH_VAL( segments, ToAIUnits, bInEditor ); 
	FOR_EACH_VAL( firePoints, ToAIUnits, bInEditor ); 
	FOR_EACH_VAL( smokePoints, ToAIUnits, bInEditor ); 
	FOR_EACH_VAL( dirExplosions, ToAIUnits, bInEditor ); 
	//
	center.passability.PostLoad( bInEditor );
	end.passability.PostLoad( bInEditor );
}//
/*
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int GetTypeFromIndex( const int nIndex, const int nDamageState ) const
{
	if ( find(states[nDamageState].begins.begin(), states[nDamageState].begins.end(), nIndex) != states[nDamageState].begins.end() ) 
		return BRIDGE_SPAN_TYPE_BEGIN;
	if ( find(states[nDamageState].lines.begin(), states[nDamageState].lines.end(), nIndex) != states[nDamageState].lines.end() ) 
		return BRIDGE_SPAN_TYPE_CENTER;
	if ( find(states[nDamageState].ends.begin(), states[nDamageState].ends.end(), nIndex) != states[nDamageState].ends.end() ) 
		return BRIDGE_SPAN_TYPE_END;
	//
	NI_ASSERT( false, StrFmt("Unknown span index %d in bridge \"%s\"", nIndex, szParentName.c_str()) );
	return -1;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int GetIndexFromTypeLocal( const int nType, const int nDamageState, int *pCurRandomSeed ) const
{
	switch ( nType ) 
	{
	case BRIDGE_SPAN_TYPE_BEGIN:
		return GetRandomBeginIndex( -1, nDamageState, pCurRandomSeed );
	case BRIDGE_SPAN_TYPE_CENTER:
		return GetRandomLineIndex( -1, nDamageState, pCurRandomSeed );
	case BRIDGE_SPAN_TYPE_END:
		return GetRandomEndIndex( -1, nDamageState, pCurRandomSeed );
	}
	NI_ASSERT( false, StrFmt("Unknown span type %d of damage state %d for the bridge \"%s\"", nType, nDamageState, szParentName.c_str()) );
	return -1;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
virtual const int GetIndexFromType( const int nType, const int nDamageState = 0 ) const
{ return GetIndexFromTypeLocal( nType, nDamageState, 0 ); }
virtual const int GetIndexFromType( const int nType, int *pCurRandomSeed ) const
{ return GetIndexFromTypeLocal( nType, 0, pCurRandomSeed ); }*/