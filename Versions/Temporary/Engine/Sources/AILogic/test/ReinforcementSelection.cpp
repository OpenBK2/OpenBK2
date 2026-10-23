#include "../Reinforcement.h"

#include <array>

#include <gtest/gtest.h>

TEST( ReinforcementSelection, PreservesCandidateWeightsInsteadOfRotatingThem )
{
	const float weights[] = { 1.0f, 3.0f, 10.0f };
	std::array<int, 3> counts{};
	for ( int sample = 0; sample < 100; ++sample )
	{
		const int index = NReinforcement::SelectByCumulativeWeight( weights, 3, ( sample + 0.5f ) / 10.0f );
		ASSERT_GE( index, 0 );
		ASSERT_LT( index, 3 );
		++counts[index];
	}
	EXPECT_EQ( counts, ( std::array<int, 3>{ 10, 20, 70 } ) );
}

TEST( ReinforcementSelection, HandlesIntervalBoundariesAndInclusiveRandomEndpoint )
{
	const float weights[] = { 1.0f, 3.0f, 10.0f };
	EXPECT_EQ( NReinforcement::SelectByCumulativeWeight( weights, 3, 0.0f ), 0 );
	EXPECT_EQ( NReinforcement::SelectByCumulativeWeight( weights, 3, 1.0f ), 1 );
	EXPECT_EQ( NReinforcement::SelectByCumulativeWeight( weights, 3, 3.0f ), 2 );
	EXPECT_EQ( NReinforcement::SelectByCumulativeWeight( weights, 3, 10.0f ), 2 );
}

TEST( ReinforcementSelection, NeverSelectsZeroWeightIntervalsWhenPositiveWeightsExist )
{
	const float weights[] = { 0.0f, 1.0f, 1.0f, 3.0f, 3.0f };
	EXPECT_EQ( NReinforcement::SelectByCumulativeWeight( weights, 5, 0.0f ), 1 );
	EXPECT_EQ( NReinforcement::SelectByCumulativeWeight( weights, 5, 1.0f ), 3 );
	EXPECT_EQ( NReinforcement::SelectByCumulativeWeight( weights, 5, 3.0f ), 3 );
}

TEST( ReinforcementSelection, KeepsTheFallbackForZeroWeightsAndSingleCandidates )
{
	const float weights[] = { 0.0f, 0.0f, 0.0f };
	EXPECT_EQ( NReinforcement::SelectByCumulativeWeight( weights, 3, 0.0f ), 2 );
	EXPECT_EQ( NReinforcement::SelectByCumulativeWeight( weights, 1, 0.0f ), 0 );
	const float single[] = { 2.0f };
	EXPECT_EQ( NReinforcement::SelectByCumulativeWeight( single, 1, 0.0f ), 0 );
	EXPECT_EQ( NReinforcement::SelectByCumulativeWeight( single, 1, 2.0f ), 0 );
}
