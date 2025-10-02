#pragma once

struct IPlayer;

namespace NUIElementsHelper
{

struct SExpProgressCareer
{
	ZDATA
	bool bFinal;
	float fStart;
	float fCur;
	float fTarget;
	float fStep;
	float fTotal;
	float fProgress;
	float fNewProgress;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&bFinal); f.Add(3,&fStart); f.Add(4,&fCur); f.Add(5,&fTarget); f.Add(6,&fStep); f.Add(7,&fTotal); f.Add(8,&fProgress); f.Add(9,&fNewProgress); return 0; }
	
	NTimer::STime timePrev; // don't save

	SExpProgressCareer() : timePrev( 0 ) {}

	void Init();
	void Step( bool bWait );
};

struct SExpProgressRank
{
	ZDATA
	bool bFinal;
	float fStart;
	float fCur;
	float fTarget;
	float fStep;
	vector<float> levels;
	int nNextLevel;
	float fCurLevelExp;
	float fTargetLevelExp;
	float fTargetNextLevelExp;
	float fProgress;
	float fNewProgress;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&bFinal); f.Add(3,&fStart); f.Add(4,&fCur); f.Add(5,&fTarget); f.Add(6,&fStep); f.Add(7,&levels); f.Add(8,&nNextLevel); f.Add(9,&fCurLevelExp); f.Add(10,&fTargetLevelExp); f.Add(11,&fTargetNextLevelExp); f.Add(12,&fProgress); f.Add(13,&fNewProgress); return 0; }
	
	NTimer::STime timePrev; // don't save

	SExpProgressRank() : timePrev( 0 ) {}

	void Init();
	void Step( bool bWait );
};

void InitRoller( IPlayer *pRoller );
void PlayRollerAnim( vector<IPlayer*> &rollers, int nStart, int nEnd, float fRollerTime );

} //namespace NUIElementsHelper


