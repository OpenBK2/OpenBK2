#pragma once
#include "window.h"

interface IML;

class CWindowStatsSystem : public CWindow, public IStatsSystemWindow
{
	OBJECT_BASIC_METHODS( CWindowStatsSystem )
	struct SColorString
	{
		wstring szString;
		DWORD dwColor;
		CPtr<IML> pGfxText;

		void Init( const wchar_t *pszStr, DWORD col, const int nWidth );
		void SetText( const wchar_t *pszStr, DWORD col, const int nWidth );
		SColorString() : dwColor( 0xffffffff ) {  }
		SColorString( const wchar_t *pszStr, DWORD col, const int nWidth );
		int operator&( IBinSaver &saver );
	};

	CDBPtr<NDb::SWindowStatsSystemShared> pShared;
	CPtr<NDb::SWindowStatsSystem> pInstance;

	typedef hash_map<wstring, SColorString> CEntries;
	CEntries entries;

public:
	virtual NDb::SWindow* GetInstance() { return pInstance; }

	void InitByDesc( const struct NDb::SUIDesc *_pDesc );
	void UpdateEntry( const wstring &szEntry, const wstring &szValue, const DWORD dwColor );
	void Visit( interface IUIVisitor *pVisitor );

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, (CWindow *)this );
		saver.Add( 2, &pShared );
		saver.Add( 3, &pInstance );

		return 0;
	}
};

