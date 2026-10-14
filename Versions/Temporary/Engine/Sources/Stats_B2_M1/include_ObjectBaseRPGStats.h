void ToAIUnits( bool bInEditor ) override
{
	SStaticObjectRPGStats::ToAIUnits( bInEditor );
	passability.PostLoad( bInEditor );
	visibility.PostLoad( bInEditor );
}

//
const CVec2 &GetOrigin( const int nIndex = -1 ) const override
{
	return vOrigin;
}

const SByteArray2 &GetPassability( const int nIndex = -1 ) const override
{
	return passability;
}

const CVec2 &GetVisOrigin( const int nIndex = -1 ) const override
{
	if ( bUsePassabilityForVisibility )
	{
		return vOrigin;
	}
	else
	{
		return vVisOrigin;
	}
}

const SByteArray2 &GetVisibility( const int nIndex = -1 ) const override
{
	if ( bUsePassabilityForVisibility )
	{
		return passability;
	}
	else
	{
		return visibility;
	}
}

const SPassProfile &GetPassProfile( const int nIndex = -1 ) const override
{
	return passProfile;
}

const SPassProfile &GetVisProfile( const int nIndex = -1 ) const override
{
	if ( bUsePassabilityForVisibility )
	{
		return passProfile;
	}
	else
	{
		static SPassProfile emptyProfile;
		return emptyProfile;
	}
}

virtual const NDb::SComplexSoundDesc *GetAmbientSoundDesc( const NDb::ESeason season,
                                                           const NDb::EDayNight daytime ) const
{
	const int nDayTimeMask = 0x01 << (int)daytime;
	for ( std::vector<SAmbientSound>::const_iterator it = ambientSounds.begin(); it != ambientSounds.end(); ++it )
	{
		if ( it->eSeason == season && ( it->nDayTime & nDayTimeMask ) == nDayTimeMask )
		{
			return it->pSoundDesc;
		}
	}
	return pAmbientSound;
}
