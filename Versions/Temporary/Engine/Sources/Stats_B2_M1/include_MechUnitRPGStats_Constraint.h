#include <cstdint>

uint16_t wMin;
uint16_t wMax;

bool ToAIUnits( bool bInEditor )
{
	wMin = fMin >= FP_2PI ? 65535 : uint16_t( fMin / FP_2PI * 65535.0f );
	wMax = fMax >= FP_2PI ? 65535 : uint16_t( fMax / FP_2PI * 65535.0f );

	return true;
} 
