#include "UISpecificB2/stdafx.h"

#include "UI/test/MakeDescription.h"
#include "UISpecificB2/DBUISpecificB2.h"
#include "UISpecificB2/UISpecificB2.h"

#include <gtest/gtest.h>

namespace
{
struct SFogFields
{
	bool bM1;
	std::vector<NGfx::SPixel8888> colors;
	CArray2D<uint8_t> visibility;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 12, &colors );
		saver.Add( bM1 ? 38 : 27, &visibility );
		return 0;
	}
};

class MiniMapWarFog : public testing::TestWithParam<bool>
{
protected:
	CObj<IMiniMap> minimap;

	void SetUp() override
	{
		NSingleton::RegisterSingleton( CreateUIInitialization(), IUIInitialization::tidTypeID );
		auto shared = NUITest::MakeDescription<NDb::SWindowMiniMapShared>();
		shared->placement.size = CVec2( 4, 4 );
		shared->vPoint01 = CVec2( 0, 4 );
		shared->vPoint10 = CVec2( 4, 0 );
		shared->vPoint11 = CVec2( 4, 4 );
		auto desc = NUITest::MakeDescription<NDb::SWindowMiniMap>();
		desc->PostLoad( false );
		desc->pShared = shared;
		desc->placement.size = CVec2( 4, 4 );
		minimap = dynamic_cast<IMiniMap *>( AddWindowCopy( nullptr, desc.GetPtr() ) );
		ASSERT_NE( minimap, nullptr );
		NGlobal::SetVar( "m1", int( GetParam() ) );
		NGlobal::SetVar( "minimap_dark_warfog_alpha", 7 );
		NGlobal::SetVar( "minimap_light_warfog_alpha", 3 );
	}

	void TearDown() override
	{
		NGlobal::SetVar( "minimap_dark_warfog_alpha", 128 );
		NGlobal::SetVar( "minimap_light_warfog_alpha", 0 );
		NGlobal::RemoveVar( "m1" );
		minimap = nullptr;
		NSingleton::UnRegisterSingleton( IUIInitialization::tidTypeID );
	}

	SFogFields ReadFog() const
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->AddPolymorphicBase( 1, minimap );
		}
		stream.Seek( 0 );
		SFogFields fields{ GetParam() };
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		loader->Add( 1, &fields );
		return fields;
	}
};

TEST_P( MiniMapWarFog, LiveMatchUsesDefaultOpacityAndOwnsTheVisibilitySnapshot )
{
	minimap->LoadMap( 4, 4, 3, true );
	CArray2D<uint8_t> visibility( 2, 2 );
	visibility.FillEvery( 0 );
	visibility[1][1] = 2;
	minimap->SetWarFog( &visibility );
	visibility.FillEvery( 1 );
	const SFogFields fields = ReadFog();
	ASSERT_EQ( fields.colors.size(), 3u );
	EXPECT_EQ( fields.colors[0].a, 128 );
	EXPECT_EQ( fields.colors[1].a, 64 );
	EXPECT_EQ( fields.colors[2].a, 0 );
	ASSERT_EQ( fields.visibility.GetSizeX(), 2 );
	ASSERT_EQ( fields.visibility.GetSizeY(), 2 );
	EXPECT_EQ( fields.visibility[0][0], 0 );
	EXPECT_EQ( fields.visibility[1][1], 2 );
}

TEST_P( MiniMapWarFog, OfflineMapKeepsTheConfiguredOpacity )
{
	minimap->LoadMap( 4, 4, 3, false );
	const SFogFields fields = ReadFog();
	ASSERT_EQ( fields.colors.size(), 3u );
	EXPECT_EQ( fields.colors[0].a, 7 );
	EXPECT_EQ( fields.colors[1].a, 5 );
	EXPECT_EQ( fields.colors[2].a, 3 );
}

INSTANTIATE_TEST_SUITE_P( StockAndM1, MiniMapWarFog, testing::Bool() );
} // namespace
