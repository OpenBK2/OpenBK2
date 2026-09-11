#pragma once

// GetComboBoxEditParameters and SetComboBoxEditParameters from EditParameter.h,
// for a wxChoice.
//
// The edit-parameter palettes keep a list of strings and an index into it in
// their SEditParameters, and exchange either or both with a combo box as nFlags
// says. EditParameter.h does that for a CComboBox, once, and every MFC palette
// calls it. The wx palettes had grown a private copy each -- the field palette
// for its fields, the map object palette for its players -- and the model
// palette has six combo boxes on its own. This is the one copy.
//
// **The list index travels as client data, always.** A combo box with
// CBS_SORT puts an item somewhere other than where it was added, so the
// position of an item in the control is not its position in the list the state
// holds, and the state only knows items by the latter. MFC keeps the index as
// item data for the same reason. For an unsorted combo box position and index
// agree, and carrying the index anyway costs nothing and means a palette can
// change its mind about sorting without changing anything else.
//
// Header-only and guarded, like the other Wx*.h here, because MapEditorLib
// does not link wx; only the front ends that include this do.

#ifdef OBK2_WITH_WX

#include <wx/choice.h>

#include <cstdint>
#include <string>
#include <vector>

namespace NWxEditParameter
{
	// The list index the item at nPosition stands for.
	inline int IndexAt( const wxChoice &rChoice, unsigned nPosition )
	{
		return static_cast<int>( reinterpret_cast<uintptr_t>( rChoice.GetClientData( nPosition ) ) );
	}

	// The list index of the selected item, or -1 with nothing selected --
	// which is what GetComboBoxEditParameters answers too.
	inline int SelectedIndex( const wxChoice &rChoice )
	{
		const int nPosition = rChoice.GetSelection();
		return ( nPosition == wxNOT_FOUND ) ? -1 : IndexAt( rChoice, nPosition );
	}

	// GetComboBoxEditParameters. With bCount, the list is rebuilt in list
	// order from the client data rather than read off the control top to
	// bottom, so a sorted control reports the list the state gave it. With
	// bIndex, the selected item's list index.
	template <class TList>
	void ReadChoice( const wxChoice &rChoice, TList *pList, int *pIndex, bool bCount, bool bIndex )
	{
		if ( bCount && ( pList != 0 ) )
		{
			std::vector<std::string> stringList( rChoice.GetCount(), std::string() );
			for ( unsigned nPosition = 0; nPosition < rChoice.GetCount(); ++nPosition )
			{
				const int nListIndex = IndexAt( rChoice, nPosition );
				if ( ( nListIndex >= 0 ) && ( nListIndex < static_cast<int>( stringList.size() ) ) )
				{
					stringList[nListIndex] = std::string( rChoice.GetString( nPosition ).utf8_str() );
				}
			}
			// clear and push_back rather than assignment, as the MFC helper
			// does, so that any list type with those two will do.
			pList->clear();
			for ( std::vector<std::string>::const_iterator itString = stringList.begin(); itString != stringList.end(); ++itString )
			{
				pList->push_back( *itString );
			}
		}
		if ( bIndex && ( pIndex != 0 ) )
		{
			( *pIndex ) = SelectedIndex( rChoice );
		}
	}

	// SetComboBoxEditParameters. With bCount the control is refilled from
	// rList; with bIndex the selection moves to nIndex. Refilling loses the
	// selection, so it is remembered first as a list index -- the one thing
	// that survives a re-sort -- and found again afterwards. An index past the
	// end falls back to the first item, as it always has.
	template <class TList>
	void WriteChoice( wxChoice *pChoice, const TList &rList, int nIndex, bool bCount, bool bIndex )
	{
		if ( ( pChoice == 0 ) || ( !bCount && !bIndex ) )
		{
			return;
		}
		int nSelectedIndex = SelectedIndex( *pChoice );
		if ( nSelectedIndex < 0 )
		{
			nSelectedIndex = 0;
		}
		if ( bCount )
		{
			pChoice->Clear();
			uintptr_t nListIndex = 0;
			for ( typename TList::const_iterator itString = rList.begin(); itString != rList.end(); ++itString )
			{
				pChoice->Append( wxString::FromUTF8( itString->c_str() ), reinterpret_cast<void*>( nListIndex ) );
				++nListIndex;
			}
		}
		if ( bIndex )
		{
			nSelectedIndex = nIndex;
		}
		if ( nSelectedIndex > static_cast<int>( pChoice->GetCount() ) - 1 )
		{
			nSelectedIndex = 0;
		}
		for ( unsigned nPosition = 0; nPosition < pChoice->GetCount(); ++nPosition )
		{
			if ( IndexAt( *pChoice, nPosition ) == nSelectedIndex )
			{
				pChoice->SetSelection( nPosition );
				break;
			}
		}
	}
}

#endif // OBK2_WITH_WX
