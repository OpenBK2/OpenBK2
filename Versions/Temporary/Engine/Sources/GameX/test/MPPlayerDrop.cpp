#include "../MPPlayerDrop.h"

#include <limits>

#include <gtest/gtest.h>

namespace
{
constexpr int DEPARTING_HOST = 266084;
constexpr int REPLACEMENT_HOST = 266025;
constexpr int THIRD_PLAYER = 266060;
constexpr int LATENCY = 2;
} // namespace

TEST( MPPlayerDrop, OppositeAnnouncementAndBackendRemovalOrdersAgree )
{
	// The captured match announced cutoff 7779, while the successor initially
	// chose 7780 after receiving backend removal first. Both must retain 7779.
	NMPPlayerDrop::SPlayerDrop announcementFirst;
	announcementFirst.RememberHost( DEPARTING_HOST );
	ASSERT_TRUE( announcementFirst.IsAuthoritative( DEPARTING_HOST, DEPARTING_HOST, DEPARTING_HOST, REPLACEMENT_HOST ) );
	EXPECT_EQ( announcementFirst.Schedule( 7779, 7781, LATENCY ), NMPPlayerDrop::SCHEDULED );
	ASSERT_TRUE( announcementFirst.IsAuthoritative( REPLACEMENT_HOST, REPLACEMENT_HOST, DEPARTING_HOST, THIRD_PLAYER ) );
	EXPECT_EQ( announcementFirst.Schedule( 7780, 7781, LATENCY ), NMPPlayerDrop::UNCHANGED );

	NMPPlayerDrop::SPlayerDrop backendFirst;
	backendFirst.RememberHost( DEPARTING_HOST );
	ASSERT_TRUE( backendFirst.IsAuthoritative( REPLACEMENT_HOST, REPLACEMENT_HOST, DEPARTING_HOST, THIRD_PLAYER ) );
	EXPECT_EQ( backendFirst.Schedule( 7780, 7780, LATENCY ), NMPPlayerDrop::SCHEDULED );
	ASSERT_TRUE( backendFirst.IsAuthoritative( DEPARTING_HOST, REPLACEMENT_HOST, DEPARTING_HOST, THIRD_PLAYER ) );
	EXPECT_EQ( backendFirst.Schedule( 7779, 7781, LATENCY ), NMPPlayerDrop::SCHEDULED );

	EXPECT_EQ( announcementFirst.GetRemovalSegment( LATENCY ), 7781 );
	EXPECT_EQ( backendFirst.GetRemovalSegment( LATENCY ), 7781 );
	EXPECT_TRUE( announcementFirst.IsPending() );
	EXPECT_TRUE( backendFirst.IsPending() );
}

TEST( MPPlayerDrop, CorrectionAtExactBoundaryIsStillApplicable )
{
	NMPPlayerDrop::SPlayerDrop drop;
	ASSERT_EQ( drop.Schedule( 7780, 7780, LATENCY ), NMPPlayerDrop::SCHEDULED );
	EXPECT_EQ( drop.Schedule( 7779, 7781, LATENCY ), NMPPlayerDrop::SCHEDULED );
	EXPECT_EQ( drop.GetRemovalSegment( LATENCY ), 7781 );
	drop.MarkApplied();
	EXPECT_FALSE( drop.IsPending() );
}

TEST( MPPlayerDrop, OverdueCorrectionCannotBecomeALocalImmediateRemoval )
{
	NMPPlayerDrop::SPlayerDrop drop;
	ASSERT_EQ( drop.Schedule( 7780, 7780, LATENCY ), NMPPlayerDrop::SCHEDULED );
	EXPECT_EQ( drop.Schedule( 7779, 7782, LATENCY ), NMPPlayerDrop::TOO_LATE );
	EXPECT_EQ( drop.GetSegment(), 7780 );
	EXPECT_EQ( drop.GetRemovalSegment( LATENCY ), 7782 );
	EXPECT_TRUE( drop.IsPending() );

	// An initial packet can be late too; retaining an unset decision prevents
	// the caller from confusing a local fallback with a synchronized cutoff.
	NMPPlayerDrop::SPlayerDrop lateFirstPacket;
	EXPECT_EQ( lateFirstPacket.Schedule( 7779, 7782, LATENCY ), NMPPlayerDrop::TOO_LATE );
	EXPECT_EQ( lateFirstPacket.GetSegment(), -1 );
	EXPECT_FALSE( lateFirstPacket.IsPending() );
}

