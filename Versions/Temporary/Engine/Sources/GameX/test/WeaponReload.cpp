#include "../stdafx.h"

#include "3Dmotor/RectLayout.h"
#include "B2_M1_World/MapObj.h"
#include "GameX/WeaponReloadBackground.h"
#include "Main/GameTimer.h"
#include "UI/DBUserInterface.h"
#include "UI/test/MakeDescription.h"
#include "UI/UIVisitor.h"
#include "UISpecificB2/DBUISpecificB2.h"

#include <limits>

#include <gtest/gtest.h>

namespace
{
// Capture production draw calls without initializing a renderer or loading icon assets.
struct ReloadVisitor : IUIVisitor
{
	std::vector<CRectLayout::SRect> rects;

	void ClipSet( const CTRect<float> & ) override {}

	void ClipRestore() override {}

	void VisitZClearRect( const CRectLayout &, const CTPoint<float> &, const CTRect<float> &, float ) override {}

	void VisitUIRect( const NDb::STexture *, int, const CRectLayout &layout ) override
	{
		rects.insert( rects.end(), layout.rects.begin(), layout.rects.end() );
	}

	void
	VisitUIRect( const NDb::STexture *, int, const CVec2 *, const NGfx::SPixel8888 *, const CTRect<float> & ) override
	{
	}

	void VisitUITextureRect( CPtrFuncBase<NGfx::CTexture> *, int, const CRectLayout & ) override {}

	void VisitUIText( IML *, const CTPoint<float> &, const CTRect<float> & ) override {}
};

class WeaponReload : public testing::Test
{
protected:
	CObj<IGameTimer> timer;
	CObj<CWeaponReloadBackground> background;
	NTimer::STime clock = 100;

	void SetUp() override
	{
		NSingleton::RegisterSingleton( CreateUIInitialization(), IUIInitialization::tidTypeID );
		Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( 1024, 768 );
		timer = MakeObject<IGameTimer>( 0x10075C05 );
		ASSERT_NE( timer, nullptr );
		NSingleton::RegisterSingleton( timer, IGameTimer::tidTypeID );
		timer->Reset( clock );
		background = new CWeaponReloadBackground;
		background->SetPos( CVec2( 10, 20 ), CVec2( 60, 40 ) );
	}

	void TearDown() override
	{
		background = nullptr;
		NSingleton::UnRegisterSingleton( IGameTimer::tidTypeID );
		timer = nullptr;
		NSingleton::UnRegisterSingleton( IUIInitialization::tidTypeID );
	}

	void Advance( int milliseconds )
	{
		// The real timer clamps individual frames to 500 ms.
		while ( milliseconds > 0 )
		{
			const int step = (std::min)( milliseconds, 100 );
			clock += step;
			timer->Update( clock );
			milliseconds -= step;
		}
	}

