#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../DBAIConsts.h"
#include "../Diplomacy.h"
#include "../GlobalWarFog.h"
#include "../NewUpdater.h"
#include "../Technics.h"
#include "../UnitSound.h"
#include "Common_RTS_AI/AIMap.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "Stats_B2_M1/Vis2AI.h"

#include <cmath>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace
{
struct SFields
{
	std::function<void( IBinSaver & )> visit;

	int operator&( IBinSaver &saver )
	{
		visit( saver );
		return 0;
	}
};

class VisibleTerrainDistance : public testing::Test
{
	CObj<CAIMap> map;
	CMemoryStream savedGlobals;

protected:
	static constexpr int cellSize = AI_TILE_SIZE * AI_TILES_IN_VIS_TILE;

	void SetDiplomacy( bool netGame, int localPlayer = 0 )
	{
		std::vector<int> parties{ 0, 1, 2 };
		SFields fields{ [&]( IBinSaver &saver )
		                {
											saver.Add( 1, &parties );
											saver.Add( 2, &localPlayer );
											saver.Add( 3, &netGame );
										} };
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		loader->Add( 1, &theDipl );
	}

	void SetUp() override
	{
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &savedGlobals, SAVER_MODE_WRITE );
			saver->Add( 1, &theDipl );
			saver->Add( 2, &updater );
		}
		SetDiplomacy( true );
		// Headless tests have no presentation grid; neither combat party is the UI observer.
		CMemoryStream observerStream;
		int neutralParty = 2;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &observerStream, SAVER_MODE_WRITE );
			saver->Add( 10, &neutralParty );
		}
		observerStream.Seek( 0 );
		{
			CPtr<IBinSaver> loader = CreateBinSaver( &observerStream, SAVER_MODE_READ );
			updater.operator&( *loader );
		}
		map = new CAIMap( 64, 64, AI_TILE_SIZE, 2, 16 );
		SetAIMap( map );
		theWarFog.Init( 64, 64, 16, 2.0f );
	}

	void TearDown() override
	{
		theWarFog.Clear();
		SetAIMap( nullptr );
		savedGlobals.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &savedGlobals, SAVER_MODE_READ );
		loader->Add( 1, &theDipl );
		loader->Add( 2, &updater );
	}

	SWarFogUnitInfo Sight( int x = 10, int y = 10 ) const
	{
		SWarFogUnitInfo info;
		info.vPos = SVector( x, y );
		info.nRadius = 1;
		info.bPlane = true;
		// Reveal terrain without close camouflage detection, so the distinction is exercised.
		info.fSpyDiscoveryRange = 0.1f;
		return info;
	}

	void AddObserver( int x = 10, int y = 10, int party = 1, int id = 123 )
	{
		theWarFog.AddUnit( id, Sight( x, y ), party );
		theWarFog.Segment();
	}

	float Distance( float x, float y, float limit, int party = 1 ) const
	{
		return theWarFog.GetNormalizedDistanceToVisibleTerrain( CVec2( x, y ), party, limit );
	}

	CObj<CAIUnit> MakeUnit( const CVec2 &position, bool visible )
	{
		// Load the real factory on Windows and populate only the path and visibility snapshots.
		EXPECT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
		CObj<CAIUnit> unit = dynamic_cast<CAIUnit *>( NObjectFactory::MakeObject( AI_TANK ) );
		CVec3 center( position.x, position.y, 0 );
		SVector tile( int( position.x / AI_TILE_SIZE ), int( position.y / AI_TILE_SIZE ) );
		std::vector<int> visibility{ false, visible, false };
		SFields path{ [&]( IBinSaver &saver )
		              {
										saver.Add( 9, &center );
										saver.Add( 10, &tile );
									} };
		SFields common{ [&]( IBinSaver &saver ) { saver.Add( 20, &path ); } };
		SFields ai{ [&]( IBinSaver &saver )
		            {
									saver.Add( 1, &common );
									saver.Add( 51, &visibility );
								} };
		SFields mechanical{ [&]( IBinSaver &saver ) { saver.Add( 1, &ai ); } };
		SFields tank{ [&]( IBinSaver &saver ) { saver.Add( 1, &mechanical ); } };
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->Add( 1, &tank );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		loader->AddPolymorphicBase( 1, unit );
		return unit;
	}

	CObj<NDb::SAIGameConsts> MakeSoundConstants()
	{
		EXPECT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
		return dynamic_cast<NDb::SAIGameConsts *>( NObjectFactory::MakeObject( NDb::SAIGameConsts::typeID ) );
	}

	NDb::SScriptArea RevealSingleTile()
	{
		NDb::SScriptArea area;
		area.szName = "distance-test";
		area.eType = EAT_RECTANGLE;
		area.vCenter = CVec2( 21.5f * AI_TILE_SIZE, 20.5f * AI_TILE_SIZE );
		area.vAABBHalfSize = CVec2( 1.0f, 1.0f );
		theWarFog.ToggleOpenForScriptAreaTiles( area, true );
		return area;
	}
};

