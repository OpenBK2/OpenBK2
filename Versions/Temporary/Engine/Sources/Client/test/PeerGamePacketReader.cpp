#include "Client/stdafx.h"

#include "Client/PeerGamePacket.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/NetSaver.h"

#include <gtest/gtest.h>

namespace
{

CMemoryStream SavePacket( CPtr<CNetPacket> pPacket )
{
	CMemoryStream stream;
	CPtr<IBinSaver> pSaver = CreateNetSaver( &stream, SAVER_MODE_WRITE );
	pSaver->Add( 1, &pPacket );
	return stream;
}

CPtr<CNetPacket> LoadPacket( CMemoryStream &stream )
{
	stream.Seek( 0 );
	CPtr<IBinSaver> pSaver = CreateNetSaver( &stream, SAVER_MODE_READ );
	CPtr<CNetPacket> pPacket;
	pSaver->Add( 1, &pPacket );
	return pPacket;
}

TEST( PeerGamePacketReader, ReadsHeartbeatsAndCompletePayloads )
{
	for ( const auto &bytes : { std::vector<char>(), std::vector<char>{ 'a', 'b', 'c' } } )
	{
		CPtr<CPeerGamePacket> pOriginal = new CPeerGamePacket();
		pOriginal->data = bytes;
		pOriginal->nAcknowledged = 9;
		CMemoryStream stream = SavePacket( pOriginal.GetPtr() );
		CPtr<CNetPacket> pLoaded = LoadPacket( stream );
		auto *pGame = dynamic_cast<CPeerGamePacket *>( pLoaded.GetPtr() );
		ASSERT_NE( pGame, nullptr );
		EXPECT_EQ( pGame->data, bytes );
		EXPECT_EQ( pGame->nAcknowledged, 9u );
	}
}

TEST( PeerGamePacketReader, RejectsIncompleteDataAndLengthOnDirectAndRelay )
{
	for ( bool bRelay : { false, true } )
	{
		CPtr<CPeerGamePacket> pGame = new CPeerGamePacket();
		pGame->data = { 'a', 'b', 'c' };
		CPtr<CNetPacket> pWirePacket = pGame.GetPtr();
		if ( bRelay )
		{
			pWirePacket = new CThroughServerGamePacket( 0, 17, pGame );
		}
		CMemoryStream stream = SavePacket( pWirePacket );
		ASSERT_TRUE( LoadPacket( stream ) );
		// Remove one payload byte, then part of the length prefix. Neither is an empty heartbeat.
		stream.SetSize( stream.GetSize() - 1 );
		EXPECT_FALSE( LoadPacket( stream ) );
		stream.SetSize( stream.GetSize() - 3 );
		EXPECT_FALSE( LoadPacket( stream ) );
	}
}

} // namespace
