#include "AILogic/stdafx.h"

#include "AILogic/AIClassesID.h"
#include "AILogic/Formation.h"
#include "AILogic/Soldier.h"
#include "AILogic/Technics.h"
#include "Common_RTS_AI/GroupSmoothPath.h"

#include <functional>
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

class CInspectableGroupPath : public CGroupSmoothPath
{
public:
	using CGroupSmoothPath::GetUnitFormationProjection;
	using CGroupSmoothPath::GetUnitFormationShift;
	using CGroupSmoothPath::RecalcCells;
};

class GroupGeometry : public testing::Test
{
protected:
	CObj<CFormation> owner;
	CObj<CSoldier> first;
	CObj<CSoldier> second;
	CObj<CInspectableGroupPath> path;

	void SetUp() override
	{
		// Retain the real unit factories in DLL builds; movement itself is not mocked.
		EXPECT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
		owner = dynamic_cast<CFormation *>( NObjectFactory::MakeObject( 0x1108D498 ) );
		first = dynamic_cast<CSoldier *>( NObjectFactory::MakeObject( AI_INFANTRY ) );
		second = dynamic_cast<CSoldier *>( NObjectFactory::MakeObject( AI_INFANTRY ) );
		ASSERT_NE( owner, nullptr );
		ASSERT_NE( first, nullptr );
		ASSERT_NE( second, nullptr );
		owner->nUniqueID = 1;
		first->nUniqueID = 2;
		second->nUniqueID = 3;
		path = new CInspectableGroupPath;

		// Set only the owner through the real saved smooth-path layout, avoiding a
		// terrain/pathfinder fixture for a test of formation cell assignment.
		const std::vector<SBinSaverExternalObject> external{ SBinSaverExternalObject( 1, owner ) };
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			CPtr<CObjectBase> ownerPointer = owner.GetPtr();
			SFields basis{ [&]( IBinSaver &fields ) { fields.Add( 2, &ownerPointer ); } };
			SFields group{ [&]( IBinSaver &fields ) { fields.Add( 1, &basis ); } };
			saver->Add( 1, &group );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->AddPolymorphicBase( 1, path );
		ASSERT_EQ( path->GetUnit(), owner );
	}
};

TEST_F( GroupGeometry, MissingPriorityConsumesGroupAndAssignsFollowingValidGroup )
{
	path->AddGeometry( { SGeometryCellInfo( CVec2( 10, 20 ), 1 ), SGeometryCellInfo( CVec2( 30, 40 ), 1 ) } );
	ASSERT_TRUE( path->AddUnit( first, 2 ) );
	ASSERT_TRUE( path->AddUnit( second, 1 ) );
	EXPECT_EQ( path->GetUnitFormationShift( first ), VNULL2 );
	EXPECT_EQ( path->GetUnitFormationProjection( first ), 0 );
	EXPECT_NE( path->GetUnitFormationShift( second ), VNULL2 );
	path->AlignGeometriesToCenter();
	EXPECT_EQ( path->GetUnitFormationShift( first ), VNULL2 );
}

TEST_F( GroupGeometry, OverfilledPriorityClearsStaleCellAndCanRecoverAfterCasualty )
{
	path->AddGeometry( { SGeometryCellInfo( CVec2( 10, 20 ), 1 ), SGeometryCellInfo( CVec2( 30, 40 ), 2 ) } );
	ASSERT_TRUE( path->AddUnit( first, 1 ) );
	EXPECT_NE( path->GetUnitFormationShift( first ), VNULL2 );
	ASSERT_TRUE( path->AddUnit( second, 1 ) );
	EXPECT_EQ( path->GetUnitFormationShift( first ), VNULL2 );
	EXPECT_EQ( path->GetUnitFormationShift( second ), VNULL2 );
	path->AlignGeometriesToCenter();
	EXPECT_EQ( path->GetUnitFormationProjection( first ), 0 );
	ASSERT_TRUE( path->DeleteUnit( second ) );
	EXPECT_NE( path->GetUnitFormationShift( first ), VNULL2 );
}

TEST_F( GroupGeometry, MissingGeometryIsSafeAndEmptyAlignmentKeepsValidCells )
{
	path->AddGeometry( { SGeometryCellInfo( CVec2( 10, 20 ), 1 ) } );
	path->AlignGeometriesToCenter();
	ASSERT_TRUE( path->AddUnit( first, 1 ) );
	const CVec2 original = path->GetUnitFormationShift( first );
	for ( const int invalid : { -1, 1 } )
	{
		path->ChangeGeometry( invalid );
		path->RecalcCells( VNULL2, CVec2( 1, 0 ) );
		EXPECT_EQ( path->GetUnitFormationShift( first ), VNULL2 );
		EXPECT_EQ( path->GetUnitFormationProjection( first ), 0 );
		EXPECT_EQ( path->GetRadius(), 0 );
		EXPECT_EQ( path->GetMaxProjection(), 0 );
	}
	path->ChangeGeometry( 0 );
	path->RecalcCells( VNULL2, CVec2( 1, 0 ) );
	EXPECT_EQ( path->GetUnitFormationShift( first ), original );
}
} // namespace