TEST( MPPlayerDrop, AppliedDecisionRejectsAnEarlierCorrection )
{
	NMPPlayerDrop::SPlayerDrop drop;
	ASSERT_EQ( drop.Schedule( 7780, 7780, LATENCY ), NMPPlayerDrop::SCHEDULED );
	drop.MarkApplied();
	EXPECT_EQ( drop.Schedule( 7779, 7782, LATENCY ), NMPPlayerDrop::TOO_LATE );
	EXPECT_EQ( drop.GetSegment(), 7780 );
	EXPECT_FALSE( drop.IsPending() );
}

TEST( MPPlayerDrop, AppliedDecisionToleratesDuplicateAndLaterRelays )
{
	NMPPlayerDrop::SPlayerDrop drop;
	ASSERT_EQ( drop.Schedule( 7779, 7781, LATENCY ), NMPPlayerDrop::SCHEDULED );
	drop.MarkApplied();
	EXPECT_EQ( drop.Schedule( 7779, 7790, LATENCY ), NMPPlayerDrop::UNCHANGED );
	EXPECT_EQ( drop.Schedule( 7780, 7790, LATENCY ), NMPPlayerDrop::UNCHANGED );
	EXPECT_EQ( drop.GetRemovalSegment( LATENCY ), 7781 );
	EXPECT_FALSE( drop.IsPending() );
}

TEST( MPPlayerDrop, ThirdPeerAcceptsSuccessorRelayAcrossHostPromotion )
{
	NMPPlayerDrop::SPlayerDrop beforePromotion;
	NMPPlayerDrop::SPlayerDrop afterPromotion;
	beforePromotion.RememberHost( DEPARTING_HOST );
	afterPromotion.RememberHost( DEPARTING_HOST );
	ASSERT_TRUE( beforePromotion.IsAuthoritative( REPLACEMENT_HOST, DEPARTING_HOST, DEPARTING_HOST, REPLACEMENT_HOST ) );
	ASSERT_TRUE( afterPromotion.IsAuthoritative( REPLACEMENT_HOST, REPLACEMENT_HOST, DEPARTING_HOST, THIRD_PLAYER ) );
	EXPECT_EQ( beforePromotion.Schedule( 7779, 7780, LATENCY ), NMPPlayerDrop::SCHEDULED );
	EXPECT_EQ( afterPromotion.Schedule( 7779, 7781, LATENCY ), NMPPlayerDrop::SCHEDULED );
	EXPECT_EQ( beforePromotion.GetRemovalSegment( LATENCY ), afterPromotion.GetRemovalSegment( LATENCY ) );
}

TEST( MPPlayerDrop, FormerHostAuthorityIsLimitedToItsOwnDeparture )
{
	NMPPlayerDrop::SPlayerDrop drop;
	drop.RememberHost( DEPARTING_HOST );
	EXPECT_TRUE( drop.IsAuthoritative( DEPARTING_HOST, REPLACEMENT_HOST, DEPARTING_HOST, THIRD_PLAYER ) );
	EXPECT_FALSE( drop.IsAuthoritative( DEPARTING_HOST, REPLACEMENT_HOST, THIRD_PLAYER, THIRD_PLAYER ) );
	EXPECT_FALSE( drop.IsAuthoritative( THIRD_PLAYER, REPLACEMENT_HOST, DEPARTING_HOST, THIRD_PLAYER ) );
	EXPECT_FALSE( drop.IsAuthoritative( THIRD_PLAYER, DEPARTING_HOST, DEPARTING_HOST, REPLACEMENT_HOST ) );

	NMPPlayerDrop::SPlayerDrop unknownFormerHost;
	EXPECT_FALSE( unknownFormerHost.IsAuthoritative( DEPARTING_HOST, REPLACEMENT_HOST, DEPARTING_HOST, THIRD_PLAYER ) );
}

