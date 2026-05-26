enum
{
	FENCE_DIRECTION_0		= 0x00000001,
	FENCE_DIRECTION_1		= 0x00000002,
	FENCE_DIRECTION_2		= 0x00000004,
	FENCE_DIRECTION_3		= 0x00000008,
	FENCE_TYPE_NORMAL		= 0x00010000,
	FENCE_TYPE_LDAMAGE	= 0x00020000,
	FENCE_TYPE_RDAMAGE	= 0x00040000,
	FENCE_TYPE_CDAMAGE	= 0x00080000,
};

// 
virtual void ToAIUnits( bool bInEditor )
{
	eGameType = SGVOGT_FENCE;
	SStaticObjectRPGStats::ToAIUnits( bInEditor );
	FOR_EACH_VAL( stats, ToAIUnits, bInEditor );
	//
	centerSegments.passability.PostLoad( bInEditor );
	damagedSegments.passability.PostLoad( bInEditor );
	damagedSegmentsOtherSide.passability.PostLoad( bInEditor );
	destroyedSegments.passability.PostLoad( bInEditor );
}


// general fences rule:
// frame index counting - 
// 1) center segments vis objects
// 2) damage segments vis objects
// 3) destroyed segments vis objects
// 


//
const SSegments& GetSegmentsByFrameIndex( const int nFrameIndex, int *nVisObj ) const
{
	int & nCur = *nVisObj;
	nCur = nFrameIndex;
	NI_ASSERT( nCur >= 0, "wrong frame index for fence" );

	if ( nCur < centerSegments.visObjes.size() )
		return centerSegments;
	nCur -= centerSegments.visObjes.size();
	
	if ( nCur < damagedSegmentsOtherSide.visObjes.size() )
		return damagedSegmentsOtherSide;
	nCur -= damagedSegmentsOtherSide.visObjes.size();

	if ( nCur < damagedSegments.visObjes.size() )
		return damagedSegments;
	nCur -= damagedSegments.visObjes.size();
	
	if ( nCur < destroyedSegments.visObjes.size() )
		return destroyedSegments;
	NI_ASSERT( false, StrFmt( "wrong frame index for fence, %i", nFrameIndex ) );
	return centerSegments;
}

const NDb::SVisObj * GetVisObjByFrameIndex( const int nFrameIndex ) const
{
	int nCur;
	const SSegments & seg = GetSegmentsByFrameIndex( nFrameIndex, &nCur );
	// check is already done, no need to check
	if ( nCur < seg.visObjes.size() )
		return seg.visObjes[nCur];

	return 0;
}

virtual const CVec2& GetOrigin( const int nIndex = -1 ) const
{
	int nCur; 
	return GetSegmentsByFrameIndex( nIndex, &nCur ).vOrigin;
	//NI_ASSERT( nIndex > -1 && nIndex < stats.size(), StrFmt("Index %d for the \"%s\"must be in the range [0..%d]", nIndex, szKeyName.c_str(), stats.size()) );
	//return stats[nIndex].vOrigin;
}

virtual const SByteArray2& GetPassability( const int nIndex = -1 ) const
{
	int nCur; 
	return GetSegmentsByFrameIndex( nIndex, &nCur ).passability;
	//NI_ASSERT( nIndex > -1 && nIndex < stats.size(), StrFmt("Index %d for the \"%s\"must be in the range [0..%d]", nIndex, szKeyName.c_str(), stats.size()) );
	//return stats[nIndex].passability;
}

virtual const CVec2& GetVisOrigin( const int nIndex = -1 ) const
{
	int nCur; 
	return GetSegmentsByFrameIndex( nIndex, &nCur ).vOrigin;
	//NI_ASSERT( nIndex > -1 && nIndex < stats.size(), StrFmt("Index %d for the \"%s\"must be in the range [0..%d]", nIndex, szKeyName.c_str(), stats.size()) );
	//return stats[nIndex].vVisOrigin;
}

