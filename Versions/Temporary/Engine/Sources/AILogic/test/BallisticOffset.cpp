#include "../stdafx.h"
#include "../Shell.h"

#include <gtest/gtest.h>
#include <limits>

namespace
{
using Shell = NDb::SWeaponRPGStats::SShell;
constexpr float speed = 2.0f;
constexpr float range = 9000.0f;
constexpr uint16_t elevationLimit = 8192;

class BallisticOffset : public testing::TestWithParam<Shell::ETrajectoryType>
{
};

TEST_P( BallisticOffset, ZeroOffsetKeepsLegacyTrajectory )
{
	const CVec3 start( 100, 200, 0 ), target( 3100, 200, 0 );
	CObj<CBallisticTraj> legacy = new CBallisticTraj( start, target, speed, GetParam(), elevationLimit, range );
	CObj<CBallisticTraj> zero = new CBallisticTraj( start, target, speed, GetParam(), elevationLimit, range, 0.0f );
	EXPECT_EQ( legacy->GetExplTime(), zero->GetExplTime() );
	EXPECT_EQ( legacy->GetInitialVelocity(), zero->GetInitialVelocity() );
	for ( const unsigned elapsed : { 0u, 100u, 500u, 1000u } )
	{
		EXPECT_EQ( legacy->GetCoordinatesAtTime( legacy->GetStartTime() + elapsed ),
		           zero->GetCoordinatesAtTime( zero->GetStartTime() + elapsed ) );
	}
	// The unmodified shot still arrives at the target (time is quantized to milliseconds).
	const CVec3 impact = legacy->GetCoordinatesAtTime( legacy->GetExplTime() );
	EXPECT_NEAR( impact.x, target.x, speed );
	EXPECT_NEAR( impact.z, 0.0f, speed );
}

TEST_P( BallisticOffset, SignedOffsetAdjustsSpeedAndKeepsTargetAtEveryRange )
{
	for ( const float distance : { 100.0f, 500.0f, 3000.0f, range } )
	{
		const CVec3 start( 100, 200, 0 ), target( start.x + distance, start.y, 0 );
		CObj<CBallisticTraj> legacy = new CBallisticTraj( start, target, speed, GetParam(), elevationLimit, range );
		const CVec3 oldVelocity = legacy->GetInitialVelocity();
		const float oldAngle = atan2( oldVelocity.z, oldVelocity.x );
		const unsigned sampleTime = 100;
		const float oldGravity =
				2.0f * ( oldVelocity.z * sampleTime - legacy->GetCoordinatesAtTime( legacy->GetStartTime() + sampleTime ).z ) /
				( sampleTime * sampleTime );
		for ( const float offset : { -(std::min)( 0.05f, oldAngle / 2.0f ), 0.2f, 0.35f } )
		{
			SCOPED_TRACE( testing::Message() << "distance=" << distance << " offset=" << offset );
			CObj<CBallisticTraj> changed =
					new CBallisticTraj( start, target, speed, GetParam(), elevationLimit, range, offset );
			const CVec3 velocity = changed->GetInitialVelocity();
			EXPECT_GT( fabs( fabs( velocity ) - fabs( oldVelocity ) ), 0.001f );
			EXPECT_NEAR( atan2( velocity.z, velocity.x ) - oldAngle, offset, 0.00001f );
			const CVec3 position = changed->GetCoordinatesAtTime( changed->GetStartTime() + sampleTime );
			const float gravity = 2.0f * ( velocity.z * sampleTime - position.z ) / ( sampleTime * sampleTime );
			EXPECT_NEAR( gravity, oldGravity, 0.000001f );
			const CVec3 impact = changed->GetCoordinatesAtTime( changed->GetExplTime() );
			// Millisecond quantization permits at most one millisecond of travel error.
			EXPECT_NEAR( impact.x, target.x, fabs( velocity.x ) + 0.01f );
			EXPECT_NEAR( impact.z, 0.0f, fabs( velocity.z ) + 0.01f );
			EXPECT_EQ( impact.y, start.y );
			for ( int part = 0; part <= 10; ++part )
			{
				const NTimer::STime time =
						changed->GetStartTime() + ( changed->GetExplTime() - changed->GetStartTime() ) * part / 10;
				const CVec3 point = changed->GetCoordinatesAtTime( time );
				EXPECT_LE( point.x, target.x + 0.01f );
				EXPECT_GE( point.z, -0.01f );
			}
		}
	}
}

TEST_P( BallisticOffset, UnreachableOrNonFiniteOffsetRetainsTheOriginalSolution )
{
	const CVec3 start( 100, 200, 0 ), target( 3100, 200, 0 );
	CObj<CBallisticTraj> legacy = new CBallisticTraj( start, target, speed, GetParam(), elevationLimit, range );
	for ( const float offset :
	      { -1.0f, 2.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() } )
	{
		CObj<CBallisticTraj> changed =
				new CBallisticTraj( start, target, speed, GetParam(), elevationLimit, range, offset );
		EXPECT_EQ( changed->GetInitialVelocity(), legacy->GetInitialVelocity() );
		EXPECT_EQ( changed->GetExplTime(), legacy->GetExplTime() );
	}
}

TEST_P( BallisticOffset, SaveLoadPreservesOffsetTrajectory )
{
	CObj<CBallisticTraj> original = new CBallisticTraj(
			CVec3( 100, 200, 0 ), CVec3( 3100, 1200, 0 ), speed, GetParam(), elevationLimit, range, 0.2f );
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, original.GetPtr() );
	}
	stream.Seek( 0 );
	CObj<CBallisticTraj> loaded = new CBallisticTraj;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->Add( 1, loaded.GetPtr() );
	}
	EXPECT_EQ( loaded->GetExplTime(), original->GetExplTime() );
	EXPECT_EQ( loaded->GetInitialVelocity(), original->GetInitialVelocity() );
	for ( const unsigned elapsed : { 0u, 100u, 500u, 1000u } )
	{
		EXPECT_EQ( loaded->GetCoordinatesAtTime( loaded->GetStartTime() + elapsed ),
		           original->GetCoordinatesAtTime( original->GetStartTime() + elapsed ) );
	}
}

INSTANTIATE_TEST_SUITE_P( ArtilleryTrajectories,
                          BallisticOffset,
                          testing::Values( Shell::TRAJECTORY_CANNON,
                                           Shell::TRAJECTORY_HOWITZER,
                                           Shell::TRAJECTORY_GRENADE ) );
}
