int nRelaxTime;
int nFireRate;

float GetRandomDamage() const { return GetPositiveRandom( fDamagePower, nDamageRandom ); }
int GetRandomPiercing() const { return GetPositiveRandom( nPiercing, nPiercingRandom ); }
int GetMaxPossiblePiercing() const { return nPiercing + nPiercingRandom; }
int GetMinPossiblePiercing() const { return (std::max)( 0, nPiercing - nPiercingRandom ); }
const bool HasCraters() const { return pCraters != 0; }
// Only side-attack ATGMs use line-shell armor selection.
bool IsLineTrajectory() const { return etrajectory == TRAJECTORY_LINE || etrajectory == TRAJECTORY_ATGM_LINE; }
// ATGMs and SAMs share target-aware guidance and visible-projectile handling.
bool IsATGMTrajectory() const { return etrajectory == TRAJECTORY_ATGM_LINE || etrajectory == TRAJECTORY_ATGM_TOP_ATTACK || etrajectory == TRAJECTORY_SAM; }
bool IsSAMTrajectory() const { return etrajectory == TRAJECTORY_SAM; }
//const string& GetRandomCrater() const { return craters[rand() % craters.size()]; }
// преобразовать из человеческих единиц в AI
bool ToAIUnits( bool bInEditor )
{
	if ( !bInEditor )
	{
		// метры <=> AI точки
		fArea *= 32.0f;
		fArea2 *= 32.0f;
		// метры/секунду <=> AI точки/тик
		fSpeed *= 32.0f / 1000.0f;
		// [0..100] <=> [0..1]
		fBrokeTrackProbability *= 0.01f;
	}
	// пули/минуту <=> ticks между вылетами пуль в очереди
	nFireRate = int( 60000.0f / fFireRate );
	// секунды <=> ticks
	nRelaxTime = int( fRelaxTime * 1000.0f );
	return true;
}

