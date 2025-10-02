#include "StdAfx.h"

#include "MapSizeSorter.h"
#include "../Misc/StrProc.h"

REGISTER_SAVELOAD_CLASS( 0x1927ABC1, CListControlSorterMapSize )

bool CListControlSorterMapSize::Compare( IWindow *pSubItem1, IWindow *pSubItem2 )
{
	IListControlItem *p1 = dynamic_cast<IListControlItem*>( pSubItem1 );
	IListControlItem *p2 = dynamic_cast<IListControlItem*>( pSubItem2 );
	IWindow *pColumnItem1 = p1->GetSubItem( nColumn );
	IWindow *pColumnItem2 = p2->GetSubItem( nColumn );
	string szItem1;
	string szItem2;

	ITextView *pTextItem1 = dynamic_cast<ITextView*>( pColumnItem1 );
	if ( pTextItem1 )
		szItem1 = NStr::ToMBCS( pTextItem1->GetText() );
	else
		szItem1 = NStr::ToMBCS( pColumnItem1->GetTextString() );

	ITextView *pTextItem2 = dynamic_cast<ITextView*>( pColumnItem2 );
	if ( pTextItem2 )
		szItem2 = NStr::ToMBCS( pTextItem2->GetText() );
	else
		szItem2 = NStr::ToMBCS( pColumnItem2->GetTextString() );

	vector<string> tokens;
	int nSize1 = 0;
	NStr::SplitString( szItem1, &tokens, 'x' );
	if ( tokens.size() > 1 )
	{
		NStr::TrimBoth( tokens[1] );
		sscanf( tokens[1].c_str(), "%d", &nSize1 );
	}

	int nSize2 = 0;
	tokens.clear();
	NStr::SplitString( szItem2, &tokens, 'x' );
	if ( tokens.size() > 1 )
	{
		NStr::TrimBoth( tokens[1] );
		sscanf( tokens[1].c_str(), "%d", &nSize2 );
	}

	int nCompare = nSize1 - nSize2;
	if ( bAscending )
		return (nCompare != 0) ? nCompare < 0 : pColumnItem1 < pColumnItem2;
	else
		return (nCompare != 0) ? nCompare > 0 : pColumnItem1 > pColumnItem2;
}

void CListControlSorterMapSize::SetDirection( const bool _bAscending )
{
	bAscending = _bAscending;
}

bool CListControlSorterMapSize::IsAscending() const
{
	return bAscending;
}


