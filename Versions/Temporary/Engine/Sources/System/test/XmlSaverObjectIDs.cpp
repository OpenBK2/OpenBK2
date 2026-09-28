// Pins how the XML saver identifies shared objects in a file.
//
// It used to write each object's address, cut to four bytes, as its
// __ServerPtr and in every reference to it, so saving the same data twice gave
// a different file (types.xml, written by dbcodegen, changed completely on
// every run) and on x64 two objects could have collided. It now numbers
// objects in the order it first reaches them, as the binary saver does. These
// tests save one object graph at different addresses and require identical
// text with IDs from 1 on, and load it back to check that a shared reference
// and a cycle still resolve to the right objects.

// The standard headers the engine's stdafx.h prelude would supply, which
// XmlSaver.h relies on without including
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
#include "Misc/Geom.h"
#include "System/Streams.h"
#include "System/BinSaver.h"
#include "System/XmlResource.h"
#include "System/XmlSaver.h"

#include <gtest/gtest.h>

namespace {

class CXmlNode : public CXmlResource
{
	OBJECT_NOCOPY_METHODS( CXmlNode );
public:
	int nValue = 0;
	CPtr<CXmlNode> pFirst;
	CPtr<CXmlNode> pSecond;

	int operator&( IXmlSaver &saver ) override
	{
		saver.Add( "Value", &nValue );
		saver.Add( "First", &pFirst );
		saver.Add( "Second", &pSecond );
		return 0;
	}
};

}

// REGISTER_SAVELOAD_CLASS names an export macro after its module argument; a
// test executable exports nothing
#define XMLSAVERTEST_EXPORT
REGISTER_SAVELOAD_CLASS( XMLSAVERTEST, 0x7E57AB02, CXmlNode )

namespace {

// Every graph saved stays alive for the rest of the run, so that the next one
// cannot be allocated where it was and an address-writing saver could not
// produce matching text by accident.
std::vector<CObj<CXmlNode>> savedGraphs;

// Root, with a node shared by two parents and a cycle back to the root:
//   root -> shared, root -> middle, middle -> shared, middle -> root
std::string SaveGraph()
{
	CObj<CXmlNode> pRoot = new CXmlNode;
	savedGraphs.push_back( pRoot );
	CPtr<CXmlNode> pMiddle = new CXmlNode;
	CPtr<CXmlNode> pShared = new CXmlNode;
	pRoot->nValue = 1;
	pMiddle->nValue = 2;
	pShared->nValue = 3;
	pRoot->pFirst = pShared;
	pRoot->pSecond = pMiddle;
	pMiddle->pFirst = pShared;
	pMiddle->pSecond = pRoot.GetPtr();

	CMemoryStream stream;
	{
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_WRITE );
		pSaver->Add( "Root", &pRoot );
	}
	return std::string( stream.GetBuffer(), stream.GetBuffer() + stream.GetSize() );
}

TEST( XmlSaverObjectIDs, SameGraphSavesToSameText )
{
	EXPECT_EQ( SaveGraph(), SaveGraph() );
}

TEST( XmlSaverObjectIDs, ObjectsAreNumberedFromOne )
{
	const std::string text = SaveGraph();
	// three objects, written as four little-endian bytes each, 1 to 3
	EXPECT_NE( text.find( "<__ServerPtr>01000000</__ServerPtr>" ), std::string::npos ) << text;
	EXPECT_NE( text.find( "<__ServerPtr>02000000</__ServerPtr>" ), std::string::npos ) << text;
	EXPECT_NE( text.find( "<__ServerPtr>03000000</__ServerPtr>" ), std::string::npos ) << text;
	EXPECT_EQ( text.find( "<__ServerPtr>04000000</__ServerPtr>" ), std::string::npos ) << text;
}

TEST( XmlSaverObjectIDs, SharedAndCyclicReferencesLoadBack )
{
	const std::string text = SaveGraph();
	CMemoryStream stream;
	stream.Write( text.data(), static_cast<int>( text.size() ) );
	stream.Seek( 0 );
	CObj<CXmlNode> pRoot;
	{
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
		pSaver->Add( "Root", &pRoot );
	}
	ASSERT_TRUE( pRoot != 0 );
	CXmlNode *pShared = pRoot->pFirst;
	CXmlNode *pMiddle = pRoot->pSecond;
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

}
