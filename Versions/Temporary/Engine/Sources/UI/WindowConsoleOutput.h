#pragma once

#include "Window.h"

#include <cstdint>

struct SWindowEditLine;
class CWindowEditLine;

struct ISound;

class CWindowConsoleOutput : public CWindow, public IConsoleOutput
{
	OBJECT_BASIC_METHODS( CWindowConsoleOutput )

	struct SColorString
	{
	public:
		std::wstring szString;
		uint32_t dwColor;
		CPtr<IML> pGfxText;

		SColorString()
			: dwColor( 0xffffffff )
		{
		}

		SColorString( const wchar_t *pszStr, uint32_t col, const int nWidth, IWindow *pFontScaleWindow );
		int operator&( IBinSaver &saver );
	};

	CDBPtr<NDb::SWindowConsoleOutputShared> pShared;
	CPtr<NDb::SWindowConsoleOutput> pInstance;
	CPtr<IML> pUpperSign;

	typedef std::vector<SColorString> CColorStrings;
	CColorStrings vectorOfStrings; //все строчки в консоли

	int nBeginString; //начальная отображаемая строка из списка строк
										//0 считается самой свежей строчкой
protected:
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

public:
	CWindowConsoleOutput()
		: nBeginString( 0 )
	{
	}

	int operator&( IBinSaver &saver ) override;
	void InitByDesc( const struct NDb::SUIDesc *_pDesc ) override;
	void Visit( struct IUIVisitor *pVisitor ) override;

	void AddString( const std::wstring &szString, const uint32_t color ) override;
	void Scroll( const int bUp ) override;
	void ToBegin() override;
	void ToEnd() override;
	void Init() override;

	void ClearContent() override
	{
		nBeginString = 0;
		vectorOfStrings.clear();
	}

	void Segment( const int timeDiff ) override;
};
