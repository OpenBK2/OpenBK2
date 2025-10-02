virtual const CVec2& GetOrigin( const int nIndex = -1 ) const = 0;
virtual const SByteArray2& GetPassability( const int nIndex = -1 ) const = 0;
virtual const CVec2& GetVisOrigin( const int nIndex = -1 ) const = 0;
virtual const SByteArray2& GetVisibility( const int nIndex = -1 ) const = 0;
virtual const SPassProfile& GetPassProfile( const int nIndex = -1 ) const = 0;
virtual const SPassProfile& GetVisProfile( const int nIndex = -1 ) const = 0;
//
virtual void ToAIUnits( bool bInEditor )
{
	SHPObjectRPGStats::ToAIUnits( bInEditor );
}