TEST( MPPlayerDrop, SuccessiveHostDeparturesRetainSeparateAuthority )
{
	NMPPlayerDrop::SPlayerDrop firstHost;
	NMPPlayerDrop::SPlayerDrop secondHost;
	firstHost.RememberHost( DEPARTING_HOST );
	secondHost.RememberHost( REPLACEMENT_HOST );
	ASSERT_EQ( firstHost.Schedule( 7779, 7781, LATENCY ), NMPPlayerDrop::SCHEDULED );
	firstHost.MarkApplied();

	// After the next handoff, each former host can still describe its own
	// departure, but cannot use that remembered role to remove another slot.
	EXPECT_TRUE( firstHost.IsAuthoritative( DEPARTING_HOST, THIRD_PLAYER, DEPARTING_HOST, -1 ) );
	EXPECT_TRUE( secondHost.IsAuthoritative( REPLACEMENT_HOST, THIRD_PLAYER, REPLACEMENT_HOST, -1 ) );
	EXPECT_FALSE( firstHost.IsAuthoritative( REPLACEMENT_HOST, THIRD_PLAYER, DEPARTING_HOST, -1 ) );
	EXPECT_FALSE( secondHost.IsAuthoritative( DEPARTING_HOST, THIRD_PLAYER, REPLACEMENT_HOST, -1 ) );
	EXPECT_EQ( secondHost.Schedule( 7800, 7801, LATENCY ), NMPPlayerDrop::SCHEDULED );
	EXPECT_EQ( firstHost.GetRemovalSegment( LATENCY ), 7781 );
	EXPECT_EQ( secondHost.GetRemovalSegment( LATENCY ), 7802 );
}

TEST( MPPlayerDrop, InvalidAuthorityIdentifiersCannotMatchSentinelValues )
{
	NMPPlayerDrop::SPlayerDrop drop;
	EXPECT_FALSE( drop.IsAuthoritative( -1, -1, DEPARTING_HOST, -1 ) );
	EXPECT_FALSE( drop.IsAuthoritative( REPLACEMENT_HOST, REPLACEMENT_HOST, -1, THIRD_PLAYER ) );
	drop.RememberHost( DEPARTING_HOST );
	drop.RememberHost( -1 );
	EXPECT_TRUE( drop.IsAuthoritative( DEPARTING_HOST, REPLACEMENT_HOST, DEPARTING_HOST, -1 ) );
}

TEST( MPPlayerDrop, FutureHostSelfDepartureWaitsForThePrecedingHandoff )
{
	NMPPlayerDrop::SPlayerDrop nextHost;
	EXPECT_FALSE( nextHost.IsAuthoritative( REPLACEMENT_HOST, DEPARTING_HOST, REPLACEMENT_HOST, THIRD_PLAYER ) );
	ASSERT_TRUE( nextHost.DeferSelfAnnouncement( REPLACEMENT_HOST, REPLACEMENT_HOST, 7800, LATENCY ) );
	EXPECT_EQ( nextHost.GetDeferredSegment(), 7800 );
	EXPECT_EQ( nextHost.GetSegment(), -1 );
	EXPECT_FALSE( nextHost.IsPending() );
	EXPECT_FALSE( nextHost.IsAuthoritative( REPLACEMENT_HOST, DEPARTING_HOST, REPLACEMENT_HOST, THIRD_PLAYER ) );

	// Once the first host's removal transfers authority to this client, its
	// retained self-announcement passes the same authority check as a new one.
	ASSERT_TRUE( nextHost.IsAuthoritative( REPLACEMENT_HOST, REPLACEMENT_HOST, REPLACEMENT_HOST, THIRD_PLAYER ) );
	EXPECT_EQ( nextHost.Schedule( nextHost.TakeDeferredSegment(), 7801, LATENCY ), NMPPlayerDrop::SCHEDULED );
	EXPECT_EQ( nextHost.GetRemovalSegment( LATENCY ), 7802 );
	EXPECT_EQ( nextHost.GetDeferredSegment(), -1 );
	EXPECT_EQ( nextHost.TakeDeferredSegment(), -1 );
}

