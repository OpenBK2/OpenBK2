#include "SceneB2/stdafx.h"

#include "SceneB2/Scene.h"

#include <gtest/gtest.h>

TEST( MultiplayerSceneDebug, RejectsUnfilteredGeometryBeforeAccessingTheScene )
{
	CObj<IScene> scene = CreateScene();
	EXPECT_FALSE( scene->IsLiveMultiplayer() );
	scene->SetLiveMultiplayer( true );
	EXPECT_TRUE( scene->IsLiveMultiplayer() );
	// Input and editor endpoints must both reject the request before touching any renderer or AI map.
	EXPECT_FALSE( scene->ToggleShow( SCENE_SHOW_AI_GEOM ) );
	EXPECT_FALSE( scene->ToggleShow( SCENE_SHOW_AI_GRID ) );
	EXPECT_FALSE( scene->ToggleShow( SCENE_SHOW_BBOXES ) );
	EXPECT_FALSE( scene->ToggleShow( SCENE_SHOW_PASS_MARKERS ) );
	EXPECT_FALSE( scene->ToggleAIGeometryMode() );
	scene->SetLiveMultiplayer( false );
	EXPECT_FALSE( scene->IsLiveMultiplayer() );
}

TEST( MultiplayerSceneDebug, NewLiveMatchClearsMipVisualization )
{
	CObj<IScene> scene = CreateScene();
	const NGlobal::CValue previous = NGlobal::GetVar( "gfx_draw_mip", 0 );
	NGlobal::SetVar( "gfx_draw_mip", 1 );
	scene->SetLiveMultiplayer( true );
	EXPECT_EQ( NGlobal::GetVar( "gfx_draw_mip" ).GetFloat(), 0.0f );
	scene->SetLiveMultiplayer( false );
	NGlobal::SetVar( "gfx_draw_mip", previous );
}
