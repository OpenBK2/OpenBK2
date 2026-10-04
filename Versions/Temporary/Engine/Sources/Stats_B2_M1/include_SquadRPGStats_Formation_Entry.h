#include <cstdint>

// Give the converted direction a default before PostLoad.
uint16_t nDir = 0;
bool ToAIUnits( bool bInEditor )
{
	//Vis2AI( &vPos, vPos );
	nDir = int( fDir / 360.0f * 65535.0f ) % 65535;
	return true;
} 
