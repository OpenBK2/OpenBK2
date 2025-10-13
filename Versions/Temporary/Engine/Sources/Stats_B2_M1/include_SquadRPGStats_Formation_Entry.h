#include <cstdint>

uint16_t nDir;
bool ToAIUnits( bool bInEditor )
{
	//Vis2AI( &vPos, vPos );
	nDir = int( fDir / 360.0f * 65535.0f ) % 65535;
	return true;
} 