	std::vector<CRectLayout::SRect> Draw()
	{
		ReloadVisitor visitor;
		background->Visit( &visitor );
		return visitor.rects;
	}
};

TEST_F( WeaponReload, TwoGunsClearIndependentlyFromBottomToTop )
{
	background->SetReloads( { { 0, 1000 }, { 0, 2000 } }, 0 );
	auto rects = Draw();
	ASSERT_EQ( rects.size(), 2 );
	EXPECT_FLOAT_EQ( rects[0].fX, 10 );
	EXPECT_FLOAT_EQ( rects[1].fX, 40 );
	EXPECT_FLOAT_EQ( rects[0].fSizeX, 30 );
	EXPECT_FLOAT_EQ( rects[1].fSizeX, 30 );
	EXPECT_FLOAT_EQ( rects[0].fSizeY, 40 );
	EXPECT_EQ( rects[0].sColor.dwColor, 0x80404040u );
	Advance( 500 );
	rects = Draw();
	ASSERT_EQ( rects.size(), 2 );
	EXPECT_FLOAT_EQ( rects[0].fY, 20 );
	EXPECT_FLOAT_EQ( rects[0].fSizeY, 20 );
	EXPECT_FLOAT_EQ( rects[1].fSizeY, 30 );
	Advance( 500 );
	rects = Draw();
	ASSERT_EQ( rects.size(), 1 );
	EXPECT_FLOAT_EQ( rects[0].fX, 40 );
	EXPECT_FLOAT_EQ( rects[0].fSizeX, 30 );
	Advance( 1000 );
	EXPECT_TRUE( Draw().empty() );
}

TEST_F( WeaponReload, PauseAndSpeedFollowTheGameClock )
{
	background->SetReloads( { { 0, 2000 } }, 0x90606060 );
	Advance( 500 );
	timer->Pause( true );
	Advance( 1000 );
	auto rects = Draw();
	ASSERT_EQ( rects.size(), 1 );
	EXPECT_FLOAT_EQ( rects[0].fSizeY, 30 );
	EXPECT_EQ( rects[0].sColor.dwColor, 0x90606060u );
	timer->Pause( false );
	timer->SetSpeed( 1 );
	Advance( 500 );
	rects = Draw();
	ASSERT_EQ( rects.size(), 1 );
	EXPECT_FLOAT_EQ( rects[0].fSizeY, 10 );
	Advance( 250 );
	EXPECT_TRUE( Draw().empty() );
}

TEST_F( WeaponReload, GroupedSquadWeaponsKeepEveryPhysicalGunIncludingReadyOnes )
{
	SObjectStatus status;
	SObjectStatus::SWeapon member;
	member.nCount = 2;
	member.reloads = { { 0, 1000 }, {} };
	status.AddWeapon( member );
	member.nCount = 1;
	member.reloads = { { 0, 2000 } };
	status.AddWeapon( member );
	ASSERT_EQ( status.weapons.size(), 1 );
	EXPECT_EQ( status.weapons[0].nCount, 3 );
	ASSERT_EQ( status.weapons[0].reloads.size(), 3 );
	background->SetReloads( status.weapons[0].reloads, 0 );
	const auto rects = Draw();
	ASSERT_EQ( rects.size(), 2 );
	EXPECT_FLOAT_EQ( rects[0].fX, 10 );
	EXPECT_FLOAT_EQ( rects[0].fSizeX, 20 );
	EXPECT_FLOAT_EQ( rects[1].fX, 50 );
	EXPECT_FLOAT_EQ( rects[1].fSizeX, 20 );
}

TEST_F( WeaponReload, SaveLoadPreservesTheOriginalBackgroundAndCooldown )
{
	CObj<NDb::SBackgroundSimpleScallingTexture> desc;
	desc = NUITest::MakeDescription<NDb::SBackgroundSimpleScallingTexture>();
	desc->nColor = 0xffabcdef;
	CObj<IWindowPart> original = MakeObject<IWindowPart>( 0x11075B40 );
	ASSERT_NE( original, nullptr );
	original->InitByDesc( desc );
	background = new CWeaponReloadBackground( original );
	background->SetPos( CVec2( 10, 20 ), CVec2( 60, 40 ) );
	background->SetReloads( { { 0, 2000 } }, 0x90606060 );
	Advance( 500 );
	CMemoryStream stream;
	// Preserve the existing icon separately while round-tripping its new decorator.
	std::vector<SBinSaverExternalObject> external{ { 1, original.GetPtr() } };
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
		saver->Add( 1, &background );
	}
	background = nullptr;
	stream.Seek( 0 );
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		saver->Add( 1, &background );
	}
	ASSERT_NE( background, nullptr );
	background->SetPos( CVec2( 30, 40 ), CVec2( 90, 60 ) );
	background->SetFadeValue( 0.5f );
	const auto rects = Draw();
	ASSERT_EQ( rects.size(), 2 );
	EXPECT_FLOAT_EQ( rects[0].fX, 30 );
	EXPECT_FLOAT_EQ( rects[0].fSizeX, 90 );
	EXPECT_FLOAT_EQ( rects[0].fSizeY, 60 );
	EXPECT_EQ( rects[0].sColor.dwColor, FadeColor( desc->nColor, 0.5f ) );
	EXPECT_FLOAT_EQ( rects[1].fX, 30 );
	EXPECT_FLOAT_EQ( rects[1].fSizeY, 45 );
	EXPECT_EQ( rects[1].sColor.dwColor, FadeColor( 0x90606060, 0.5f ) );
}

TEST_F( WeaponReload, LegacyAndInvalidDurationsStayClearAndFutureShotsStayFull )
{
	CObj<NDb::SUIConstsB2> constants;
	constants = NUITest::MakeDescription<NDb::SUIConstsB2>();
	EXPECT_EQ( uint32_t( constants->nWeaponReloadColor ), 0x80404040u );
	background->SetReloads( { {}, { 0, -1 }, { 0, std::numeric_limits<float>::infinity() } }, 0 );
	EXPECT_TRUE( Draw().empty() );
	background->SetReloads( { { 50, 1000 } }, 0x00404040 );
	const auto rects = Draw();
	ASSERT_EQ( rects.size(), 1 );
	EXPECT_FLOAT_EQ( rects[0].fSizeY, 40 );
	EXPECT_EQ( rects[0].sColor.dwColor, 0x80404040u );
}
} // namespace
