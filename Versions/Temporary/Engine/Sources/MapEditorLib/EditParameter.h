#pragma once

#include "Misc/StrProc.h"

template <class TList, class TControl>
void GetComboBoxEditParameters( TList *pList, int *pIndex, const TControl &rControl, const bool bCount, const bool bIndex )
{
	if ( bCount && ( pList != 0 ) )
	{
		std::vector<std::string> stringList;
		stringList.resize( rControl.GetCount(), std::string() );
		for ( int nControlIndex = 0; nControlIndex < rControl.GetCount(); ++nControlIndex )
		{
			const int nListIndex = rControl.GetItemData( nControlIndex );
			if ( ( nListIndex >= 0 ) && ( nListIndex < stringList.size() ) )
			{
				CString strText;
				rControl.GetLBText( nControlIndex, strText );
				stringList[nListIndex] = std::string( strText );
			}
		}
		pList->clear();
		for ( std::vector<std::string>::const_iterator itString = stringList.begin(); itString != stringList.end(); ++itString )
		{
			pList->push_back( *itString );
		}
	}
	//
	if ( bIndex && ( pIndex != 0 ) )
	{
		const int nControlIndex = rControl.GetCurSel();
		if ( nControlIndex >= 0 )
		{
			( *pIndex ) = rControl.GetItemData( nControlIndex );
		}
		else
		{
			( *pIndex ) = -1;
		}
	}
}


template <class TList, class TControl>
void SetComboBoxEditParameters( const TList &rList, const int nIndex, TControl *pControl, const bool bCount, const bool bIndex )
{
	if ( pControl != 0 )
	{
		if ( bCount || bIndex )
		{
			int nSelectedIndex = pControl->GetCurSel();
			if ( nSelectedIndex >= 0 )
			{
				nSelectedIndex = pControl->GetItemData( nSelectedIndex );
			}
			else
			{
				nSelectedIndex = 0;
			}
			//
			// устанавливаем новый список
			if ( bCount )
			{
				pControl->ResetContent();
				int nListIndex = 0;
				for ( typename TList::const_iterator itString = rList.begin(); itString != rList.end(); ++itString )
				{
					const int nControlIndex = pControl->AddString( itString->c_str() );
					pControl->SetItemData( nControlIndex, nListIndex );
					++nListIndex;	
				}
			}
			// устанавливаем новый index
			if ( bIndex )
			{
				nSelectedIndex = nIndex;
			}
			if ( nSelectedIndex > ( pControl->GetCount() - 1 ) )
			{
				nSelectedIndex = 0;
			}
			for ( int nControlIndex = 0; nControlIndex < pControl->GetCount(); ++nControlIndex )
			{
				const int nListIndex = pControl->GetItemData( nControlIndex );
				if (  nListIndex == nSelectedIndex )
				{
					pControl->SetCurSel( nControlIndex );
					break;
				}
			}
		}
	}
}


template <class TList, class TControl>
void GetListEditParameters( TList *pList, int *pIndex, const TControl &rControl, const bool bCount, const bool bIndex )
{
	if ( bCount && ( pList != 0 ) )
	{
		std::vector<std::string> stringList;
		stringList.resize( rControl.GetItemCount(), std::string() );
		for ( int nControlIndex = 0; nControlIndex < rControl.GetItemCount(); ++nControlIndex )
		{
			const int nListIndex = rControl.GetItemData( nControlIndex );
			if ( ( nListIndex >= 0 ) && ( nListIndex < stringList.size() ) )
			{
				CString strText = rControl.GetItemText( nControlIndex, 0 );
				stringList[nListIndex] = std::string( strText );
			}
		}
		pList->clear();
		for ( std::vector<std::string>::const_iterator itString = stringList.begin(); itString != stringList.end(); ++itString )
		{
			pList->push_back( *itString );
		}
	}
	//
	if ( bIndex && ( pIndex != 0 ) )  
	{
		const int nControlIndex = rControl.GetNextItem( -1, LVNI_SELECTED );
		if ( nControlIndex >= 0 )
		{
			( *pIndex ) = rControl.GetItemData( nControlIndex );
		}
		else
		{
			( *pIndex ) = -1;
		}
	}
}


template <class TList, class TControl>
void SetListEditParameters( const TList &rList, const int nIndex, TControl *pControl, const std::string &rszTypeName, TList *pList, const bool bCount, const bool bIndex )
{
	CWaitCursor waitCursor;
	if ( pControl )
	{
		if ( bCount || bIndex )
		{
			int nSelectedIndex = pControl->GetNextItem( -1, LVNI_SELECTED );
			if ( nSelectedIndex >= 0 )
			{
				nSelectedIndex = pControl->GetItemData( nSelectedIndex );
			}
			else
			{
				nSelectedIndex = 0;
			}
			//
			if ( bCount )
			{
				pControl->DeleteAllItems();
				IObjectCollector::CObjectCollection objectCollection;
				Singleton<IObjectCollector>()->ApplyFilter( &objectCollection, rszTypeName );
				IObjectCollector::CObjectCollection::const_iterator posObjectCollection = objectCollection.find( rszTypeName );
				if ( posObjectCollection != objectCollection.end() )
				{
					int nListCount = 0;
					for ( int nListIndex = 0; nListIndex < rList.size(); ++nListIndex )
					{
						std::string szNameForCompare = rList[nListIndex];
						//NStr::ToLower( &szNameForCompare );
						IObjectCollector::CObjectNameCollection::const_iterator posObjectNameCollection = posObjectCollection->second.find( szNameForCompare );
						if ( posObjectNameCollection != posObjectCollection->second.end() )
						{
							int nControlIndex = pControl->InsertItem( nListCount, posObjectNameCollection->second.strLabel, posObjectNameCollection->second.nIconIndex );
							pControl->SetItemData( nControlIndex, nListIndex );
							++nListCount;
						}
					}
				}
				( *pList ) = rList;
			}
			if ( bIndex )
			{
				nSelectedIndex = nIndex;
			}
			for ( int nControlIndex = 0; nControlIndex < pControl->GetItemCount(); ++nControlIndex )
			{
				const int nListIndex = pControl->GetItemData( nControlIndex );
				if (  nListIndex == nSelectedIndex )
				{
					pControl->SetItemState( nControlIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED );
					pControl->EnsureVisible( nControlIndex, true );
				}
				else
				{
					pControl->SetItemState( nControlIndex, 0, LVIS_FOCUSED | LVIS_SELECTED );
				}
			}	
		}
	}
}


template <class TControl>
int GetSelectedComboBoxIndex( const TControl &rControl )
{
	int nControlIndex = rControl.GetCurSel();
	if ( nControlIndex >= 0 )
	{
		return rControl.GetItemData( nControlIndex );
	}
	else
	{
		return -1;
	}
}


template <class TControl>
int GetSelectedListIndex( const TControl &rControl )
{
	int nControlIndex = rControl.GetNextItem( -1, LVNI_SELECTED );
	if ( nControlIndex >= 0 )
	{
		return rControl.GetItemData( nControlIndex );
	}
	else
	{
		return -1;
	}
}



