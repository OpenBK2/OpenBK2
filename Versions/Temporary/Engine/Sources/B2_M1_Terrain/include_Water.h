#define DEF_WAVES_PHASE_OFFSET_VAR 0.2f

void PostLoad( bool bInEditor )
{
  float fPhaseOffset = 0.0f;
	for ( vector<SWaterWaveType>::iterator it = waves.begin(); it != waves.end(); ++it )
  {
		it->PostLoad( bInEditor );
    fPhaseOffset += DEF_WAVES_PHASE_OFFSET_VAR * ( 0.5f + (float)rand() / RAND_MAX * 0.5f );
  }
}

