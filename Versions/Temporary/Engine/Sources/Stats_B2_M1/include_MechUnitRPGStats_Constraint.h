WORD wMin;
WORD wMax;

bool ToAIUnits( bool bInEditor )
{
	wMin = fMin >= FP_2PI ? 65535 : WORD( fMin / FP_2PI * 65535.0f );
	wMax = fMax >= FP_2PI ? 65535 : WORD( fMax / FP_2PI * 65535.0f );

	return true;
} 