TEST( MPPlayerDrop, DeferredSuccessiveHostDeparturesPreserveTheirOwnCutoffs )
{
	NMPPlayerDrop::SPlayerDrop secondHost;
	NMPPlayerDrop::SPlayerDrop thirdHost;
	ASSERT_TRUE( thirdHost.DeferSelfAnnouncement( THIRD_PLAYER, THIRD_PLAYER, 7810, LATENCY ) );
	ASSERT_TRUE( secondHost.DeferSelfAnnouncement( REPLACEMENT_HOST, REPLACEMENT_HOST, 7800, LATENCY ) );
	EXPECT_FALSE( secondHost.IsAuthoritative( REPLACEMENT_HOST, DEPARTING_HOST, REPLACEMENT_HOST, THIRD_PLAYER ) );
	EXPECT_FALSE( thirdHost.IsAuthoritative( THIRD_PLAYER, DEPARTING_HOST, THIRD_PLAYER, REPLACEMENT_HOST ) );

	ASSERT_TRUE( secondHost.IsAuthoritative( REPLACEMENT_HOST, REPLACEMENT_HOST, REPLACEMENT_HOST, THIRD_PLAYER ) );
	secondHost.RememberHost( REPLACEMENT_HOST );
	ASSERT_EQ( secondHost.Schedule( secondHost.TakeDeferredSegment(), 7801, LATENCY ), NMPPlayerDrop::SCHEDULED );
	EXPECT_FALSE( thirdHost.IsAuthoritative( THIRD_PLAYER, REPLACEMENT_HOST, THIRD_PLAYER, -1 ) );
	EXPECT_EQ( thirdHost.GetDeferredSegment(), 7810 );
	EXPECT_EQ( thirdHost.GetSegment(), -1 );

	ASSERT_TRUE( thirdHost.IsAuthoritative( THIRD_PLAYER, THIRD_PLAYER, THIRD_PLAYER, -1 ) );
	ASSERT_EQ( thirdHost.Schedule( thirdHost.TakeDeferredSegment(), 7801, LATENCY ), NMPPlayerDrop::SCHEDULED );
	EXPECT_EQ( secondHost.GetRemovalSegment( LATENCY ), 7802 );
	EXPECT_EQ( thirdHost.GetRemovalSegment( LATENCY ), 7812 );
	EXPECT_TRUE( secondHost.IsAuthoritative( REPLACEMENT_HOST, THIRD_PLAYER, REPLACEMENT_HOST, -1 ) );
}

