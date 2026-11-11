#include "../stdafx.h"

#include "../HungarianMethod.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <numeric>
#include <vector>

#include <gtest/gtest.h>

namespace
{
void ExpectIdentity( const CHungarianMethod &solver, int size )
{
	ASSERT_EQ( solver.GetSize(), size );
	for ( int i = 0; i < size; ++i )
	{
		EXPECT_EQ( solver.GetResult( i ), i );
	}
}

TEST( HungarianMethod, InvalidCostsReturnStableOneToOneFallback )
{
	for ( const float invalid : { std::numeric_limits<float>::quiet_NaN(),
	                              std::numeric_limits<float>::infinity(),
	                              -std::numeric_limits<float>::infinity() } )
	{
		CArray2D<float> costs( 3, 3 );
		costs.FillZero();
		for ( int column = 0; column < 3; ++column )
		{
			costs[column][1] = invalid;
		}
		CHungarianMethod solver;
		EXPECT_FALSE( solver.Init( costs ) );
		// Production callers historically ignore Init's result and still call Solve.
		solver.Solve();
		ExpectIdentity( solver, 3 );
		solver.Solve();
		ExpectIdentity( solver, 3 );
	}
}

TEST( HungarianMethod, FiniteSubtractionOverflowReturnsFallback )
{
	CArray2D<float> costs( 2, 2 );
	costs.FillZero();
	costs[0][0] = -( std::numeric_limits<float>::max )();
	costs[1][0] = ( std::numeric_limits<float>::max )();
	CHungarianMethod solver;
	ASSERT_TRUE( solver.Init( costs ) );
	solver.Solve();
	ExpectIdentity( solver, 2 );
}

TEST( HungarianMethod, EmptyAndRejectedReinitializationDoNotRetainOldAssignments )
{
	CHungarianMethod solver;
	solver.Solve();
	EXPECT_EQ( solver.GetSize(), 0 );
	CArray2D<float> empty;
	ASSERT_TRUE( solver.Init( empty ) );
	solver.Solve();
	EXPECT_EQ( solver.GetSize(), 0 );
	CArray2D<float> square( 2, 2 );
	square.FillZero();
	ASSERT_TRUE( solver.Init( square ) );
	solver.Solve();
	ExpectIdentity( solver, 2 );
	CArray2D<float> rectangle( 2, 3 );
	EXPECT_FALSE( solver.Init( rectangle ) );
	solver.Solve();
	EXPECT_EQ( solver.GetSize(), 0 );
}

TEST( HungarianMethod, FiniteAssignmentsMatchBruteForceOptimum )
{
	// Exercise ties, zeros, negative costs and repeated matrix adjustments against
	// an independent exhaustive assignment oracle, with a fixed input sequence.
	uint32_t random = 0x156ec5u;
	for ( int size = 1; size <= 6; ++size )
	{
		for ( int example = 0; example < 80; ++example )
		{
			CArray2D<float> costs( size, size );
			for ( int row = 0; row < size; ++row )
			{
				for ( int column = 0; column < size; ++column )
				{
					random = random * 1664525u + 1013904223u;
					costs[column][row] = float( int( random % 31 ) - 15 );
				}
			}
			std::vector<int> assignment( size );
			std::iota( assignment.begin(), assignment.end(), 0 );
			float optimum = std::numeric_limits<float>::infinity();
			do
			{
				float sum = 0;
				for ( int row = 0; row < size; ++row )
				{
					sum += costs[assignment[row]][row];
				}
				optimum = (std::min)( optimum, sum );
			} while ( std::next_permutation( assignment.begin(), assignment.end() ) );

			CHungarianMethod solver;
			ASSERT_TRUE( solver.Init( costs ) );
			solver.Solve();
			float actual = 0;
			std::vector<bool> used( size, false );
			for ( int row = 0; row < size; ++row )
			{
				const int column = solver.GetResult( row );
				ASSERT_GE( column, 0 );
				ASSERT_LT( column, size );
				EXPECT_FALSE( used[column] );
				used[column] = true;
				actual += costs[column][row];
			}
			EXPECT_EQ( actual, optimum ) << "size=" << size << " example=" << example;
		}
	}
}
} // namespace
