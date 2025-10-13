#pragma once

#include <boost/predef.h>

#if BOOST_OS_WINDOWS
#include <wtypes.h>
#endif

#include "Cursor.h"

class CCursor : public ICursor
{
	OBJECT_NOCOPY_METHODS( CCursor )
	//
	typedef std::unordered_map<int, HCURSOR> CModesMap;
	CModesMap modes;											// registered modes
	
	std::unordered_map<int, std::string> modesFiles;
	//
	int nCurrMode;												// current cursor mode
	HCURSOR hCurrCursor;									// current cursor
	CTRect<long> rcClip;									// current cursor clip area
	bool bAcquired;												// is control over cursor acquired?
	bool bShow;														// do we need show cursor?
	bool bCanShow;
	//
	void AcquireLocal();
	//
	~CCursor();
public:
	CCursor();
	bool Init() { return true; }
	// cursor mode
	void RegisterMode( const int nMode, const std::string &szFileName );
	bool SetMode( const int nMode );
	void OnSetCursor();
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


