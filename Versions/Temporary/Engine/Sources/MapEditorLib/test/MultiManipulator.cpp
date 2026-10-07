#include "MapEditorLib/stdafx.h"
#include "MapEditorLib/MultiManipulator.h"

#include <gtest/gtest.h>

namespace
{
void ExpectEmptyIterator( CMultiManipulator *pManipulator )
{
	for ( const bool bShowHidden : { false, true } )
	{
		for ( const ECacheType eCache : { ECT_NO_CACHE, ECT_CACHE_LOCAL, ECT_CACHE_GLOBAL } )
		{
			CPtr<IManipulatorIterator> pIterator = pManipulator->Iterate( bShowHidden, eCache );
			ASSERT_NE( pIterator, nullptr );
			EXPECT_TRUE( pIterator->IsEnd() );
			EXPECT_FALSE( pIterator->Next() );
			EXPECT_TRUE( pIterator->IsEnd() );
			EXPECT_EQ( pIterator->GetDesc(), nullptr );
			EXPECT_EQ( pIterator->GetID(), INVALID_NODE_ID );
			std::string szName, szType;
			EXPECT_FALSE( pIterator->GetName( &szName ) );
			EXPECT_FALSE( pIterator->GetType( &szType ) );
		}
	}
}
}

TEST( MultiManipulator, EmptySelectionHasNoProperties )
{
	// The selection manager returns an empty collection when all selected resources fail to load.
	CPtr<CMultiManipulator> pManipulator = new CMultiManipulator();
	ASSERT_TRUE( pManipulator->IsEmpty() );
	ExpectEmptyIterator( pManipulator );
}

TEST( MultiManipulator, RemovingLastManipulatorLeavesNoProperties )
{
	for ( const bool bActive : { false, true } )
	{
		for ( const bool bPropertyDesc : { false, true } )
		{
			CPtr<CMultiManipulator> pManipulator = new CMultiManipulator();
			CPtr<CMultiManipulator> pChild = new CMultiManipulator();
			const CDBID dbid( "removed.xdb" );
			pManipulator->InsertManipulator( dbid, pChild, bActive, bPropertyDesc );
			ASSERT_FALSE( pManipulator->IsEmpty() );
			pManipulator->RemoveManipulator( dbid );
			ASSERT_TRUE( pManipulator->IsEmpty() );
			EXPECT_EQ( pManipulator->GetActiveManipulator(), nullptr );
			EXPECT_EQ( pManipulator->GetPropertyDescManipulator(), nullptr );
			EXPECT_EQ( pManipulator->GetFirstManipulator(), nullptr );
			ExpectEmptyIterator( pManipulator );
		}
	}
}
