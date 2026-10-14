#pragma once

#include "Cursor.h"
#include "System/WinCursor.h"

#include <chrono>

class CCursor : public ICursor
{
	OBJECT_NOCOPY_METHODS( CCursor )
	//
	typedef std::unordered_map<int, NWinCursor::TCursor> CModesMap;
	CModesMap modes; // registered modes

	std::unordered_map<int, std::string> modesFiles;
	//
	int nCurrMode;                   // current cursor mode
	NWinCursor::TCursor hCurrCursor; // current cursor
	// Where the animation of hCurrCursor has got to, how many steps it has, and
	// when the step it is showing runs out. A still cursor has one step, and so
	// does every cursor on Windows, where the animation is USER32's to run.
	int nCurrStep;
	int nStepCount;
	std::chrono::steady_clock::time_point timeNextStep;
	CTRect<long> rcClip; // current cursor clip area
	bool bAcquired;      // is control over cursor acquired?
	bool bShow;          // do we need show cursor?
	bool bCanShow;
	//
	void AcquireLocal();
	//! Point the animation at hCurrCursor's first step, from now.
	void RestartAnimation();
	//! Free every registered cursor and forget the current one.
	void ClearModes();
	//
	~CCursor() override;

public:
	CCursor();

	bool Init() override
	{
		return true;
	}

	// cursor mode
	void RegisterMode( const int nMode, const std::string &szFileName ) override;
	bool SetMode( const int nMode ) override;
	void OnSetCursor() override;
	void Step() override;
	// show/hide cursor
	void Show( const bool bShow ) override;
	// set movement bounds
	void SetBounds( const int x1, const int y1, const int x2, const int y2 ) override;
	// acquire control over cursor
	void Acquire( const bool bAcquire ) override;
	// direct set cursor position
	void SetPos( const int nX, const int nY ) override;
	// get current (!) cursor position
	const CVec2 GetPos() const override;
	void CanShow( const bool bCanShow ) override;
	//
	int operator&( IBinSaver &saver ) override;
};
