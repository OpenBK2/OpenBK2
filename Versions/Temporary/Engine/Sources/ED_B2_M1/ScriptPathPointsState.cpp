#include "stdafx.h"
#include "ScriptPathPointsState.h"
#include "CommandHandlerDefines.h"

#include "MapEditorLib/Clipboard.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/ResourceDefines.h"
#include "ResourceDefines.h"
#include "SceneB2/Camera.h"
#include "port/vkcodes.h"

#include <fmt/format.h>

void CScriptPathPointsState::Enter()
{
	points.clear();
	sceneDrawTool.Clear();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}

void CScriptPathPointsState::Leave()
{
	// Keep the last copied list available after Escape, switching tools or closing the map.
	points.clear();
	sceneDrawTool.Clear();
}

void CScriptPathPointsState::Draw( IPaintContext *pPaintDC )
{
	if ( !EditorScene() )
		return;

	// Match the Fields control points, but leave the polyline open.
	std::vector<CVec3> terrainPoints = points;
	for ( CVec3 &point : terrainPoints )
	{
		UpdateTerrainHeight( &point );
		sceneDrawTool.DrawCircle( point, AI_TILE_SIZE, 8, 0xFFFF4040, false );
	}
	sceneDrawTool.DrawPolyline( terrainPoints, 0xFFFF4040, false, false );
	sceneDrawTool.Draw();
}

void CScriptPathPointsState::OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	IEditorScene *pScene = EditorScene();
	if ( !pScene || !Camera() )
		return;

	// Mouse capture can deliver a release outside the view. Misses must not add {0, 0}.
	const CVec2 screenSize = pScene->GetScreenRect();
	if ( rMousePoint.x < 0 || rMousePoint.y < 0 ||
		 rMousePoint.x >= screenSize.x || rMousePoint.y >= screenSize.y )
		return;

	CVec3 vNear, vFar, vPoint;
	Camera()->GetProjectiveRayPoints( &vNear, &vFar, CVec2( rMousePoint.x, rMousePoint.y ) );
	// Script coordinates use AI units, just like the Fields polygon editor.
	Vis2AI( &vNear );
	Vis2AI( &vFar );
	if ( !pScene->GetIntersectionWithTerrainForEditor( &vPoint, vNear, vFar ) )
		return;

	points.push_back( vPoint );
	UpdatePath();
}

void CScriptPathPointsState::OnRButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( !points.empty() )
	{
		points.pop_back();
		UpdatePath();
	}
}

void CScriptPathPointsState::OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	if ( nChar == VK_ESCAPE )
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_EDITOR, ID_TOOLS_CREATE_SCRIPT_PATH_POINTS, 0 );
}

void CScriptPathPointsState::UpdatePath()
{
	if ( !CopyPathToClipboard() )
		NLog::Log( LT_ERROR, "Could not copy script path points to the clipboard.\n" );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}

bool CScriptPathPointsState::CopyPathToClipboard() const
{
	std::string text = "{";
	for ( const CVec3 &point : points )
	{
		if ( text.size() > 1 )
			text += ", ";
		// Default fmt formatting preserves float precision and uses a Lua-compatible decimal dot.
		text += fmt::format( "{{{}, {}}}", point.x, point.y );
	}
	text += "}";
	// The GlobalAlloc, GlobalLock, OpenClipboard, SetClipboardData( CF_UNICODETEXT )
	// sequence this replaces existed to hand the Win32 clipboard an HGLOBAL it
	// would then own; wx takes the data object instead. See Clipboard.h.
	return NClipboard::SetText( text );
}
