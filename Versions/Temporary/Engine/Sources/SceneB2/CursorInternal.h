#pragma once

#include "System/WinCursor.h"

#include "Cursor.h"

#include <chrono>

class CCursor : public ICursor
{
	OBJECT_NOCOPY_METHODS( CCursor )
	//
	typedef std::unordered_map<int, NWinCursor::TCursor> CModesMap;
	CModesMap modes;											// registered modes
	
	std::unordered_map<int, std::string> modesFiles;
	//
	int nCurrMode;												// current cursor mode
	NWinCursor::TCursor hCurrCursor;			// current cursor
	// Where the animation of hCurrCursor has got to, how many steps it has, and
	// when the step it is showing runs out. A still cursor has one step, and so
	// does every cursor on Windows, where the animation is USER32's to run.
	int nCurrStep;
	int nStepCount;
	std::chrono::steady_clock::time_point timeNextStep;
	CTRect<long> rcClip;									// current cursor clip area
	bool bAcquired;												// is control over cursor acquired?
	bool bShow;														// do we need show cursor?
	bool bCanShow;
	//
	void AcquireLocal();
	//! Point the animation at hCurrCursor's first step, from now.
	void RestartAnimation();
	//! Free every registered cursor and forget the current one.
	void ClearModes();
	//
	~CCursor();
public:
	CCursor();
	bool Init() { return true; }
	// cursor mode
	void RegisterMode( const int nMode, const std::string &szFileName );
	bool SetMode( const int nMode );
	void OnSetCursor();
	void Step();
	// show/hide cursor
	void Show( const bool bShow );
	// set movement bounds
	void SetBounds( const int x1, const int y1, const int x2, const int y2 );
	// acquire control over cursor
	void Acquire( const bool bAcquire );
	// direct set cursor position
	void SetPos( const int nX, const int nY );
	// get current (!) cursor position
	const CVec2 GetPos() const;
	virtual void CanShow( const bool bCanShow );
	//
	int operator&( IBinSaver &saver );
};


