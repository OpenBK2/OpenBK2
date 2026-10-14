#pragma once

#include "UI/UI.h"

class CListControlSorterMapSize : public IWindowSorter
{
	OBJECT_BASIC_METHODS( CListControlSorterMapSize )

	bool bAscending;
	int nColumn;

public:
	CListControlSorterMapSize()
		: bAscending( true )
	{
	}

	int operator&( IBinSaver &saver ) override
	{
		saver.Add( 1, &bAscending );
		saver.Add( 2, &nColumn );
		return 0;
	}

	bool Compare( IWindow *pSubItem1, IWindow *pSubItem2 ) override;
	void SetDirection( const bool bAscending ) override;

	void SetColumn( const int _nColumn ) override
	{
		nColumn = _nColumn;
	}

	//
	bool IsAscending() const override;
};
