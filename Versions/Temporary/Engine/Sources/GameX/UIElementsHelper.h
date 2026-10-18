#pragma once

struct IPlayer;

namespace NUIElementsHelper
{

// Only Init fills these in, and only the campaign statistics screen calls it:
// a custom mission has no career or rank progression, so its statistics screen
// reads bFinal, steps the animation and saves the lot without Init ever having
// run. Stated here so that path gets "there is nothing to animate" rather than
// whatever the allocation left, which decided whether the popups waited for an
// animation that was never set up and what the progress bars were driven with.
struct SExpProgressCareer
{
	ZDATA
	bool bFinal = true;
	float fStart = 0.0f;
	float fCur = 0.0f;
	float fTarget = 0.0f;
	float fStep = 0.0f;
	float fTotal = 0.0f;
	float fProgress = 0.0f;
	float fNewProgress = 0.0f;

	ZEND int operator&( IBinSaver &f )
	{
		f.Add( 2, &bFinal );
		f.Add( 3, &fStart );
		f.Add( 4, &fCur );
		f.Add( 5, &fTarget );
		f.Add( 6, &fStep );
		f.Add( 7, &fTotal );
		f.Add( 8, &fProgress );
		f.Add( 9, &fNewProgress );
		return 0;
	}

	NTimer::STime timePrev; // don't save

	SExpProgressCareer()
		: timePrev( 0 )
	{
	}

	void Init();
	void Step( bool bWait );
};

// Same as SExpProgressCareer above: Init is the only thing that fills these,
// and the custom-mission screen never calls it.
struct SExpProgressRank
{
	ZDATA
	bool bFinal = true;
	float fStart = 0.0f;
	float fCur = 0.0f;
	float fTarget = 0.0f;
	float fStep = 0.0f;
	std::vector<float> levels;
	int nNextLevel = 0;
	float fCurLevelExp = 0.0f;
	float fTargetLevelExp = 0.0f;
	float fTargetNextLevelExp = 0.0f;
	float fProgress = 0.0f;
	float fNewProgress = 0.0f;

	ZEND int operator&( IBinSaver &f )
	{
		f.Add( 2, &bFinal );
		f.Add( 3, &fStart );
		f.Add( 4, &fCur );
		f.Add( 5, &fTarget );
		f.Add( 6, &fStep );
		f.Add( 7, &levels );
		f.Add( 8, &nNextLevel );
		f.Add( 9, &fCurLevelExp );
		f.Add( 10, &fTargetLevelExp );
		f.Add( 11, &fTargetNextLevelExp );
		f.Add( 12, &fProgress );
		f.Add( 13, &fNewProgress );
		return 0;
	}

	NTimer::STime timePrev; // don't save

	SExpProgressRank()
		: timePrev( 0 )
	{
	}

	void Init();
	void Step( bool bWait );
};

void InitRoller( IPlayer *pRoller );
void PlayRollerAnim( std::vector<IPlayer *> &rollers, int nStart, int nEnd, float fRollerTime );

} //namespace NUIElementsHelper