virtual const SByteArray2& GetVisibility( const int nIndex = -1 ) const
{
	static SByteArray2 empty;
	int nCur; 
	const SSegments &segm = GetSegmentsByFrameIndex( nIndex, &nCur );
	if ( segm.bUsePassabilityForVisibility )
		return segm.passability;
	return empty;
}

virtual const SPassProfile& GetPassProfile( const int nIndex = -1 ) const
{
	int nCur; 
	return GetSegmentsByFrameIndex( nIndex, &nCur ).passProfile;
}

virtual const SPassProfile& GetVisProfile( const int nIndex = -1 ) const
{
	static SPassProfile emptyProfile;
	return emptyProfile;
}

enum ETypesOfLife 
{ 
	ETOL_SAFE, 
	ETOL_LEFT, 
	ETOL_RIGHT,
	ETOL_DESTROYED 
};

bool HasLeftDamaged() const
{
	return !damagedSegmentsOtherSide.visObjes.empty();
}

int GetRandomFrameIndexByDamageType( const ETypesOfLife eType ) const
{
	switch( eType )
	{
	case ETOL_SAFE:
		{ RecordRandomCall(); return NRandom::Random( 0, centerSegments.visObjes.size() -1); }
	case ETOL_LEFT:
		if ( HasLeftDamaged() )
			{ RecordRandomCall(); return centerSegments.visObjes.size() + NRandom::Random( 0, damagedSegmentsOtherSide.visObjes.size() -1 ); }
	case ETOL_RIGHT:
		{
			RecordRandomCall();
			return centerSegments.visObjes.size() +
						damagedSegmentsOtherSide.visObjes.size() + 
						NRandom::Random( 0, damagedSegments.visObjes.size() -1);
		}
	case ETOL_DESTROYED:
		{
			NI_VERIFY( !destroyedSegments.visObjes.empty(), StrFmt( "can't find destroyed segment for SFenceRPGStats \"%s\"", GetDBID().ToString().c_str() ), return -1 );
			RecordRandomCall();
			return
				centerSegments.visObjes.size() + damagedSegmentsOtherSide.visObjes.size() +
				damagedSegments.visObjes.size() + NRandom::Random( 0, destroyedSegments.visObjes.size() -1 );
		}
	}
	NI_ASSERT( false, "unknown fence life type" );
	return -1;
}

ETypesOfLife GetDamageTypeByFrameIndex( const int _nFrameIndex ) const
{
	const int nMaxIndexSafe = centerSegments.visObjes.size();
	const int nMaxIndexLeft = nMaxIndexSafe + damagedSegmentsOtherSide.visObjes.size();
	const int nMaxIndexRight = nMaxIndexLeft + damagedSegments.visObjes.size();
	const int nMaxIndexDestroyed = nMaxIndexRight + destroyedSegments.visObjes.size();
	if ( _nFrameIndex >= 0 && _nFrameIndex < nMaxIndexSafe )
		return ETOL_SAFE;
	else if ( _nFrameIndex >= nMaxIndexSafe && _nFrameIndex < nMaxIndexLeft )
	{
		NI_VERIFY( HasLeftDamaged(), StrFmt( "[MAP DESIGNER ERROR] Invalid frame index (%d). Object \"%d\" has no left damage.", GetDBID().ToString().c_str() ), return ETOL_DESTROYED );
		return ETOL_LEFT;
	}
	else if ( _nFrameIndex >= nMaxIndexLeft && _nFrameIndex < nMaxIndexRight )
		return ETOL_RIGHT;
	else if ( _nFrameIndex >= nMaxIndexRight && _nFrameIndex < nMaxIndexDestroyed )
		return ETOL_DESTROYED;
	NI_ASSERT( false, StrFmt( "[MAP DESIGNER ERROR] Invalid frame index (%d). Object \"%d\" has no appropriate lite type.", GetDBID().ToString().c_str() ) );
	return ETOL_SAFE;
}


