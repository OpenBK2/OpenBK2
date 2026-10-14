void ToAIUnits( bool bInEditor ) override
{
	eGameType = SGVOGT_TERRAOBJ;
	SStaticObjectRPGStats::ToAIUnits( bInEditor );

	for ( int i = 0; i < segments.size(); ++i )
	{
		segments[i].ToAIUnits( bInEditor );
	}
}

//
const CVec2 &GetOrigin( const int nIndex = -1 ) const override
{
	return segments[nIndex].vOrigin;
}

const SByteArray2 &GetPassability( const int nIndex = -1 ) const override
{
	return segments[nIndex].passability;
}

const CVec2 &GetVisOrigin( const int nIndex = -1 ) const override
{
	return segments[nIndex].vVisOrigin;
}

const SByteArray2 &GetVisibility( const int nIndex = -1 ) const override
{
	return segments[nIndex].visibility;
}

//

const SPassProfile &GetPassProfile( const int nIndex = -1 ) const override
{
	static SPassProfile emptyProfile;
	return emptyProfile;
}

const SPassProfile &GetVisProfile( const int nIndex = -1 ) const override
{
	static SPassProfile emptyProfile;
	return emptyProfile;
}
