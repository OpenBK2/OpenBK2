enum
{
	ENTRENCHMENT_LINE				= 0x00000001,
	ENTRENCHMENT_FIREPLACE	= 0x00000002,
	ENTRENCHMENT_TERMINATOR	= 0x00000004,
	ENTRENCHMENT_ARC				= 0x00000008
};

int GetIndexLocal( int nIndex, const std::vector<int> &indices, const char *pszName, int *pCurRandomSeed = 0 ) const
{
	//NI_VERIFY( !indices.empty() && (nIndex < indices.size()), StrFmt("Can't find any \"%s\" segment for entrenchment", pszName), return -1 );
	if ( nIndex == -1 )
	{
		if ( pCurRandomSeed == 0 )
			return indices[ rand() % indices.size() ];
		else
		{
			*pCurRandomSeed %= indices.size();
			return indices[*pCurRandomSeed];
		}
	}
	else
		return indices[nIndex];
}
//
int GetLineIndex( int nIndex = -1 ) const { return GetIndexLocal( nIndex, lines, "line", 0 ); }
int GetFirePlaceIndex( int nIndex = -1 ) const { return GetIndexLocal( nIndex, fireplaces, "fireplace", 0 ); }
int GetTerminatorIndex( int nIndex = -1 ) const { return GetIndexLocal( nIndex, terminators, "terminator", 0 ); }
int GetArcIndex( int nIndex = -1 ) const { return GetIndexLocal( nIndex, arcs, "arc", 0 ); }
//
int GetLineIndex( int *pCurRandomSeed ) const { return GetIndexLocal( -1, lines, "line", pCurRandomSeed ); }
int GetFirePlaceIndex( int *pCurRandomSeed ) const { return GetIndexLocal( -1, fireplaces, "fireplace", pCurRandomSeed ); }
int GetTerminatorIndex( int *pCurRandomSeed ) const { return GetIndexLocal( -1, terminators, "terminator", pCurRandomSeed ); }
int GetArcIndex( int *pCurRandomSeed ) const { return GetIndexLocal( -1, arcs, "arc", pCurRandomSeed ); }
//
const SEntrenchSegmentRPGStats& GetSegmentStats( const int nIndex ) const
{
	NI_ASSERT( nIndex >= 0 && nIndex < segments.size(), "Invalid segment index" );
	return segments[nIndex];
}
//
virtual void ToAIUnits( bool bInEditor )
{
	eGameType = SGVOGT_ENTRENCHMENT;
	SHPObjectRPGStats::ToAIUnits( bInEditor );
	FOR_EACH_VAL( segments, ToAIUnits, bInEditor ); 
}
//

virtual const int GetTypeFromIndex( const int nIndex ) const
{
	if ( find(lines.begin(), lines.end(), nIndex) != lines.end() )
		return ENTRENCHMENT_LINE;
	if ( find(fireplaces.begin(), fireplaces.end(), nIndex) != fireplaces.end() )
		return ENTRENCHMENT_FIREPLACE;
	if ( find(terminators.begin(), terminators.end(), nIndex) != terminators.end() )
		return ENTRENCHMENT_TERMINATOR;
	if ( find(arcs.begin(), arcs.end(), nIndex) != arcs.end() )
		return ENTRENCHMENT_ARC;
	NI_ASSERT( false, StrFmt("Wrong index %d in entrenchment \"%s\"", nIndex, szParentName.c_str()) );
	return -1;
}

virtual const int GetIndexFromType( const int nType, int *pCurRandomSeed ) const
{
	switch ( nType ) 
	{
	case ENTRENCHMENT_LINE:
		return GetLineIndex( pCurRandomSeed );
	case ENTRENCHMENT_FIREPLACE:
		return GetFirePlaceIndex( pCurRandomSeed );
	case ENTRENCHMENT_TERMINATOR:
		return GetTerminatorIndex( pCurRandomSeed );
	case ENTRENCHMENT_ARC:
		return GetArcIndex( pCurRandomSeed );
	}
	NI_ASSERT( false, StrFmt("Wrong type %d in entrenchment \"%s\"", nType, szParentName.c_str()) );
	return -1;
}
// 
