virtual void ToAIUnits( bool bInEditor )
{
	eGameType = SGVOGT_TERRAOBJ;
	SStaticObjectRPGStats::ToAIUnits( bInEditor );

	for ( int i = 0; i < segments.size(); ++i )
		segments[i].ToAIUnits( bInEditor );
}
//
virtual const CVec2& GetOrigin( const int nIndex = -1 ) const { return segments[nIndex].vOrigin; }
virtual const SByteArray2& GetPassability( const int nIndex = -1 ) const { return segments[nIndex].passability; }

virtual const CVec2& GetVisOrigin( const int nIndex = -1 ) const { return segments[nIndex].vVisOrigin; }
virtual const SByteArray2& GetVisibility( const int nIndex = -1 ) const 
{ 
	return segments[nIndex].visibility; 
}
//

virtual const SPassProfile& GetPassProfile( const int nIndex = -1 ) const
{
	static SPassProfile emptyProfile;
	return emptyProfile;
}

virtual const SPassProfile& GetVisProfile( const int nIndex = -1 ) const
{
	static SPassProfile emptyProfile;
	return emptyProfile;
}

