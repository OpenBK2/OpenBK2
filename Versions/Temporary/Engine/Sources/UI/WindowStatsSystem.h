#pragma once

#include "Window.h"

#include <cstdint>

struct ISound;

class CWindowStatsSystem : public CWindow, public IStatsSystemWindow
{
	OBJECT_BASIC_METHODS( CWindowStatsSystem )
	struct SColorString
	{
		std::wstring szString;
		uint32_t dwColor;
		CPtr<IML> pGfxText;

		void Init( const wchar_t *pszStr, uint32_t col, const int nWidth );
		void SetText( const wchar_t *pszStr, uint32_t col, const int nWidth );
		SColorString() : dwColor( 0xffffffff ) {  }
		SColorString( const wchar_t *pszStr, uint32_t col, const int nWidth );
		int operator&( IBinSaver &saver );
	};

	CDBPtr<NDb::SWindowStatsSystemShared> pShared;
	CPtr<NDb::SWindowStatsSystem> pInstance;

	typedef std::unordered_map<std::wstring, SColorString> CEntries;
	CEntries entries;

public:
	virtual NDb::SWindow* GetInstance() { return pInstance; }

	void InitByDesc( const struct NDb::SUIDesc *_pDesc );
	void UpdateEntry( const std::wstring &szEntry, const std::wstring &szValue, const uint32_t dwColor );
	void Visit( struct IUIVisitor *pVisitor );

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, (CWindow *)this );
		saver.Add( 2, &pShared );
		saver.Add( 3, &pInstance );

		return 0;
	}
};
