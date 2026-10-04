#include <cstdint>

// Converted limits are not covered by the generated constructor.
uint16_t wMin = 0;
uint16_t wMax = 0;

bool ToAIUnits( bool bInEditor )
{
	const auto ToAngle = []( float fAngle ) -> uint16_t
	{
		// Keep malformed/out-of-range values bounded before the integer conversion.
		if ( std::isnan( fAngle ) )
			return 0;
		if ( fAngle >= FP_2PI )
			return 65535;
		if ( fAngle < -FP_2PI )
			fAngle = -FP_2PI;
		// Imported limits can be negative. The final integer conversion wraps the
		// signed angle without the undefined negative-float-to-unsigned conversion.
		return uint16_t( int32_t( fAngle / FP_2PI * 65535.0f ) );
	};
	wMin = ToAngle( fMin );
	wMax = ToAngle( fMax );

	return true;
} 
