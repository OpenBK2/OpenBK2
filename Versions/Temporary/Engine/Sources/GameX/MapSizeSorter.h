#pragma once

#include "../UI/UI.h"

class CListControlSorterMapSize : public IWindowSorter
{
	OBJECT_BASIC_METHODS(CListControlSorterMapSize)

		bool bAscending;
	int nColumn;
public:
	CListControlSorterMapSize() : bAscending ( true ) {  }

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &bAscending );
		saver.Add( 2, &nColumn );
		return 0;
	}

	virtual bool Compare( IWindow *pSubItem1, IWindow *pSubItem2 );
	virtual void SetDirection( const bool bAscending );
	virtual void SetColumn( const int _nColumn ) { nColumn = _nColumn; }
	// 
	virtual bool IsAscending() const;
};

