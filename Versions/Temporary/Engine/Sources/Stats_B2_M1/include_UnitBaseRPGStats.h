int nUninstallRotate;
int nUninstallTransport;
int nAmmos[2];
int nPrimaryGun;
int nPrimaryPlatform;	// platform with primary gun
EUnitRPGType etype;
std::vector<NDb::EUnitSpecialAbility> abilities;

inline void AddValue( CUserCommands &array, int nBit ) { array.SetData( nBit ); }
inline void RemValue( CUserCommands &array, int nBit ) { array.RemoveData( nBit ); }
const NDb::SUnitSpecialAblityDesc * GetAbilityDescByCmd( const int nCmdType ) const;

void ToAIUnits( bool bInEditor );
int GetAnimTime( int nAnim ) const { return nAnim < 0 || nAnim >= animdescs.size() || animdescs[nAnim].anims.empty() ? 0 : animdescs[nAnim].anims[0].nLength; }
int GetAnimActionTime( int nAnim ) const 
{ 
	if ( nAnim < 0 || nAnim >= animdescs.size() || animdescs[nAnim].anims.empty() )
		return 0;
	else
	{
		NI_ASSERT( animdescs[nAnim].anims[0].nAction < animdescs[nAnim].anims[0].nLength, StrFmt("AnimPoint is incorrect (too large) for unit \"%s\" animation %d", GetDBID().ToString().c_str(), nAnim ) );
		if ( animdescs[nAnim].anims[0].nAction < animdescs[nAnim].anims[0].nLength )
			return animdescs[nAnim].anims[0].nAction;
		else
			return 0;
	}

}
// chooses sound for given ack etype and writes it to passed string
// returns false if no acknowledgement is chosen
virtual const NDb::SComplexSoundDesc * ChooseAcknowledgement( const EUnitAckType etype, const int nSet ) const
{
	if ( acksNames.size() <= nSet )
		return 0;
	else 
		return ::ChooseAcknowledgement( acksNames[nSet].GetPtr(), etype );
}
const EUnitRPGClass GetRPGClass() const { return NDb::GetRPGClass( etype ); }
NDb::EUnitRPGType GetMainetype() const { return NDb::GetMainType( etype ); }
int IsInfantry() const { return NDb::IsInfantry( etype ); }
int IsTransport() const { return NDb::IsTransport( etype ); }
int IsArtillery() const { return NDb::IsArtillery( etype ); }
int IsNaval() const { return 0; }
int IsSPG() const { return NDb::IsSPG( etype ); }
int IsArmor() const { return NDb::IsArmor( etype ); }
int IsAviation() const { return NDb::IsAviation( etype ); }
int IsTrain() const { return NDb::IsTrain( etype ); }

virtual int GetArmor( const int n ) const = 0;
virtual int GetMinPossibleArmor( const int n ) const = 0;
virtual int GetMaxPossibleArmor( const int n ) const = 0;
virtual int GetRandomArmor( const int n ) const = 0;
virtual const SBaseGunRPGStats& GetGun( const int nUniqueID, const int nPlatform, const int nGun ) const = 0;
virtual const int GetGunsSize( const int nUniqueID, const int nPlatform ) const = 0;
//
void CountPrimaryGuns( const int nUniqueID, const int nPlatform );
void CountShellTypes( const int nUniqueID, const int nPlatform );
//
const bool HasCommand( const int nCmd ) const;

virtual const float GetTurnRadius() const { return 0.0f; }
//
void GetUserActions( CUserActions *pActions ) const;
const bool HasUserAction( const int nAction ) const; 
//
const std::vector<SAnimDesc>* GetAnims( const int netype ) const { return netype < animdescs.size() ? &(animdescs[netype].anims) : 0; }
//
virtual const CUserActions* GetUserActions( bool bActionsBy ) const;
//
const SUnitActions* GetActions() const
{
	return pActions ? pActions : NDb::Get<NDb::SUnitActions>( CDBID("Other/UnitActions/Game/Units/dummy.xdb") );
}