TEST_F( VisibleTerrainDistance, SoundDefaultsPreserveLegacyPlaybackAndOldBinaryRecords )
{
	auto constants = MakeSoundConstants();
	ASSERT_NE( constants, nullptr );
	EXPECT_FLOAT_EQ( constants->fFOWSoundDistance, -1.0f );
	EXPECT_FLOAT_EQ( constants->fFOWSoundMinBasePercentage, 0.1f );
	auto unit = MakeUnit( CVec2( 1500, 1500 ), false );
	EXPECT_FLOAT_EQ( GetUnitFOWSoundVolume( *unit, 1, constants ), 1.0f );
	EXPECT_FLOAT_EQ( GetUnitFOWSoundVolume( *unit, 1, nullptr ), 1.0f );

	// Missing binary chunks must not replace the nonzero schema defaults with zero.
	CMemoryStream stream;
	SFields oldRecord{ []( IBinSaver & ) {} };
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &oldRecord );
	}
	stream.Seek( 0 );
	CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
	loader->AddPolymorphicBase( 1, constants );
	EXPECT_FLOAT_EQ( constants->fFOWSoundDistance, -1.0f );
	EXPECT_FLOAT_EQ( constants->fFOWSoundMinBasePercentage, 0.1f );
}

TEST_F( VisibleTerrainDistance, SoundUsesMetersAndHasAnExactSilentCutoff )
{
	AddObserver();
	auto constants = MakeSoundConstants();
	constants->fFOWSoundDistance = 2.0f;
	for ( const auto &sample : std::vector<std::pair<float, float>>{
					{ 0.0f, 1.0f }, { 16.0f, 0.775f }, { 32.0f, 0.55f }, { 64.0f, 0.0f }, { 96.0f, 0.0f } } )
	{
		auto unit = MakeUnit( CVec2( 12 * cellSize + sample.first, 10.5f * cellSize ), false );
		EXPECT_FLOAT_EQ( GetUnitFOWSoundVolume( *unit, 1, constants ), sample.second );
	}
	auto almostOutside = MakeUnit( CVec2( 12 * cellSize + 63.999f, 10.5f * cellSize ), false );
	EXPECT_NEAR( GetUnitFOWSoundVolume( *almostOutside, 1, constants ), 0.1f, 0.00002f );
	EXPECT_FALSE( almostOutside->IsVisible( 1 ) );
}

TEST_F( VisibleTerrainDistance, SoundHonorsVisibilityOverridesAndZeroDistance )
{
	AddObserver();
	auto constants = MakeSoundConstants();
	constants->fFOWSoundDistance = 2.0f;
	auto camouflaged = MakeUnit( CVec2( 10.5f * cellSize, 10.5f * cellSize ), false );
	EXPECT_FLOAT_EQ( GetUnitFOWSoundVolume( *camouflaged, 1, constants ), 1.0f );
	auto visible = MakeUnit( CVec2( 1500, 1500 ), true );
	EXPECT_FLOAT_EQ( GetUnitFOWSoundVolume( *visible, 1, constants ), 1.0f );
	constants->fFOWSoundDistance = 0.0f;
	EXPECT_FLOAT_EQ( GetUnitFOWSoundVolume( *visible, 1, constants ), 1.0f );
	EXPECT_FLOAT_EQ( GetUnitFOWSoundVolume( *camouflaged, 1, constants ), 0.0f );
}

