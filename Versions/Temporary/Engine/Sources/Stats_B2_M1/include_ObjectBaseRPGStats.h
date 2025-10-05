virtual void ToAIUnits( bool bInEditor )
{
	SStaticObjectRPGStats::ToAIUnits( bInEditor );
	passability.PostLoad( bInEditor );
	visibility.PostLoad( bInEditor );
}
//
virtual const CVec2& GetOrigin( const int nIndex = -1 ) const { return vOrigin; }
virtual const SByteArray2& GetPassability( const int nIndex = -1 ) const { return passability; }

virtual const CVec2& GetVisOrigin( const int nIndex = -1 ) const 
{ 
	if ( bUsePassabilityForVisibility )
		return vOrigin;
	else
		return vVisOrigin; 
}
virtual const SByteArray2& GetVisibility( const int nIndex = -1 ) const 
{ 
	if ( bUsePassabilityForVisibility )
		return passability;
	else
		return visibility; 
}

virtual const SPassProfile& GetPassProfile( const int nIndex = -1 ) const
{
	return passProfile;
}

virtual const SPassProfile& GetVisProfile( const int nIndex = -1 ) const
{
	if ( bUsePassabilityForVisibility )
		return passProfile;
	else
	{
		static SPassProfile emptyProfile;
		return emptyProfile;
	}
}

virtual const NDb::SComplexSoundDesc* GetAmbientSoundDesc( const NDb::ESeason season, const NDb::EDayNight daytime ) const
{
	const int nDayTimeMask = 0x01 << (int)daytime;
	for ( std::vector<SAmbientSound>::const_iterator it = ambientSounds.begin(); it != ambientSounds.end(); ++it )
	{
		if ( it->eSeason == season && ( it->nDayTime & nDayTimeMask ) == nDayTimeMask )
			return it->pSoundDesc;
	}
	return pAmbientSound;
}