TEST( MPPlayerDrop, DeferredAnnouncementsRetainOnlyTheEarliestValidSelfCutoff )
{
	const int nMax = ( std::numeric_limits<int>::max )();
	NMPPlayerDrop::SPlayerDrop drop;
	EXPECT_EQ( drop.GetDeferredSegment(), -1 );
	EXPECT_FALSE( drop.DeferSelfAnnouncement( DEPARTING_HOST, REPLACEMENT_HOST, 7779, LATENCY ) );
	EXPECT_FALSE( drop.DeferSelfAnnouncement( -1, -1, 7779, LATENCY ) );
	EXPECT_FALSE( drop.DeferSelfAnnouncement( DEPARTING_HOST, DEPARTING_HOST, -1, LATENCY ) );
	EXPECT_FALSE( drop.DeferSelfAnnouncement( DEPARTING_HOST, DEPARTING_HOST, 7779, -1 ) );
	EXPECT_FALSE( drop.DeferSelfAnnouncement( DEPARTING_HOST, DEPARTING_HOST, nMax, LATENCY ) );
	EXPECT_EQ( drop.GetDeferredSegment(), -1 );
	ASSERT_TRUE( drop.DeferSelfAnnouncement( DEPARTING_HOST, DEPARTING_HOST, 7780, LATENCY ) );
	ASSERT_TRUE( drop.DeferSelfAnnouncement( DEPARTING_HOST, DEPARTING_HOST, 7779, LATENCY ) );
	ASSERT_TRUE( drop.DeferSelfAnnouncement( DEPARTING_HOST, DEPARTING_HOST, 7781, LATENCY ) );
	ASSERT_TRUE( drop.DeferSelfAnnouncement( DEPARTING_HOST, DEPARTING_HOST, 7779, LATENCY ) );
	EXPECT_FALSE( drop.DeferSelfAnnouncement( REPLACEMENT_HOST, DEPARTING_HOST, 7778, LATENCY ) );
	EXPECT_EQ( drop.TakeDeferredSegment(), 7779 );
	EXPECT_EQ( drop.GetDeferredSegment(), -1 );
	EXPECT_EQ( drop.GetSegment(), -1 );
	EXPECT_FALSE( drop.IsPending() );
}

TEST( MPPlayerDrop, DeferredLateAnnouncementStillFailsWithoutChangingTheDecision )
{
	NMPPlayerDrop::SPlayerDrop drop;
	ASSERT_TRUE( drop.DeferSelfAnnouncement( REPLACEMENT_HOST, REPLACEMENT_HOST, 7800, LATENCY ) );
	ASSERT_TRUE( drop.IsAuthoritative( REPLACEMENT_HOST, REPLACEMENT_HOST, REPLACEMENT_HOST, THIRD_PLAYER ) );
	EXPECT_EQ( drop.Schedule( drop.TakeDeferredSegment(), 7803, LATENCY ), NMPPlayerDrop::TOO_LATE );
	EXPECT_EQ( drop.GetSegment(), -1 );
	EXPECT_FALSE( drop.IsPending() );
}

TEST( MPPlayerDrop, BackendDisconnectIsRememberedWithoutGrantingAuthority )
{
	NMPPlayerDrop::SPlayerDrop drop;
	EXPECT_FALSE( drop.IsDisconnected() );
	drop.RememberDisconnect();
	drop.RememberDisconnect();
	EXPECT_TRUE( drop.IsDisconnected() );
	EXPECT_EQ( drop.GetSegment(), -1 );
	EXPECT_FALSE( drop.IsPending() );
	EXPECT_FALSE( drop.IsAuthoritative( REPLACEMENT_HOST, DEPARTING_HOST, REPLACEMENT_HOST, THIRD_PLAYER ) );
	ASSERT_TRUE( drop.DeferSelfAnnouncement( REPLACEMENT_HOST, REPLACEMENT_HOST, 7800, LATENCY ) );
	EXPECT_TRUE( drop.IsDisconnected() );
	EXPECT_EQ( drop.GetDeferredSegment(), 7800 );

	// A fresh match resets remembered backend and deferred network state.
	drop = NMPPlayerDrop::SPlayerDrop();
	EXPECT_FALSE( drop.IsDisconnected() );
	EXPECT_EQ( drop.GetDeferredSegment(), -1 );
	EXPECT_EQ( drop.GetSegment(), -1 );
	EXPECT_FALSE( drop.IsPending() );
}

