#pragma once

interface IManipulator;

namespace NEditorOptions
{

// get editor options system manipulator
IManipulator* CreateOptionsManipulator();
//
string GetStringFromOptions( const string &szPreName, const string &szPostName, const string &szDesiredSeason );
int GetIntFromOptions( const string &szPreName, const string &szPostName, const string &szDesiredSeason );
float GetFloatFromOptions( const string &szPreName, const string &szPostName, const string &szDesiredSeason );
CVec2 GetVec2FromOptions( const string &szPreName, const string &szPostName, const string &szDesiredSeason );
CVec3 GetVec3FromOptions( const string &szPreName, const string &szPostName, const string &szDesiredSeason );

// get peak mask texture for desired season (or for any season if no suitable)
string GetPeakMaskTexture( const string &szDesiredSeason );

// get tileset for desired season (or for any season if no suitable)
string GetTileset( const string &szDesiredSeason );

// get light & pre-light for desired season and daytime (or for any season/daytime if no suitable)
string GetLight( const string &szDesiredSeason, const string &szDayTime );
string GetPreLight( const string &szDesiredSeason, const string &szDayTime );

// get ocean water descriptor for desired season (or for any season if no suitable)
string GetOceanWater( const string &szDesiredSeason );

// get minimap creation params
string GetMinimap( const string &szDesiredSeason );

// get background map (for effects, etc) for desired season (or for any season if no suitable)
string GetBgMap( const string &szDesiredSeason );

// get background map anchor (for effects, etc) for desired season (or for any season if no suitable)
CVec3 GetBgMapAnchor( const string &szDesiredSeason );

string GetMiscString( const string &szName );

}

