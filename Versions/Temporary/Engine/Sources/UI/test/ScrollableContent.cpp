#include "UI/stdafx.h"
#include "UI/Window.h"

#include <gtest/gtest.h>

namespace
{
void SetPlacement( NDb::SWindowPlacement *placement, float width, float height )
{
	placement->position = VNULL2;
	placement->size = CVec2( width, height );
	placement->lowerMargin = VNULL2;
	placement->upperMargin = VNULL2;
	placement->horAllign = NDb::EPA_LOW_END;
	placement->verAllign = NDb::EPA_LOW_END;
}

CPtr<NDb::SWindowSimple> SimpleDescription( float width, float height )
{
	CPtr<NDb::SWindowSimpleShared> shared = MakeObject<NDb::SWindowSimpleShared>( NDb::SWindowSimpleShared::typeID );
	SetPlacement( &shared->placement, width, height );
	CPtr<NDb::SWindowSimple> desc = MakeObject<NDb::SWindowSimple>( NDb::SWindowSimple::typeID );
	desc->nClassTypeID = 0x110772C1; // CWindowSimple, as in the shipped UI descriptors.
	desc->pShared = shared;
	desc->bVisible = true;
	SetPlacement( &desc->placement, width, height );
	return desc;
}

CWindow *MakeScrollWindow( float width, float height )
{
	CPtr<NDb::SWindowSimple> border = SimpleDescription( width, height );
	border->placement.horAllign = NDb::EPA_MARGIN;
	border->placement.verAllign = NDb::EPA_MARGIN;
	CPtr<NDb::SWindowScrollableContainerShared> shared =
		MakeObject<NDb::SWindowScrollableContainerShared>( NDb::SWindowScrollableContainerShared::typeID );
	SetPlacement( &shared->placement, width, height );
	shared->pBorder = border;
	CPtr<NDb::SWindowScrollableContainer> desc =
		MakeObject<NDb::SWindowScrollableContainer>( NDb::SWindowScrollableContainer::typeID );
	desc->nClassTypeID = 0x170AF301; // CWindowScrollableContainer.
	desc->pShared = shared;
	desc->bVisible = true;
	SetPlacement( &desc->placement, width, height );
	return CUIFactory::MakeWindow( desc );
}
}

TEST( ScrollableContent, ParentRepositionKeepsTheEntireDescriptionScrollable )
{
	for ( int height : { 200, 600, 1200 } )
	{
		SCOPED_TRACE( height );
		CObj<CWindow> panel = CUIFactory::MakeWindow( SimpleDescription( 300, 300 ) );
		CObj<CWindow> scroll = MakeScrollWindow( 255, 100 );
		panel->AddChild( scroll, true );
		IScrollableContainer *container = dynamic_cast<IScrollableContainer*>( scroll.GetPtr() );
		ISliderNotify *slider = dynamic_cast<ISliderNotify*>( scroll.GetPtr() );
		ASSERT_NE( container, nullptr );
		ASSERT_NE( slider, nullptr );

		CObj<CWindow> description = CUIFactory::MakeWindow( SimpleDescription( 255, height ) );
		container->PushBack( description, false );
		container->Update();
		ASSERT_FLOAT_EQ( description->GetParent()->GetWindowRect().Height(), height );

		// Campaign panels are reparented into an outer scrolling list after
		// their text and inner scrollbar have already been sized.
		panel->Reposition( CTRect<float>( 100, 50, 400, 350 ) );
		EXPECT_FLOAT_EQ( description->GetParent()->GetWindowRect().Height(), height );

		// Moving that outer list must also leave the last line reachable.
		panel->SetPlacement( -80, 0, 0, 0, EWPF_POS_X );
		slider->SliderPosition( height - 100, nullptr );
		const CTRect<float> content = description->GetParent()->GetWindowRect();
		const CTRect<float> viewport = scroll->GetWindowRect();
		EXPECT_FLOAT_EQ( content.y2, viewport.y2 );
		EXPECT_GE( content.y2, description->GetWindowRect().y2 );
	}
}

TEST( ScrollableContent, RepositionRemeasuresChangedChildHeights )
{
	CObj<CWindow> scroll = MakeScrollWindow( 255, 100 );
	scroll->Reposition( CTRect<float>( 0, 0, 255, 100 ) );
	IScrollableContainer *container = dynamic_cast<IScrollableContainer*>( scroll.GetPtr() );
	ASSERT_NE( container, nullptr );
	CObj<CWindow> description = CUIFactory::MakeWindow( SimpleDescription( 255, 200 ) );
	container->PushBack( description, false );
	// A font or wrapping-width change can increase a paragraph's height.
	description->SetPlacement( 0, 0, 0, 700, EWPF_SIZE_Y );
	scroll->Reposition( CTRect<float>( 0, 0, 255, 100 ) );
	EXPECT_FLOAT_EQ( description->GetParent()->GetWindowRect().Height(), 700 );
}
