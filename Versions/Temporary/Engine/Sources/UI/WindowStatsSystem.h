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

		SColorString()
			: dwColor( 0xffffffff )
		{
		}

		SColorString( const wchar_t *pszStr, uint32_t col, const int nWidth );
		int operator&( IBinSaver &saver );
	};

	CDBPtr<NDb::SWindowStatsSystemShared> pShared;
	CPtr<NDb::SWindowStatsSystem> pInstance;

	typedef std::unordered_map<std::wstring, SColorString> CEntries;
	CEntries entries;

public:
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

	// Debug statistics share the console's GUI scale.
	bool UsesHudFontScale() const override
	{
		return false;
	}

	void InitByDesc( const struct NDb::SUIDesc *_pDesc ) override;
	void UpdateEntry( const std::wstring &szEntry, const std::wstring &szValue, const uint32_t dwColor ) override;
	void Visit( struct IUIVisitor *pVisitor ) override;

	int operator&( IBinSaver &saver ) override
	{
		saver.Add( 1, (CWindow *)this );
		saver.Add( 2, &pShared );
		saver.Add( 3, &pInstance );

		return 0;
	}
};