TEST_F( VisibleTerrainDistance, SoundFollowsFogChangesAndTheExplicitObserver )
{
	AddObserver();
	auto constants = MakeSoundConstants();
	constants->fFOWSoundDistance = 2.0f;
	auto unit = MakeUnit( CVec2( 12 * cellSize + 32, 10.5f * cellSize ), false );
	for ( int player : { 0, 1 } )
	{
		SetDiplomacy( true, player );
		EXPECT_FLOAT_EQ( GetUnitFOWSoundVolume( *unit, 1, constants ), 0.55f );
		EXPECT_FLOAT_EQ( GetUnitFOWSoundVolume( *unit, 0, constants ), 0.0f );
		EXPECT_FALSE( unit->IsVisible( 1 ) );
	}
	theWarFog.DeleteUnit( 123 );
	theWarFog.Segment();
	EXPECT_FLOAT_EQ( GetUnitFOWSoundVolume( *unit, 1, constants ), 0.0f );
	AddObserver();
	EXPECT_FLOAT_EQ( GetUnitFOWSoundVolume( *unit, 1, constants ), 0.55f );
	constants->fFOWSoundMinBasePercentage = 0.4f;
	EXPECT_FLOAT_EQ( GetUnitFOWSoundVolume( *unit, 1, constants ), 0.7f );
}

TEST_F( VisibleTerrainDistance, EmptyAndClearedFogSaturate )
{
	EXPECT_FLOAT_EQ( Distance( 640, 640, 1 ), 1.0f );
	EXPECT_FLOAT_EQ( Distance( 640, 640, 100000 ), 1.0f );
	AddObserver();
	theWarFog.Clear();
	EXPECT_FLOAT_EQ( Distance( 640, 640, 100000 ), 1.0f );
}

TEST_F( VisibleTerrainDistance, MeasuresToCellEdgesAndHonorsTheLimit )
{
	AddObserver();
	// The rightmost visible cell ends at x = 12 * 64, not at its center.
	EXPECT_FLOAT_EQ( Distance( 12 * cellSize + 16, 10.5f * cellSize, 64 ), 0.25f );
	EXPECT_FLOAT_EQ( Distance( 12 * cellSize + 16, 10.5f * cellSize, 32 ), 0.5f );
	EXPECT_FLOAT_EQ( Distance( 12 * cellSize + 16, 10.5f * cellSize, 16 ), 1.0f );
	EXPECT_FLOAT_EQ( Distance( 12 * cellSize + 16, 10.5f * cellSize, 15 ), 1.0f );
	EXPECT_FLOAT_EQ( Distance( 12 * cellSize + 0.5f, 10.5f * cellSize, 2 ), 0.25f );
	EXPECT_FLOAT_EQ( Distance( 12 * cellSize, 10.5f * cellSize, 1 ), 0.0f );
	EXPECT_FLOAT_EQ( Distance( 12 * cellSize, 10.5f * cellSize, ( std::numeric_limits<float>::min )() ), 0.0f );
	EXPECT_FLOAT_EQ( Distance( 10.5f * cellSize, 10.5f * cellSize, 1 ), 0.0f );
}

TEST_F( VisibleTerrainDistance, UsesEuclideanDistanceAndFindsTheNearestVisibleCell )
{
	AddObserver();
	// Offsets of 96 and 128 from the nearest visible corner give a distance of 160.
	EXPECT_FLOAT_EQ( Distance( 12 * cellSize + 96, 11 * cellSize + 128, 320 ), 0.5f );
	AddObserver( 18, 10, 1, 124 );
	EXPECT_FLOAT_EQ( Distance( 20.5f * cellSize, 10.5f * cellSize, 1000 ), 32.0f / 1000.0f );
}