TEST( MPPlayerDrop, SegmentValidationIsSharedBeforeAnyAuthorityChange )
{
	const int nMax = ( std::numeric_limits<int>::max )();
	EXPECT_TRUE( NMPPlayerDrop::SPlayerDrop::IsValidSegment( 0, 0 ) );
	EXPECT_TRUE( NMPPlayerDrop::SPlayerDrop::IsValidSegment( nMax - LATENCY, LATENCY ) );
	EXPECT_FALSE( NMPPlayerDrop::SPlayerDrop::IsValidSegment( nMax, LATENCY ) );
	EXPECT_FALSE( NMPPlayerDrop::SPlayerDrop::IsValidSegment( -1, LATENCY ) );
	EXPECT_FALSE( NMPPlayerDrop::SPlayerDrop::IsValidSegment( 0, -1 ) );
}

TEST( MPPlayerDrop, RetiringHostCanStillCorrectAnAlreadyKnownDeparture )
{
	NMPPlayerDrop::SPlayerDrop drop;
	ASSERT_EQ( drop.Schedule( 7780, 7780, LATENCY ), NMPPlayerDrop::SCHEDULED );
	// The successor's relay can arrive after its own departure. It still has
	// authority over this decision, which was known before that second handoff.
	drop.RememberCorrectionAuthority( REPLACEMENT_HOST );
	drop.RememberCorrectionAuthority( REPLACEMENT_HOST );
	EXPECT_TRUE( drop.IsAuthoritative( REPLACEMENT_HOST, THIRD_PLAYER, DEPARTING_HOST, -1 ) );
	EXPECT_EQ( drop.Schedule( 7779, 7781, LATENCY ), NMPPlayerDrop::SCHEDULED );
	EXPECT_EQ( drop.GetRemovalSegment( LATENCY ), 7781 );
	EXPECT_EQ( drop.Schedule( 7780, 7781, LATENCY ), NMPPlayerDrop::UNCHANGED );
}

TEST( MPPlayerDrop, RetainedCorrectionAuthorityCannotCreateANewDeparture )
{
	NMPPlayerDrop::SPlayerDrop unknownDrop;
	unknownDrop.RememberCorrectionAuthority( REPLACEMENT_HOST );
	EXPECT_FALSE( unknownDrop.IsAuthoritative( REPLACEMENT_HOST, THIRD_PLAYER, DEPARTING_HOST, -1 ) );
	ASSERT_EQ( unknownDrop.Schedule( 7780, 7780, LATENCY ), NMPPlayerDrop::SCHEDULED );
	// Scheduling later must not retroactively grant the earlier host authority.
	EXPECT_FALSE( unknownDrop.IsAuthoritative( REPLACEMENT_HOST, THIRD_PLAYER, DEPARTING_HOST, -1 ) );
	unknownDrop.RememberCorrectionAuthority( -1 );
	EXPECT_FALSE( unknownDrop.IsAuthoritative( -1, THIRD_PLAYER, DEPARTING_HOST, -1 ) );
}

TEST( MPPlayerDrop, HistoricalCorrectionAuthoritiesSurviveFurtherHandoffsButCannotRewriteAppliedTicks )
{
	NMPPlayerDrop::SPlayerDrop drop;
	ASSERT_EQ( drop.Schedule( 7780, 7780, LATENCY ), NMPPlayerDrop::SCHEDULED );
	drop.RememberCorrectionAuthority( REPLACEMENT_HOST );
	drop.RememberCorrectionAuthority( THIRD_PLAYER );
	constexpr int NEXT_HOST = 266012;
	EXPECT_TRUE( drop.IsAuthoritative( REPLACEMENT_HOST, NEXT_HOST, DEPARTING_HOST, -1 ) );
	EXPECT_TRUE( drop.IsAuthoritative( THIRD_PLAYER, NEXT_HOST, DEPARTING_HOST, -1 ) );
	drop.MarkApplied();
	EXPECT_EQ( drop.Schedule( 7779, 7782, LATENCY ), NMPPlayerDrop::TOO_LATE );
	EXPECT_EQ( drop.GetSegment(), 7780 );
}

