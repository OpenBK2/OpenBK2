#include "stdafx.h"

#include "EditorOptions.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MechIconRenderer.h"
#include "ModelEditorSettings.h"
#include "SceneB2/UnitIconRendering.h"

bool NMechIconRenderer::Render( const CDBID &dbid,
                                const NIconRendering::SOptions &options,
                                CArray2D<NGfx::SPixel8888> *pCapture,
                                std::string *pError )
{
	CModelEditorSettings settings;
	Singleton<IUserDataContainer>()->Get()->SerializeSettings(
			settings, "UnitStats", SUserData::EDITOR_SETTINGS, SUserData::ST_LOAD );
	std::string szLight = settings.lightDBID.ToString();
	if ( szLight.empty() )
	{
		szLight = NEditorOptions::GetMiscString( "Light" );
	}
	if ( szLight.empty() )
	{
		szLight = NEditorOptions::GetLight( "SEASON_SUMMER", "DAY_DAY" );
	}
	// Retain the editor's environment while sharing all model/camera/capture work with IconRender.
	NUnitIconRendering::SOptions renderOptions;
	renderOptions.nWidth = options.nWidth;
	renderOptions.nHeight = options.nHeight;
	renderOptions.nGridSize = settings.nGridSize;
	renderOptions.szLight = szLight;
	renderOptions.bEditorDatabase = true;
	NUnitIconRendering::SResult result;
	const auto *pOverride = NUnitIconRendering::FindOverride( NUnitIconRendering::GetDefaultOverrides(), dbid );
	if ( !NUnitIconRendering::Render( dbid, renderOptions, pOverride, &result, pError ) )
	{
		return false;
	}
	if ( !result.shot.bFromStats )
	{
		NLog::Log( LT_IMPORTANT,
		           "Icon rendering: %s has no valid saved camera; using automatic framing.\n",
		           dbid.ToString().c_str() );
	}
	if ( !result.szCorrections.empty() )
	{
		NLog::Log( LT_IMPORTANT, "Icon rendering: %s: %s\n", dbid.ToString().c_str(), result.szCorrections.c_str() );
	}
	pCapture->Swap( result.image );
	return true;
}