TEST_F( VisibleTerrainDistance, HiddenCamouflagedUnitOnVisibleTerrainReturnsZero )
{
	AddObserver();
	CObj<CAIUnit> unit = MakeUnit( CVec2( 10.5f * cellSize, 10.5f * cellSize ), false );
	ASSERT_NE( unit, nullptr );
	ASSERT_TRUE( unit->IsCamoulflated() );
	ASSERT_FALSE( unit->IsVisible( 1 ) );
	ASSERT_FALSE( theWarFog.IsUnitVisible( unit->GetCenterTile(), 1, true ) );
	ASSERT_TRUE( theWarFog.IsTileVisible( unit->GetCenterTile(), 1 ) );
	EXPECT_FLOAT_EQ( theWarFog.GetNormalizedDistanceToVisibleTerrain( *unit, 1, 32 ), 0.0f );
}

TEST_F( VisibleTerrainDistance, UnitQueryUsesVisibilityThenTheActualPosition )
{
	AddObserver();
	CObj<CAIUnit> unit = MakeUnit( CVec2( 12 * cellSize + 16, 10.5f * cellSize ), false );
	ASSERT_NE( unit, nullptr );
	EXPECT_FLOAT_EQ( theWarFog.GetNormalizedDistanceToVisibleTerrain( *unit, 1, 64 ), 0.25f );
	unit->SetVisibility( 1, true );
	EXPECT_FLOAT_EQ( theWarFog.GetNormalizedDistanceToVisibleTerrain( *unit, 1, 1 ), 0.0f );
	EXPECT_FLOAT_EQ( theWarFog.GetNormalizedDistanceToVisibleTerrain( *unit, -1, 64 ), 1.0f );
	EXPECT_FLOAT_EQ( theWarFog.GetNormalizedDistanceToVisibleTerrain( *unit, 256, 64 ), 1.0f );
	EXPECT_FLOAT_EQ( theWarFog.GetNormalizedDistanceToVisibleTerrain( *unit, 1, 0 ), 1.0f );
}

TEST_F( VisibleTerrainDistance, MultiplayerQueriesUseTheRequestedPartyRegardlessOfLocalPlayer )
{
	AddObserver();
	for ( int player = 0; player < 3; ++player )
	{
		theDipl.SetMyNumber( player );
		EXPECT_FLOAT_EQ( Distance( 10.5f * cellSize, 10.5f * cellSize, 64, 0 ), 1.0f );
		EXPECT_FLOAT_EQ( Distance( 10.5f * cellSize, 10.5f * cellSize, 64, 1 ), 0.0f );
	}
}

TEST_F( VisibleTerrainDistance, ReflectsObserverMovementTransferAndRemovalAfterFogUpdates )
{
	AddObserver();
	theWarFog.UpdateUnit( 123, Sight( 20, 20 ) );
	theWarFog.Segment();
	EXPECT_FLOAT_EQ( Distance( 10.5f * cellSize, 10.5f * cellSize, 64 ), 1.0f );
	EXPECT_FLOAT_EQ( Distance( 20.5f * cellSize, 20.5f * cellSize, 64 ), 0.0f );
	theWarFog.ChangeParty( 123, Sight( 20, 20 ), 0 );
	theWarFog.Segment();
	EXPECT_FLOAT_EQ( Distance( 20.5f * cellSize, 20.5f * cellSize, 64, 1 ), 1.0f );
	EXPECT_FLOAT_EQ( Distance( 20.5f * cellSize, 20.5f * cellSize, 64, 0 ), 0.0f );
	theWarFog.DeleteUnit( 123 );
	theWarFog.Segment();
	EXPECT_FLOAT_EQ( Distance( 20.5f * cellSize, 20.5f * cellSize, 64, 0 ), 1.0f );
}

TEST_F( VisibleTerrainDistance, HandlesMapEdgesAndPositionsOutsideTheMap )
{
	AddObserver( 0, 0 );
	EXPECT_FLOAT_EQ( Distance( -32, 32, 64 ), 0.5f );
	EXPECT_FLOAT_EQ( Distance( -65, 32, 64 ), 1.0f );
	AddObserver( 31, 31, 1, 124 );
	EXPECT_FLOAT_EQ( Distance( 32 * cellSize + 32, 31.5f * cellSize, 64 ), 0.5f );
}