TEST( MPPlayerDrop, InvalidAndOverflowingSegmentsLeaveTheDecisionUnchanged )
{
	const int nMax = ( std::numeric_limits<int>::max )();
	NMPPlayerDrop::SPlayerDrop drop;
	EXPECT_EQ( drop.Schedule( -1, 0, LATENCY ), NMPPlayerDrop::INVALID );
	EXPECT_EQ( drop.Schedule( 0, -1, LATENCY ), NMPPlayerDrop::INVALID );
	EXPECT_EQ( drop.Schedule( 0, 0, -1 ), NMPPlayerDrop::INVALID );
	EXPECT_EQ( drop.Schedule( nMax, 0, 1 ), NMPPlayerDrop::INVALID );
	EXPECT_EQ( drop.GetSegment(), -1 );
	EXPECT_EQ( drop.GetRemovalSegment( LATENCY ), -1 );
	EXPECT_FALSE( drop.IsPending() );

	ASSERT_EQ( drop.Schedule( nMax - LATENCY, nMax, LATENCY ), NMPPlayerDrop::SCHEDULED );
	EXPECT_EQ( drop.GetRemovalSegment( LATENCY ), nMax );
	EXPECT_EQ( drop.Schedule( nMax, nMax, LATENCY ), NMPPlayerDrop::INVALID );
	EXPECT_EQ( drop.GetSegment(), nMax - LATENCY );
	EXPECT_EQ( drop.GetRemovalSegment( LATENCY + 1 ), -1 );
	EXPECT_EQ( drop.GetRemovalSegment( -1 ), -1 );
}

TEST( MPPlayerDrop, UnscheduledApplicationDoesNotPreventAFirstValidDecision )
{
	NMPPlayerDrop::SPlayerDrop drop;
	drop.MarkApplied();
	EXPECT_EQ( drop.Schedule( 0, 0, 0 ), NMPPlayerDrop::SCHEDULED );
	EXPECT_EQ( drop.GetRemovalSegment( 0 ), 0 );
	EXPECT_TRUE( drop.IsPending() );
	drop.MarkApplied();
	drop.MarkApplied();
	EXPECT_FALSE( drop.IsPending() );
}

TEST( MPPlayerDrop, PacketExecutionIncludesBothLatencyAndPackOffset )
{
	EXPECT_EQ( NMPPlayerDrop::GetRequiredCommandPacket( 7780, LATENCY, 1 ), 7777 );
	EXPECT_EQ( NMPPlayerDrop::GetRequiredCommandPacket( 7780, LATENCY, 4 ), 7774 );
	EXPECT_EQ( NMPPlayerDrop::GetRequiredCommandPacket( LATENCY + 1, LATENCY, 1 ), 0 );
	EXPECT_EQ( NMPPlayerDrop::GetRequiredCommandPacket( LATENCY, LATENCY, 1 ), -1 );
}

TEST( MPPlayerDrop, PendingDropStillRequiresTheEarlierCommandPrefix )
{
	NMPPlayerDrop::SPlayerDrop drop;
	ASSERT_EQ( drop.Schedule( 7779, 7780, LATENCY ), NMPPlayerDrop::SCHEDULED );
	// Packet 7777 must execute at 7780. Clearing the leaver's ordinary wait
	// mask cannot allow a client missing that packet to skip its commands.
	EXPECT_TRUE( drop.RequiresPacket( 7780, LATENCY, 1, 7776 ) );
	EXPECT_FALSE( drop.RequiresPacket( 7780, LATENCY, 1, 7777 ) );
	EXPECT_FALSE( drop.RequiresPacket( 7780, LATENCY, 1, 7778 ) );
	// At removal 7781, packet 7778 is outside the surviving command prefix.
	EXPECT_FALSE( drop.RequiresPacket( 7781, LATENCY, 1, 7776 ) );
	EXPECT_FALSE( drop.RequiresPacket( 7782, LATENCY, 1, 7776 ) );
}

