#pragma once

struct IManipulator;

namespace NEditorOptions
{

// get editor options system manipulator
IManipulator* CreateOptionsManipulator();
//
std::string GetStringFromOptions( const std::string &szPreName, const std::string &szPostName, const std::string &szDesiredSeason );
int GetIntFromOptions( const std::string &szPreName, const std::string &szPostName, const std::string &szDesiredSeason );
float GetFloatFromOptions( const std::string &szPreName, const std::string &szPostName, const std::string &szDesiredSeason );
CVec2 GetVec2FromOptions( const std::string &szPreName, const std::string &szPostName, const std::string &szDesiredSeason );
CVec3 GetVec3FromOptions( const std::string &szPreName, const std::string &szPostName, const std::string &szDesiredSeason );

// get peak mask texture for desired season (or for any season if no suitable)
std::string GetPeakMaskTexture( const std::string &szDesiredSeason );

// get tileset for desired season (or for any season if no suitable)
std::string GetTileset( const std::string &szDesiredSeason );

// get light & pre-light for desired season and daytime (or for any season/daytime if no suitable)
std::string GetLight( const std::string &szDesiredSeason, const std::string &szDayTime );
std::string GetPreLight( const std::string &szDesiredSeason, const std::string &szDayTime );

// get ocean water descriptor for desired season (or for any season if no suitable)
std::string GetOceanWater( const std::string &szDesiredSeason );

// get minimap creation params
std::string GetMinimap( const std::string &szDesiredSeason );

// get background map (for effects, etc) for desired season (or for any season if no suitable)
std::string GetBgMap( const std::string &szDesiredSeason );

// get background map anchor (for effects, etc) for desired season (or for any season if no suitable)
CVec3 GetBgMapAnchor( const std::string &szDesiredSeason );

std::string GetMiscString( const std::string &szName );

}