TEST_F( VisibleTerrainDistance, RejectsInvalidInputsAndClipsHugeFiniteSearches )
{
	AddObserver();
	const float infinity = std::numeric_limits<float>::infinity();
	const float nan = std::numeric_limits<float>::quiet_NaN();
	for ( int party : { -1, 2, 3, 256 } )
	{
		EXPECT_FLOAT_EQ( Distance( 640, 640, 64, party ), 1.0f );
	}
	for ( float limit : { -1.0f, 0.0f, infinity, nan } )
	{
		EXPECT_FLOAT_EQ( Distance( 640, 640, limit ), 1.0f );
	}
	EXPECT_FLOAT_EQ( Distance( nan, 640, 64 ), 1.0f );
	EXPECT_FLOAT_EQ( Distance( 640, infinity, 64 ), 1.0f );
	const float maximum = ( std::numeric_limits<float>::max )();
	EXPECT_FLOAT_EQ( Distance( maximum, maximum, 64 ), 1.0f );
	EXPECT_FLOAT_EQ( Distance( -maximum, -maximum, 64 ), 1.0f );
	EXPECT_FLOAT_EQ( Distance( 12 * cellSize + 16, 10.5f * cellSize, maximum ), 16.0f / maximum );
}

TEST_F( VisibleTerrainDistance, ScriptRevealsKeepTheirOneAITilePrecisionAndSinglePlayerScope )
{
	SetDiplomacy( false );
	NDb::SScriptArea area = RevealSingleTile();
	ASSERT_TRUE( theWarFog.IsTileVisible( SVector( 21, 20 ), 0 ) );
	ASSERT_FALSE( theWarFog.IsTileVisible( SVector( 20, 20 ), 0 ) );
	EXPECT_FLOAT_EQ( Distance( 20.5f * AI_TILE_SIZE, 20.5f * AI_TILE_SIZE, 64, 0 ), 0.25f );
	EXPECT_FLOAT_EQ( Distance( 20.5f * AI_TILE_SIZE, 20.5f * AI_TILE_SIZE, 64, 1 ), 1.0f );
	SetDiplomacy( true );
	EXPECT_FLOAT_EQ( Distance( 20.5f * AI_TILE_SIZE, 20.5f * AI_TILE_SIZE, 64, 0 ), 1.0f );
	SetDiplomacy( false );
	theWarFog.ToggleOpenForScriptAreaTiles( area, false );
	EXPECT_FLOAT_EQ( Distance( 20.5f * AI_TILE_SIZE, 20.5f * AI_TILE_SIZE, 64, 0 ), 1.0f );
}

TEST_F( VisibleTerrainDistance, BoundedSearchMatchesAnExhaustiveAITileReference )
{
	AddObserver( 5, 9 );
	AddObserver( 24, 19, 1, 124 );
	AddObserver( 14, 24, 0, 125 );
	for ( int party = 0; party < 2; ++party )
	{
		for ( int i = 0; i < 35; ++i )
		{
			const CVec2 point( ( i * 137 ) % 2200 - 80.25f, ( i * 281 ) % 2200 - 96.5f );
			for ( float limit : { 1.0f, 31.0f, 65.0f, 300.0f, 4000.0f } )
			{
				// Independent reference: every visible one-meter tile, without spatial or best-distance pruning.
				double best = limit;
				for ( int y = 0; y < 64; ++y )
				{
					for ( int x = 0; x < 64; ++x )
					{
						if ( theWarFog.IsTileVisible( SVector( x, y ), party ) )
						{
							const double dx =
									(std::max)( 0.0, std::abs( double( point.x ) - ( x + 0.5 ) * AI_TILE_SIZE ) - AI_TILE_SIZE / 2.0 );
							const double dy =
									(std::max)( 0.0, std::abs( double( point.y ) - ( y + 0.5 ) * AI_TILE_SIZE ) - AI_TILE_SIZE / 2.0 );
							best = (std::min)( best, std::sqrt( dx * dx + dy * dy ) );
						}
					}
				}
				EXPECT_FLOAT_EQ( Distance( point.x, point.y, limit, party ), float( best / limit ) )
						<< "party=" << party << " sample=" << i << " limit=" << limit;
			}
		}
	}
}
} // namespace
