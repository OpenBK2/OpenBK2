#include "../stdafx.h"
#include "../PresizePath.h"
#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/BasePathUnit.h"

#include <gtest/gtest.h>

namespace
{
// Only the coarse approach is substituted. Precision turning uses the real
// CBasePathUnit direction/movement-mode logic on a small, flat terrain map.
class CFinishedApproach : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CFinishedApproach );

public:
	bool Init( CBasePathUnit *, IPath *, bool, bool, CAIMap * ) override { return true; }
	bool Init( IMemento *, CBasePathUnit *, CAIMap * ) override { return true; }
	bool IsFinished() const override { return true; }
	void Stop() override {}
	void Segment( NTimer::STime ) override {}
	const bool CanGoBackward() const override { return true; }
	const bool CanGoForward() const override { return true; }
	const CVec2 PeekPathPoint( int ) const override { return VNULL2; }
	IMemento *CreateMemento() const override { return nullptr; }
};

class CTestTruck : public CBasePathUnit
{
	SUnitProfile profile;
	CVec2 halfSize = CVec2( 16, 16 );

protected:
	void UpdatePlacement( const CVec3 &, uint16_t, bool ) override {}
	void UpdateTile() override {}
	void CheckForDestroyedObjects( const CVec2 & ) const override {}
	void NullSegmTime() override {}
	ISmoothPath *CreateSmoothPath() override { return new CFinishedApproach; }

public:
	const int GetUniqueID() const override { return 1; }
	const float GetMaxPossibleSpeed() const override { return 1.0f; }
	const EAIClasses GetAIPassabilityClass() const override { return EAC_WHELL; }
	const float GetTurnRadius() const override { return 0.0f; }
	const float GetTurnSpeed() const override { return 32.0f; }
	const SUnitProfile &GetUnitProfile() const override { return profile; }
	const SRect &GetUnitRect() const override { return profile.rect; }
	const CVec2 &GetAABBHalfSize() const override { return halfSize; }
	const CVec2 GetCenterShift() const override { return VNULL2; }
	const bool CanGoBackward() const override { return true; }
	const bool CanTurnRound() const override { return true; }
	const bool IsRound() const override { return true; }
	const int GetBoundTileRadius() const override { return 1; }
	const float GetSmoothTurnThreshold() const override { return 0.0f; }
	const float GetPassability() const override { return 1.0f; }
	const bool CanGoToPoint( const CVec2 & ) const override { return true; }
	void UnitTrampled( const CBasePathUnit * ) override {}
	const bool CanUnitTrampled( const CBasePathUnit * ) const override { return false; }
	const bool IsDangerousDirExist() const override { return false; }
	const uint16_t GetDangerousDir() const override { return 0; }
	const bool IterateUnits( const CVec2 &, float, bool, const SIterateUnitsCallback & ) const override { return false; }
	const bool IsStaticUnit() const override { return false; }
};

TEST( TowingPrecision, FinalApproachTurnsToOneStableReverseHeading )
{
	CObj<CAIMap> map = new CAIMap( 16, 16, 64, 2, 16 );
	for ( const bool initiallyForward : { false, true } )
	{
		CTestTruck truck;
		truck.Init( CVec3( 400, 400, 0 ), 12000, map, nullptr, nullptr );
		truck.SetGoForward( initiallyForward );
		const CVec2 destination( 600, 500 );
		const uint16_t travelDirection = GetDirectionByVector( destination - truck.GetCenterPlain() );
		CObj<CPresizePath> path = new CPresizePath( &truck, destination, CVec2( 1, 0 ), truck.GetSmoothPath() );
		truck.SetSmoothPath( path );
		path->Segment( 50 ); // Finish the supplied coarse approach.
		bool aligned = false;
		for ( int step = 0; step < 50 && !aligned; ++step )
		{
			path->Segment( 50 );
			aligned = !truck.IsTurning() && !truck.IsGoForward() && truck.GetDirection() == travelDirection;
			// Stop before translation: this test isolates the original heading/mode conflict.
		}
		EXPECT_TRUE( aligned );
		EXPECT_EQ( truck.GetFrontDirection(), uint16_t( travelDirection + 32768 ) );
	}
}

TEST( TowingPrecision, AtDestinationFinishesFacingTheTruckDirection )
{
	CObj<CAIMap> map = new CAIMap( 16, 16, 64, 2, 16 );
	// Opposite destination headings cover both front- and rear-towed artillery.
	for ( const CVec2 finalDirection : { CVec2( 1, 0 ), CVec2( -1, 0 ) } )
	{
		CTestTruck truck;
		truck.Init( CVec3( 400, 400, 0 ), 12000, map, nullptr, nullptr );
		truck.SetGoForward( false );
		CObj<CPresizePath> path = new CPresizePath( &truck, truck.GetCenterPlain(), finalDirection, truck.GetSmoothPath() );
		truck.SetSmoothPath( path );
		for ( int step = 0; step < 50 && !path->IsFinished(); ++step )
		{
			path->Segment( 50 );
		}
		ASSERT_TRUE( path->IsFinished() );
		EXPECT_TRUE( truck.IsGoForward() );
		EXPECT_EQ( truck.GetFrontDirection(), GetDirectionByVector( finalDirection ) );
		const uint16_t facing = truck.GetFrontDirection();
		for ( int step = 0; step < 10; ++step )
		{
			path->Segment( 50 );
		}
		EXPECT_EQ( truck.GetFrontDirection(), facing );
	}
}

TEST( TowingPrecision, FullApproachReachesAuthoredFrontAndRearHookPoints )
{
	CObj<CAIMap> map = new CAIMap( 32, 32, 64, 2, 32 );
	struct SMapScope
	{
		CAIMap *previous = GetAIMap();
		explicit SMapScope( CAIMap *map ) { SetAIMap( map ); }
		~SMapScope() { SetAIMap( previous ); }
	} mapScope( map );
	for ( const bool forwardTowed : { false, true } )
	{
		const CVec2 artilleryCenter( 1000, 1000 );
		const CVec2 artilleryFront = GetVectorByDirection( 6000 );
		const float artilleryTowY = forwardTowed ? 31.0f : -31.0f;
		const float truckTowY = -80.0f;
		const CVec2 towDirection = forwardTowed ? artilleryFront : -artilleryFront;
		const CVec2 destination = artilleryCenter + towDirection * ( fabs( artilleryTowY ) + fabs( truckTowY ) );
		CTestTruck truck;
		truck.Init( CVec3( destination + CVec2( 150, 100 ), 0 ), 12000, map, nullptr, nullptr );
		truck.SetGoForward( false );
		CObj<CPresizePath> path = new CPresizePath( &truck, destination, towDirection, truck.GetSmoothPath() );
		truck.SendAlongSmoothPath( path );
		// Drive the real movement scheduler as well as the precision path. A correct
		// turning-only test cannot catch an idle path that never reaches attachment.
		for ( int step = 0; step < 100 && !path->IsFinished(); ++step )
		{
			truck.FirstSegment( 50 );
			truck.SecondSegment( 50 );
		}
		ASSERT_TRUE( path->IsFinished() );
		EXPECT_EQ( truck.GetCenterPlain(), destination );
		EXPECT_TRUE( truck.IsGoForward() );
		const CVec2 artilleryHook = artilleryCenter + artilleryFront * artilleryTowY;
		const CVec2 truckHook = truck.GetCenterPlain() + truck.GetFrontDirectionVector() * truckTowY;
		EXPECT_LT( fabs( artilleryHook - truckHook ), 0.1f );
	}
}
}
