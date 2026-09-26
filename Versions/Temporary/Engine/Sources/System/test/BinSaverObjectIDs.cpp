// Pins how the binary saver identifies objects in a file.
//
// It used to write each object's address as its ID, so saving the same data
// twice produced different bytes: font blobs, save games and replays could not
// be compared or hashed. It now numbers objects in the order it first reaches
// them. These tests save one object graph at different addresses and require
// identical bytes, and load it back to check that shared references and a
// cycle still resolve to the right objects, in both the 32-bit compatible mode
// and the 64-bit one.

// The standard headers the engine's stdafx.h prelude would supply, which
// BinSaver.h relies on without including
#include <cmath>
#include <cstdint>
#include <cstring>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Misc/Asserts.h"
#include "Misc/Tools.h"
#include "System/System.h"
#include "System/Basic.h"
#include "System/Streams.h"
#include "System/BinSaver.h"

#include <gtest/gtest.h>

namespace {

class CNode : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CNode );
public:
	int nValue = 0;
	CPtr<CNode> pFirst;
	CPtr<CNode> pSecond;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &nValue );
		saver.Add( 2, &pFirst );
		saver.Add( 3, &pSecond );
		return 0;
	}
};

}

// REGISTER_SAVELOAD_CLASS names an export macro after its module argument; a
// test executable exports nothing
#define BINSAVERTEST_EXPORT
REGISTER_SAVELOAD_CLASS( BINSAVERTEST, 0x7E57AB01, CNode )

namespace {

// Every graph saved stays alive for the rest of the run, so that the next one
// cannot be allocated where it was. Freed and rebuilt, a graph tends to land on
// exactly the same addresses, and then even an address-writing saver produces
// matching bytes; measured, it did.
std::vector<CObj<CNode>> savedGraphs;

// Root, with a node shared by two parents and a cycle back to the root:
//   root -> shared, root -> middle, middle -> shared, middle -> root
std::vector<char> SaveGraph( ESaverMode mode )
{
	CObj<CNode> pRoot = new CNode;
	savedGraphs.push_back( pRoot );
	CPtr<CNode> pMiddle = new CNode;
	CPtr<CNode> pShared = new CNode;
	pRoot->nValue = 1;
	pMiddle->nValue = 2;
	pShared->nValue = 3;
	pRoot->pFirst = pShared;
	pRoot->pSecond = pMiddle;
	pMiddle->pFirst = pShared;
	pMiddle->pSecond = pRoot.GetPtr();

	CMemoryStream stream;
	{
		CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, mode );
		pSaver->Add( 1, &pRoot );
	}
	return std::vector<char>( stream.GetBuffer(), stream.GetBuffer() + stream.GetSize() );
}

void CheckLoadedGraph( const std::vector<char> &bytes, ESaverMode mode )
{
	CMemoryStream stream;
	stream.Write( bytes.data(), static_cast<int>( bytes.size() ) );
	stream.Seek( 0 );
	CObj<CNode> pRoot;
	{
		CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, mode );
		pSaver->Add( 1, &pRoot );
	}
	ASSERT_TRUE( pRoot != 0 );
	CNode *pShared = pRoot->pFirst;
	CNode *pMiddle = pRoot->pSecond;
	ASSERT_TRUE( pShared != 0 );
	ASSERT_TRUE( pMiddle != 0 );
	EXPECT_EQ( pRoot->nValue, 1 );
	EXPECT_EQ( pMiddle->nValue, 2 );
	EXPECT_EQ( pShared->nValue, 3 );
	// one object reached two ways, not two copies
	EXPECT_EQ( pMiddle->pFirst.GetPtr(), pShared );
	// and the cycle closes on the root itself
	EXPECT_EQ( pMiddle->pSecond.GetPtr(), pRoot.GetPtr() );
	pMiddle->pSecond = 0;
}

TEST( BinSaverObjectIDs, SameGraphSavesToSameBytes )
{
	const std::vector<char> first = SaveGraph( SAVER_MODE_WRITE );
	const std::vector<char> second = SaveGraph( SAVER_MODE_WRITE );
	EXPECT_EQ( first, second );
}

TEST( BinSaverObjectIDs, SameGraphSavesToSameBytes64 )
{
	const std::vector<char> first = SaveGraph( SAVER_MODE_WRITE_64 );
	const std::vector<char> second = SaveGraph( SAVER_MODE_WRITE_64 );
	EXPECT_EQ( first, second );
}

TEST( BinSaverObjectIDs, SharedAndCyclicReferencesLoadBack )
{
	CheckLoadedGraph( SaveGraph( SAVER_MODE_WRITE ), SAVER_MODE_READ );
}

TEST( BinSaverObjectIDs, SharedAndCyclicReferencesLoadBack64 )
{
	CheckLoadedGraph( SaveGraph( SAVER_MODE_WRITE_64 ), SAVER_MODE_READ_64 );
}

}
