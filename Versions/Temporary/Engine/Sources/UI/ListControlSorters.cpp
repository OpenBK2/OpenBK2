#include "stdafx.h"
#include "ListControlSorters.h"

REGISTER_SAVELOAD_CLASS(UI, SCRNE_UI_LIST_SORTER_ALPHABET, CListControlSorterAlphabet)

bool CListControlSorterAlphabet::Compare( IWindow *pSubItem1, IWindow *pSubItem2 )
{
	IListControlItem *p1 = dynamic_cast<IListControlItem*>( pSubItem1 );
	IListControlItem *p2 = dynamic_cast<IListControlItem*>( pSubItem2 );
	IWindow *pColumnItem1 = p1->GetSubItem( nColumn );
	IWindow *pColumnItem2 = p2->GetSubItem( nColumn );
	std::wstring wszItem1;
	std::wstring wszItem2;

	ITextView *pTextItem1 = dynamic_cast<ITextView*>( pColumnItem1 );
	if ( pTextItem1 )
		wszItem1 = pTextItem1->GetText();
	else
		wszItem1 = pColumnItem1->GetTextString();

	ITextView *pTextItem2 = dynamic_cast<ITextView*>( pColumnItem2 );
	if ( pTextItem2 )
		wszItem2 = pTextItem2->GetText();
	else
		wszItem2 = pColumnItem2->GetTextString();

	int nCompare = wszItem1.compare( wszItem2 );
	if ( bAscending )
		return (nCompare != 0) ? nCompare < 0 : pColumnItem1 < pColumnItem2;
	else
		return (nCompare != 0) ? nCompare > 0 : pColumnItem1 > pColumnItem2;
}

void CListControlSorterAlphabet::SetDirection( const bool _bAscending )
{
	bAscending = _bAscending;
}

bool CListControlSorterAlphabet::IsAscending() const
{
	return bAscending;
}

