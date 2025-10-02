#if !defined(__COMMON_TOOLS_SCENE__GEOMETRY__)
#define __COMMON_TOOLS_SCENE__GEOMETRY__
#pragma once


// Возвращает результат в AI точках
float GetDistanceTo3DLine( const CVec3 &rvPoint, CVec2 &rvScreenPos );
void Get2DPosOnMapHeights( CVec3 *pvPos, const CVec2 &rvScreenPos );
void Get3DPosOnMapHeights( CVec3 *pvPos, const CVec2 &rvScreenPos );
float GetTerrainHeight( float x, float y );
void UpdateTerrainHeight( CVec3 *pvPos );

void GetCameraPosition( CVec3 *pvCamAnchor );
void SetCameraPosition( const CVec3 &rvCamAnchor );

#endif // !defined(__COMMON_TOOLS_SCENE__GEOMETRY__)


