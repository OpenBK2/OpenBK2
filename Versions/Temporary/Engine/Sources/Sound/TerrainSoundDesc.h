#pragma once

struct SSoundTerrainInfo
{
	//for sorting
	class CPrSoundsMassSort
	{
	public: bool operator()( const SSoundTerrainInfo &s1, const SSoundTerrainInfo &s2 ) const
					{ return s1.fWeight > s2.fWeight; }
	};

	class CPrTerrainTypeSort
	{
	public: bool operator()( const SSoundTerrainInfo &s1, const SSoundTerrainInfo &s2 ) const
					{ return s1.nTerrainType < s2.nTerrainType; }
	};
	// for finding objects with zero mass
	class CPrZeroMass
	{
	public: bool operator()( const SSoundTerrainInfo &s1 ) const
					{ return s1.fWeight == 0.0f ; }
	};

	CVec3 vPos;														// position of mass center of terrain
	float fWeight;												// weight of this terrain on screen
	int nTerrainType;											// type of this terrain
	SSoundTerrainInfo() : fWeight( 0.0f ), vPos( VNULL3 ), nTerrainType( -1 ) {}
};

