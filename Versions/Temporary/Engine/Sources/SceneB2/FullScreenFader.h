#pragma once

// simple grayscale fullscreen fader
// easly can be upgraded to color fader, etc
#define SCREEN_FADER_BLACK 0.0f
#define SCREEN_FADER_CLEAR 1.0f

struct IFullScreenFader : public CObjectBase
{
	virtual void Start( float _fDuration, float _fStartValue, float _fEndValue, bool bFadeInterfaces = true ) = 0;
	virtual bool IsInProgress() = 0;
	virtual void Draw( const NTimer::STime &time, bool bAfterDrawingInterfaces ) = 0;

	virtual void AddColorModificator( CFuncBase<CVec3> *pvColor, bool bModifyInterfaces = true ) = 0;
	virtual void RemoveColorModificator( CFuncBase<CVec3> *pvColor ) = 0;
};

IFullScreenFader *CreateFullScreenFader();