TEST( MPPlayerDrop, EarlierCutoffCorrectionChangesTheRequiredPrefix )
{
	NMPPlayerDrop::SPlayerDrop drop;
	ASSERT_EQ( drop.Schedule( 7780, 7780, LATENCY ), NMPPlayerDrop::SCHEDULED );
	EXPECT_TRUE( drop.RequiresPacket( 7781, LATENCY, 1, 7777 ) );
	ASSERT_EQ( drop.Schedule( 7779, 7781, LATENCY ), NMPPlayerDrop::SCHEDULED );
	EXPECT_FALSE( drop.RequiresPacket( 7781, LATENCY, 1, 7777 ) );
	// The correction does not waive an earlier required packet.
	EXPECT_TRUE( drop.RequiresPacket( 7780, LATENCY, 1, 7776 ) );
}

TEST( MPPlayerDrop, LargerPacksPreserveTheSameRemovalBoundary )
{
	NMPPlayerDrop::SPlayerDrop drop;
	ASSERT_EQ( drop.Schedule( 7779, 7780, LATENCY ), NMPPlayerDrop::SCHEDULED );
	EXPECT_TRUE( drop.RequiresPacket( 7780, LATENCY, 4, 7773 ) );
	EXPECT_FALSE( drop.RequiresPacket( 7780, LATENCY, 4, 7774 ) );
	EXPECT_FALSE( drop.RequiresPacket( 7781, LATENCY, 4, 7773 ) );
}

TEST( MPPlayerDrop, UnscheduledAndAppliedDropsDoNotAddPacketWaits )
{
	NMPPlayerDrop::SPlayerDrop drop;
	EXPECT_FALSE( drop.RequiresPacket( 7780, LATENCY, 1, -1 ) );
	ASSERT_EQ( drop.Schedule( 7779, 7780, LATENCY ), NMPPlayerDrop::SCHEDULED );
	EXPECT_TRUE( drop.RequiresPacket( 7780, LATENCY, 1, -1 ) );
	drop.MarkApplied();
	EXPECT_FALSE( drop.RequiresPacket( 7780, LATENCY, 1, -1 ) );
}

TEST( MPPlayerDrop, PacketBoundaryArithmeticHandlesStartupAndIntegerLimits )
{
	const int nMax = ( std::numeric_limits<int>::max )();
	EXPECT_EQ( NMPPlayerDrop::GetRequiredCommandPacket( 0, LATENCY, 1 ), -1 );
	EXPECT_EQ( NMPPlayerDrop::GetRequiredCommandPacket( 0, nMax, nMax ), -1 );
	EXPECT_EQ( NMPPlayerDrop::GetRequiredCommandPacket( nMax, nMax, nMax ), -1 );
	EXPECT_EQ( NMPPlayerDrop::GetRequiredCommandPacket( nMax, nMax - 1, 1 ), 0 );
	EXPECT_EQ( NMPPlayerDrop::GetRequiredCommandPacket( nMax, 0, 1 ), nMax - 1 );
	EXPECT_EQ( NMPPlayerDrop::GetRequiredCommandPacket( -1, 0, 1 ), -1 );
	EXPECT_EQ( NMPPlayerDrop::GetRequiredCommandPacket( 0, -1, 1 ), -1 );
	EXPECT_EQ( NMPPlayerDrop::GetRequiredCommandPacket( 0, 0, 0 ), -1 );
	EXPECT_EQ( NMPPlayerDrop::GetRequiredCommandPacket( 0, 0, -1 ), -1 );

	NMPPlayerDrop::SPlayerDrop drop;
	ASSERT_EQ( drop.Schedule( 0, 0, 0 ), NMPPlayerDrop::SCHEDULED );
	EXPECT_FALSE( drop.RequiresPacket( 0, 0, nMax, -1 ) );
}
