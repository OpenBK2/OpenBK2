#pragma once

// CWarFogVisibility
class CWarFogVisibility : public CObjectBase
{
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	CGlobalWarFog *pWarFog = {};
protected:
	const bool IsTileInside( const SVector &vTile ) const { return pWarFog->IsTileInside( vTile ); }
	const CArray1Bit &GetVisibleInfoForTile( const SVector &vTile ) const { return pWarFog->GetVisibleInfoForTile( vTile ); }
	const SSpiralPoint &GetSpiralPoint( const int nSpiralIndex ) const { return pWarFog->GetSpiralPoint( nSpiralIndex ); }
public:
	CWarFogVisibility() : pWarFog( 0 ) {}
	CWarFogVisibility( CGlobalWarFog *_pWarFog ) : pWarFog( _pWarFog ) {}

	virtual const bool IsVisible( const SVector &vTile, const int nSpiralIndex ) const = 0;
};

// CVisForPlane
class CVisForPlane : public CWarFogVisibility
{
	OBJECT_NOCOPY_METHODS( CVisForPlane )
public:
	CVisForPlane() : CWarFogVisibility() {}
	CVisForPlane( CGlobalWarFog *pWarFog ) : CWarFogVisibility( pWarFog ) {}

	const bool IsVisible( const SVector &vTile, const int nSpiralIndex ) const { return IsTileInside( vTile ); }
};

// CVisForGroundUnitBasis
class CVisForGroundUnitBasis : public CWarFogVisibility
{
	// Default construction has no visibility data; never retain a reference to a temporary array.
	const CArray1Bit *pVisibleInfo = nullptr;
public:
	CVisForGroundUnitBasis() : CWarFogVisibility() {}
	CVisForGroundUnitBasis( CGlobalWarFog *pWarFog, const SVector &vTile ) : CWarFogVisibility( pWarFog ), pVisibleInfo( &GetVisibleInfoForTile( vTile ) ) {}

	virtual const bool IsVisible( const SVector &vTile, const int nSpiralIndex ) const { return pVisibleInfo && pVisibleInfo->GetData( nSpiralIndex ) > 0; }
};

// CVisForGroundUnit
class CVisForGroundUnit : public CVisForGroundUnitBasis
{
	OBJECT_NOCOPY_METHODS( CVisForGroundUnit )
public:
	CVisForGroundUnit() : CVisForGroundUnitBasis() {}
	CVisForGroundUnit( CGlobalWarFog *pWarFog, const SVector &vTile ) : CVisForGroundUnitBasis( pWarFog, vTile ) {}
};

// CVisForGroundUnitWithSector
class CVisForGroundUnitWithSector : public CVisForGroundUnitBasis
{
	OBJECT_NOCOPY_METHODS( CVisForGroundUnitWithSector )
		SSector sector;
public:
	CVisForGroundUnitWithSector() : CVisForGroundUnitBasis(), sector() {}
	CVisForGroundUnitWithSector( CGlobalWarFog *pWarFog, const SVector &vTile, const SSector &_sector ) :	CVisForGroundUnitBasis( pWarFog, vTile ), sector( _sector ) {}

	const bool IsVisible( const SVector &vTile, const int nSpiralIndex ) const
	{
		if ( GetSpiralPoint( nSpiralIndex ).sector.IsIntersec( sector ) )
			return CVisForGroundUnitBasis::IsVisible( vTile, nSpiralIndex );
		return false;
	}
};

CWarFogVisibility* CreateWarFogVisibility( CGlobalWarFog *pWarFog, const SWarFogUnitInfo &unitInfo )
{
	if ( unitInfo.bPlane )
		return new CVisForPlane( pWarFog );
	else if ( unitInfo.sector.IsWholeCircle() )
		return new CVisForGroundUnit( pWarFog, unitInfo.vPos );
	else
		return new CVisForGroundUnitWithSector( pWarFog, unitInfo.vPos, unitInfo.sector );
}



